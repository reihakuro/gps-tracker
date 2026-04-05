#include "firebase_manager.h"

#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <WiFi.h>

#include "addons/RTDBHelper.h"
#include "addons/TokenHelper.h"
#include "gps.h"
#include "gyr.h"
#include "key.h"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

void firebaseTask(void* parameter) {
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

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
        json.set("mpu/accel/x", mpuData.gForceX);
        json.set("mpu/accel/y", mpuData.gForceY);
        json.set("mpu/accel/z", mpuData.gForceZ);
        json.set("mpu/gyro/x", mpuData.rotX);
        json.set("mpu/gyro/y", mpuData.rotY);
        json.set("mpu/gyro/z", mpuData.rotZ);
        hasData = true;
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
        if (Firebase.RTDB.updateNode(&fbdo, "/tracker/live", &json)) {
          Serial.println("Firebase update successful");
        } else {
          Serial.printf("Firebase error: %s\n", fbdo.errorReason().c_str());
        }
      }
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}