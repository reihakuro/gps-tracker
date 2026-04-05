#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "wifi_setup.h"

TaskHandle_t wifiTaskHandle = NULL;

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (wifiTaskHandle != NULL) {
    xTaskNotifyGive(wifiTaskHandle);
  }
}

void wifiSetup(void *parameter) {
  wifiTaskHandle = xTaskGetCurrentTaskHandle();
  WiFiManager wm;
  wm.setConfigPortalTimeout(
      180); // Timeout for WiFi configuration portal (in seconds)
  void wifiSetup(void *parameter) {
    wifiTaskHandle = xTaskGetCurrentTaskHandle();
    WiFiManager wm;
    wm.setConfigPortalTimeout(180);

    Serial.println("WiFi setup starting via WiFiManager...");
    bool res = wm.autoConnect("GPS_TRACKER", "12345678");
    if (!res) {
      Serial.println("No input received, restarting...");
      delay(3000);
      ESP.restart();
    }
    WiFi.onEvent(WiFiStationDisconnected,
                 WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    Serial.println("Wifi connected successfully!");

    for (;;) {
      if (WiFi.status() == WL_CONNECTED) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // Serial.println("Reconnecting to WiFi...");
      } else {
        // Serial.println("Finding WiFi...");
        WiFi.reconnect();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
      }
    }
  }