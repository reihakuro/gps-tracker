#include <Arduino.h>

void readGPS(void *parameter) {
  for (;;){
    // read GPS data

    vTaskDelay(1000 / portTICK_PERIOD_MS); 
  }
  vTaskDelete(NULL); 
}