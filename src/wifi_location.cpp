#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <algorithm>
#include <vector>

#include "gps.h"
#include "key.h"

String lastWiFiSignature = "";

String formatMac(String mac) {
  mac.replace(":", "");
  mac.toUpperCase();
  return mac;
}

void wifiLocationTask(void* parameter) {
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  vTaskDelay(5000 / portTICK_PERIOD_MS);

  bool isFirstRun = true;

  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      int n = WiFi.scanNetworks();

      if (n > 0) {
        // Tạo signature để kiểm tra di chuyển
        std::vector<String> macList;
        for (int i = 0; i < ((n > 5) ? 5 : n); i++) macList.push_back(WiFi.BSSIDstr(i));
        std::sort(macList.begin(), macList.end());
        
        String currentSignature = "";
        for (String mac : macList) currentSignature += mac;

        // KIỂM TRA DI CHUYỂN
        if (isFirstRun || currentSignature != lastWiFiSignature) {
          lastWiFiSignature = currentSignature;
          isFirstRun = false;

          Serial.println("\n[Wi-Fi] Environment changed. Calling HERE API...");

          DynamicJsonDocument doc(2048);
          JsonArray wlan = doc.createNestedArray("wlan");

          int max_ap_for_api = (n > 10) ? 10 : n;
          for (int i = 0; i < max_ap_for_api; i++) {
            JsonObject ap = wlan.createNestedObject();
            
            // KEY CHUẨN: "mac" (có dấu :) và "rss" (tín hiệu)
            ap["mac"] = WiFi.BSSIDstr(i); 
            ap["rss"] = WiFi.RSSI(i); 
          }

          String requestBody;
          serializeJson(doc, requestBody);

          Serial.println("--- FINAL JSON TO HERE ---");
          Serial.println(requestBody); 

          WiFiClientSecure client;
          client.setInsecure();
          HTTPClient http;
          
          String url = "https://positioning.hereapi.com/v2/locate?apiKey=" + String(HERE_API_KEY);
          
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
              
              xQueueSend(gpsQueue, &currentGPS, 0);
              Serial.printf("HERE | OK! Lat: %.6f, Lng: %.6f\n", currentGPS.latitude, currentGPS.longitude);
            } else {
              Serial.printf("HERE | Error %d\n", httpCode);
              Serial.println("Details: " + http.getString());
            }
            http.end();
          }
        } else {
          Serial.println("HERE | Static - No API call needed.");
        }
      }
      WiFi.scanDelete();
    }
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}
  
