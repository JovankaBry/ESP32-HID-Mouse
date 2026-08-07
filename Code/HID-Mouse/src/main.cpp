#include <Arduino.h>
#include <BleMouse.h>

#define BUTTON_PIN 22
#define DEBOUNCE_MS 50

BleMouse bleMouse("Logitech MX Master 3", "Logitech", 88);

bool automationEnabled = true;

void moveMouse (int dx, int dy) {
  bleMouse.move(dx, dy);
}

void scroll(int amount) {
  bleMouse.move(0,0, amount);
}

const int step = 20;
const int stepDelay = 150;
int mouseStep = 0;
unsigned long lastStepTime = 0;

// Non-blocking: advances one step every `stepDelay` ms instead of using
// delay(), so loop() stays free to check the button in between steps.
void runMouse() {
  if (millis() - lastStepTime < stepDelay) return;
  lastStepTime = millis();

  switch (mouseStep) {
    case 0: moveMouse(step, 0); break;
    case 1: scroll(1); break;
    case 2: moveMouse(0, step); break;
    case 3: scroll(-1); break;
    case 4: moveMouse(-step, 0); break;
    case 5: scroll(1); break;
    case 6: moveMouse(0, -step); break;
    case 7: scroll(-1); break;
  }

  mouseStep = (mouseStep + 1) % 8;
}

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

void handleButton() {
  if (buttonPressed()) {
    automationEnabled = !automationEnabled;
    Serial.println(automationEnabled ? "Automation resumed" : "Automation paused");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Logitech MX Master 3 mouse emulation");
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  bleMouse.begin();
}

void loop() {
  handleButton();

  if (bleMouse.isConnected() && automationEnabled) {
    runMouse();
  }
}