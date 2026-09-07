#include "Navigation.h"
#include "Config.h"
#include "MenuData.h"
#include "Settings.h"

ScreenState currentScreen = HOME;
uint8_t cursor = 0;
static uint8_t menuCursorMemory[SCREEN_COUNT] = {0};
struct ButtonState { uint8_t pin; bool stable; bool lastReading; unsigned long lastChange; };
static ButtonState buttons[] = {
  { PIN_UP, HIGH, HIGH, 0 }, { PIN_DOWN, HIGH, HIGH, 0 },
  { PIN_SELECT, HIGH, HIGH, 0 }, { PIN_BACK, HIGH, HIGH, 0 },
};
static void beep(int freq, int ms);
static bool buttonPressed(uint8_t i);
static uint8_t itemCountFor(ScreenState s);
static ScreenState parentOf(ScreenState s);
static void enterScreen(ScreenState target);
static void toggleSetting(uint8_t index);
static void navUp(); static void navDown(); static void navSelect(); static void navBack();

void navigationBegin() {
  pinMode(PIN_UP, INPUT_PULLUP); pinMode(PIN_DOWN, INPUT_PULLUP);
  pinMode(PIN_SELECT, INPUT_PULLUP); pinMode(PIN_BACK, INPUT_PULLUP);
}
void navigationPoll() {
  if (buttonPressed(0)) navUp(); if (buttonPressed(1)) navDown();
  if (buttonPressed(2)) navSelect(); if (buttonPressed(3)) navBack();
}
static void beep(int freq, int ms) { if (settings.buzzerEnabled) tone(PIN_BUZZER, freq, ms); }
static bool buttonPressed(uint8_t i) {
  ButtonState &b = buttons[i]; bool reading = digitalRead(b.pin);
  if (reading != b.lastReading) { b.lastChange = millis(); b.lastReading = reading; }
  if (millis() - b.lastChange > DEBOUNCE_MS && reading != b.stable) {
    b.stable = reading; if (b.stable == LOW) return true;
  }
  return false;
}
static uint8_t itemCountFor(ScreenState s) {
  if (s == SETTINGS_SCREEN) return SETTINGS_ITEM_COUNT;
  const Menu* m = getMenu(s); return m ? m->count : 0;
}
static ScreenState parentOf(ScreenState s) {
  if (s == HOME || s == MAIN_MENU) return HOME;
  static const ScreenState menus[] = { MAIN_MENU, ENV_MENU, VITALS_MENU, INVESTIGATE_MENU, NAVIGATION_MENU, SYSTEM_MENU };
  for (ScreenState menuId : menus) {
    const Menu* m = getMenu(menuId);
    for (uint8_t i = 0; i < m->count; i++) if (m->items[i].target == s) return menuId;
  }
  return HOME;
}
static void enterScreen(ScreenState target) { currentScreen = target; cursor = menuCursorMemory[target]; }
static void toggleSetting(uint8_t index) {
  switch (index) {
    case 0: settings.ledsEnabled = !settings.ledsEnabled; settingsSaveFlag("leds", settings.ledsEnabled); break;
    case 1: settings.buzzerEnabled = !settings.buzzerEnabled; settingsSaveFlag("buzzer", settings.buzzerEnabled); if (!settings.buzzerEnabled) noTone(PIN_BUZZER); break;
    case 2: settings.automaticAlerts = !settings.automaticAlerts; settingsSaveFlag("alerts", settings.automaticAlerts); break;
    case 3: settings.useFahrenheit = !settings.useFahrenheit; settingsSaveFlag("fahren", settings.useFahrenheit); break;
  }
}
static void navUp() { beep(950,25); uint8_t count=itemCountFor(currentScreen); if(!count)return; cursor=(cursor+count-1)%count; menuCursorMemory[currentScreen]=cursor; }
static void navDown() { beep(1050,25); uint8_t count=itemCountFor(currentScreen); if(!count)return; cursor=(cursor+1)%count; menuCursorMemory[currentScreen]=cursor; }
static void navSelect() {
  beep(1200,40); if(currentScreen==HOME){enterScreen(MAIN_MENU);return;}
  if(currentScreen==SETTINGS_SCREEN){toggleSetting(cursor);return;}
  const Menu* m=getMenu(currentScreen); if(m && cursor<m->count) enterScreen(m->items[cursor].target);
}
static void navBack() { beep(800,30); currentScreen=parentOf(currentScreen); cursor=menuCursorMemory[currentScreen]; }
