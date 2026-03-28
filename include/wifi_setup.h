#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H
#include <Arduino.h>
#include <WiFi.h>

extern TaskHandle_t wifiTaskHandle;

void wifiSetup(void* parameter);
#endif