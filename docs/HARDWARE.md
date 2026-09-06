# VIGIL-01 Hardware Architecture — V1

## 1. System Objective

VIGIL-01 is a portable, handheld sensing instrument centered on an ESP32. The hardware combines environmental sensing, close-range investigation sensors, a pulse sensor, a local user interface, and status outputs.

V1 is deliberately **live-only**. There is no SD storage, no historical database, and no cloud telemetry.

## 2. Development Power Architecture

The current breadboard prototype is powered from a computer USB connection to the ESP32.

```text
USB
 |
v
ESP32
 |
 +-- 3V3 rail --> low-voltage sensors + OLED
 |
 +-- GND rail --> common ground
```

The planned single-cell Li-ion charger and battery system is not part of the current breadboard wiring.

A resistor must **not** be used as a substitute for a voltage regulator. The ESP32 supply must remain regulated. Battery power will be finalized after selecting a compact regulator/power stage that can handle ESP32 current transients.

## 3. ESP32 GPIO Allocation

| GPIO | Direction / role | Device |
|---:|---|---|
| 4 | Output | Passive buzzer |
| 13 | Output | Green LED through 330 Ω |
| 14 | Output | Red LED through 330 Ω |
| 16 | Input pull-up | UP button |
| 17 | Input pull-up | DOWN button |
| 18 | Input pull-up | SELECT button |
| 19 | Input pull-up | BACK button |
| 21 | I²C SDA | SSD1306 OLED |
| 22 | I²C SCL | SSD1306 OLED |
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

GPIO39 remains unused while the system is USB powered.

## 4. OLED

Display: 0.96-inch SSD1306, 128×64, I²C, four pins.

The module labels its clock pin `SCK`; in this application that pin is the I²C clock (`SCL`).

```text
OLED VCC  -> ESP32 3V3
OLED GND  -> ESP32 GND
OLED SDA  -> GPIO21
OLED SCK  -> GPIO22
```

## 5. User Controls

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

## 6. DHT11

For the three-pin module version:

```text
DHT11 VCC  -> 3V3
DHT11 GND  -> GND
DHT11 DATA -> GPIO25
```

If a bare four-pin DHT11 is used, its pin order and pull-up requirement must be verified before wiring.

## 7. HW502 Heartbeat Sensor

V1 uses the analog pulse signal.

```text
HW502 VCC -> 3V3
HW502 GND -> GND
HW502 AO  -> GPIO32
```

The module should not be powered at 5 V while its analog output is directly connected to the ESP32 ADC unless the output voltage has been verified safe.

The heartbeat algorithm is an initial experimental estimator. It is not a medical measurement or diagnosis.

## 8. Photoresistor

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

## 9. Sound Sensor

For a module exposing VCC, GND, AO, and DO:

```text
VCC -> 3V3
GND -> GND
AO  -> GPIO34
DO  -> unused
```

The analog signal is treated as a relative acoustic signal, not a calibrated SPL/dB measurement.

## 10. HW511 / TCRT5000

The actual module in the current prototype has three pins: `V+`, `G`, and `S`.

```text
V+ -> 3V3
G  -> GND
S  -> GPIO26
```

`S` is the module's signal output.

## 11. IR Obstacle Sensor

The current module has four pins: `GND`, `VCC`, `OUT`, and `EN`.

Initial wiring:

```text
GND -> GND
VCC -> 3V3
OUT -> GPIO27
EN  -> NC (not connected during initial test)
```

The exact enable polarity is not assumed. If the sensor does not operate with EN floating, its module behavior must be characterized before tying EN high or low.

## 12. 49E Linear Hall-Effect Module

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

The analog output changes with magnetic field strength and direction. The firmware uses it as a relative magnetic signal and estimates pole direction from the deviation around the no-field baseline. It is not treated as a calibrated gaussmeter without further characterization.

## 13. Water Sensor

For the analog-output version:

```text
VCC -> 3V3
GND -> GND
AO  -> GPIO36
```

The raw ADC value is used as a relative wetness/water signal. Continuous powering may accelerate corrosion on exposed water-sensor electrodes; later revisions may switch sensor power only during measurement.

## 14. Flame / IR Sensor

Initial digital interface:

```text
VCC -> 3V3
GND -> GND
DO  -> GPIO23
```

The device is treated as a strong IR/flame-like event sensor. It is not a certified fire detector.

## 15. Status LEDs

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

The LED orientation must be verified on the physical LED. Long leg is normally the anode on standard through-hole LEDs.

## 16. Passive Buzzer

Current breadboard connection:

```text
GPIO4 -> passive buzzer -> GND
```

This is acceptable for initial testing if the specific buzzer's current requirement is appropriate for direct GPIO drive. The final design may use a transistor driver if required by the buzzer load.

## 17. Common Ground

All active modules must share the ESP32 ground reference.

```text
ESP32 GND
   |
   +-- OLED GND
   +-- DHT11 GND
   +-- heartbeat GND
   +-- sensor grounds
   +-- button grounds
   +-- LED cathodes
   +-- buzzer ground
```

## 18. Analog Input Safety

ESP32 ADC pins must not receive voltages above their permitted input range. Any module powered at a higher voltage must have its output checked before direct connection to an ESP32 ADC.

ADC1 is intentionally used for VIGIL-01 analog sensing because Wi-Fi can interfere with ADC2 operation on the classic ESP32.

## 19. Components Used in V1 Breadboard

- ESP32 development board
- 0.96-inch SSD1306 OLED
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

## 20. Components Explicitly Excluded from V1

- MPU6050
- SD card
- Cloud storage/database
- Battery/charger power system
- RTC/history system
- Additional unselected kit modules

The MPU6050 was intentionally removed because motion sensing did not justify its complexity for this V1 objective.
