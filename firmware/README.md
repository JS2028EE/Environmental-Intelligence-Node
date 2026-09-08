# VIGIL-01 — V1.5 Firmware

This directory contains the active modular VIGIL-01 firmware. V1.5 preserves the corrected MPU-9250-family motion subsystem and V1.4 alarm behavior while adding persistent web-tunable fall thresholds and a bounded in-RAM event history endpoint.

## Current architecture

```text
VIGIL01.ino       Main setup/loop coordinator
Config.h          Pins, I2C addresses, defaults, timing
Secrets.h         Local-only AP credential file (ignored by Git)
Types.h           Shared screen/status types
Sensors.cpp/.h    Sensor acquisition, MPU-9250-family processing, fall detection
EventLog.cpp/.h   Volatile 16-entry event ring buffer
MenuData.h        Menu tree
Navigation.cpp/.h Buttons/navigation/debounce
DisplayUI.cpp/.h OLED rendering
Alerts.cpp/.h     LED/buzzer presentation
Settings.cpp/.h   Persistent NVS settings
WebDashboard.cpp/.h Wi-Fi AP/dashboard/API/captive portal/mDNS
Watchdog.cpp/.h   Watchdog recovery
```

## Credentials

The AP SSID remains `VIGIL-01`, but the deployment password is supplied by local `firmware/Secrets.h`. The real file is ignored by Git. Copy `firmware/Secrets.h.example` to `firmware/Secrets.h` and set the password before deployment. If the local file is absent, the firmware uses a compile-safe placeholder that must not be used for deployment.

## MPU-9250 / MPU-6500 / MPU-9255 motion subsystem

The physical module is a board marked for the MPU-9250/MPU-6500/MPU-9255 family. The firmware reads `WHO_AM_I` and accepts `0x70`, `0x71`, and `0x73`, probing `0x68` and `0x69` on the shared OLED I²C bus. The MPU is accessed directly through `Wire`; no MPU6050 library is used.

The accelerometer and gyro are active. Magnetometer support is intentionally not enabled until the exact module/silicon path is positively validated.

## Live IMU fault handling

`mpuPresent` is based on live communication, not only the boot-time probe. A failed motion register read clears `mpuPresent`, invalidates motion telemetry, and causes periodic rediscovery/reconfiguration attempts. This prevents a stale `IMU OK` state after a breadboard connection fails and permits recovery without rebooting.

## Motion vs. alarm behavior

Motion, impact, and tilt flags are telemetry and do not directly trigger the physical alarm.

A fall is treated as:

```text
LOW-G / FREE-FALL
      ↓
HIGH-G IMPACT
      ↓
SUSTAINED POST-IMPACT TILT
      ↓
FALL EVENT
```

Default thresholds remain free-fall `< 4.0 m/s²`, impact `> 25.0 m/s²`, and post-impact tilt `> 45°`. These three thresholds are now loaded from NVS and can be changed through the dashboard or `/api/settings`. Fall sequence timing remains compile-time.

## PAGE vs GLOBAL alerts

PAGE mode responds only to the alarm condition associated with the selected page. GLOBAL mode uses defined system-wide alarm conditions. The HW511/TCRT5000 IR reflection sensor is intentionally excluded from GLOBAL mode because outdoor testing demonstrated nuisance detections. A validated fall remains system-level and can activate the physical alarm regardless of the selected page.

Validated falls use a distinct rapid 2500 Hz buzzer pattern while other critical conditions retain the standard critical tone.

## I²C ownership

`Sensors.cpp` owns initialization of the shared I²C bus:

```text
Wire.begin(GPIO21, GPIO22)
Wire.setClock(100000)
```

`DisplayUI.cpp` uses the configured `Wire` object without reinitializing the bus.

## Dashboard

`/data` exposes live sensor, motion, status, settings, and alert telemetry. `/api/settings` provides persistent settings read/write access for LEDs, buzzer, PAGE/GLOBAL alerts, units, sound threshold, water threshold, and the three fall thresholds.

## Event history

V1.5 adds a 16-entry volatile event ring buffer. It records rising transitions for sound, water, object detection, IR reflection, flame, and validated fall events.

```text
GET /events
```

The endpoint returns event uptime, type, and severity. Reboot clears the buffer. Events are not written to flash, SD, or cloud storage. See `docs/EVENT_LOGGING.md`.

## Libraries

Required Arduino Library Manager libraries:

- Adafruit GFX Library
- Adafruit SSD1306
- DHT sensor library by Adafruit
- ArduinoJson 6.x

## Automated build

GitHub Actions compiles the `firmware/` sketch on pushes and pull requests using the ESP32 Arduino core and the required libraries. ArduinoJson is pinned to 6.21.5 to match the firmware API. CI does not replace physical validation.

## Validation

1. Confirm the Serial message identifies the MPU family and I²C address.
2. Disconnect the motion module during operation and confirm `mpuPresent` becomes false.
3. Restore the connection and confirm the IMU can recover without rebooting.
4. Walk/run normally and confirm motion telemetry changes without an alarm.
5. Change fall thresholds from the dashboard and confirm persistence after reboot.
6. Confirm PAGE/GLOBAL alarm behavior remains unchanged.
7. Trigger defined events and confirm they appear once in `/events`.
8. Generate more than 16 events and confirm the oldest records roll off.
9. Reboot and confirm RAM event history clears.
10. Test controlled fall-like sequences only in a safe setup.
