# VIGIL-01 Hardware Architecture — V1.4

## 1. System objective

VIGIL-01 is a portable ESP32-based environmental and situational sensing instrument. The current prototype combines environmental sensing, close-range investigation sensors, pulse sensing, a 6-axis motion sensor, a local OLED interface, LEDs, a buzzer, and a local Wi-Fi dashboard.

V1 remains live-only: no SD storage, historical database, or cloud telemetry is active.

## 2. Prototype stage

VIGIL-01 remains a solderless breadboard prototype. The breadboard is the validation platform before schematic finalization, PCB layout, fabrication, soldered assembly, and enclosure work.

## 3. GPIO map

| GPIO | Function |
|---:|---|
| 4 | Passive buzzer |
| 13 | Green LED through 330 Ω |
| 14 | Red LED through 330 Ω |
| 16 | UP button |
| 17 | DOWN button |
| 18 | SELECT button |
| 19 | BACK button |
| 21 | I²C SDA — OLED + motion sensor |
| 22 | I²C SCL — OLED + motion sensor |
| 23 | Flame/IR digital signal |
| 25 | DHT11 data |
| 26 | HW511/TCRT5000 signal |
| 27 | IR obstacle OUT |
| 32 | HW502 heartbeat analog |
| 33 | Photoresistor divider |
| 34 | Sound sensor analog |
| 35 | 49E Hall A0 |
| 36 | Water sensor analog |
| 39 | Battery monitor divider input (reserved/firmware-disabled in V1.4) |

## 4. OLED + MPU-9250-family shared I²C bus

The 0.96-inch SSD1306 OLED and the installed motion module share one I²C bus:

```text
ESP32 GPIO21 SDA
  ├── OLED SDA
  └── Motion SDA

ESP32 GPIO22 SCL
  ├── OLED SCL
  └── Motion SCL
```

Motion-module wiring:

```text
VCC -> ESP32 3V3
GND -> ESP32 GND
SDA -> GPIO21
SCL -> GPIO22
AD0 -> GND for 0x68
```

AD0 HIGH selects `0x69`.

### Actual motion hardware clarification

The module currently used is marked for the **MPU-9250 / MPU-6500 / MPU-9255 family**, rather than being treated as a confirmed MPU6050. The firmware reads `WHO_AM_I` to identify supported silicon:

```text
0x70 -> MPU-6500
0x71 -> MPU-9250
0x73 -> MPU-9255
```

The actual identity is established by the boot diagnostic, not by the generic breakout-board label alone.

### Live disconnect behavior

Because the prototype is solderless, an I²C wire can be disturbed during testing. The firmware now clears `mpuPresent` when a live motion register read fails, invalidates motion telemetry, and periodically attempts to rediscover and reconfigure the sensor. This prevents a stale `IMU OK` state after a physical disconnect and allows recovery without a reboot when the connection is restored.

## 5. Motion capability

The motion module supplies:

- 3-axis acceleration
- 3-axis gyroscope
- acceleration magnitude
- tilt angle
- movement classification
- impact telemetry
- fall-event detection

The firmware configures ±8 g acceleration and ±500 °/s gyro at 100 kHz I²C.

### Alarm behavior

Normal movement is intentionally **not** an alarm. Walking, running, rotation, ordinary tilt, and an isolated acceleration spike are retained as telemetry.

Fall detection uses:

```text
Low-g/free-fall
      ↓
Impact
      ↓
Sustained post-impact tilt
      ↓
Fall event
```

Current prototype parameters are `<4.0 m/s²` low-g, `>25.0 m/s²` impact, `>45°` post-impact tilt, a `1200 ms` sequence window, and `300 ms` tilt confirmation.

This is experimental fall-detection logic and is not a certified safety system.

### Position/orientation clarification

The IMU can describe orientation and movement, including tilt and angular rotation. It cannot provide reliable absolute 3D position over long periods by simply integrating acceleration because sensor bias and drift accumulate. V1.4 therefore reports **how the device is oriented and moving**, not a guaranteed geographic/spatial position.

## 6. Other sensor wiring

### DHT11

```text
VCC -> 3V3
GND -> GND
DATA -> GPIO25
```

### HW502 heartbeat

```text
VCC -> 3V3
GND -> GND
AO -> GPIO32
```

The result is an experimental pulse/heart-rate estimate, not a medical measurement.

### Photoresistor

Voltage divider with 10 kΩ resistor, divider node to GPIO33.

### Sound sensor

```text
VCC -> 3V3
GND -> GND
AO -> GPIO34
DO -> unused
```

The reading is a raw/relative acoustic signal, not calibrated dB SPL.

### HW511/TCRT5000

```text
V+ -> 3V3
G -> GND
S -> GPIO26
```

### IR obstacle sensor

```text
GND -> GND
VCC -> 3V3
OUT -> GPIO27
EN -> NC during initial testing
```

### 49E linear Hall module

```text
G -> GND
+ -> 3V3
A0 -> GPIO35
D0 -> unused
```

### Water sensor

```text
VCC -> 3V3
GND -> GND
AO -> GPIO36
```

The reading is relative wetness/water level. Continuous electrode power may accelerate corrosion; pulsed power remains a planned hardware improvement.

### Flame/IR sensor

```text
VCC -> 3V3
GND -> GND
DO -> GPIO23
```

Current verified firmware polarity:

```text
LOW  = CLEAR
HIGH = FLAME/IR EVENT
```

## 7. LEDs and buzzer

Green LED: GPIO13 → 330 Ω → LED → GND.

Red LED: GPIO14 → 330 Ω → LED → GND.

Passive buzzer:

```text
GPIO4 -> passive buzzer -> GND
```

A transistor driver may be used in the final design if the selected buzzer requires more current than an ESP32 GPIO should provide.

## 8. Battery-monitor divider

GPIO39 is the reserved battery-monitor ADC input. When battery monitoring is enabled, the tracked firmware assumes a 2:1 divider:

```text
Battery +
   │
  100K
   │
   ├──── GPIO39
   │
  100K
   │
Battery - / GND
```

The divider midpoint is approximately half the battery voltage. For example, a 4.05 V cell produces about 2.03 V at GPIO39. The firmware then reconstructs the battery voltage and maps it to a rough percentage estimate. The current firmware keeps this feature disabled while the battery power architecture is finalized.

## 9. Common ground and voltage safety

All active modules share ESP32 ground. ESP32 GPIO/ADC inputs must never receive an unsafe voltage. Any higher-voltage module output must be checked or level-shifted before direct connection.

## 10. Current exclusions

Not active in V1.4:

- BME280
- GPS
- SD card
- cloud storage/database
- historical telemetry
- battery monitoring in tracked firmware

## 11. Planned hardware progression

```text
Breadboard validation
  ↓
Sensor characterization
  ↓
Schematic finalization
  ↓
PCB layout + design rules
  ↓
PCB fabrication
  ↓
Soldered assembly
  ↓
Enclosure
  ↓
Final validation
```
