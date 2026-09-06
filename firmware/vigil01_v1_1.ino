#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// VIGIL-01 V1.1
// Portable Environmental Intelligence Node
// Live-only breadboard firmware; USB powered during development.
// UI layout preserved from V1.0. Adds configurable alert behavior,
// navigation feedback, and sound-event thresholding.
// V1.1.2: corrected flame/IR sensor polarity from physical validation.

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WebServer server(80);
const char* AP_SSID="VIGIL-01";
const char* AP_PASSWORD="VIGIL01_2026";

#define PIN_BUZZER 4
#define PIN_GREEN_LED 13
#define PIN_RED_LED 14
#define PIN_UP 16
#define PIN_DOWN 17
#define PIN_SELECT 18
#define PIN_BACK 19
#define PIN_OLED_SDA 21
#define PIN_OLED_SCL 22
#define PIN_FLAME 23
#define PIN_DHT 25
#define PIN_TCRT 26
#define PIN_IR 27
#define PIN_HEART 32
#define PIN_LIGHT 33
#define PIN_SOUND 34
#define PIN_HALL 35
#define PIN_WATER 36
#define PIN_BATTERY 39
#define DHTTYPE DHT11
DHT dht(PIN_DHT,DHTTYPE);

enum ScreenState{HOME,MAIN_MENU,ENV_MENU,VITALS_MENU,INVESTIGATE_MENU,SYSTEM_MENU,TEMP_SCREEN,HUMIDITY_SCREEN,LIGHT_SCREEN,SOUND_SCREEN,CONDITIONS_SCREEN,HEART_SCREEN,SIGNAL_SCREEN,MEASUREMENT_SCREEN,IR_REFLECTION_SCREEN,OBJECT_SCREEN,MAGNETIC_SCREEN,WATER_SCREEN,FLAME_SCREEN,BATTERY_SCREEN,HARDWARE_SCREEN,SENSOR_STATUS_SCREEN,ABOUT_SCREEN,SETTINGS_SCREEN};
ScreenState currentScreen=HOME;
int mainCursor=0,envCursor=0,vitalsCursor=0,investigateCursor=0,systemCursor=0,settingsCursor=0;

float temperatureC=NAN,humidity=NAN;
int lightRaw=0,soundRaw=0,heartRaw=0,hallRaw=0,waterRaw=0;
bool tcrtDetected=false,irDetected=false,flameDetected=false;
const int SOUND_ALARM_THRESHOLD=135;
const int WATER_ALARM_THRESHOLD=2500;

// Digital detection polarity is defined independently for each module.
// The flame module was physically validated during breadboard testing:
// CLEAR = HIGH and FLAME/IR DETECTED = LOW.
// Therefore a LOW reading is the actual flame event.
const bool TCRT_ACTIVE_LOW=true;
const bool IR_ACTIVE_LOW=true;
const bool FLAME_ACTIVE_LOW=true;

int heartBaseline=2048,heartRate=0;
bool heartAboveThreshold=false;
unsigned long lastBeatTime=0;
String heartSignal="WAIT";

enum SystemStatus{STATUS_NORMAL,STATUS_NOTICE,STATUS_WARNING,STATUS_CRITICAL};
SystemStatus systemStatus=STATUS_NORMAL;

// User-configurable output/alert settings. Defaults: outputs ON, page-scoped alerts.
bool ledsEnabled=true;
bool buzzerEnabled=true;
bool automaticAlerts=false; // false=PAGE, true=GLOBAL

struct Button{uint8_t pin;bool stableState;bool lastReading;unsigned long lastChange;};
Button buttons[]={{PIN_UP,HIGH,HIGH,0},{PIN_DOWN,HIGH,HIGH,0},{PIN_SELECT,HIGH,HIGH,0},{PIN_BACK,HIGH,HIGH,0}};
const unsigned long DEBOUNCE_MS=40;
unsigned long lastSensorRead=0,lastDHTRead=0,lastDisplayUpdate=0;

void beep(int frequency,int duration){if(buzzerEnabled)tone(PIN_BUZZER,frequency,duration);}

bool isAlertScreen(){
  switch(currentScreen){
    case TEMP_SCREEN:case HUMIDITY_SCREEN:case LIGHT_SCREEN:case SOUND_SCREEN:case CONDITIONS_SCREEN:
    case HEART_SCREEN:case SIGNAL_SCREEN:case MEASUREMENT_SCREEN:case IR_REFLECTION_SCREEN:
    case OBJECT_SCREEN:case MAGNETIC_SCREEN:case WATER_SCREEN:case FLAME_SCREEN:return true;
    default:return false;
  }
}

bool pageHasActiveAlert(){
  switch(currentScreen){
    case SOUND_SCREEN:return soundRaw>SOUND_ALARM_THRESHOLD;
    case WATER_SCREEN:return waterRaw>WATER_ALARM_THRESHOLD;
    case OBJECT_SCREEN:return irDetected;
    case FLAME_SCREEN:return flameDetected;
    case CONDITIONS_SCREEN:return flameDetected||soundRaw>SOUND_ALARM_THRESHOLD||waterRaw>WATER_ALARM_THRESHOLD||irDetected;
    default:return false;
  }
}

bool alertsAllowedHere(){return automaticAlerts?true:isAlertScreen();}
bool alertActiveHere(){return automaticAlerts?(systemStatus==STATUS_WARNING||systemStatus==STATUS_CRITICAL):pageHasActiveAlert();}

void setup(){
  Serial.begin(115200);
  pinMode(PIN_UP,INPUT_PULLUP);pinMode(PIN_DOWN,INPUT_PULLUP);pinMode(PIN_SELECT,INPUT_PULLUP);pinMode(PIN_BACK,INPUT_PULLUP);
  pinMode(PIN_GREEN_LED,OUTPUT);pinMode(PIN_RED_LED,OUTPUT);pinMode(PIN_BUZZER,OUTPUT);
  pinMode(PIN_TCRT,INPUT);pinMode(PIN_IR,INPUT);pinMode(PIN_FLAME,INPUT);
  digitalWrite(PIN_GREEN_LED,LOW);digitalWrite(PIN_RED_LED,LOW);noTone(PIN_BUZZER);
  Wire.begin(PIN_OLED_SDA,PIN_OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC,OLED_ADDRESS)){Serial.println("OLED FAILED");while(true)delay(1000);}
  display.setTextColor(SSD1306_WHITE);dht.begin();showBootScreen();
  WiFi.mode(WIFI_AP);WiFi.softAP(AP_SSID,AP_PASSWORD);
  server.on("/",handleRoot);server.on("/data",handleData);server.begin();
  Serial.println("VIGIL-01 SYSTEM READY");Serial.print("AP IP: ");Serial.println(WiFi.softAPIP());
}

void loop(){
  unsigned long now=millis();server.handleClient();handleButtons();
  if(now-lastSensorRead>=100){lastSensorRead=now;readFastSensors();processHeartRate();evaluateSystemStatus();}
  if(now-lastDHTRead>=2000){lastDHTRead=now;temperatureC=dht.readTemperature();humidity=dht.readHumidity();}
  if(now-lastDisplayUpdate>=100){lastDisplayUpdate=now;drawCurrentScreen();}
  updateStatusOutputs();
}

void showBootScreen(){
  display.clearDisplay();display.setTextSize(2);display.setCursor(34,4);display.println("VIGIL");display.setTextSize(1);
  display.setCursor(18,27);display.println("ENVIRONMENTAL");display.setCursor(21,39);display.println("INTELLIGENCE");display.setCursor(43,51);display.println("NODE");display.display();delay(1200);
  display.clearDisplay();display.setTextSize(1);display.setCursor(37,5);display.println("VIGIL-01");display.setCursor(28,20);display.println("SYSTEM READY");display.setTextSize(2);display.setCursor(51,36);display.println("OK");display.display();beep(1500,100);delay(800);
}

void readFastSensors(){
  heartRaw=analogRead(PIN_HEART);lightRaw=analogRead(PIN_LIGHT);soundRaw=analogRead(PIN_SOUND);hallRaw=analogRead(PIN_HALL);waterRaw=analogRead(PIN_WATER);
  int tcrtRaw=digitalRead(PIN_TCRT);int irRaw=digitalRead(PIN_IR);int flameRaw=digitalRead(PIN_FLAME);
  tcrtDetected=TCRT_ACTIVE_LOW?(tcrtRaw==LOW):(tcrtRaw==HIGH);
  irDetected=IR_ACTIVE_LOW?(irRaw==LOW):(irRaw==HIGH);
  // Physical validation confirmed this flame module's active state is LOW.
  flameDetected=FLAME_ACTIVE_LOW?(flameRaw==LOW):(flameRaw==HIGH);
}

void processHeartRate(){
  static unsigned long lastSample=0;if(millis()-lastSample<10)return;lastSample=millis();
  heartBaseline=(heartBaseline*99+heartRaw)/100;int deviation=abs(heartRaw-heartBaseline);
  if(deviation<20)heartSignal="WEAK";else if(deviation<80)heartSignal="FAIR";else heartSignal="GOOD";
  int threshold=heartBaseline+100;
  if(!heartAboveThreshold&&heartRaw>threshold){
    heartAboveThreshold=true;unsigned long now=millis();
    if(lastBeatTime>0){unsigned long interval=now-lastBeatTime;if(interval>300&&interval<2000){int bpm=60000/interval;if(bpm>=40&&bpm<=200){if(heartRate==0)heartRate=bpm;else heartRate=(heartRate*3+bpm)/4;}}}
    lastBeatTime=now;
  }
  if(heartAboveThreshold&&heartRaw<heartBaseline+50)heartAboveThreshold=false;
}

void evaluateSystemStatus(){
  bool warning=false,critical=false;
  if(waterRaw>WATER_ALARM_THRESHOLD)warning=true;
  if(irDetected)warning=true;
  if(soundRaw>SOUND_ALARM_THRESHOLD)warning=true;
  if(flameDetected)critical=true;
  if(critical)systemStatus=STATUS_CRITICAL;else if(warning)systemStatus=STATUS_WARNING;else systemStatus=STATUS_NORMAL;
}

void updateStatusOutputs(){
  static unsigned long lastFlash=0;static bool flashState=false;unsigned long now=millis();
  if(!ledsEnabled){digitalWrite(PIN_GREEN_LED,LOW);digitalWrite(PIN_RED_LED,LOW);noTone(PIN_BUZZER);return;}
  bool activeAlert=alertActiveHere();
  if(!activeAlert){digitalWrite(PIN_GREEN_LED,HIGH);digitalWrite(PIN_RED_LED,LOW);noTone(PIN_BUZZER);return;}
  digitalWrite(PIN_GREEN_LED,LOW);
  if(systemStatus==STATUS_CRITICAL){
    if(now-lastFlash>100){lastFlash=now;flashState=!flashState;if(flashState&&buzzerEnabled)tone(PIN_BUZZER,2200,90);}
    digitalWrite(PIN_RED_LED,flashState);
  }else{
    if(now-lastFlash>150){lastFlash=now;flashState=!flashState;if(flashState&&buzzerEnabled)tone(PIN_BUZZER,1800,80);}
    digitalWrite(PIN_RED_LED,flashState);
  }
}

bool buttonPressed(int index){
  Button &b=buttons[index];bool reading=digitalRead(b.pin);
  if(reading!=b.lastReading){b.lastChange=millis();b.lastReading=reading;}
  if(millis()-b.lastChange>DEBOUNCE_MS&&reading!=b.stableState){b.stableState=reading;if(b.stableState==LOW)return true;}
  return false;
}
void handleButtons(){if(buttonPressed(0))navigateUp();if(buttonPressed(1))navigateDown();if(buttonPressed(2))selectCurrent();if(buttonPressed(3))goBack();}

void navigateUp(){
  beep(950,25);
  if(currentScreen==MAIN_MENU){if(--mainCursor<0)mainCursor=3;}
  else if(currentScreen==ENV_MENU){if(--envCursor<0)envCursor=4;}
  else if(currentScreen==VITALS_MENU){if(--vitalsCursor<0)vitalsCursor=2;}
  else if(currentScreen==INVESTIGATE_MENU){if(--investigateCursor<0)investigateCursor=4;}
  else if(currentScreen==SYSTEM_MENU){if(--systemCursor<0)systemCursor=4;}
  else if(currentScreen==SETTINGS_SCREEN){if(--settingsCursor<0)settingsCursor=2;}
}
void navigateDown(){
  beep(1050,25);
  if(currentScreen==MAIN_MENU){if(++mainCursor>3)mainCursor=0;}
  else if(currentScreen==ENV_MENU){if(++envCursor>4)envCursor=0;}
  else if(currentScreen==VITALS_MENU){if(++vitalsCursor>2)vitalsCursor=0;}
  else if(currentScreen==INVESTIGATE_MENU){if(++investigateCursor>4)investigateCursor=0;}
  else if(currentScreen==SYSTEM_MENU){if(++systemCursor>4)systemCursor=0;}
  else if(currentScreen==SETTINGS_SCREEN){if(++settingsCursor>2)settingsCursor=0;}
}

void selectCurrent(){
  beep(1200,40);
  if(currentScreen==HOME)currentScreen=MAIN_MENU;
  else if(currentScreen==MAIN_MENU){const ScreenState s[]={ENV_MENU,VITALS_MENU,INVESTIGATE_MENU,SYSTEM_MENU};currentScreen=s[mainCursor];}
  else if(currentScreen==ENV_MENU){const ScreenState s[]={TEMP_SCREEN,HUMIDITY_SCREEN,LIGHT_SCREEN,SOUND_SCREEN,CONDITIONS_SCREEN};currentScreen=s[envCursor];}
  else if(currentScreen==VITALS_MENU){const ScreenState s[]={HEART_SCREEN,SIGNAL_SCREEN,MEASUREMENT_SCREEN};currentScreen=s[vitalsCursor];}
  else if(currentScreen==INVESTIGATE_MENU){const ScreenState s[]={IR_REFLECTION_SCREEN,OBJECT_SCREEN,MAGNETIC_SCREEN,WATER_SCREEN,FLAME_SCREEN};currentScreen=s[investigateCursor];}
  else if(currentScreen==SYSTEM_MENU){const ScreenState s[]={BATTERY_SCREEN,HARDWARE_SCREEN,SENSOR_STATUS_SCREEN,ABOUT_SCREEN,SETTINGS_SCREEN};currentScreen=s[systemCursor];}
  else if(currentScreen==SETTINGS_SCREEN){
    if(settingsCursor==0)ledsEnabled=!ledsEnabled;
    else if(settingsCursor==1)buzzerEnabled=!buzzerEnabled;
    else automaticAlerts=!automaticAlerts;
    if(!buzzerEnabled)noTone(PIN_BUZZER);
  }
}

void goBack(){
  beep(800,30);
  switch(currentScreen){
    case HOME:break;case MAIN_MENU:currentScreen=HOME;break;
    case ENV_MENU:case VITALS_MENU:case INVESTIGATE_MENU:case SYSTEM_MENU:currentScreen=MAIN_MENU;break;
    case TEMP_SCREEN:case HUMIDITY_SCREEN:case LIGHT_SCREEN:case SOUND_SCREEN:case CONDITIONS_SCREEN:currentScreen=ENV_MENU;break;
    case HEART_SCREEN:case SIGNAL_SCREEN:case MEASUREMENT_SCREEN:currentScreen=VITALS_MENU;break;
    case IR_REFLECTION_SCREEN:case OBJECT_SCREEN:case MAGNETIC_SCREEN:case WATER_SCREEN:case FLAME_SCREEN:currentScreen=INVESTIGATE_MENU;break;
    case BATTERY_SCREEN:case HARDWARE_SCREEN:case SENSOR_STATUS_SCREEN:case ABOUT_SCREEN:case SETTINGS_SCREEN:currentScreen=SYSTEM_MENU;break;
    default:currentScreen=HOME;break;
  }
}

void header(const char* title){display.clearDisplay();display.setTextSize(1);display.setCursor(0,0);display.println(title);display.drawLine(0,10,127,10,SSD1306_WHITE);}
void menuItem(int y,const char* text,bool selected){display.setCursor(0,y);display.print(selected?"> ":"  ");display.println(text);}

void drawCurrentScreen(){
  switch(currentScreen){
    case HOME:drawHome();break;case MAIN_MENU:drawMainMenu();break;case ENV_MENU:drawEnvironmentMenu();break;case VITALS_MENU:drawVitalsMenu();break;case INVESTIGATE_MENU:drawInvestigateMenu();break;case SYSTEM_MENU:drawSystemMenu();break;
    case TEMP_SCREEN:drawValueScreen("TEMPERATURE",isnan(temperatureC)?"--":String(temperatureC,1)+" C");break;
    case HUMIDITY_SCREEN:drawValueScreen("HUMIDITY",isnan(humidity)?"--":String(humidity,0)+" %");break;
    case LIGHT_SCREEN:drawValueScreen("LIGHT",String(lightRaw));break;case SOUND_SCREEN:drawValueScreen("SOUND",String(soundRaw));break;case CONDITIONS_SCREEN:drawConditions();break;
    case HEART_SCREEN:drawHeart();break;case SIGNAL_SCREEN:drawValueScreen("HEART SIGNAL",heartSignal);break;case MEASUREMENT_SCREEN:drawValueScreen("HEART RATE",String(heartRate)+" BPM");break;
    case IR_REFLECTION_SCREEN:drawValueScreen("IR REFLECTION",tcrtDetected?"DETECTED":"CLEAR");break;case OBJECT_SCREEN:drawValueScreen("OBJECT",irDetected?"DETECTED":"CLEAR");break;case MAGNETIC_SCREEN:drawMagnetic();break;case WATER_SCREEN:drawValueScreen("WATER",String(waterRaw));break;case FLAME_SCREEN:drawValueScreen("FLAME / IR",flameDetected?"DETECTED":"CLEAR");break;
    case BATTERY_SCREEN:drawValueScreen("BATTERY","NOT CONNECTED");break;case HARDWARE_SCREEN:drawHardware();break;case SENSOR_STATUS_SCREEN:drawSensorStatus();break;case ABOUT_SCREEN:drawAbout();break;case SETTINGS_SCREEN:drawSettings();break;
  }display.display();
}

void drawHome(){
  display.clearDisplay();display.setTextSize(1);display.setCursor(40,0);display.println("VIGIL-01");
  display.setCursor(8,15);display.print(isnan(temperatureC)?"--":String(temperatureC,1));display.print("C");display.setCursor(73,15);display.print(isnan(humidity)?"--":String(humidity,0));display.println("%");
  display.setCursor(8,29);display.print(lightRaw);display.setCursor(73,29);display.print(soundRaw);display.setCursor(8,44);display.print(heartRate);display.print(" BPM");display.setCursor(72,44);
  if(systemStatus==STATUS_NORMAL)display.print("NORMAL");else if(systemStatus==STATUS_WARNING)display.print("WARNING");else if(systemStatus==STATUS_CRITICAL)display.print("CRITICAL");else display.print("NOTICE");
  display.setCursor(38,56);display.print("[ SELECT ]");
}
void drawMainMenu(){header("MENU");menuItem(16,"ENVIRONMENT",mainCursor==0);menuItem(27,"VITALS",mainCursor==1);menuItem(38,"INVESTIGATE",mainCursor==2);menuItem(49,"SYSTEM",mainCursor==3);}
void drawEnvironmentMenu(){header("ENVIRONMENT");menuItem(14,"TEMPERATURE",envCursor==0);menuItem(25,"HUMIDITY",envCursor==1);menuItem(36,"LIGHT",envCursor==2);menuItem(47,"SOUND",envCursor==3);menuItem(58,"CONDITIONS",envCursor==4);}
void drawVitalsMenu(){header("VITALS");menuItem(18,"HEART",vitalsCursor==0);menuItem(31,"SIGNAL",vitalsCursor==1);menuItem(44,"MEASUREMENT",vitalsCursor==2);}
void drawInvestigateMenu(){header("INVESTIGATE");menuItem(13,"IR REFLECTION",investigateCursor==0);menuItem(23,"OBJECT",investigateCursor==1);menuItem(33,"MAGNETIC",investigateCursor==2);menuItem(43,"WATER",investigateCursor==3);menuItem(53,"FLAME / IR",investigateCursor==4);}
void drawSystemMenu(){header("SYSTEM");menuItem(12,"BATTERY",systemCursor==0);menuItem(23,"HARDWARE",systemCursor==1);menuItem(34,"SENSORS",systemCursor==2);menuItem(45,"ABOUT",systemCursor==3);menuItem(56,"SETTINGS",systemCursor==4);}
void drawSettings(){
  header("SETTINGS");display.setCursor(0,14);display.print(settingsCursor==0?"> ":"  ");display.print("LEDS: ");display.println(ledsEnabled?"ON":"OFF");
  display.setCursor(0,29);display.print(settingsCursor==1?"> ":"  ");display.print("BUZZER: ");display.println(buzzerEnabled?"ON":"OFF");
  display.setCursor(0,44);display.print(settingsCursor==2?"> ":"  ");display.print("ALERTS: ");display.println(automaticAlerts?"GLOBAL":"PAGE");
  display.setCursor(4,57);display.println("SELECT = TOGGLE");
}
void drawValueScreen(const char* title,String value){header(title);display.setTextSize(2);int16_t x1,y1;uint16_t w,h;display.getTextBounds(value,0,0,&x1,&y1,&w,&h);display.setCursor((128-w)/2,28);display.println(value);display.setTextSize(1);display.setCursor(4,56);display.println("BACK = RETURN");}
void drawHeart(){header("HEART RATE");display.setCursor(4,16);display.println(heartSignal=="WEAK"?"PLACE FINGER":"SIGNAL DETECTED");int baseY=36;for(int x=0;x<118;x+=4){int sample=analogRead(PIN_HEART);int y=baseY-constrain((sample-heartBaseline)/15,-10,10);display.drawPixel(x+5,y,SSD1306_WHITE);}display.setCursor(4,48);display.print(heartRate);display.print(" BPM");display.setCursor(75,48);display.print(heartSignal);}
void drawMagnetic(){header("MAGNETIC");display.setCursor(4,17);display.print("RAW: ");display.println(hallRaw);display.setCursor(4,31);if(hallRaw>2148)display.println("FIELD: SOUTH");else if(hallRaw<1948)display.println("FIELD: NORTH");else display.println("FIELD: NONE");display.setCursor(4,46);display.println("RELATIVE SIGNAL");display.setCursor(4,57);display.println("BACK = RETURN");}
void drawConditions(){header("CONDITIONS");display.setCursor(4,17);if(systemStatus==STATUS_NORMAL)display.println("NORMAL");else if(systemStatus==STATUS_WARNING)display.println("ATTENTION");else if(systemStatus==STATUS_CRITICAL)display.println("CRITICAL");else display.println("NOTICE");display.setCursor(4,31);if(flameDetected)display.println("FLAME / IR EVENT");else if(soundRaw>SOUND_ALARM_THRESHOLD)display.println("SOUND EVENT");else if(waterRaw>WATER_ALARM_THRESHOLD)display.println("WATER DETECTED");else if(irDetected)display.println("OBJECT DETECTED");else display.println("NO MAJOR EVENTS");display.setCursor(4,48);display.print("T:");if(!isnan(temperatureC))display.print(temperatureC,1);else display.print("--");display.print("C H:");if(!isnan(humidity))display.print(humidity,0);else display.print("--");display.print("%");}
void drawHardware(){header("HARDWARE");display.setCursor(4,17);display.println("ESP32");display.setCursor(4,29);display.println("OLED 128x64");display.setCursor(4,41);display.println("VIGIL-01 V1");display.setCursor(4,53);display.println("LIVE MODE");}
void drawSensorStatus(){header("SENSOR STATUS");display.setCursor(0,14);display.println("DHT11       OK");display.setCursor(0,24);display.println("HEART       OK");display.setCursor(0,34);display.println("LIGHT       OK");display.setCursor(0,44);display.println("SOUND       OK");display.setCursor(0,54);display.println("IR/HALL/WTR OK");}
void drawAbout(){header("ABOUT");display.setCursor(4,16);display.println("VIGIL-01");display.setCursor(4,28);display.println("Environmental");display.setCursor(4,39);display.println("Intelligence Node");display.setCursor(4,52);display.println("V1 LIVE");}

void handleRoot(){
  String html=R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>VIGIL-01</title>
<style>body{font-family:Arial;background:#111;color:#fff;margin:20px}.card{background:#222;padding:16px;margin:10px 0;border-radius:12px}.value{font-size:28px}.status{font-size:24px;font-weight:bold}</style>
<script>async function update(){const r=await fetch('/data');const d=await r.json();document.getElementById('temp').innerText=(d.temperature??'--')+' °C';document.getElementById('hum').innerText=(d.humidity??'--')+' %';document.getElementById('light').innerText=d.light;document.getElementById('sound').innerText=d.sound;document.getElementById('heart').innerText=d.heart+' BPM';document.getElementById('hall').innerText=d.hall;document.getElementById('water').innerText=d.water;document.getElementById('status').innerText=d.status}setInterval(update,500);</script></head>
<body><h1>VIGIL-01</h1><p>Live Environmental Intelligence Node</p>
<div class="card">Temperature:<div class="value" id="temp">--</div></div><div class="card">Humidity:<div class="value" id="hum">--</div></div>
<div class="card">Light:<div class="value" id="light">--</div></div><div class="card">Sound:<div class="value" id="sound">--</div></div>
<div class="card">Heart:<div class="value" id="heart">--</div></div><div class="card">Magnetic:<div class="value" id="hall">--</div></div>
<div class="card">Water:<div class="value" id="water">--</div></div><div class="card">Status:<div class="status" id="status">--</div></div>
</body></html>)rawliteral";
  server.send(200,"text/html",html);
}

void handleData(){
  String status=systemStatus==STATUS_NORMAL?"NORMAL":systemStatus==STATUS_NOTICE?"NOTICE":systemStatus==STATUS_WARNING?"WARNING":"CRITICAL";
  String json="{";
  json+="\"temperature\":"+String(isnan(temperatureC)?String("null"):String(temperatureC,1));
  json+=",\"humidity\":"+String(isnan(humidity)?String("null"):String(humidity,0));
  json+=",\"light\":"+String(lightRaw);json+=",\"sound\":"+String(soundRaw);json+=",\"heart\":"+String(heartRate);json+=",\"hall\":"+String(hallRaw);json+=",\"water\":"+String(waterRaw);
  json+=",\"tcrt\":"+String(tcrtDetected?"true":"false");json+=",\"object\":"+String(irDetected?"true":"false");json+=",\"flame\":"+String(flameDetected?"true":"false");
  json+=",\"status\":\""+status+"\"";json+=",\"soundThreshold\":"+String(SOUND_ALARM_THRESHOLD);json+=",\"waterThreshold\":"+String(WATER_ALARM_THRESHOLD);json+=",\"ledsEnabled\":"+String(ledsEnabled?"true":"false");json+=",\"buzzerEnabled\":"+String(buzzerEnabled?"true":"false");json+=",\"automaticAlerts\":"+String(automaticAlerts?"true":"false");json+=",\"activeAlert\":"+String(alertActiveHere()?"true":"false");json+="}";
  server.send(200,"application/json",json);
}
