#include <Arduino.h>
#include <wifi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "secrets.h"
#include "ota.h"
#include "ble.h"
#include "led.h"

// WiFi credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

bool otaInProgress = false;

void otaInit() {
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
    ledOtaBlink();
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
}

void otaHandler() {
  ArduinoOTA.handle();
}
