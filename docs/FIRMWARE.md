# VIGIL-01 Firmware Architecture — V1.4

## Overview

V1.4 is the active modular ESP32 firmware for VIGIL-01. It samples the live sensor set, renders the OLED interface, serves the local dashboard, manages persistent settings, and drives physical status outputs.

The installed MPU module is handled as an **MPU-9250 / MPU-6500 / MPU-9255 family device**, not as an MPU6050. The firmware identifies supported silicon through `WHO_AM_I` and accesses the common accelerometer/gyroscope registers directly.

## Firmware modules

```text
VIGIL01.ino       Main coordinator
Config.h          GPIOs, I2C, thresholds, timing, fall parameters
Types.h           Shared screen/status types
Sensors.cpp/.h    Sensor acquisition + motion/fall processing
Navigation.cpp/.h Button input/navigation
MenuData.h        Menu definitions
DisplayUI.cpp/.h  OLED rendering
Alerts.cpp/.h     Physical alert presentation
Settings.cpp/.h   Persistent NVS/flash settings
WebDashboard.cpp/.h Local AP/dashboard/API/captive portal/mDNS
Watchdog.cpp/.h   ESP32 watchdog
```

## Motion subsystem

The motion module shares GPIO21/GPIO22 with the SSD1306 OLED.

```text
VCC -> 3V3
GND -> GND
SDA -> GPIO21
SCL -> GPIO22
AD0 -> GND for 0x68
```

Supported `WHO_AM_I` values:

```text
0x70 = MPU-6500
0x71 = MPU-9250
0x73 = MPU-9255
```

The firmware probes `0x68` first and `0x69` second. Configuration is ±8 g acceleration, ±500 °/s gyro, and 100 kHz I²C.

## Motion telemetry

The motion subsystem provides acceleration X/Y/Z, gyro X/Y/Z, acceleration magnitude, tilt angle, motion flag, impact flag, tilt flag, human-readable motion state, and fall-event state.

Motion state is classified as `STABLE`, `MOVING`, `ROTATING`, `FAST/IMPACT`, or `FREE-FALL` where applicable. These classifications describe movement and **are not themselves alarm conditions**.

## Fall detection

V1.4 uses a staged fall detector:

```text
Free-fall / low-g
       ↓
Significant impact
       ↓
Sustained post-impact tilt
       ↓
Fall event / CRITICAL
```

Current prototype parameters:

```text
Free-fall:             < 4.0 m/s²
Impact:                > 25.0 m/s²
Post-impact tilt:      > 45°
Free-fall → impact:    ≤ 1200 ms
Tilt confirmation:     300 ms
Fall alert hold:       3000 ms
```

Normal running/walking acceleration, tilt by itself, or an isolated impact does not activate the physical alarm.

## System status / physical alarms

The system-wide alarm sources are:

```text
Water threshold exceeded -> WARNING
Object/IR detected       -> WARNING
Sound threshold exceeded -> WARNING
Fall event               -> CRITICAL
Flame/IR event           -> CRITICAL
```

The HW511/TCRT5000 **IR reflection** sensor is intentionally not part of `systemStatus`. Outdoor testing showed that sunlight/ambient infrared can make this close-range reflection sensor report detection. It is therefore PAGE-only.

Its alert path is:

```text
IR_REFLECTION_SCREEN -> sensors.tcrtDetected
```

when `ALERTS: PAGE` is selected.

## PAGE vs GLOBAL alert policy

`ALERTS: PAGE` and `ALERTS: GLOBAL` are not equivalent.

### PAGE

Only the alarm condition belonging to the currently selected page can activate the physical alarm. This includes the IR reflection page:

```text
SOUND page          -> sound threshold
WATER page          -> water threshold
OBJECT page         -> IR obstacle detection
FLAME page          -> flame/IR event
IR REFLECTION page  -> TCRT5000 detection
```

An unrelated sensor condition on another page must not activate the physical alarm.

### GLOBAL

Only defined system-wide alarm conditions activate the physical alarm. The TCRT5000 IR reflection sensor is intentionally excluded from GLOBAL mode because outdoor testing demonstrated nuisance detections.

A validated fall remains system-level and can activate the physical alarm regardless of the selected page.

## Shared I²C bus

`Sensors.cpp` is the sole owner of I²C initialization:

```text
Wire.begin(GPIO21, GPIO22)
Wire.setClock(100000)
```

`DisplayUI.cpp` attaches the OLED to the existing `Wire` object without calling `Wire.begin()` again.

## Main execution model

```text
setup()
  -> Serial / GPIO
  -> settings
  -> navigation
  -> sensors + I2C
  -> display
  -> web services
  -> watchdog

loop()
  -> web + navigation
  -> fast sensors / motion / heartbeat / status
  -> slow DHT
  -> OLED
  -> status outputs
  -> watchdog feed
```

## Dashboard

The local dashboard provides live telemetry through `/data`, including `mpuPresent`, acceleration, gyro, tilt, `motionState`, `motionDetected`, `impactDetected`, `tiltDetected`, and `fallDetected`. Browser polling is 1000 ms.

## Required libraries

- Adafruit GFX Library
- Adafruit SSD1306
- DHT sensor library by Adafruit
- ArduinoJson 6.x

No MPU6050-specific library is required. `Wire`, WiFi, WebServer, DNSServer, ESPmDNS, Preferences, and watchdog functionality come from the ESP32 environment.

## Current limitations

- Motion/fall thresholds are prototype heuristics.
- Absolute 3D position cannot be determined reliably from this 6-axis IMU alone because integrated acceleration and gyro data drift over time.
- Heartbeat is experimental and not medical.
- Analog sensors remain raw/relative until characterized/calibrated.
- Flame/IR is not certified fire detection.
- The TCRT5000 reflection signal is environmental-condition sensitive and is intentionally PAGE-only.
- Battery monitoring remains disabled.
- No historical telemetry or cloud storage is implemented.

## Validation checklist

- Confirm boot identifies `WHO_AM_I` and address.
- Confirm stationary magnitude near 9.8 m/s².
- Confirm rotation changes gyro/tilt.
- Confirm normal walking/running changes telemetry without physical alarm.
- Confirm a standalone impact does not alarm.
- Confirm PAGE + IR REFLECTION + TCRT5000 detection activates the alarm.
- Confirm unrelated-page conditions do not activate a PAGE alarm.
- Confirm GLOBAL + IR reflection only does not activate the alarm.
- Confirm GLOBAL defined alarm sources still activate the alarm.
- Confirm the complete fall sequence produces a fall event under controlled testing.
