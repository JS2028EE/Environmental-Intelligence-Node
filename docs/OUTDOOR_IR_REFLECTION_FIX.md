# VIGIL-01 — Outdoor IR Reflection Alarm Investigation

Date: 2026-09-07

## Incident

During outdoor testing, VIGIL-01 was set to `ALERTS: GLOBAL`. The physical alarm began sounding outdoors and stopped when the device returned indoors.

The photoresistor also changed significantly, rising from typical indoor readings around 2000–3000 toward the ESP32 ADC maximum of about 4095. This was investigated as a possible cause, but the light sensor is not an alarm source in the current firmware.

## Sensors isolated

VIGIL-01 has two separate IR-related modules plus the photoresistor:

| GPIO | Sensor | Function |
|---:|---|---|
| 26 | HW511 / TCRT5000 | IR reflection / reflectivity |
| 27 | IR obstacle module | Object detection |
| 33 | Photoresistor | Relative light |

The decisive field observation was that the **HW511/TCRT5000 IR reflection sensor changed to DETECTED outdoors and cleared indoors at the same time the physical alarm stopped**. The IR obstacle sensor was not the changing condition identified during this test.

## Root cause

The TCRT5000/HW511 is a close-range reflectivity sensor. Outdoor illumination contains substantial infrared energy and can change the sensor's reflected-IR detection state. Therefore, its digital detection state is not suitable as a continuously active global alarm condition.

The LDR reaching approximately 4095 is expected 12-bit ESP32 ADC saturation. It is a measurement effect, not an alarm condition by itself.

## Firmware correction

The alert system previously had no PAGE-scoped alarm path for `IR_REFLECTION_SCREEN`. The correction in `firmware/Alerts.cpp` adds:

```text
IR_REFLECTION_SCREEN -> sensors.tcrtDetected
```

The resulting policy is:

| Alert source | PAGE mode | GLOBAL mode |
|---|---|---|
| Sound threshold | SOUND page only | Global |
| Water threshold | WATER page only | Global |
| IR obstacle | OBJECT page only | Global |
| Flame/IR | FLAME page only | Global |
| Validated fall | System-wide | System-wide |
| IR reflection / TCRT5000 | IR REFLECTION page only | **No alarm** |

The TCRT5000 has **not** been removed. It remains a live investigation/telemetry sensor.

## Why this design is intentional

`PAGE` and `GLOBAL` have different purposes.

### PAGE

Only the alarm condition belonging to the currently selected sensor page is allowed to activate the physical alarm. For example, while on `IR REFLECTION`, a TCRT5000 detection can sound the alarm. Being on another page does not make the TCRT5000 alarm.

### GLOBAL

Only explicitly defined system-wide alarm conditions activate the physical alarm. The TCRT5000 reflection sensor is intentionally excluded because its outdoor behavior demonstrated that it can create nuisance detections.

A validated fall remains system-wide because suppressing a validated fall merely by navigating to another page would be undesirable.

## Engineering principle reinforced

A sensor can be functioning correctly while its signal is inappropriate for a global alarm.

The correct architecture is:

```text
Sensor detects condition
        ↓
Raw/state telemetry remains available
        ↓
Alarm policy decides whether condition is actionable
        ↓
PAGE   -> selected sensor/page condition
GLOBAL -> defined system-wide alarm conditions
```

This prevents environmental effects from turning an investigation sensor into a nuisance alarm while preserving its usefulness.

## Validation matrix

The following tests are required after the firmware update:

1. `PAGE + IR REFLECTION`: cause TCRT5000 detection -> physical alarm must activate.
2. `PAGE + unrelated page`: TCRT5000 detection -> physical alarm must remain off.
3. `GLOBAL + IR reflection only`: TCRT5000 detection -> physical alarm must remain off.
4. `GLOBAL + sound/water/object/flame`: defined alarm condition -> physical alarm must activate.
5. `PAGE + sound/water/object/flame`: corresponding page condition -> physical alarm must activate.
6. Outdoor sunlight: record LDR, TCRT5000, IR obstacle, and alarm state together.
7. Confirm that an LDR value near 4095 alone does not produce an alarm.

## Status

Firmware routing fix implemented in `Alerts.cpp`.

Physical validation of the corrected PAGE/GLOBAL behavior remains part of the next test campaign.
