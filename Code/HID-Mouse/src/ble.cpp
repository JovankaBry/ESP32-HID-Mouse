#include <Arduino.h>
#include <BLEDevice.h>
#include "ble.h"
#include "button.h"

BleMouse bleMouse("Logitech MX Master 3", "Logitech", 88);

bool wasConnected = false;

void moveMouse (int dx, int dy) {
  bleMouse.move(dx, dy);
}

void scroll(int amount) {
  bleMouse.move(0,0, amount);
}

const int step = 30;
const int stepDelay = 10000;
int mouseStep = 0;
unsigned long lastStepTime = 0;

// Non-blocking: advances one step every `stepDelay` ms instead of using
// delay(), so loop() stays free to check the button in between steps.
void runMouse() {
  if (millis() - lastStepTime < stepDelay) return;
  lastStepTime = millis();

  switch (mouseStep) {
    case 0: moveMouse(step, 0); break;
    case 1: scroll(5); break;
    case 2: moveMouse(0, step); break;
    case 3: scroll(-5); break;
    case 4: moveMouse(-step, 0); break;
    case 5: scroll(5); break;
    case 6: moveMouse(0, -step); break;
    case 7: scroll(-5); break;
  }

  mouseStep = (mouseStep + 1) % 8;
}

void bleInit() {
  Serial.println("Starting Logitech MX Master 3 mouse emulation");
  bleMouse.begin();
}

void bleHandle() {
  bool connected = bleMouse.isConnected();

  if (wasConnected && !connected) {
    BLEDevice::startAdvertising();
  }
  wasConnected = connected;

  if (connected && automationEnabled) {
    runMouse();
  }
}
