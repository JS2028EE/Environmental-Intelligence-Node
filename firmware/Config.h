#pragma once
#include <Arduino.h>

// V1.2 pins preserved. Navigation sensors use the existing I2C bus plus GPIO5
// as a receive-only GPS UART input.
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
#define PIN_GPS_RX      5
#define GPS_BAUD        9600
#define DHTTYPE DHT11

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define OLED_ADDRESS   0x3C
#define MPU6050_ADDRESS 0x68
#define MPU6050_ALT_ADDRESS 0x69
#define BME280_ADDRESS 0x76
#define BME280_ALT_ADDRESS 0x77

#define AP_SSID       "VIGIL-01"
#define AP_PASSWORD   "VIGIL01_2026"
#define MDNS_HOSTNAME "vigil01"

constexpr bool TCRT_ACTIVE_LOW  = true;
constexpr bool IR_ACTIVE_LOW    = true;
constexpr bool FLAME_ACTIVE_LOW = false;
constexpr unsigned long FLAME_CONFIRM_MS = 100;
constexpr int DEFAULT_SOUND_ALARM_THRESHOLD = 135;
constexpr int DEFAULT_WATER_ALARM_THRESHOLD = 2500;

constexpr float MOTION_ACCEL_THRESHOLD_MS2 = 1.5f;
constexpr float IMPACT_ACCEL_THRESHOLD_MS2 = 25.0f;
constexpr float TILT_THRESHOLD_DEG = 30.0f;
constexpr float SURFACE_DEADBAND_M = 2.0f;

constexpr unsigned long DEBOUNCE_MS        = 40;
constexpr unsigned long SENSOR_POLL_MS     = 100;
constexpr unsigned long DHT_POLL_MS        = 2000;
constexpr unsigned long GPS_POLL_MS        = 20;
constexpr unsigned long DISPLAY_REFRESH_MS = 100;
constexpr uint8_t SETTINGS_ITEM_COUNT = 4;

#define BATTERY_MONITORING_ENABLED 0
constexpr float BATTERY_DIVIDER_RATIO = 2.0f;
constexpr float BATTERY_ADC_REF_V     = 3.3f;
constexpr float BATTERY_EMPTY_V       = 3.3f;
constexpr float BATTERY_FULL_V        = 4.2f;

#define WATCHDOG_ENABLED 1
