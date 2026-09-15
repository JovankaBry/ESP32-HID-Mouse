#pragma once

#define BUTTON_PIN 23
#define DEBOUNCE_MS 50

extern bool automationEnabled;

void buttonInit();
void handleButton();
