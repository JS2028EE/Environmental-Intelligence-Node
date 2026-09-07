// =============================================================================
//  VIGIL-01 — Portable Environmental Intelligence Node
//  Firmware V1.3
// -----------------------------------------------------------------------------
//  V1.3 adds a navigation/motion sensing subsystem without changing the
//  existing sensor pins, button pins, alarm behavior, or I2C display bus.
//  MPU6050 + BME280 share I2C; a receive-only GPS feed uses GPIO5.
// =============================================================================
// Required libraries (Arduino Library Manager):
//   Adafruit GFX Library
//   Adafruit SSD1306
//   DHT sensor library
//   Adafruit MPU6050
//   Adafruit Unified Sensor
//   Adafruit BME280 Library
//   TinyGPSPlus
//   ArduinoJson 6.x
// Everything else used here ships with the ESP32 board package.
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
  pinMode(PIN_GREEN_LED, OUTPUT); pinMode(PIN_RED_LED, OUTPUT); pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_GREEN_LED, LOW); digitalWrite(PIN_RED_LED, LOW); noTone(PIN_BUZZER);
  settingsLoad(); navigationBegin(); sensorsBegin(); displayBegin(); displayShowBoot(); webBegin();
#if WATCHDOG_ENABLED
  watchdogBegin();
#endif
}

void loop() {
  unsigned long now = millis();
  webPoll(); navigationPoll();
  if (now-lastSensorRead >= SENSOR_POLL_MS) { lastSensorRead=now; sensorsReadFast(); heartRateProcess(); systemStatus=evaluateSystemStatus(); }
  if (now-lastDHTRead >= DHT_POLL_MS) { lastDHTRead=now; sensorsReadSlow(); }
  if (now-lastDisplayUpdate >= DISPLAY_REFRESH_MS) { lastDisplayUpdate=now; displayRender(); }
  updateStatusOutputs();
#if WATCHDOG_ENABLED
  watchdogFeed();
#endif
}
