#include <Arduino.h>
#include <WiFi.h>
#include "wifi_setup.h"

const char* ssid = "my_wifi_ssid";
const char* password = "123456789";

void wifiSetup(void *parameter) {

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS); 
  }
  vTaskDelete(NULL); 
}