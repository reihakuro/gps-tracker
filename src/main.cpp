#include <Arduino.h>
#include "wifi_setup.h"
#include "gps.h"
// put function declarations here:



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
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
  vTaskDelete(NULL);
}

// put function definitions here:
