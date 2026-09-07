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

This preserves the earlier debugging correction that removed duplicate I²C initialization.

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

The intention is to reject normal walking/running as an alarm while retaining useful motion data.

## Motion-state telemetry

The firmware exposes:

```text
STABLE
MOVING
ROTATING
FAST/IMPACT
FREE-FALL
```

The web dashboard now exposes the same motion state and a dedicated `fallDetected` field.

## Dashboard improvement

Dashboard polling was reduced from 700 ms to 1000 ms. The dashboard now visibly reports:

- motion sensor presence
- motion state
- acceleration
- gyro
- tilt
- motion telemetry
- impact telemetry
- tilt telemetry
- fall-event state

## Important engineering limitation

The MPU can describe orientation and movement but cannot provide reliable absolute 3D position indefinitely from accelerometer/gyro integration alone. Sensor bias and integration drift accumulate. V1.4 therefore treats the feature as **orientation and movement awareness**, not precise absolute position tracking.

## Documentation audit

The following active documentation was updated to remove stale MPU6050 assumptions and align with V1.4:

- `README.md`
- `firmware/README.md`
- `docs/FIRMWARE.md`
- `docs/HARDWARE.md`
- `docs/DEVELOPMENT_LOG.md`

The historical `docs/V1_1_UPDATE.md` remains a historical record and is not rewritten to pretend V1.1 contained later features.

## Validation plan

- Confirm boot `WHO_AM_I` identity and I²C address.
- Confirm stationary acceleration magnitude near 9.8 m/s².
- Confirm rotation changes gyro and tilt.
- Walk and run normally; verify no physical alarm.
- Produce controlled acceleration changes; verify telemetry remains available.
- Verify a standalone impact does not alarm.
- Perform controlled, safe fall-sequence testing and verify the staged detector.

V1.4 fall detection is experimental and is not a certified life-safety system.
