# VIGIL-01 V1.4 Firmware Update

Date: 2026-09-07

## Summary

V1.4 records the successful return of the motion subsystem using the actual installed MPU-9250/MPU-6500/MPU-9255-family module and corrects the previous alert behavior that treated normal movement as an alarm condition.

## Motion hardware correction

The installed breakout is marked for the MPU-9250 / MPU-6500 / MPU-9255 family. The firmware no longer assumes an MPU6050 or depends on the Adafruit MPU6050 library.

The firmware reads `WHO_AM_I` and accepts:

```text
0x70 = MPU-6500
0x71 = MPU-9250
0x73 = MPU-9255
```

It probes I²C addresses `0x68` and `0x69` and uses the existing OLED bus on GPIO21/GPIO22.

## Shared I²C architecture

The motion module and OLED share one I²C bus. `Sensors.cpp` owns `Wire.begin(GPIO21, GPIO22)` and sets 100 kHz. The OLED no longer reinitializes the bus.

## Problem discovered during integrated testing

The previous motion logic used a simple acceleration threshold and tilt threshold in overall system status. Fast walking/running could therefore produce `WARNING`, while a large acceleration spike could produce `CRITICAL`.

That behavior was not aligned with the purpose of the motion subsystem. The goal is to know how the device is moving/oriented and to identify a possible fall, not to alarm every time the user moves quickly.

## V1.4 behavior change

Motion is now divided into two paths:

```text
MOTION TELEMETRY
  ├── acceleration X/Y/Z
  ├── gyro X/Y/Z
  ├── acceleration magnitude
  ├── tilt
  ├── movement state
  ├── impact telemetry
  └── tilt telemetry

FALL DETECTION
  └── staged fall sequence -> physical alarm
```

Normal movement, rotation, tilt, and isolated impact telemetry no longer directly activate the physical alarm.

## Fall detector

The detector requires a sequence:

```text
1. Low-g / free-fall
        ↓
2. Significant impact
        ↓
3. Sustained post-impact tilt
        ↓
4. FALL EVENT / CRITICAL
```

Prototype parameters:

```text
Free-fall threshold       < 4.0 m/s²
Impact threshold           > 25.0 m/s²
Post-impact tilt           > 45°
Free-fall → impact window  ≤ 1200 ms
Tilt confirmation          300 ms
Fall alert hold            3000 ms
```

## Motion-state telemetry

The firmware exposes `STABLE`, `MOVING`, `ROTATING`, `FAST/IMPACT`, and `FREE-FALL` where applicable. The dashboard exposes the same motion state and a dedicated `fallDetected` field.

## Alert-system incident: outdoor IR reflection

### Observation

During outdoor testing on 2026-09-07, the physical alarm activated while the device was set to `ALERTS: GLOBAL`. The photoresistor reading also increased substantially, reaching the ESP32 ADC maximum of approximately 4095, but the light sensor itself was not the alarm source.

The two separate IR-related modules were isolated during the investigation:

```text
GPIO26 -> HW511 / TCRT5000 -> IR reflection
GPIO27 -> IR obstacle sensor -> object detection
GPIO33 -> photoresistor -> relative light
```

The decisive observation was that the **IR reflection sensor changed state when moving between indoor and outdoor conditions at the same moment the alarm behavior changed**. Outdoors, the reflection sensor reported detection. Returning indoors cleared the reflection detection and the alarm stopped.

The field behavior is consistent with strong outdoor illumination/infrared content interacting with the reflective IR subsystem. The photoresistor reaching 4095 is ADC saturation and is not itself an alarm condition in the current firmware.

### Root cause and design decision

The HW511/TCRT5000 is an investigation sensor intended for close-range reflectivity measurements. Its digital state can change because of environmental IR and surface reflections, so it should not be a continuously active global alarm source.

The previous alert routing also had no PAGE-scoped alarm path for `IR_REFLECTION_SCREEN`. The sensor could therefore be observed but could not produce the intended physical alert when specifically investigating it.

### Correction

`Alerts.cpp` now maps the IR reflection page to the TCRT5000 detection state:

```text
IR_REFLECTION_SCREEN -> sensors.tcrtDetected
```

The routing is now:

| Alert source | PAGE mode | GLOBAL mode |
|---|---|---|
| Sound threshold | SOUND page only | Global |
| Water threshold | WATER page only | Global |
| IR obstacle | OBJECT page only | Global |
| Flame/IR | FLAME page only | Global |
| Validated fall | System-wide | System-wide |
| IR reflection / TCRT5000 | IR REFLECTION page only | **No alarm** |

The TCRT5000 sensor remains fully present in sensing and telemetry; it was not removed.

## Engineering hardening added after the initial V1.4 implementation

### Live IMU fault handling

The motion sensor is no longer considered permanently healthy just because the boot-time `WHO_AM_I` probe succeeded. A failed live register read now clears `mpuPresent`, invalidates motion telemetry, and causes periodic rediscovery/reconfiguration attempts. This is especially important on a solderless breadboard where an I²C connection can be disturbed during testing.

### Dashboard settings

The dashboard now exposes the persistent settings that were already supported by `/api/settings` but were not rendered in the HTML: LEDs, buzzer, PAGE/GLOBAL alerts, and C/F units. Sound and water thresholds remain editable as before.

### Distinct fall alert

Validated falls now use a distinct rapid 2500 Hz tone rather than sharing the standard critical tone used for flame/IR. The severity remains CRITICAL; only the presentation differs.

### Credential separation

The AP password was removed from tracked source and documentation. The password is now supplied through local `firmware/Secrets.h`, which is ignored by Git, with `firmware/Secrets.h.example` provided as a template. See `docs/SECURITY.md`.

### Automated build checking

A GitHub Actions workflow now compiles the ESP32 firmware on pushes and pull requests using the ESP32 Arduino core and required libraries.

## Engineering rationale

The change explicitly separates **sensor detection** from **alarm policy**:

```text
Sensor detects something
        ↓
Sensor state remains available
        ↓
Alert policy determines whether it is actionable
        ↓
PAGE   = selected page's alarm condition
GLOBAL = defined system-wide alarm conditions
```

The TCRT5000 is deliberately PAGE-only because outdoor testing demonstrated that making it global would create nuisance alarms.

## Dashboard improvement

Dashboard polling was reduced from 700 ms to 1000 ms, and the dashboard now displays motion-sensor presence, motion state, acceleration, gyro, tilt, motion/impact/tilt telemetry, fall-event state, and the existing persistent settings.

## Important engineering limitation

The MPU can describe orientation and movement but cannot provide reliable absolute 3D position indefinitely from accelerometer/gyro integration alone. Sensor bias and integration drift accumulate. V1.4 therefore treats the feature as orientation and movement awareness, not precise absolute position tracking.

## Documentation audit

Active documentation must remain aligned with this V1.4 alert routing change. The historical `docs/V1_1_UPDATE.md` remains historical and is not rewritten to claim later features existed in V1.1.

## Validation plan

### Motion

- Confirm boot `WHO_AM_I` identity and I²C address.
- Confirm a live IMU disconnect changes `mpuPresent` from true to false.
- Confirm a restored IMU connection can recover without rebooting.
- Confirm stationary acceleration magnitude near 9.8 m/s².
- Confirm rotation changes gyro and tilt.
- Walk and run normally; verify no physical alarm.
- Verify a standalone impact does not alarm.
- Perform controlled, safe fall-sequence testing and verify the staged detector.

### Alert routing

- PAGE + SOUND: exceed sound threshold on SOUND page -> alarm.
- PAGE + WATER: exceed water threshold on WATER page -> alarm.
- PAGE + OBJECT: trigger IR obstacle -> alarm.
- PAGE + FLAME: trigger flame/IR condition -> alarm.
- PAGE + IR REFLECTION: trigger TCRT5000 detection -> alarm.
- PAGE + unrelated page: unrelated sensor condition -> no physical alarm.
- GLOBAL + sound/water/object/flame/fall: defined global condition -> alarm.
- GLOBAL + IR reflection only: TCRT5000 detection -> **no physical alarm**.
- Outdoor sunlight: record LDR, TCRT5000, IR obstacle, and alarm state.
- Confirm fall and non-fall critical alert tones are distinguishable.

V1.4 fall detection and sensor alarms are experimental and are not a certified life-safety system.
