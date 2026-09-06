# VIGIL-01 V1.1 Firmware Update

Date: 2026-09-05

V1.1 is a targeted refinement of the working VIGIL-01 V1 **breadboard prototype** firmware. The sensing architecture and OLED presentation are preserved; the update improves alert control and physical-interface feedback. VIGIL-01 remains live-only: no SD card, historical database, cloud storage, or persistent telemetry was added.

## Prototype Hardware Status

V1.1 is being developed and validated on the same **solderless breadboard prototype** documented in `docs/HARDWARE.md`. The firmware is being tested against temporary jumper-wire connections before the hardware is committed to a permanent PCB.

The breadboard stage allows firmware, sensor thresholds, GPIO assignments, alert behavior, and electrical connections to be changed and validated without permanently soldering the design. Once the prototype has passed sufficient characterization and validation, the validated circuit will move to a custom PCB and the components will be soldered into the permanent hardware assembly.

The development path is:

```text
Breadboard Prototype
        ↓
Testing / Characterization
        ↓
Validated Schematic
        ↓
PCB Design + Fabrication
        ↓
Soldered Assembly
        ↓
Enclosure + Final Validation
```

Prototype photographs and later PCB/soldering photographs will be retained as part of the engineering record so the hardware evolution is traceable.

## Changes

### Sound threshold

Bench testing showed the sound sensor normally moving around 0-100 while a clap produced a much larger response. V1.1 therefore uses the explicit prototype trigger:

```text
soundRaw > 135 -> STATUS_WARNING
```

This is a raw/relative ADC threshold, not calibrated dB SPL.

### Navigation feedback

UP, DOWN, SELECT, and BACK now each produce a short audible feedback tone. The existing 40 ms software debounce remains in place. The buzzer setting controls both navigation tones and alarm tones.

### Alert presentation vs. detection

Sensor sampling and status evaluation remain continuous. What changed is the output policy. By default, alerts are page-scoped, so HOME and menu navigation do not continuously flash the red LED or sound the alarm even when a sensor event exists.

The default is:

```text
ALERTS: PAGE
```

In PAGE mode, warning/critical outputs are presented while viewing a sensor or condition screen. GLOBAL mode presents them regardless of the current screen.

### Output settings

SYSTEM now contains SETTINGS with:

```text
SETTINGS
> LEDS: ON/OFF
  BUZZER: ON/OFF
  ALERTS: PAGE/GLOBAL
```

UP/DOWN selects a setting; SELECT toggles it; BACK returns to SYSTEM.

`LEDS OFF` suppresses both LEDs. `BUZZER OFF` suppresses navigation and alarm tones while sensor detection continues.

The green LED remains solid when LEDs are enabled and no alert is being presented. The red LED is reserved for warning/critical indication.

### Alert priority

```text
FLAME/IR event -> CRITICAL
WATER > 2500 -> WARNING
OBJECT detected -> WARNING
SOUND > 135 -> WARNING
Otherwise -> NORMAL
```

Critical has priority over warning. When an alert is permitted by the selected alert mode, WARNING uses rapid red flashing plus warning tones; CRITICAL uses faster red flashing plus critical tones.

## Sensor-module potentiometers

Several inexpensive modules contain onboard trimmer potentiometers. These generally adjust comparator sensitivity or a digital switching threshold; they should not be assumed to represent a calibrated change in the physical quantity. V1.1 therefore keeps the software sound threshold explicit at 135 and treats module potentiometer adjustments as hardware characterization variables.

This applies particularly to comparator-equipped sound, Hall, flame/IR, and similar modules.

## Heartbeat

No heartbeat-processing change was made. The HW502 continues using the existing experimental baseline/threshold BPM estimation because the bench sensor is producing a changing signal. The result remains an experimental estimate, not a medical measurement.

## Files

```text
firmware/vigil01_v1.ino
firmware/vigil01_v1_1.ino
docs/HARDWARE.md
```

The original V1 file is retained as the baseline for comparison; V1.1 is a separate firmware artifact so the evolution can be documented cleanly.

## Validation checklist

- [ ] OLED boot and HOME layout remain unchanged
- [ ] Existing menus navigate correctly
- [ ] UP/DOWN/SELECT/BACK produce feedback tones
- [ ] Green LED is solid when enabled
- [ ] Red LED stays off on HOME/menu screens in PAGE mode
- [ ] Sound above 135 produces WARNING
- [ ] Sensor pages can present warnings/critical alerts
- [ ] GLOBAL mode presents alerts outside sensor pages
- [ ] LEDS OFF disables both LEDs
- [ ] BUZZER OFF disables navigation/alarm tones
- [ ] Live `/data` dashboard remains functional
- [ ] Breadboard prototype remains electrically stable during integrated testing
- [ ] Validated breadboard behavior is recorded before PCB design begins

## Engineering rationale

The key design decision is to separate **sensing**, **interpretation**, and **presentation**. VIGIL-01 does not stop monitoring when the user is navigating. Instead, V1.1 lets the user choose whether the interpreted event should be surfaced through the physical alarm outputs globally or only when inspecting sensor pages. This preserves automatic sensing while making the human interface controllable and demonstrable.

The same engineering philosophy applies to the hardware: the breadboard is used as a flexible validation platform, while the future PCB and soldered assembly represent a later, permanent hardware revision based on measured and validated prototype results.
