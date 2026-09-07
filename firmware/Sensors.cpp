#include "Sensors.h"
#include "Settings.h"
#include <Wire.h>
#include <DHT.h>
#include <math.h>

static DHT dht(PIN_DHT,DHTTYPE);
SensorData sensors; SystemStatus systemStatus=STATUS_NORMAL;
static unsigned long flameHighSince=0,lastBeatTime=0; static int heartBaseline=2048; static bool heartAboveThreshold=false;

// MPU-9250 / MPU-6500 / MPU-9255 register interface.
// The accelerometer and gyroscope portion of these devices uses the same
// register map for the measurements needed by VIGIL-01, so no MPU6050 library
// is required.
static uint8_t motionAddress=0;
static uint8_t motionWhoAmI=0;

// Fall detection state machine. Motion, impact, and tilt remain useful
// telemetry, but only the validated multi-stage fall event can raise an alarm.
static unsigned long freeFallSince=0;
static unsigned long impactCandidateSince=0;
static unsigned long postImpactSince=0;
static unsigned long fallTiltSince=0;
static unsigned long fallAlertUntil=0;

static bool motionWriteByte(uint8_t reg,uint8_t value){
  Wire.beginTransmission(motionAddress);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission()==0;
}

static bool motionReadBytes(uint8_t reg,uint8_t* data,size_t length){
  Wire.beginTransmission(motionAddress);
  Wire.write(reg);
  if(Wire.endTransmission(false)!=0)return false;
  size_t received=Wire.requestFrom((int)motionAddress,(int)length);
  if(received!=length)return false;
  for(size_t i=0;i<length;i++)data[i]=Wire.read();
  return true;
}

static bool motionProbe(uint8_t address,uint8_t& whoAmI){
  motionAddress=address;
  uint8_t value=0;
  if(!motionReadBytes(0x75,&value,1))return false;
  // Known WHO_AM_I values: MPU-6500 = 0x70, MPU-9250 = 0x71,
  // MPU-9255 = 0x73. Accept these motion-sensor variants only.
  if(value!=0x70 && value!=0x71 && value!=0x73)return false;
  whoAmI=value;
  return true;
}

void sensorsBegin(){
  pinMode(PIN_TCRT,INPUT); pinMode(PIN_IR,INPUT); pinMode(PIN_FLAME,INPUT_PULLDOWN); dht.begin();

  // Motion sensor shares the OLED I2C bus on GPIO21/GPIO22.
  Wire.begin(PIN_OLED_SDA,PIN_OLED_SCL);
  Wire.setClock(100000);
  delay(50);

  uint8_t detectedWhoAmI=0;
  sensors.mpuPresent=motionProbe(MOTION_I2C_ADDRESS,detectedWhoAmI);
  if(!sensors.mpuPresent)sensors.mpuPresent=motionProbe(MOTION_I2C_ALT_ADDRESS,detectedWhoAmI);

  if(sensors.mpuPresent){
    motionWhoAmI=detectedWhoAmI;
    Serial.print("MPU-9250 family motion sensor detected at 0x");
    Serial.print(motionAddress,HEX);
    Serial.print(" (WHO_AM_I=0x");
    Serial.print(motionWhoAmI,HEX);
    Serial.println(")");

    // Wake device and configure the accelerometer to +/-8 g and gyro to +/-500 dps.
    motionWriteByte(0x6B,0x00); // PWR_MGMT_1: wake, internal clock
    delay(10);
    motionWriteByte(0x1A,0x03); // CONFIG: DLPF ~44 Hz gyro / ~41 Hz accel path
    motionWriteByte(0x1C,0x10); // ACCEL_CONFIG: +/-8 g
    motionWriteByte(0x1D,0x03); // ACCEL_CONFIG2: accelerometer DLPF
    motionWriteByte(0x1B,0x08); // GYRO_CONFIG: +/-500 dps
  } else {
    Serial.println("ERROR: MPU-9250/MPU-6500/MPU-9255 not detected at 0x68 or 0x69");
  }
}

void sensorsReadFast(){
  sensors.heartRaw=analogRead(PIN_HEART); sensors.lightRaw=analogRead(PIN_LIGHT); sensors.soundRaw=analogRead(PIN_SOUND); sensors.hallRaw=analogRead(PIN_HALL); sensors.waterRaw=analogRead(PIN_WATER);
  int tcrtRaw=digitalRead(PIN_TCRT),irRaw=digitalRead(PIN_IR),flameRaw=digitalRead(PIN_FLAME);
  sensors.tcrtDetected=TCRT_ACTIVE_LOW?(tcrtRaw==LOW):(tcrtRaw==HIGH); sensors.irDetected=IR_ACTIVE_LOW?(irRaw==LOW):(irRaw==HIGH);
  bool flameActive=FLAME_ACTIVE_LOW?(flameRaw==LOW):(flameRaw==HIGH);
  if(flameActive){if(flameHighSince==0)flameHighSince=millis(); sensors.flameDetected=(millis()-flameHighSince>=FLAME_CONFIRM_MS);} else {flameHighSince=0;sensors.flameDetected=false;}

  if(sensors.mpuPresent){
    uint8_t raw[14];
    if(motionReadBytes(0x3B,raw,sizeof(raw))){
      int16_t ax=(int16_t)((raw[0]<<8)|raw[1]);
      int16_t ay=(int16_t)((raw[2]<<8)|raw[3]);
      int16_t az=(int16_t)((raw[4]<<8)|raw[5]);
      int16_t gx=(int16_t)((raw[8]<<8)|raw[9]);
      int16_t gy=(int16_t)((raw[10]<<8)|raw[11]);
      int16_t gz=(int16_t)((raw[12]<<8)|raw[13]);

      // +/-8 g = 4096 LSB/g. Convert to m/s^2 for the existing VIGIL API.
      constexpr float ACCEL_SCALE=9.80665f/4096.0f;
      // +/-500 degrees/s = 65.5 LSB/(degrees/s). Convert to rad/s.
      constexpr float GYRO_SCALE=(3.14159265359f/180.0f)/65.5f;
      sensors.accelX=ax*ACCEL_SCALE; sensors.accelY=ay*ACCEL_SCALE; sensors.accelZ=az*ACCEL_SCALE;
      sensors.gyroX=gx*GYRO_SCALE; sensors.gyroY=gy*GYRO_SCALE; sensors.gyroZ=gz*GYRO_SCALE;
      sensors.accelMagnitude=sqrtf(sensors.accelX*sensors.accelX+sensors.accelY*sensors.accelY+sensors.accelZ*sensors.accelZ);
      sensors.tiltDegrees=atan2f(sqrtf(sensors.accelX*sensors.accelX+sensors.accelY*sensors.accelY),fabsf(sensors.accelZ))*57.2958f;

      // These are telemetry classifiers only. They no longer directly affect
      // the physical alarm system.
      sensors.motionDetected=fabsf(sensors.accelMagnitude-9.80665f)>MOTION_ACCEL_THRESHOLD_MS2;
      sensors.impactDetected=sensors.accelMagnitude>IMPACT_ACCEL_THRESHOLD_MS2;
      sensors.tiltDetected=sensors.tiltDegrees>TILT_THRESHOLD_DEG;

      float gyroMagnitude=sqrtf(sensors.gyroX*sensors.gyroX+sensors.gyroY*sensors.gyroY+sensors.gyroZ*sensors.gyroZ);
      if(sensors.accelMagnitude<0.5f || !isfinite(sensors.accelMagnitude)) sensors.motionState="UNKNOWN";
      else if(sensors.accelMagnitude<FALL_FREEFALL_THRESHOLD_MS2) sensors.motionState="FREE-FALL";
      else if(gyroMagnitude>3.5f) sensors.motionState="ROTATING";
      else if(sensors.accelMagnitude>15.0f) sensors.motionState="FAST/IMPACT";
      else if(sensors.motionDetected) sensors.motionState="MOVING";
      else sensors.motionState="STABLE";

      // Multi-stage fall detector:
      // 1) detect a low-g/free-fall phase,
      // 2) require a significant impact shortly afterward,
      // 3) require a sustained post-impact orientation change.
      // Normal walking/running can create motion and occasional acceleration
      // spikes, but should not satisfy this complete sequence.
      unsigned long now=millis();
      if(sensors.accelMagnitude<FALL_FREEFALL_THRESHOLD_MS2){
        if(freeFallSince==0)freeFallSince=now;
      } else if(freeFallSince>0 && now-freeFallSince>FALL_SEQUENCE_TIMEOUT_MS){
        freeFallSince=0;
      }

      if(freeFallSince>0 && sensors.accelMagnitude>IMPACT_ACCEL_THRESHOLD_MS2 && now-freeFallSince<=FALL_SEQUENCE_TIMEOUT_MS){
        impactCandidateSince=now;
        postImpactSince=now;
        freeFallSince=0;
        fallTiltSince=0;
      }

      if(impactCandidateSince>0){
        if(now-impactCandidateSince>FALL_POST_IMPACT_WINDOW_MS){
          impactCandidateSince=0;
          postImpactSince=0;
          fallTiltSince=0;
        } else if(sensors.tiltDegrees>FALL_POST_IMPACT_TILT_DEG){
          if(fallTiltSince==0)fallTiltSince=now;
          if(now-fallTiltSince>=FALL_TILT_CONFIRM_MS){
            fallAlertUntil=now+FALL_ALERT_HOLD_MS;
            impactCandidateSince=0;
            postImpactSince=0;
            fallTiltSince=0;
            Serial.println("FALL EVENT: low-g -> impact -> sustained post-impact tilt");
          }
        } else {
          fallTiltSince=0;
        }
      }

      sensors.fallDetected=(fallAlertUntil>now);
    } else {
      sensors.motionDetected=false; sensors.impactDetected=false; sensors.tiltDetected=false; sensors.fallDetected=false; sensors.motionState="UNKNOWN";
    }
  } else {
    sensors.fallDetected=false; sensors.motionState="UNKNOWN";
  }
}

void sensorsReadSlow(){sensors.temperatureC=dht.readTemperature();sensors.humidity=dht.readHumidity();}
void heartRateProcess(){static unsigned long lastSample=0;if(millis()-lastSample<10)return;lastSample=millis();heartBaseline=(heartBaseline*99+sensors.heartRaw)/100;int deviation=abs(sensors.heartRaw-heartBaseline);if(deviation<20)sensors.heartSignal="WEAK";else if(deviation<80)sensors.heartSignal="FAIR";else sensors.heartSignal="GOOD";int threshold=heartBaseline+100;if(!heartAboveThreshold&&sensors.heartRaw>threshold){heartAboveThreshold=true;unsigned long now=millis();if(lastBeatTime>0){unsigned long interval=now-lastBeatTime;if(interval>300&&interval<2000){int bpm=60000/interval;if(bpm>=40&&bpm<=200)sensors.heartRate=(sensors.heartRate==0)?bpm:(sensors.heartRate*3+bpm)/4;}}lastBeatTime=now;}if(heartAboveThreshold&&sensors.heartRaw<heartBaseline+50)heartAboveThreshold=false;}
SystemStatus evaluateSystemStatus(){bool warning=false,critical=false;if(sensors.waterRaw>settings.waterThreshold)warning=true;if(sensors.irDetected)warning=true;if(sensors.soundRaw>settings.soundThreshold)warning=true;if(sensors.fallDetected)critical=true;if(sensors.flameDetected)critical=true;/* Motion/impact/tilt are telemetry only; they do not alarm. */if(critical)return STATUS_CRITICAL;if(warning)return STATUS_WARNING;return STATUS_NORMAL;}
#if BATTERY_MONITORING_ENABLED
float batteryReadPercent(){int raw=analogRead(PIN_BATTERY);float vAdc=(raw/4095.0f)*BATTERY_ADC_REF_V;float vBatt=vAdc*BATTERY_DIVIDER_RATIO;float pct=(vBatt-BATTERY_EMPTY_V)/(BATTERY_FULL_V-BATTERY_EMPTY_V)*100.0f;if(pct<0)pct=0;if(pct>100)pct=100;return pct;}
#endif
