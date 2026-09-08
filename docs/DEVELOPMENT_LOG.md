# VIGIL-01 Development Log

## 2026-09-05 — V1 Architecture Locked

### Project direction

The project was formalized as **VIGIL-01 — Portable Environmental Intelligence Node**. The objective is to build a coherent portable sensing instrument rather than a collection of unrelated sensor demonstrations.

### V1 scope

V1 is live/in-the-moment only:

- No SD card
- No historical data logging
- No cloud database
- No cloud storage
- Local Wi-Fi dashboard remains available for live telemetry

### Selected sensing functions

**Environment**
- DHT11 temperature/humidity
- Photoresistor relative light
- Sound sensor relative acoustic signal
- Flame/IR detection

**Vitals**
- HW502 heartbeat/pulse sensing

**Investigate**
- HW511/TCRT5000 IR reflectivity
- IR obstacle detection
- 49E linear Hall-effect magnetic sensing
- Water sensing
- Flame/IR investigation

### UI architecture

The 0.96-inch 128×64 SSD1306 OLED uses I²C on GPIO21/GPIO22. Four buttons provide UP/DOWN/SELECT/BACK navigation.

### Hardware decisions recorded

- OLED: SDA GPIO21, SCL GPIO22
- UP: GPIO16
- DOWN: GPIO17
- SELECT: GPIO18
- BACK: GPIO19
- DHT11 data: GPIO25
- HW511 signal: GPIO26
- IR obstacle OUT: GPIO27
- Flame/IR digital output: GPIO23
- Heartbeat analog: GPIO32
- Light divider: GPIO33
- Sound analog: GPIO34
- Linear Hall A0: GPIO35
- Water analog: GPIO36
- Green LED: GPIO13 through 330 Ω
- Red LED: GPIO14 through 330 Ω
- Passive buzzer: GPIO4 to GND
- GPIO39 reserved for future battery monitoring

### Power decision

The current breadboard is powered from the ESP32 USB connection. Battery development was postponed until a compact regulated power architecture could be selected. A later bench experiment demonstrated battery-power feasibility using an XTR 502030 3.7 V 200 mAh Li-ion cell through the prototype power-conversion path. This remains a feasibility result rather than the final PCB power architecture.

### Engineering principle

Do not claim precision that has not been measured. Raw sensor values remain raw/relative until calibration and characterization are performed.

## 2026-09-05 — V1.1 Alert-System Debugging and Correction

Integrated breadboard testing exposed an alert-presentation bug. V1.1 separated sensing, interpretation, and presentation so unrelated sensor conditions would not automatically make the currently displayed page appear to be in alarm.

The current project continues that separation: sensor telemetry is not automatically an alarm condition unless the active system logic explicitly promotes it.

## 2026-09-05 — Flame Sensor Polarity Reversal Corrected

Physical testing showed that the actual flame sensor module had the opposite polarity from the earlier assumption. Current configuration is:

```text
FLAME_ACTIVE_LOW = false
LOW  = CLEAR
HIGH = FLAME/IR EVENT
```

## 2026-09-07 — MPU-9250 Family Identification and Motion Subsystem Correction

### Hardware identification

The installed motion breakout is marked for the **MPU-9250 / MPU-6500 / MPU-9255 family**, not as a confirmed MPU6050. The firmware was updated to identify supported silicon by reading `WHO_AM_I`:

```text
0x70 = MPU-6500
0x71 = MPU-9250
0x73 = MPU-9255
```

Both I²C addresses `0x68` and `0x69` are probed. The module remains on the shared OLED bus at GPIO21/GPIO22.

### Firmware implementation

The old Adafruit MPU6050 dependency was removed. `Sensors.cpp` now accesses the common accelerometer/gyroscope register map directly and converts the raw data to acceleration and angular velocity values used by the existing UI/dashboard.

This was a necessary correction because the physical module is not being treated as an MPU6050.

### Successful validation

The motion subsystem is now operational on the physical prototype. This marks the transition from motion-sensor debugging to motion-feature development.

## 2026-09-07 — V1.4 Motion Telemetry and Fall Detection

### Problem observed

Integrated testing showed that fast walking/running could trigger the alarm because the previous status logic treated motion and raw impact as alarm conditions. That was not the intended behavior.

### Design decision

The MPU is now treated as both a movement/orientation telemetry instrument and a fall detector.

Normal movement is not an alarm:

```text
Walking       -> telemetry only
Running       -> telemetry only
Rotation      -> telemetry only
Tilt          -> telemetry only
Impact spike  -> telemetry only
```

Only a staged fall sequence is promoted to a physical critical alarm.

### Fall detection sequence

```text
LOW-G / FREE-FALL
      ↓
HIGH-G IMPACT
      ↓
SUSTAINED POST-IMPACT TILT
      ↓
FALL EVENT / CRITICAL
```

Prototype parameters:

```text
Free-fall:          < 4.0 m/s²
Impact:             > 25.0 m/s²
Post-impact tilt:   > 45°
Sequence window:    1200 ms
Tilt confirmation:  300 ms
Alert hold:         3000 ms
```

The sequence is intentionally stricter than the previous single-threshold approach so normal running/walking does not produce an alarm merely because acceleration changes.

### New motion-state telemetry

The firmware now classifies movement as:

```text
STABLE
MOVING
ROTATING
FAST/IMPACT
FREE-FALL
```

The dashboard exposes this state along with acceleration, gyro, tilt, impact telemetry, and the dedicated `fallDetected` event.

### Dashboard update

Polling was reduced from 700 ms to 1000 ms, and the dashboard now displays motion-sensor presence, motion state, and fall-event state.

### Position limitation

The IMU can report orientation and movement, but it cannot provide reliable absolute 3D position indefinitely through acceleration/gyro integration because drift accumulates. V1.4 therefore reports how the device is moving and oriented rather than claiming precise absolute position.

### Documentation update

The following active project references were brought into alignment with V1.4:

- `README.md`
- `firmware/README.md`
- `docs/FIRMWARE.md`
- `docs/HARDWARE.md`
- `docs/DEVELOPMENT_LOG.md`
- new `docs/V1_4_UPDATE.md`

The historical V1.1 document remains historical and is not rewritten to claim later features existed in V1.1.

## 2026-09-07 — V1.4 Engineering Hardening

A review of the active firmware and documentation identified several low-risk improvements that strengthen the prototype without changing its core sensor architecture.

### Credential separation

The Wi-Fi AP password was removed from tracked source. `firmware/Secrets.h` is now a local-only file ignored by Git, while `firmware/Secrets.h.example` provides the setup template. A compile-safe placeholder is used when the local file has not yet been created. This prevents a deployment credential from being published in the public repository.

### Live IMU fault detection

Previously, `mpuPresent` reflected only the boot-time `WHO_AM_I` probe. It now follows live register-read success. A failed read clears the status and invalidates motion telemetry; periodic rediscovery allows recovery after a breadboard connection is restored.

### Dashboard settings controls

The backend already supported persistent LEDs, buzzer, alert-mode, and unit settings, but the HTML exposed only numeric thresholds. The dashboard now renders controls for all existing settings and saves them through the same `/api/settings` interface.

### Distinct fall alert presentation

A validated fall now uses a distinct rapid 2500 Hz tone while other critical conditions retain the standard critical tone. Severity and alarm routing are unchanged.

### Automated build checking

A GitHub Actions workflow now compiles the ESP32 firmware on pushes and pull requests using the ESP32 Arduino core and required libraries. This adds an automated regression check for common source/build failures.

### Scope intentionally left unchanged

The review also identified runtime-tunable fall thresholds, a possible MPU-9250-family magnetometer, in-memory event history, documentation consolidation, and pulsed water-sensor power as useful future work. These were not implemented in this hardening pass because they either require new validation data, add a new subsystem, or represent V2-level architecture work.

Battery monitoring also remains disabled in tracked firmware. The existing divider/percentage calculation is preserved as prototype functionality while the battery power architecture and state-of-charge characterization are finalized.
