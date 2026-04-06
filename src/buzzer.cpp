#include "buzzer.h"

#include <Arduino.h>

#define BUZZER_CHANNEL 0 //
#define BUZZER_FREQ 2000 //
#define BUZZER_RESOLUTION 8

void setupBuzzer() {
  ledcSetup(BUZZER_CHANNEL, BUZZER_FREQ, BUZZER_RESOLUTION);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
  ledcWrite(BUZZER_CHANNEL, 0);
}

void ringBuzzer() {
  Serial.println("[Buzzer] Ringing");

  for (int i = 0; i < 3; i++) {
    ledcWrite(BUZZER_CHANNEL, 128);
    vTaskDelay(200 / portTICK_PERIOD_MS);

    ledcWrite(BUZZER_CHANNEL, 0);
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }

  vTaskDelay(2000 / portTICK_PERIOD_MS);
}