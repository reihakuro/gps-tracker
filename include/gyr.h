#ifndef GYR_H
#define GYR_H
#include <Arduino.h>

struct MPUData {
  float gForceX;
  float gForceY;
  float gForceZ;
  float rotX;
  float rotY;
  float rotZ;
};

extern QueueHandle_t mpuQueue;

void readGyro(void *parameter);
#endif