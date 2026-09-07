# VIGIL-01 V1.1 Firmware Update — Historical Record

Date: 2026-09-05

> **Status: SUPERSEDED by V1.3.** This document is retained as a historical engineering record. The current firmware architecture, sensor set, GPIO map, alert behavior, and polarity settings are documented in `docs/FIRMWARE.md`, `docs/HARDWARE.md`, and the current `firmware/` source.

V1.1 was a targeted refinement of the working VIGIL-01 V1 **breadboard prototype** firmware. The sensing architecture and OLED presentation were preserved; the update improved alert control and physical-interface feedback. V1.1 remained live-only: no SD card, historical database, cloud storage, or persistent telemetry was added.

## Prototype Hardware Status

V1.1 was developed and validated on the same **solderless breadboard prototype** documented in `docs/HARDWARE.md`. The firmware was tested against temporary jumper-wire connections before the hardware was committed to a permanent PCB.

## Changes

### Sound threshold

Bench testing showed the sound sensor normally moving around 0-100 while a clap produced a much larger response. V1.1 therefore used the explicit prototype trigger:

```text
soundRaw > 135 -> STATUS_WARNING
```

This is a raw/relative ADC threshold, not calibrated dB SPL.

### Navigation feedback

UP, DOWN, SELECT, and BACK produced short audible feedback tones. The existing 40 ms software debounce remained in place. The buzzer setting controlled both navigation tones and alarm tones.

### Alert presentation vs. detection

V1.1 separated **overall system status** from **whether the currently displayed page should physically alarm**.

```text
ALERTS: PAGE
  -> Only the currently relevant danger/event condition can activate red LED + alarm.

ALERTS: GLOBAL
  -> Any defined system danger/event can activate red LED + alarm from any screen.
```

Opening a sensor page by itself was never an alert. A normal value was never an alarm.

### Normal-state LED behavior

When LEDs were enabled and there was no active alert, the green LED stayed solid ON. The red LED was reserved for active warning/critical conditions.

### Output settings

SYSTEM contained SETTINGS with:

```text
SETTINGS
> LEDS: ON/OFF
  BUZZER: ON/OFF
  ALERTS: PAGE/GLOBAL
```

### Alert conditions

The V1.1 prototype defined:

```text
FLAME/IR event -> CRITICAL
WATER > 2500 -> WARNING
OBJECT detected -> WARNING
SOUND > 135 -> WARNING
Other currently implemented sensor pages -> no physical alarm condition by default
Otherwise -> NORMAL
```

Critical had priority over warning.

### Digital sensor polarity

V1.1 made digital detection polarity explicit through configuration constants:

```text
TCRT_ACTIVE_LOW
IR_ACTIVE_LOW
FLAME_ACTIVE_LOW
```

The physical module behavior was intended to be verified during breadboard characterization because inexpensive module variants can differ.

### Sensor-module potentiometers

Onboard potentiometers were treated as comparator sensitivity/digital switching-threshold adjustments unless proven otherwise. They were not assumed to calibrate the physical quantity.

### Heartbeat

No heartbeat-processing change was made. The HW502 continued using the experimental baseline/threshold BPM estimator. The result remained an experimental estimate, not a medical measurement.

## Later V1.3 Changes

The following changes occurred after this V1.1 record and are intentionally not described as part of the original V1.1 implementation:

- Firmware was reorganized into the current modular V1.3 architecture.
- MPU6050/GY-521 motion sensing was added on the shared OLED I²C bus.
- Current GY-521 wiring is SDA GPIO21, SCL GPIO22, with address `0x68` when AD0 is LOW and `0x69` when AD0 is HIGH.
- BME280 and GPS functionality were removed from the current firmware scope.
- Persistent NVS/flash settings were added.
- The local dashboard gained MPU6050 telemetry, settings controls, captive-portal handling, and mDNS.
- Watchdog support was added.
- The physical flame-module polarity was later revalidated and changed in the current configuration. **Current V1.3 behavior is `FLAME_ACTIVE_LOW = false`: LOW = clear, HIGH = flame/IR event.** The earlier active-low statement in this historical record must not be used as the current wiring/firmware reference.

## Historical Files

The original V1/V1.1 filenames referenced by this document are historical references. The active firmware is now the modular `firmware/VIGIL01.ino` architecture and its supporting `.cpp/.h` files.

## Historical Validation Checklist

The V1.1 validation goals included:

- OLED boot and HOME layout remain unchanged
- Existing menus navigate correctly
- UP/DOWN/SELECT/BACK produce feedback tones
- Green LED is solid when LEDs are enabled and the system has no active alert
- Red LED stays off during normal operation
- Red LED stays off on unrelated sensor/menu pages in PAGE mode
- Opening a normal sensor page does not trigger the alarm
- Sound above 135 produces WARNING when the sound condition is being presented
- Water above 2500 produces WARNING when the water condition is being presented
- Object detection produces WARNING on the object/conditions page
- Flame/IR detection produces CRITICAL on the flame/conditions page
- GLOBAL mode presents defined alerts outside the relevant sensor page
- LEDS OFF disables both LEDs
- BUZZER OFF disables navigation/alarm tones
- Live `/data` dashboard remains functional
- Breadboard prototype remains electrically stable during integrated testing

## Engineering Rationale

The key V1.1 design decision was to separate **sensing**, **interpretation**, and **presentation**. The later V1.3 architecture preserves that principle while adding the GY-521 motion subsystem, persistent configuration, improved network services, and watchdog recovery.
