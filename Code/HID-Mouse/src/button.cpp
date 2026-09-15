#include <Arduino.h>
#include "button.h"
#include "ble.h"

bool automationEnabled = false;
bool lastButtonState = HIGH;

bool buttonPressed() {
  bool current = digitalRead(BUTTON_PIN);
  bool pressed = false;

  if (lastButtonState == HIGH && current == LOW) {
    delay(50); // debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      pressed = true;
    }
  }

  lastButtonState = current;
  return pressed;
}

// Single visible cursor twitch to confirm the button press registered.
// Moves out and straight back so the cursor ends where it started.
void nudgeCursor() {
  if (!bleMouse.isConnected()) return;
  moveMouse(50, 0);
  delay(30); // let the host process the first report before the return move
  moveMouse(-20, 0);
}

void handleButton() {
  bool connected = bleMouse.isConnected();

  if (buttonPressed() && connected) {
    automationEnabled = !automationEnabled;
    if (automationEnabled) {
      nudgeCursor();
      lastStepTime = millis(); // otherwise runMouse() fires a real step on the
                               // next loop() and blurs into the nudge
    }
    Serial.println(automationEnabled ? "Automation resumed" : "Automation paused");
  }
}

void buttonInit() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}
