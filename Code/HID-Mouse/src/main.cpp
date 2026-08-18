#include <Arduino.h>
#include <BleMouse.h>
#include <wifi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "secrets.h"
#include <BLEDevice.h>
#include <HTTPClient.h>

#define BUTTON_PIN 23
#define LED_PIN 21
#define DEBOUNCE_MS 50

BleMouse bleMouse("Logitech MX Master 3", "Logitech", 88);

// WiFi credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

bool otaInProgress = false;
unsigned long lastBlinkTime = 0;
bool ledBlinkState = false;
const int blinkInterval = 200;
bool automationEnabled = false;
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
const int tempInterval = 1000;
unsigned long lastTempTime = 0;

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
  if (buttonPressed()) {
    automationEnabled = !automationEnabled;
    if (automationEnabled) {
      nudgeCursor();
      lastStepTime = millis(); // otherwise runMouse() fires a real step on the
                               // next loop() and blurs into the nudge
    }
    Serial.println(automationEnabled ? "Automation resumed" : "Automation paused");
  }
}

void updateLed() {
 digitalWrite(LED_PIN, automationEnabled ? HIGH : LOW);
}

void reportTemp() {
  if (millis() - lastTempTime < tempInterval) return;
  lastTempTime = millis();
  float temp = temperatureRead();

  HTTPClient http;
  http.begin("http://192.168.0.147:5000/api/temp");
  http.addHeader("Content-Type", "application/json");
  int code = http.POST("{\"temp\":" + String(temp) + "}");
  if (code <= 0) {
    Serial.println("reportTemp failed: " + http.errorToString(code));
  } else if (code >= 300) {
    Serial.println("reportTemp failed: HTTP " + String(code));
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  Serial.println("boot"); // pre-initializes newlib's stdio lock on the main
                           // task before WiFi/mDNS's background task can
                           // race to init it concurrently during OTA

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
    otaInProgress = true;
    bleMouse.end(); // Stop BLE mouse to avoid issues during OTA update
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nUpdate Complete");
    otaInProgress = false;
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if (millis() - lastBlinkTime >= blinkInterval) {
      lastBlinkTime = millis();
      ledBlinkState = !ledBlinkState;
      digitalWrite(LED_PIN, ledBlinkState);
    }
    static int lastBucket = -1;
    unsigned int percent = progress / (total / 100);
    int bucket = percent / 10; // groups 0-9%, 10-19%, ... into one print each
    if (bucket != lastBucket) {
      Serial.printf("Progress: %u%%\r\n", percent);
      lastBucket = bucket;
    }
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    otaInProgress = false;
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  MDNS.end(); // ArduinoOTA starts mDNS internally for discovery, but we
              // always connect via explicit IP — shutting it down removes
              // the mDNS packet-handling code path that's been crashing
  Serial.println("OTA Ready");
  ////////////////////////////////////////////////////////////////////END////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////OTA////////////////////////////////////////////////////////////////////////////////////////////////

  Serial.println("Starting Logitech MX Master 3 mouse emulation");
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  bleMouse.begin();
}

void loop() {
  ArduinoOTA.handle();
  reportTemp();
  handleButton();
  updateLed();
  bool connected = bleMouse.isConnected();

  if (wasConnected && !connected) {
    BLEDevice::startAdvertising();
  }
  wasConnected = connected;

  if (bleMouse.isConnected() && automationEnabled) {
    runMouse();
  }
}