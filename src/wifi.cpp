#include <Arduino.h>
#include <WiFi.h>

#include "key.h"
#include "wifi_setup.h"

TaskHandle_t wifiTaskHandle = NULL;

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (wifiTaskHandle != NULL) {
    xTaskNotifyGive(wifiTaskHandle);
  }
}

void wifiSetup(void* parameter) {
  WiFi.onEvent(WiFiStationDisconnected,
               WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  WiFi.begin(SSID, PASSWORD);
  Serial.println("Dang khoi dong WiFi...");

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