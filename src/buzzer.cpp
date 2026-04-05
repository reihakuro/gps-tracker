#include "buzzer.h"

void setupBuzzer() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void ringBuzzer() {
  Serial.println("[Buzzer]Ringing");

  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    digitalWrite(BUZZER_PIN, LOW);
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}