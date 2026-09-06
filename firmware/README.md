# VIGIL-01 — V1.2 Firmware

A refactor of your V1.1 sketch. Same pins, same sensor tuning, same button
feel — reorganized into modules, with two real bugs fixed, settings that
survive a reboot, and a nicer web dashboard.

**I don't have your board, so I couldn't flash or bench-test this myself.**
Everything below is reasoned through carefully and the logic mirrors your
original line-for-line where nothing needed to change, but please build it
once on the bench — especially the flame path — before relying on it.

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
