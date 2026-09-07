# VIGIL-01 Hardware Architecture — V1.3

## 1. System Objective

VIGIL-01 is a portable, handheld sensing instrument centered on an ESP32. The hardware combines environmental sensing, close-range investigation sensors, a pulse sensor, a 6-axis motion sensor, a local user interface, and status outputs.

V1.3 is deliberately **live-only**. There is no SD storage, no historical database, and no cloud telemetry.

## 2. Hardware Development Stage

VIGIL-01 is currently a **solderless breadboard prototype**. The breadboard is intentionally temporary: it allows sensors, wiring, GPIO assignments, indicators, and other hardware to be changed quickly while the electrical architecture and firmware are being validated.

The breadboard stage is used to:

- verify that individual modules operate correctly
- validate the GPIO and power architecture
- characterize raw analog and digital sensor behavior
- test the user interface and alert behavior
- validate the shared I²C bus
- identify wiring, signal-integrity, power, and firmware problems before committing to permanent hardware

The breadboard prototype is **not the final physical construction** of VIGIL-01. After the electrical design and firmware behavior have been sufficiently validated, the design will be transferred to a custom PCB. The PCB revision will contain the validated circuit in a more compact and permanent form, followed by soldered assembly and, eventually, an enclosure.

Development progression:

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
Validation of Final Revision
```

## 3. Development Power Architecture

The normal breadboard prototype is powered from a computer USB connection to the ESP32.

```text
USB
 |
v
ESP32
 |
 +-- 3V3 rail --> low-voltage sensors + OLED + GY-521
 |
 +-- GND rail --> common ground
```

A separate bench experiment demonstrated portable-power feasibility using an XTR 502030 3.7 V 200 mAh Li-ion cell through the prototype power-conversion path. This is recorded as a feasibility experiment; it is not yet the final battery architecture.

A resistor must **not** be used as a substitute for a voltage regulator. The ESP32 supply must remain regulated. Battery power will be finalized after selecting a compact regulator/power stage that can handle ESP32 current transients.

## 4. ESP32 GPIO Allocation

| GPIO | Direction / role | Device |
|---:|---|---|
| 4 | Output | Passive buzzer |
| 13 | Output | Green LED through 330 Ω |
| 14 | Output | Red LED through 330 Ω |
| 16 | Input pull-up | UP button |
| 17 | Input pull-up | DOWN button |
| 18 | Input pull-up | SELECT button |
| 19 | Input pull-up | BACK button |
| 21 | I²C SDA | SSD1306 OLED + GY-521 MPU6050 |
| 22 | I²C SCL | SSD1306 OLED + GY-521 MPU6050 |
| 23 | Digital input | Flame/IR sensor |
| 25 | Digital input | DHT11 data |
| 26 | Digital input | HW511 signal `S` |
| 27 | Digital input | IR obstacle sensor `OUT` |
| 32 | ADC1 input | HW502 heartbeat analog output |
| 33 | ADC1 input | Photoresistor divider |
| 34 | ADC1 input | Sound sensor analog output |
| 35 | ADC1 input | 49E linear Hall `A0` |
| 36 | ADC1 input | Water sensor analog output |
| 39 | ADC1 input | Reserved battery monitor |

GPIO39 remains unused while battery monitoring is disabled.

## 5. OLED

Display: 0.96-inch SSD1306, 128×64, I²C, four pins.

The module labels its clock pin `SCK`; in this application that pin is the I²C clock (`SCL`).

```text
OLED VCC  -> ESP32 3V3
OLED GND  -> ESP32 GND
OLED SDA  -> GPIO21
OLED SCK  -> GPIO22
```

## 6. GY-521 / MPU6050

The GY-521 is the V1.3 motion-sensing module. It provides a 3-axis accelerometer and 3-axis gyroscope over I²C.

### Wiring

```text
GY-521 VCC -> ESP32 3V3
GY-521 GND -> ESP32 GND
GY-521 SDA -> GPIO21
GY-521 SCL -> GPIO22
GY-521 INT -> not connected
GY-521 AD0 -> GND for address 0x68
```

If AD0 is HIGH, the MPU6050 uses address `0x69`. The firmware probes both addresses.

The GY-521 does **not** require its own GPIO pair. It shares the same I²C bus as the OLED:

```text
GPIO21 (SDA)
   ├── OLED SDA
   └── GY-521 SDA

GPIO22 (SCL)
   ├── OLED SCL
   └── GY-521 SCL
```

Multiple I²C devices can share the bus because each device is selected by its I²C address. The current firmware uses a conservative 100 kHz I²C clock.

### Firmware configuration

The MPU6050 is configured for:

- ±8 g accelerometer range
- ±500 °/s gyroscope range
- 21 Hz digital filter bandwidth

Derived values include:

- acceleration X/Y/Z
- gyroscope X/Y/Z
- acceleration magnitude
- tilt angle
- motion detection
- impact detection
- tilt detection

Prototype thresholds:

```text
Motion = 1.5 m/s² deviation from nominal gravity magnitude
Impact = 25.0 m/s²
Tilt   = 30°
```

These thresholds are engineering/prototype values and are not safety limits.

### GY-521 troubleshooting

At boot, V1.3 explicitly initializes the I²C bus before probing the MPU6050 and checks both `0x68` and `0x69`. Serial output at 115200 baud reports the detected address or a failure message.

If the firmware reports that neither address is detected, check the physical bus before changing application logic:

1. GY-521 VCC is connected to the intended supply.
2. GY-521 GND and ESP32 GND are common.
3. SDA is on GPIO21 and SCL is on GPIO22.
4. AD0 is LOW for `0x68` or HIGH for `0x69`.
5. No loose breadboard jumper is interrupting SDA/SCL.
6. The GY-521 is not being driven by an unsafe I²C voltage level.
7. The OLED still works on the same bus; if the OLED works but the MPU does not, verify the MPU address and module wiring.

## 7. User Controls

Four momentary pushbuttons are connected to ground. Firmware enables internal pull-ups.

```text
GPIO16 -> UP     -> GND
GPIO17 -> DOWN   -> GND
GPIO18 -> SELECT -> GND
GPIO19 -> BACK   -> GND
```

Electrical logic:

```text
Released = HIGH
Pressed  = LOW
```

Software debounce is required.

## 8. DHT11

For the three-pin module version:

```text
DHT11 VCC  -> 3V3
DHT11 GND  -> GND
DHT11 DATA -> GPIO25
```

If a bare four-pin DHT11 is used, its pin order and pull-up requirement must be verified before wiring.

## 9. HW502 Heartbeat Sensor

V1 uses the analog pulse signal.

```text
HW502 VCC -> 3V3
HW502 GND -> GND
HW502 AO  -> GPIO32
```

The module should not be powered at 5 V while its analog output is directly connected to the ESP32 ADC unless the output voltage has been verified safe.

The heartbeat algorithm is an initial experimental estimator. It is not a medical measurement or diagnosis.

## 10. Photoresistor

The photoresistor is used in a voltage divider with a 10 kΩ resistor.

```text
3V3
 |
LDR
 |
 +------ GPIO33
 |
10 kΩ
 |
GND
```

The result is a relative light measurement. It is not automatically a calibrated lux measurement.

## 11. Sound Sensor

For a module exposing VCC, GND, AO, and DO:

```text
VCC -> 3V3
GND -> GND
AO  -> GPIO34
DO  -> unused
```

The analog signal is treated as a relative acoustic signal, not a calibrated SPL/dB measurement. The current prototype software trigger defaults to `soundRaw > 135`.

## 12. HW511 / TCRT5000

The actual module in the current prototype has three pins: `V+`, `G`, and `S`.

```text
V+ -> 3V3
G  -> GND
S  -> GPIO26
```

`S` is the module's signal output.

## 13. IR Obstacle Sensor

The current module has four pins: `GND`, `VCC`, `OUT`, and `EN`.

Initial wiring:

```text
GND -> GND
VCC -> 3V3
OUT -> GPIO27
EN  -> NC (not connected during initial test)
```

The exact enable polarity is not assumed. If the sensor does not operate with EN floating, its module behavior must be characterized before tying EN high or low.

## 14. 49E Linear Hall-Effect Module

The selected Hall module is the analog/linear 49E board. It exposes:

- `GND / G`
- `+ / VCC`
- `A0`
- `D0`

V1 uses the analog output:

```text
G   -> GND
+   -> 3V3
A0  -> GPIO35
D0  -> unused
```

The analog output changes with magnetic field strength and direction. It is treated as a relative magnetic signal rather than a calibrated gauss measurement.

## 15. Water Sensor

For the analog-output version:

```text
VCC -> 3V3
GND -> GND
AO  -> GPIO36
```

The raw ADC value is used as a relative wetness/water signal. Continuous powering may accelerate corrosion on exposed water-sensor electrodes; later revisions may switch sensor power only during measurement.

## 16. Flame / IR Sensor

Initial digital interface:

```text
VCC -> 3V3
GND -> GND
DO  -> GPIO23
```

For the current module configuration, firmware uses:

```text
CLEAR          = LOW
FLAME/IR EVENT = HIGH
```

This polarity is intentionally configurable because inexpensive module variants can differ. It should remain validated against the actual hardware.

## 17. Status LEDs

### Green

```text
GPIO13 -> 330 Ω -> LED anode
LED cathode -> GND
```

### Red

```text
GPIO14 -> 330 Ω -> LED anode
LED cathode -> GND
```

## 18. Passive Buzzer

Current breadboard connection:

```text
GPIO4 -> passive buzzer -> GND
```

This is acceptable for initial testing if the specific buzzer's current requirement is appropriate for direct GPIO drive. The final design may use a transistor driver if required by the buzzer load.

## 19. Common Ground

All active modules must share the ESP32 ground reference.

```text
ESP32 GND
   |
   +-- OLED GND
   +-- GY-521 GND
   +-- DHT11 GND
   +-- heartbeat GND
   +-- sensor grounds
   +-- button grounds
   +-- LED cathodes
   +-- buzzer ground
```

## 20. Analog Input Safety

ESP32 ADC pins must not receive voltages above their permitted input range. Any module powered at a higher voltage must have its output checked before direct connection to an ESP32 ADC.

ADC1 is intentionally used for VIGIL-01 analog sensing because Wi-Fi can interfere with ADC2 operation on the classic ESP32.

## 21. Components Used in V1.3 Breadboard

- ESP32 development board
- 0.96-inch SSD1306 OLED
- GY-521 / MPU6050
- DHT11
- HW502 heartbeat module
- Photoresistor
- 10 kΩ resistor
- Sound sensor
- HW511 / TCRT5000
- IR obstacle sensor
- 49E linear Hall module
- Water sensor
- Flame/IR sensor
- 4 pushbuttons
- Green LED
- Red LED
- 330 Ω resistors ×2
- Passive buzzer
- Breadboard and jumper wires

## 22. Components / Features Explicitly Not in V1.3

- BME280
- GPS module/functionality
- SD card
- Cloud storage/database
- RTC/history system
- Battery monitoring circuitry (GPIO39 remains reserved)
- Additional unselected kit modules

The MPU6050/GY-521 is **not excluded** from V1.3; it is an implemented motion-sensing subsystem on the shared I²C bus.

## 23. Planned PCB Transition

The PCB revision will be created only after the breadboard prototype has been sufficiently tested. The intent is to preserve the validated electrical behavior while improving reliability, compactness, wiring integrity, and physical assembly.

The PCB phase will include:

1. Capture the final validated schematic.
2. Assign PCB footprints and verify connector/pin orientation.
3. Route power and signal paths with appropriate grounding and decoupling.
4. Run electrical/design-rule checks.
5. Fabricate the board.
6. Solder components onto the PCB.
7. Perform bring-up and compare behavior against the breadboard baseline.
8. Document any PCB-specific failures and revisions.

The breadboard prototype therefore serves as the **engineering validation platform**, while the future PCB serves as the **permanent hardware implementation**.
