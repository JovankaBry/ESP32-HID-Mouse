#include <Arduino.h>
#include <BleMouse.h>
#include <wifi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "secrets.h"

#define BUTTON_PIN 22
#define DEBOUNCE_MS 50

BleMouse bleMouse("Logitech MX Master 3", "Logitech", 88);

// WiFi credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

bool automationEnabled = false;

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

  // Wifi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("Connecting to WiFi...");  // Attempting to connect to wifi
  }
  Serial.println("Connected! IP: " + WiFi.localIP().toString()); // Succesfully connected to wifi

  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  //////////////////////////////////////////////////////////////////START//////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////OTA///////////////////////////////////////////////////////////////////////////////////////////////////
  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("Start updating " + type);
    bleMouse.end(); // Stop BLE mouse to avoid issues during OTA update
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nUpdate Complete");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.println("OTA Ready");
  ////////////////////////////////////////////////////////////////////END////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////OTA////////////////////////////////////////////////////////////////////////////////////////////////

  Serial.println("Starting Logitech MX Master 3 mouse emulation");
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  bleMouse.begin();
}

void loop() {
  ArduinoOTA.handle();
  handleButton();

  if (bleMouse.isConnected() && automationEnabled) {
    runMouse();
  }
}