#include <Arduino.h>
#include "buzzer.h"

#define BUZZER_PIN 4

void buzzerInit() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void bleConnectedBeep() {
    tone(BUZZER_PIN, 1500, 250);
    tone(BUZZER_PIN, 2000, 250);
    tone(BUZZER_PIN, 2500, 250);
}

void automationEnabledBeep() {
    tone(BUZZER_PIN, 1000, 100);
    tone(BUZZER_PIN, 3000, 100);
}

void automationDisabledBeep() {
    tone(BUZZER_PIN, 3000, 100);
    tone(BUZZER_PIN, 1000, 100);
}

unsigned long lastBeepTime = 0;
const int beepInterval = 1000;

void otaBeep() {
  if (millis() - lastBeepTime >= beepInterval) {
    lastBeepTime = millis();
    tone(BUZZER_PIN, 2000, 100); // 100ms beep at 2kHz
  }
}