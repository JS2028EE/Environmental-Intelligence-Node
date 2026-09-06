# VIGIL-01 — V1.2 Firmware

A refactor of your V1.1 sketch. Same pins, same sensor tuning, same button
feel — reorganized into modules, with two real bugs fixed, settings that
survive a reboot, and a nicer web dashboard.

**I don't have your board, so I couldn't flash or bench-test this myself.**
Everything below is reasoned through carefully and the logic mirrors your
original line-for-line where nothing needed to change, but please build it
once on the bench — especially the flame path — before relying on it.

---

## Recent Update — September 6, 2026

**Documentation update only. No firmware source files were changed as part of this update.**

VIGIL-01 V1.2 has now been documented as the current firmware revision in the engineering record. This update records the architectural refactor, reliability improvements, bug fixes, and newly added configuration features already present in the firmware.

### Engineering changes recorded

- Refactored the previous single-file V1.1 firmware into separate modules for configuration, settings, sensors, navigation, alerts, display rendering, web services, and watchdog recovery.
- Replaced duplicated menu logic with a data-driven menu structure, allowing scrolling, cursor movement, selection, and Back navigation to stay synchronized.
- Fixed the invisible `FLAME` menu item in Investigate and the invisible/mis-mapped `ABOUT` / `SETTINGS` items in System.
- Corrected flame detection so the `FLAME_ACTIVE_LOW` configuration constant is actually respected by the sensor logic.
- Added persistent settings using ESP32 NVS/flash so user preferences survive reboot.
- Added Celsius/Fahrenheit selection.
- Added live web-dashboard editing for sound and water alarm thresholds, with persistence.
- Redesigned the web dashboard with sensor status cards, alarm highlighting, status information, and threshold controls.
- Added captive-portal behavior and mDNS access through `vigil01.local`.
- Replaced manual JSON construction with ArduinoJson.
- Changed the frequently updated `heartSignal` value to a string literal to reduce unnecessary heap churn.
- Added an optional watchdog recovery system, enabled by default.
- Added battery-monitoring support behind a configuration flag; it remains disabled until the required voltage-divider hardware is connected.

### Compatibility preserved

The V1.2 refactor intentionally preserves the V1.1 hardware interface and core behavior:

- GPIO assignments remain unchanged.
- Sensor polarity and tuning remain unchanged.
- Default sound/water thresholds remain `135 / 2500`.
- Heart-rate processing and BPM gating remain unchanged.
- LED and buzzer timing/tones remain unchanged.
- Button debounce remains 40 ms.
- Navigation sound effects remain unchanged.
- Menu hierarchy remains unchanged.
- Wi-Fi AP credentials remain `VIGIL-01` / `VIGIL01_2026`.
- Analog sensor inputs remain on ADC1 pins 32–39.

### Validation status

This revision has been reviewed at the firmware/code level but has **not yet been bench-tested on the physical VIGIL-01 hardware**. The flame-detection path and watchdog behavior should receive particular attention during the next hardware validation session.

### Required library change

V1.2 adds **ArduinoJson** as a library dependency. The firmware was written against the ArduinoJson 6.x API. If Arduino Library Manager installs v7 and compilation issues appear, use a compatible 6.21.x release or port the JSON calls to the newer API.

---

## What stayed exactly the same

- **Every pin.** Buttons, LEDs, buzzer, OLED, and all 12 sensors are on the
  same GPIOs as V1.1 (see `Config.h`).
- **AP SSID/password** (`VIGIL-01` / `VIGIL01_2026`).
- **Sensor polarity and tuning**: TCRT/IR active-low, flame active-HIGH with
  the 100 ms confirm window, default sound/water alarm thresholds (135 /
  2500), heart-rate EMA weighting and BPM gating, LED/buzzer flash timings
  and tones, 40 ms button debounce, and every nav sound effect.
- **The menu hierarchy** — Home → Main → {Environment, Vitals, Investigate,
  System} → each screen. Nothing was renamed, reordered, or removed.
- **Your analog pin choices** (32–39) are all ADC1, so they don't fight with
  Wi-Fi the way ADC2 pins would — that was already right, so I left it alone.

## Bugs fixed

1. **Two menu items were invisible.** `INVESTIGATE_MENU` had 5 logical
   items but the old `drawInvestigateMenu()` only drew 4 rows — `FLAME` was
   selectable (the cursor could land on it) but never appeared on screen.
   `SYSTEM_MENU` had the same problem: `ABOUT` was invisible and `SETTINGS`
   was drawn against the wrong cursor index. The new menu renderer scrolls,
   so every item in every menu is now actually visible.
2. **`FLAME_ACTIVE_LOW` was declared but silently ignored.** The comment
   said the flame module was active-HIGH and physically validated, but the
   detection code hardcoded `flameRaw==HIGH` instead of reading the
   constant. It's wired in for real now in `Sensors.cpp`. Behavior today is
   unchanged (the constant is still `false`), but if you ever swap flame
   modules, flipping one line now actually does something.
3. Removed `isAlertScreen()` — defined in V1.1 but never called anywhere.

## Architecture changes

The old ~250-line single file is now a proper sketch:

| File | What it owns |
|---|---|
| `VIGIL01.ino` | `setup()`/`loop()` — just wires the modules together |
| `Types.h` | Shared enums and structs |
| `Config.h` | Pins, network creds, thresholds, timing, feature flags |
| `Settings.h/.cpp` | Persisted user settings (NVS/flash) |
| `Sensors.h/.cpp` | All sensor reads, heart-rate processing, status evaluation |
| `MenuData.h` | **The entire menu tree, as one data table** |
| `Navigation.h/.cpp` | Buttons, debounce, cursor, Back/Select logic |
| `Alerts.h/.cpp` | LED/buzzer alarm driver |
| `DisplayUI.h/.cpp` | All OLED rendering |
| `WebDashboard.h/.cpp` | AP, captive portal, mDNS, HTTP + JSON API |
| `Watchdog.h/.cpp` | Optional auto-recovery timer |

The biggest structural change is `MenuData.h`: instead of five nearly
identical `drawXMenu()` / cursor-wrapping / selection blocks (one per menu),
there's a single table of menus and items. `Navigation.cpp` and
`DisplayUI.cpp` both walk that table generically — Back navigation is
*derived* from it (searching for which menu contains the current screen)
rather than hand-maintained as a parallel switch statement that could drift
out of sync. Add a menu item in one place and wrapping, scrolling, and Back
all just work.

## What's new

- **Settings persist across power cycles** (LEDs on/off, buzzer on/off,
  page vs. global alerts) — V1.1 reset these to defaults every boot.
- **Celsius/Fahrenheit toggle**, added as a 4th Settings row.
- **Sound and water alarm thresholds are now live-tunable** from the web
  dashboard (no reflashing) and persist once changed. The compiled-in
  values (135 / 2500) are just the starting defaults.
- **Redesigned web dashboard** — dark UI, a status pill, per-sensor cards
  that highlight red when active, and the threshold editor. Served straight
  from flash via `PROGMEM`/`send_P` instead of building a `String` on every
  request.
- **Captive portal + mDNS**: joining the AP now pops the dashboard
  automatically on most phones, and it's reachable at `http://vigil01.local/`
  without needing to know the AP's IP.
- **JSON via ArduinoJson** instead of manual string concatenation — safer
  (correct escaping) and far easier to extend with new fields.
- **`heartSignal` is now a string literal, not an Arduino `String`.** Small
  change, but `String` reassignment on ESP32 can fragment the heap over a
  long-running session; this field updates 100x/second, so it's worth
  avoiding entirely.
- **Optional watchdog** (`WATCHDOG_ENABLED` in `Config.h`, on by default) —
  if any loop iteration ever hangs for >8s, the board resets itself instead
  of sitting frozen with a stale display and no alerts.
- **Battery monitoring code is included but off by default**
  (`BATTERY_MONITORING_ENABLED` in `Config.h`), matching your current
  "NOT CONNECTED" hardware. Wire a voltage divider, set
  `BATTERY_DIVIDER_RATIO` to match it, flip the flag to `1`, and the
  Battery screen starts reporting a real percentage.

## Requirements

Same libraries as before, plus one addition:

- Adafruit GFX Library
- Adafruit SSD1306
- DHT sensor library (Adafruit)
- **ArduinoJson** (new — install via Library Manager)

`WiFi`, `WebServer`, `DNSServer`, `ESPmDNS`, `Preferences`, and
`esp_task_wdt` all ship with the ESP32 board package, nothing extra needed
for those.

This was written against **ArduinoJson v6.x** (`StaticJsonDocument`,
`containsKey`). If Library Manager installs v7 by default and you hit
deprecation warnings or errors, either pin to a 6.21.x release or let me
know and I'll port the API calls — the JSON structure itself doesn't change.

## Before you trust it on the bench

- `Watchdog.cpp` branches on your installed ESP32 core version
  (`esp_task_wdt_init` changed signature between core 2.x and 3.x). If it
  still fails to compile against your specific core version, set
  `WATCHDOG_ENABLED` to `0` in `Config.h` and it's fully inert.
- I'd re-verify the flame path specifically, given how deliberately you
  validated it originally — the logic is unchanged, but it's worth
  double-checking after any refactor.
- Everything else should behave identically to V1.1 unless you touch a new
  setting (Fahrenheit toggle, or edited thresholds via the dashboard).
