# VIGIL-01 Firmware Architecture — V1.3

## Overview

V1.3 firmware is written for the Arduino ESP32 environment. It is a live embedded instrument: sensors are sampled, interpreted, and displayed without persistent historical event storage.

The firmware uses a modular architecture so sensing, navigation, display rendering, alert presentation, settings, networking, and watchdog servicing can be changed independently.

## Firmware Modules

```text
VIGIL01.ino       Main coordinator and timing loop
Config.h          GPIOs, addresses, thresholds, timing, feature flags
Types.h           Screen and system-state types
Sensors.cpp/.h    Sensor acquisition and sensor-derived metrics
Navigation.cpp/.h Button input and screen navigation
MenuData.h        Menu definitions
DisplayUI.cpp/.h  OLED rendering
Alerts.cpp/.h     Physical alert presentation
Settings.cpp/.h   Persistent NVS/flash settings
WebDashboard.cpp/.h Local AP, dashboard, JSON API, captive portal, mDNS
Watchdog.cpp/.h   ESP32 watchdog initialization/feed
```

## Required Libraries

Install with Arduino IDE Library Manager:

- Adafruit GFX Library
- Adafruit SSD1306
- DHT sensor library by Adafruit
- Adafruit MPU6050
- Adafruit Unified Sensor
- ArduinoJson 6.x

Provided by the ESP32 Arduino core:

- Wire
- WiFi
- WebServer
- DNSServer
- ESPmDNS

BME280 and GPS libraries are **not required by V1.3** because those subsystems are not implemented.

## Main Execution Model

The main `.ino` file coordinates the system rather than containing the sensor implementation itself.

```text
setup()
  -> Serial
  -> GPIO outputs
  -> settingsLoad()
  -> navigationBegin()
  -> sensorsBegin()
  -> displayBegin()
  -> boot screen
  -> webBegin()
  -> watchdogBegin()

loop()
  -> webPoll()
  -> navigationPoll()
  -> fast sensor read / heartbeat / status
  -> slow DHT read
  -> OLED refresh
  -> status outputs
  -> watchdog feed
```

The normal loop uses `millis()` scheduling instead of blocking delays. A short 50 ms startup delay is used inside sensor initialization to allow the GY-521 to settle before its I²C probe.

## Timing Model

- Fast sensor processing: approximately every 100 ms
- DHT11 reading: approximately every 2 s
- OLED refresh: approximately every 100 ms
- Web server: serviced continuously
- Button debounce: approximately 40 ms
- I²C bus: 100 kHz

## Sensor Architecture

### DHT11

Temperature and humidity are read in the slower sensor task because the DHT11 is not intended for rapid polling.

### Analog Sensors

The firmware samples:

- HW502 heartbeat
- Photoresistor
- Sound sensor
- 49E Hall module
- Water sensor

These values are raw ADC measurements unless otherwise noted.

### Digital Sensors

The firmware samples:

- HW511/TCRT5000
- IR obstacle sensor
- Flame/IR sensor

Detection polarity is configurable in `Config.h` through named constants so module-specific behavior can be validated without burying polarity assumptions in application logic.

### MPU6050 / GY-521

The GY-521 is the V1.3 motion subsystem. It shares the OLED I²C bus on GPIO21/GPIO22.

Initialization now explicitly performs:

1. `Wire.begin(GPIO21, GPIO22)`
2. 100 kHz I²C clock selection
3. short power-up settling delay
4. MPU6050 probe at `0x68`
5. fallback probe at `0x69`
6. sensor-range/filter configuration when detected

The firmware uses the explicit `mpu.begin(address, &Wire)` form so the Adafruit MPU6050 driver is guaranteed to use the already-configured ESP32 I²C bus.

At boot, Serial at 115200 baud reports one of:

```text
MPU6050/GY-521 detected at 0x68
MPU6050/GY-521 detected at 0x69
ERROR: MPU6050/GY-521 not detected on I2C bus (0x68/0x69)
```

The MPU6050 is configured for:

- Accelerometer: ±8 g
- Gyroscope: ±500 °/s
- Filter bandwidth: 21 Hz

Each fast read obtains:

- acceleration X/Y/Z
- gyroscope X/Y/Z
- acceleration magnitude
- tilt angle

Derived event flags are:

```text
Motion: |acceleration magnitude - 9.80665| > 1.5 m/s²
Impact: acceleration magnitude > 25.0 m/s²
Tilt:   tilt angle > 30°
```

These values are prototype detection thresholds, not calibrated safety limits.

## MPU6050 Failure Diagnosis

The previous implementation already called `Wire.begin()` and probed the MPU6050, but it did not explicitly pass the configured `Wire` instance to the driver, did not give the GY-521 time to settle after power-up, and did not provide useful Serial diagnostics when the probe failed.

V1.3 now makes the bus and probe path explicit. This removes ambiguity in the firmware and makes the actual failure mode visible at boot.

If the Serial monitor reports that neither address is detected, the remaining fault is outside the application-level read loop and should be investigated as an electrical/I²C problem:

- VCC/GND wiring
- common ground
- SDA/SCL wiring
- AD0 address state
- loose breadboard connections
- I²C voltage level
- damaged GY-521/module

If the device is detected but values are not changing, the next test is to inspect `/data` and the MOTION screen while physically rotating/moving the module.

## UI State Machine

Top-level states:

```text
HOME
 |
 +-- MAIN_MENU
      |
      +-- ENV_MENU
      |    +-- Temperature
      |    +-- Humidity
      |    +-- Light
      |    +-- Sound
      |    +-- Conditions
      |
      +-- VITALS_MENU
      |    +-- Heart
      |    +-- Signal
      |    +-- Measurement
      |
      +-- INVESTIGATE_MENU
      |    +-- IR Reflection
      |    +-- Object
      |    +-- Magnetic
      |    +-- Water
      |    +-- Flame
      |
      +-- MOTION
      |    +-- Acceleration X/Y/Z
      |    +-- Acceleration magnitude
      |    +-- Tilt
      |    +-- Motion state
      |    +-- Impact state
      |    +-- Tilt state
      |
      +-- SYSTEM_MENU
           +-- Battery
           +-- Hardware
           +-- Sensor Status
           +-- About
           +-- Settings
```

Button behavior:

- UP: previous menu item
- DOWN: next menu item
- SELECT: enter selected item
- BACK: return to parent screen

## Heartbeat Processing

The HW502 analog signal uses a first-pass baseline/threshold BPM estimator:

1. Sample the analog signal.
2. Slowly track the baseline.
3. Calculate signal deviation.
4. Classify signal quality as WEAK, FAIR, or GOOD.
5. Detect upward threshold crossings.
6. Measure intervals between crossings.
7. Convert valid intervals to BPM.
8. Smooth successive BPM estimates.

This is an experimental signal-processing feature, not a medical measurement.

## System Status Logic

Current overall status is evaluated continuously:

```text
WATER > configured threshold       -> WARNING
IR/object detected                 -> WARNING
SOUND > configured threshold       -> WARNING
Motion detected                    -> WARNING
Tilt detected                      -> WARNING
Impact detected                    -> CRITICAL
Flame/IR detected                  -> CRITICAL
Otherwise                          -> NORMAL
```

Critical status has priority over warning.

## Alert Presentation

Sensing and overall status are separate from physical presentation.

`ALERTS: PAGE` means only a defined alert condition associated with the currently displayed page can activate the physical outputs.

`ALERTS: GLOBAL` means the overall system status can activate physical outputs from any screen.

The current page-specific physical alert conditions are defined in `Alerts.cpp`:

```text
SOUND_SCREEN      -> sound threshold exceeded
WATER_SCREEN      -> water threshold exceeded
OBJECT_SCREEN     -> object/IR detected
FLAME_SCREEN      -> flame detected
CONDITIONS_SCREEN -> any defined condition above
Other pages       -> no page-specific physical alarm
```

Motion currently contributes to the overall system status, but it does not have a dedicated page-specific physical alarm condition in PAGE mode. This keeps the MOTION screen informational while preserving motion/impact/tilt visibility and GLOBAL-mode status behavior.

## Output Behavior

When LEDs are enabled and there is no active physical alert:

```text
Green LED = ON
Red LED   = OFF
Buzzer    = OFF
```

During an active warning/critical alert:

```text
Green LED = OFF
Red LED   = flashing
Buzzer    = tone bursts when enabled
```

Critical alerts use faster flashing and a higher-frequency tone than warning alerts.

## Persistent Settings

Settings are stored in ESP32 NVS/flash.

Current persisted settings:

```text
LEDS: ON/OFF
BUZZER: ON/OFF
ALERTS: PAGE/GLOBAL
UNITS: C/F
Sound threshold
Water threshold
```

Default thresholds:

```text
Sound = 135
Water = 2500
```

Changing a setting does not disable sensor acquisition; it changes configuration or presentation behavior.

## Wi-Fi Dashboard

V1.3 creates a local access point:

```text
SSID: VIGIL-01
Password: VIGIL01_2026
mDNS: vigil01.local
```

The browser dashboard provides live values and threshold/settings controls.

Endpoints:

```text
GET  /
GET  /data
GET  /api/settings
POST /api/settings
```

`/data` includes the current sensor state, MPU6050 presence and measurements, derived motion flags, overall status, thresholds, active-alert state, output settings, alert mode, and units preference.

The device also provides a captive-portal DNS redirect and mDNS service.

## Watchdog

The watchdog is enabled with an 8-second timeout. The implementation accounts for ESP-IDF API differences across ESP32 Arduino core generations.

The main loop feeds the watchdog during normal execution. If the application becomes stuck and stops servicing the watchdog, the ESP32 can reset.

## Storage Model

V1.3 does not implement historical event storage. It does use NVS/flash for persistent configuration settings.

Not implemented:

- SD event logging
- historical telemetry database
- cloud telemetry storage
- historical graphs
- RTC-based event history

## Known Limitations

- GY-521 detection depends on correct physical I²C wiring and module condition.
- Heart-rate estimation is experimental.
- Light is raw/relative ADC data rather than calibrated lux.
- Sound is raw/relative signal rather than calibrated dB SPL.
- Hall measurement is relative rather than a calibrated gauss measurement.
- Water threshold is an initial engineering threshold and requires characterization.
- Flame/IR detection is not certified fire detection.
- MPU6050 motion/impact/tilt thresholds are prototype rules.
- Sensor presence detection is not a complete electrical fault-detection system.
- The IR obstacle module's EN behavior has not been assumed; it is initially left unconnected.
- Battery monitoring remains disabled.

## Validation Plan

For each sensor, record:

- supply voltage
- normal output range
- response to a controlled stimulus
- noise/variation
- repeatability
- useful operating distance where applicable
- threshold selection
- failure modes

For the GY-521 additionally record:

- I²C address observed at boot
- X/Y/Z acceleration at rest
- expected gravity magnitude near 9.8 m/s² at rest
- gyroscope output at rest
- response to controlled rotation
- response to controlled translation
- false motion triggers
- impact threshold response
- tilt threshold response

For the heartbeat sensor additionally record:

- finger placement conditions
- raw waveform behavior
- baseline drift
- motion sensitivity
- false beat rate
- comparison against a known reference when possible

## Future Firmware Work

- Better sensor health/fault detection
- Sensor-specific filtering
- Calibrated light characterization
- Sound signal characterization
- Hall-field baseline calibration
- Water-sensor threshold characterization
- More robust heartbeat peak detection
- Signal waveform rendering from a sampled buffer
- Better event prioritization
- Battery voltage and estimated battery state
- Power-management behavior
- More formal MPU6050 motion/impact calibration
