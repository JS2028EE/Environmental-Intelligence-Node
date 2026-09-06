#pragma once
#include <Arduino.h>
#include "Types.h"
#include "Config.h"

struct SensorData {
  float temperatureC = NAN;
  float humidity = NAN;
  int lightRaw = 0;
  int soundRaw = 0;
  int heartRaw = 0;
  int hallRaw = 0;
  int waterRaw = 0;
  bool tcrtDetected = false;
  bool irDetected = false;
  bool flameDetected = false;
  int heartRate = 0;
  const char* heartSignal = "WAIT"; // string literal, not Arduino String —
                                     // avoids heap fragmentation over long runs
};

extern SensorData sensors;
extern SystemStatus systemStatus;

void sensorsBegin();
void sensorsReadFast();   // digital + analog sensors, called every SENSOR_POLL_MS
void sensorsReadSlow();   // DHT11, called every DHT_POLL_MS (it's a slow sensor)
void heartRateProcess();
SystemStatus evaluateSystemStatus();

#if BATTERY_MONITORING_ENABLED
float batteryReadPercent();
#endif
