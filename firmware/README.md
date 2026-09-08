# VIGIL-01 — V1.4 Firmware

This directory contains the active modular VIGIL-01 firmware. V1.4 adds the corrected MPU-9250-family motion subsystem and changes motion handling so ordinary walking/running/rotation is telemetry only. Physical alarms are reserved for defined environmental events and a staged fall event.

## Current architecture

```text
VIGIL01.ino       Main setup/loop coordinator
Config.h          Pins, I2C addresses, thresholds, timing
Secrets.h         Local-only AP credential file (ignored by Git)
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

## Credentials

The AP SSID remains `VIGIL-01`, but the deployment password is supplied by local `firmware/Secrets.h`. The real file is ignored by Git. Copy `firmware/Secrets.h.example` to `firmware/Secrets.h` and set the password before deployment. If the local file is absent, the firmware uses a compile-safe placeholder that must not be used for deployment.

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

## Live IMU fault handling

`mpuPresent` is based on live communication, not only the boot-time probe. A failed motion register read clears `mpuPresent`, invalidates motion telemetry, and causes periodic rediscovery/reconfiguration attempts. This prevents a stale `IMU OK` state after a breadboard connection fails and permits recovery without rebooting when the connection is restored.

## Motion vs. alarm behavior

Motion sensing has two jobs: describe how the device is moving and identify a possible fall. **Motion, impact, and tilt flags are telemetry and do not directly trigger the physical alarm.**

The dashboard also reports:

```text
STABLE
MOVING
ROTATING
FAST/IMPACT
FREE-FALL
```

### Fall detection

A fall is treated as a sequence:

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

## PAGE vs GLOBAL alerts

`ALERTS: PAGE` means the physical alarm responds only to the alarm condition associated with the currently selected page:

```text
SOUND page          -> sound threshold
WATER page          -> water threshold
OBJECT page         -> IR obstacle detection
FLAME page          -> flame/IR event
IR REFLECTION page  -> TCRT5000 reflection detection
```

An unrelated sensor condition must not activate the alarm on another page.

`ALERTS: GLOBAL` means defined system-wide alarm conditions can activate the physical alarm:

```text
Water > configured threshold -> WARNING
Object/IR detected           -> WARNING
Sound > configured threshold -> WARNING
Fall event                   -> CRITICAL
Flame/IR event               -> CRITICAL
```

The HW511/TCRT5000 IR reflection sensor is **intentionally excluded from GLOBAL mode**. Outdoor testing demonstrated that strong environmental IR can make it report reflection detection and create nuisance alarms. It remains fully active as a sensing/telemetry subsystem and is actionable only from its dedicated `IR REFLECTION` page.

A validated fall remains system-level and can activate the physical alarm regardless of the selected page.

## System status

Critical status has priority over warning. Motion telemetry does not directly change system status.

## I²C ownership

`Sensors.cpp` owns initialization of the shared I²C bus:

```text
Wire.begin(GPIO21, GPIO22)
Wire.setClock(100000)
```

`DisplayUI.cpp` uses the configured `Wire` object without reinitializing the bus. This prevents the earlier duplicate-I²C-initialization problem.

## Dashboard

The ESP32 provides a local Wi-Fi dashboard at `vigil01.local` when the client is connected to the VIGIL-01 AP. `/data` exposes environmental values, heartbeat values, investigation sensors, motion telemetry, motion state, fall state, overall status, settings, and alert state. Browser refresh is 1000 ms.

The dashboard now exposes all existing persistent settings:

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

## Automated build

GitHub Actions compiles the `firmware/` sketch on pushes and pull requests using the ESP32 Arduino core and the required libraries.

## Other V1 scope

BME280, GPS, SD storage, cloud telemetry, historical database, and battery monitoring are not active V1.4 subsystems. GPIO39 remains reserved for future battery monitoring.

## Validation

1. Boot at 115200 baud.
2. Confirm the Serial message identifies the MPU family and I²C address.
3. Disconnect the motion module during operation and confirm `mpuPresent` becomes false.
4. Restore the connection and confirm the IMU can recover without rebooting.
5. Leave the unit still and confirm acceleration magnitude is near 9.8 m/s².
6. Rotate it and verify tilt and gyro values change.
7. Walk/run normally and confirm motion telemetry changes without an alarm.
8. PAGE + IR REFLECTION + TCRT5000 detection -> alarm.
9. PAGE + unrelated sensor condition -> no alarm.
10. GLOBAL + IR reflection only -> no alarm.
11. GLOBAL + defined system alarm -> alarm.
12. Test controlled fall-like sequences only in a safe setup; confirm the staged detector rather than a raw impact is what produces a fall event.
