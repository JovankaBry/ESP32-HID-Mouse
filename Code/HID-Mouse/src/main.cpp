#include <Arduino.h>
#include "ble.h"
#include "button.h"
#include "led.h"
#include "ota.h"
#include "temp.h"
#include "buzzer.h"

void setup() {
  Serial.begin(115200);
  ledInit();
  otaInit();
  buttonInit();
  bleInit();
  tempInit();
  buzzerInit();
}

void loop() {
  otaHandler();
  handleButton();
  updateLed();
  bleHandle();
}
