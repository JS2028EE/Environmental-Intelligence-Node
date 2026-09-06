#pragma once
#include <Arduino.h>
#include "Types.h"

extern ScreenState currentScreen;
extern uint8_t cursor;

void navigationBegin();  // call once from setup()
void navigationPoll();   // call every loop(); reads + debounces the 4 buttons
