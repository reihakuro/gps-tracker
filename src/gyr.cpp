// gyr.cpp - Task to read data from MPU-6050 gyroscope and accelerometer
//----------------------------------------------------------------------------------
// This file is part of the ESP32 GPS Tracker project.
// This file implements the readGyro (main) task, which continuously reads data
// from the MPU-6050 sensor and sends it to a queue for processing by other
// tasks. The task is designed to run on an ESP32 microcontroller using FreeRTOS
// for multitasking.
//----------------------------------------------------------------------------------

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

float gyroOffsetX = 0, gyroOffsetY = 0, gyroOffsetZ = 0;

TaskHandle_t gyroTaskHandle = NULL;

float convertRawGyroToDps(int16_t rawValue) { return (float)rawValue / 131.0; }

float convertRawAccelToG(int16_t rawValue) { return (float)rawValue / 16384.0; }

float applyDeadband(float value, float threshold) {
  if (value > -threshold && value < threshold) {
    return 0.0;
  }
  return value;
}

void calibrateGyro() {
  long sumX = 0, sumY = 0, sumZ = 0;
  int numSamples = 200;

  for (int i = 0; i < numSamples; i++) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x43);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 6, true);

    if (Wire.available() == 6) {
      sumX += (Wire.read() << 8 | Wire.read());
      sumY += (Wire.read() << 8 | Wire.read());
      sumZ += (Wire.read() << 8 | Wire.read());
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }

  gyroOffsetX = (float)sumX / numSamples;
  gyroOffsetY = (float)sumY / numSamples;
  gyroOffsetZ = (float)sumZ / numSamples;
}

void readGyro(void *parameter) {
  gyroTaskHandle = xTaskGetCurrentTaskHandle();

  Wire.begin();
  // Wire.setClock(400000);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // Power management register
  Wire.write(0x00); // Wake up the MPU-6050
  Wire.endTransmission(true);

  calibrateGyro();

  MPUData currentData;
  for (;;) {
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

      gForceX = convertRawAccelToG(accelX);
      gForceY = convertRawAccelToG(accelY);
      gForceZ = convertRawAccelToG(accelZ);

      float rawRotX = convertRawGyroToDps(gyroX - gyroOffsetX);
      float rawRotY = convertRawGyroToDps(gyroY - gyroOffsetY);
      float rawRotZ = convertRawGyroToDps(gyroZ - gyroOffsetZ);

      rotX = applyDeadband(rawRotX, 1.0);
      rotY = applyDeadband(rawRotY, 1.0);
      rotZ = applyDeadband(rawRotZ, 1.0);

      currentData.gForceX = gForceX;
      currentData.gForceY = gForceY;
      currentData.gForceZ = gForceZ;
      currentData.rotX = rotX;
      currentData.rotY = rotY;
      currentData.rotZ = rotZ;

      // Serial.printf(
      //     "MPU6050 -> Accel (g): X=%.2f, Y=%.2f, Z=%.2f | Gyro (°/s): X=%.2f,
      //     "
      //    "Y=%.2f, Z=%.2f\n",
      //   gForceX, gForceY, gForceZ, rotX, rotY, rotZ);

      xQueueSend(mpuQueue, &currentData, 0);
    } else {
      Serial.println("No Data from MPU-6050! Check connections and try again.");
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}