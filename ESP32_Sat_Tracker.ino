#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AccelStepper.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LittleFS.h>
#include <Sgp4.h> 
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Preferences.h>
#include "secrets.h"

// ==========================================
// 1. ADVANCED AEROSPACE DASHBOARD (PROGMEM)
// ==========================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>AEROSPACE TELEMETRY CMD</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Inter:wght@400;600&display=swap');
    
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: 'Inter', sans-serif; background: #030712; color: #e2e8f0; min-height: 100vh; display: flex; flex-direction: column; align-items: center; padding: 20px; }
    h1 { font-family: 'Share Tech Mono', monospace; font-size: 1.8rem; color: #22d3ee; letter-spacing: 4px; margin: 15px 0 5px 0; text-align: center; text-shadow: 0 0 10px rgba(34, 211, 238, 0.3); }
    .subtitle { font-size: 0.7rem; color: #64748b; letter-spacing: 3px; margin-bottom: 25px; text-align: center; }

    .badge-container { display: flex; gap: 10px; margin-bottom: 25px; }
    .status-badge { padding: 6px 16px; border-radius: 4px; font-size: 0.75rem; font-weight: 600; letter-spacing: 2px; text-transform: uppercase; border: 1px solid transparent; }
    .status-tracking { background: rgba(16, 185, 129, 0.1); color: #10b981; border-color: rgba(16, 185, 129, 0.3); }
    .status-offline { background: rgba(239, 68, 68, 0.1); color: #ef4444; border-color: rgba(239, 68, 68, 0.3); }
    .status-mode { background: rgba(34, 211, 238, 0.1); color: #22d3ee; border-color: rgba(34, 211, 238, 0.3); }

    .container { display: grid; grid-template-columns: 1fr; gap: 20px; width: 100%; max-width: 500px; }
    
    .panel { background: #0f172a; border: 1px solid #1e293b; border-radius: 8px; padding: 20px; box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.5); }
    .panel-header { font-size: 0.75rem; color: #94a3b8; letter-spacing: 2px; text-transform: uppercase; margin-bottom: 15px; border-bottom: 1px solid #1e293b; padding-bottom: 8px; }

    .telemetry-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin-bottom: 20px; }
    .data-card { background: #020617; border: 1px solid #1e293b; border-radius: 6px; padding: 15px; text-align: center; position: relative; }
    .data-card::before { content: ''; position: absolute; top: 0; left: 0; right: 0; height: 2px; background: #22d3ee; }
    .data-label { font-size: 0.65rem; letter-spacing: 2px; color: #64748b; text-transform: uppercase; margin-bottom: 5px; }
    .data-value { font-family: 'Share Tech Mono', monospace; font-size: 2.5rem; color: #f8fafc; }
    .data-unit { font-size: 1rem; color: #22d3ee; vertical-align: super; }

    .info-row { display: flex; justify-content: space-between; margin-bottom: 8px; font-size: 0.85rem; border-bottom: 1px dashed #1e293b; padding-bottom: 4px; }
    .info-key { color: #64748b; } .info-val { font-family: 'Share Tech Mono', monospace; color: #cbd5e1; }
    .highlight { color: #22d3ee; }

    input, select, button { width: 100%; padding: 12px; margin-bottom: 12px; background: #020617; border: 1px solid #1e293b; color: #e2e8f0; border-radius: 4px; font-family: 'Inter', sans-serif; font-size: 0.85rem; outline: none; transition: 0.2s; }
    input:focus, select:focus { border-color: #22d3ee; }
    
    button { background: rgba(34, 211, 238, 0.1); color: #22d3ee; border: 1px solid #22d3ee; cursor: pointer; font-weight: 600; letter-spacing: 1px; text-transform: uppercase; }
    button:hover { background: #22d3ee; color: #020617; }
    .input-group { display: flex; gap: 8px; } .input-group input { margin-bottom: 0; } .input-group button { margin-bottom: 0; width: auto; flex-shrink: 0; }

    .radar-wrapper { display: flex; justify-content: center; margin: 20px 0; position: relative; }
    canvas { background: #020617; border-radius: 50%; box-shadow: 0 0 20px rgba(34, 211, 238, 0.05); }

    .joystick { display: grid; grid-template-columns: repeat(3, 50px); gap: 6px; justify-content: center; margin-top: 15px; }
    .joystick button { padding: 15px 0; margin: 0; background: #0f172a; border: 1px solid #1e293b; color: #94a3b8; }
    .joystick button:hover { background: #22d3ee; color: #020617; }
    
    .conn-lost { display: none; position: fixed; top: 0; left: 0; width: 100%; background: #ef4444; color: white; text-align: center; padding: 8px; font-size: 0.8rem; font-weight: bold; letter-spacing: 2px; z-index: 999; }
    
    .search-box { background: rgba(34, 211, 238, 0.05); border: 1px solid #22d3ee; color: #22d3ee; font-weight: bold; }
    .search-box::placeholder { color: rgba(34, 211, 238, 0.4); }
  </style>
</head>
<body>

  <div class="conn-lost" id="connLost">⚠ LINK LOST — REESTABLISHING CONNECTION...</div>
  
  <h1>AEROSPACE CMD</h1>
  <p class="subtitle">ESP32 GROUND STATION</p>
  
  <div class="badge-container">
    <div class="status-badge status-offline" id="statusBadge">CONNECTING...</div>
    <div class="status-badge status-mode" id="modeBadge">--</div>
  </div>

  <div class="container">
    
    <div class="panel">
      <div class="panel-header">Live Telemetry</div>
      <div class="telemetry-grid">
        <div class="data-card">
          <div class="data-label">Azimuth</div>
          <div class="data-value" id="curAz">--<span class="data-unit">°</span></div>
        </div>
        <div class="data-card">
          <div class="data-label">Elevation</div>
          <div class="data-value" id="curEl">--<span class="data-unit">°</span></div>
        </div>
      </div>
      
      <div class="info-row"><span class="info-key">Target AZ</span><span class="info-val highlight" id="tgtAz">--°</span></div>
      <div class="info-row"><span class="info-key">Target EL</span><span class="info-val highlight" id="tgtEl">--°</span></div>
      <div class="info-row"><span class="info-key">Tracking Target</span><span class="info-val" id="satName">--</span></div>
      <div class="info-row"><span class="info-key">System Uptime</span><span class="info-val" id="uptime">--</span></div>
      <div class="info-row" style="border:none; margin-bottom:0;"><span class="info-key">Free Memory</span><span class="info-val" id="ram">-- KB</span></div>
    </div>

    <div class="panel">
      <div class="panel-header">Tactical Radar</div>
      <div class="radar-wrapper">
        <canvas id="radar" width="300" height="300"></canvas>
      </div>
      <div class="joystick">
        <div></div><button onclick="nudge(0, 5)">▲</button><div></div>
        <button onclick="nudge(-5, 0)">◀</button><div></div><button onclick="nudge(5, 0)">▶</button>
        <div></div><button onclick="nudge(0, -5)">▼</button><div></div>
      </div>
    </div>

    <div class="panel">
      <div class="panel-header">Orbital Database (SGP4)</div>
      
      <div class="info-row" style="border:none; margin-bottom:12px;">
        <span class="info-key" style="line-height:2.5;">Tracking Engine</span>
        <select id="mode-select" onchange="updateMode()" style="width:160px; margin:0;">
          <option value="0">External (Look4Sat / Manual)</option>
          <option value="1">Internal SGP4 (Auto)</option>
        </select>
      </div>

      <div class="input-group">
        <input type="text" id="grid-input" placeholder="Observer Grid (e.g. MM71dl)" maxlength="6">
        <button onclick="updateGrid()">SET GRID</button>
      </div>

      <button onclick="fetchCelestrak()" id="btn-fetch" style="margin-top:10px;">1. DOWNLOAD CELESTRAK DB</button>
      
      <input type="text" id="sat-search" class="search-box" placeholder="Search Satellite (e.g. NOAA 19)" oninput="filterSatellites()" disabled>
      
      <select id="sat-select"><option>Awaiting Database...</option></select>
      <button onclick="pushTLE()" id="btn-push" style="margin-bottom: 0;">2. UPLOAD TLE TO HARDWARE</button>
    </div>

  </div>

<script>
  let currentAz = 0, currentEl = 0, targetAz = 0, targetEl = 0;
  let allTleData = []; // Stores the raw downloaded list
  let filteredTleData = []; // Stores the list filtered by search
  let satPath = [];
  let failCount = 0;
  let sweepAngle = 0;

  async function fetchTelemetry() {
    try {
      const response = await fetch('/api/status');
      if (!response.ok) throw new Error('Network error');
      const data = await response.json();
      
      failCount = 0;
      document.getElementById('connLost').style.display = 'none';

      currentAz = data.cAz; currentEl = data.cEl;
      targetAz = data.tAz; targetEl = data.tEl;
      
      document.getElementById('curAz').innerHTML = currentAz.toFixed(1).padStart(5, '0') + '<span class="data-unit">°</span>';
      document.getElementById('curEl').innerHTML = currentEl.toFixed(1).padStart(4, '0') + '<span class="data-unit">°</span>';
      document.getElementById('tgtAz').innerText = targetAz.toFixed(1) + '°';
      document.getElementById('tgtEl').innerText = targetEl.toFixed(1) + '°';
      
      document.getElementById('satName').innerText = data.sat;
      document.getElementById('ram').innerText = data.freeHeap + ' KB';
      
      const up = data.uptime;
      document.getElementById('uptime').innerText = `${Math.floor(up/3600)}h ${Math.floor((up%3600)/60)}m ${up%60}s`;

      document.getElementById('grid-input').placeholder = data.grid;
      document.getElementById('mode-select').value = data.mode === 1 ? "1" : "0";
      
      const badge = document.getElementById('statusBadge');
      badge.className = 'status-badge status-tracking';
      badge.innerText = '● LINK ACTIVE';
      
      document.getElementById('modeBadge').innerText = data.mode === 1 ? 'MODE: ONBOARD SGP4' : 'MODE: EXTERNAL L4S';

    } catch (error) {
      failCount++;
      if (failCount > 2) {
        document.getElementById('connLost').style.display = 'block';
        document.getElementById('statusBadge').className = 'status-badge status-offline';
        document.getElementById('statusBadge').innerText = '✕ OFFLINE';
      }
    }
  }

  async function fetchPath() {
    try {
      const response = await fetch('/api/path');
      if (response.ok) satPath = await response.json();
    } catch (error) { console.log("Path fetch failed"); }
  }

  function fetchCelestrak() {
    const btn = document.getElementById('btn-fetch');
    btn.innerText = "DOWNLOADING...";
    fetch('https://celestrak.org/NORAD/elements/gp.php?GROUP=active&FORMAT=tle')
    .then(r => r.text())
    .then(txt => {
      allTleData = [];
      const lines = txt.split('\n');
      for(let i=0; i<lines.length-2; i+=3) {
        const name = lines[i].trim();
        // AUTO-FILTER: Remove Starlink, OneWeb, and Flock trash
        if(name.length > 0 && lines[i+1].startsWith('1 ') && lines[i+2].startsWith('2 ')) {
          if(!name.includes("STARLINK") && !name.includes("ONEWEB") && !name.includes("FLOCK")) {
            allTleData.push({name: name, l1: lines[i+1].trim(), l2: lines[i+2].trim()});
          }
        }
      }
      btn.innerText = "DB LOADED (" + allTleData.length + " TARGETS)";
      btn.style.color = "#10b981"; btn.style.borderColor = "#10b981";
      document.getElementById('sat-search').disabled = false;
      filterSatellites(); // Populate the dropdown
    }).catch(e => { btn.innerText = "DOWNLOAD FAILED"; });
  }

  function filterSatellites() {
    const query = document.getElementById('sat-search').value.toLowerCase();
    const select = document.getElementById('sat-select');
    select.innerHTML = '';
    filteredTleData = allTleData.filter(sat => sat.name.toLowerCase().includes(query));
    
    if (filteredTleData.length === 0) {
      select.innerHTML = '<option>No matches found</option>';
      return;
    }
    
    filteredTleData.forEach((sat, idx) => {
      const opt = document.createElement('option');
      opt.value = idx; 
      opt.innerHTML = sat.name;
      select.appendChild(opt);
    });
  }

  function pushTLE() {
    const idx = document.getElementById('sat-select').value;
    if(!filteredTleData[idx]) return;
    const btn = document.getElementById('btn-push');
    btn.innerText = "TRANSMITTING...";
    
    // Auto-switch to SGP4 mode when uploading a TLE
    document.getElementById('mode-select').value = "1";
    updateMode();

    fetch('/api/tle', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({name: filteredTleData[idx].name, line1: filteredTleData[idx].l1, line2: filteredTleData[idx].l2})
    }).then(() => {
      btn.innerText = "UPLOAD SUCCESSFUL";
      setTimeout(() => { btn.innerText = "2. UPLOAD TLE TO HARDWARE"; fetchPath(); }, 3000);
    });
  }

  function updateMode() { fetch('/api/config', { method:'POST', body: JSON.stringify({obMode: document.getElementById('mode-select').value === "1" ? true : false}) }); setTimeout(fetchPath, 1000); }
  function updateGrid() { const g = document.getElementById('grid-input').value; if(g) fetch('/api/config', { method:'POST', body: JSON.stringify({grid: g}) }); setTimeout(fetchPath, 1000); }
  
  function nudge(az, el) { 
    // AUTO-KICK OUT OF SGP4 MODE TO ALLOW MANUAL CONTROL
    if (document.getElementById('mode-select').value === "1") {
      document.getElementById('mode-select').value = "0";
      updateMode();
    }
    targetAz = (targetAz + az + 360) % 360; 
    targetEl = Math.max(0, Math.min(90, targetEl + el)); 
    fetch('/api/manual', { method:'POST', body: JSON.stringify({az: targetAz, el: targetEl}) }); 
  }

  function animateRadar() {
    requestAnimationFrame(animateRadar);
    const canvas = document.getElementById('radar');
    const ctx = canvas.getContext('2d');
    const cx = canvas.width / 2, cy = canvas.height / 2, r = cx - 25;

    ctx.clearRect(0,0,canvas.width,canvas.height);

    const toXY = (az, el) => {
      let rr = r * (1 - el/90);
      let rad = (az - 90) * Math.PI / 180;
      return {x: cx + rr*Math.cos(rad), y: cy + rr*Math.sin(rad)};
    };

    // Draw SGP4 Trajectory Path (Background)
    if (satPath.length > 0) {
      ctx.strokeStyle = 'rgba(16, 185, 129, 0.8)'; 
      ctx.lineWidth = 2;
      ctx.setLineDash([4, 4]); 
      ctx.beginPath();
      satPath.forEach((pt, i) => {
        let xy = toXY(pt.az, pt.el);
        if (i === 0) ctx.moveTo(xy.x, xy.y);
        else ctx.lineTo(xy.x, xy.y);
      });
      ctx.stroke();
      ctx.setLineDash([]); 

      let aos = toXY(satPath[0].az, satPath[0].el);
      let los = toXY(satPath[satPath.length-1].az, satPath[satPath.length-1].el);
      ctx.fillStyle = '#10b981'; ctx.font = '10px "Share Tech Mono"'; ctx.textAlign = 'center';
      ctx.fillText('AOS', aos.x, aos.y - 8);
      ctx.fillStyle = '#ef4444';
      ctx.fillText('LOS', los.x, los.y - 8);
    }

    // Compass Rings & Crosshairs
    ctx.strokeStyle = '#1e293b'; ctx.lineWidth = 1;
    [0.33, 0.66, 1].forEach(fac => {
      ctx.beginPath(); ctx.arc(cx, cy, r*fac, 0, 2*Math.PI); ctx.stroke();
    });
    ctx.beginPath(); ctx.moveTo(cx, cy-r); ctx.lineTo(cx, cy+r); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(cx-r, cy); ctx.lineTo(cx+r, cy); ctx.stroke();

    // Explicit Elevation Labels for the rings
    ctx.fillStyle = '#475569';
    ctx.textAlign = 'left';
    ctx.fillText('60°', cx + 2, cy - (r*0.33) + 8);
    ctx.fillText('30°', cx + 2, cy - (r*0.66) + 8);

    // Degree Ticks & Compass Labels
    ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
    for(let i=0; i<360; i+=30) {
      let rad = (i-90) * Math.PI / 180;
      if (i % 90 === 0) {
        ctx.strokeStyle = '#38bdf8'; ctx.lineWidth = 2; 
      } else {
        ctx.strokeStyle = '#475569'; ctx.lineWidth = 1;
      }
      ctx.beginPath(); 
      ctx.moveTo(cx + r*Math.cos(rad), cy + r*Math.sin(rad));
      ctx.lineTo(cx + (r+5)*Math.cos(rad), cy + (r+5)*Math.sin(rad));
      ctx.stroke();
      
      ctx.font = (i % 90 === 0) ? 'bold 13px "Share Tech Mono"' : '10px "Share Tech Mono"';
      ctx.fillStyle = (i % 90 === 0) ? '#38bdf8' : '#64748b'; 

      if(i === 0) ctx.fillText('N', cx, cy - r - 15);
      else if(i === 90) ctx.fillText('E', cx + r + 15, cy);
      else if(i === 180) ctx.fillText('S', cx, cy + r + 15);
      else if(i === 270) ctx.fillText('W', cx - r - 15, cy);
      else {
        ctx.fillText(i.toString(), cx + (r+15)*Math.cos(rad), cy + (r+15)*Math.sin(rad));
      }
    }

    // Sweeping Radar Beam Animation
    sweepAngle += 0.03;
    ctx.fillStyle = 'rgba(34, 211, 238, 0.15)';
    ctx.beginPath();
    ctx.moveTo(cx,cy);
    ctx.arc(cx, cy, r, sweepAngle, sweepAngle + 0.6);
    ctx.lineTo(cx,cy);
    ctx.fill();

    // Antenna Reticle (A sleek crosshair instead of a line to the center)
    let pAnt = toXY(currentAz, currentEl);
    ctx.strokeStyle = '#0284c7'; ctx.lineWidth=2;
    ctx.beginPath(); ctx.arc(pAnt.x, pAnt.y, 8, 0, 2*Math.PI); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(pAnt.x - 12, pAnt.y); ctx.lineTo(pAnt.x + 12, pAnt.y); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(pAnt.x, pAnt.y - 12); ctx.lineTo(pAnt.x, pAnt.y + 12); ctx.stroke();

    // Target Satellite Position Blip
    if(targetEl > 0 || currentEl > 0) {
      let pSat = toXY(targetAz, targetEl);
      ctx.fillStyle = '#22d3ee';
      ctx.shadowBlur = 15; ctx.shadowColor = '#22d3ee';
      ctx.beginPath(); ctx.arc(pSat.x, pSat.y, 6, 0, 2*Math.PI); ctx.fill();
      ctx.shadowBlur = 0;
    }
  }

  setInterval(fetchTelemetry, 500);
  setInterval(fetchPath, 60000); 
  setTimeout(fetchPath, 2000);   
  animateRadar(); 
</script>
</body>
</html>
)rawliteral";

// ==========================================
// 2. MAIN FIRMWARE
// ==========================================



#define AZ_STEP     32
#define AZ_DIR      14
#define EL_STEP     27
#define EL_DIR      26
#define ENABLE_PIN  25   
#define OLED_SDA    21   
#define OLED_SCL    22   
#define BOOT_BUTTON  0   

Adafruit_SSD1306 display(128, 64, &Wire, -1);
AccelStepper azStepper(AccelStepper::DRIVER, AZ_STEP, AZ_DIR);
AccelStepper elStepper(AccelStepper::DRIVER, EL_STEP, EL_DIR);

AsyncWebServer  webServer(80);
WiFiServer      tcpServer(4533);
WiFiClient      tcpClient;
WiFiUDP         ntpUDP;
NTPClient       timeClient(ntpUDP, "pool.ntp.org", 0, 60000);
Preferences     prefs;
Sgp4            sat;

char tleLine1[70] = "";
char tleLine2[70] = "";
char satName[25]  = "NONE";
bool tleLoaded    = false;

float currentAz = 0.0, currentEl = 0.0;
float targetAz  = 0.0, targetEl  = 0.0;
bool  isMoving    = false;

const float STEPS_PER_DEGREE_AZ = 8.88;
const float STEPS_PER_DEGREE_EL = 8.88;

bool onboardMode  = false;
double obsLat = 31.52, obsLon = 74.35, obsAlt = 210.0; 
String maidenhead = "MM71dl"; 

bool ntpSynced = false;
unsigned long lastSGP4Update = 0;

struct PathPoint { float az, el; };
PathPoint globalPath[90];
int globalPathLen = 0;
unsigned long lastPathCalc = 0;

unsigned long lastPageSwitch  = 0;
unsigned long lastButtonPress = 0;
int  currentPage              = 0;
const int TOTAL_PAGES         = 5; 
bool autoScroll               = true;
int  slewFrame                = 0;
unsigned long lastSlewFrame   = 0;
const char* slewFrames[]      = {"|", "/", "-", "\\"};
bool lastButtonState          = HIGH;

TaskHandle_t Core0Task;

void parseEasyComm(String cmd);
void updateOLED();
void runSGP4();
void calculatePathPrediction();
void oledPageTracking();
void oledPageNetwork();
void oledPageMotors();
void oledPageStatus();
void oledPageRadar();

void maidenheadToLatLon(String grid, double &lat, double &lon) {
  grid.toUpperCase();
  if (grid.length() < 4) return;
  lon = (grid[0] - 'A') * 20.0 - 180.0;
  lat = (grid[1] - 'A') * 10.0 - 90.0;
  lon += (grid[2] - '0') * 2.0;
  lat += (grid[3] - '0') * 1.0;
  if (grid.length() >= 6) {
    lon += ((tolower(grid[4]) - 'a') * 5.0) / 60.0;
    lat += ((tolower(grid[5]) - 'a') * 2.5) / 60.0;
  }
  lon += 1.0; lat += 0.5;
}

String buildTelemetryJson() {
  StaticJsonDocument<512> doc;
  doc["tAz"] = targetAz; doc["tEl"] = targetEl;
  doc["cAz"] = currentAz; doc["cEl"] = currentEl;
  doc["isMoving"] = isMoving;
  doc["rssi"] = WiFi.RSSI();
  doc["freeHeap"] = ESP.getFreeHeap() / 1024;
  doc["uptime"] = millis() / 1000;
  doc["mode"] = onboardMode ? 1 : 0;
  doc["sat"] = satName;
  doc["grid"] = maidenhead;
  String out; serializeJson(doc, out);
  return out;
}

void setupWebServer() {
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  webServer.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", buildTelemetryJson());
  });

  webServer.on("/api/path", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!tleLoaded || !ntpSynced || !onboardMode) {
      request->send(200, "application/json", "[]");
      return;
    }
    String json = "[";
    for (int i=0; i < globalPathLen; i++) {
      if (i > 0) json += ",";
      json += "{\"az\":" + String(globalPath[i].az, 1) + ",\"el\":" + String(globalPath[i].el, 1) + "}";
    }
    json += "]";
    request->send(200, "application/json", json);
  });

  webServer.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    StaticJsonDocument<256> doc;
    String body = ""; for(size_t i=0; i<len; i++) body += (char)data[i];
    if (!deserializeJson(doc, body)) {
      if (doc.containsKey("grid")) {
        maidenhead = doc["grid"].as<String>();
        maidenheadToLatLon(maidenhead, obsLat, obsLon);
        prefs.begin("sattracker", false); prefs.putString("grid", maidenhead); prefs.end();
        lastPathCalc = 0; 
      }
      if (doc.containsKey("obMode")) {
        onboardMode = doc["obMode"];
        prefs.begin("sattracker", false); prefs.putBool("obMode", onboardMode); prefs.end();
        lastPathCalc = 0;
      }
    }
  });

  webServer.on("/api/tle", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    StaticJsonDocument<512> doc;
    String body = ""; for(size_t i=0; i<len; i++) body += (char)data[i];
    if (!deserializeJson(doc, body)) {
      String name = doc["name"] | "UNKNOWN";
      String l1   = doc["line1"] | "";
      String l2   = doc["line2"] | "";
      File f = LittleFS.open("/tle.txt", "w");
      if (f) {
        f.println(name); f.println(l1); f.println(l2); f.close();
        name.toCharArray(satName, 25); l1.toCharArray(tleLine1, 70); l2.toCharArray(tleLine2, 70);
        sat.init(satName, tleLine1, tleLine2);
        tleLoaded = true;
        lastPathCalc = 0; 
      }
    }
  });

  webServer.on("/api/manual", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    StaticJsonDocument<128> doc;
    String body = ""; for(size_t i=0; i<len; i++) body += (char)data[i];
    if (!deserializeJson(doc, body)) {
      if (!onboardMode) {
        if (doc.containsKey("az")) targetAz = doc["az"].as<float>();
        if (doc.containsKey("el")) targetEl = doc["el"].as<float>();
      }
    }
  });

  webServer.begin();
}

void calculatePathPrediction() {
  if (!tleLoaded || !ntpSynced || !onboardMode) {
    globalPathLen = 0;
    return;
  }
  
  Sgp4 pathSat;
  pathSat.site(obsLat, obsLon, obsAlt);
  pathSat.init(satName, tleLine1, tleLine2);

  unsigned long t = timeClient.getEpochTime();
  bool inPass = false;

  pathSat.findsat(t);
  if (pathSat.satEl > 0) {
    inPass = true;
    while (pathSat.satEl > 0) { t -= 60; pathSat.findsat(t); }
    t += 60; 
  } else {
    for (int i=0; i < 1440; i+=2) {
      t += 120;
      pathSat.findsat(t);
      if (pathSat.satEl > 0) { inPass = true; break; }
      if (i % 60 == 0) yield();
    }
  }

  globalPathLen = 0;
  if (inPass) {
    for (int i=0; i < 90; i++) {
      pathSat.findsat(t);
      if (pathSat.satEl < 0 && i > 0) break;
      if (pathSat.satEl >= 0) {
        globalPath[globalPathLen].az = pathSat.satAz;
        globalPath[globalPathLen].el = pathSat.satEl;
        globalPathLen++;
      }
      t += 60;
    }
  }
}

void Core0TaskCode(void * pvParameters) {
  for(;;) {
    if (WiFi.status() == WL_CONNECTED && !ntpSynced) {
      if (timeClient.update()) {
        ntpSynced = true;
        Serial.println("[NTP] UTC Time Synchronized");
      }
    }

    if (!tcpClient || !tcpClient.connected()) {
      tcpClient = tcpServer.available();
    }

    if (tcpClient && tcpClient.available()) {
      tcpClient.setTimeout(10); 
      String cmd = tcpClient.readStringUntil('\n');
      if (cmd.length() > 0) {
        if (!onboardMode) parseEasyComm(cmd);
        tcpClient.println("OK");
      }
    }

    if (millis() - lastPathCalc > 60000) {
      calculatePathPrediction();
      lastPathCalc = millis();
    }

    static unsigned long lastOled = 0;
    if (millis() - lastOled > 250) {
      updateOLED();
      lastOled = millis();
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(BOOT_BUTTON, INPUT_PULLUP);
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, HIGH); 

  azStepper.setMaxSpeed(2000.0); azStepper.setAcceleration(1000.0);
  elStepper.setMaxSpeed(2000.0); elStepper.setAcceleration(1000.0);

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000);
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
    display.setCursor(20, 10); display.println("SAT TRACKER");
    display.setCursor(10, 25); display.println("Booting up..."); display.display();
  }

  LittleFS.begin(true);

  prefs.begin("sattracker", true);
  maidenhead  = prefs.getString("grid", maidenhead);
  onboardMode = prefs.getBool("obMode", false);
  prefs.end();
  maidenheadToLatLon(maidenhead, obsLat, obsLon);

  if (LittleFS.exists("/tle.txt")) {
    File f = LittleFS.open("/tle.txt", "r");
    if (f) {
      String n = f.readStringUntil('\n'); n.trim(); n.toCharArray(satName, 25);
      String l1 = f.readStringUntil('\n'); l1.trim(); l1.toCharArray(tleLine1, 70);
      String l2 = f.readStringUntil('\n'); l2.trim(); l2.toCharArray(tleLine2, 70);
      f.close();
      sat.init(satName, tleLine1, tleLine2);
      tleLoaded = true;
    }
  }

  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  Serial.println("\n[WIFI] Connected! -> IP: http://" + WiFi.localIP().toString());

  timeClient.begin();
  setupWebServer();
  tcpServer.begin();

  digitalWrite(ENABLE_PIN, LOW); 

  xTaskCreatePinnedToCore(Core0TaskCode, "Core0Task", 8192, NULL, 1, &Core0Task, 0);
}

void loop() {
  bool state = digitalRead(BOOT_BUTTON);
  if (state == LOW && lastButtonState == HIGH && millis() - lastButtonPress > 200) {
    currentPage = (currentPage + 1) % TOTAL_PAGES;
    lastPageSwitch = millis();
    lastButtonPress = millis();
    autoScroll = false;
  }
  lastButtonState = state;

  targetEl = constrain(targetEl, 0.0, 90.0);
  targetAz = fmod(targetAz + 360.0, 360.0);

  azStepper.moveTo(targetAz * STEPS_PER_DEGREE_AZ);
  elStepper.moveTo(targetEl * STEPS_PER_DEGREE_EL);
  
  azStepper.run();
  elStepper.run();

  currentAz = azStepper.currentPosition() / STEPS_PER_DEGREE_AZ;
  currentEl = elStepper.currentPosition() / STEPS_PER_DEGREE_EL;
  isMoving = azStepper.isRunning() || elStepper.isRunning();

  if (onboardMode && tleLoaded && ntpSynced && millis() - lastSGP4Update > 1000) {
    runSGP4();
    lastSGP4Update = millis();
  }
}

void parseEasyComm(String cmd) {
  cmd.trim();
  if (cmd.length() < 2) return;
  
  if (cmd.startsWith("P ") || cmd.startsWith("p ")) {
    int fSpc = cmd.indexOf(" ");
    int sSpc = cmd.indexOf(" ", fSpc + 1);
    if (fSpc != -1 && sSpc != -1) {
      targetAz = cmd.substring(fSpc + 1, sSpc).toFloat();
      targetEl = cmd.substring(sSpc + 1).toFloat();
    }
  } else if (cmd.indexOf("AZ") != -1 || cmd.indexOf("az") != -1) {
    int azIdx = cmd.indexOf("AZ"); if (azIdx == -1) azIdx = cmd.indexOf("az");
    int elIdx = cmd.indexOf("EL"); if (elIdx == -1) elIdx = cmd.indexOf("el");
    if (azIdx != -1 && elIdx != -1) {
      targetAz = cmd.substring(azIdx + 2, elIdx).toFloat();
      int elEnd = cmd.indexOf(" ", elIdx); if (elEnd == -1) elEnd = cmd.length();
      targetEl = cmd.substring(elIdx + 2, elEnd).toFloat();
    }
  } else if (cmd == "p" || cmd == "P") {
    if (tcpClient) tcpClient.printf("AZ%.2f EL%.2f\n", currentAz, currentEl);
  }
}

void runSGP4() {
  unsigned long now = timeClient.getEpochTime();
  sat.site(obsLat, obsLon, obsAlt);
  sat.findsat(now);
  targetAz = sat.satAz;
  targetEl = sat.satEl;
}

void updateOLED() {
  if (millis() - lastSlewFrame > 150) {
    slewFrame = (slewFrame + 1) % 4;
    lastSlewFrame = millis();
  }
  if (autoScroll && millis() - lastPageSwitch > 5000) {
    currentPage = (currentPage + 1) % TOTAL_PAGES;
    lastPageSwitch = millis();
  }

  display.clearDisplay();
  switch (currentPage) {
    case 0: oledPageTracking(); break;
    case 1: oledPageRadar();    break;
    case 2: oledPageNetwork();  break;
    case 3: oledPageMotors();   break;
    case 4: oledPageStatus();   break;
  }

  int dotSpacing = 10;
  int startX = (128 - (TOTAL_PAGES * dotSpacing)) / 2;
  for (int i = 0; i < TOTAL_PAGES; i++) {
    if (i == currentPage) display.fillCircle(startX + (i * dotSpacing), 62, 2, SSD1306_WHITE);
    else display.drawCircle(startX + (i * dotSpacing), 62, 2, SSD1306_WHITE);
  }
  display.display();
}

void oledPageRadar() {
  int cx = 64, cy = 32, r = 26;
  
  display.drawCircle(cx, cy, r, SSD1306_WHITE);
  
  // Clean crosshair inside the radar circle
  display.drawLine(cx, cy - 2, cx, cy + 2, SSD1306_WHITE); 
  display.drawLine(cx - 2, cy, cx + 2, cy, SSD1306_WHITE); 
  
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(cx - 2, 0); display.print("N");
  display.setCursor(cx - 2, 56); display.print("S");
  display.setCursor(cx - r - 8, cy - 3); display.print("W");
  display.setCursor(cx + r + 2, cy - 3); display.print("E");
  display.setCursor(0, 0); display.print("RAD");

  auto toXY = [cx, cy, r](float az, float el, int &x, int &y) {
    float rr = r * (1.0 - el/90.0);
    float rad = (az - 90.0) * PI / 180.0;
    x = cx + (int)(rr * cos(rad));
    y = cy + (int)(rr * sin(rad));
  };
  
  // Removed the messy trajectory path from OLED. 
  
  // Antenna Vector (Clean pointer)
  int antX, antY;
  toXY(currentAz, currentEl, antX, antY);
  display.drawLine(cx, cy, antX, antY, SSD1306_WHITE);
  display.fillCircle(antX, antY, 2, SSD1306_WHITE);
  
  // Target Sat Blip
  if (targetEl > 0 || currentEl > 0) {
    int satX, satY;
    toXY(targetAz, targetEl, satX, satY);
    display.drawCircle(satX, satY, 4, SSD1306_WHITE);
  }
}

void oledPageTracking() {
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  if (isMoving) {
    display.fillRoundRect(72, 0, 55, 10, 2, SSD1306_WHITE); display.setTextColor(SSD1306_BLACK); 
    display.setCursor(74, 1); display.print(slewFrames[slewFrame]); display.print(" SLEW");
  } else {
    display.fillRoundRect(60, 0, 67, 10, 2, SSD1306_WHITE); display.setTextColor(SSD1306_BLACK); 
    display.setCursor(62, 1); display.print("* TRACKING");
  }
  display.setTextColor(SSD1306_WHITE); display.setCursor(0, 2); display.print("AZ");
  display.setTextSize(2); display.setCursor(0, 12); display.print(currentAz, 1); display.setTextSize(1); display.print((char)247);
  display.drawLine(0, 31, 128, 31, SSD1306_WHITE);
  display.setCursor(0, 34); display.print("EL");
  display.setTextSize(2); display.setCursor(0, 44); display.print(currentEl, 1); display.setTextSize(1); display.print((char)247);
}

void oledPageNetwork() {
  display.setTextSize(1); display.setCursor(0, 0);
  display.println(" -- NETWORK -- "); display.drawLine(0, 9, 128, 9, SSD1306_WHITE);
  display.setCursor(0, 12); display.print("WiFi: "); display.println(WiFi.status() == WL_CONNECTED ? "CONNECTED" : "OFFLINE");
  display.setCursor(0, 22); display.print("IP: "); display.println(WiFi.localIP().toString());
  display.setCursor(0, 32); display.print("RSSI: "); display.print(WiFi.RSSI()); display.println(" dBm");
  display.setCursor(0, 42); display.print("Look4Sat: "); display.println((tcpClient && tcpClient.connected()) ? "LINKED" : "WAITING");
  display.setCursor(0, 52); display.print("Mode: "); display.println(onboardMode ? "SGP4 Internal" : "External");
}

void oledPageMotors() {
  display.setTextSize(1); display.setCursor(0, 0);
  if (isMoving) { display.print(slewFrames[slewFrame]); display.print(" MOTORS "); display.println(slewFrames[slewFrame]); } 
  else { display.println(" -- MOTORS -- "); }
  display.drawLine(0, 9, 128, 9, SSD1306_WHITE);
  display.setCursor(0, 12); display.println("AZIMUTH:"); display.setCursor(0, 21); display.print(" Pos: "); display.print(azStepper.currentPosition()); display.println(" stp");
  display.setCursor(0, 30); display.print(" Spd: "); display.print(abs(azStepper.speed()), 0); display.println(" sps");
  display.drawLine(0, 39, 128, 39, SSD1306_WHITE);
  display.setCursor(0, 41); display.println("ELEVATION:"); display.setCursor(0, 50); display.print(" Pos: "); display.print(elStepper.currentPosition()); display.println(" stp");
}

void oledPageStatus() {
  display.setTextSize(1); display.setCursor(0, 0);
  display.println(" -- SYSTEM -- "); display.drawLine(0, 9, 128, 9, SSD1306_WHITE);
  unsigned long s = millis() / 1000; unsigned long m = s / 60, h = m / 60;
  display.setCursor(0, 12); display.print("Up: "); display.print(h); display.print("h "); display.print(m % 60); display.print("m "); display.print(s % 60); display.println("s");
  display.setCursor(0, 22); display.print("RAM: "); display.print(ESP.getFreeHeap() / 1024); display.println(" KB");
  display.setCursor(0, 32); display.print("Mode: "); display.println(onboardMode ? "SGP4" : "Look4Sat");
  display.setCursor(0, 42); display.print("SAT: "); String satStr = String(satName); if (satStr.length() > 13) satStr = satStr.substring(0, 11) + ".."; display.println(satStr);
  display.setCursor(0, 52); display.print("NTP: "); display.println(ntpSynced ? "SYNCED" : "NO SYNC");
}