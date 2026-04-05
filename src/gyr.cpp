// gyr.cpp - Task to read data from MPU-6050 gyroscope and accelerometer
//----------------------------------------------------------------------------------
// This file is part of the ESP32 GPS Tracker project.
// This file implements the readGyro (main) task, which continuously reads data
// from the MPU-6050 sensor and sends it to a queue for processing by other
// tasks. The task is designed to run on an ESP32 microcontroller using FreeRTOS
// for multitasking.
//----------------------------------------------------------------------------------
// The MPU-6050 is configured to trigger an interrupt when motion is detected,
// allowing for efficient data collection without constant polling.
// The task also includes calibration of the gyroscope to account for any
// offsets, and applies a deadband to filter out small, insignificant movements.
//-----------------------------------------------------------------------------------
// The data collected includes acceleration in g's and rotation in degrees per
// second, which can be used for various applications such as motion tracking,
// gesture recognition, or as part of a GPS tracking system to provide
// additional context about the movement of the device.

#include "gyr.h"

#include <Arduino.h>
#include <Wire.h>

#define SDA 21
// default SDA pin for ESP32, can be changed in Wire.begin() if needed
#define SCL 22
// default SCL pin for ESP32, can be changed in Wire.begin() if needed
#define INT 19

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

void IRAM_ATTR mpuInterruptHandler() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(gyroTaskHandle, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

void setupMotionInterrupt() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C);  // ACCEL_CONFIG
  Wire.write(0x01);  // Set accelerometer to ±4g range (00: ±2g, 01: ±4g, 10:
                     // ±8g, 11: ±16g)
  Wire.endTransmission();

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1F);  // ACCEL_INTEL_CTRL
  Wire.write(20);    // Set motion detection threshold (20 * 4mg = 80mg)
  Wire.endTransmission();

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x20);  // ACCEL_WOM_THR
  Wire.write(1);     // Set motion detection duration (1 * 1ms = 1ms)
  Wire.endTransmission();

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x69);  // INT_PIN_CFG
  Wire.write(0x15);  // Configure interrupt pin: active high, push-pull, latched
                     // until cleared
  Wire.endTransmission();

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x38);  // INT_ENABLE
  Wire.write(0x40);  // Enable motion detection interrupt
  Wire.endTransmission();
}

void readGyro(void* parameter) {
  gyroTaskHandle = xTaskGetCurrentTaskHandle();
  pinMode(INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INT), mpuInterruptHandler, RISING);

  Wire.begin();
  // Wire.setClock(400000);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);  // Power management register
  Wire.write(0x00);  // Wake up the MPU-6050
  Wire.endTransmission(true);
  calibrateGyro();
  setupMotionInterrupt();
  MPUData currentData;
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    // Open a connection to the MPU-6050 and request data
    Serial.println("Motion Detected! Reading data...");
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
      float rawRotX = convertRawGyroToDps(gyroX - gyroOffsetX);
      float rawRotY = convertRawGyroToDps(gyroY - gyroOffsetY);
      float rawRotZ = convertRawGyroToDps(gyroZ - gyroOffsetZ);

      rotX = applyDeadband(rawRotX, 1.0);
      rotY = applyDeadband(rawRotY, 1.0);
      rotZ = applyDeadband(rawRotZ, 1.0);
      // tempRaw is in raw value, need to convert to °C
      // float temp = (tempRaw / 340.0) + 36.53;

      // Send the data to the queue
      currentData.gForceX = gForceX;
      currentData.gForceY = gForceY;
      currentData.gForceZ = gForceZ;
      currentData.rotX = rotX;
      currentData.rotY = rotY;
      currentData.rotZ = rotZ;

      Serial.printf(
          "MPU6050 -> Accel (g): X=%.2f, Y=%.2f, Z=%.2f | Gyro\
      (°/s): X=%.2f, Y=%.2f, Z=%.2f\n",
          gForceX, gForceY, gForceZ, rotX, rotY, rotZ);

      xQueueSend(mpuQueue, &currentData, 0);
    } else {
      // Uncomment for debugging, can be removed
      Serial.println("No Data from MPU-6050! Check connections and try again.");
    }  // handle error if needed
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3A);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 1, true);
    Wire.read();

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}
