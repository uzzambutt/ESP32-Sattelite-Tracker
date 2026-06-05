#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AccelStepper.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h> 
#include <LittleFS.h>
#include <Sgp4.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <esp_task_wdt.h>
#include "secrets.h"

// =============================================
//   PROGMEM AEROSPACE DASHBOARD
// =============================================
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
    h1 { font-family: 'Share Tech Mono', monospace; font-size: 1.8rem; color: #22d3ee; letter-spacing: 4px; margin: 15px 0 5px 0; text-align: center; text-shadow: 0 0 10px rgba(34,211,238,0.3); }
    .subtitle { font-size: 0.7rem; color: #64748b; letter-spacing: 3px; margin-bottom: 25px; text-align: center; }
    .badge-container { display: flex; gap: 10px; margin-bottom: 25px; flex-wrap: wrap; justify-content: center; }
    .status-badge { padding: 6px 16px; border-radius: 4px; font-size: 0.75rem; font-weight: 600; letter-spacing: 2px; text-transform: uppercase; border: 1px solid transparent; }
    .status-tracking { background: rgba(16,185,129,0.1); color: #10b981; border-color: rgba(16,185,129,0.3); }
    .status-offline  { background: rgba(239,68,68,0.1);  color: #ef4444; border-color: rgba(239,68,68,0.3); }
    .status-mode     { background: rgba(34,211,238,0.1); color: #22d3ee; border-color: rgba(34,211,238,0.3); }
    .container { display: grid; grid-template-columns: 1fr; gap: 20px; width: 100%; max-width: 520px; }
    .panel { background: #0f172a; border: 1px solid #1e293b; border-radius: 8px; padding: 20px; }
    .panel-header { font-size: 0.75rem; color: #94a3b8; letter-spacing: 2px; text-transform: uppercase; margin-bottom: 15px; border-bottom: 1px solid #1e293b; padding-bottom: 8px; }
    .telemetry-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin-bottom: 20px; }
    .data-card { background: #020617; border: 1px solid #1e293b; border-radius: 6px; padding: 15px; text-align: center; position: relative; }
    .data-card::before { content: ''; position: absolute; top: 0; left: 0; right: 0; height: 2px; background: #22d3ee; }
    .data-label { font-size: 0.65rem; letter-spacing: 2px; color: #64748b; text-transform: uppercase; margin-bottom: 5px; }
    .data-value { font-family: 'Share Tech Mono', monospace; font-size: 2.5rem; color: #f8fafc; }
    .data-unit  { font-size: 1rem; color: #22d3ee; vertical-align: super; }
    .info-row { display: flex; justify-content: space-between; margin-bottom: 8px; font-size: 0.85rem; border-bottom: 1px dashed #1e293b; padding-bottom: 4px; }
    .info-key { color: #64748b; }
    .info-val { font-family: 'Share Tech Mono', monospace; color: #cbd5e1; }
    .highlight { color: #22d3ee; }
    input, select, button { width: 100%; padding: 12px; margin-bottom: 12px; background: #020617; border: 1px solid #1e293b; color: #e2e8f0; border-radius: 4px; font-size: 0.85rem; outline: none; }
    input:focus, select:focus { border-color: #22d3ee; }
    button { background: rgba(34,211,238,0.1); color: #22d3ee; border: 1px solid #22d3ee; cursor: pointer; font-weight: 600; letter-spacing: 1px; text-transform: uppercase; transition: 0.2s; }
    button:hover { background: #22d3ee; color: #020617; }
    .input-group { display: flex; gap: 8px; }
    .input-group input { margin-bottom: 0; }
    .input-group button { margin-bottom: 0; width: auto; flex-shrink: 0; }
    .radar-wrapper { display: flex; justify-content: center; margin: 20px 0; }
    canvas { background: #020617; border-radius: 50%; }
    .joystick { display: grid; grid-template-columns: repeat(3, 50px); gap: 6px; justify-content: center; margin-top: 15px; }
    .joystick button { padding: 15px 0; margin: 0; background: #0f172a; border: 1px solid #1e293b; color: #94a3b8; }
    .joystick button:hover { background: #22d3ee; color: #020617; }
    .sat-filter { width: 100%; padding: 10px; margin-bottom: 8px; background: #020617; border: 1px solid #22d3ee; color: #22d3ee; border-radius: 4px; font-size: 0.85rem; outline: none; }
    .sat-filter::placeholder { color: rgba(34,211,238,0.4); }
    .conn-lost { display: none; position: fixed; top: 0; left: 0; width: 100%; background: #ef4444; color: white; text-align: center; padding: 8px; font-size: 0.8rem; font-weight: bold; letter-spacing: 2px; z-index: 999; }
  </style>
</head>
<body>
  <div class="conn-lost" id="connLost">⚠ LINK LOST — REESTABLISHING...</div>
  <h1>AEROSPACE CMD</h1>
  <p class="subtitle">ESP32 GROUND STATION</p>
  <div class="badge-container">
    <div class="status-badge status-offline" id="statusBadge">CONNECTING...</div>
    <div class="status-badge status-mode"    id="modeBadge">--</div>
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
      <div class="info-row"><span class="info-key">Doppler Shift</span><span class="info-val highlight" id="doppler">-- Hz</span></div>
      <div class="info-row"><span class="info-key">Distance</span><span class="info-val" id="satDist">-- km</span></div>
      <div class="info-row"><span class="info-key">System Uptime</span><span class="info-val" id="uptime">--</span></div>
      <div class="info-row" style="border:none;margin-bottom:0"><span class="info-key">Free Memory</span><span class="info-val" id="ram">-- KB</span></div>
    </div>

    <div class="panel">
      <div class="panel-header">Tactical Radar</div>
      <div class="radar-wrapper">
        <canvas id="radar" width="280" height="280"></canvas>
      </div>
      <div class="joystick">
        <div></div><button onclick="nudge(0,5)">▲</button><div></div>
        <button onclick="nudge(-5,0)">◀</button><div></div><button onclick="nudge(5,0)">▶</button>
        <div></div><button onclick="nudge(0,-5)">▼</button><div></div>
      </div>
    </div>

    <div class="panel">
      <div class="panel-header">Orbital Database — SGP4</div>
      <div class="info-row" style="border:none;margin-bottom:12px;">
        <span class="info-key" style="line-height:2.5">Tracking Engine</span>
        <select id="mode-select" onchange="updateMode()" style="width:160px;margin:0">
          <option value="0">External (Look4Sat)</option>
          <option value="1">Internal SGP4</option>
        </select>
      </div>
      <div class="input-group">
        <input type="text" id="grid-input" placeholder="Observer Grid (e.g. MM71dl)" maxlength="6">
        <button onclick="updateGrid()">SET GRID</button>
      </div>
      <button onclick="fetchCelestrak()" id="btn-fetch">1. DOWNLOAD CELESTRAK DB</button>
      <input type="text" id="sat-filter" class="sat-filter" placeholder="🔍 Filter satellites (e.g. NOAA, ISS)..." oninput="filterSats()" disabled>
      <select id="sat-select" size="4" style="height:80px"><option>Awaiting Database...</option></select>
      <button onclick="pushTLE()" id="btn-push">2. UPLOAD TLE TO HARDWARE</button>
    </div>

    <div class="panel">
      <div class="panel-header">Pass Log</div>
      <button onclick="loadPassLog()" style="margin-bottom:12px">REFRESH PASS LOG</button>
      <div id="passLogBody" style="font-family:monospace;font-size:0.8rem;color:#94a3b8">
        No passes logged yet.
      </div>
    </div>

  </div>
<script>
  let currentAz=0, currentEl=0, targetAz=0, targetEl=0;
  let allTleData=[], tleData=[];
  let satPath=[], satDist=0;
  let failCount=0, sweepAngle=0;

  async function fetchTelemetry() {
    try {
      const r = await fetch('/api/status', {cache:'no-store'});
      if (!r.ok) throw new Error();
      const d = await r.json();
      failCount=0;
      document.getElementById('connLost').style.display='none';
      currentAz=d.cAz; currentEl=d.cEl;
      targetAz=d.tAz;  targetEl=d.tEl;
      satDist = d.dist || 0;
      document.getElementById('curAz').innerHTML = currentAz.toFixed(1).padStart(5,'0')+'<span class="data-unit">°</span>';
      document.getElementById('curEl').innerHTML = currentEl.toFixed(1).padStart(4,'0')+'<span class="data-unit">°</span>';
      document.getElementById('tgtAz').innerText  = targetAz.toFixed(1)+'°';
      document.getElementById('tgtEl').innerText  = targetEl.toFixed(1)+'°';
      document.getElementById('satName').innerText = d.sat;
      document.getElementById('ram').innerText     = d.freeHeap+' KB';
      document.getElementById('doppler').innerText = (d.doppler||0).toFixed(0)+' Hz';
      document.getElementById('satDist').innerText = (d.dist||0).toFixed(0)+' km';
      const up=d.uptime;
      document.getElementById('uptime').innerText=`${Math.floor(up/3600)}h ${Math.floor((up%3600)/60)}m ${up%60}s`;
      document.getElementById('grid-input').placeholder=d.grid;
      document.getElementById('mode-select').value=d.mode===1?"1":"0";
      const badge=document.getElementById('statusBadge');
      badge.className='status-badge status-tracking';
      badge.innerText = d.isMoving ? '⟳ SLEWING' : '● LINK ACTIVE';
      document.getElementById('modeBadge').innerText=d.mode===1?'SGP4: ONBOARD':'MODE: EXT L4S';
    } catch(e) {
      failCount++;
      if(failCount>3){
        document.getElementById('connLost').style.display='block';
        document.getElementById('statusBadge').className='status-badge status-offline';
        document.getElementById('statusBadge').innerText='✕ OFFLINE';
      }
    }
  }

  async function fetchPath() {
    try {
      const r=await fetch('/api/path');
      if(r.ok) satPath=await r.json();
    } catch(e){}
  }

  async function loadPassLog() {
    try {
      const r=await fetch('/api/passlog');
      const d=await r.json();
      const el=document.getElementById('passLogBody');
      if(!d.length){ el.innerText='No passes logged yet.'; return; }
      el.innerHTML=d.reverse().map(p=>
        `<div style="margin-bottom:6px;border-bottom:1px solid #1e293b;padding-bottom:4px">
          <span style="color:#22d3ee">${p.sat}</span> &nbsp;
          <span style="color:#64748b">${p.time}</span> &nbsp;
          <span style="color:#10b981">MaxEl: ${parseFloat(p.maxEl).toFixed(1)}°</span>
        </div>`
      ).join('');
    } catch(e){ document.getElementById('passLogBody').innerText='Failed to load.'; }
  }

  function fetchCelestrak() {
    const btn=document.getElementById('btn-fetch');
    btn.innerText='DOWNLOADING...';
    fetch('https://celestrak.org/NORAD/elements/gp.php?GROUP=active&FORMAT=tle')
    .then(r=>r.text())
    .then(txt=>{
      allTleData=[];
      const lines=txt.split('\n');
      for(let i=0;i<lines.length-2;i+=3){
        const name=lines[i].trim();
        if(name.length>0 && lines[i+1].startsWith('1 ') && lines[i+2].startsWith('2 ')){
          if(!name.includes('STARLINK') && !name.includes('ONEWEB') && !name.includes('FLOCK'))
            allTleData.push({name, l1:lines[i+1].trim(), l2:lines[i+2].trim()});
        }
      }
      btn.innerText='DB LOADED ('+allTleData.length+' TARGETS)';
      btn.style.color='#10b981'; btn.style.borderColor='#10b981';
      document.getElementById('sat-filter').disabled=false;
      filterSats();
    }).catch(()=>{ btn.innerText='DOWNLOAD FAILED'; });
  }

  function filterSats() {
    const q=document.getElementById('sat-filter').value.toLowerCase();
    tleData = q ? allTleData.filter(s=>s.name.toLowerCase().includes(q)) : allTleData;
    const sel=document.getElementById('sat-select');
    sel.innerHTML=tleData.slice(0,200).map((s,i)=>`<option value="${i}">${s.name}</option>`).join('');
  }

  function pushTLE() {
    const idx=document.getElementById('sat-select').value;
    if(!tleData[idx]) return;
    const btn=document.getElementById('btn-push');
    btn.innerText='TRANSMITTING...';
    fetch('/api/tle',{
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body:JSON.stringify({name:tleData[idx].name, line1:tleData[idx].l1, line2:tleData[idx].l2})
    }).then(()=>{
      btn.innerText='UPLOAD SUCCESSFUL';
      document.getElementById('mode-select').value='1';
      updateMode();
      setTimeout(()=>{ btn.innerText='2. UPLOAD TLE TO HARDWARE'; fetchPath(); }, 3000);
    });
  }

  function updateMode() {
    fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},
      body:JSON.stringify({obMode:document.getElementById('mode-select').value==='1'})});
    setTimeout(fetchPath,1000);
  }
  function updateGrid() {
    const g=document.getElementById('grid-input').value;
    if(g) fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},
      body:JSON.stringify({grid:g})});
  }
  function nudge(az,el) {
    targetAz=(targetAz+az+360)%360;
    targetEl=Math.max(0,Math.min(90,targetEl+el));
    fetch('/api/manual',{method:'POST',headers:{'Content-Type':'application/json'},
      body:JSON.stringify({az:targetAz,el:targetEl})});
  }

  function animateRadar() {
    requestAnimationFrame(animateRadar);
    const cv=document.getElementById('radar');
    const ctx=cv.getContext('2d');
    const cx=cv.width/2, cy=cv.height/2, r=cx-22;
    ctx.clearRect(0,0,cv.width,cv.height);
    const toXY=(az,el)=>{
      const rr=r*(1-el/90);
      const rad=(az-90)*Math.PI/180;
      return {x:cx+rr*Math.cos(rad), y:cy+rr*Math.sin(rad)};
    };
    
    // Draw Web Radar Trajectory
    if(satPath.length>0){
      ctx.strokeStyle='rgba(16,185,129,0.7)'; ctx.lineWidth=2; ctx.setLineDash([4,4]);
      ctx.beginPath();
      satPath.forEach((pt,i)=>{
        const p=toXY(pt.az,pt.el);
        if(i===0) ctx.moveTo(p.x,p.y); else ctx.lineTo(p.x,p.y);
      });
      ctx.stroke(); ctx.setLineDash([]);
      const aos=toXY(satPath[0].az,satPath[0].el);
      const los=toXY(satPath[satPath.length-1].az,satPath[satPath.length-1].el);
      ctx.fillStyle='#10b981'; ctx.font='10px "Share Tech Mono"'; ctx.textAlign='center';
      ctx.fillText('AOS',aos.x,aos.y-8);
      ctx.fillStyle='#ef4444'; ctx.fillText('LOS',los.x,los.y-8);
    }
    
    ctx.strokeStyle='#1e293b'; ctx.lineWidth=1;
    [0.33,0.66,1].forEach(f=>{ ctx.beginPath(); ctx.arc(cx,cy,r*f,0,2*Math.PI); ctx.stroke(); });
    ctx.beginPath(); ctx.moveTo(cx,cy-r); ctx.lineTo(cx,cy+r); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(cx-r,cy); ctx.lineTo(cx+r,cy); ctx.stroke();
    ctx.textAlign='center'; ctx.textBaseline='middle';
    for(let i=0;i<360;i+=30){
      const rad=(i-90)*Math.PI/180;
      ctx.strokeStyle=i%90===0?'#38bdf8':'#475569'; ctx.lineWidth=i%90===0?2:1;
      ctx.beginPath(); ctx.moveTo(cx+r*Math.cos(rad),cy+r*Math.sin(rad));
      ctx.lineTo(cx+(r+5)*Math.cos(rad),cy+(r+5)*Math.sin(rad)); ctx.stroke();
      ctx.font=i%90===0?'bold 12px "Share Tech Mono"':'9px "Share Tech Mono"';
      ctx.fillStyle=i%90===0?'#38bdf8':'#64748b';
      if(i===0)   ctx.fillText('N',cx,cy-r-14);
      else if(i===90)  ctx.fillText('E',cx+r+14,cy);
      else if(i===180) ctx.fillText('S',cx,cy+r+14);
      else if(i===270) ctx.fillText('W',cx-r-14,cy);
      else ctx.fillText(i,cx+(r+15)*Math.cos(rad),cy+(r+15)*Math.sin(rad));
    }
    sweepAngle+=0.025;
    ctx.fillStyle='rgba(34,211,238,0.1)';
    ctx.beginPath(); ctx.moveTo(cx,cy); ctx.arc(cx,cy,r,sweepAngle,sweepAngle+0.55); ctx.lineTo(cx,cy); ctx.fill();
    const ap=toXY(currentAz,currentEl);
    ctx.strokeStyle='rgba(148,163,184,0.6)'; ctx.lineWidth=2;
    ctx.beginPath(); ctx.moveTo(cx,cy); ctx.lineTo(ap.x,ap.y); ctx.stroke();
    ctx.fillStyle='#0284c7';
    ctx.beginPath(); ctx.arc(ap.x,ap.y,5,0,2*Math.PI); ctx.fill();
    if(targetEl>0){
      const sp=toXY(targetAz,targetEl);
      ctx.fillStyle='#22d3ee'; ctx.shadowBlur=15; ctx.shadowColor='#22d3ee';
      ctx.beginPath(); ctx.arc(sp.x,sp.y,6,0,2*Math.PI); ctx.fill();
      ctx.shadowBlur=0;
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

// =============================================
//   PIN DEFINITIONS
// =============================================
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST   4

#define AZ_STEP  32
#define AZ_DIR   14
#define EL_STEP  27
#define EL_DIR   26
#define ENABLE_PIN 25

// =============================================
//   TFT COLOURS
// =============================================
#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_CYAN    0x07FF
#define C_GREEN   0x07E0
#define C_YELLOW  0xFFE0
#define C_ORANGE  0xFD20
#define C_RED     0xF800
#define C_BLUE    0x001F
#define C_DGRAY   0x2945 
#define C_MGRAY   0x4A69 
#define C_DBLUE   0x0811 

// =============================================
//   HARDWARE OBJECTS
// =============================================
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST); 
AccelStepper  azStepper(AccelStepper::DRIVER, AZ_STEP, AZ_DIR);
AccelStepper  elStepper(AccelStepper::DRIVER, EL_STEP, EL_DIR);
AsyncWebServer  webServer(80);
AsyncWebSocket  ws("/ws");
WiFiServer      tcpServer(4533);
WiFiClient      tcpClient;
WiFiUDP         ntpUDP;
NTPClient       timeClient(ntpUDP, "pool.ntp.org", 0, 60000);
Preferences     prefs;
Sgp4            sat;

// =============================================
//   STATE VARIABLES (VOLATILE FOR DUAL-CORE SYNC)
// =============================================
char tleLine1[70] = "";
char tleLine2[70] = "";
char satName[25]  = "NO SAT";
volatile bool tleLoaded = false;

volatile float currentAz = 0.0, currentEl = 0.0;
volatile float targetAz  = 0.0, targetEl  = 0.0;
volatile bool  isMoving  = false;

const float STEPS_PER_DEGREE_AZ = 8.88;
const float STEPS_PER_DEGREE_EL = 8.88;

volatile bool onboardMode = false;
double obsLat = 31.52, obsLon = 74.35, obsAlt = 0.21;
String maidenhead = "MM71dl";

volatile bool ntpSynced = false;
volatile double lastSatDist = 0;
volatile float  dopplerFreq = 0.0;
volatile float  satDistance = 0.0;
unsigned long lastDopplerTime = 0;
unsigned long lastSGP4Update = 0;

time_t nextAosTime = 0;
time_t nextLosTime = 0;
float  nextMaxEl   = 0.0;

struct PassLog { char sat[25]; char time[20]; float maxEl; };
PassLog passLog[10];
int  passLogCount    = 0;
bool passInProgress  = false;
float passMaxEl      = 0.0;
char  passStartBuf[20] = "";

struct PathPoint { float az, el; };
PathPoint globalPath[45];
volatile int globalPathLen  = 0;
unsigned long lastPathCalc = 0;

// TFT Redraw Tracking Cache
float  prev_cAz = -999, prev_cEl = -999;
float  prev_tAz = -999, prev_tEl = -999;
float  prev_doppler = -999;
float  prev_dist = -999;
int    prev_rssi = 0;
bool   prev_moving = false;
bool   prev_ntp = false;
bool   prev_l4s = false;
int    prev_mode = -1;
char   prev_sat[25] = "";
unsigned long prev_uptime = 0;
long   prev_countdown = -999;

int prev_antX = -1, prev_antY = -1;
int prev_satX = -1, prev_satY = -1;

TaskHandle_t Core0Task;

// =============================================
//   FORWARD DECLARATIONS
// =============================================
void parseEasyComm(String cmd);
void runSGP4();
void calculatePathPrediction();
void tftDrawStaticFrame();
void tftUpdateDynamic();
void tftRadarUpdate();
void setupWebServer();

// =============================================
//   MAIDENHEAD → LAT/LON
// =============================================
void maidenheadToLatLon(String grid, double &lat, double &lon) {
  grid.toUpperCase();
  if (grid.length() < 4) return;
  lon = (grid[0]-'A')*20.0 - 180.0;
  lat = (grid[1]-'A')*10.0 -  90.0;
  lon += (grid[2]-'0')*2.0;
  lat += (grid[3]-'0')*1.0;
  if (grid.length() >= 6) {
    lon += ((tolower(grid[4])-'a')*5.0)/60.0;
    lat += ((tolower(grid[5])-'a')*2.5)/60.0;
  }
  lon += 1.0; lat += 0.5;
}

// =============================================
//   BUILD TELEMETRY JSON
// =============================================
String buildTelemetryJson() {
  StaticJsonDocument<512> doc;
  doc["tAz"]      = (float)targetAz;
  doc["tEl"]      = (float)targetEl;
  doc["cAz"]      = (float)currentAz;
  doc["cEl"]      = (float)currentEl;
  doc["isMoving"] = isMoving;
  doc["rssi"]     = WiFi.RSSI();
  doc["freeHeap"] = ESP.getFreeHeap() / 1024;
  doc["uptime"]   = millis() / 1000;
  doc["mode"]     = onboardMode ? 1 : 0;
  doc["sat"]      = satName;
  doc["grid"]     = maidenhead;
  doc["doppler"]  = (float)dopplerFreq;
  doc["dist"]     = (float)satDistance;
  String out;
  serializeJson(doc, out);
  return out;
}

// =============================================
//   TFT — DRAW STATIC FRAME
// =============================================
void tftDrawStaticFrame() {
  tft.fillScreen(C_BLACK);
  
  tft.fillRect(0, 0, 240, 25, C_DBLUE);
  tft.setTextColor(C_CYAN); tft.setTextSize(1);
  tft.setCursor(5, 8);
  tft.print("AEROSPACE CMD TELEMETRY");
  tft.drawFastHLine(0, 25, 240, C_DGRAY);

  tft.setTextColor(C_MGRAY); tft.setTextSize(1);
  tft.setCursor(5, 30);   tft.print("TARGET:");
  
  tft.setCursor(5, 55);   tft.print("CUR AZ:");
  tft.setCursor(120, 55); tft.print("CUR EL:");
  
  tft.setCursor(5, 85);   tft.print("TGT AZ:");
  tft.setCursor(120, 85); tft.print("TGT EL:");
  
  tft.drawFastHLine(0, 110, 240, C_DGRAY);
  
  tft.setCursor(5, 115);  tft.print("MODE:");
  tft.setCursor(5, 127);  tft.print("NTP:");
  tft.setCursor(5, 139);  tft.print("L4S:");
  tft.setCursor(120, 115); tft.print("RSSI:");
  tft.setCursor(120, 127); tft.print("RAM:");
  tft.setCursor(120, 139); tft.print("UP:");
  tft.setCursor(5, 151);  tft.print("DOPP:");
  tft.setCursor(120, 151); tft.print("DIST:");

  tft.drawFastHLine(0, 165, 240, C_DGRAY);
  
  tft.setTextColor(C_CYAN); 
  tft.setCursor(5, 170); tft.print("NEXT AOS (UTC):");
  tft.setCursor(150, 170); tft.print("MAX EL:");
  tft.setCursor(5, 190); tft.print("NEXT LOS (UTC):");
  tft.setCursor(150, 190); tft.print("T-MINUS:");

  int cx=120, cy=265, r=50;
  tft.drawCircle(cx, cy, r,    C_DGRAY);
  tft.drawCircle(cx, cy, r*2/3, C_DGRAY);
  tft.drawCircle(cx, cy, r*1/3, C_DGRAY);
  tft.drawFastVLine(cx, cy-r, r*2,   C_DGRAY);
  tft.drawFastHLine(cx-r, cy, r*2+1, C_DGRAY);
  
  tft.setTextColor(C_CYAN); tft.setTextSize(1);
  tft.setCursor(cx-3, cy-r-10); tft.print("N");
  tft.setCursor(cx-3, cy+r+2);  tft.print("S");
  tft.setCursor(cx+r+3, cy-3);  tft.print("E");
  tft.setCursor(cx-r-9, cy-3);  tft.print("W");
}

// =============================================
//   TFT — DYNAMIC TEXT UPDATE
// =============================================
void tftUpdateDynamic() {
  char buf[32]; 

  if (strcmp(satName, prev_sat) != 0) {
    tft.fillRect(5, 40, 230, 15, C_BLACK);
    tft.setTextColor(C_YELLOW); tft.setTextSize(1);
    tft.setCursor(5, 40);
    snprintf(buf, sizeof(buf), "%.25s", satName); 
    tft.print(buf);
    strncpy(prev_sat, satName, 24);
  }

  float cAz = currentAz;
  if (fabsf(cAz - prev_cAz) > 0.05f) {
    tft.fillRect(55, 53, 60, 16, C_BLACK);
    tft.setTextColor(C_WHITE); tft.setTextSize(2);
    tft.setCursor(55, 53);
    snprintf(buf, sizeof(buf), "%05.1f", (double)cAz);
    tft.print(buf);
    prev_cAz = cAz;
  }

  float cEl = currentEl;
  if (fabsf(cEl - prev_cEl) > 0.05f) {
    tft.fillRect(170, 53, 60, 16, C_BLACK);
    tft.setTextColor(C_WHITE); tft.setTextSize(2);
    tft.setCursor(170, 53);
    snprintf(buf, sizeof(buf), "%04.1f", (double)cEl);
    tft.print(buf);
    prev_cEl = cEl;
  }

  float tAz = targetAz;
  if (fabsf(tAz - prev_tAz) > 0.05f) {
    tft.fillRect(55, 83, 60, 16, C_BLACK);
    tft.setTextColor(C_YELLOW); tft.setTextSize(2);
    tft.setCursor(55, 83);
    snprintf(buf, sizeof(buf), "%05.1f", (double)tAz);
    tft.print(buf);
    prev_tAz = tAz;
  }

  float tEl = targetEl;
  if (fabsf(tEl - prev_tEl) > 0.05f) {
    tft.fillRect(170, 83, 60, 16, C_BLACK);
    tft.setTextColor(C_YELLOW); tft.setTextSize(2);
    tft.setCursor(170, 83);
    snprintf(buf, sizeof(buf), "%04.1f", (double)tEl);
    tft.print(buf);
    prev_tEl = tEl;
  }

  int modeVal = onboardMode ? 1 : 0;
  if (modeVal != prev_mode) {
    tft.fillRect(40, 115, 75, 9, C_BLACK);
    tft.setTextColor(onboardMode ? C_CYAN : C_YELLOW);
    tft.setTextSize(1); tft.setCursor(40, 115);
    tft.print(onboardMode ? "SGP4" : "EXT");
    prev_mode = modeVal;
  }
  
  if (ntpSynced != prev_ntp) {
    tft.fillRect(40, 127, 75, 9, C_BLACK);
    tft.setTextColor(ntpSynced ? C_GREEN : C_RED);
    tft.setTextSize(1); tft.setCursor(40, 127);
    tft.print(ntpSynced ? "SYNCED" : "NO SYNC");
    prev_ntp = ntpSynced;
  }
  
  bool l4sNow = (tcpClient && tcpClient.connected());
  if (l4sNow != prev_l4s) {
    tft.fillRect(40, 139, 75, 9, C_BLACK);
    tft.setTextColor(l4sNow ? C_GREEN : C_MGRAY);
    tft.setTextSize(1); tft.setCursor(40, 139);
    tft.print(l4sNow ? "LINKED" : "WAITING");
    prev_l4s = l4sNow;
  }
  
  int rssiNow = WiFi.RSSI();
  if (abs(rssiNow - prev_rssi) > 2) {
    tft.fillRect(155, 115, 80, 9, C_BLACK);
    tft.setTextColor(C_MGRAY); tft.setTextSize(1);
    tft.setCursor(155, 115);
    snprintf(buf, sizeof(buf), "%d dBm", rssiNow);
    tft.print(buf);
    prev_rssi = rssiNow;
  }
  
  tft.fillRect(155, 127, 80, 9, C_BLACK);
  tft.setTextColor(C_MGRAY); tft.setTextSize(1);
  tft.setCursor(155, 127);
  snprintf(buf, sizeof(buf), "%u KB", ESP.getFreeHeap() / 1024);
  tft.print(buf);

  unsigned long up = millis()/1000;
  if (up/10 != prev_uptime/10) { 
    tft.fillRect(155, 139, 80, 9, C_BLACK);
    tft.setTextColor(C_MGRAY); tft.setTextSize(1);
    tft.setCursor(155, 139);
    snprintf(buf, sizeof(buf), "%luh %lum", up/3600, (up%3600)/60);
    tft.print(buf);
    prev_uptime = up;
  }
  
  float doppler = dopplerFreq;
  if (fabsf(doppler - prev_doppler) > 50.0f) {
    tft.fillRect(40, 151, 75, 9, C_BLACK);
    tft.setTextColor(doppler >= 0 ? C_GREEN : C_ORANGE);
    tft.setTextSize(1); tft.setCursor(40, 151);
    snprintf(buf, sizeof(buf), "%+d Hz", (int)doppler);
    tft.print(buf);
    prev_doppler = doppler;
  }
  
  float dist = satDistance;
  if (fabsf(dist - prev_dist) > 10.0f) {
    tft.fillRect(155, 151, 80, 9, C_BLACK);
    tft.setTextColor(C_MGRAY); tft.setTextSize(1);
    tft.setCursor(155, 151);
    snprintf(buf, sizeof(buf), "%d km", (int)dist);
    tft.print(buf);
    prev_dist = dist;
  }

  // PASS PREDICTION COUNTDOWN
  unsigned long nowEpoch = timeClient.getEpochTime();
  long countdown = nextAosTime - nowEpoch;
  
  if (countdown != prev_countdown) {
    tft.fillRect(5, 180, 120, 9, C_BLACK);
    tft.fillRect(150, 180, 80, 9, C_BLACK);
    tft.fillRect(5, 200, 120, 9, C_BLACK);
    tft.fillRect(150, 200, 80, 9, C_BLACK);
    
    tft.setTextSize(1);
    if (nextAosTime > 0) {
      struct tm *tmAos = gmtime(&nextAosTime);
      struct tm *tmLos = gmtime(&nextLosTime);
      
      tft.setTextColor(C_WHITE);
      tft.setCursor(5, 180);
      snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tmAos->tm_hour, tmAos->tm_min, tmAos->tm_sec);
      tft.print(buf);
      
      tft.setCursor(5, 200);
      snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tmLos->tm_hour, tmLos->tm_min, tmLos->tm_sec);
      tft.print(buf);
      
      tft.setCursor(150, 180);
      tft.setTextColor(C_YELLOW);
      snprintf(buf, sizeof(buf), "%.1f deg", nextMaxEl);
      tft.print(buf);
      
      tft.setCursor(150, 200);
      if (countdown < 0 && nowEpoch < nextLosTime) {
        tft.setTextColor(C_GREEN); tft.print("IN PASS");
      } else if (countdown >= 0) {
        tft.setTextColor(C_ORANGE);
        snprintf(buf, sizeof(buf), "%02ld:%02ld:%02ld", countdown/3600, (countdown%3600)/60, countdown%60);
        tft.print(buf);
      } else {
        tft.setTextColor(C_MGRAY); tft.print("CALCULATING");
      }
    } else {
      tft.setTextColor(C_MGRAY);
      tft.setCursor(5, 180); tft.print("WAITING FOR TLE");
    }
    prev_countdown = countdown;
  }
}

// =============================================
//   TFT — RADAR UPDATE (With Full Trajectory Arc)
// =============================================
void tftRadarUpdate() {
  int cx=120, cy=265, r=50;

  auto toXY = [cx,cy,r](float az, float el, int &x, int &y){
    float rr  = r * (1.0f - el/90.0f);
    float rad = (az - 90.0f) * PI / 180.0f;
    x = cx + (int)(rr * cosf(rad));
    y = cy + (int)(rr * sinf(rad));
  };

  // 1. Erase old moving components
  if (prev_antX >= 0) {
    tft.drawLine(cx, cy, prev_antX, prev_antY, C_BLACK);
    tft.fillCircle(prev_antX, prev_antY, 3, C_BLACK);
  }
  if (prev_satX >= 0) {
    tft.fillCircle(prev_satX, prev_satY, 5, C_BLACK);
  }

  // 2. Repair grid lines
  tft.drawCircle(cx, cy, r*2/3, C_DGRAY);
  tft.drawCircle(cx, cy, r*1/3, C_DGRAY);
  tft.drawFastVLine(cx, cy-r+1, r*2-2, C_DGRAY);
  tft.drawFastHLine(cx-r+1, cy, r*2-2, C_DGRAY);

  // 3. Draw Solid Trajectory Path + Labels
  int len = globalPathLen;
  if (len > 1) {
    int lastX = -1, lastY = -1;
    for (int i = 0; i < len; i++) {
      int px, py;
      toXY(globalPath[i].az, globalPath[i].el, px, py);
      
      // Draw the connecting line
      if (lastX != -1) {
        tft.drawLine(lastX, lastY, px, py, C_GREEN);
      }
      
      // Draw precise AOS / LOS Text Flags
      if (i == 0) {
        tft.setTextColor(C_GREEN); tft.setTextSize(1);
        tft.setCursor(px - 10, py - 10); tft.print("AOS");
      } else if (i == len - 1) {
        tft.setTextColor(C_RED); tft.setTextSize(1);
        tft.setCursor(px - 10, py - 10); tft.print("LOS");
      }
      lastX = px; lastY = py;
    }
  }

  // 4. Draw Current Antenna Pointer
  int antX, antY;
  toXY((float)currentAz, (float)currentEl, antX, antY);
  tft.drawLine(cx, cy, antX, antY, C_BLUE);
  tft.fillCircle(antX, antY, 3, C_CYAN);
  prev_antX = antX; prev_antY = antY;

  // 5. Draw Target Satellite Blip
  if (targetEl > 0.0f) {
    int satX, satY;
    toXY((float)targetAz, (float)targetEl, satX, satY);
    tft.fillCircle(satX, satY, 5, C_CYAN);
    tft.drawCircle(satX, satY, 8, C_CYAN);
    prev_satX = satX; prev_satY = satY;
  } else {
    prev_satX = -1; prev_satY = -1;
  }
}

// =============================================
//   PASS LOGGER
// =============================================
void logPass() {
  if (passLogCount >= 10) {
    for (int i = 0; i < 9; i++) passLog[i] = passLog[i+1];
    passLogCount = 9;
  }
  strncpy(passLog[passLogCount].sat,  satName,       24);
  strncpy(passLog[passLogCount].time, passStartBuf, 19);
  passLog[passLogCount].maxEl = passMaxEl;
  passLogCount++;
  Serial.printf("[PASS] Logged %s MaxEl:%.1f\n", satName, passMaxEl);
}

// =============================================
//   WEB SERVER
// =============================================
void setupWebServer() {
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  webServer.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", buildTelemetryJson());
  });
  webServer.on("/api/path", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!tleLoaded || !ntpSynced || !onboardMode) {
      request->send(200, "application/json", "[]"); return;
    }
    String json = "[";
    int len = globalPathLen;
    for (int i = 0; i < len; i++) {
      if (i > 0) json += ",";
      json += "{\"az\":" + String(globalPath[i].az,1) +
              ",\"el\":" + String(globalPath[i].el,1) + "}";
    }
    json += "]";
    request->send(200, "application/json", json);
  });
  webServer.on("/api/passlog", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "[";
    for (int i = 0; i < passLogCount; i++) {
      if (i > 0) json += ",";
      json += "{\"sat\":\"" + String(passLog[i].sat)  + "\","
              "\"time\":\"" + String(passLog[i].time) + "\","
              "\"maxEl\":"  + String(passLog[i].maxEl,1) + "}";
    }
    json += "]";
    request->send(200, "application/json", json);
  });
  webServer.on("/api/config", HTTP_POST,
    [](AsyncWebServerRequest *request){
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      String body = ""; for (size_t i = 0; i < len; i++) body += (char)data[i];
      StaticJsonDocument<256> doc;
      if (!deserializeJson(doc, body)) {
        if (doc.containsKey("grid")) {
          maidenhead = doc["grid"].as<String>();
          maidenheadToLatLon(maidenhead, obsLat, obsLon);
          prefs.begin("sattracker", false); prefs.putString("grid", maidenhead); prefs.end();
          lastPathCalc = 0;
          Serial.printf("[WEB] Grid updated to: %s\n", maidenhead.c_str());
        }
        if (doc.containsKey("obMode")) {
          onboardMode = doc["obMode"].as<bool>();
          prefs.begin("sattracker", false); prefs.putBool("obMode", onboardMode); prefs.end();
          lastPathCalc = 0;
          Serial.printf("[WEB] Tracking Mode -> %s\n", onboardMode ? "INTERNAL SGP4" : "EXTERNAL L4S");
        }
      }
    }
  );
  webServer.on("/api/tle", HTTP_POST,
    [](AsyncWebServerRequest *request){
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      String body = ""; for (size_t i = 0; i < len; i++) body += (char)data[i];
      StaticJsonDocument<512> doc;
      if (!deserializeJson(doc, body)) {
        String name = doc["name"]  | "UNKNOWN";
        String l1   = doc["line1"] | "";
        String l2   = doc["line2"] | "";
        if (l1.length() > 0 && l2.length() > 0) {
          File f = LittleFS.open("/tle.txt", "w");
          if (f) { f.println(name); f.println(l1); f.println(l2); f.close(); }
          name.toCharArray(satName, 25);
          l1.toCharArray(tleLine1, 70); l2.toCharArray(tleLine2, 70);
          sat.site(obsLat, obsLon, obsAlt); sat.init(satName, tleLine1, tleLine2);
          tleLoaded    = true; lastPathCalc = 0; prev_sat[0] = '\0';
          Serial.printf("[WEB] TLE Received & Saved for: %s\n", satName);
        }
      }
    }
  );
  webServer.on("/api/manual", HTTP_POST,
    [](AsyncWebServerRequest *request){
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      String body = ""; for (size_t i = 0; i < len; i++) body += (char)data[i];
      StaticJsonDocument<128> doc;
      if (!deserializeJson(doc, body)) {
        if (!onboardMode) {
          if (doc.containsKey("az")) targetAz = doc["az"].as<float>();
          if (doc.containsKey("el")) targetEl = doc["el"].as<float>();
          Serial.printf("[WEB] Manual Joystick Slew -> AZ:%.1f EL:%.1f\n", (float)targetAz, (float)targetEl);
        }
      }
    }
  );
  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
    if (type == WS_EVT_CONNECT) {
      Serial.printf("[WS] Client #%u connected to Dashboard\n", client->id());
      client->text(buildTelemetryJson());
    }
  });
  webServer.addHandler(&ws);
  webServer.begin();
}

// =============================================
//   PRECISE PATH PREDICTION (WATCHDOG SAFE)
// =============================================
void calculatePathPrediction() {
  if (!tleLoaded || !ntpSynced || !onboardMode) { globalPathLen = 0; return; }
  Sgp4 pathSat; pathSat.site(obsLat, obsLon, obsAlt); pathSat.init(satName, tleLine1, tleLine2);

  unsigned long t = timeClient.getEpochTime();
  bool inPass = false;
  
  pathSat.findsat(t);
  if (pathSat.satEl > 0) {
    inPass = true;
    while (pathSat.satEl > 0) { 
      t -= 60; 
      pathSat.findsat(t); 
      vTaskDelay(pdMS_TO_TICKS(2)); // Pause to feed watchdog
    } 
    t += 60;
  } else {
    for (int i = 0; i < 1440; i += 2) {
      t += 120; pathSat.findsat(t);
      if (pathSat.satEl > 0) { inPass = true; break; }
      
      // Pause every 20 loops to prevent FreeRTOS crash
      if (i % 20 == 0) vTaskDelay(pdMS_TO_TICKS(2)); 
    }
    if (inPass) {
       while (pathSat.satEl > 0) { 
         t -= 60; 
         pathSat.findsat(t); 
         vTaskDelay(pdMS_TO_TICKS(2)); 
       } 
       t += 60;
    }
  }
  
  float tempMaxEl = 0.0;
  int tempLen = 0;
  
  if (inPass) {
    nextAosTime = t;
    for (int i = 0; i < 45; i++) {
      pathSat.findsat(t);
      if (pathSat.satEl < 0 && i > 0) break;
      if (pathSat.satEl >= 0) {
        if (pathSat.satEl > tempMaxEl) tempMaxEl = pathSat.satEl;
        globalPath[tempLen].az = pathSat.satAz; 
        globalPath[tempLen].el = pathSat.satEl; 
        tempLen++;
      }
      t += 60;
      vTaskDelay(pdMS_TO_TICKS(2)); // Pause to feed watchdog
    }
    nextLosTime = t;
  } else {
    nextAosTime = 0; nextLosTime = 0;
  }
  
  // Safely write to global variables after calculations are finished
  nextMaxEl = tempMaxEl;
  globalPathLen = tempLen;
  
  Serial.printf("[SGP4] Pass Array Calculated: %d points. Max El: %.1f deg\n", globalPathLen, nextMaxEl);
}

// =============================================
//   CORE 0 TASK (TCP, WebSocket, Display)
// =============================================
void Core0TaskCode(void *pvParameters) {
  for (;;) {
    esp_task_wdt_reset();

    if (WiFi.status() == WL_CONNECTED) {
      if (timeClient.update()) {
          if (!ntpSynced) Serial.println("[NTP] UTC Time Synchronized Successfully");
          ntpSynced = true;
      }
    }

    static unsigned long lastWifiCheck = 0;
    if (millis() - lastWifiCheck > 10000) {
      lastWifiCheck = millis();
      if (WiFi.status() != WL_CONNECTED) { Serial.println("[WIFI] Connection lost. Reconnecting..."); WiFi.reconnect(); }
    }

    if (!tcpClient || !tcpClient.connected()) tcpClient = tcpServer.available();
    if (tcpClient && tcpClient.available()) {
      tcpClient.setTimeout(10);
      String cmd = tcpClient.readStringUntil('\n');
      if (cmd.length() > 0) {
        if (!onboardMode) {
            parseEasyComm(cmd);
            Serial.printf("[L4S] Incoming TCP Cmd -> AZ: %.1f | EL: %.1f\n", (float)targetAz, (float)targetEl);
        }
        tcpClient.println("OK");
      }
    }

    if (millis() - lastPathCalc > 60000) { calculatePathPrediction(); lastPathCalc = millis(); }

    static unsigned long lastWsPush = 0;
    if (millis() - lastWsPush > 1000) {
      if (ws.count() > 0) ws.textAll(buildTelemetryJson());
      lastWsPush = millis();
    }

    static unsigned long lastTft = 0;
    if (millis() - lastTft > 300) { tftUpdateDynamic(); tftRadarUpdate(); lastTft = millis(); }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// =============================================
//   SETUP
// =============================================
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n====================================");
  Serial.println(" ESP32 Aerospace Tracker v5.2");
  Serial.println("====================================");

  esp_task_wdt_init(30, true);
  esp_task_wdt_add(NULL);

  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, HIGH);

  azStepper.setMaxSpeed(2000.0); azStepper.setAcceleration(1000.0);
  elStepper.setMaxSpeed(2000.0); elStepper.setAcceleration(1000.0);

  tft.init(240, 320);       
  tft.setRotation(0);       
  tft.invertDisplay(false); 
  tft.fillScreen(C_BLACK);  
  
  tft.setTextColor(C_CYAN); tft.setTextSize(2);
  tft.setCursor(20, 90);  tft.print("AEROSPACE CMD");
  tft.setTextSize(1); tft.setTextColor(C_MGRAY);
  tft.setCursor(60, 120); tft.print("Booting ST7789 system...");

  if (!LittleFS.begin(false)) LittleFS.begin(true);

  prefs.begin("sattracker", true);
  maidenhead  = prefs.getString("grid", maidenhead);
  onboardMode = prefs.getBool("obMode", false);
  prefs.end();
  maidenheadToLatLon(maidenhead, obsLat, obsLon);
  
  if (LittleFS.exists("/tle.txt")) {
    File f = LittleFS.open("/tle.txt", "r");
    if (f) {
      String n  = f.readStringUntil('\n'); n.trim();  n.toCharArray(satName, 25);
      String l1 = f.readStringUntil('\n'); l1.trim(); l1.toCharArray(tleLine1, 70);
      String l2 = f.readStringUntil('\n'); l2.trim(); l2.toCharArray(tleLine2, 70);
      f.close();
      if (strlen(tleLine1) > 0 && strlen(tleLine2) > 0) {
        sat.site(obsLat, obsLon, obsAlt);
        sat.init(satName, tleLine1, tleLine2);
        tleLoaded = true;
        Serial.printf("[BOOT] Loaded TLE for: %s\n", satName);
      }
    }
  }

  tft.fillScreen(C_BLACK);
  tft.setTextColor(C_CYAN); tft.setTextSize(2);
  tft.setCursor(40, 90); tft.print("CONNECTING...");
  
  WiFi.begin(ssid, password);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry++ < 20) { delay(500); }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    
    // START mDNS BROADCASTER
    if (MDNS.begin("sattracker")) {
      Serial.println("[mDNS] Broadcast active: sattracker.local");
      MDNS.addService("http", "tcp", 80);
      MDNS.addService("easycomm", "tcp", 4533);
    }
  }

  timeClient.begin();
  timeClient.update();
  if (timeClient.getEpochTime() > 1000000) ntpSynced = true;

  ArduinoOTA.setHostname("sattracker");
  ArduinoOTA.begin();

  setupWebServer();
  tcpServer.begin();
  Serial.println("[WEB] Port 80 Active | TCP Listener Port 4533 Active");

  tftDrawStaticFrame();
  digitalWrite(ENABLE_PIN, LOW);

  xTaskCreatePinnedToCore(Core0TaskCode, "Core0Task", 16384, NULL, 1, &Core0Task, 0);
}

// =============================================
//   LOOP (Core 1 — steppers + SGP4 + OTA)
// =============================================
void loop() {
  esp_task_wdt_reset();
  ArduinoOTA.handle();

  targetEl = constrain((float)targetEl, 0.0f, 90.0f);
  targetAz = fmod((float)targetAz + 360.0f, 360.0f);

  azStepper.moveTo((long)((float)targetAz * STEPS_PER_DEGREE_AZ));
  elStepper.moveTo((long)((float)targetEl * STEPS_PER_DEGREE_EL));
  azStepper.run();
  elStepper.run();

  currentAz = azStepper.currentPosition() / STEPS_PER_DEGREE_AZ;
  currentEl = elStepper.currentPosition() / STEPS_PER_DEGREE_EL;
  isMoving  = azStepper.isRunning() || elStepper.isRunning();

  static unsigned long lastLog = 0;

  if (onboardMode) {
      // Run SGP4 engine exactly 1 time per second
      if (tleLoaded && ntpSynced && millis() - lastSGP4Update > 1000) {
        runSGP4();
        lastSGP4Update = millis();
      }
      
      if (millis() - lastLog > 5000) {
          if (!tleLoaded) Serial.println("[SGP4] Awaiting TLE Data from Web Dashboard...");
          else if (!ntpSynced) Serial.println("[SGP4] Awaiting NTP Time Sync...");
          else Serial.printf("[SGP4] Internal Tracking -> AZ: %.1f | EL: %.1f | Dist: %.0fkm\n", (float)targetAz, (float)targetEl, (float)satDistance);
          lastLog = millis();
      }
  } else {
      if (millis() - lastLog > 10000) {
          Serial.println("[SYS] Awaiting External Look4Sat Commands on Port 4533...");
          lastLog = millis();
      }
  }
}

// =============================================
//   EASYCOMM II PARSER
// =============================================
void parseEasyComm(String cmd) {
  cmd.trim();
  if (cmd.length() < 2) return;
  if (cmd.startsWith("P ") || cmd.startsWith("p ")) {
    int fs = cmd.indexOf(" ");
    int ss = cmd.indexOf(" ", fs+1);
    if (fs != -1 && ss != -1) {
      targetAz = cmd.substring(fs+1, ss).toFloat();
      targetEl = cmd.substring(ss+1).toFloat();
    }
  } else if (cmd.indexOf("AZ") != -1 || cmd.indexOf("az") != -1) {
    int ai = cmd.indexOf("AZ"); if (ai==-1) ai=cmd.indexOf("az");
    int ei = cmd.indexOf("EL"); if (ei==-1) ei=cmd.indexOf("el");
    if (ai != -1 && ei != -1) {
      targetAz = cmd.substring(ai+2, ei).toFloat();
      int ee   = cmd.indexOf(" ", ei); if (ee==-1) ee=cmd.length();
      targetEl = cmd.substring(ei+2, ee).toFloat();
    }
  } else if (cmd == "p" || cmd == "P") {
    if (tcpClient) tcpClient.printf("AZ%.2f EL%.2f\n", (float)currentAz, (float)currentEl);
  }
}

// =============================================
//   SGP4 RUN + DOPPLER + PASS LOGGING
// =============================================
void runSGP4() {
  unsigned long now = timeClient.getEpochTime();
  sat.site(obsLat, obsLon, obsAlt);
  sat.findsat(now);

  unsigned long curMs = millis();
  if (lastSatDist > 0 && curMs > lastDopplerTime) {
    double dt = (curMs - lastDopplerTime) / 1000.0;
    if (dt > 0) {
      double rangeRate = (sat.satDist - lastSatDist) / dt;
      dopplerFreq = 145800000.0 * (-rangeRate / 299792.458);
    }
  }
  lastSatDist     = sat.satDist;
  lastDopplerTime = curMs;
  satDistance     = (float)sat.satDist;

  if (sat.satEl >= 0.0) {
    targetAz = sat.satAz;
    targetEl = sat.satEl;
    if (!passInProgress) {
      passInProgress = true;
      passMaxEl      = sat.satEl;
      time_t t       = (time_t)now;
      struct tm *tm  = gmtime(&t);
      sprintf(passStartBuf, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
      Serial.printf("[PASS] Active Pass Triggered for: %s\n", satName);
    } else {
      if ((float)sat.satEl > passMaxEl) passMaxEl = sat.satEl;
    }
  } else {
    if (passInProgress) {
      passInProgress = false;
      logPass();
    }
  }
}