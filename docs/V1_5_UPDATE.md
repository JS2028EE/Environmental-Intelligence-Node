# VIGIL-01 V1.5 Update

**Date:** 2026-09-07

V1.5 is a controlled extension of the V1.4 firmware. Existing sensor pins, shared I²C architecture, alarm routing, fall sequence timing, battery-divider behavior, and historical development records are preserved.

## Changes implemented

### 1. Persistent fall thresholds

The three numeric fall thresholds are now part of the existing NVS settings system:

- Free-fall threshold
- Impact threshold
- Post-impact tilt threshold

The defaults remain the V1.4 values, so existing behavior is preserved until a user changes a threshold. The local dashboard and `/api/settings` expose these values, and the API applies sanity bounds before saving them.

The fall sequence timing values remain compile-time constants. This limits the scope of routine tuning and avoids changing the temporal behavior of the validated-fall state machine through an ordinary settings edit.

### 2. In-RAM event history

A new `EventLog.cpp/.h` module records the most recent 16 rising event transitions. `/events` exposes the records to the local dashboard.

The buffer is volatile and intentionally clears on reboot. No persistent storage was added.

### 3. Documentation boundary

Historical V1.1 and V1.4 update documents are retained as historical records. They were not rewritten to make them falsely describe later firmware. Current-state information is represented in the main README and this V1.5 update.

## Intentionally not changed

- MPU accelerometer/gyro register architecture
- Shared OLED + MPU I²C wiring on GPIO21/GPIO22
- MPU live fault handling
- PAGE/GLOBAL alarm policy
- TCRT5000 exclusion from GLOBAL alarms
- Validated fall as a system-level critical event
- Distinct fall buzzer pattern
- Battery percentage/divider implementation
- Existing historical development/event documentation
- Magnetometer support

## Magnetometer decision

The MPU board is identified as belonging to the MPU-9250/MPU-6500/MPU-9255 family, but the exact magnetometer silicon and module routing have not yet been positively validated. V1.5 therefore does **not** enable a magnetometer merely because the board is labeled as a 9250-family module. This avoids changing the working I²C subsystem based on an unverified hardware assumption.

## Validation required

V1.5 software changes require physical validation on the actual breadboard:

1. Confirm fall thresholds load correctly after reboot.
2. Change each fall threshold from the dashboard and verify the detector uses the new value.
3. Confirm invalid threshold values are rejected.
4. Trigger defined sensor transitions and verify `/events` records each rising transition once.
5. Verify the 16-entry ring buffer overwrites its oldest records when full.
6. Verify reboot clears the RAM event history.
7. Confirm existing PAGE/GLOBAL behavior remains unchanged.
8. Confirm ordinary walking/running still does not produce a fall alarm.

V1.5 is still a prototype and these changes do not constitute safety certification or calibrated sensing.
