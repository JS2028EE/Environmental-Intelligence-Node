#pragma once
#include <Arduino.h>

struct EventRecord {
  unsigned long uptimeMs;
  const char* type;
  const char* severity;
};

constexpr uint8_t EVENT_BUFFER_SIZE = 16;

void eventLogBegin();
void eventLogPoll();
uint8_t eventLogCount();
const EventRecord& eventLogAt(uint8_t indexFromOldest);
