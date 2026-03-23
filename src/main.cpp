#include <Arduino.h>
#include "wifi_setup.h"
#include "gps.h"
#include "gyr.h"

QueueHandle_t mpuQueue;
// put function declarations here:

void setup() {
  // put your setup code here, to run once:
  mpuQueue = xQueueCreate(5, sizeof(MPUData));
  if (mpuQueue == NULL) {
        Serial.println("Loi tao Queue!");
  }
  // Serial.begin(115200);

  // RTOS tasks creation here:
  xTaskCreatePinnedToCore(
    readGyro,          // Function that implements the task.
    "Read Gyroscope",  // Text name for the task.
    10000,            // Stack size in words, not bytes.
    NULL,             // Parameter passed into the task.
    1,                // Priority at which the task is created.
    NULL,             // Used to pass out the created task's handle.
    1                // Core where the task should run. 0 or 1.
  );

  xTaskCreatePinnedToCore(
    readGPS,          // Function that implements the task.
    "Read GPS",  // Text name for the task.
    10000,            // Stack size in words, not bytes.
    NULL,             // Parameter passed into the task.
    1,                // Priority at which the task is created.
    NULL,             // Used to pass out the created task's handle.
    1                // Core where the task should run. 0 or 1.
  );

  xTaskCreatePinnedToCore(
    wifiSetup,        // Function that implements the task.
    "WiFi Setup",  // Text name for the task.
    10000,            // Stack size in words, not bytes.
    NULL,             // Parameter passed into the task.
    1,                // Priority at which the task is created.
    NULL,             // Used to pass out the created task's handle.
    0               // Core where the task should run. 0 or 1.
  );

}
void loop() {
  // put your main code here, to run repeatedly:
  MPUData receivedData;
  
  vTaskDelete(NULL);
}

// put function definitions here:
