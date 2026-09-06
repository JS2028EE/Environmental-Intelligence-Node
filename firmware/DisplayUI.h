#pragma once

void displayBegin();     // init the OLED — call once from setup()
void displayShowBoot();  // the two-frame boot splash
void displayRender();    // draw whatever `currentScreen` currently is
