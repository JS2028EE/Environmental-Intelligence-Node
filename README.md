# VIGIL-01 — Portable Environmental Intelligence Node

VIGIL-01 is a handheld, ESP32-based environmental and situational sensing instrument designed to provide live, local information through a compact OLED interface and a local Wi-Fi dashboard.

> **Project status:** V1.3 breadboard prototype / live-sensing development
>
> **Current development power:** USB-powered prototype, with a separate battery feasibility test documented in the development log
>
> **V1 scope:** Live data only. No SD card, historical data logging, cloud database, or cloud storage.

## Purpose

The goal of VIGIL-01 is to combine several inexpensive sensors into one coherent engineering instrument rather than building a collection of unrelated sensor demos. The device is intended for experimentation, environmental awareness, close-range investigation, motion awareness, and learning about embedded systems, signal processing, hardware integration, networking, and human-centered instrument design.

VIGIL-01 is an educational prototype. It is **not a medical device, certified fire detector, calibrated laboratory instrument, or life-safety system**.

## Current Firmware: V1.3

V1.3 uses a modular firmware architecture. `VIGIL01.ino` coordinates initialization and the main timing loop while dedicated modules handle sensors, navigation, display rendering, alerts, settings, web services, and watchdog servicing.

```text
firmware/
├── VIGIL01.ino
├── Config.h
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

The current firmware includes the **MPU6050 motion subsystem using the GY-521 module**. BME280 and GPS functionality is not part of V1.3.

## V1 Sensor Architecture

### Environment
- DHT11 — temperature and humidity
- Photoresistor — relative ambient light level
- Sound sensor — relative acoustic signal level
- Flame/IR sensor — strong IR/flame-like event detection

### Vitals
- HW502 heartbeat sensor — optical pulse signal / experimental heart-rate estimation

### Investigate
- HW511 / TCRT5000 — short-range IR reflectivity
- IR obstacle sensor — short-range object detection
- 49E linear Hall-effect module — relative magnetic-field response and pole indication
- Water sensor — relative water/wetness signal
- Flame/IR sensor — intentional close-range IR investigation

### Motion
- GY-521 / MPU6050 — 3-axis acceleration and 3-axis gyroscope
- Derived motion metrics: acceleration magnitude, tilt angle, motion event, impact event, and tilt event

The MPU6050 is connected through the existing I²C bus, so it does **not consume an additional GPIO**.

### System
- Battery monitoring remains disabled in firmware and GPIO39 remains reserved.
- Hardware/sensor status and system information are exposed through the UI.

## User Interface

VIGIL-01 uses a 0.96-inch 128×64 SSD1306 I²C OLED and four physical buttons:

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

The interface is implemented as a state machine with software button debouncing.

## GY-521 / MPU6050 Wiring

The GY-521 shares the OLED I²C bus:

```text
GY-521 VCC -> ESP32 3V3
GY-521 GND -> ESP32 GND
GY-521 SDA -> GPIO21
GY-521 SCL -> GPIO22
GY-521 INT -> not connected
GY-521 AD0 -> GND for I²C address 0x68
```

If AD0 is HIGH, the device uses address `0x69`. V1.3 firmware probes both `0x68` and `0x69`.

```text
ESP32 GPIO21 (SDA)
       ├── OLED SDA
       └── GY-521 SDA

ESP32 GPIO22 (SCL)
       ├── OLED SCL
       └── GY-521 SCL
```

The firmware initializes the I²C bus explicitly at 100 kHz, waits briefly for sensor power-up, probes both MPU6050 addresses, and prints the detected address or an error to Serial at 115200 baud.

### MPU6050 processing

V1.3 configures:

- Accelerometer range: ±8 g
- Gyroscope range: ±500 °/s
- Digital filter bandwidth: 21 Hz
- Motion threshold: 1.5 m/s² deviation from nominal gravity magnitude
- Impact threshold: 25.0 m/s²
- Tilt threshold: 30°

These are prototype engineering thresholds, not calibrated safety limits.

## Current ESP32 Pin Map

| GPIO | Function |
|---:|---|
| 4 | Passive buzzer |
| 13 | Green status LED |
| 14 | Red status LED |
| 16 | UP button |
| 17 | DOWN button |
| 18 | SELECT button |
| 19 | BACK button |
| 21 | I²C SDA — OLED + GY-521 |
| 22 | I²C SCL — OLED + GY-521 |
| 23 | Flame/IR digital signal |
| 25 | DHT11 data |
| 26 | HW511 signal (`S`) |
| 27 | IR obstacle sensor `OUT` |
| 32 | HW502 heartbeat analog output |
| 33 | Photoresistor divider node |
| 34 | Sound sensor analog output |
| 35 | 49E linear Hall module `A0` |
| 36 | Water sensor analog output |
| 39 | Reserved for future battery monitoring |

The analog inputs are placed on ESP32 ADC1 GPIOs so they remain compatible with Wi-Fi operation.

## Alerts and Status

The current system status is:

```text
NORMAL
WARNING
CRITICAL
```

Current interpreted conditions include:

```text
Water above configured threshold -> WARNING
Object/IR detection             -> WARNING
Sound above configured threshold -> WARNING
Motion or tilt event             -> WARNING
Impact event                     -> CRITICAL
Flame/IR event                   -> CRITICAL
```

Critical status has priority over warning status.

Physical alert presentation is controlled separately from sensing through the PAGE/GLOBAL alert setting. This prevents an unrelated event from automatically making every displayed sensor page appear to be in alarm when PAGE mode is selected.

## Persistent Settings

User settings are stored in ESP32 NVS/flash and survive reboot.

Current settings:

- LEDs ON/OFF
- Buzzer ON/OFF
- Alerts PAGE/GLOBAL
- Units C/F
- Sound threshold
- Water threshold

Default thresholds are:

```text
Sound = 135
Water = 2500
```

## Web Dashboard

V1.3 provides a local Wi-Fi access point and browser dashboard.

```text
SSID: VIGIL-01
Password: VIGIL01_2026
mDNS: vigil01.local
```

The dashboard exposes live sensor data and settings controls. The `/data` endpoint includes environmental, heartbeat, investigation, MPU6050, system-status, threshold, alert-mode, and output-setting fields.

The device also uses a captive-portal DNS service so common client connectivity checks can be redirected to the dashboard.

## Watchdog

The watchdog is enabled with an 8-second timeout. The firmware supports the relevant ESP-IDF watchdog API differences across ESP32 Arduino core generations.

The main loop feeds the watchdog during normal operation so a stuck application can be recovered by a hardware/software reset path.

## Breadboard Power and Battery Feasibility

The normal prototype configuration remains USB powered while the electrical architecture is being validated.

A separate bench experiment demonstrated portable-power feasibility using an **XTR 502030 3.7 V 200 mAh Li-ion cell (0.74 Wh)** through the prototype's power-conversion path. This experiment is documented as a feasibility result, not yet as the final battery architecture.

The final PCB power system will be selected and validated before permanent battery integration.

## Hardware Development Method

VIGIL-01 is currently being developed on a **solderless breadboard**. This is an intentional prototype stage, not the final hardware construction.

The breadboard allows the engineering team to quickly change wiring, replace modules, test individual circuits, characterize sensors, and debug electrical or firmware problems before committing the design to a permanent PCB.

The planned hardware progression is:

```text
Concept
  ↓
Solderless Breadboard Prototype
  ↓
Sensor Characterization / Testing
  ↓
Schematic Finalization
  ↓
PCB Layout
  ↓
PCB Fabrication
  ↓
Soldered Assembly
  ↓
Enclosure / Final Device
  ↓
Final Validation
```

## Firmware Libraries

Install through Arduino IDE Library Manager:

- Adafruit GFX Library
- Adafruit SSD1306
- DHT sensor library by Adafruit
- Adafruit MPU6050
- Adafruit Unified Sensor
- ArduinoJson 6.x

Provided by the ESP32 Arduino environment:

- Wire
- WiFi
- WebServer
- DNSServer
- ESPmDNS

## Engineering Notes

Cheap sensor modules are treated as raw sensing hardware, not automatically as calibrated instruments. Unless a sensor is characterized and calibrated, values are reported as relative/raw measurements or estimates.

The V1 heartbeat feature is a signal-processing experiment and heart-rate estimate, not medical diagnosis. The flame/IR sensor is not a certified fire alarm. MPU6050 motion/tilt thresholds are prototype detection rules rather than safety specifications.

## Development Method

VIGIL-01 is being developed incrementally:

1. ESP32 + OLED + buttons
2. Environment sensors
3. Investigation sensors
4. Heartbeat sensor
5. LEDs + buzzer
6. MPU6050/GY-521 motion subsystem
7. Sensor characterization and calibration
8. Breadboard validation
9. Power architecture validation
10. Final schematic
11. PCB design and fabrication
12. Soldered assembly
13. Enclosure
14. Final validation and documented revision

The design intentionally avoids wiring the entire system at once so that failures can be isolated and measured.

## Repository Structure

```text
Environmental-Intelligence-Node/
├── README.md
├── firmware/
│   ├── VIGIL01.ino
│   ├── Config.h
│   ├── Types.h
│   ├── Sensors.cpp
│   ├── Sensors.h
│   ├── Navigation.cpp
│   ├── Navigation.h
│   ├── MenuData.h
│   ├── DisplayUI.cpp
│   ├── DisplayUI.h
│   ├── Alerts.cpp
│   ├── Alerts.h
│   ├── Settings.cpp
│   ├── Settings.h
│   ├── WebDashboard.cpp
│   ├── WebDashboard.h
│   ├── Watchdog.cpp
│   └── Watchdog.h
└── docs/
    ├── HARDWARE.md
    ├── FIRMWARE.md
    ├── DEVELOPMENT_LOG.md
    └── V1_1_UPDATE.md
```

## Roadmap

- [x] Define VIGIL-01 V1 concept
- [x] Select V1 sensors
- [x] Define four-button UI
- [x] Assign ESP32 GPIOs
- [x] Confirm OLED interface
- [x] Add MPU6050/GY-521 motion subsystem
- [x] Connect passive buzzer to GPIO4
- [x] Implement modular V1.3 firmware
- [x] Implement persistent settings
- [x] Implement local dashboard, captive portal, and mDNS
- [x] Implement watchdog support
- [x] Demonstrate battery-power feasibility on the bench
- [ ] Validate GY-521 physical wiring and I²C detection on the final breadboard assembly
- [ ] Validate every sensor's actual module voltage/output behavior
- [ ] Characterize analog sensors
- [ ] Improve heartbeat signal processing
- [ ] Add robust sensor fault detection
- [ ] Finalize battery power architecture
- [ ] Design schematic
- [ ] Design PCB
- [ ] Fabricate PCB
- [ ] Solder permanent hardware assembly
- [ ] Build enclosure
- [ ] Perform final validation and document revision

## Project Philosophy

VIGIL-01 is being built as an engineering project, not just a collection of modules. Every design choice should have a reason, every sensor should be characterized, failures should be documented, and later revisions should be based on measured results.

The breadboard is the **prototype and validation platform**. The planned PCB and soldered assembly are the next hardware revision, not a replacement for the documented prototype history. This distinction is intentional so the project record shows how the design evolved from experimental wiring into permanent hardware.
