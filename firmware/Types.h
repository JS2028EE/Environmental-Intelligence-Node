#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------------------
// Every screen the device can show. SCREEN_COUNT must stay last — it sizes
// the per-screen cursor-memory table in Navigation.cpp.
// ---------------------------------------------------------------------------
enum ScreenState : uint8_t {
  HOME,
  MAIN_MENU, ENV_MENU, VITALS_MENU, INVESTIGATE_MENU, SYSTEM_MENU,
  TEMP_SCREEN, HUMIDITY_SCREEN, LIGHT_SCREEN, SOUND_SCREEN, CONDITIONS_SCREEN,
  HEART_SCREEN, SIGNAL_SCREEN, MEASUREMENT_SCREEN,
  IR_REFLECTION_SCREEN, OBJECT_SCREEN, MAGNETIC_SCREEN, WATER_SCREEN, FLAME_SCREEN,
  BATTERY_SCREEN, HARDWARE_SCREEN, SENSOR_STATUS_SCREEN, ABOUT_SCREEN, SETTINGS_SCREEN,
  SCREEN_COUNT
};

enum SystemStatus : uint8_t { STATUS_NORMAL, STATUS_WARNING, STATUS_CRITICAL };

inline const char* toString(SystemStatus s) {
  switch (s) {
    case STATUS_CRITICAL: return "CRITICAL";
    case STATUS_WARNING:  return "WARNING";
    default:              return "NORMAL";
  }
}

// One row in a menu: what it says, and what pressing SELECT on it does.
struct MenuItem {
  const char* label;
  ScreenState target;
};

// A menu screen: a header title plus its rows.
struct Menu {
  const char* title;
  const MenuItem* items;
  uint8_t count;
};
