# VIGIL-01 Firmware Architecture — V1.4

## Overview

V1.4 is the active modular ESP32 firmware for VIGIL-01. It samples the live sensor set, renders the OLED interface, serves the local dashboard, manages persistent settings, and drives physical status outputs.

The major current motion revision is that the installed MPU module is handled as an **MPU-9250 / MPU-6500 / MPU-9255 family device**, not as an MPU6050. The firmware identifies the actual supported silicon through `WHO_AM_I` and accesses the common accelerometer/gyroscope registers directly.

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

The firmware probes `0x68` first and `0x69` second. Serial reports the detected address and ID.

Configuration is ±8 g acceleration, ±500 °/s gyro, and 100 kHz I²C. The implementation reads acceleration and gyro registers directly, so the obsolete Adafruit MPU6050 library is not required.

## Motion telemetry

The motion subsystem provides:

- acceleration X/Y/Z in m/s²
- gyro X/Y/Z in rad/s
- acceleration magnitude
- tilt angle
- motion flag
- impact flag
- tilt flag
- human-readable motion state
- fall-event state

Motion state is classified as `STABLE`, `MOVING`, `ROTATING`, `FAST/IMPACT`, or `FREE-FALL` where applicable.

These classifications describe movement. **They are not themselves alarm conditions.**

## Fall detection

The previous design treated motion/tilt as a warning and raw high acceleration as a critical event. This caused fast walking/running and other normal movement to activate the alarm.

V1.4 replaces that behavior with a staged fall detector:

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

The detector therefore requires a temporal sequence. A normal running/walking acceleration spike, a tilt by itself, or an impact without the preceding low-g phase does not activate the physical alarm.

This is an engineering prototype, not a certified fall-detection or life-safety system. Thresholds should be characterized with controlled, safe tests before being treated as reliable.

## System status / physical alarms

```text
Water threshold exceeded -> WARNING
Object/IR detected       -> WARNING
Sound threshold exceeded -> WARNING
Fall event               -> CRITICAL
Flame/IR event           -> CRITICAL
```

Motion, raw impact, tilt, gyro rotation, and movement classification are telemetry only.

`ALERTS: PAGE` and `ALERTS: GLOBAL` continue to control whether defined physical alert conditions are presented from the current page or globally. The fall event is a system-level critical condition and can therefore be presented in GLOBAL mode.

## Shared I²C bus

`Sensors.cpp` is the sole owner of I²C initialization:

```text
Wire.begin(GPIO21, GPIO22)
Wire.setClock(100000)
```

`DisplayUI.cpp` attaches the OLED to the existing `Wire` object without calling `Wire.begin()` again. This corrected the earlier shared-bus initialization flaw.

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

Timing remains approximately 100 ms for fast sensing and OLED refresh, 2 s for DHT reads, and 40 ms button debounce.

## Dashboard

The local dashboard provides live telemetry through `/data`, including `mpuPresent`, acceleration, gyro, tilt, `motionState`, `motionDetected`, `impactDetected`, `tiltDetected`, and `fallDetected`. Browser polling is now 1000 ms rather than 700 ms.

## Required libraries

- Adafruit GFX Library
- Adafruit SSD1306
- DHT sensor library by Adafruit
- ArduinoJson 6.x

No MPU6050-specific library is required. `Wire`, WiFi, WebServer, DNSServer, ESPmDNS, Preferences, and watchdog functionality come from the ESP32 environment.

## Current limitations

- Motion/fall thresholds are prototype heuristics.
- Absolute 3D position cannot be determined reliably from this 6-axis IMU alone because integrated acceleration and gyro data drift over time. V1.4 reports orientation and movement rather than a trustworthy absolute location.
- Heartbeat is experimental and not medical.
- Analog sensors remain raw/relative until characterized/calibrated.
- Flame/IR is not certified fire detection.
- Battery monitoring remains disabled.
- No historical telemetry or cloud storage is implemented.

## Validation checklist

- Confirm boot identifies `WHO_AM_I` and address.
- Confirm stationary magnitude near 9.8 m/s².
- Confirm rotation changes gyro/tilt.
- Confirm normal walking/running changes telemetry without physical alarm.
- Confirm a standalone impact does not alarm.
- Confirm the complete fall sequence produces a fall event under controlled testing.
- Verify dashboard and OLED expose the same motion state.
