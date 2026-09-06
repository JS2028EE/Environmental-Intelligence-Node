#include "Watchdog.h"
#include <esp_task_wdt.h>
#include <esp_idf_version.h>

static const uint32_t WDT_TIMEOUT_S = 8;

void watchdogBegin() {
  // The TWDT init signature is an ESP-IDF API, so check the IDF version
  // that's actually bundled with the installed core, not the Arduino core
  // version — that's what really changed the function signature.
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  // IDF 5.x (Arduino-ESP32 core 3.x) — config-struct API.
  esp_task_wdt_config_t cfg = {};
  cfg.timeout_ms = WDT_TIMEOUT_S * 1000;
  cfg.idle_core_mask = 0;
  cfg.trigger_panic = true;
  esp_task_wdt_init(&cfg);
  esp_task_wdt_add(NULL);
#else
  // IDF 4.x (Arduino-ESP32 core 2.x) — (timeout_seconds, panic) API.
  esp_task_wdt_init(WDT_TIMEOUT_S, true);
  esp_task_wdt_add(NULL);
#endif
}

void watchdogFeed() {
  esp_task_wdt_reset();
}
