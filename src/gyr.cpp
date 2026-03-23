#include <Arduino.h>
#include <Wire.h>

#define SDA 21
#define SCL 22

void readGyro(void *parameter) {
  for (;;){
    // read gyro data

    vTaskDelay(1000 / portTICK_PERIOD_MS); 
  }
  vTaskDelete(NULL);
}