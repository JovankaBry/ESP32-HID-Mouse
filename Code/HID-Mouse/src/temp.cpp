#include <Arduino.h>
#include <HTTPClient.h>
#include "temp.h"

const int tempInterval = 1000;
unsigned long lastTempTime = 0;

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
