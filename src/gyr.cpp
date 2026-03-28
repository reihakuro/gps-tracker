#include "gyr.h"

#include <Arduino.h>
#include <Wire.h>

#define SDA 21
// default SDA pin for ESP32, can be changed in Wire.begin() if needed
#define SCL 22
// default SCL pin for ESP32, can be changed in Wire.begin() if needed

const int MPU_ADDR = 0x68;
// do not change this address, it is fixed for the MPU-6050
int16_t accelX, accelY, accelZ;
int16_t gyroX, gyroY, gyroZ;
int16_t tempRaw;

float gForceX, gForceY, gForceZ;
float rotX, rotY, rotZ;

float convertRawGyroToDps(int16_t rawValue) { return (float)rawValue / 131.0; }

float convertRawAccelToG(int16_t rawValue) { return (float)rawValue / 16384.0; }

float applyDeadband(float value, float threshold) {
  if (value > -threshold && value < threshold) {
    return 0.0;
  }
  return value;
}

void readGyro(void* parameter) {
  Wire.begin();
  // Wire.setClock(400000);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);  // Power management register
  Wire.write(0x00);  // Wake up the MPU-6050
  Wire.endTransmission(true);
  MPUData currentData;
  for (;;) {
    // Open a connection to the MPU-6050 and request data
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);

    Wire.requestFrom(MPU_ADDR, 14, true);

    if (Wire.available() == 14) {
      accelX = Wire.read() << 8 | Wire.read();
      accelY = Wire.read() << 8 | Wire.read();
      accelZ = Wire.read() << 8 | Wire.read();

      tempRaw = Wire.read() << 8 | Wire.read();

      gyroX = Wire.read() << 8 | Wire.read();
      gyroY = Wire.read() << 8 | Wire.read();
      gyroZ = Wire.read() << 8 | Wire.read();
      // accelX, accelY, accelZ are in raw values, need to convert to g's
      gForceX = convertRawAccelToG(accelX);
      gForceY = convertRawAccelToG(accelY);
      gForceZ = convertRawAccelToG(accelZ);
      // gyroX, gyroY, gyroZ are in raw values, need to convert to °/s
      float rawRotX = convertRawGyroToDps(gyroX);
      float rawRotY = convertRawGyroToDps(gyroY);
      float rawRotZ = convertRawGyroToDps(gyroZ);

      rotX = applyDeadband(rawRotX, 1.5);
      rotY = applyDeadband(rawRotY, 1.5);
      rotZ = applyDeadband(rawRotZ, 1.5);
      // tempRaw is in raw value, need to convert to °C
      // float temp = (tempRaw / 340.0) + 36.53;
      // Send the data to the queue
      currentData.gForceX = gForceX;
      currentData.gForceY = gForceY;
      currentData.gForceZ = gForceZ;
      currentData.rotX = rotX;
      currentData.rotY = rotY;
      currentData.rotZ = rotZ;

      // Serial.printf("MPU6050 -> Accel (g): X=%.2f, Y=%.2f, Z=%.2f | Gyro
      // (°/s): X=%.2f, Y=%.2f, Z=%.2f\n",
      //               gForceX, gForceY, gForceZ, rotX, rotY, rotZ);

      xQueueSend(mpuQueue, &currentData, 0);
    } else {
      // Uncomment for debugging, can be removed
      // Serial.println(
      //    "No Data from MPU-6050! Check connections and try again.");
    };  // handle error if needed
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}
