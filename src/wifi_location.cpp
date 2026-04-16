// ----------------------------------wifi_location---------------------------------
// This file is responsible for determining the device's location using nearby
// Wi-Fi access points and the HERE Location Services API. It runs as a FreeRTOS
// task on the ESP32, continuously monitoring the Wi-Fi environment for changes.
// When a change is detected (e.g., new access points appear, or signal
// strengths change significantly), it sends a request to the HERE API with the
// current Wi-Fi data to get an updated location estimate. The task also
// implements a caching mechanism to avoid unnecessary API calls when the Wi-Fi
// environment remains static, thus optimizing power consumption and API usage.
// --------------------------------------------------------------------------------
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <math.h>

#include <algorithm>
#include <vector>

#include "gps.h"
#include "key.h"

std::vector<String> lastMacList;

int changeCounter = 0;
const int CHANGE_THRESHOLD = 2; // 2 change detected before calling API
const int MIN_MATCH = 3;        // Match check
const int RSSI_THRESHOLD = -85; // Weak signal threshold

// Các hằng số cho bộ lọc vận tốc
const float MIN_DISTANCE_M = 30.0; // Dưới 30m coi như đứng im (lọc nhiễu tọa độ)
const float MAX_SPEED_KMH = 150.0; // Tốc độ tối đa hợp lý để chống văng tọa độ

String formatMac(String mac) {
  mac.replace(":", "");
  mac.toUpperCase();
  return mac;
}

// Hàm tính khoảng cách giữa 2 tọa độ (mét) bằng công thức Haversine
float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
  float R = 6371000; // Bán kính Trái Đất (mét)
  float phi1 = lat1 * M_PI / 180;
  float phi2 = lat2 * M_PI / 180;
  float dPhi = (lat2 - lat1) * M_PI / 180;
  float dLambda = (lon2 - lon1) * M_PI / 180;

  float a = sin(dPhi / 2) * sin(dPhi / 2) +
            cos(phi1) * cos(phi2) *
            sin(dLambda / 2) * sin(dLambda / 2);
  float c = 2 * atan2(sqrt(a), sqrt(1 - a));

  return R * c; 
}

void wifiLocationTask(void *parameter) {
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  vTaskDelay(5000 / portTICK_PERIOD_MS);

  bool isFirstRun = true;
  
  // Biến lưu trữ trạng thái cho việc tính vận tốc
  static float lastLat = 0.0;
  static float lastLng = 0.0;
  static unsigned long lastTime = 0;
  static float lastValidSpeed = 0.0;

  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      int n = WiFi.scanNetworks();

      if (n > 0) {
        std::vector<String> currentMacList;

        // Filter APs based on RSSI and build current MAC list
        for (int i = 0; i < n; i++) {
          if (WiFi.RSSI(i) < RSSI_THRESHOLD)
            continue;

          currentMacList.push_back(WiFi.BSSIDstr(i));
          if (currentMacList.size() >= 5)
            break;
        }

        // Counting matches between current and last MAC lists
        int matchCount = 0;
        for (const String &mac : currentMacList) {
          if (std::find(lastMacList.begin(), lastMacList.end(), mac) !=
              lastMacList.end()) {
            matchCount++;
          }
        }

        // 3. Is really changes?
        bool isChanged = (matchCount < MIN_MATCH) && (!lastMacList.empty());

        if (isFirstRun || isChanged) {
          changeCounter++;

          // 4. Debounce changes: Only call API if we see consistent changes
          if (isFirstRun || changeCounter >= CHANGE_THRESHOLD) {
            Serial.println(
                "\n[Wi-Fi] Environment changed. Calling HERE API...");

            lastMacList = currentMacList;
            changeCounter = 0;
            isFirstRun = false;

            // JSON payload for HERE API
            DynamicJsonDocument doc(2048);
            JsonArray wlan = doc.createNestedArray("wlan");

            // Limit the number of APs sent to HERE API to 10
            int max_ap_for_api = (n > 10) ? 10 : n;
            for (int i = 0; i < max_ap_for_api; i++) {
              JsonObject ap = wlan.createNestedObject();
              ap["mac"] = WiFi.BSSIDstr(i);
              ap["rss"] = WiFi.RSSI(i);
            }

            String requestBody;
            serializeJson(doc, requestBody);

            Serial.println("--- FINAL JSON TO HERE ---");
            Serial.println(requestBody);

            WiFiClientSecure client;
            client.setInsecure(); // bypass SSL cert
            HTTPClient http;

            String url = "https://positioning.hereapi.com/v2/locate?apiKey=" +
                         String(HERE_API_KEY);

            if (http.begin(client, url)) {
              http.addHeader("Content-Type", "application/json");
              int httpCode = http.POST(requestBody);

              if (httpCode == 200) {
                DynamicJsonDocument res(1024);
                deserializeJson(res, http.getString());

                GPSData currentGPS;
                currentGPS.latitude = res["location"]["lat"];
                currentGPS.longitude = res["location"]["lng"];
                currentGPS.isValid = true;

                // --- TÍNH TOÁN VẬN TỐC (CÓ BỘ LỌC) ---
                unsigned long currentTime = millis();

                if (lastLat != 0.0 && lastLng != 0.0 && lastTime != 0) {
                  float distanceMeters = calculateDistance(lastLat, lastLng, currentGPS.latitude, currentGPS.longitude);
                  float timeElapsedSeconds = (currentTime - lastTime) / 1000.0;
                  
                  if (timeElapsedSeconds > 0) {
                    if (distanceMeters < MIN_DISTANCE_M) {
                        // Lọc nhiễu: Di chuyển quá ít -> Đứng yên
                        currentGPS.speed = 0.0;
                        lastValidSpeed = 0.0;
                    } 
                    else {
                        float rawSpeedMPS = distanceMeters / timeElapsedSeconds; 
                        float rawSpeedKMH = rawSpeedMPS * 3.6; 
                        
                        if (rawSpeedKMH > MAX_SPEED_KMH) {
                            // Lọc nhiễu: Văng tọa độ -> Giữ tốc độ cũ
                            Serial.println("[WARNING] Văng tọa độ do nhiễu Wi-Fi. Bỏ qua vận tốc này.");
                            currentGPS.speed = lastValidSpeed; 
                        } else {
                            currentGPS.speed = rawSpeedKMH;
                            lastValidSpeed = rawSpeedKMH;
                        }
                    }
                  } else {
                    currentGPS.speed = 0.0;
                  }
                } else {
                  currentGPS.speed = 0.0; // Lần đầu lấy mẫu
                }

                // Lưu lại trạng thái cho lần gọi API tiếp theo
                lastLat = currentGPS.latitude;
                lastLng = currentGPS.longitude;
                lastTime = currentTime;
                // ----------------------------------------

                // Queue GPS data to be sent to Firebase
                xQueueSend(gpsQueue, &currentGPS, 0);
                Serial.printf("HERE | OK! Lat: %.6f, Lng: %.6f, Speed: %.2f km/h\n",
                              currentGPS.latitude, currentGPS.longitude, currentGPS.speed);
              } else {
                Serial.printf("HERE | Error %d\n", httpCode);
                Serial.println("Details: " + http.getString());
              }
              http.end();
            }
            client.stop();
            // End of API call

          } else {
            Serial.printf(
                "HERE | Change detected (%d/%d). Waiting for confirmation...\n",
                changeCounter, CHANGE_THRESHOLD);
          }
        } else {
          // No significant change detected, reset counter and skip API call
          changeCounter = 0;
          Serial.println("HERE | Static - No API call needed.");
        }
      }
      WiFi.scanDelete();
    }
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}
