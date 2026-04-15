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
    Serial.println("Timeout");
  }
}

bool initFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.signer.test_mode = true;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (!Firebase.RTDB.beginStream(&fbdo_read, "/tracker/action/ring")) {
    return false;
  }

  Firebase.RTDB.setStreamCallback(&fbdo_read, streamCallback,
                                  streamTimeoutCallback);
  return true;
}

void processAndUploadSensorData() {
  FirebaseJson json;
  bool hasData = false;
  MPUData mpuData;
  GPSData gpsData;

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

  bool gpsUpdated = false;
  if (xQueueReceive(gpsQueue, &gpsData, 0) == pdPASS) {
    gpsUpdated = true;
  }

  if (gpsUpdated && gpsData.isValid) {
    hasData = true;
    json.set("gps/lat", gpsData.latitude);
    json.set("gps/lng", gpsData.longitude);
    json.set("gps/speed", gpsData.speed);
    json.set("gps/sats", gpsData.satellites);
  }

  if (hasData) {
    if (Firebase.RTDB.updateNode(&fbdo_write, "/tracker/live", &json)) {
      Serial.println("Uploaded.");
    } else {
      Serial.printf("Firebase Error: %s\n", fbdo_write.errorReason().c_str());
    }
  }
}

void handleBuzzerAction() {
  if (triggerBuzzer) {
    ringBuzzer();
    triggerBuzzer = false;
  }
}

void firebaseTask(void *parameter) {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  setupBuzzer();
  initFirebase();

  unsigned long lastUploadTime = 0;
  const unsigned long uploadInterval = 3000;

  for (;;) {
    if (Firebase.ready()) {
      handleBuzzerAction();

      if (millis() - lastUploadTime >= uploadInterval) {
        lastUploadTime = millis();
        processAndUploadSensorData();
      }
    }
    // Serial.printf("[DEBUG-MEM] Free Heap: %d\n", ESP.getFreeHeap());

    vTaskDelay(1500 / portTICK_PERIOD_MS);
  }
}
