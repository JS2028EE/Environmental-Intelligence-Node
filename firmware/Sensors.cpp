#include "Sensors.h"
#include "Settings.h"
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

static DHT dht(PIN_DHT,DHTTYPE); static Adafruit_MPU6050 mpu;
SensorData sensors; SystemStatus systemStatus=STATUS_NORMAL;
static unsigned long flameHighSince=0,lastBeatTime=0; static int heartBaseline=2048; static bool heartAboveThreshold=false;

void sensorsBegin(){
  pinMode(PIN_TCRT,INPUT); pinMode(PIN_IR,INPUT); pinMode(PIN_FLAME,INPUT_PULLDOWN); dht.begin();
  Wire.begin(PIN_OLED_SDA,PIN_OLED_SCL);
  sensors.mpuPresent=mpu.begin(MPU6050_ADDRESS);
  if(!sensors.mpuPresent) sensors.mpuPresent=mpu.begin(MPU6050_ALT_ADDRESS);
  if(sensors.mpuPresent){ mpu.setAccelerometerRange(MPU6050_RANGE_8_G); mpu.setGyroRange(MPU6050_RANGE_500_DEG); mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); }
}

void sensorsReadFast(){
  sensors.heartRaw=analogRead(PIN_HEART); sensors.lightRaw=analogRead(PIN_LIGHT); sensors.soundRaw=analogRead(PIN_SOUND); sensors.hallRaw=analogRead(PIN_HALL); sensors.waterRaw=analogRead(PIN_WATER);
  int tcrtRaw=digitalRead(PIN_TCRT),irRaw=digitalRead(PIN_IR),flameRaw=digitalRead(PIN_FLAME);
  sensors.tcrtDetected=TCRT_ACTIVE_LOW?(tcrtRaw==LOW):(tcrtRaw==HIGH); sensors.irDetected=IR_ACTIVE_LOW?(irRaw==LOW):(irRaw==HIGH);
  bool flameActive=FLAME_ACTIVE_LOW?(flameRaw==LOW):(flameRaw==HIGH);
  if(flameActive){if(flameHighSince==0)flameHighSince=millis(); sensors.flameDetected=(millis()-flameHighSince>=FLAME_CONFIRM_MS);} else {flameHighSince=0;sensors.flameDetected=false;}

  if(sensors.mpuPresent){ sensors_event_t a,g,t; mpu.getEvent(&a,&g,&t); sensors.accelX=a.acceleration.x;sensors.accelY=a.acceleration.y;sensors.accelZ=a.acceleration.z;sensors.gyroX=g.gyro.x;sensors.gyroY=g.gyro.y;sensors.gyroZ=g.gyro.z;
    sensors.accelMagnitude=sqrtf(sensors.accelX*sensors.accelX+sensors.accelY*sensors.accelY+sensors.accelZ*sensors.accelZ);
    sensors.tiltDegrees=atan2f(sqrtf(sensors.accelX*sensors.accelX+sensors.accelY*sensors.accelY),fabsf(sensors.accelZ))*57.2958f;
    sensors.motionDetected=fabsf(sensors.accelMagnitude-9.80665f)>MOTION_ACCEL_THRESHOLD_MS2; sensors.impactDetected=sensors.accelMagnitude>IMPACT_ACCEL_THRESHOLD_MS2; sensors.tiltDetected=sensors.tiltDegrees>TILT_THRESHOLD_DEG;
  }
}

void sensorsReadSlow(){sensors.temperatureC=dht.readTemperature();sensors.humidity=dht.readHumidity();}
void heartRateProcess(){static unsigned long lastSample=0;if(millis()-lastSample<10)return;lastSample=millis();heartBaseline=(heartBaseline*99+sensors.heartRaw)/100;int deviation=abs(sensors.heartRaw-heartBaseline);if(deviation<20)sensors.heartSignal="WEAK";else if(deviation<80)sensors.heartSignal="FAIR";else sensors.heartSignal="GOOD";int threshold=heartBaseline+100;if(!heartAboveThreshold&&sensors.heartRaw>threshold){heartAboveThreshold=true;unsigned long now=millis();if(lastBeatTime>0){unsigned long interval=now-lastBeatTime;if(interval>300&&interval<2000){int bpm=60000/interval;if(bpm>=40&&bpm<=200)sensors.heartRate=(sensors.heartRate==0)?bpm:(sensors.heartRate*3+bpm)/4;}}lastBeatTime=now;}if(heartAboveThreshold&&sensors.heartRaw<heartBaseline+50)heartAboveThreshold=false;}
SystemStatus evaluateSystemStatus(){bool warning=false,critical=false;if(sensors.waterRaw>settings.waterThreshold)warning=true;if(sensors.irDetected)warning=true;if(sensors.soundRaw>settings.soundThreshold)warning=true;if(sensors.motionDetected||sensors.tiltDetected)warning=true;if(sensors.impactDetected)critical=true;if(sensors.flameDetected)critical=true;if(critical)return STATUS_CRITICAL;if(warning)return STATUS_WARNING;return STATUS_NORMAL;}
#if BATTERY_MONITORING_ENABLED
float batteryReadPercent(){int raw=analogRead(PIN_BATTERY);float vAdc=(raw/4095.0f)*BATTERY_ADC_REF_V;float vBatt=vAdc*BATTERY_DIVIDER_RATIO;float pct=(vBatt-BATTERY_EMPTY_V)/(BATTERY_FULL_V-BATTERY_EMPTY_V)*100.0f;if(pct<0)pct=0;if(pct>100)pct=100;return pct;}
#endif
