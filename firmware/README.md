# VIGIL-01 — V1.4 Firmware

This directory contains the active modular VIGIL-01 firmware. V1.4 adds the corrected MPU-9250-family motion subsystem and changes motion handling so ordinary walking/running/rotation is telemetry only. Physical alarms are reserved for environmental events and a staged fall event.

## Current architecture

```text
VIGIL01.ino       Main setup/loop coordinator
Config.h          Pins, I2C addresses, thresholds, timing
Types.h           Shared screen/status types
Sensors.cpp/.h    Sensor acquisition, MPU-9250-family processing, fall detection
MenuData.h        Menu tree
Navigation.cpp/.h Buttons/navigation/debounce
DisplayUI.cpp/.h OLED rendering
Alerts.cpp/.h     LED/buzzer presentation
Settings.cpp/.h   Persistent NVS settings
WebDashboard.cpp/.h Wi-Fi AP/dashboard/API/captive portal/mDNS
Watchdog.cpp/.h   Watchdog recovery
```

## MPU-9250 / MPU-6500 / MPU-9255 motion subsystem

The physical module is a board marked for the MPU-9250/MPU-6500/MPU-9255 family. The firmware reads `WHO_AM_I` and accepts:

```text
0x70 -> MPU-6500
0x71 -> MPU-9250
0x73 -> MPU-9255
```

The device is probed at `0x68` and `0x69` and shares the OLED I²C bus:

```text
VCC -> ESP32 3V3
GND -> ESP32 GND
SDA -> GPIO21
SCL -> GPIO22
AD0 -> GND for 0x68
```

The MPU is accessed directly through `Wire`; the obsolete Adafruit MPU6050 dependency is no longer used.

Configuration:

- Accelerometer: ±8 g
- Gyroscope: ±500 °/s
- I²C: 100 kHz
- Acceleration magnitude and tilt are calculated from the accelerometer.
- Gyroscope X/Y/Z reports angular velocity.

## Motion vs. alarm behavior

Motion sensing has two jobs: describe how the device is moving and identify a possible fall. **Motion, impact, and tilt flags are telemetry and do not directly trigger the physical alarm.**

The dashboard also reports a human-readable motion state:

```text
STABLE
MOVING
ROTATING
FAST/IMPACT
FREE-FALL
```

### Fall detection

A fall is treated as a sequence rather than a single acceleration spike:

```text
LOW-G / FREE-FALL
      ↓
HIGH-G IMPACT
      ↓
SUSTAINED POST-IMPACT TILT
      ↓
FALL EVENT
```

Prototype parameters:

- Free-fall threshold: `< 4.0 m/s²`
- Impact threshold: `> 25.0 m/s²`
- Post-impact tilt: `> 45°`
- Maximum free-fall-to-impact window: `1200 ms`
- Tilt confirmation: `300 ms`
- Fall alert hold: `3000 ms`

This deliberately prevents normal walking/running motion or a standalone acceleration spike from becoming an alarm. These are prototype heuristics and require real-world characterization; they are not certified fall-detection or life-safety logic.

## System status

Current alarm-producing conditions are:

```text
Water > configured threshold -> WARNING
Object/IR detected           -> WARNING
Sound > configured threshold -> WARNING
Fall event                   -> CRITICAL
Flame/IR event               -> CRITICAL
```

The following **do not** directly change alarm status:

```text
Motion detected
Impact telemetry
Tilt telemetry
Gyroscope rotation
```

Critical status has priority over warning.

## I²C ownership

`Sensors.cpp` owns initialization of the shared I²C bus:

```text
Wire.begin(GPIO21, GPIO22)
Wire.setClock(100000)
```

`DisplayUI.cpp` uses the configured `Wire` object without reinitializing the bus. This prevents the earlier duplicate-I²C-initialization problem.

## Dashboard

The ESP32 provides:

```text
SSID: VIGIL-01
Password: VIGIL01_2026
mDNS: vigil01.local
```

`/data` exposes environmental values, heartbeat values, investigation sensors, motion telemetry, motion state, fall state, overall status, settings, and alert state. Browser refresh was reduced to 1000 ms from the earlier 700 ms interval.

## Settings

Persistent settings:

```text
LEDS: ON/OFF
BUZZER: ON/OFF
ALERTS: PAGE/GLOBAL
UNITS: C/F
Sound threshold
Water threshold
```

## Libraries

Required Arduino Library Manager libraries:

- Adafruit GFX Library
- Adafruit SSD1306
- DHT sensor library by Adafruit
- ArduinoJson 6.x

The MPU-9250 family is accessed directly and **does not require Adafruit MPU6050 or Adafruit Unified Sensor**.

The ESP32 core provides Wire, WiFi, WebServer, DNSServer, ESPmDNS, Preferences, and watchdog support.

## Other V1 scope

BME280, GPS, SD storage, cloud telemetry, historical database, and battery monitoring are not active V1.4 subsystems. GPIO39 remains reserved for future battery monitoring.

## Validation

1. Boot at 115200 baud.
2. Confirm the Serial message identifies the MPU family and I²C address.
3. Leave the unit still and confirm acceleration magnitude is near 9.8 m/s².
4. Rotate it and verify tilt and gyro values change.
5. Walk/run normally and confirm motion telemetry changes without an alarm.
6. Test controlled fall-like sequences only in a safe setup; confirm the staged detector rather than a raw impact is what produces a fall event.
