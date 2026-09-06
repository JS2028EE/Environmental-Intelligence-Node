# VIGIL-01 Firmware Architecture — V1

## Overview

V1 firmware is written for the Arduino ESP32 environment. It is a live embedded instrument: sensors are sampled, interpreted, and displayed without persistent historical storage.

## Required Libraries

Install with Arduino IDE Library Manager:

- Adafruit GFX Library
- Adafruit SSD1306
- DHT sensor library by Adafruit
- Adafruit Unified Sensor

Provided by the ESP32 Arduino core:

- Wire
- WiFi
- WebServer

## Firmware Responsibilities

1. Initialize hardware.
2. Display boot and ready screens.
3. Sample analog and digital sensors.
4. Read DHT11 at a slower interval appropriate for the sensor.
5. Estimate heart rate from the HW502 signal.
6. Determine an overall live system state.
7. Drive green/red LEDs and buzzer.
8. Handle four-button navigation with software debounce.
9. Render the OLED UI.
10. Host a local Wi-Fi access point and live dashboard.
11. Expose live values as JSON.

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
      |    +-- Flame / IR
      |
      +-- SYSTEM_MENU
           +-- Battery
           +-- Hardware
           +-- Sensors
           +-- About
```

Button behavior:

- UP: previous menu item
- DOWN: next menu item
- SELECT: enter selected item
- BACK: return to parent screen

## Timing Model

The main loop uses `millis()` instead of blocking delays for normal operation.

- Fast sensor processing: approximately every 100 ms
- DHT11 reading: approximately every 2 s
- OLED refresh: approximately every 100 ms
- Web server: serviced continuously
- Button debounce: approximately 40 ms

## Heartbeat Processing

The first V1 implementation uses the HW502 analog output and performs basic baseline tracking and threshold crossing detection.

The algorithm:

1. Sample the analog signal.
2. Slowly track the baseline.
3. Calculate signal deviation from baseline.
4. Classify signal quality as WEAK, FAIR, or GOOD.
5. Detect upward threshold crossings.
6. Measure the interval between crossings.
7. Convert valid intervals to BPM.
8. Smooth successive BPM estimates.

This is intentionally a first-pass signal-processing implementation. It should be experimentally characterized before being described as accurate.

## System Status Logic

Current V1 status logic is deliberately simple:

```text
NO MAJOR EVENT
    -> NORMAL

WATER ABOVE CURRENT THRESHOLD
    -> WARNING

OBJECT DETECTED
    -> WARNING

FLAME/IR EVENT
    -> CRITICAL
```

The status model will be refined after sensor characterization. Thresholds should not be treated as universally valid until measured and documented.

## Output Behavior

### NORMAL

- Green LED solid
- Red LED off
- Buzzer off

### NOTICE

- Green LED pulses
- Buzzer off

### WARNING

- Red LED flashes slowly
- Buzzer off in the current prototype

### CRITICAL

- Red LED flashes rapidly
- Passive buzzer produces alert tones

## Wi-Fi Dashboard

V1 creates a local access point:

```text
SSID: VIGIL-01
Password: VIGIL01_2026
```

The ESP32 hosts a browser dashboard at its access-point address, normally `192.168.4.1`.

The dashboard displays live values only.

Endpoint:

```text
/data
```

The endpoint returns JSON containing the current sensor state.

## No Persistent Storage

The V1 firmware does not use:

- SD card
- EEPROM-based event history
- cloud database
- cloud telemetry storage
- historical graphs

This is intentional. The current objective is reliable live instrumentation.

## Known Limitations

- Heart-rate estimation is experimental.
- Light is raw/relative ADC data rather than calibrated lux.
- Sound is raw/relative signal rather than calibrated dB SPL.
- Hall measurement is relative rather than a calibrated gauss measurement.
- Water threshold is an initial engineering threshold and requires characterization.
- Flame/IR detection is not certified fire detection.
- Sensor presence is currently represented optimistically in the sensor-status screen; robust electrical fault detection is a future improvement.
- The IR obstacle module's EN behavior has not been assumed; it is initially left unconnected.

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
