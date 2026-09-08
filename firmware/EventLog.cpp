#include "EventLog.h"
#include "Sensors.h"
#include "Settings.h"

static EventRecord events[EVENT_BUFFER_SIZE];
static uint8_t eventCount=0;
static uint8_t eventWriteIndex=0;
static bool lastSound=false,lastWater=false,lastObject=false,lastFlame=false,lastTcrt=false,lastFall=false;

static void addEvent(const char* type,const char* severity){
  events[eventWriteIndex]={millis(),type,severity};
  eventWriteIndex=(eventWriteIndex+1)%EVENT_BUFFER_SIZE;
  if(eventCount<EVENT_BUFFER_SIZE)eventCount++;
}

void eventLogBegin(){
  eventCount=0;
  eventWriteIndex=0;
  lastSound=lastWater=lastObject=lastFlame=lastTcrt=lastFall=false;
}

void eventLogPoll(){
  bool sound=sensors.soundRaw>settings.soundThreshold;
  bool water=sensors.waterRaw>settings.waterThreshold;
  bool object=sensors.irDetected;
  bool flame=sensors.flameDetected;
  bool tcrt=sensors.tcrtDetected;
  bool fall=sensors.fallDetected;

  if(sound&&!lastSound)addEvent("SOUND","WARNING");
  if(water&&!lastWater)addEvent("WATER","WARNING");
  if(object&&!lastObject)addEvent("OBJECT","WARNING");
  if(tcrt&&!lastTcrt)addEvent("IR_REFLECTION","INFO");
  if(flame&&!lastFlame)addEvent("FLAME","CRITICAL");
  if(fall&&!lastFall)addEvent("FALL","CRITICAL");

  lastSound=sound;
  lastWater=water;
  lastObject=object;
  lastFlame=flame;
  lastTcrt=tcrt;
  lastFall=fall;
}

uint8_t eventLogCount(){return eventCount;}

const EventRecord& eventLogAt(uint8_t indexFromOldest){
  uint8_t oldest=(eventCount<EVENT_BUFFER_SIZE)?0:eventWriteIndex;
  uint8_t index=(oldest+indexFromOldest)%EVENT_BUFFER_SIZE;
  return events[index];
}
