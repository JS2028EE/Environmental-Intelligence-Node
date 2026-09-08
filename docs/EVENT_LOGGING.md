# VIGIL-01 Event Logging — V1.5

## Purpose

V1.5 adds a small volatile event history so recent sensor/alarm transitions can be inspected during live testing without introducing SD storage, cloud storage, or a persistent event database.

## Architecture

```text
Sensor readings
      ↓
EventLog.cpp
      ↓
16-entry RAM ring buffer
      ↓
GET /events
      ↓
Local dashboard
```

The buffer stores the most recent 16 event transitions. When it becomes full, the oldest record is overwritten.

## Recorded events

Only rising transitions are recorded, which prevents a continuously active condition from filling the buffer on every 100 ms sensor cycle.

| Event | Severity | Trigger |
|---|---|---|
| `SOUND` | WARNING | Sound value crosses above the configured threshold |
| `WATER` | WARNING | Water value crosses above the configured threshold |
| `OBJECT` | WARNING | IR obstacle detection becomes active |
| `IR_REFLECTION` | INFO | TCRT5000 reflection detection becomes active |
| `FLAME` | CRITICAL | Flame/IR condition becomes confirmed |
| `FALL` | CRITICAL | Validated multi-stage fall becomes active |

## Endpoint

```text
GET /events
```

Example response shape:

```json
{
  "events": [
    {"uptimeMs": 12450, "type": "SOUND", "severity": "WARNING"},
    {"uptimeMs": 18720, "type": "FALL", "severity": "CRITICAL"}
  ]
}
```

`uptimeMs` is milliseconds since the ESP32 booted. It is not a wall-clock timestamp because V1.5 does not require an RTC or internet time service.

## Persistence and limitations

The event history is **RAM-only**. A reboot clears the buffer. Events are not written to NVS, flash, SD, cloud storage, or an external database.

This is intentional: VIGIL-01 remains a live-sensing prototype rather than a long-term data logger. A future persistent event system should be designed separately so flash wear, storage format, timestamps, and privacy are handled deliberately.

## Relationship to alarm behavior

The event log does not create new alarm conditions. It observes conditions already produced by the existing sensor/alarm architecture.

In particular:

- Motion, impact, and ordinary tilt telemetry do not create events by themselves.
- TCRT5000 reflection is logged as an investigation event but remains excluded from GLOBAL physical alarms.
- A validated fall remains a system-level critical event.

## Testing

1. Trigger a sound threshold crossing and request `/events`.
2. Trigger a water threshold crossing and request `/events`.
3. Trigger object detection and request `/events`.
4. Trigger TCRT5000 reflection and confirm `IR_REFLECTION` appears.
5. Trigger a confirmed flame condition and confirm `FLAME` appears.
6. Produce a validated fall sequence only in a safe controlled test and confirm `FALL` appears.
7. Keep a sensor condition active and confirm it is not logged repeatedly every polling cycle.
8. Generate more than 16 transitions and confirm the oldest records roll off.
9. Reboot the device and confirm the RAM history is cleared.
