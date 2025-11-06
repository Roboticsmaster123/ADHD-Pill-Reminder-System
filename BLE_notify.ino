/*
  Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
  Ported to Arduino ESP32 by Evandro Copercini
  updated by chegewara and MoThunderz
*/
#include <WiFi.h>
#include <WebServer.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include "time.h"

const char* ssid = "DukeVisitor";
const char* password = "";

const char* ntpServer1 = "time.google.com";
const char* ntpServer2 = "time.nist.gov";

const char* tzString = "EST5EDT,M3.2.0/2,M11.1.0/2";

WebServer server(80);

//variables to hold user input data
String dayTimes[7][10];
int dayCount[7] = {0};                     // number of entries per day
const char* dayKeys[7] = {"sun","mon","tue","wed","thu","fri","sat"};

//put the HTML
const char index_html[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>Reminder Scheduler</title>
<style>
:root{
  --bg:#06181a;--card:#0f2a2e;--muted:#94d2bd;--accent:#2ec4b6;--accent-2:#00a896;
  --text:#e6fffb;--sub:#b7f4e8;--chip:#07373c;
  --blue:#219ebc;--blue-light:#38c3da;
  --radius:16px;
}
*{box-sizing:border-box}
body{margin:0;font-family:system-ui;color:var(--text);
background:linear-gradient(180deg,#031013,#06181a 40%,#041013)}
header{text-align:center;padding:28px 20px 10px}
.title{font-size:clamp(22px,4vw,32px);font-weight:700}
.subtitle{max-width:750px;margin:0 auto;color:var(--sub);opacity:.85;font-size:14px}
.container{max-width:980px;margin:22px auto 60px;padding:0 16px}
.grid{display:grid;gap:18px;grid-template-columns:repeat(auto-fit,minmax(450px,1fr));}
.card{background:linear-gradient(180deg,#103239,#0f2326);
border:1px solid rgba(255,255,255,.06);border-radius:var(--radius);box-shadow:0 6px 18px rgba(0,0,0,.35)}
.card .head{padding:16px 18px;border-bottom:1px solid rgba(255,255,255,.06)}
.card .head h2{margin:0;font-size:16px;font-weight:700}
.card .body{padding:18px}
.pill-days{display:grid;grid-template-columns:repeat(7,1fr);gap:8px}
.day{border:1px solid rgba(255,255,255,.08);background:#0b262a;color:var(--sub);
padding:10px 0;border-radius:10px;cursor:pointer;text-align:center;font-weight:600;
transition:.2s}
.day[aria-pressed="true"]{background:linear-gradient(180deg,var(--accent),var(--accent-2));
color:#012a2a;box-shadow:0 0 0 3px rgba(46,196,182,.35);}
.field{display:flex;align-items:center;gap:6px;background:#0b262a;
border:1px solid rgba(255,255,255,.1);border-radius:12px;padding:10px 12px;position:relative}
input[type="time"]{background:transparent;border:none;color:var(--text);font-size:16px;width:130px;outline:none}
.edit-hint{position:absolute;right:10px;color:var(--sub);font-size:12px;opacity:.8;cursor:pointer;transition:.2s}
.edit-hint.fade{opacity:0;pointer-events:none}
.hint{font-size:12px;color:var(--sub);opacity:.8;margin-top:6px}
.btn{border:none;cursor:pointer;padding:10px 14px;font-weight:700;border-radius:12px;
transition:.15s}
.btn.teal{background:linear-gradient(180deg,#3ed7c1,#22a593);color:#012a2a;}
.btn.save{background:linear-gradient(180deg,#3fa9f5,#219ebc);color:#012a2a;}
.btn.load{background:linear-gradient(180deg,#42d4e0,#38c3da);color:#012a2a;}
.btn.secondary{background:#0b262a;color:var(--sub);border:1px solid rgba(255,255,255,.12)}
.btn.ghost{background:transparent;color:var(--sub);border:1px dashed rgba(255,255,255,.18)}
.btn:active{transform:translateY(1px)}
.schedule{display:grid;gap:12px;grid-template-columns:repeat(auto-fit,minmax(130px,1fr))}
.daycard{background:var(--card);border:1px solid rgba(255,255,255,.06);border-radius:14px;
padding:12px;display:flex;flex-direction:column;gap:10px}
.daycard h3{margin:0;font-size:13px;text-transform:uppercase;color:var(--muted)}
.chips{display:flex;flex-wrap:wrap;gap:8px}
.chip{background:var(--chip);border:1px solid rgba(255,255,255,.1);border-radius:999px;
padding:6px 10px;font-weight:700;font-size:13px;display:inline-flex;align-items:center;gap:8px}
.chip button{border:none;cursor:pointer;width:18px;height:18px;border-radius:50%;
font-weight:900;color:#062d31;background:var(--muted)}
.toast{position:fixed;left:50%;transform:translateX(-50%);bottom:18px;background:#0b262a;
border:1px solid rgba(255,255,255,.12);padding:10px 14px;border-radius:12px;display:none}
.toast.show{display:block;animation:pop .2s ease}@keyframes pop{from{transform:translate(-50%,8px);opacity:0}to{transform:translate(-50%,0);opacity:1}}
</style>
</head>
<body>
<header>
  <h1 class="title">Reminder Scheduler</h1>
  <p class="subtitle">Set your daily reminders. Pick a time, choose days, and add them to your schedule.</p>
</header>

<main class="container">
  <div class="grid">
    <!-- Build Reminder -->
    <section class="card">
      <div class="head"><h2>Build a Reminder</h2></div>
      <div class="body">
        <label class="hint">Pick a time</label>
        <div class="field">
          <input id="time" type="time" value="08:00">
          <div id="editHint" class="edit-hint">⮞ Click to edit</div>
        </div>
        <div class="hint">Time format:</div>
        <div style="margin-top:4px;">
          <input type="checkbox" id="toggleFormat">
          <label for="toggleFormat" class="hint">Use 24-hour (military) time</label>
        </div>
        <div class="hint" style="margin-top:14px;">Select days</div>
        <div class="pill-days" style="margin-top:6px;">
          <button class="day" data-day="sun">Sun</button>
          <button class="day" data-day="mon">Mon</button>
          <button class="day" data-day="tue">Tue</button>
          <button class="day" data-day="wed">Wed</button>
          <button class="day" data-day="thu">Thu</button>
          <button class="day" data-day="fri">Fri</button>
          <button class="day" data-day="sat">Sat</button>
        </div>
        <div class="row" style="margin-top:12px;display:flex;gap:8px;flex-wrap:wrap;">
          <button id="select-all" class="btn secondary">Select All Days</button>
          <button id="add-time" class="btn teal">Add Time to Schedule</button>
        </div>
      </div>
    </section>

    <!-- Device Controls -->
    <section class="card">
      <div class="head"><h2>Device Controls</h2></div>
      <div class="body">
        <div class="row" style="display:flex;gap:10px;flex-wrap:wrap;">
          <button id="save" class="btn save">Save to Device</button>
          <button id="load" class="btn load">Load Schedule from Device</button>
          <button id="clear-all" class="btn ghost">Clear All</button>
        </div>
      </div>
    </section>
  </div>

  <!-- Schedule below -->
  <section class="card" style="margin-top:18px;">
    <div class="head"><h2>Per-Day Schedule</h2></div>
    <div class="body">
      <div id="schedule" class="schedule"></div>
    </div>
  </section>
</main>

<div id="toast" class="toast"></div>

<script>
const DAYS=["sun","mon","tue","wed","thu","fri","sat"];
const LABELS={sun:"Sunday",mon:"Monday",tue:"Tuesday",wed:"Wednesday",thu:"Thursday",fri:"Friday",sat:"Saturday"};
const schedule=Object.fromEntries(DAYS.map(d=>[d,[]]));
const $=s=>document.querySelector(s),$$=s=>[...document.querySelectorAll(s)];
const toast=(m,d=false)=>{const t=$("#toast");t.textContent=m;t.className="toast show"+(d?" danger":"");setTimeout(()=>t.className="toast",2200)};
const uniqSorted=a=>[...new Set(a)].sort();

function formatTime(t){if($("#toggleFormat").checked)return t;let[h,m]=t.split(":").map(Number);const ampm=h>=12?"PM":"AM";h=h%12||12;return`${h}:${m.toString().padStart(2,"0")} ${ampm}`;}

function getSelectedDays(){return $$(".day[aria-pressed='true']").map(b=>b.dataset.day);}
function render(){
  const wrap=$("#schedule");wrap.innerHTML="";
  for(const d of DAYS){
    const card=document.createElement("div");card.className="daycard";
    card.innerHTML=`<h3>${LABELS[d]}</h3>`;
    const chips=document.createElement("div");chips.className="chips";
    if(!schedule[d].length){const empty=document.createElement("div");empty.textContent="No times";empty.style.opacity=".6";chips.appendChild(empty);}
    for(const t of uniqSorted(schedule[d])){
      const chip=document.createElement("span");chip.className="chip";chip.textContent=formatTime(t)+" ";
      const btn=document.createElement("button");btn.textContent="×";btn.onclick=()=>{schedule[d]=schedule[d].filter(x=>x!==t);render();};
      chip.appendChild(btn);chips.appendChild(chip);
    }
    card.appendChild(chips);wrap.appendChild(card);
  }
}

function addTime(){
  const t=$("#time").value.trim();if(!/^\d{2}:\d{2}$/.test(t))return toast("Invalid time",true);
  const days=getSelectedDays();if(!days.length)return toast("Select at least one day",true);
  for(const d of days){schedule[d].push(t);schedule[d]=uniqSorted(schedule[d]);}
  render();toast("Added "+t);
}

function clearAll(){for(const d of DAYS)schedule[d]=[];render();}

$$(".day").forEach(b=>b.onclick=()=>b.setAttribute("aria-pressed",b.getAttribute("aria-pressed")!=="true"));
$("#select-all").onclick=()=>{$$(".day").forEach(b=>b.setAttribute("aria-pressed","true"));};
$("#add-time").onclick=addTime;
$("#clear-all").onclick=()=>{if(confirm("Clear all times?"))clearAll();};
$("#toggleFormat").onchange=render;
$("#time").onclick=()=>$("#editHint").classList.add("fade");

async function saveSchedule(){try{await fetch("/api/schedule",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(schedule)});toast("Saved.");}catch(e){toast("Save failed",true);}}
async function loadSchedule(){try{const r=await fetch("/api/schedule");const j=await r.json();
for(const d of DAYS)schedule[d]=uniqSorted((Array.isArray(j[d])?j[d]:[]).filter(x=>/^\d{2}:\d{2}$/.test(x)));
render();toast("Loaded.");}catch(e){toast("Load failed",true);}}
$("#save").onclick=saveSchedule;$("#load").onclick=loadSchedule;
render();
</script>
</body>
</html>)rawliteral";



// Initialize all pointers
BLEServer* pServer = NULL;                        // Pointer to the server
BLECharacteristic* pCharacteristic_1 = NULL;      // Pointer to Characteristic 1
BLECharacteristic* pCharacteristic_2 = NULL;      // Pointer to Characteristic 2
BLEDescriptor *pDescr_1;                          // Pointer to Descriptor of Characteristic 1
BLE2902 *pBLE2902_1;                              // Pointer to BLE2902 of Characteristic 1
BLE2902 *pBLE2902_2;                              // Pointer to BLE2902 of Characteristic 2

// Some variables to keep track on device connected
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Variable that will continuously be increased and written to the client
uint32_t value = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
// UUIDs used in this example:
#define SERVICE_UUID          "ab219760-9820-4be9-ba75-36a441f56e67"
#define CHARACTERISTIC_UUID_1 "ab29a8ea-6f7e-4d57-8eeb-e19b70ec9f68"
#define CHARACTERISTIC_UUID_2 "5ecbecb9-faef-4a23-96b6-3f84b577ccf9"

// Callback function that is called whenever a client is connected or disconnected
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

//simple web server
void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

// Receive schedule data from the web page
void handleSchedule() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "no body");
    return;
  }

  String json = server.arg("plain");
  Serial.println("Received JSON:");
  Serial.println(json);

  StaticJsonDocument<2048> doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    server.send(400, "text/plain", "bad json");
    Serial.println("JSON parse error!");
    return;
  }

  // loop through all days
  for (int d = 0; d < 7; d++) {
    JsonArray arr = doc[dayKeys[d]].as<JsonArray>();
    dayCount[d] = 0;
    for (const char* t : arr) {
      if (dayCount[d] < 10) {
        dayTimes[d][dayCount[d]++] = String(t);
      }
    }
  }
  // print what we parsed
  for (int d = 0; d < 7; d++) {
    Serial.print(dayKeys[d]);
    Serial.println(":");
    for (int i = 0; i < dayCount[d]; i++) {
      Serial.print("  ");
      Serial.println(dayTimes[d][i]);
    }
  }

  server.send(200, "text/plain", "Saved");

  // Example: forward data into your BLE characteristic
  if (pCharacteristic_2) pCharacteristic_2->setValue(json.c_str());
  server.send(200, "text/plain", "OK");
}

void handleScheduleGet() {
  StaticJsonDocument<1024> doc;

  // Example: send back whatever you saved earlier
  for (int d = 0; d < 7; d++) {
    JsonArray arr = doc[dayKeys[d]].to<JsonArray>();
    for (int i = 0; i < dayCount[d]; i++) {
      arr.add(dayTimes[d][i]);
    }
  }

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic_1 = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_1,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );                   

  pCharacteristic_2 = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_2,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |                      
                      BLECharacteristic::PROPERTY_NOTIFY
                    );  

  // Create a BLE Descriptor  
  pDescr_1 = new BLEDescriptor((uint16_t)0x2901);
  pDescr_1->setValue("A very interesting variable");
  pCharacteristic_1->addDescriptor(pDescr_1);

  // Add the BLE2902 Descriptor because we are using "PROPERTY_NOTIFY"
  pBLE2902_1 = new BLE2902();
  pBLE2902_1->setNotifications(true);                 
  pCharacteristic_1->addDescriptor(pBLE2902_1);

  pBLE2902_2 = new BLE2902();
  pBLE2902_2->setNotifications(true);
  pCharacteristic_2->addDescriptor(pBLE2902_2);

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");

  //WIFI and HTTP SetUP
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi‑Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  // Register HTTP routes
  server.on("/", handleRoot);
  server.on("/api/schedule", HTTP_POST, handleSchedule);
  server.on("/api/schedule", HTTP_GET, handleScheduleGet);    // "Load" button
  server.begin();
  Serial.println("HTTP server started");

  //Set local time zone: EST
  configTime(0, 0, ntpServer1, ntpServer2);   // start NTP client
  setenv("TZ", tzString, 1);                 // set time‑zone string
  tzset();

  struct tm timeinfo;

  if (getLocalTime(&timeinfo)) {
    Serial.println("Got NTP time:");
    Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S %Z%z");
  } else {
    Serial.println("Failed to get time");
  }
  }

void loop() {
    // notify changed value
    if (deviceConnected) {
      // pCharacteristic_1 is an integer that is increased with every second
      // in the code below we send the value over to the client and increase the integer counter
      pCharacteristic_1->setValue(value);
      pCharacteristic_1->notify();
      value++;

      // pCharacteristic_2 is a std::string (NOT a String). In the code below we read the current value
      // write this to the Serial interface and send a different value back to the Client
      // Here the current value is read using getValue() 
      String rxValue = pCharacteristic_2->getValue();
      //Serial.print("Characteristic 2 (getValue): ");
      //Serial.println(rxValue.c_str());

      // Here the value is written to the Client using setValue();
      String txValue = "String with random value from Server: " + String(random(1000));
      pCharacteristic_2->setValue(txValue.c_str());
      //Serial.println("Characteristic 2 (setValue): " + txValue);

      // In this example "delay" is used to delay with one second. This is of course a very basic 
      // implementation to keep things simple. I recommend to use millis() for any production code
      delay(1000);
    }
    // The code below keeps the connection status uptodate:
    // Disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // Connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
    //server
    server.handleClient();
}
