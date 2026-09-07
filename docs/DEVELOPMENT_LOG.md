# VIGIL-01 Development Log

## 2026-09-05 — V1 Architecture Locked

### Project direction

The project was formalized as **VIGIL-01 — Portable Environmental Intelligence Node**. The objective is to build a coherent portable sensing instrument rather than a collection of unrelated sensor demonstrations.

### V1 scope

V1 is live/in-the-moment only:

- No SD card
- No historical data logging
- No cloud database
- No cloud storage
- Local Wi-Fi dashboard remains available for live telemetry

### Selected sensing functions

**Environment**
- DHT11 temperature/humidity
- Photoresistor relative light
- Sound sensor relative acoustic signal
- Flame/IR detection

**Vitals**
- HW502 heartbeat/pulse sensing

**Investigate**
- HW511/TCRT5000 IR reflectivity
- IR obstacle detection
- 49E linear Hall-effect magnetic sensing
- Water sensing
- Flame/IR investigation

### Removed from original V1

The MPU6050 was intentionally removed in the original V1 architecture. That decision was later reversed during V1.3 development when motion sensing was brought back as a dedicated subsystem on the existing I²C bus.

### UI architecture

The 0.96-inch 128×64 SSD1306 OLED uses I²C on GPIO21/GPIO22. Four buttons provide UP/DOWN/SELECT/BACK navigation.

Top-level menus in the original architecture were Environment, Vitals, Investigate, and System. V1.3 later added Motion.

### Hardware decisions recorded

- OLED: SDA GPIO21, SCL GPIO22
- UP: GPIO16
- DOWN: GPIO17
- SELECT: GPIO18
- BACK: GPIO19
- DHT11 data: GPIO25
- HW511 signal: GPIO26
- IR obstacle OUT: GPIO27
- Flame/IR digital output: GPIO23
- Heartbeat analog: GPIO32
- Light divider: GPIO33
- Sound analog: GPIO34
- Linear Hall A0: GPIO35
- Water analog: GPIO36
- Green LED: GPIO13 through 330 Ω
- Red LED: GPIO14 through 330 Ω
- Passive buzzer: GPIO4 to GND
- GPIO39 reserved for future battery monitoring

### Power decision

The current breadboard is powered from the ESP32 USB connection. Battery development was postponed until a compact regulated power architecture could be selected.

A later bench experiment demonstrated battery-power feasibility using an XTR 502030 3.7 V 200 mAh Li-ion cell through the prototype power-conversion path. This remains a feasibility result rather than the final PCB power architecture.

### Development methodology

The prototype is being assembled incrementally so faults can be isolated:

1. ESP32 + OLED + buttons
2. Environment sensors
3. Investigation sensors
4. Heartbeat
5. LEDs/buzzer
6. Characterization
7. Battery/regulator
8. Schematic
9. PCB/enclosure
10. Validation

### Engineering principle

Do not claim precision that has not been measured. Raw sensor values remain raw/relative until calibration and characterization are performed.

## 2026-09-05 — V1.1 Alert-System Debugging and Correction

### Trigger for revision

Integrated breadboard testing exposed an alert-presentation bug. Entering a sensor page could cause the red LED and buzzer to activate even when the displayed sensor was normal. The green LED also did not remain continuously asserted during normal operation.

### Root cause identified

The first V1.1 implementation mixed **overall system status** with **page-specific physical alert presentation**. In PAGE mode, an unrelated sensor condition could therefore make the currently displayed page appear to be in alarm.

Digital IR/flame-related modules were also being interpreted without an explicit polarity configuration. Common comparator modules are often active-low, so a LOW output may represent detection rather than a HIGH output.

### Corrected architecture

V1.1 separated:

```text
SENSING
  ↓
INTERPRETATION
  ↓
PRESENTATION
```

Sensors continue being sampled regardless of the current screen. The firmware determines overall status continuously, while the physical LED/buzzer presentation is gated by the selected alert mode.

## 2026-09-05 — Flame Sensor Polarity Reversal Corrected

Physical testing later showed that the actual flame sensor module used in the prototype had the opposite polarity from the earlier assumption. The current firmware configuration now uses:

```text
FLAME_ACTIVE_LOW = false
LOW  = CLEAR
HIGH = FLAME/IR EVENT
```

The older V1.1 active-low statement is retained only as a historical record and is superseded by the current configuration.

## 2026-09-07 — V1.3 Documentation and GY-521 I²C Debugging

### Documentation audit

The repository was audited against the active modular V1.3 firmware. The README, hardware architecture, firmware architecture, and V1.1 update record were corrected so they no longer describe the obsolete single-file firmware or claim that the MPU6050 is excluded.

Current documentation now records:

- V1.3 modular firmware architecture
- GY-521 / MPU6050 wiring and shared I²C bus
- MPU6050 ranges, filtering, derived motion metrics, and thresholds
- current five-section menu structure including MOTION
- persistent NVS/flash settings
- local dashboard, `/data`, settings API, captive portal, and mDNS
- watchdog operation
- current flame-sensor polarity
- BME280/GPS absence from V1.3
- battery feasibility experiment

### GY-521 failure investigation

The first V1.3 sensor implementation did initialize `Wire` and attempt an MPU6050 probe, but the shared I²C bus was not architecturally owned by one module. `Sensors.cpp` initialized `Wire` for the MPU6050, then `DisplayUI.cpp` initialized `Wire` again when starting the OLED.

That duplicate bus initialization occurred **after** the MPU6050 had already been probed. It is a real firmware design flaw because the OLED and MPU6050 share the same bus and should not independently reinitialize it.

The V1.3 correction makes `Sensors.cpp` the owner of I²C initialization:

```text
Wire.begin(GPIO21, GPIO22)
Wire.setClock(100000)
```

`DisplayUI.cpp` now attaches the OLED to the already-initialized `Wire` object without calling `Wire.begin()` again.

The MPU6050 driver is also called explicitly as:

```text
mpu.begin(address, &Wire)
```

and both `0x68` and `0x69` are tested.

A 50 ms startup settling delay was added before the probe, and Serial now reports the detected address or a clear failure message at 115200 baud.

### What this proves — and what it does not

The duplicate I²C initialization was a genuine firmware bug and has been removed. The previous firmware also lacked sufficient diagnostics to distinguish a software read problem from a physical I²C problem.

However, the repository cannot prove from source code alone that duplicate `Wire.begin()` was the only reason the physical GY-521 produced no data. If the corrected firmware still reports that neither `0x68` nor `0x69` is detected, the remaining fault is almost certainly in the physical I²C path or module configuration and must be checked at the breadboard.

The new boot diagnostics make that distinction explicit.

### Current GY-521 validation procedure

1. Connect GY-521 VCC to the intended supply and GND to ESP32 GND.
2. Connect SDA to GPIO21 and SCL to GPIO22.
3. Hold AD0 LOW for address `0x68`, or HIGH for `0x69`.
4. Open Serial Monitor at 115200 baud.
5. Confirm the boot message reports `MPU6050/GY-521 detected at 0x68` or `0x69`.
6. Open the MOTION screen or `/data` endpoint.
7. With the board stationary, acceleration magnitude should be near gravitational acceleration and should change when the device is rotated or moved.
8. If neither address is detected, inspect wiring, power, common ground, AD0, I²C voltage levels, breadboard contacts, and the GY-521 itself before changing the sensor algorithm again.

This debugging cycle is retained as part of the engineering record because it exposed a shared-resource initialization problem and improved the firmware's ability to distinguish software faults from hardware faults.
