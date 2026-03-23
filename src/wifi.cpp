#include <Arduino.h>
#include <WiFi.h>
#include "wifi_setup.h"


// Điền thông tin mạng WiFi của bạn vào đây
const char* ssid = "my_wifi_ssid";
const char* password = "123456789";

// Định nghĩa nội dung thực thi của Task
void wifiSetup(void *parameter) {

  WiFi.begin(ssid, password);

  // Vòng lặp chờ kết nối
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS); 
  }
  vTaskDelete(NULL); 
}