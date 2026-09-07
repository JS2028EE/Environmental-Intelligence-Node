#pragma once
#include <Arduino.h>
#include "Types.h"
#include "Config.h"

struct SensorData {
  float temperatureC=NAN, humidity=NAN; int lightRaw=0,soundRaw=0,heartRaw=0,hallRaw=0,waterRaw=0;
  bool tcrtDetected=false,irDetected=false,flameDetected=false; int heartRate=0; const char* heartSignal="WAIT";
  bool mpuPresent=false;
  float accelX=NAN,accelY=NAN,accelZ=NAN,gyroX=NAN,gyroY=NAN,gyroZ=NAN,accelMagnitude=NAN,tiltDegrees=NAN;
  bool motionDetected=false,impactDetected=false,tiltDetected=false,fallDetected=false;
  const char* motionState="UNKNOWN";
};

extern SensorData sensors;
extern SystemStatus systemStatus;
void sensorsBegin(); void sensorsReadFast(); void sensorsReadSlow(); void heartRateProcess(); SystemStatus evaluateSystemStatus();
#if BATTERY_MONITORING_ENABLED
float batteryReadPercent();
#endif
