#include "firebase_manager.h"

#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <WiFi.h>

#include "addons/RTDBHelper.h"
#include "addons/TokenHelper.h"
#include "buzzer.h"
#include "gps.h"
#include "gyr.h"
#include "key.h"

FirebaseData fbdo_write;
FirebaseData fbdo_read;
FirebaseAuth auth;
FirebaseConfig config;

volatile bool triggerBuzzer = false;

void streamCallback(FirebaseStream data) {
  if (data.dataType() == "boolean") {
    bool shouldRing = data.boolData();
    if (shouldRing) {
      triggerBuzzer = true;
    }
  }
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    Serial.println("Firebase stream timeout, resuming...");
  }
}

void firebaseTask(void* parameter) {
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  setupBuzzer();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.signer.test_mode = true;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (!Firebase.RTDB.beginStream(&fbdo_read, "/tracker/action/ring")) {
    Serial.printf("Stream begin error, %s\n", fbdo_read.errorReason().c_str());
  }
  Firebase.RTDB.setStreamCallback(&fbdo_read, streamCallback,
                                  streamTimeoutCallback);

  MPUData mpuData;
  GPSData gpsData;

  unsigned long lastUploadTime = 0;
  const unsigned long uploadInterval = 3000;

  for (;;) {
    if (Firebase.ready()) {
      // Check if we need to ring the buzzer
      if (triggerBuzzer) {
        ringBuzzer();  // Ring the buzzer
        Firebase.RTDB.setBool(&fbdo_write, "/tracker/action/ring", false);
        triggerBuzzer = false;
      }

      if (millis() - lastUploadTime >= uploadInterval) {
        lastUploadTime = millis();

        FirebaseJson json;
        bool hasData = false;
        // Check for new MPU data in the queue
        bool mpuUpdated = false;
        while (xQueueReceive(mpuQueue, &mpuData, 0) == pdPASS) {
          mpuUpdated = true;
        }

        if (mpuUpdated) {
          hasData = true;
          json.set("mpu/gForceX", mpuData.gForceX);
          json.set("mpu/gForceY", mpuData.gForceY);
          json.set("mpu/gForceZ", mpuData.gForceZ);
          json.set("mpu/rotX", mpuData.rotX);
          json.set("mpu/rotY", mpuData.rotY);
          json.set("mpu/rotZ", mpuData.rotZ);
        }
        // Check for new GPS data in the queue

        bool gpsUpdated = false;
        if (xQueueReceive(gpsQueue, &gpsData, 0) == pdPASS) {
          gpsUpdated = true;
        }

        if (gpsUpdated) {
          if (gpsData.isValid) {
            json.set("gps/lat", gpsData.latitude);
            json.set("gps/lng", gpsData.longitude);
            json.set("gps/speed", gpsData.speed);
            json.set("gps/sats", gpsData.satellites);
            hasData = true;
          }
        }

        if (hasData) {
          if (Firebase.RTDB.updateNode(&fbdo_write, "/tracker/live", &json)) {
            Serial.println("Firebase update successful");
          } else {
            Serial.printf("Firebase error: %s\n",
                          fbdo_write.errorReason().c_str());
          }
        }
      }
    }
    // Serial.printf("Free Heap: %d\n", ESP.getFreeHeap()); //debug memory leak
    vTaskDelay(1500 / portTICK_PERIOD_MS);
  }
}