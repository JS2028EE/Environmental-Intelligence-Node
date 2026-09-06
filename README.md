# VIGIL-01 — Portable Environmental Intelligence Node

VIGIL-01 is a handheld, ESP32-based environmental and situational sensing instrument designed to provide live, local information through a compact OLED interface and a local Wi-Fi dashboard.

> **Project status:** V1 breadboard prototype / live-sensing development
>
> **Current power method:** ESP32 powered by USB during development
>
> **V1 scope:** Live data only. No SD card, historical data logging, cloud database, or cloud storage.

## Purpose

The goal of VIGIL-01 is to combine several inexpensive sensors into one coherent engineering instrument rather than building a collection of unrelated sensor demos. The device is intended for experimentation, environmental awareness, close-range investigation, and learning about embedded systems, signal processing, hardware integration, and human-centered instrument design.

VIGIL-01 is an educational prototype. It is **not** a medical device, certified fire detector, calibrated laboratory instrument, or life-safety system.

## V1 Sensor Architecture

### Environment
- DHT11 — temperature and humidity
- Photoresistor — relative ambient light level
- Sound sensor — relative acoustic signal level
- Flame/IR sensor — strong IR/flame-like event detection

### Vitals
- HW502 heartbeat sensor — optical pulse signal / heart-rate estimation

### Investigate
- HW511 / TCRT5000 — short-range IR reflectivity
- IR obstacle sensor — short-range object detection
- 49E linear Hall-effect module — relative magnetic-field response and pole indication
- Water sensor — relative water/wetness signal
- Flame/IR sensor — intentional close-range IR investigation

### System
- Battery monitoring is planned for the battery-powered revision but is **not connected in the current USB-powered prototype**.
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
4. System

The interface is implemented as a state machine with software button debouncing.

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
| 21 | OLED SDA |
| 22 | OLED SCL (`SCK` on the user's OLED module) |
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

## Breadboard Power During V1 Development

For the current prototype:

```text
Computer USB
     |
     v
ESP32 USB
     |
     +---- 3V3 rail ---- sensors / OLED
     |
     +---- GND rail ---- common ground
```

The battery and charger are deliberately excluded from the current prototype until the final power architecture is designed and verified.

## Current OLED Wiring

```text
OLED VCC -> ESP32 3V3
OLED GND -> ESP32 GND
OLED SDA -> GPIO21
OLED SCK/SCL -> GPIO22
```

## Buttons

Each button is connected from its GPIO to GND and uses the ESP32's internal pull-up:

```text
GPIO16 -> UP -> GND
GPIO17 -> DOWN -> GND
GPIO18 -> SELECT -> GND
GPIO19 -> BACK -> GND
```

Released = HIGH; pressed = LOW.

## Indicators

- Green LED: normal/stable state
- Red LED: warning/attention state
- Passive buzzer: event/alert tones

The LEDs use 330 Ω series resistors.

## Firmware

The current V1 firmware is located at:

`firmware/vigil01_v1.ino`

The firmware provides:

- OLED boot and home screens
- menu navigation
- sensor acquisition
- basic heartbeat signal processing
- basic system-state interpretation
- LED/buzzer status outputs
- local Wi-Fi access point
- live browser dashboard
- JSON live-data endpoint

## Libraries

Install through Arduino IDE Library Manager:

- Adafruit GFX Library
- Adafruit SSD1306
- DHT sensor library by Adafruit
- Adafruit Unified Sensor

These are built into the ESP32 Arduino environment and require no separate installation:

- Wire
- WiFi
- WebServer

## Development Method

VIGIL-01 is being developed incrementally:

1. ESP32 + OLED + buttons
2. Environment sensors
3. Investigation sensors
4. Heartbeat sensor
5. LEDs + buzzer
6. Sensor characterization and calibration
7. Battery/regulator architecture
8. Final schematic
9. PCB/enclosure
10. Validation and documented revision

The design intentionally avoids wiring the entire system at once so that failures can be isolated and measured.

## Engineering Notes

Cheap sensor modules are treated as raw sensing hardware, not automatically as calibrated instruments. Unless a sensor is characterized and calibrated, values are reported as relative/raw measurements or estimates.

The V1 heartbeat feature is a signal-processing experiment and heart-rate estimate, not medical diagnosis. The flame/IR sensor is not a certified fire alarm.

## Repository Structure

```text
Environmental-Intelligence-Node/
├── README.md
├── firmware/
│   └── vigil01_v1.ino
└── docs/
    ├── HARDWARE.md
    ├── FIRMWARE.md
    └── DEVELOPMENT_LOG.md
```

## Roadmap

- [x] Define VIGIL-01 V1 concept
- [x] Select V1 sensors
- [x] Define four-button UI
- [x] Assign ESP32 GPIOs
- [x] Confirm OLED interface
- [x] Build USB-powered breadboard prototype
- [x] Connect passive buzzer to GPIO4
- [ ] Validate every sensor's actual module voltage/output behavior
- [ ] Characterize analog sensors
- [ ] Improve heartbeat signal processing
- [ ] Add robust sensor fault detection
- [ ] Finalize battery power architecture
- [ ] Design schematic
- [ ] Design PCB
- [ ] Build enclosure
- [ ] Perform validation testing

## Project Philosophy

VIGIL-01 is being built as an engineering project, not just a collection of modules. Every design choice should have a reason, every sensor should be characterized, failures should be documented, and later revisions should be based on measured results.
