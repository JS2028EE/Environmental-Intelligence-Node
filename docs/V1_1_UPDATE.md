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

Sensor sampling and overall status evaluation remain continuous. The corrected V1.1 alert logic now separates **overall system status** from **whether the currently displayed page should physically alarm**.

Previously, PAGE mode could still flash the red LED and sound the buzzer while viewing an unrelated sensor because `systemStatus` was being generated from other sensors. For example, an object/IR condition could make the entire system WARNING, which then caused a warning output on a temperature, humidity, heart, light, or other page. This was not the intended PAGE behavior.

The corrected behavior is:

```text
ALERTS: PAGE
  -> Only the currently relevant danger/event condition can activate red LED + alarm.

ALERTS: GLOBAL
  -> Any defined system danger/event can activate red LED + alarm from any screen.
```

Opening a sensor page by itself is never an alert. A normal value is never an alarm.

### Normal-state LED behavior

When LEDs are enabled and there is no active alert, the **green LED stays solid ON**, including while navigating and while viewing sensor pages. The green LED turns OFF only while a real warning/critical alert is actively being presented, or when `LEDS: OFF` is selected.

The red LED is reserved for active warning/critical conditions. It is OFF during normal operation.

This gives the physical interface a clear state model:

```text
GREEN SOLID = system normal / no active physical alert
RED FLASH   = active warning or critical event
NO LED      = LEDs disabled
```

### Output settings

SYSTEM contains SETTINGS with:

```text
SETTINGS
> LEDS: ON/OFF
  BUZZER: ON/OFF
  ALERTS: PAGE/GLOBAL
```

UP/DOWN selects a setting; SELECT toggles it; BACK returns to SYSTEM.

`LEDS OFF` suppresses both LEDs. `BUZZER OFF` suppresses navigation and alarm tones while sensor detection continues.

### Alert conditions

The firmware now defines physical alarm conditions explicitly instead of treating every sensor reading or every sensor page as an alert:

```text
FLAME/IR event -> CRITICAL
WATER > 2500 -> WARNING
OBJECT detected -> WARNING
SOUND > 135 -> WARNING
Other currently implemented sensor pages -> no physical alarm condition by default
Otherwise -> NORMAL
```

Critical has priority over warning. In PAGE mode, only the condition associated with the displayed sensor/condition page is allowed to activate the physical alert outputs. In GLOBAL mode, the overall system status can activate them from any screen.

The `/data` endpoint also reports `activeAlert` so the dashboard can distinguish an overall interpreted status from whether the physical alert output is currently active on the present screen.

### Digital sensor polarity correction

The previous firmware treated the digital outputs of the IR/flame-related modules as active-high without an explicit polarity conversion. Common versions of these inexpensive comparator modules use **active-low detection outputs**, meaning LOW represents an event and HIGH represents clear. V1.1.1 now makes this polarity explicit in firmware:

```text
TCRT_ACTIVE_LOW
IR_ACTIVE_LOW
FLAME_ACTIVE_LOW
```

The physical module behavior should still be verified during breadboard characterization because inexpensive module variants can differ. Keeping polarity as a named configuration makes that verification and later PCB revision easier.

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
- [ ] Green LED is solid when LEDs are enabled and the system has no active alert
- [ ] Green LED turns off only during an active physical warning/critical alert
- [ ] Red LED stays off during normal operation
- [ ] Red LED stays off on unrelated sensor/menu pages in PAGE mode
- [ ] Opening a normal sensor page does not trigger the alarm
- [ ] Sound above 135 produces WARNING when the sound condition is being presented
- [ ] Water above 2500 produces WARNING when the water condition is being presented
- [ ] Object detection produces WARNING on the object/conditions page
- [ ] Flame/IR detection produces CRITICAL on the flame/conditions page
- [ ] GLOBAL mode presents defined alerts outside the relevant sensor page
- [ ] LEDS OFF disables both LEDs
- [ ] BUZZER OFF disables navigation/alarm tones
- [ ] Live `/data` dashboard remains functional
- [ ] Breadboard prototype remains electrically stable during integrated testing
- [ ] Digital sensor polarity is verified against the actual module hardware
- [ ] Validated breadboard behavior is recorded before PCB design begins

## Engineering rationale

The key design decision is to separate **sensing**, **interpretation**, and **presentation**. VIGIL-01 does not stop monitoring when the user is navigating. Instead, V1.1 lets the user choose whether the interpreted event should be surfaced through the physical alarm outputs globally or only when inspecting the relevant sensor/condition page.

The corrected implementation goes one step further: a page must have a defined danger condition before it can physically alarm in PAGE mode. This prevents unrelated sensor activity from making every screen appear to be in danger and prevents normal sensor readings from producing warning behavior.

The same engineering philosophy applies to the hardware: the breadboard is used as a flexible validation platform, while the future PCB and soldered assembly represent a later, permanent hardware revision based on measured and validated prototype results.
