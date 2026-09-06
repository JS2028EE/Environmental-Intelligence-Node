#pragma once
#include <Arduino.h>  // needed here for uint8_t — don't rely on other headers
                      // having pulled this in first (Alerts.cpp didn't)

// =============================================================================
// Hardware pin map — identical to V1.1. Changing any of these means rewiring.
// =============================================================================
#define PIN_BUZZER      4
#define PIN_GREEN_LED   13
#define PIN_RED_LED     14
#define PIN_UP          16
#define PIN_DOWN        17
#define PIN_SELECT      18
#define PIN_BACK        19
#define PIN_OLED_SDA    21
#define PIN_OLED_SCL    22
#define PIN_FLAME       23
#define PIN_DHT         25
#define PIN_TCRT        26
#define PIN_IR          27
#define PIN_HEART       32
#define PIN_LIGHT       33
#define PIN_SOUND       34
#define PIN_HALL        35
#define PIN_WATER       36
#define PIN_BATTERY     39

#define DHTTYPE DHT11

// =============================================================================
// OLED display
// =============================================================================
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define OLED_ADDRESS   0x3C

// =============================================================================
// Wi-Fi access point
// =============================================================================
#define AP_SSID       "VIGIL-01"
#define AP_PASSWORD   "VIGIL01_2026"
#define MDNS_HOSTNAME "vigil01"        // dashboard reachable at http://vigil01.local

// =============================================================================
// Sensor polarity — validated on the current prototype wiring. The flame
// module was physically tested with a real flame: CLEAR = LOW,
// FLAME/IR EVENT = HIGH. Don't flip FLAME_ACTIVE_LOW without re-testing.
// =============================================================================
constexpr bool TCRT_ACTIVE_LOW  = true;
constexpr bool IR_ACTIVE_LOW    = true;
constexpr bool FLAME_ACTIVE_LOW = false;

// Flame input must read "active" continuously for this long before the
// alarm latches, so a brief electrical glitch can't trigger a false alarm.
constexpr unsigned long FLAME_CONFIRM_MS = 100;

// =============================================================================
// Default alarm thresholds (raw ADC counts). These are just the starting
// values now — both are live-tunable from the web dashboard and persist
// in flash from then on (see Settings.h/.cpp).
// =============================================================================
constexpr int DEFAULT_SOUND_ALARM_THRESHOLD = 135;
constexpr int DEFAULT_WATER_ALARM_THRESHOLD = 2500;

// =============================================================================
// Timing
// =============================================================================
constexpr unsigned long DEBOUNCE_MS        = 40;
constexpr unsigned long SENSOR_POLL_MS     = 100;
constexpr unsigned long DHT_POLL_MS        = 2000;
constexpr unsigned long DISPLAY_REFRESH_MS = 100;

constexpr uint8_t SETTINGS_ITEM_COUNT = 4; // LEDs, buzzer, alert mode, units

// =============================================================================
// Battery monitoring — OFF by default, matching the current breadboard
// build (PIN_BATTERY isn't wired to anything yet). The read/percent code
// is already written in Sensors.cpp; flip this to 1 and set your real
// divider ratio once you actually add a battery + divider.
// =============================================================================
#define BATTERY_MONITORING_ENABLED 0
constexpr float BATTERY_DIVIDER_RATIO = 2.0f;  // (R1+R2)/R2 of your divider
constexpr float BATTERY_ADC_REF_V     = 3.3f;  // ESP32 ADC reference
constexpr float BATTERY_EMPTY_V       = 3.3f;  // volts treated as 0%
constexpr float BATTERY_FULL_V        = 4.2f;  // volts treated as 100%

// =============================================================================
// Watchdog — auto-recovers the board if a sensor or Wi-Fi call ever hangs.
// The reset API changed between ESP32 core versions; Watchdog.cpp handles
// both, but if your installed core still complains on compile, set this to 0.
// =============================================================================
#define WATCHDOG_ENABLED 1
