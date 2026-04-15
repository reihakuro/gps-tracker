#include <Arduino.h>

#include "firebase_manager.h"
#include "gps.h"
#include "gyr.h"
#include "wifi_location.h"
#include "wifi_setup.h"

// Global variables and definitions here:
QueueHandle_t mpuQueue;
QueueHandle_t gpsQueue;

// put function declarations here:

void setup() {
  // put your setup code here, to run once:
  mpuQueue = xQueueCreate(5, sizeof(MPUData));
  gpsQueue = xQueueCreate(5, sizeof(GPSData));

  Serial.begin(115200);  // Uncomment for debugging

  // RTOS tasks creation here:
  xTaskCreatePinnedToCore(readGyro,  // Function that implements the task.
                          "Read Gyroscope",  // Text name for the task.
                          4096,              // Stack size in words, not bytes.
                          NULL,              // Parameter passed into the task.
                          2,     // Priority at which the task is created.
                          NULL,  // Used to pass out the created task's handle.
                          1      // Core where the task should run. 0 or 1.
  );

  xTaskCreatePinnedToCore(
      wifiLocationTask,  // Function that implements the task.
      "WiFi Location",   // Text name for the task.
      12288,             // Stack size in words, not bytes.
      NULL,              // Parameter passed into the task.
      1,                 // Priority at which the task is created.
      NULL,              // Used to pass out the created task's handle.
      1                  // Core where the task should run. 0 or 1.
  );

  xTaskCreatePinnedToCore(wifiSetup,     // Function that implements the task.
                          "WiFi Setup",  // Text name for the task.
                          4096,          // Stack size in words, not bytes.
                          NULL,          // Parameter passed into the task.
                          1,     // Priority at which the task is created.
                          NULL,  // Used to pass out the created task's handle.
                          0      // Core where the task should run. 0 or 1.
  );

  xTaskCreatePinnedToCore(firebaseTask,  // Function that implements the task.
                          "Firebase Manager",  // Text name for the task.
                          24576,  // Stack size in words, not bytes.
                          NULL,   // Parameter passed into the task.
                          1,      // Priority at which the task is created.
                          NULL,   // Used to pass out the created task's handle.
                          1       // Core where the task should run. 0 or 1.
  );
}
void loop() {
  // put your main code here, to run repeatedly:
  MPUData receivedData;

  vTaskDelete(NULL);
}

// put function definitions here:
