#include "Alerts.h"
#include "Config.h"
#include "Types.h"
#include "Sensors.h"
#include "Settings.h"
#include "Navigation.h"
#include <Arduino.h>

static bool pageHasActiveAlert() {
  switch (currentScreen) {
    case SOUND_SCREEN:      return sensors.soundRaw > settings.soundThreshold;
    case WATER_SCREEN:      return sensors.waterRaw > settings.waterThreshold;
    case OBJECT_SCREEN:     return sensors.irDetected;
    case IR_REFLECTION_SCREEN: return sensors.tcrtDetected;
    case FLAME_SCREEN:      return sensors.flameDetected;
    case CONDITIONS_SCREEN:
      return sensors.flameDetected
          || sensors.soundRaw > settings.soundThreshold
          || sensors.waterRaw > settings.waterThreshold
          || sensors.irDetected;
    default: return false;
  }
}

bool alertActiveHere() {
  // A validated fall is a system-level critical event. It remains physically
  // actionable even in PAGE mode so changing screens cannot suppress a fall.
  if(sensors.fallDetected) return true;

  // GLOBAL mode intentionally uses only the system-wide alarm conditions.
  // IR reflection is an investigation sensor and is therefore PAGE-scoped only;
  // sunlight/reflection outdoors must not create a global physical alarm.
  return settings.automaticAlerts
       ? (systemStatus == STATUS_WARNING || systemStatus == STATUS_CRITICAL)
       : pageHasActiveAlert();
}

void updateStatusOutputs() {
  static unsigned long lastFlash = 0;
  static bool flashState = false;
  unsigned long now = millis();

  if (!settings.ledsEnabled) {
    digitalWrite(PIN_GREEN_LED, LOW);
    digitalWrite(PIN_RED_LED, LOW);
    noTone(PIN_BUZZER);
    return;
  }

  if (!alertActiveHere()) {
    digitalWrite(PIN_GREEN_LED, HIGH);
    digitalWrite(PIN_RED_LED, LOW);
    noTone(PIN_BUZZER);
    return;
  }

  digitalWrite(PIN_GREEN_LED, LOW);
  bool critical = (systemStatus == STATUS_CRITICAL);
  unsigned long flashPeriod = critical ? 100 : 150;

  if (now - lastFlash > flashPeriod) {
    lastFlash = now;
    flashState = !flashState;
    if (flashState && settings.buzzerEnabled) {
      tone(PIN_BUZZER, critical ? 2200 : 1800, critical ? 90 : 80);
    }
  }
  digitalWrite(PIN_RED_LED, flashState);
}
