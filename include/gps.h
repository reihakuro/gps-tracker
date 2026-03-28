#ifndef GPS_H
#define GPS_H
#include <Arduino.h>
struct GPSData {
  double latitude;
  double longitude;
  double altitude;
  float speed;
  int satellites;
  bool isValid;
};
extern QueueHandle_t gpsQueue;
void readGPS(void* parameter);
#endif