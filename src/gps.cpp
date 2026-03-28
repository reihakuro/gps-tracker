#include "gps.h"

#include <Arduino.h>
#include <HardwareSerial.h>
#include <TinyGPS++.h>

HardwareSerial GPSSerial(2);  // Use UART2 for GPS, can be changed if needed
TinyGPSPlus gps;

void readGPS(void* parameter) {
  GPSSerial.begin(9600, SERIAL_8N1, 16, 17);

  GPSData currentGPS;
  // uint32_t lastPrintTime = 0; // this parameter is for debugging, can be
  // removed

  for (;;) {
    while (GPSSerial.available() > 0) {
      if (gps.encode(GPSSerial.read())) {
        if (gps.location.isValid()) {
          currentGPS.latitude = gps.location.lat();
          currentGPS.longitude = gps.location.lng();
          currentGPS.altitude = gps.altitude.meters();
          currentGPS.speed = gps.speed.kmph();
          currentGPS.satellites = gps.satellites.value();
          currentGPS.isValid = true;

          xQueueSend(gpsQueue, &currentGPS, 0);
        } else {
          currentGPS.isValid = false;
        }
      }
    }

    // This is a debugging block to print raw GPS data, can be removed

    // while (GPSSerial.available() > 0) {
    //     char c = GPSSerial.read();
    //     Serial.print(c);
    // }

    /*
    //
    if (millis() - lastPrintTime > 1000) {
    if (currentGPS.isValid) {
        Serial.printf("Lat: %.6f, Lon: %.6f, Sat: %d\n",
                      currentGPS.latitude,
                      currentGPS.longitude,
                      currentGPS.satellites);
    } else {
        Serial.println("No signal.");
    }
    lastPrintTime = millis();

}
    */
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}