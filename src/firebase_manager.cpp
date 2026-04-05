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

  MPUData mpuData;
  GPSData gpsData;

  for (;;) {
    if (Firebase.ready()) {
      FirebaseJson json;
      bool hasData = false;

      if (xQueueReceive(mpuQueue, &mpuData, 0) == pdPASS) {
        hasData = true;
      }
      if (hasData) {
        json.set("mpu/gForceX", mpuData.gForceX);
        json.set("mpu/gForceY", mpuData.gForceY);
        json.set("mpu/gForceZ", mpuData.gForceZ);
        json.set("mpu/rotX", mpuData.rotX);
        json.set("mpu/rotY", mpuData.rotY);
        json.set("mpu/rotZ", mpuData.rotZ);
      }
      if (xQueueReceive(gpsQueue, &gpsData, 0) == pdPASS) {
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

      if (Firebase.RTDB.getBool(&fbdo_read, "/tracker/action/ring")) {
        bool shouldRing = fbdo_read.boolData();

        if (shouldRing) {
          ringBuzzer();
          Firebase.RTDB.setBool(&fbdo_write, "/tracker/action/ring", false);
        }
      }
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}