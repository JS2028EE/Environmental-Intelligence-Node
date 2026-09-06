// =============================================================================
//  VIGIL-01  —  Portable Environmental Intelligence Node
//  Firmware V1.2
// -----------------------------------------------------------------------------
//  Refactor of V1.1. Same pins, same sensor polarities/thresholds, same
//  button feel — reorganized into readable modules, with a data-driven menu
//  system, settings that survive a reboot, a friendlier web dashboard, and
//  a couple of real bugs fixed along the way. See README.md for the full
//  changelog.
//
//  Required libraries (Arduino Library Manager):
//    Adafruit GFX Library, Adafruit SSD1306, DHT sensor library, ArduinoJson
//  Everything else (WiFi, WebServer, DNSServer, ESPmDNS, Preferences,
//  esp_task_wdt) ships with the ESP32 board package — nothing else to install.
// =============================================================================

#include <Arduino.h>
#include "Types.h"
#include "Config.h"
#include "Settings.h"
#include "Sensors.h"
#include "Navigation.h"
#include "DisplayUI.h"
#include "Alerts.h"
#include "WebDashboard.h"
#if WATCHDOG_ENABLED
#include "Watchdog.h"
#endif

static unsigned long lastSensorRead = 0;
static unsigned long lastDHTRead = 0;
static unsigned long lastDisplayUpdate = 0;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_GREEN_LED, OUTPUT);
  pinMode(PIN_RED_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_GREEN_LED, LOW);
  digitalWrite(PIN_RED_LED, LOW);
  noTone(PIN_BUZZER);

  settingsLoad();
  navigationBegin();
  sensorsBegin();
  displayBegin();
  displayShowBoot();
  webBegin();

#if WATCHDOG_ENABLED
  watchdogBegin();
#endif
}

void loop() {
  unsigned long now = millis();

  webPoll();
  navigationPoll();

  if (now - lastSensorRead >= SENSOR_POLL_MS) {
    lastSensorRead = now;
    sensorsReadFast();
    heartRateProcess();
    systemStatus = evaluateSystemStatus();
  }

  if (now - lastDHTRead >= DHT_POLL_MS) {
    lastDHTRead = now;
    sensorsReadSlow();
  }

  if (now - lastDisplayUpdate >= DISPLAY_REFRESH_MS) {
    lastDisplayUpdate = now;
    displayRender();
  }

  updateStatusOutputs();

#if WATCHDOG_ENABLED
  watchdogFeed();
#endif
}
