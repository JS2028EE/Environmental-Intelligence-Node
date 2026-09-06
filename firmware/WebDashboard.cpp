#include "WebDashboard.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "Types.h"
#include "Sensors.h"
#include "Settings.h"
#include "Alerts.h"

static WebServer server(80);
static DNSServer dnsServer;
static const byte DNS_PORT = 53;

static void handleRoot();
static void handleData();
static void handleGetSettings();
static void handlePostSettings();
static void handleCaptivePortal();

void webBegin() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress apIP = WiFi.softAPIP();

  // Answers every DNS query with our own IP, so a phone that joins the AP
  // and probes for internet connectivity gets redirected here instead of
  // reporting "no internet" — the dashboard just pops up.
  dnsServer.start(DNS_PORT, "*", apIP);

  if (MDNS.begin(MDNS_HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.on("/api/settings", HTTP_GET, handleGetSettings);
  server.on("/api/settings", HTTP_POST, handlePostSettings);
  server.on("/generate_204", handleCaptivePortal);       // Android probe
  server.on("/hotspot-detect.html", handleCaptivePortal); // Apple probe
  server.on("/ncsi.txt", handleCaptivePortal);            // Windows probe
  server.onNotFound(handleCaptivePortal);
  server.begin();

  Serial.println("VIGIL-01 SYSTEM READY");
  Serial.print("AP IP: ");
  Serial.println(apIP);
  Serial.print("Dashboard: http://");
  Serial.print(MDNS_HOSTNAME);
  Serial.println(".local/");
}

void webPoll() {
  dnsServer.processNextRequest();
  server.handleClient();
}

static void handleCaptivePortal() {
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
}

static void handleRoot() {
  static const char PAGE[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>VIGIL-01</title>
<style>
  :root{--bg:#0b0f14;--panel:#121821;--line:#212b38;--fg:#e8edf3;--muted:#7c8a9c;
        --ok:#3ddc84;--warn:#ffb020;--crit:#ff4d4f;--accent:#4da3ff}
  *{box-sizing:border-box}
  body{margin:0;background:var(--bg);color:var(--fg);
       font:15px/1.4 -apple-system,Segoe UI,Roboto,sans-serif;padding:20px 20px 60px}
  h1{font-size:20px;margin:0 0 2px;letter-spacing:.5px}
  .sub{color:var(--muted);font-size:13px;margin-bottom:16px}
  .status{display:inline-block;padding:4px 12px;border-radius:20px;font-weight:600;
          font-size:13px;margin-bottom:18px}
  .status.NORMAL{background:rgba(61,220,132,.15);color:var(--ok)}
  .status.WARNING{background:rgba(255,176,32,.15);color:var(--warn)}
  .status.CRITICAL{background:rgba(255,77,79,.15);color:var(--crit)}
  .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:12px}
  .card{background:var(--panel);border:1px solid var(--line);border-radius:10px;padding:12px 14px}
  .card .label{color:var(--muted);font-size:12px;text-transform:uppercase;letter-spacing:.5px}
  .card .value{font-size:22px;font-weight:600;margin-top:4px}
  .card.flag-on{border-color:var(--crit)}
  section{margin-top:26px}
  h2{font-size:14px;color:var(--muted);text-transform:uppercase;letter-spacing:.5px;margin-bottom:10px}
  .row{display:flex;gap:10px;align-items:center;margin-bottom:10px;flex-wrap:wrap}
  label{width:150px;color:var(--muted);font-size:13px}
  input[type=number]{background:var(--panel);border:1px solid var(--line);color:var(--fg);
                      border-radius:6px;padding:6px 8px;width:100px}
  button{background:var(--accent);color:#04121f;border:0;border-radius:6px;
         padding:8px 16px;font-weight:600;cursor:pointer}
  button:active{opacity:.8}
  #saveMsg{color:var(--ok);font-size:13px;opacity:0;transition:opacity .3s}
</style>
</head>
<body>
  <h1>VIGIL-01</h1>
  <div class="sub">Portable Environmental Intelligence Node</div>
  <div id="statusPill" class="status NORMAL">NORMAL</div>
  <div class="grid" id="grid"></div>

  <section>
    <h2>Alarm Thresholds</h2>
    <div class="row"><label>Sound (raw ADC)</label><input id="soundThr" type="number"></div>
    <div class="row">
      <label>Water (raw ADC)</label><input id="waterThr" type="number">
      <button onclick="saveThresholds()">Save</button><span id="saveMsg">Saved</span>
    </div>
  </section>

<script>
const grid = document.getElementById('grid');
const fields = [
  ['temperatureC','Temp \u00b0C'], ['humidity','Humidity %'],
  ['lightRaw','Light'], ['soundRaw','Sound'],
  ['heartRate','Heart Rate BPM'], ['heartSignal','Signal'],
  ['hallRaw','Magnetic'], ['waterRaw','Water'],
  ['tcrtDetected','IR Reflection'], ['irDetected','Object'],
  ['flameDetected','Flame / IR']
];

function card(label, value, alert){
  return '<div class="card' + (alert ? ' flag-on' : '') + '">' +
         '<div class="label">' + label + '</div>' +
         '<div class="value">' + value + '</div></div>';
}

async function refresh(){
  try{
    const r = await fetch('/data');
    const j = await r.json();
    const pill = document.getElementById('statusPill');
    pill.textContent = j.status;
    pill.className = 'status ' + j.status;
    grid.innerHTML = fields.map(([key,label]) => {
      let v = j[key];
      let alert = false;
      if (typeof v === 'boolean'){ alert = v; v = v ? 'DETECTED' : 'CLEAR'; }
      if (v === null) v = '--';
      return card(label, v, alert);
    }).join('');
  }catch(e){ /* AP briefly busy — next tick will retry */ }
}

async function loadSettings(){
  try{
    const r = await fetch('/api/settings');
    const j = await r.json();
    document.getElementById('soundThr').value = j.soundThreshold;
    document.getElementById('waterThr').value = j.waterThreshold;
  }catch(e){}
}

async function saveThresholds(){
  const body = {
    soundThreshold: parseInt(document.getElementById('soundThr').value, 10),
    waterThreshold: parseInt(document.getElementById('waterThr').value, 10)
  };
  await fetch('/api/settings', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify(body)
  });
  const m = document.getElementById('saveMsg');
  m.style.opacity = 1;
  setTimeout(() => m.style.opacity = 0, 1500);
}

loadSettings();
refresh();
setInterval(refresh, 700);
</script>
</body>
</html>)HTML";
  server.send_P(200, "text/html", PAGE);
}

static void handleData() {
  StaticJsonDocument<640> doc;
  doc["temperatureC"]    = sensors.temperatureC;  // NaN serializes to JSON null automatically
  doc["humidity"]        = sensors.humidity;
  doc["lightRaw"]        = sensors.lightRaw;
  doc["soundRaw"]        = sensors.soundRaw;
  doc["heartRaw"]        = sensors.heartRaw;
  doc["heartRate"]       = sensors.heartRate;
  doc["heartSignal"]     = sensors.heartSignal;
  doc["hallRaw"]         = sensors.hallRaw;
  doc["waterRaw"]        = sensors.waterRaw;
  doc["tcrtDetected"]    = sensors.tcrtDetected;
  doc["irDetected"]      = sensors.irDetected;
  doc["flameDetected"]   = sensors.flameDetected;
  doc["status"]          = toString(systemStatus);
  doc["soundThreshold"]  = settings.soundThreshold;
  doc["waterThreshold"]  = settings.waterThreshold;
  doc["activeAlert"]     = alertActiveHere();
  doc["ledsEnabled"]     = settings.ledsEnabled;
  doc["buzzerEnabled"]   = settings.buzzerEnabled;
  doc["alertsMode"]      = settings.automaticAlerts ? "GLOBAL" : "PAGE";
  doc["unitsPreference"] = settings.useFahrenheit ? "F" : "C";

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

static void handleGetSettings() {
  StaticJsonDocument<256> doc;
  doc["ledsEnabled"]     = settings.ledsEnabled;
  doc["buzzerEnabled"]   = settings.buzzerEnabled;
  doc["automaticAlerts"] = settings.automaticAlerts;
  doc["useFahrenheit"]   = settings.useFahrenheit;
  doc["soundThreshold"]  = settings.soundThreshold;
  doc["waterThreshold"]  = settings.waterThreshold;
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

static void handlePostSettings() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"missing body\"}");
    return;
  }
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, server.arg("plain"));
  if (err) {
    server.send(400, "application/json", "{\"error\":\"bad json\"}");
    return;
  }

  if (doc.containsKey("soundThreshold")) {
    settings.soundThreshold = doc["soundThreshold"].as<int>();
    settingsSaveInt("soundThr", settings.soundThreshold);
  }
  if (doc.containsKey("waterThreshold")) {
    settings.waterThreshold = doc["waterThreshold"].as<int>();
    settingsSaveInt("waterThr", settings.waterThreshold);
  }
  if (doc.containsKey("ledsEnabled")) {
    settings.ledsEnabled = doc["ledsEnabled"].as<bool>();
    settingsSaveFlag("leds", settings.ledsEnabled);
  }
  if (doc.containsKey("buzzerEnabled")) {
    settings.buzzerEnabled = doc["buzzerEnabled"].as<bool>();
    settingsSaveFlag("buzzer", settings.buzzerEnabled);
    if (!settings.buzzerEnabled) noTone(PIN_BUZZER);
  }
  if (doc.containsKey("automaticAlerts")) {
    settings.automaticAlerts = doc["automaticAlerts"].as<bool>();
    settingsSaveFlag("alerts", settings.automaticAlerts);
  }
  if (doc.containsKey("useFahrenheit")) {
    settings.useFahrenheit = doc["useFahrenheit"].as<bool>();
    settingsSaveFlag("fahren", settings.useFahrenheit);
  }
  server.send(200, "application/json", "{\"ok\":true}");
}
