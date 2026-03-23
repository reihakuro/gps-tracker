#ifndef GYR_H
#define GYR_H
#include <Arduino.h>

extern float gForceX, gForceY, gForceZ;
extern float rotX, rotY, rotZ;

void readGyro(void *parameter);
#endif