#pragma once

#include <BleMouse.h>

extern BleMouse bleMouse;
extern unsigned long lastStepTime;

void bleInit();
void bleHandle();
void moveMouse(int dx, int dy);
