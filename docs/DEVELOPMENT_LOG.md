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

### Removed from V1

The MPU6050 was intentionally removed. Motion sensing was not considered valuable enough for this version's objective.

### UI architecture

The 0.96-inch 128×64 SSD1306 OLED uses I²C on GPIO21/GPIO22. Four buttons provide UP/DOWN/SELECT/BACK navigation.

Top-level menus:

- Environment
- Vitals
- Investigate
- System

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

### Specific module confirmations

**HW511**

The physical module has `V+`, `G`, and `S` pins.

```text
V+ -> 3V3
G  -> GND
S  -> GPIO26
```

**IR obstacle sensor**

The physical module has `GND`, `VCC`, `OUT`, and `EN`.

Initial connection:

```text
GND -> GND
VCC -> 3V3
OUT -> GPIO27
EN  -> not connected during initial test
```

The EN polarity is not assumed until tested.

**49E linear Hall module**

The selected Hall module exposes `GND`, `+`, `A0`, and `D0`. V1 uses A0 for continuous relative magnetic-field information and leaves D0 unused.

```text
G   -> GND
+   -> 3V3
A0  -> GPIO35
D0  -> unused
```

### Power decision

The current breadboard is powered from the ESP32 USB connection. The battery and charger are postponed until a compact regulated power architecture is selected.

A resistor is not being used as a voltage regulator. Resistors remain reserved for functions such as the photoresistor voltage divider and LED current limiting.

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
