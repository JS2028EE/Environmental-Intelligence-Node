#include "Sensors.h"
#include "Settings.h"
#include <DHT.h>

static DHT dht(PIN_DHT, DHTTYPE);

SensorData sensors;
SystemStatus systemStatus = STATUS_NORMAL;

static unsigned long flameHighSince = 0;
static int heartBaseline = 2048;
static bool heartAboveThreshold = false;
static unsigned long lastBeatTime = 0;

void sensorsBegin() {
  pinMode(PIN_TCRT, INPUT);
  pinMode(PIN_IR, INPUT);
  // Pulldown so this pin reads a deterministic LOW at rest; the flame
  // module drives it HIGH on a flame/IR event (physically validated).
  pinMode(PIN_FLAME, INPUT_PULLDOWN);
  dht.begin();
}

void sensorsReadFast() {
  sensors.heartRaw = analogRead(PIN_HEART);
  sensors.lightRaw = analogRead(PIN_LIGHT);
  sensors.soundRaw = analogRead(PIN_SOUND);
  sensors.hallRaw  = analogRead(PIN_HALL);
  sensors.waterRaw = analogRead(PIN_WATER);

  int tcrtRaw  = digitalRead(PIN_TCRT);
  int irRaw    = digitalRead(PIN_IR);
  int flameRaw = digitalRead(PIN_FLAME);

  sensors.tcrtDetected = TCRT_ACTIVE_LOW ? (tcrtRaw == LOW) : (tcrtRaw == HIGH);
  sensors.irDetected   = IR_ACTIVE_LOW   ? (irRaw   == LOW) : (irRaw   == HIGH);

  // Flame: physically validated as active-HIGH. V1.1 declared
  // FLAME_ACTIVE_LOW but the detection code below never actually read it
  // (it hardcoded flameRaw==HIGH) — wired it in for real here. Behavior
  // today is unchanged, since the constant is false.
  bool flameActive = FLAME_ACTIVE_LOW ? (flameRaw == LOW) : (flameRaw == HIGH);
  if (flameActive) {
    if (flameHighSince == 0) flameHighSince = millis();
    sensors.flameDetected = (millis() - flameHighSince >= FLAME_CONFIRM_MS);
  } else {
    flameHighSince = 0;
    sensors.flameDetected = false;
  }
}

void sensorsReadSlow() {
  sensors.temperatureC = dht.readTemperature();
  sensors.humidity = dht.readHumidity();
}

void heartRateProcess() {
  static unsigned long lastSample = 0;
  if (millis() - lastSample < 10) return;
  lastSample = millis();

  heartBaseline = (heartBaseline * 99 + sensors.heartRaw) / 100;
  int deviation = abs(sensors.heartRaw - heartBaseline);

  if (deviation < 20)      sensors.heartSignal = "WEAK";
  else if (deviation < 80) sensors.heartSignal = "FAIR";
  else                     sensors.heartSignal = "GOOD";

  int threshold = heartBaseline + 100;
  if (!heartAboveThreshold && sensors.heartRaw > threshold) {
    heartAboveThreshold = true;
    unsigned long now = millis();
    if (lastBeatTime > 0) {
      unsigned long interval = now - lastBeatTime;
      if (interval > 300 && interval < 2000) {
        int bpm = 60000 / interval;
        if (bpm >= 40 && bpm <= 200) {
          sensors.heartRate = (sensors.heartRate == 0) ? bpm : (sensors.heartRate * 3 + bpm) / 4;
        }
      }
    }
    lastBeatTime = now;
  }
  if (heartAboveThreshold && sensors.heartRaw < heartBaseline + 50) {
    heartAboveThreshold = false;
  }
}

SystemStatus evaluateSystemStatus() {
  bool warning = false, critical = false;
  if (sensors.waterRaw > settings.waterThreshold) warning = true;
  if (sensors.irDetected) warning = true;
  if (sensors.soundRaw > settings.soundThreshold) warning = true;
  if (sensors.flameDetected) critical = true;
  if (critical) return STATUS_CRITICAL;
  if (warning)  return STATUS_WARNING;
  return STATUS_NORMAL;
}

#if BATTERY_MONITORING_ENABLED
float batteryReadPercent() {
  int raw = analogRead(PIN_BATTERY);
  float vAdc = (raw / 4095.0f) * BATTERY_ADC_REF_V;
  float vBatt = vAdc * BATTERY_DIVIDER_RATIO;
  float pct = (vBatt - BATTERY_EMPTY_V) / (BATTERY_FULL_V - BATTERY_EMPTY_V) * 100.0f;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}
#endif
