#include <Arduino.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include "gps.h"

HardwareSerial GPSSerial(2); // Use UART2 for GPS, can be changed if needed
TinyGPSPlus gps;

void readGPS(void *parameter) {
  GPSSerial.begin(9600, SERIAL_8N1, 16, 17);
  uint32_t lastPrintTime = 0;

  for (;;){
    /*while (GPSSerial.available() > 0) {
            if (gps.encode(GPSSerial.read())) {
                
                if (gps.location.isValid()) {
                    currentGPS.latitude = gps.location.lat();
                    currentGPS.longitude = gps.location.lng();
                    currentGPS.altitude = gps.altitude.meters();
                    currentGPS.speed = gps.speed.kmph();
                    currentGPS.satellites = gps.satellites.value();
                    currentGPS.isValid = true;
                } else {
                    currentGPS.isValid = false;
                }
            }
        }*/
    while (GPSSerial.available() > 0) {
        char c = GPSSerial.read();
        Serial.print(c);
    }
        /*
        //Debug
        if (millis() - lastPrintTime > 1000) {
        if (currentGPS.isValid) {
            Serial.printf("Lat: %.6f, Lon: %.6f, Sat: %d\n", 
                          currentGPS.latitude, 
                          currentGPS.longitude, 
                          currentGPS.satellites);
        } else {
            Serial.println("Đang chờ tín hiệu GPS (No Fix)... Hãy mang mạch ra ngoài trời.");
        }
        lastPrintTime = millis(); // Cập nhật lại mốc thời gian
    }
        */
    vTaskDelay(1000 / portTICK_PERIOD_MS); 
  }
  vTaskDelete(NULL);
}