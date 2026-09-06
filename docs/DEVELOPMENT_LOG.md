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

## 2026-09-05 — V1.1 Alert-System Debugging and Correction

### Trigger for revision

Integrated breadboard testing exposed an alert-presentation bug. Entering a sensor page could cause the red LED and buzzer to activate even when the displayed sensor was normal. The green LED also did not remain continuously asserted during normal operation.

### Root cause identified

The first V1.1 implementation mixed **overall system status** with **page-specific physical alert presentation**. In PAGE mode, an unrelated sensor condition could therefore make the currently displayed page appear to be in alarm.

Digital IR/flame-related modules were also being interpreted without an explicit polarity configuration. Common comparator modules are often active-low, so a LOW output may represent detection rather than a HIGH output.

### Corrected architecture

V1.1 now separates three stages:

```text
SENSING
  ↓
INTERPRETATION
  ↓
PRESENTATION
```

Sensors continue being sampled regardless of the current screen. The firmware determines overall status continuously, while the physical LED/buzzer presentation is gated by the selected alert mode.

**PAGE mode:** only the danger condition associated with the currently displayed sensor/condition page can activate the physical alert outputs.

**GLOBAL mode:** any defined system danger/event can activate the physical alert outputs from any screen.

Opening a sensor page is never itself treated as an alert.

### Normal-state output model

When LEDs are enabled and no active alert exists:

```text
GREEN = solid ON
RED   = OFF
BUZZER = silent
```

During a real warning or critical event:

```text
GREEN = OFF
RED   = flashing
BUZZER = active
```

If LEDs are disabled, both LEDs remain off. If the buzzer is disabled, both navigation and alarm tones are suppressed while sensor detection continues.

### Defined V1.1 alert conditions

```text
FLAME/IR event -> CRITICAL
WATER > 2500   -> WARNING
OBJECT detected -> WARNING
SOUND > 135    -> WARNING
Otherwise      -> NORMAL
```

These are prototype thresholds based on raw/relative sensor behavior and are not calibrated safety limits.

### Sound characterization

Bench testing showed the sound sensor normally moving around 0–100, while a clap produced a substantially larger response. A prototype trigger of:

```text
soundRaw > 135
```

was therefore selected for the current revision. This value is an ADC threshold, not a measurement in dB SPL.

### Navigation feedback

UP, DOWN, SELECT, and BACK now generate short audible feedback tones. The existing software debounce remains in place. The buzzer enable setting controls both navigation feedback and alarm tones.

### Digital polarity configuration

The corrected firmware makes digital detection polarity explicit through configuration constants:

```text
TCRT_ACTIVE_LOW
IR_ACTIVE_LOW
FLAME_ACTIVE_LOW
```

The actual module behavior must still be verified during breadboard characterization because inexpensive module variants can differ. Keeping polarity configurable prevents a module-specific wiring characteristic from being hidden inside the alert logic.

### Sensor-module trimmer characterization

Onboard potentiometers found on several inexpensive modules are treated as comparator sensitivity/digital switching-threshold adjustments unless proven otherwise. They are not assumed to calibrate the analog physical quantity. Firmware thresholds therefore remain explicitly documented separately from hardware trimmer settings.

### GitHub implementation records

The corrected V1.1 firmware was committed as:

`af3aa0e1547c0d110f905c8215a4339447afe42c`

The V1.1 engineering update documentation was committed as:

`5ee93cfafce40cb516b85de7e87a8667b9579730`

The hardware-stage documentation was also updated to record the current solderless-breadboard stage and the planned transition to a custom PCB and soldered assembly.

### Validation status

The next breadboard test is intended to verify:

- Green LED remains solid during normal operation.
- Red LED remains off when no relevant alert exists.
- Buzzer remains silent during normal operation.
- Entering a normal sensor page does not create an alert.
- Unrelated sensor activity does not trigger PAGE-mode alarms.
- Defined warning/critical conditions activate the correct outputs.
- GLOBAL mode activates defined alerts regardless of the current page.
- Actual digital sensor polarity matches the configured active-low assumptions.
- The live dashboard remains functional.

### Engineering significance

This debugging cycle is being retained as part of the project record rather than treating the bug as an implementation detail. The failure exposed an architectural coupling between sensing and presentation; the fix establishes a clearer separation that should scale better when additional sensors, thresholds, and future PCB hardware are introduced.

## 2026-09-05 — Flame Sensor Polarity Reversal Corrected

### Trigger for revision

Physical testing of the VIGIL-01 flame sensor showed that the previously configured interpretation was reversed for the actual module being used. The observed behavior was:

```text
FLAME PRESENT -> no alarm
NO FLAME      -> alarm
```

This occurred when viewing the Flame/IR page and also when using GLOBAL/automatic alert mode.

### Root cause

The firmware's flame input polarity did not match the electrical behavior of the physical module. The module's digital output was being interpreted as though its active state were opposite to the state observed during bench testing.

### Correction

The firmware now explicitly records the validated flame-module behavior:

```text
CLEAR            = HIGH
FLAME/IR EVENT   = LOW
```

Therefore:

```cpp
const bool FLAME_ACTIVE_LOW=true;
```

and the sensor read is interpreted as:

```cpp
flameDetected = FLAME_ACTIVE_LOW ? (flameRaw == LOW) : (flameRaw == HIGH);
```

This corrected polarity feeds the existing alert pipeline, so the same physical event now drives the correct CRITICAL status in both PAGE and GLOBAL modes.

### Validation plan

The next firmware test must verify both states explicitly:

1. No flame near the sensor → Flame/IR page reports `CLEAR`, green remains on, red remains off, buzzer remains silent.
2. Flame near the sensor → Flame/IR page reports `DETECTED`, green turns off, red flashes, buzzer activates.
3. Repeat the same test in GLOBAL mode.
4. Confirm unrelated sensor pages do not create a flame alarm in PAGE mode.

### Implementation record

Corrected firmware commit:

`0ec981e78d927f1416072993950533142e6682af0`

The correction is intentionally documented as a physical-validation result rather than an assumed module characteristic. The firmware polarity constants remain configurable because module variants can differ.
