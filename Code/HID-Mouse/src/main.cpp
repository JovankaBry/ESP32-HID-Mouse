#include <Arduino.h>
#include "ble.h"
#include "button.h"
#include "led.h"
#include "ota.h"
#include "temp.h"

void setup() {
  Serial.begin(115200);
  ledInit();
  otaInit();
  buttonInit();
  bleInit();
  tempInit();
}

void loop() {
  otaHandler();
  handleButton();
  updateLed();
  bleHandle();
}
