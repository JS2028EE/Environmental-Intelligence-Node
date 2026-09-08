# VIGIL-01 Firmware Architecture — V1.5

## Overview

V1.5 is the active modular ESP32 firmware for VIGIL-01. It samples the live sensor set, renders the OLED interface, serves the local dashboard, manages persistent settings, maintains a bounded volatile event history, and drives physical status outputs.

The installed MPU module is handled as an **MPU-9250 / MPU-6500 / MPU-9255 family device**, not as an MPU6050. The firmware identifies supported silicon through `WHO_AM_I` and accesses the common accelerometer/gyroscope registers directly. Magnetometer support remains deferred pending positive validation of the exact module/silicon path.

## Firmware modules

```text
VIGIL01.ino       Main coordinator
Config.h          GPIOs, I2C, defaults, timing, AP configuration
Secrets.h         Local-only AP credential file (ignored by Git)
Types.h            Shared screen/status types
Sensors.cpp/.h    Sensor acquisition + motion/fall processing
EventLog.cpp/.h   Volatile 16-entry event ring buffer
Navigation.cpp/.h Button input/navigation
MenuData.h        Menu definitions
DisplayUI.cpp/.h  OLED rendering
Alerts.cpp/.h     Physical alert presentation
Settings.cpp/.h   Persistent NVS/flash settings
WebDashboard.cpp/.h Local AP/dashboard/API/captive portal/mDNS
Watchdog.cpp/.h   ESP32 watchdog
```

## Credential handling

The AP SSID remains `VIGIL-01`, but the password is no longer hardcoded in tracked source. A local `firmware/Secrets.h` supplies `AP_PASSWORD`; `firmware/Secrets.h.example` is the committed template and `.gitignore` excludes the real file.

If `Secrets.h` is absent, the firmware uses a compile-safe placeholder that must be replaced before deployment. See `docs/SECURITY.md`.

## Motion subsystem

The motion module shares GPIO21/GPIO22 with the SSD1306 OLED. The firmware probes `0x68` first and `0x69` second and accepts WHO_AM_I values `0x70`, `0x71`, and `0x73`. Configuration is ±8 g acceleration, ±500 °/s gyro, and 100 kHz I²C.

The accelerometer and gyro are active in V1.5. The magnetometer is intentionally not enabled yet because the exact board/silicon path has not been positively validated.

## Live IMU fault handling

`mpuPresent` is tied to live register reads, not only the boot-time probe. A failed read clears `mpuPresent`, invalidates motion telemetry, and triggers periodic rediscovery/reconfiguration so a restored connection can recover without rebooting.

## Motion and fall behavior

Motion, impact, and tilt flags are telemetry and do not directly trigger the physical alarm. The staged fall sequence remains:

```text
LOW-G / FREE-FALL
      ↓
HIGH-G IMPACT
      ↓
SUSTAINED POST-IMPACT TILT
      ↓
FALL EVENT / CRITICAL
```

Default prototype thresholds remain `< 4.0 m/s²` free-fall, `> 25.0 m/s²` impact, and `> 45°` post-impact tilt. These three thresholds are now loaded from NVS and can be changed through the local dashboard or `/api/settings`. The sequence timing values remain compile-time constants.

## Alarm policy

PAGE mode is page-scoped. GLOBAL mode uses only defined system-wide alarm conditions. The TCRT5000 IR reflection sensor remains intentionally excluded from GLOBAL mode because outdoor testing demonstrated nuisance detections. A validated fall remains system-level and can activate the physical alarm regardless of the selected page.

Validated falls use a distinct rapid 2500 Hz buzzer pattern while other critical conditions retain the standard critical tone.

## Dashboard and API

`/data` exposes live sensor, motion, status, settings, and alert telemetry. `/api/settings` reads/writes persistent settings, including the three fall thresholds. `/events` exposes the volatile event history.

## Event history

V1.5 adds a bounded 16-entry in-RAM ring buffer. It records rising transitions for sound, water, object detection, IR reflection, flame, and validated fall events. Each record contains uptime, type, and severity. Reboot clears the buffer; no SD, flash, NVS, or cloud storage is used.

See `docs/EVENT_LOGGING.md` for the event model and limitations.

## Required libraries

- Adafruit GFX Library
- Adafruit SSD1306
- DHT sensor library
- ArduinoJson 6.x

The MPU family is accessed directly through `Wire`; no MPU6050-specific library is required.

## Automated build checking

GitHub Actions compiles the firmware on pushes and pull requests. The workflow pins ArduinoJson to 6.21.5 to match the firmware's ArduinoJson 6 API usage. CI build success does not replace physical hardware validation.

## Current limitations

- Fall thresholds are prototype heuristics and require controlled characterization.
- Fall timing remains compile-time.
- Magnetometer support is deferred pending positive hardware validation.
- Absolute 3D position cannot be maintained reliably from this 6-axis IMU alone because integration drifts.
- Heartbeat and analog sensors remain experimental/raw until characterized.
- Battery monitoring remains disabled in tracked firmware.
- Event history is volatile and limited to 16 entries.
- No persistent telemetry or cloud storage is implemented.
- The dashboard is a local prototype service without production authentication or encryption.

## Validation checklist

- Confirm live IMU disconnect changes `mpuPresent` to false and restored connection recovers.
- Confirm normal walking/running does not physically alarm.
- Change fall thresholds through the dashboard and confirm persistence after reboot.
- Confirm invalid fall thresholds are rejected.
- Confirm defined events appear once per rising transition in `/events`.
- Confirm more than 16 events roll out the oldest records.
- Confirm reboot clears the RAM event history.
- Confirm PAGE/GLOBAL behavior remains unchanged.
- Confirm fall and non-fall critical tones remain distinguishable.
- Validate the complete fall sequence only in a safe controlled test.
