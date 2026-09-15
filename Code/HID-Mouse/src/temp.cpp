#include <Arduino.h>
#include <HTTPClient.h>
#include "temp.h"

void tempTask(void* param) {
  for (;;) {
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

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void tempInit() {
  xTaskCreate(tempTask, "tempTask", 4096, NULL, 1, NULL);
}
