# VIGIL-01 — Portable Environmental Intelligence Node

VIGIL-01 is a handheld, ESP32-based environmental and situational sensing instrument designed to provide live local information through a compact OLED interface and a local Wi-Fi dashboard.

> **Project status:** V1.4 solderless-breadboard prototype / live-sensing development
>
> **Current power:** USB-powered prototype; battery feasibility demonstrated separately
>
> **V1 scope:** Live data only — no SD history, cloud database, or cloud storage

## Purpose

VIGIL-01 combines inexpensive sensors into one coherent engineering instrument for experimentation, environmental awareness, close-range investigation, motion awareness, embedded programming, signal processing, networking, and human-centered instrument design.

VIGIL-01 is an educational prototype. It is not a medical device, certified fire detector, calibrated laboratory instrument, or life-safety system.

## Current firmware — V1.4

The modular firmware contains:

```text
firmware/
├── VIGIL01.ino
├── Config.h
├── Secrets.h.example
├── Types.h
├── Sensors.cpp / Sensors.h
├── Navigation.cpp / Navigation.h
├── MenuData.h
├── DisplayUI.cpp / DisplayUI.h
├── Alerts.cpp / Alerts.h
├── Settings.cpp / Settings.h
├── WebDashboard.cpp / WebDashboard.h
└── Watchdog.cpp / Watchdog.h
```

V1.4 includes the corrected MPU-9250/MPU-6500/MPU-9255-family motion subsystem, motion-state telemetry, staged fall detection, live IMU fault reporting, and a local dashboard settings interface.

## Sensor architecture

### Environment
- DHT11 — temperature/humidity
- Photoresistor — relative light
- Sound sensor — relative acoustic signal
- Flame/IR digital detection

### Vitals
- HW502 — optical pulse signal / experimental heart-rate estimate

### Investigate
- HW511/TCRT5000 — IR reflection/reflectivity
- IR obstacle sensor — object detection
- 49E linear Hall module — relative magnetic response
- Water sensor — relative wetness/water signal
- Flame/IR sensor — close-range IR investigation

### Motion
- MPU-9250 / MPU-6500 / MPU-9255-family module — 3-axis acceleration + 3-axis gyroscope
- Acceleration X/Y/Z
- Gyro X/Y/Z
- Acceleration magnitude
- Tilt angle
- Motion state
- Impact telemetry
- Fall-event state

The motion module shares the OLED I²C bus and consumes no additional GPIO.

## MPU-9250-family identification

Firmware reads `WHO_AM_I` and accepts:

```text
0x70 -> MPU-6500
0x71 -> MPU-9250
0x73 -> MPU-9255
```

The firmware probes both `0x68` and `0x69` and reports the detected ID/address over Serial at 115200 baud. If a live motion read fails, `mpuPresent` is cleared and the firmware periodically attempts recovery, so the UI/dashboard cannot remain falsely stuck at `IMU OK` after a breadboard disconnect.

## Motion and fall behavior

V1.4 separates **movement telemetry** from **physical alarms**.

Walking, running, rotating, ordinary tilt, and an isolated acceleration spike do **not** trigger the buzzer/red alarm.

A fall requires:

```text
LOW-G / FREE-FALL
      ↓
HIGH-G IMPACT
      ↓
SUSTAINED POST-IMPACT TILT
      ↓
FALL EVENT / CRITICAL
```

Prototype parameters:

```text
Free-fall:          < 4.0 m/s²
Impact:             > 25.0 m/s²
Post-impact tilt:   > 45°
Sequence window:    1200 ms
Tilt confirmation:  300 ms
Alert hold:         3000 ms
```

The physical alert presentation uses a distinct rapid tone for a validated fall and the standard critical tone for other critical conditions such as flame/IR.

## Alarm routing

VIGIL-01 separates sensor detection from alarm policy.

### PAGE mode

Only the alarm condition belonging to the currently selected page can activate the physical alarm:

```text
SOUND page          -> sound threshold
WATER page          -> water threshold
OBJECT page         -> IR obstacle detection
FLAME page          -> flame/IR event
IR REFLECTION page  -> TCRT5000 detection
```

An unrelated sensor condition must not activate the alarm on another page.

### GLOBAL mode

Defined system-wide conditions activate the physical alarm:

```text
Water threshold exceeded -> WARNING
Object/IR detected       -> WARNING
Sound threshold exceeded -> WARNING
Fall event               -> CRITICAL
Flame/IR event           -> CRITICAL
```

The **TCRT5000 IR reflection sensor is intentionally excluded from GLOBAL mode**. Outdoor testing showed that sunlight/ambient infrared can make the reflection sensor report detection, producing nuisance alarms. It remains fully available on its investigation page.

A validated fall remains system-level and can activate the physical alarm regardless of the selected page.

## Orientation vs. position

The IMU can report orientation and movement, but it cannot maintain reliable absolute 3D position indefinitely from acceleration/gyro integration alone because drift accumulates. V1.4 therefore reports how the device is moving and oriented rather than claiming a precise absolute position.

## User interface

0.96-inch 128×64 SSD1306 OLED, four buttons:

- UP
- DOWN
- SELECT
- BACK

Top-level menu:

1. Environment
2. Vitals
3. Investigate
4. Motion
5. System

## Current ESP32 pin map

| GPIO | Function |
|---:|---|
| 4 | Passive buzzer |
| 13 | Green LED |
| 14 | Red LED |
| 16 | UP button |
| 17 | DOWN button |
| 18 | SELECT button |
| 19 | BACK button |
| 21 | I²C SDA — OLED + motion module |
| 22 | I²C SCL — OLED + motion module |
| 23 | Flame/IR digital signal |
| 25 | DHT11 data |
| 26 | TCRT5000 signal |
| 27 | IR obstacle OUT |
| 32 | Heartbeat analog |
| 33 | Photoresistor divider |
| 34 | Sound analog |
| 35 | Hall A0 |
| 36 | Water analog |
| 39 | Reserved battery monitor |

## Dashboard

VIGIL-01 provides a local Wi-Fi AP:

```text
SSID: VIGIL-01
Password: provisioned locally in firmware/Secrets.h
mDNS: vigil01.local
```

`/data` exposes environmental, vital, investigation, motion, fall, system-status, settings, and alert-state telemetry. Browser polling is 1000 ms.

The dashboard now exposes all existing persistent settings:

- LEDs ON/OFF
- Buzzer ON/OFF
- Alerts PAGE/GLOBAL
- Units C/F
- Sound threshold
- Water threshold

## Security and repository hygiene

The AP password is intentionally not committed to the public repository. Copy `firmware/Secrets.h.example` to `firmware/Secrets.h` locally and set the deployment password. `firmware/Secrets.h` is ignored by Git. See `docs/SECURITY.md`.

The repository also includes `.gitignore` for local secrets/build artifacts and an MIT `LICENSE`.

## Battery monitoring

GPIO39 and the battery-divider calculation are reserved in the firmware. The current repository keeps `BATTERY_MONITORING_ENABLED` disabled while the battery power architecture and characterization are still being finalized. When enabled, the existing 2:1 divider calculation reports a **rough percentage estimate**, not a calibrated state-of-charge measurement.

## Firmware libraries

Required through Arduino Library Manager:

- Adafruit GFX Library
- Adafruit SSD1306
- DHT sensor library by Adafruit
- ArduinoJson 6.x

The MPU-9250 family is accessed directly through `Wire`; no Adafruit MPU6050 or Adafruit Unified Sensor library is required.

## Hardware development method

VIGIL-01 is intentionally being developed on a solderless breadboard before permanent hardware:

```text
Breadboard validation
  ↓
Sensor characterization
  ↓
Schematic
  ↓
PCB layout
  ↓
PCB fabrication
  ↓
Soldered assembly
  ↓
Enclosure
  ↓
Final validation
```

## Scope exclusions

BME280, GPS, SD storage, cloud telemetry, and cloud storage are not active V1.4 subsystems. Battery monitoring circuitry is being developed separately and is not yet enabled in the tracked firmware.

## Engineering record

The development record includes the MPU identification/debugging cycle, shared-I²C correction, motion false-alarm redesign, outdoor IR-reflection incident, live IMU fault handling, dashboard settings improvement, credential separation, and automated firmware build checking.

## Roadmap

- [x] Define VIGIL-01 concept
- [x] Select V1 sensors
- [x] Define UI and GPIO map
- [x] Add motion subsystem
- [x] Identify MPU-9250-family silicon through `WHO_AM_I`
- [x] Separate motion telemetry from alarm behavior
- [x] Add staged fall detection
- [x] Add local dashboard and persistent settings
- [x] Add watchdog support
- [x] Document breadboard and battery feasibility work
- [x] Diagnose outdoor TCRT5000 reflection nuisance alarm
- [x] Implement PAGE-only TCRT5000 alarm routing
- [x] Separate deployment credentials from tracked source
- [x] Add live IMU communication fault reporting
- [x] Expose existing dashboard settings controls
- [x] Add automated firmware build workflow
- [ ] Characterize all sensor outputs
- [ ] Validate fall detector with controlled tests
- [ ] Validate PAGE/GLOBAL alarm matrix
- [ ] Improve sensor fault detection across all sensors
- [ ] Finalize battery architecture
- [ ] Design schematic
- [ ] Design PCB
- [ ] Fabricate and assemble PCB
- [ ] Build enclosure
- [ ] Perform final validation

## Project philosophy

VIGIL-01 is an engineering project, not just a collection of modules. Design decisions should be justified, raw sensors should be characterized, failures should be documented, and revisions should be based on measured results.
