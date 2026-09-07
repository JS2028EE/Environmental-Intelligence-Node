# VIGIL-01 — V1.3 Firmware

This directory contains the active modular VIGIL-01 firmware. V1.3 builds on the earlier V1.1/V1.2 refactor and adds the GY-521 / MPU6050 motion subsystem while preserving the existing sensor GPIO assignments.

**Hardware note:** I do not have the physical board, so firmware changes cannot be bench-verified here. The new GY-521 diagnostics are designed to make the first physical test unambiguous.

---

## V1.3 Current Architecture

```text
VIGIL01.ino       Main setup/loop coordinator
Config.h          Pins, I2C addresses, thresholds, timing
Types.h           Shared screen/status types
Sensors.cpp/.h    Sensor acquisition + MPU6050 processing
MenuData.h        Menu tree
Navigation.cpp/.h Buttons/navigation/debounce
DisplayUI.cpp/.h OLED rendering
Alerts.cpp/.h     LED/buzzer presentation
Settings.cpp/.h   Persistent NVS settings
WebDashboard.cpp/.h Wi-Fi AP/dashboard/API/captive portal/mDNS
Watchdog.cpp/.h   Watchdog recovery
```

## GY-521 / MPU6050

The GY-521 is connected to the same I²C bus as the SSD1306 OLED:

```text
GY-521 VCC -> ESP32 3V3
GY-521 GND -> ESP32 GND
GY-521 SDA -> GPIO21
GY-521 SCL -> GPIO22
GY-521 INT -> NC
GY-521 AD0 -> GND for 0x68
```

If AD0 is HIGH, the address is `0x69`.

The firmware now:

- initializes the shared I²C bus once in `Sensors.cpp`
- uses 100 kHz I²C
- waits 50 ms after bus initialization
- explicitly passes `&Wire` to `mpu.begin()`
- probes both `0x68` and `0x69`
- prints the detected address at 115200 baud
- configures ±8 g acceleration, ±500 °/s gyro, and a 21 Hz filter
- calculates acceleration magnitude and tilt
- detects motion, impact, and tilt events

### Important I²C fix

The previous V1.3 source had a shared-bus initialization flaw: `Sensors.cpp` initialized `Wire` for the MPU6050 and then `DisplayUI.cpp` called `Wire.begin()` again for the OLED. Since the OLED and GY-521 share GPIO21/GPIO22, the bus should have one owner.

V1.3 now makes `Sensors.cpp` the I²C owner. `DisplayUI.cpp` only initializes the OLED driver against the already-configured `Wire` object.

This is the main firmware-side defect identified during the GY-521 investigation.

### Boot diagnostics

Expected Serial output is:

```text
MPU6050/GY-521 detected at 0x68
```

or:

```text
MPU6050/GY-521 detected at 0x69
```

If the module is not found:

```text
ERROR: MPU6050/GY-521 not detected on I2C bus (0x68/0x69)
```

If the error appears, the next step is **not** to rewrite the sensor math. Check VCC, GND, SDA, SCL, AD0, breadboard contacts, and I²C voltage levels.

---

## Sensor and Alert Behavior

The existing V1 sensor set remains:

- DHT11
- Photoresistor
- Sound sensor
- HW502 heartbeat
- HW511/TCRT5000
- IR obstacle sensor
- 49E linear Hall sensor
- Water sensor
- Flame/IR sensor
- GY-521 / MPU6050

Current overall status logic:

```text
Water > threshold -> WARNING
Object detected   -> WARNING
Sound > threshold -> WARNING
Motion detected   -> WARNING
Tilt detected     -> WARNING
Impact detected   -> CRITICAL
Flame detected    -> CRITICAL
```

PAGE mode uses page-specific physical alarm conditions. GLOBAL mode uses the overall system status from any screen.

The MOTION screen is currently informational in PAGE mode; motion/tilt/impact still appear in the overall status and dashboard data.

---

## Settings

Persistent ESP32 NVS/flash settings:

```text
LEDS: ON/OFF
BUZZER: ON/OFF
ALERTS: PAGE/GLOBAL
UNITS: C/F
Sound threshold
Water threshold
```

Default sound/water thresholds are `135 / 2500`.

## Dashboard

The ESP32 hosts:

```text
SSID: VIGIL-01
Password: VIGIL01_2026
mDNS: vigil01.local
```

Endpoints:

```text
GET  /
GET  /data
GET  /api/settings
POST /api/settings
```

The `/data` response includes MPU6050 presence, acceleration, gyro, magnitude, tilt, motion/impact/tilt flags, system status, alert state, thresholds, settings, and the other sensor values.

## Watchdog

`WATCHDOG_ENABLED` is enabled by default with an 8-second timeout. The implementation handles ESP32 Arduino core watchdog API differences.

## Libraries

Install:

- Adafruit GFX Library
- Adafruit SSD1306
- DHT sensor library by Adafruit
- Adafruit MPU6050
- Adafruit Unified Sensor
- ArduinoJson 6.x

The ESP32 core provides Wire, WiFi, WebServer, DNSServer, ESPmDNS, Preferences, and watchdog support.

## BME280 / GPS Status

BME280 and GPS are **not part of V1.3**. They have no active sensor implementation in the current firmware.

## Validation

Because the firmware cannot be physically flashed from this repository session, the first GY-521 validation should be performed with Serial Monitor at 115200 baud.

1. Power the board.
2. Confirm the GY-521 detection message.
3. If detected at `0x68` or `0x69`, open the MOTION screen.
4. With the board stationary, acceleration magnitude should be near 9.8 m/s².
5. Rotate/move the board and confirm X/Y/Z and tilt change.
6. If neither address is detected, inspect the physical I²C connection before modifying software again.
