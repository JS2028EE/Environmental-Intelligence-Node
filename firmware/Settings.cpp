#include "Settings.h"
#include "Config.h"
#include <Preferences.h>

Settings settings;
static Preferences prefs;
static const char* NS = "vigil01";

void settingsLoad() {
  prefs.begin(NS, false);
  settings.ledsEnabled     = prefs.getBool("leds", true);
  settings.buzzerEnabled   = prefs.getBool("buzzer", true);
  settings.automaticAlerts = prefs.getBool("alerts", false);
  settings.useFahrenheit   = prefs.getBool("fahren", false);
  settings.soundThreshold  = prefs.getInt("soundThr", DEFAULT_SOUND_ALARM_THRESHOLD);
  settings.waterThreshold  = prefs.getInt("waterThr", DEFAULT_WATER_ALARM_THRESHOLD);
  settings.fallFreefallThreshold = prefs.getFloat("fallFree", FALL_FREEFALL_THRESHOLD_MS2);
  settings.fallImpactThreshold   = prefs.getFloat("fallImpact", IMPACT_ACCEL_THRESHOLD_MS2);
  settings.fallTiltThreshold     = prefs.getFloat("fallTilt", FALL_POST_IMPACT_TILT_DEG);
  prefs.end();
}

void settingsSaveFlag(const char* key, bool value) {
  prefs.begin(NS, false);
  prefs.putBool(key, value);
  prefs.end();
}

void settingsSaveInt(const char* key, int value) {
  prefs.begin(NS, false);
  prefs.putInt(key, value);
  prefs.end();
}

void settingsSaveFloat(const char* key, float value) {
  prefs.begin(NS, false);
  prefs.putFloat(key, value);
  prefs.end();
}
