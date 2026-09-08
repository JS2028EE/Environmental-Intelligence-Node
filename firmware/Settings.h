#pragma once
#include <Arduino.h>

// User-configurable behavior. Loaded from flash (NVS) at boot and written
// back immediately whenever something changes, so it survives power loss.
struct Settings {
  bool ledsEnabled     = true;
  bool buzzerEnabled   = true;
  bool automaticAlerts = false;  // false = PAGE scoped, true = GLOBAL
  bool useFahrenheit   = false;
  int  soundThreshold;
  int  waterThreshold;
  float fallFreefallThreshold;
  float fallImpactThreshold;
  float fallTiltThreshold;
};

extern Settings settings;

void settingsLoad();
void settingsSaveFlag(const char* key, bool value);
void settingsSaveInt(const char* key, int value);
void settingsSaveFloat(const char* key, float value);
