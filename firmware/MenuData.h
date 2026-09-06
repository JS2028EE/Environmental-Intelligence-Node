#pragma once
#include "Types.h"

// Central definition of every menu screen: what it's called, and what each
// row leads to. Add, remove, or reorder an item here and wrapping,
// scrolling, selection, and Back navigation all just work — nothing else
// in the firmware needs to change.
//
// This also fixes two display bugs from V1.1: INVESTIGATE had 5 logical
// items but only 4 menu rows were ever drawn, so "FLAME" was selectable
// but invisible. SYSTEM had the same problem with "ABOUT". The generic,
// scrollable renderer in DisplayUI.cpp shows every item now.
inline const Menu* getMenu(ScreenState screen) {
  static const MenuItem mainItems[] = {
    { "ENVIRONMENT", ENV_MENU },
    { "VITALS",      VITALS_MENU },
    { "INVESTIGATE", INVESTIGATE_MENU },
    { "SYSTEM",      SYSTEM_MENU },
  };
  static const MenuItem envItems[] = {
    { "TEMPERATURE", TEMP_SCREEN },
    { "HUMIDITY",    HUMIDITY_SCREEN },
    { "LIGHT",       LIGHT_SCREEN },
    { "SOUND",       SOUND_SCREEN },
    { "CONDITIONS",  CONDITIONS_SCREEN },
  };
  static const MenuItem vitalsItems[] = {
    { "HEART",       HEART_SCREEN },
    { "SIGNAL",      SIGNAL_SCREEN },
    { "MEASUREMENT", MEASUREMENT_SCREEN },
  };
  static const MenuItem investigateItems[] = {
    { "IR REFLECTION", IR_REFLECTION_SCREEN },
    { "OBJECT",        OBJECT_SCREEN },
    { "MAGNETIC",      MAGNETIC_SCREEN },
    { "WATER",         WATER_SCREEN },
    { "FLAME",         FLAME_SCREEN },
  };
  static const MenuItem systemItems[] = {
    { "BATTERY",       BATTERY_SCREEN },
    { "HARDWARE",      HARDWARE_SCREEN },
    { "SENSOR STATUS", SENSOR_STATUS_SCREEN },
    { "ABOUT",         ABOUT_SCREEN },
    { "SETTINGS",      SETTINGS_SCREEN },
  };

  static const Menu MAIN = { "MENU",        mainItems,        4 };
  static const Menu ENV  = { "ENVIRONMENT", envItems,         5 };
  static const Menu VIT  = { "VITALS",      vitalsItems,      3 };
  static const Menu INV  = { "INVESTIGATE", investigateItems, 5 };
  static const Menu SYS  = { "SYSTEM",      systemItems,      5 };

  switch (screen) {
    case MAIN_MENU:        return &MAIN;
    case ENV_MENU:         return &ENV;
    case VITALS_MENU:      return &VIT;
    case INVESTIGATE_MENU: return &INV;
    case SYSTEM_MENU:      return &SYS;
    default:               return nullptr; // not a menu screen (leaf, or HOME)
  }
}
