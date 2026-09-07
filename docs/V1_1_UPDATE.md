# VIGIL-01 V1.1 Firmware Update — Historical Record

Date: 2026-09-05

> **Status: SUPERSEDED by V1.4.** This document is retained as a historical engineering record. The current firmware architecture, sensor set, GPIO map, motion hardware, and alert behavior are documented in `docs/FIRMWARE.md`, `docs/HARDWARE.md`, `docs/V1_4_UPDATE.md`, and the active `firmware/` source.

V1.1 was a targeted refinement of the working VIGIL-01 V1 breadboard prototype firmware. The sensing architecture and OLED presentation were preserved; the update improved alert control and physical-interface feedback. V1.1 remained live-only: no SD card, historical database, cloud storage, or persistent telemetry was added.

## Prototype Hardware Status

V1.1 was developed on the solderless breadboard prototype documented in `docs/HARDWARE.md`. The firmware was tested against temporary jumper-wire connections before the hardware was committed to a permanent PCB.

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

### Historical alert conditions

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

### Heartbeat

No heartbeat-processing change was made. The HW502 continued using the experimental baseline/threshold BPM estimator. The result remained an experimental estimate, not a medical measurement.

## Later revisions

The following changes occurred after this V1.1 record and are intentionally not described as part of the original V1.1 implementation:

- Firmware was reorganized into the modular V1.3 architecture and then advanced to V1.4.
- The installed motion module was identified as an MPU-9250/MPU-6500/MPU-9255-family device and is accessed directly over I²C.
- The motion module shares GPIO21/GPIO22 with the OLED.
- BME280 and GPS functionality were removed from the active firmware scope.
- Persistent NVS/flash settings, the local dashboard, captive portal, mDNS, and watchdog support were added.
- The physical flame-module polarity was revalidated. Current behavior is `FLAME_ACTIVE_LOW = false`: LOW = clear, HIGH = flame/IR event.
- V1.4 separated motion telemetry from physical alarms.
- V1.4 replaced raw motion/impact alarm behavior with a staged fall detector requiring low-g, impact, and sustained post-impact tilt.

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

The key V1.1 design decision was to separate **sensing**, **interpretation**, and **presentation**. Later revisions preserve that principle while adding the identified MPU-9250-family motion subsystem, staged fall detection, persistent configuration, network services, and watchdog recovery.
