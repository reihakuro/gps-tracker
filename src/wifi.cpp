#include <Arduino.h>
#include <WiFi.h>
#include "wifi_setup.h"
#include "key.h"



void wifiSetup(void *parameter) {
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS); 
  }
  vTaskDelete(NULL); 
}