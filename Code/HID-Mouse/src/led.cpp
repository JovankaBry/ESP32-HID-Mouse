#include <Arduino.h>
#include "led.h"
#include "button.h"

unsigned long lastBlinkTime = 0;
bool ledBlinkState = false;
const int blinkInterval = 200;

void updateLed() {
 digitalWrite(LED_PIN, automationEnabled ? HIGH : LOW);
}

void ledOtaBlink() {
  if (millis() - lastBlinkTime >= blinkInterval) {
    lastBlinkTime = millis();
    ledBlinkState = !ledBlinkState;
    digitalWrite(LED_PIN, ledBlinkState);
  }
}

void ledInit() {
  pinMode(LED_PIN, OUTPUT);
}
