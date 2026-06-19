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
#include <HTTPClient.h>
#include "qrcodegen.h"
#include "secrets.h"

// ╔══════════════════════════════════════════════════════════════╗
// ║  MPU6050  GY-521   VCC→3.3V  GND→GND  SDA→21  SCL→22       ║
// ╚══════════════════════════════════════════════════════════════╝
#include <Wire.h>
#define MPU_ADDR  0x68
#define MPU_SDA   21
#define MPU_SCL   22

// ── Kalman filter state (replaces complementary filter) ────────
// Two independent 2-state Kalman filters, one per axis.
// State vector: [angle, gyro_bias]
// Measurement:  accelerometer-derived angle
struct KalmanAxis {
  float angle   = 0;   // estimated angle (deg)
  float bias    = 0;   // estimated gyro bias (deg/s)
  float P[2][2] = {{1,0},{0,1}}; // error covariance

  // Process noise covariance
  static constexpr float Q_angle = 0.001f;
  static constexpr float Q_bias  = 0.003f;
  // Measurement noise covariance
  static constexpr float R_meas  = 0.03f;

  float update(float newAngle, float newRate, float dt) {
    // Predict
    angle += dt * (newRate - bias);
    P[0][0] += dt*(dt*P[1][1] - P[0][1] - P[1][0] + Q_angle);
    P[0][1] -= dt * P[1][1];
    P[1][0] -= dt * P[1][1];
    P[1][1] += Q_bias * dt;
    // Update
    float S  = P[0][0] + R_meas;
    float K0 = P[0][0] / S;
    float K1 = P[1][0] / S;
    float y  = newAngle - angle;
    angle += K0 * y;
    bias  += K1 * y;
    float P00 = P[0][0];
    float P01 = P[0][1];
    P[0][0] -= K0 * P00;
    P[0][1] -= K0 * P01;
    P[1][0] -= K1 * P00;
    P[1][1] -= K1 * P01;
    return angle;
  }

  void seed(float a, float b=0){ angle=a; bias=b; }
};

static KalmanAxis kalPitch, kalRoll;

// ── IMU filter parameters ──────────────────────────────────────
#define MPU_DLPF_CFG   4      // hardware 21 Hz LPF
#define ACCEL_LPF      0.15f  // software accel pre-smoother
#define GYRO_DEAD      0.8f   // deg/s deadband
#define SPIKE_LSB      800.0f // accel spike rejection threshold

// ── IMU correction ─────────────────────────────────────────────
#define IMU_CORRECT_ENABLED  false
#define IMU_CORRECT_THRESH   2.5f

// ── IMU public outputs ─────────────────────────────────────────
volatile float imuPitch = 0, imuRoll = 0;
volatile bool  imuOK    = false;

// ── IMU internals ──────────────────────────────────────────────
static float _lpfAx=0,_lpfAy=0,_lpfAz=0;
static bool  _seeded=false;
static float _gBiasX=0,_gBiasY=0;
static float _aBiasX=0,_aBiasY=0,_aBiasZ=0;
static unsigned long _lastUs=0;

// ── IMU error history for TFT drift graph ─────────────────────
#define ERR_HIST 60
static float errHistory[ERR_HIST];
static int   errHistIdx=0;
static bool  errHistFull=false;

// ── Button ─────────────────────────────────────────────────────
#define BTN_PIN     0       // IO0 — BOOT button on most ESP32 devboards
#define SCREEN_COUNT 5
// Screen 0: Main HUD (existing)
// Screen 1: Pass Schedule
// Screen 2: IMU Drift Graph
// Screen 3: Satellite Footprint
// Screen 4: Weather
volatile int  currentScreen = 0;
volatile bool screenChanged  = false;
static unsigned long lastBtnMs = 0;

// ── Satellite queue (feature 4) ────────────────────────────────
#define MAX_QUEUED_SATS 8
struct QueuedSat {
  char name[25];
  char line1[70];
  char line2[70];
  bool valid;
};
QueuedSat satQueue[MAX_QUEUED_SATS];
int  queueHead    = 0;   // index of currently tracked sat
int  queueCount   = 0;

// ── Pass schedule (feature 6) ──────────────────────────────────
#define MAX_SCHEDULED_PASSES 24
struct ScheduledPass {
  char    satName[25];
  time_t  aos;
  time_t  los;
  float   maxEl;
  float   aosAz;
};
ScheduledPass schedule[MAX_SCHEDULED_PASSES];
int  scheduleCount   = 0;
bool scheduleReady   = false;
bool scheduleBusy    = false;
volatile bool scheduleDirty = false;   // (A3) recompute when queue changes / web refresh

// ── Auto-park (feature 1) ──────────────────────────────────────
#define PARK_IDLE_SEC   300   // park after 5 min idle with no pass
#define PARK_AZ         0.0f
#define PARK_EL         0.0f
bool     parked         = false;
unsigned long lastActiveMs = 0;

// ── Weather (feature 14) ───────────────────────────────────────
// Requires free API key from openweathermap.org
// Set OWM_API_KEY in secrets.h as:  #define OWM_API_KEY "your_key"
struct WeatherData {
  float   tempC      = 0;
  float   feelsC     = 0;     // (D) apparent/feels-like temp
  float   windMps    = 0;
  float   windDeg    = 0;
  int     humidity   = 0;
  int     pressure   = 0;     // (D) hPa
  int     visibility = 0;     // (D) metres
  char    desc[32]   = "---";
  char    city[32]   = "---";
  time_t  fetchedAt  = 0;
  bool    valid      = false;
};
WeatherData weather;
unsigned long lastWeatherFetch = 0;
#define WEATHER_INTERVAL_MS  600000UL  // fetch every 10 min

// ── Radar safe zone (unchanged) ────────────────────────────────
#define RADAR_SAFE_TOP   164
#define RADAR_SAFE_BOT   293
#define RADAR_SAFE_LEFT    0
#define RADAR_SAFE_RIGHT 239

// ================= WEB DASHBOARD =================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>ORBITAL OPS</title><style>
@import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Rajdhani:wght@500;600;700&display=swap');
*{margin:0;padding:0;box-sizing:border-box}
:root{--pri:#b9a0dc;--acc:#78e69a;--grn:#78e69a;--amb:#b9a0dc;--red:#ff6b8a;--dim:#2e2a44;--txt:#cabfe6;--bg:#0a0810;--pnl:#120e1c}
body{font-family:'Rajdhani',sans-serif;background:var(--bg);color:var(--txt);min-height:100vh;display:flex;flex-direction:column;align-items:center;padding:16px;
background-image:linear-gradient(rgba(120,230,154,.03) 1px,transparent 1px),linear-gradient(90deg,rgba(185,160,220,.03) 1px,transparent 1px);background-size:32px 32px}
body::after{content:'';position:fixed;inset:0;pointer-events:none;background:repeating-linear-gradient(0deg,transparent 0 2px,rgba(0,0,0,.08) 2px 4px)}
.topbar{width:100%;max-width:980px;display:flex;justify-content:space-between;align-items:center;border:1px solid var(--dim);background:var(--pnl);padding:10px 16px;margin-bottom:14px;position:relative}
.topbar::before,.topbar::after{content:'';position:absolute;width:10px;height:10px;border:2px solid var(--grn)}
.topbar::before{top:-2px;left:-2px;border-right:0;border-bottom:0}
.topbar::after{bottom:-2px;right:-2px;border-left:0;border-top:0}
.logo{font-family:'Share Tech Mono',monospace;font-size:1.3rem;color:var(--acc);letter-spacing:4px;text-shadow:0 0 12px rgba(120,230,154,.45)}
.logo span{color:var(--pri)}
.clock{font-family:'Share Tech Mono',monospace;font-size:1.1rem;color:var(--pri)}
.badges{display:flex;gap:8px;margin-bottom:14px;flex-wrap:wrap;justify-content:center}
.bdg{padding:5px 14px;font-size:.7rem;font-weight:700;letter-spacing:2px;border:1px solid var(--dim);background:var(--pnl);text-transform:uppercase}
.bdg.ok{color:var(--acc);border-color:var(--acc);box-shadow:0 0 12px rgba(120,230,154,.2)}
.bdg.err{color:var(--red);border-color:var(--red);box-shadow:0 0 12px rgba(255,107,138,.2)}
.bdg.inf{color:var(--pri);border-color:var(--pri)}
.statstrip{display:grid;grid-template-columns:repeat(4,1fr);gap:10px;width:100%;max-width:980px;margin-bottom:14px}
.stat{background:var(--pnl);border:1px solid var(--dim);padding:12px;text-align:center;position:relative}
.stat::before{content:'';position:absolute;top:0;left:0;width:24px;height:2px;background:var(--acc)}
.stat .lbl{font-size:.6rem;letter-spacing:3px;color:#7a7493;text-transform:uppercase}
.stat .val{font-family:'Share Tech Mono',monospace;font-size:1.9rem;color:var(--acc);text-shadow:0 0 14px rgba(120,230,154,.4)}
.stat.a .val{color:var(--pri);text-shadow:0 0 14px rgba(185,160,220,.4)}
.stat .unit{font-size:.8rem;color:#7a7493}
.grid{display:grid;grid-template-columns:1fr;gap:14px;width:100%;max-width:980px}
@media(min-width:880px){.grid{grid-template-columns:1fr 1fr}}
.panel{background:var(--pnl);border:1px solid var(--dim);padding:18px;position:relative}
.panel::before,.panel::after{content:'';position:absolute;width:8px;height:8px;border:2px solid var(--acc)}
.panel::before{top:-2px;left:-2px;border-right:0;border-bottom:0}
.panel::after{bottom:-2px;right:-2px;border-left:0;border-top:0}
.ph{font-family:'Share Tech Mono',monospace;font-size:.72rem;color:var(--acc);letter-spacing:3px;margin-bottom:14px;border-bottom:1px solid var(--dim);padding-bottom:8px;display:flex;align-items:center;gap:8px}
.ph::before{content:'▸';color:var(--pri)}
.row{display:flex;justify-content:space-between;margin-bottom:8px;font-size:.9rem;border-bottom:1px dotted #1c2733;padding-bottom:4px}
.k{color:#7a7493;letter-spacing:1px}.v{font-family:'Share Tech Mono',monospace;color:var(--txt)}
.vg{color:var(--acc)}.va{color:var(--pri)}.vr{color:var(--red)}
input,select,button{width:100%;padding:11px;margin-bottom:10px;background:#0c0a16;border:1px solid var(--dim);color:var(--txt);font-size:.85rem;outline:none;font-family:'Rajdhani',sans-serif;font-weight:600}
input:focus,select:focus{border-color:var(--acc);box-shadow:0 0 10px rgba(120,230,154,.2)}
button{background:rgba(120,230,154,.07);color:var(--acc);border:1px solid var(--acc);cursor:pointer;letter-spacing:2px;text-transform:uppercase;transition:.15s}
button:hover{background:var(--acc);color:#04130a;box-shadow:0 0 20px rgba(120,230,154,.45)}
.ig{display:flex;gap:8px}.ig input,.ig button{margin-bottom:0}.ig button{width:auto;flex-shrink:0}
.radwrap{display:flex;justify-content:center;margin:10px 0}
canvas{background:radial-gradient(circle,#100a1c 0%,#08060f 100%);border-radius:50%;box-shadow:0 0 30px rgba(185,160,220,.15),inset 0 0 50px rgba(120,230,154,.05)}
.joy{display:grid;grid-template-columns:repeat(3,50px);gap:5px;justify-content:center;margin-top:12px}
.joy button{padding:13px 0;margin:0;background:#15111f;border:1px solid var(--dim);color:#7a7493}
.joy button:hover{background:var(--acc);color:#04130a}
.flt{border-color:var(--pri)!important;color:var(--pri)!important}
.flt::placeholder{color:rgba(185,160,220,.4)}
.lost{display:none;position:fixed;top:0;left:0;width:100%;background:var(--red);color:#fff;text-align:center;padding:8px;font-size:.8rem;font-weight:bold;letter-spacing:3px;z-index:999}
.sched-row{display:flex;justify-content:space-between;padding:6px 0;border-bottom:1px dotted #1c2733;font-size:.8rem}
.sched-sat{color:var(--acc);font-family:'Share Tech Mono',monospace;min-width:120px}
.sched-time{color:#7a7493;font-family:'Share Tech Mono',monospace}
.sched-el{color:var(--pri);font-family:'Share Tech Mono',monospace;text-align:right}
.wx-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:10px}
.wx-stat{background:#0c0a16;border:1px solid var(--dim);padding:10px;text-align:center}
.wx-val{font-family:'Share Tech Mono',monospace;font-size:1.4rem;color:var(--acc)}
.wx-lbl{font-size:.65rem;color:#7a7493;letter-spacing:2px;text-transform:uppercase}
.queue-item{display:flex;justify-content:space-between;align-items:center;padding:5px 0;border-bottom:1px dotted #1c2733}
.queue-item.active{color:var(--acc)}
.queue-item button{width:auto;padding:4px 10px;margin:0;font-size:.7rem}
</style></head><body>
<div class="lost" id="connLost">⚠ DOWNLINK LOST — REACQUIRING</div>
<div class="topbar">
  <div class="logo">ORBITAL<span>OPS</span></div>
  <div style="display:flex;align-items:center;gap:14px">
    <span id="weatherBadge" style="font-size:.75rem;color:#7a7493;font-family:'Share Tech Mono',monospace"></span>
    <div class="clock" id="utcClock">--:--:--</div>
  </div>
</div>
<div class="badges">
<div class="bdg err" id="statusBadge">CONNECTING</div>
<div class="bdg inf" id="modeBadge">--</div>
<div class="bdg inf" id="satBadge">NO TARGET</div>
<div class="bdg inf" id="parkBadge" style="display:none">PARKED</div>
</div>
<div class="statstrip">
<div class="stat"><div class="lbl">Azimuth</div><div class="val" id="curAz">---</div><div class="unit">DEG</div></div>
<div class="stat a"><div class="lbl">Elevation</div><div class="val" id="curEl">---</div><div class="unit">DEG</div></div>
<div class="stat"><div class="lbl">Range</div><div class="val" id="satDist">---</div><div class="unit">KM</div></div>
<div class="stat a"><div class="lbl">Doppler</div><div class="val" id="doppler">---</div><div class="unit">HZ</div></div>
</div>
<div class="grid">
<!-- LEFT COLUMN -->
<div>
<div class="panel" style="margin-bottom:14px"><div class="ph">TACTICAL RADAR</div>
<div class="radwrap"><canvas id="radar" width="280" height="280"></canvas></div>
<div class="joy">
<div></div><button onclick="nudge(0,5)">▲</button><div></div>
<button onclick="nudge(-5,0)">◀</button><div></div><button onclick="nudge(5,0)">▶</button>
<div></div><button onclick="nudge(0,-5)">▼</button><div></div>
</div></div>

<div class="panel"><div class="ph">WEATHER</div>
<div id="wxBody">
<div style="color:#7a7493;font-size:.8rem">Loading...</div>
</div></div>
</div>

<!-- RIGHT COLUMN -->
<div>
<div class="panel" style="margin-bottom:14px"><div class="ph">TELEMETRY</div>
<div class="row"><span class="k">TARGET AZ / EL</span><span class="v vg" id="tgt">--° / --°</span></div>
<div class="row"><span class="k">IMU PITCH / ROLL</span><span class="v va" id="imuAngles">-- / --</span></div>
<div class="row"><span class="k">IMU STATUS</span><span class="v" id="imuStatus">--</span></div>
<div class="row"><span class="k">WIND SPEED</span><span class="v" id="windSpd">-- m/s</span></div>
<div class="row"><span class="k">WIFI SIGNAL</span><span class="v" id="rssi">-- dBm</span></div>
<div class="row"><span class="k">UPTIME</span><span class="v" id="uptime">--</span></div>
<div class="row" style="border:none"><span class="k">FREE MEMORY</span><span class="v" id="ram">-- KB</span></div>
</div>

<div class="panel" style="margin-bottom:14px"><div class="ph">ORBITAL DATABASE</div>
<div class="row" style="border:none;margin-bottom:10px">
<span class="k" style="line-height:2.6">ENGINE</span>
<select id="mode-select" onchange="updateMode()" style="width:170px;margin:0">
<option value="0">EXTERNAL (LOOK4SAT)</option><option value="1">INTERNAL SGP4</option>
</select></div>
<div class="ig" style="margin-bottom:10px"><input type="text" id="grid-input" placeholder="Grid (MM71dl)" maxlength="6"><button onclick="updateGrid()">SET</button></div>
<button onclick="fetchCelestrak()" id="btn-fetch">1 ▸ DOWNLOAD CELESTRAK DB</button>
<input type="text" id="sat-filter" class="flt" placeholder="FILTER TARGETS..." oninput="filterSats()" disabled>
<select id="sat-select" size="4" style="height:84px"><option>AWAITING DATABASE</option></select>
<div style="display:flex;gap:8px">
<button onclick="pushTLE()" id="btn-push" style="flex:1">2 ▸ UPLOAD TLE</button>
<button onclick="queueTLE()" id="btn-queue" style="flex:1;border-color:#b9a0dc;color:#b9a0dc">+ QUEUE</button>
</div></div>

<div class="panel" style="margin-bottom:14px"><div class="ph">SATELLITE QUEUE</div>
<div id="queueBody"><div style="color:#7a7493;font-size:.8rem">Queue empty</div></div>
<button onclick="clearQueue()" style="margin-top:8px;border-color:#ff6b8a;color:#ff6b8a">CLEAR QUEUE</button>
</div>

<div class="panel"><div class="ph">PASS SCHEDULE (24H)</div>
<button onclick="loadSchedule()" style="margin-bottom:10px">REFRESH SCHEDULE</button>
<div id="schedBody" style="font-family:'Share Tech Mono',monospace;font-size:.75rem;color:#7a7493;max-height:200px;overflow-y:auto">
Press REFRESH to compute
</div></div>
</div>
</div>

<div class="panel" style="width:100%;max-width:980px;margin-top:14px"><div class="ph">PASS LOG</div>
<button onclick="loadPassLog()" style="margin-bottom:10px">REFRESH LOG</button>
<div id="passLogBody" style="font-family:'Share Tech Mono',monospace;font-size:.8rem;color:#7a7493">NO PASSES RECORDED</div>
</div>

<script>
let currentAz=0,currentEl=0,targetAz=0,targetEl=0;
let allTleData=[],tleData=[],satPath=[],isGeo=false,isParked=false;
let failCount=0,sweepAngle=0;
let satFootprintRadius=0;

setInterval(()=>{document.getElementById('utcClock').innerText=new Date().toISOString().substr(11,8)+' UTC'},1000);

async function fetchTelemetry(){
 try{
  const r=await fetch('/api/status',{cache:'no-store'});
  if(!r.ok)throw 0;
  const d=await r.json();
  failCount=0;
  document.getElementById('connLost').style.display='none';
  currentAz=d.cAz;currentEl=d.cEl;targetAz=d.tAz;targetEl=d.tEl;isGeo=!!d.geo;
  isParked=!!d.parked;
  satFootprintRadius=d.footprintR||0;
  document.getElementById('curAz').innerText=currentAz.toFixed(1);
  document.getElementById('curEl').innerText=currentEl.toFixed(1);
  document.getElementById('satDist').innerText=(d.dist||0).toFixed(0);
  document.getElementById('doppler').innerText=(d.doppler||0).toFixed(0);
  document.getElementById('tgt').innerText=targetAz.toFixed(1)+'° / '+targetEl.toFixed(1)+'°';
  document.getElementById('rssi').innerText=d.rssi+' dBm';
  document.getElementById('ram').innerText=d.freeHeap+' KB';
  const up=d.uptime;
  document.getElementById('uptime').innerText=Math.floor(up/3600)+'h '+Math.floor((up%3600)/60)+'m '+(up%60)+'s';
  document.getElementById('grid-input').placeholder=d.grid;
  document.getElementById('mode-select').value=d.mode===1?"1":"0";
  const b=document.getElementById('statusBadge');
  b.className='bdg ok';b.innerText=isParked?'⊙ PARKED':d.isMoving?'⟳ SLEWING':'● ONLINE';
  document.getElementById('modeBadge').innerText=d.mode===1?(isGeo?'GEO LOCK':'SGP4 INT'):'EXT L4S';
  document.getElementById('satBadge').innerText=d.sat;
  const pk=document.getElementById('parkBadge');
  pk.style.display=isParked?'':'none';
  if(d.imuOK){
   document.getElementById('imuAngles').innerText=parseFloat(d.imuPitch).toFixed(2)+'° / '+parseFloat(d.imuRoll).toFixed(2)+'°';
   document.getElementById('imuStatus').className='v vg';
   document.getElementById('imuStatus').innerText='ONLINE';
  }else{
   document.getElementById('imuAngles').innerText='-- / --';
   document.getElementById('imuStatus').className='v vr';
   document.getElementById('imuStatus').innerText='OFFLINE';
  }
  if(d.wind!==undefined) document.getElementById('windSpd').innerText=parseFloat(d.wind).toFixed(1)+' m/s';
  if(d.wxDesc) document.getElementById('weatherBadge').innerText='☁ '+d.wxDesc+' '+parseFloat(d.wxTemp||0).toFixed(0)+'°C';
  renderWeather(d);
  renderQueue(d.queue||[]);
 }catch(e){
  if(++failCount>3){
   document.getElementById('connLost').style.display='block';
   const b=document.getElementById('statusBadge');
   b.className='bdg err';b.innerText='✕ OFFLINE';
  }
 }
}

function renderWeather(d){
 if(!d.wxValid){document.getElementById('wxBody').innerHTML='<div style="color:#7a7493;font-size:.8rem">No weather data</div>';return;}
 // 16-point compass from bearing (D)
 const COMP=['N','NNE','NE','ENE','E','ESE','SE','SSE','S','SSW','SW','WSW','W','WNW','NW','NNW'];
 const wdeg=parseFloat(d.wxWindDeg||0);
 const wcomp=COMP[((Math.round(wdeg/22.5))%16+16)%16];
 document.getElementById('wxBody').innerHTML=`
 <div style="color:#cabfe6;margin-bottom:4px;font-size:.9rem">${d.wxCity}</div>
 <div style="color:#7a7493;margin-bottom:8px;font-size:.78rem;letter-spacing:1px;text-transform:uppercase">${d.wxDesc}</div>
 <div class="wx-grid">
  <div class="wx-stat"><div class="wx-val">${parseFloat(d.wxTemp||0).toFixed(1)}°C</div><div class="wx-lbl">TEMP</div></div>
  <div class="wx-stat"><div class="wx-val" style="color:var(--pri)">${parseFloat(d.wxFeels||0).toFixed(1)}°C</div><div class="wx-lbl">FEELS</div></div>
  <div class="wx-stat"><div class="wx-val">${parseFloat(d.wind||0).toFixed(1)}</div><div class="wx-lbl">WIND m/s</div></div>
  <div class="wx-stat"><div class="wx-val">${wdeg.toFixed(0)}° ${wcomp}</div><div class="wx-lbl">WIND DIR</div></div>
  <div class="wx-stat"><div class="wx-val">${d.wxHum||'--'}%</div><div class="wx-lbl">HUMIDITY</div></div>
  <div class="wx-stat"><div class="wx-val" style="color:var(--pri)">${d.wxPressure||'--'}hPa</div><div class="wx-lbl">PRESSURE</div></div>
 </div>`;
}

function renderQueue(q){
 const el=document.getElementById('queueBody');
 if(!q.length){el.innerHTML='<div style="color:#7a7493;font-size:.8rem">Queue empty</div>';return;}
 el.innerHTML=q.map((s,i)=>`
 <div class="queue-item ${s.active?'active':''}">
  <span style="font-family:\'Share Tech Mono\',monospace;font-size:.8rem">${s.active?'▶ ':''} ${s.name}</span>
  <button onclick="removeFromQueue(${i})" style="border-color:#ff6b8a;color:#ff6b8a">✕</button>
 </div>`).join('');
}

async function loadSchedule(){
 document.getElementById('schedBody').innerText='Computing...';
 try{
  // (A3) ask firmware to recompute (scheduleDirty), then poll /api/schedule
  await fetch('/api/schedule/refresh',{method:'POST'});
  let d=null, tries=0;
  do{
   await new Promise(r=>setTimeout(r,250));
   const r=await fetch('/api/schedule'); d=await r.json();
   if(d.length) break;
  }while(++tries<8);   // up to ~2s for the Core0 compute to land
  if(!d.length){document.getElementById('schedBody').innerText='No passes in next 24h';return;}
  document.getElementById('schedBody').innerHTML=d.map(p=>{
   const aos=new Date(p.aos*1000);
   const t=aos.toISOString().substr(11,5)+' UTC';
   return`<div class="sched-row">
    <span class="sched-sat">${p.sat}</span>
    <span class="sched-time">${t}</span>
    <span class="sched-el">↑${parseFloat(p.maxEl).toFixed(1)}°</span>
   </div>`;
  }).join('');
 }catch(e){document.getElementById('schedBody').innerText='LOAD FAILED';}
}

async function fetchPath(){try{const r=await fetch('/api/path');if(r.ok)satPath=await r.json();}catch(e){}}

async function loadPassLog(){
 try{
  const r=await fetch('/api/passlog');const d=await r.json();
  const el=document.getElementById('passLogBody');
  if(!d.length){el.innerText='NO PASSES RECORDED';return;}
  el.innerHTML=d.reverse().map(p=>`<div style="margin-bottom:5px;border-bottom:1px dotted #1c2733;padding-bottom:4px">
   <span style="color:#78e69a">${p.sat}</span> · <span style="color:#7a7493">${p.time}</span> ·
   <span style="color:#b9a0dc">MAX EL ${parseFloat(p.maxEl).toFixed(1)}°</span></div>`).join('');
 }catch(e){document.getElementById('passLogBody').innerText='LOAD FAILED';}
}

function processTLEText(txt,btn){
 allTleData=[];
 const L=txt.split('\n');
 for(let i=0;i<L.length-2;i+=3){
  const n=L[i].trim();
  if(n.length>0&&L[i+1].startsWith('1 ')&&L[i+2].startsWith('2 ')){
   if(!n.includes('STARLINK')&&!n.includes('ONEWEB')&&!n.includes('FLOCK'))
    allTleData.push({name:n,l1:L[i+1].trim(),l2:L[i+2].trim()});
  }
 }
 btn.innerText='DB LOADED ('+allTleData.length+' TARGETS)';
 btn.style.color='#78e69a';
 document.getElementById('sat-filter').disabled=false;
 filterSats();
}

function fetchCelestrak(){
 const btn=document.getElementById('btn-fetch');
 const c=localStorage.getItem('celestrakDB'),ct=localStorage.getItem('celestrakTime');
 const now=Date.now();
 if(c&&ct&&(now-ct<14400000)){btn.innerText='LOADING CACHE...';setTimeout(()=>processTLEText(c,btn),400);return;}
 btn.innerText='DOWNLOADING...';
 fetch('https://celestrak.org/NORAD/elements/gp.php?GROUP=active&FORMAT=tle')
 .then(r=>{if(r.status===429)throw new Error("RL");if(!r.ok)throw new Error("NET");return r.text();})
 .then(t=>{localStorage.setItem('celestrakDB',t);localStorage.setItem('celestrakTime',now);processTLEText(t,btn);})
 .catch(e=>{
  btn.innerText=e.message==="RL"?'RATE LIMITED — USING CACHE':'FAILED — USING CACHE';
  if(c)setTimeout(()=>processTLEText(c,btn),1200);
  else{btn.style.color="#ff6b8a";btn.innerText='BLOCKED — NO CACHE';}
 });
}

function filterSats(){
 const q=document.getElementById('sat-filter').value.toLowerCase();
 tleData=q?allTleData.filter(s=>s.name.toLowerCase().includes(q)):allTleData;
 document.getElementById('sat-select').innerHTML=tleData.slice(0,200).map((s,i)=>`<option value="${i}">${s.name}</option>`).join('');
}

function pushTLE(){
 const i=document.getElementById('sat-select').value;
 if(!tleData[i])return;
 const btn=document.getElementById('btn-push');btn.innerText='TRANSMITTING...';
 fetch('/api/tle',{method:'POST',headers:{'Content-Type':'application/json'},
  body:JSON.stringify({name:tleData[i].name,line1:tleData[i].l1,line2:tleData[i].l2})
 }).then(()=>{
  btn.innerText='UPLOAD OK';
  document.getElementById('mode-select').value='1';updateMode();
  setTimeout(()=>{btn.innerText='2 ▸ UPLOAD TLE';fetchPath();},3000);
 });
}

function queueTLE(){
 const i=document.getElementById('sat-select').value;
 if(!tleData[i])return;
 fetch('/api/queue',{method:'POST',headers:{'Content-Type':'application/json'},
  body:JSON.stringify({name:tleData[i].name,line1:tleData[i].l1,line2:tleData[i].l2})
 }).then(r=>r.json()).then(d=>{
  const btn=document.getElementById('btn-queue');
  btn.innerText=d.status==='ok'?'QUEUED!':'QUEUE FULL';
  setTimeout(()=>{btn.innerText='+ QUEUE';},2000);
 });
}

function removeFromQueue(idx){
 fetch('/api/queue/remove',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({idx})});
}

function clearQueue(){
 fetch('/api/queue/clear',{method:'POST'});
}

function updateMode(){
 fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},
  body:JSON.stringify({obMode:document.getElementById('mode-select').value==='1'})});
 setTimeout(fetchPath,1000);
}

function updateGrid(){
 const g=document.getElementById('grid-input').value;
 if(g)fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({grid:g})});
}

function nudge(a,e){
 targetAz=(targetAz+a+360)%360;
 targetEl=Math.max(0,Math.min(90,targetEl+e));
 fetch('/api/manual',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({az:targetAz,el:targetEl})});
}

// ── Radar canvas ───────────────────────────────────────────────
function animateRadar(){
 requestAnimationFrame(animateRadar);
 const cv=document.getElementById('radar'),ctx=cv.getContext('2d');
 const cx=cv.width/2,cy=cv.height/2,r=cx-22;
 ctx.clearRect(0,0,cv.width,cv.height);
 const toXY=(az,el)=>{
  const safeEl=Math.max(0,Math.min(90,el));
  const rr=r*(1-safeEl/90),rad=(az-90)*Math.PI/180;
  return{x:cx+rr*Math.cos(rad),y:cy+rr*Math.sin(rad)};
 };
 // Trajectory
 if(satPath.length>1){
  ctx.strokeStyle='rgba(185,160,220,0.8)';ctx.lineWidth=2;ctx.setLineDash([5,4]);
  ctx.beginPath();
  satPath.forEach((p,i)=>{const q=toXY(p.az,p.el);i?ctx.lineTo(q.x,q.y):ctx.moveTo(q.x,q.y);});
  ctx.stroke();ctx.setLineDash([]);
  const a=toXY(satPath[0].az,satPath[0].el),l=toXY(satPath[satPath.length-1].az,satPath[satPath.length-1].el);
  ctx.font='10px "Share Tech Mono"';ctx.textAlign='center';
  ctx.fillStyle='#78e69a';ctx.fillText('AOS',a.x,a.y-8);
  ctx.fillStyle='#ff6b8a';ctx.fillText('LOS',l.x,l.y-8);
 }
 // Grid
 ctx.strokeStyle='rgba(120,230,154,0.18)';ctx.lineWidth=1;ctx.setLineDash([]);
 [0.33,0.66,1].forEach(f=>{ctx.beginPath();ctx.arc(cx,cy,r*f,0,2*Math.PI);ctx.stroke();});
 ctx.beginPath();ctx.moveTo(cx,cy-r);ctx.lineTo(cx,cy+r);ctx.stroke();
 ctx.beginPath();ctx.moveTo(cx-r,cy);ctx.lineTo(cx+r,cy);ctx.stroke();
 ctx.textAlign='center';ctx.textBaseline='middle';
 for(let i=0;i<360;i+=30){
  const rad=(i-90)*Math.PI/180;
  ctx.strokeStyle=i%90===0?'#78e69a':'#1c2733';ctx.lineWidth=i%90===0?2:1;
  ctx.beginPath();ctx.moveTo(cx+r*Math.cos(rad),cy+r*Math.sin(rad));
  ctx.lineTo(cx+(r+5)*Math.cos(rad),cy+(r+5)*Math.sin(rad));ctx.stroke();
  ctx.font=i%90===0?'bold 12px "Share Tech Mono"':'9px "Share Tech Mono"';
  ctx.fillStyle=i%90===0?'#78e69a':'#2e2a44';
  if(i===0)ctx.fillText('N',cx,cy-r-13);
  else if(i===90)ctx.fillText('E',cx+r+13,cy);
  else if(i===180)ctx.fillText('S',cx,cy+r+13);
  else if(i===270)ctx.fillText('W',cx-r-13,cy);
  else ctx.fillText(i,cx+(r+14)*Math.cos(rad),cy+(r+14)*Math.sin(rad));
 }
 // Sweep
 sweepAngle+=0.025;
 ctx.fillStyle='rgba(120,230,154,0.08)';
 ctx.beginPath();ctx.moveTo(cx,cy);ctx.arc(cx,cy,r,sweepAngle,sweepAngle+0.5);ctx.lineTo(cx,cy);ctx.fill();
 ctx.strokeStyle='rgba(120,230,154,0.6)';ctx.lineWidth=1.5;ctx.setLineDash([]);
 ctx.beginPath();ctx.moveTo(cx,cy);ctx.lineTo(cx+r*Math.cos(sweepAngle+0.5),cy+r*Math.sin(sweepAngle+0.5));ctx.stroke();
 // Satellite footprint circle (feature 13)
 if(targetEl>0&&satFootprintRadius>0){
  const sp=toXY(targetAz,targetEl);
  // Convert footprint radius (km) to radar pixels roughly
  // footprint shown as fraction of the radar circle based on elevation
  const fpPx=Math.max(8,Math.min(r*0.8,(satFootprintRadius/12000)*r));
  ctx.strokeStyle='rgba(120,230,154,0.15)';ctx.lineWidth=1;ctx.setLineDash([3,3]);
  ctx.beginPath();ctx.arc(sp.x,sp.y,fpPx,0,2*Math.PI);ctx.stroke();
  ctx.setLineDash([]);
 }
 // Target arrow
 if(targetEl>0){
  const tp=toXY(targetAz,targetEl);
  ctx.strokeStyle='rgba(185,160,220,0.9)';ctx.lineWidth=1.5;ctx.setLineDash([6,4]);
  ctx.beginPath();ctx.moveTo(cx,cy);ctx.lineTo(tp.x,tp.y);ctx.stroke();
  ctx.setLineDash([]);
  const ds=6;
  ctx.strokeStyle='#b9a0dc';ctx.lineWidth=1.5;
  ctx.beginPath();ctx.moveTo(tp.x,tp.y-ds);ctx.lineTo(tp.x+ds,tp.y);
  ctx.lineTo(tp.x,tp.y+ds);ctx.lineTo(tp.x-ds,tp.y);ctx.closePath();ctx.stroke();
  ctx.fillStyle='rgba(185,160,220,0.3)';ctx.fill();
  ctx.font='9px "Share Tech Mono"';ctx.textAlign='left';ctx.textBaseline='top';
  ctx.fillStyle='#b9a0dc';
  ctx.fillText('TGT',tp.x+(tp.x>=cx?8:-26),tp.y+(tp.y>=cy?8:-16));
  if(isGeo){ctx.textAlign='center';ctx.fillText('GEO',tp.x,tp.y-18);}
 }
 // Actual needle
 const safeEl=Math.max(0,currentEl);
 const ap=toXY(currentAz,safeEl);
 ctx.strokeStyle='rgba(200,214,221,0.8)';ctx.lineWidth=2;ctx.setLineDash([]);
 ctx.beginPath();ctx.moveTo(cx,cy);ctx.lineTo(ap.x,ap.y);ctx.stroke();
 ctx.fillStyle='#c8d6dd';ctx.beginPath();ctx.arc(ap.x,ap.y,5,0,2*Math.PI);ctx.fill();
 ctx.font='9px "Share Tech Mono"';ctx.textAlign='left';ctx.textBaseline='top';
 ctx.fillStyle='#c8d6dd';
 ctx.fillText('ANT',ap.x+(ap.x>=cx?7:-25),ap.y+(ap.y>=cy?7:-15));
 // Satellite dot
 if(targetEl>0){
  const sp=toXY(targetAz,targetEl);
  const sc=isGeo?'#b9a0dc':'#78e69a';
  ctx.fillStyle=sc;ctx.shadowBlur=16;ctx.shadowColor=sc;
  ctx.beginPath();ctx.arc(sp.x,sp.y,6,0,2*Math.PI);ctx.fill();
  ctx.shadowBlur=0;
  ctx.strokeStyle=sc;ctx.lineWidth=1;ctx.beginPath();ctx.arc(sp.x,sp.y,10,0,2*Math.PI);ctx.stroke();
 }
 // Parked indicator
 if(isParked){
  ctx.fillStyle='rgba(255,107,138,0.15)';ctx.beginPath();ctx.arc(cx,cy,r,0,2*Math.PI);ctx.fill();
  ctx.font='bold 11px "Share Tech Mono"';ctx.textAlign='center';ctx.textBaseline='middle';
  ctx.fillStyle='#ff6b8a';ctx.fillText('PARKED',cx,cy+r*0.6);
 }
}

setInterval(fetchTelemetry,500);
setInterval(fetchPath,60000);
setTimeout(fetchPath,2000);
setTimeout(loadSchedule,4000);
animateRadar();
</script></body></html>
)rawliteral";

// ================= WIFI PORTAL =================
const char wifi_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>NETWORK SETUP</title><style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:sans-serif;background:#060a0d;color:#c8d6dd;display:flex;align-items:center;justify-content:center;height:100vh;padding:20px}
.panel{background:#0b1216;border:1px solid #2e2a44;padding:32px;width:100%;max-width:400px;text-align:center}
h2{color:#78e69a;margin-bottom:18px}p{color:#7a7493;font-size:.9rem;margin-bottom:22px}
input{width:100%;padding:14px;margin-bottom:14px;background:#060c10;border:1px solid #2e2a44;color:#c8d6dd;font-size:1rem;outline:none}
button{width:100%;padding:14px;background:rgba(120,230,154,.07);color:#78e69a;border:1px solid #78e69a;cursor:pointer;font-size:1rem}
</style></head><body>
<div class="panel" id="p"><h2>NETWORK SETUP</h2>
<p>Enter credentials to restore connection.</p>
<input type="text" id="s" placeholder="WIFI SSID">
<input type="password" id="p2" placeholder="PASSWORD">
<button onclick="sv()">SAVE & REBOOT</button></div>
<script>
function sv(){
 const s=document.getElementById('s').value,p=document.getElementById('p2').value;
 if(!s||!p){alert('Fill both fields');return;}
 document.getElementById('p').innerHTML='<h2>REBOOTING</h2><p>Return to your network.</p>';
 fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:s,pass:p})});
}
</script></body></html>
)rawliteral";

// ================= PINS & COLORS =================
#define TFT_CS    15
#define TFT_DC     2
#define TFT_RST    4
#define AZ_STEP   32
#define AZ_DIR    14
#define EL_STEP   27
#define EL_DIR    26
#define ENABLE_PIN 25

// Palette: PURPLE primary (chrome/titles/frames), GREEN accent
// (values/OK/radar/sat dot), near-black background. (B)
// Safe RGB565 macro — no hand-computed bit math (avoids off-color bugs).
#define RGB565(r,g,b) ((uint16_t)((((uint16_t)(r)&0xF8)<<8)|(((uint16_t)(g)&0xFC)<<3)|(((uint16_t)(b))>>3)))

#define C_BLACK  0x0000
#define C_WHITE  0xFFFF
#define C_PRIMARY RGB565(190,160,225)   // light purple — titles, frames, target
#define C_ACCENT  RGB565(130,232,156)   // light green — values, OK, radar, sat dot
#define C_NEON    C_ACCENT              // alias kept: green = accent
#define C_GREEN   RGB565( 60,220,120)
#define C_LIME    RGB565(170,240,180)
#define C_AMBER   C_PRIMARY             // alias kept: amber→purple (TGT/EL/titles)
#define C_ORANGE  RGB565(255,178,140)   // soft warning (countdown)
#define C_RED     RGB565(255,112,130)
#define C_CYAN    C_PRIMARY             // footprint screen chrome → purple
#define C_MGRAY   RGB565(125,120,142)   // muted text (slightly purple-tinted)
#define C_DGRAY   RGB565( 56,50,78)     // frame/border (purple-tinted dark)
#define C_FAINT   RGB565( 36,30,56)
#define C_HDRBG   RGB565( 28,20,44)     // dark purple header bar
#define C_PNLG    RGB565( 24,44,30)     // dark green panel-title fill
#define C_PNLA    RGB565( 32,24,52)     // dark purple panel-title fill
#define C_RING    RGB565( 72,60,104)    // radar ring (purple-tint)
#define C_DRED    RGB565( 80,20,40)

// ================= OBJECTS =================
Adafruit_ST7789 tft      = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
AccelStepper    azStepper(AccelStepper::DRIVER, AZ_STEP, AZ_DIR);
AccelStepper    elStepper(AccelStepper::DRIVER, EL_STEP, EL_DIR);
AsyncWebServer  webServer(80);
AsyncWebSocket  ws("/ws");
WiFiServer      tcpServer(4533);
WiFiClient      tcpClient;
WiFiUDP         ntpUDP;
NTPClient       timeClient(ntpUDP,"pool.ntp.org",0,60000);
Preferences     prefs;
Sgp4            sat;
static Sgp4     predSat;
static Sgp4     schedSat;  // dedicated instance for schedule computation

// ================= STATE =================
char  tleLine1[70]="";
char  tleLine2[70]="";
char  satName[25] ="NO SAT";
volatile bool   tleLoaded    =false;
volatile bool   spiLock      =false;
volatile float  currentAz    =0,currentEl=0;
volatile float  targetAz     =0,targetEl =0;
volatile bool   isMoving     =false;
const float     STEPS_PER_DEG=8.88f;
volatile bool   onboardMode  =false;
volatile bool   isAPMode     =false;
volatile bool   triggerReboot=false;
volatile bool   isGeoSat     =false;
double obsLat=31.52,obsLon=74.35,obsAlt=0.21;
String maidenhead="MM71dl";
volatile bool   ntpSynced    =false;
volatile double lastSatDist  =0;
volatile float  dopplerFreq  =0,satDistance=0;
volatile float  satFootprintKm=0;  // current satellite footprint radius km
volatile float  satAltitude  =0;  // (A4) true orbital altitude km, for footprint screen
unsigned long   lastDopplerTime=0,lastSGP4Update=0;
volatile time_t nextAosTime  =0,nextLosTime=0;
volatile float  nextMaxEl    =0;
volatile bool   predBusy     =false;

struct PassLogEntry{char sat[25];char time[20];float maxEl;};
PassLogEntry passLog[10];
int   passLogCount  =0;
bool  passInProgress=false;
float passMaxEl     =0;
char  passStartBuf[20]="";

struct PathPoint{float az,el;};
PathPoint     globalPath[45];
volatile int  globalPathLen=0;
unsigned long lastPathCalc =0;
volatile bool newPathReady =false;

// ---- TFT cache ----
float prev_cAz,prev_cEl,prev_tAz,prev_tEl,prev_dist,prev_dop;
int   prev_rssi,prev_heap;
int   prev_moving,prev_ntp,prev_l4s,prev_mode,prev_geo;
char  prev_sat[25],prev_clock[10];
long  prev_countdown;
float prev_maxElShown;
int   prev_antX,prev_antY,prev_tgtX,prev_tgtY,prev_satX,prev_satY;
int   prev_alX[2],prev_alY[2];
int   terminalY=30;
TaskHandle_t Core0Task;
const int RCX=120,RCY=228,RR=55;

inline int clampX(int x){return x<RADAR_SAFE_LEFT?RADAR_SAFE_LEFT:x>RADAR_SAFE_RIGHT?RADAR_SAFE_RIGHT:x;}
inline int clampY(int y){return y<RADAR_SAFE_TOP?RADAR_SAFE_TOP:y>RADAR_SAFE_BOT?RADAR_SAFE_BOT:y;}

// ================= IMU =================
bool mpuBegin(){
  Wire.begin(MPU_SDA,MPU_SCL);Wire.setClock(400000);
  Wire.beginTransmission(MPU_ADDR);Wire.write(0x6B);Wire.write(0x00);
  if(Wire.endTransmission(true)!=0)return false;
  delay(200);
  Wire.beginTransmission(MPU_ADDR);Wire.write(0x1A);Wire.write(MPU_DLPF_CFG&0x07);Wire.endTransmission(true);
  Wire.beginTransmission(MPU_ADDR);Wire.write(0x1B);Wire.write(0x00);Wire.endTransmission(true);
  Wire.beginTransmission(MPU_ADDR);Wire.write(0x1C);Wire.write(0x00);Wire.endTransmission(true);
  Wire.beginTransmission(MPU_ADDR);Wire.write(0x75);Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)MPU_ADDR,(uint8_t)1,(uint8_t)true);
  if(!Wire.available())return false;
  uint8_t who=Wire.read();
  if(who!=0x68&&who!=0x70&&who!=0x71&&who!=0x72)return false;

  Serial.println("[IMU] Calibrating...");delay(500);
  long sGx=0,sGy=0,sAx=0,sAy=0,sAz=0;int n=0;
  for(int i=0;i<200;i++){
    Wire.beginTransmission(MPU_ADDR);Wire.write(0x3B);Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)MPU_ADDR,(uint8_t)14,(uint8_t)true);
    if(Wire.available()<14){delay(5);continue;}
    int16_t ax=(Wire.read()<<8)|Wire.read();
    int16_t ay=(Wire.read()<<8)|Wire.read();
    int16_t az=(Wire.read()<<8)|Wire.read();
    Wire.read();Wire.read();
    int16_t gx=(Wire.read()<<8)|Wire.read();
    int16_t gy=(Wire.read()<<8)|Wire.read();
    Wire.read();Wire.read();
    sAx+=ax;sAy+=ay;sAz+=az;sGx+=gx;sGy+=gy;n++;delay(5);
  }
  if(n<20)return false;
  _gBiasX=(sGx/(float)n)/131.0f;_gBiasY=(sGy/(float)n)/131.0f;
  float mAx=sAx/(float)n,mAy=sAy/(float)n,mAz=sAz/(float)n;
  _aBiasX=mAx;_aBiasY=mAy;_aBiasZ=mAz-16384.0f;
  _lpfAx=mAx-_aBiasX;_lpfAy=mAy-_aBiasY;_lpfAz=mAz-_aBiasZ;
  _seeded=true;
  float bootPitch=atan2f(_lpfAx,sqrtf(_lpfAy*_lpfAy+_lpfAz*_lpfAz))*180.0f/PI;
  float bootRoll =atan2f(_lpfAy,_lpfAz)*180.0f/PI;
  kalPitch.seed(bootPitch,_gBiasX);
  kalRoll.seed(bootRoll,_gBiasY);
  imuPitch=bootPitch;imuRoll=bootRoll;imuOK=true;
  _lastUs=micros();
  Serial.printf("[IMU] Boot pitch=%.2f roll=%.2f\n",imuPitch,imuRoll);
  return true;
}

void mpuRead(){
  Wire.beginTransmission(MPU_ADDR);Wire.write(0x3B);
  if(Wire.endTransmission(false)!=0){imuOK=false;return;}
  Wire.requestFrom((uint8_t)MPU_ADDR,(uint8_t)14,(uint8_t)true);
  if(Wire.available()<14){imuOK=false;return;}
  int16_t raw_ax=(Wire.read()<<8)|Wire.read();
  int16_t raw_ay=(Wire.read()<<8)|Wire.read();
  int16_t raw_az=(Wire.read()<<8)|Wire.read();
  Wire.read();Wire.read();
  int16_t raw_gx=(Wire.read()<<8)|Wire.read();
  int16_t raw_gy=(Wire.read()<<8)|Wire.read();
  Wire.read();Wire.read();

  float ax=(float)raw_ax-_aBiasX,ay=(float)raw_ay-_aBiasY,az=(float)raw_az-_aBiasZ;
  float gx=((float)raw_gx/131.0f)-_gBiasX,gy=((float)raw_gy/131.0f)-_gBiasY;
  if(fabsf(gx)<GYRO_DEAD)gx=0;if(fabsf(gy)<GYRO_DEAD)gy=0;
  if(_seeded){
    if(fabsf(ax-_lpfAx)>SPIKE_LSB)ax=_lpfAx;
    if(fabsf(ay-_lpfAy)>SPIKE_LSB)ay=_lpfAy;
    if(fabsf(az-_lpfAz)>SPIKE_LSB)az=_lpfAz;
  }
  _lpfAx=ACCEL_LPF*ax+(1-ACCEL_LPF)*_lpfAx;
  _lpfAy=ACCEL_LPF*ay+(1-ACCEL_LPF)*_lpfAy;
  _lpfAz=ACCEL_LPF*az+(1-ACCEL_LPF)*_lpfAz;

  float accPitch=atan2f(_lpfAx,sqrtf(_lpfAy*_lpfAy+_lpfAz*_lpfAz))*180.0f/PI;
  float accRoll =atan2f(_lpfAy,_lpfAz)*180.0f/PI;

  unsigned long nowUs=micros();
  float dt=(nowUs-_lastUs)*1e-6f;
  _lastUs=nowUs;
  if(dt<0.0005f||dt>0.5f)dt=0.02f;

  // Kalman filter update (replaces complementary filter)
  imuPitch=kalPitch.update(accPitch,gx,dt);
  imuRoll =kalRoll.update(accRoll, gy,dt);

  // Record error history for drift graph
  float err=imuPitch-(float)currentEl;
  errHistory[errHistIdx]=err;
  errHistIdx=(errHistIdx+1)%ERR_HIST;
  if(errHistIdx==0)errHistFull=true;

  imuOK=true;
}

void mpuCorrect(volatile float &cel,AccelStepper &els,float spd,bool mov){
#if IMU_CORRECT_ENABLED
  if(!imuOK||mov)return;
  float ph=(imuPitch<0)?0:imuPitch;
  float er=ph-(float)cel;
  if(fabsf(er)>IMU_CORRECT_THRESH){
    Serial.printf("[IMU] Correct err=%.2f\n",er);
    els.setCurrentPosition((long)(ph*spd));cel=ph;
  }
#else
  (void)cel;(void)els;(void)spd;(void)mov;
#endif
}

// ================= SATELLITE FOOTPRINT (feature 13) =================
// Returns the ground radius (km) of the satellite's visibility circle:
// r = R_earth * acos(R/(R+h)), with h = TRUE altitude in km.
// BUG FIX (A4): the old version passed slant range (satDist) and did
// (satDist-6371) to fake an altitude — that is geometrically wrong and
// made the footprint ring shrink/grow as the sat moved across the sky.
// The Sgp4 lib exposes the true altitude as sat.satAlt (sgp4pred.h:81),
// so callers now pass that directly.
float computeFootprintKm(double altKm){
  float alt=(float)altKm;
  if(alt<100.0f)alt=100.0f;     // clamp to a sane LEO floor
  float R=6371.0f;
  float footKm=R*acosf(R/(R+alt));
  return footKm;
}

// ================= WEATHER (feature 14) =================
// Copy up to n-1 chars from src into dst, then ALWAYS null-terminate,
// and strip non-ASCII bytes (the Adafruit_GFX font has no UTF-8 glyphs,
// so accented city names like "São Paulo" / "München" would render as
// garbage). BUG FIX (A5): the old strncpy() left the buffer
// un-terminated when the source was >=31 chars, so snprintf("%s",city)
// read past the buffer → the glitched/flickering city name on screen.
static void copyAscii(char *dst,const char *src,size_t n){
  if(n==0)return;
  size_t i=0;
  if(src){
    for(;i<n-1 && src[i];i++){
      char c=src[i];
      // keep printable ASCII only; replace everything else with a space
      dst[i]=(c>=0x20 && c<0x7f)?c:' ';
    }
  }
  dst[i]='\0';   // always terminate
}

void fetchWeather(){
  if(isAPMode||!ntpSynced)return;
  #ifndef OWM_API_KEY
  return;
  #endif
  String url="http://api.openweathermap.org/data/2.5/weather?lat="+
             String(obsLat,4)+"&lon="+String(obsLon,4)+
             "&units=metric&appid=" OWM_API_KEY;
  HTTPClient http;
  http.begin(url);
  http.setTimeout(8000);
  int code=http.GET();
  if(code==200){
    String body=http.getString();
    StaticJsonDocument<1024> doc;
    if(!deserializeJson(doc,body)){
      weather.tempC   =doc["main"]["temp"]|0.0f;
      weather.feelsC  =doc["main"]["feels_like"]|weather.tempC;   // (D)
      weather.windMps =doc["wind"]["speed"]|0.0f;
      weather.windDeg =doc["wind"]["deg"]|0.0f;
      weather.humidity=doc["main"]["humidity"]|0;
      weather.pressure=doc["main"]["pressure"]|0;                 // (D)
      weather.visibility=doc["visibility"]|0;                     // (D)
      const char* d=doc["weather"][0]["description"]|"---";
      const char* c=doc["name"]|"---";
      copyAscii(weather.desc,d,sizeof(weather.desc));   // (A5) terminate + ASCII
      copyAscii(weather.city,c,sizeof(weather.city));
      weather.fetchedAt=timeClient.getEpochTime();
      weather.valid=true;
      Serial.printf("[WX] %s %.1f°C feels %.1f wind %.1fm/s\n",
                    weather.city,weather.tempC,weather.feelsC,weather.windMps);
    }
  } else {
    Serial.printf("[WX] HTTP %d\n",code);
  }
  http.end();
}

// (D) 16-point compass label from a wind bearing in degrees.
static const char* windCompass(float deg){
  static const char* lbl[16]={"N","NNE","NE","ENE","E","ESE","SE","SSE",
                              "S","SSW","SW","WSW","W","WNW","NW","NNW"};
  int i=((int)((deg+11.25f)/22.5f))%16;
  return lbl[i];
}

// ================= AUTO-PARK (feature 1) =================
void checkAutopark(){
  if(isAPMode||!onboardMode)return;
  bool passActive=(nextAosTime>0&&(time_t)timeClient.getEpochTime()>=nextAosTime&&
                   (time_t)timeClient.getEpochTime()<=nextLosTime);
  if(passActive){lastActiveMs=millis();parked=false;return;}
  if(isMoving){lastActiveMs=millis();}
  if(!parked&&millis()-lastActiveMs>PARK_IDLE_SEC*1000UL){
    parked=true;
    targetAz=PARK_AZ;targetEl=PARK_EL;
    Serial.println("[PARK] Parking antenna — idle timeout");
  }
}

// ================= SATELLITE QUEUE (feature 4) =================
bool tleIsGeo();   // forward decl: defined later, used by advanceQueue()
void advanceQueue(){
  if(queueCount<=1)return;
  // Rotate: remove head, shift down
  for(int i=0;i<queueCount-1;i++) satQueue[i]=satQueue[i+1];
  queueCount--;queueHead=0;
  // Load next satellite
  if(queueCount>0&&satQueue[0].valid){
    strncpy(satName,satQueue[0].name,24);
    strncpy(tleLine1,satQueue[0].line1,69);
    strncpy(tleLine2,satQueue[0].line2,69);
    sat.site(obsLat,obsLon,obsAlt);
    sat.init(satName,tleLine1,tleLine2);
    isGeoSat=tleIsGeo();tleLoaded=true;
    nextAosTime=nextLosTime=0;lastPathCalc=0;newPathReady=true;
    Serial.printf("[QUEUE] Advanced to: %s\n",satName);
  }
}

// ================= PASS SCHEDULE (feature 6) =================
// Runs on Core0 task. Scans next 24h for all queued satellites.
void computeSchedule(){
  if(scheduleBusy||!ntpSynced||isAPMode)return;
  scheduleBusy=true;scheduleCount=0;scheduleReady=false;
  unsigned long nowT=timeClient.getEpochTime();
  if(nowT<1000000000UL){scheduleBusy=false;return;}

  for(int q=0;q<queueCount&&scheduleCount<MAX_SCHEDULED_PASSES;q++){
    if(!satQueue[q].valid)continue;
    schedSat.site(obsLat,obsLon,obsAlt);
    schedSat.init(satQueue[q].name,satQueue[q].line1,satQueue[q].line2);

    unsigned long t=nowT,limit=nowT+86400;
    while(t<limit&&scheduleCount<MAX_SCHEDULED_PASSES){
      schedSat.findsat(t);
      if(schedSat.satEl>0){
        // Back up to AOS
        int lim=0;
        while(schedSat.satEl>0&&lim++<120){t-=30;schedSat.findsat(t);}
        t+=30;
        time_t aosT=(time_t)t;
        float maxEl=0;float aosAz=(float)schedSat.satAz;
        // Scan pass
        unsigned long tt=t;
        while(true){
          schedSat.findsat(tt);
          if(schedSat.satEl<0)break;
          if(schedSat.satEl>maxEl)maxEl=schedSat.satEl;
          tt+=30;
          if(tt-t>7200)break; // safety
        }
        // Record
        strncpy(schedule[scheduleCount].satName,satQueue[q].name,24);
        schedule[scheduleCount].aos=aosT;
        schedule[scheduleCount].los=(time_t)tt;
        schedule[scheduleCount].maxEl=maxEl;
        schedule[scheduleCount].aosAz=aosAz;
        scheduleCount++;
        t=tt+60; // skip past this pass
      } else {
        t+=60;
      }
      vTaskDelay(pdMS_TO_TICKS(1)); // yield
    }
  }
  // Sort by AOS time
  for(int i=0;i<scheduleCount-1;i++){
    for(int j=i+1;j<scheduleCount;j++){
      if(schedule[j].aos<schedule[i].aos){
        ScheduledPass tmp=schedule[i];schedule[i]=schedule[j];schedule[j]=tmp;
      }
    }
  }
  scheduleReady=true;scheduleBusy=false;
  Serial.printf("[SCHED] %d passes computed\n",scheduleCount);
}

// ================= HELPERS =================
bool tleIsGeo(){
  if(strlen(tleLine2)<63)return false;
  double mm=atof(tleLine2+52);return(mm>0.1&&mm<2.0);
}

void maidenheadToLatLon(String grid,double &lat,double &lon){
  grid.toUpperCase();if(grid.length()<4)return;
  lon=(grid[0]-'A')*20.0-180.0;lat=(grid[1]-'A')*10.0-90.0;
  lon+=(grid[2]-'0')*2.0;lat+=(grid[3]-'0')*1.0;
  if(grid.length()>=6){lon+=((tolower(grid[4])-'a')*5.0)/60.0;lat+=((tolower(grid[5])-'a')*2.5)/60.0;}
  lon+=1.0;lat+=0.5;
}

void resetTftCache(){
  prev_cAz=prev_cEl=prev_tAz=prev_tEl=-999;prev_dist=prev_dop=-1e9;
  prev_rssi=999;prev_heap=-1;prev_moving=prev_ntp=prev_l4s=prev_mode=prev_geo=-1;
  prev_sat[0]='\0';prev_clock[0]='\0';prev_countdown=-999999;prev_maxElShown=-999;
  prev_antX=prev_antY=prev_tgtX=prev_tgtY=prev_satX=prev_satY=-1;
  prev_alX[0]=prev_alX[1]=-1;
}

String buildTelemetryJson(){
  // BUG FIX (A1): 768 was too small — a full 8-sat queue + weather +
  // 24-char sat name overflowed it, corrupting the JSON and freezing the
  // web UI (await r.json() throws → queue list never updates/clears).
  StaticJsonDocument<1536> doc;
  doc["tAz"]=(float)targetAz;doc["tEl"]=(float)targetEl;
  doc["cAz"]=(float)currentAz;doc["cEl"]=(float)currentEl;
  doc["isMoving"]=isMoving;doc["rssi"]=isAPMode?0:WiFi.RSSI();
  doc["freeHeap"]=ESP.getFreeHeap()/1024;doc["uptime"]=millis()/1000;
  doc["mode"]=onboardMode?1:0;doc["sat"]=satName;doc["grid"]=maidenhead;
  doc["doppler"]=(float)dopplerFreq;doc["dist"]=(float)satDistance;
  doc["geo"]=isGeoSat;doc["parked"]=parked;
  doc["imuPitch"]=(float)imuPitch;doc["imuRoll"]=(float)imuRoll;doc["imuOK"]=imuOK;
  doc["footprintR"]=(float)satFootprintKm;
  // Weather
  doc["wxValid"]=weather.valid;
  if(weather.valid){
    doc["wxTemp"]=weather.tempC;doc["wind"]=weather.windMps;
    doc["wxWindDeg"]=weather.windDeg;doc["wxHum"]=weather.humidity;
    doc["wxDesc"]=weather.desc;doc["wxCity"]=weather.city;
    doc["wxFeels"]=weather.feelsC;doc["wxPressure"]=weather.pressure;   // (D)
    doc["wxVis"]=weather.visibility;                                     // (D)
  }
  // Queue summary
  JsonArray qa=doc.createNestedArray("queue");
  for(int i=0;i<queueCount;i++){
    JsonObject o=qa.createNestedObject();
    o["name"]=satQueue[i].name;o["active"]=(i==queueHead);
  }
  String out;serializeJson(doc,out);return out;
}

// ================= FORWARD DECLARATIONS =================
void parseEasyComm(String cmd);
void runSGP4();
void calculatePathPrediction();
void tftDrawStaticFrame();
void tftDrawAPMode();
void tftUpdateDynamic();
void tftRadarUpdate();
void tftDrawScheduleScreen();
void tftDrawDriftGraph();
void tftDrawFootprintScreen();
void tftDrawWeatherScreen();
void setupWebServer();

// ================= BOOT SCREENS =================
void drawBootScreen(){
  tft.fillScreen(C_BLACK);
  // purple header bar, green accent line (B/C)
  tft.fillRect(0,0,240,28,C_HDRBG);tft.fillRect(0,28,240,2,C_ACCENT);
  tft.fillRect(0,0,4,28,C_ACCENT);tft.fillRect(236,0,4,28,C_PRIMARY);
  tft.setTextColor(C_ACCENT);tft.setTextSize(1);
  tft.setCursor(8,5);tft.print("ESP32 SATELLITE TRACKER");
  tft.setTextColor(C_PRIMARY);tft.setCursor(8,16);tft.print("v10.0  //  ORBITAL OPS");
  tft.setTextColor(C_MGRAY);tft.setCursor(8,38);tft.print("DEVELOPER:");
  tft.setTextColor(C_WHITE);tft.setCursor(8,50);tft.print("Muhammad Uzzam Butt");
  tft.drawFastHLine(4,64,232,C_DGRAY);
  tft.setTextColor(C_MGRAY);tft.setCursor(8,70);tft.print("SCAN FOR SOURCE CODE:");
  const char* url="https://github.com/uzzambutt/ESP32-Sattelite-Tracker";
  uint8_t *qr=(uint8_t*)malloc(qrcodegen_BUFFER_LEN_MAX);
  uint8_t *tmp=(uint8_t*)malloc(qrcodegen_BUFFER_LEN_MAX);
  if(qr&&tmp){
    if(qrcodegen_encodeText(url,tmp,qr,qrcodegen_Ecc_LOW,
       qrcodegen_VERSION_MIN,qrcodegen_VERSION_MAX,qrcodegen_Mask_AUTO,true)){
      int qs=qrcodegen_getSize(qr),ps=3;
      int ox=(240-qs*ps)/2,oy=84;
      tft.fillRect(ox-4,oy-4,qs*ps+8,qs*ps+8,C_WHITE);
      for(int y=0;y<qs;y++)
        for(int x=0;x<qs;x++)
          if(qrcodegen_getModule(qr,x,y))
            tft.fillRect(ox+x*ps,oy+y*ps,ps,ps,C_BLACK);
    }
  }
  if(qr)free(qr);if(tmp)free(tmp);
  // tty-style footer prompt
  tft.fillRect(0,300,240,20,C_HDRBG);tft.fillRect(0,298,240,2,C_ACCENT);
  tft.setTextColor(C_PRIMARY);tft.setCursor(8,307);tft.print("user@orbital");
  tft.setTextColor(C_MGRAY);tft.setCursor(96,307);tft.print(":~$ boot");
}

// ── Linux-tty style boot log (C) ───────────────────────────────
// Each line prints a dmesg-ish status tag then the message, on a black
// terminal. Auto-scrolls when it hits the bottom. A blinking block
// cursor sits after the last line like a real VT.
//   status: 0=OK(green) 1=FAILED(red) 2=BUSY/..(purple) 3=WARN(orange) 4=info(white)
void printBootLine(String msg,int status){
  Serial.println("[BOOT] "+msg);
  const int lineH=11, topY=28, botY=300, headerH=22;
  if(terminalY+lineH>botY){
    // scroll: clear region and restart under a fresh tty header
    tft.fillScreen(C_BLACK);
    tft.fillRect(0,0,240,headerH,C_HDRBG);
    tft.fillRect(0,headerH,240,2,C_ACCENT);
    tft.setTextColor(C_ACCENT);tft.setTextSize(1);
    tft.setCursor(6,7);tft.print("orbital tty1");
    tft.setTextColor(C_MGRAY);tft.setCursor(170,7);tft.print("boot log");
    tft.drawFastHLine(0,headerH+2,240,C_DGRAY);
    terminalY=topY;
  }
  tft.setTextSize(1);
  // status tag in fixed-width bracket
  const char* tag; uint16_t tc;
  switch(status){
    case 0: tag="[  OK  ]"; tc=C_ACCENT; break;
    case 1: tag="[FAILED]"; tc=C_RED;    break;
    case 2: tag="[  ..  ]"; tc=C_PRIMARY;break;
    case 3: tag="[ WARN ]"; tc=C_ORANGE; break;
    default:tag="[ INFO ]"; tc=C_MGRAY;  break;
  }
  tft.setTextColor(tc);tft.setCursor(4,terminalY);tft.print(tag);
  // message — truncate to fit ~26 chars after the 52px tag
  tft.setTextColor(C_WHITE);tft.setCursor(54,terminalY);
  if(msg.length()>26)msg=msg.substring(0,26);
  tft.print(msg);
  terminalY+=lineH;
  // blinking block cursor on the next prompt line
  tft.setTextColor(C_ACCENT);tft.setCursor(4,terminalY);tft.print("_");
  delay(70);
  tft.fillRect(4,terminalY,6,8,C_BLACK);  // blink off
}

// legacy entry point — kept so existing call sites compile; maps to INFO
void printBootTerminal(String msg){ printBootLine(msg,4); }

void tftDrawAPMode(){
  tft.fillScreen(C_BLACK);
  tft.fillRect(0,0,240,24,C_DRED);tft.fillRect(0,0,4,24,C_RED);tft.fillRect(236,0,4,24,C_RED);
  tft.setTextColor(C_WHITE);tft.setTextSize(1);tft.setCursor(8,5);tft.print("! NETWORK LINK FAILED !");
  tft.setTextColor(C_AMBER);tft.setCursor(8,15);tft.print("ENTERING SETUP MODE");
  tft.drawFastHLine(0,24,240,C_RED);tft.setTextColor(C_RED);tft.setTextSize(2);
  tft.setCursor(10,32);tft.print("WIFI FAIL");
  tft.drawRoundRect(2,58,236,128,4,C_AMBER);tft.fillRect(3,59,234,12,C_PNLA);
  tft.setTextColor(C_AMBER);tft.setTextSize(1);tft.setCursor(6,61);tft.print("[ SETUP INSTRUCTIONS ]");
  tft.drawFastHLine(3,71,234,C_DGRAY);
  tft.setTextColor(C_AMBER);tft.setCursor(6,76);tft.print("1.");
  tft.setTextColor(C_MGRAY);tft.setCursor(20,76);tft.print("Join WiFi: AEROSPACE-TRACKER");
  tft.setTextColor(C_AMBER);tft.setCursor(6,92);tft.print("2.");
  tft.setTextColor(C_MGRAY);tft.setCursor(20,92);tft.print("Password: groundstation");
  tft.setTextColor(C_AMBER);tft.setCursor(6,108);tft.print("3.");
  tft.setTextColor(C_MGRAY);tft.setCursor(20,108);tft.print("Browse to: 192.168.4.1");
  tft.drawRoundRect(2,150,236,24,4,C_DGRAY);
  tft.setTextColor(C_AMBER);tft.setCursor(6,158);tft.print("WAITING FOR CREDENTIALS...");
  tft.fillRect(0,300,240,20,C_DRED);tft.setTextColor(C_WHITE);tft.setCursor(8,307);tft.print("AP MODE  |  192.168.4.1");
}

// ================= STATIC HUD FRAME =================
void tftDrawStaticFrame(){
  tft.fillScreen(C_BLACK);resetTftCache();
  tft.fillRect(0,0,240,16,C_HDRBG);tft.fillRect(0,16,240,2,C_NEON);tft.fillRect(0,0,3,16,C_AMBER);
  tft.setTextColor(C_NEON);tft.setTextSize(1);tft.setCursor(7,4);tft.print("ORBITAL OPS");
  tft.setTextColor(C_MGRAY);tft.setCursor(96,4);tft.print("UTC");
  tft.drawRect(2,20,236,54,C_NEON);tft.fillRect(3,21,234,11,C_PNLG);
  tft.setTextColor(C_NEON);tft.setCursor(6,23);tft.print("TGT");
  tft.drawFastHLine(3,32,234,C_FAINT);tft.drawFastVLine(118,33,40,C_FAINT);
  tft.setTextColor(C_MGRAY);tft.setCursor(6,40);tft.print("AZ");tft.setCursor(124,40);tft.print("EL");
  const char* cl[5]={"ENG","MOT","L4S","NTP","MEM"};
  for(int i=0;i<5;i++){int x=2+i*48;tft.drawRect(x,78,44,22,C_DGRAY);tft.fillRect(x+1,79,42,8,C_FAINT);tft.setTextColor(C_MGRAY);tft.setCursor(x+13,79);tft.print(cl[i]);}
  tft.drawRect(2,104,236,42,C_AMBER);tft.fillRect(3,105,234,11,C_PNLA);
  tft.setTextColor(C_AMBER);tft.setCursor(6,107);tft.print("PASS PREDICTION");
  tft.drawFastHLine(3,116,234,C_FAINT);tft.drawFastVLine(118,117,28,C_FAINT);
  tft.setTextColor(C_MGRAY);
  tft.setCursor(6,120);tft.print("AOS:");tft.setCursor(124,120);tft.print("MAX:");
  tft.setCursor(6,133);tft.print("LOS:");tft.setCursor(124,133);tft.print("T- :");
  tft.setCursor(6,151);tft.print("DST:");tft.setCursor(124,151);tft.print("DOP:");
  tft.fillRect(0,302,240,18,C_HDRBG);tft.fillRect(0,300,240,2,C_NEON);
  tft.setTextColor(C_NEON);tft.setCursor(6,307);
  tft.print(isAPMode?"192.168.4.1":WiFi.localIP().toString());
  tft.setTextColor(C_MGRAY);tft.setCursor(120,307);tft.print(maidenhead);
  newPathReady=true;
}

void drawRadarBackground(){
  tft.drawCircle(RCX,RCY,RR,C_RING);tft.drawCircle(RCX,RCY,RR*2/3,C_RING);tft.drawCircle(RCX,RCY,RR/3,C_RING);
  tft.drawFastVLine(RCX,RCY-RR,RR*2+1,C_RING);tft.drawFastHLine(RCX-RR,RCY,RR*2+1,C_RING);
  for(int i=0;i<360;i+=45){float rad=i*PI/180.0f;tft.drawLine(RCX+(int)((RR-4)*cosf(rad)),RCY+(int)((RR-4)*sinf(rad)),RCX+(int)(RR*cosf(rad)),RCY+(int)(RR*sinf(rad)),(i%90==0)?C_NEON:C_DGRAY);}
  tft.setTextSize(1);tft.setTextColor(C_NEON);
  tft.setCursor(RCX-2,RCY-RR-10);tft.print("N");tft.setCursor(RCX-2,RCY+RR+4);tft.print("S");
  tft.setCursor(RCX+RR+5,RCY-3);tft.print("E");tft.setCursor(RCX-RR-11,RCY-3);tft.print("W");
  tft.setTextColor(C_DGRAY);tft.setCursor(RCX+3,RCY-RR/3-4);tft.print("60");tft.setCursor(RCX+3,RCY-RR*2/3-4);tft.print("30");
}

void drawSigBars(int x,int y,int rssi){
  int lvl=(rssi>-55)?4:(rssi>-65)?3:(rssi>-75)?2:(rssi>-85)?1:0;
  for(int i=0;i<4;i++){int h=3+i*2;uint16_t c=(i<lvl)?((lvl>=3)?C_NEON:(lvl==2)?C_AMBER:C_RED):C_DGRAY;tft.fillRect(x+i*5,y+(9-h),3,h,c);}
}

// ================= TFT SCREEN 1: PASS SCHEDULE =================
void tftDrawScheduleScreen(){
  tft.fillScreen(C_BLACK);
  tft.fillRect(0,0,240,16,C_HDRBG);tft.fillRect(0,16,240,2,C_AMBER);
  tft.setTextColor(C_AMBER);tft.setTextSize(1);
  tft.setCursor(6,4);tft.print("PASS SCHEDULE  [1/5]");

  if(!scheduleReady){
    tft.setTextColor(C_MGRAY);tft.setCursor(6,30);
    tft.print(scheduleBusy?"COMPUTING...":"NO DATA — REFRESH");
    tft.fillRect(0,300,240,20,C_HDRBG);tft.fillRect(0,298,240,2,C_AMBER);
    tft.setTextColor(C_AMBER);tft.setCursor(6,307);tft.print("BTN: NEXT SCREEN");
    return;
  }

  int y=22;
  for(int i=0;i<scheduleCount&&y<290;i++){
    time_t aosT=schedule[i].aos;
    struct tm tmA;gmtime_r(&aosT,&tmA);
    char buf[40];
    // (A6) %.12s caps long sat names so a 20-char name can't push the
    // elevation column off the 240px screen.
    snprintf(buf,sizeof(buf),"%02d:%02d  %.12s %5.1f",
             tmA.tm_hour,tmA.tm_min,schedule[i].satName,schedule[i].maxEl);
    bool next=(aosT>(time_t)timeClient.getEpochTime());
    tft.setTextColor(next?C_NEON:C_DGRAY);
    tft.setCursor(4,y);tft.print(buf);
    y+=11;
  }
  if(scheduleCount==0){tft.setTextColor(C_RED);tft.setCursor(6,40);tft.print("NO PASSES IN 24H");}

  tft.fillRect(0,300,240,20,C_HDRBG);tft.fillRect(0,298,240,2,C_AMBER);
  tft.setTextColor(C_AMBER);tft.setCursor(6,307);tft.print("BTN: NEXT SCREEN");
}

// ================= TFT SCREEN 2: IMU DRIFT GRAPH (feature 11) =================
void tftDrawDriftGraph(){
  tft.fillScreen(C_BLACK);
  tft.fillRect(0,0,240,16,C_HDRBG);tft.fillRect(0,16,240,2,C_NEON);
  tft.setTextColor(C_NEON);tft.setTextSize(1);
  tft.setCursor(6,4);tft.print("IMU DRIFT GRAPH  [2/5]");

  // Live values
  tft.setTextColor(C_AMBER);tft.setCursor(4,22);
  char buf[40];
  snprintf(buf,sizeof(buf),"PITCH %+7.2f  ROLL %+7.2f",imuPitch,imuRoll);
  tft.print(buf);
  tft.setTextColor(imuOK?C_NEON:C_RED);tft.setCursor(4,33);
  tft.print(imuOK?"IMU ONLINE":"IMU OFFLINE");

  // Graph axes
  int gx=20,gy=50,gw=200,gh=140;
  tft.drawRect(gx,gy,gw,gh,C_DGRAY);
  tft.drawFastHLine(gx,gy+gh/2,gw,C_FAINT); // zero line
  tft.setTextColor(C_DGRAY);tft.setCursor(4,gy);tft.print("+10");
  tft.setCursor(4,gy+gh/2-4);tft.print("  0");
  tft.setCursor(4,gy+gh-4);tft.print("-10");

  // Plot error history
  int histLen=errHistFull?ERR_HIST:errHistIdx;
  if(histLen>1){
    int startIdx=errHistFull?errHistIdx:0;
    float scale=gh/20.0f; // ±10 deg full scale
    int lastX=-1,lastY=-1;
    for(int i=0;i<histLen;i++){
      int idx=(startIdx+i)%ERR_HIST;
      float err=errHistory[idx];
      err=constrain(err,-10.0f,10.0f);
      int px=gx+1+(int)((float)i/histLen*(gw-2));
      int py=gy+gh/2-(int)(err*scale);
      py=constrain(py,gy+1,gy+gh-1);
      if(lastX>=0){
        uint16_t c=(fabsf(err)>IMU_CORRECT_THRESH)?C_AMBER:C_NEON;
        tft.drawLine(lastX,lastY,px,py,c);
      }
      lastX=px;lastY=py;
    }
  }

  // Threshold markers
  float thr=IMU_CORRECT_THRESH;
  float scale=gh/20.0f;
  int thrY1=gy+gh/2-(int)(thr*scale);
  int thrY2=gy+gh/2+(int)(thr*scale);
  tft.drawFastHLine(gx,constrain(thrY1,gy,gy+gh),gw,C_DRED);
  tft.drawFastHLine(gx,constrain(thrY2,gy,gy+gh),gw,C_DRED);

  tft.setTextColor(C_MGRAY);tft.setCursor(4,200);
  snprintf(buf,sizeof(buf),"ERR NOW: %+.2f deg",(float)(imuPitch-currentEl));
  tft.print(buf);

  tft.fillRect(0,300,240,20,C_HDRBG);tft.fillRect(0,298,240,2,C_NEON);
  tft.setTextColor(C_NEON);tft.setCursor(6,307);tft.print("BTN: NEXT SCREEN");
}

// ================= TFT SCREEN 3: SATELLITE FOOTPRINT =================
void tftDrawFootprintScreen(){
  tft.fillScreen(C_BLACK);
  tft.fillRect(0,0,240,16,C_HDRBG);tft.fillRect(0,16,240,2,C_CYAN);
  tft.setTextColor(C_CYAN);tft.setTextSize(1);
  tft.setCursor(6,4);tft.print("SAT FOOTPRINT  [3/5]");

  if(!tleLoaded||satDistance<100){
    tft.setTextColor(C_MGRAY);tft.setCursor(6,30);tft.print("NO SATELLITE TRACKED");
    tft.fillRect(0,300,240,20,C_HDRBG);tft.fillRect(0,298,240,2,C_CYAN);
    tft.setTextColor(C_CYAN);tft.setCursor(6,307);tft.print("BTN: NEXT SCREEN");
    return;
  }

  char buf[40];
  tft.setTextColor(C_WHITE);tft.setCursor(4,22);
  snprintf(buf,sizeof(buf),"SAT: %-20s",satName);tft.print(buf);
  tft.setTextColor(C_MGRAY);tft.setCursor(4,33);
  // (A4) satAltitude is the true orbital altitude; slant range
  // (satDistance) is the line-of-sight distance, NOT the altitude.
  snprintf(buf,sizeof(buf),"ALT: %.0f km",(float)satAltitude);tft.print(buf);
  tft.setCursor(4,44);
  snprintf(buf,sizeof(buf),"FOOTPRINT R: %.0f km",satFootprintKm);tft.print(buf);
  tft.setCursor(4,55);
  snprintf(buf,sizeof(buf),"SLANT RANGE: %.0f km",(float)satDistance);tft.print(buf);

  // Draw a simple overhead view circle representing footprint
  int cx=120,cy=175,maxR=90;
  // Observer at centre
  tft.drawCircle(cx,cy,3,C_AMBER);
  // Footprint ring — scale: 12000 km = full maxR
  int fpPx=(int)((satFootprintKm/12000.0f)*maxR);
  fpPx=constrain(fpPx,5,maxR);
  tft.drawCircle(cx,cy,fpPx,C_NEON);
  // Horizon ring
  tft.drawCircle(cx,cy,maxR,C_DGRAY);
  // Satellite position dot on the overhead map
  // Use elevation to position: el=90 → at centre, el=0 → at horizon
  float el=(float)targetEl;if(el<0)el=0;
  float az=(float)targetAz;
  float satR=maxR*(1.0f-el/90.0f);
  float satRad=(az-90.0f)*PI/180.0f;
  int sx=cx+(int)(satR*cosf(satRad));
  int sy=cy+(int)(satR*sinf(satRad));
  sx=constrain(sx,cx-maxR,cx+maxR);sy=constrain(sy,cy-maxR,cy+maxR);
  tft.fillCircle(sx,sy,4,C_NEON);
  tft.setTextColor(C_NEON);tft.setCursor(sx+6,sy-4);tft.print("SAT");

  // Labels
  tft.setTextColor(C_AMBER);tft.setCursor(cx-4,cy-4);tft.print("*");
  tft.setTextColor(C_DGRAY);tft.setCursor(cx-5,cy-maxR-9);tft.print("HORIZ");

  tft.fillRect(0,300,240,20,C_HDRBG);tft.fillRect(0,298,240,2,C_CYAN);
  tft.setTextColor(C_CYAN);tft.setCursor(6,307);tft.print("BTN: NEXT SCREEN");
}

// ================= TFT SCREEN 4: WEATHER (feature 14) =================
// Redesigned (D): city + condition, big temp with degree glyph and
// feels-like, 2x2 stat grid (wind+compass / humidity+pressure), wind
// bar, fetch age, high-wind warning. Purple chrome, green values.
void tftDrawWeatherScreen(){
  tft.fillScreen(C_BLACK);
  tft.fillRect(0,0,240,16,C_HDRBG);tft.fillRect(0,16,240,2,C_PRIMARY);
  tft.setTextColor(C_PRIMARY);tft.setTextSize(1);
  tft.setCursor(6,4);tft.print("GROUND WEATHER  [4/5]");

  if(!weather.valid){
    tft.setTextColor(C_MGRAY);tft.setTextSize(1);tft.setCursor(6,30);
    #ifdef OWM_API_KEY
    tft.print("FETCHING...");
    #else
    tft.print("NO API KEY IN secrets.h");
    #endif
    tft.fillRect(0,300,240,20,C_HDRBG);tft.fillRect(0,298,240,2,C_PRIMARY);
    tft.setTextColor(C_PRIMARY);tft.setCursor(6,307);tft.print("BTN: NEXT SCREEN");
    return;
  }

  char buf[40];
  // Header: city + condition (ASCII-safe via copyAscii in fetchWeather)
  tft.setTextColor(C_WHITE);tft.setTextSize(1);
  tft.setCursor(4,22);snprintf(buf,sizeof(buf),"%.22s",weather.city);tft.print(buf);
  tft.setTextColor(C_MGRAY);tft.setCursor(4,33);snprintf(buf,sizeof(buf),"%.22s",weather.desc);tft.print(buf);

  // Big temperature with proper degree glyph (0xF7 in GFX font)
  tft.setTextColor(C_ACCENT);tft.setTextSize(3);
  tft.setCursor(4,46);snprintf(buf,sizeof(buf),"%+.0f%cC",(double)weather.tempC,0xF7);tft.print(buf);
  // Feels-like, right of the big temp
  tft.setTextSize(1);tft.setTextColor(C_PRIMARY);
  tft.setCursor(150,52);tft.print("FEELS");
  tft.setTextColor(C_WHITE);
  tft.setCursor(150,64);snprintf(buf,sizeof(buf),"%+.0f%cC",(double)weather.feelsC,0xF7);tft.print(buf);

  tft.setTextSize(1);
  tft.drawFastHLine(0,86,240,C_DGRAY);

  // 2x2 stat grid helper
  auto cell=[&](int x,int y,const char*lbl,const char*val,uint16_t vc){
    tft.setTextSize(1);
    tft.setTextColor(C_MGRAY);tft.setCursor(x,y);tft.print(lbl);
    tft.setTextColor(vc);tft.setCursor(x,y+11);tft.print(val);
  };

  // Row 1: wind speed (color by strength) + compass bearing
  snprintf(buf,sizeof(buf),"%.1fm/s",(double)weather.windMps);
  cell(4,94,"WIND",buf,weather.windMps>10?C_RED:weather.windMps>5?C_ORANGE:C_ACCENT);
  snprintf(buf,sizeof(buf),"%.0f%c %s",(double)weather.windDeg,0xF7,windCompass(weather.windDeg));
  cell(124,94,"DIR",buf,C_WHITE);

  // Row 2: humidity + pressure
  snprintf(buf,sizeof(buf),"%d%%",weather.humidity);
  cell(4,124,"HUMID",buf,C_ACCENT);
  snprintf(buf,sizeof(buf),"%dhPa",weather.pressure);
  cell(124,124,"PRES",buf,C_PRIMARY);

  // Wind speed bar 0..20 m/s
  tft.drawRect(4,150,232,12,C_DGRAY);
  int barW=(int)(weather.windMps/20.0f*230);barW=constrain(barW,0,230);
  uint16_t bc=weather.windMps>10?C_RED:weather.windMps>5?C_ORANGE:C_ACCENT;
  if(barW>0)tft.fillRect(5,151,barW,10,bc);
  tft.setTextColor(C_DGRAY);tft.setCursor(4,165);tft.print("0");tft.setCursor(214,165);tft.print("20m/s");

  if(weather.windMps>10){
    tft.setTextColor(C_RED);tft.setCursor(4,182);tft.print("! HIGH WIND - CHECK PARK");
  }

  // Fetch time + station location
  struct tm ft;time_t ft2=weather.fetchedAt;gmtime_r(&ft2,&ft);
  tft.setTextColor(C_DGRAY);tft.setCursor(4,250);
  snprintf(buf,sizeof(buf),"UPDATED %02d:%02d UTC",ft.tm_hour,ft.tm_min);tft.print(buf);
  tft.setCursor(4,262);
  snprintf(buf,sizeof(buf),"STATION %.4f,%.4f",(double)obsLat,(double)obsLon);tft.print(buf);

  tft.fillRect(0,300,240,20,C_HDRBG);tft.fillRect(0,298,240,2,C_PRIMARY);
  tft.setTextColor(C_PRIMARY);tft.setCursor(6,307);tft.print("BTN: NEXT SCREEN");
}

// ================= DYNAMIC HUD (screen 0) =================
void tftUpdateDynamic(){
  if(spiLock||isAPMode) return;
  char buf[32];

  time_t nowT=(time_t)timeClient.getEpochTime();
  struct tm tmN;gmtime_r(&nowT,&tmN);
  char ck[10];snprintf(ck,sizeof(ck),"%02d:%02d:%02d",tmN.tm_hour,tmN.tm_min,tmN.tm_sec);
  if(strcmp(ck,prev_clock)!=0){
    tft.fillRect(122,4,50,9,C_HDRBG);tft.setTextColor(C_WHITE);tft.setTextSize(1);
    tft.setCursor(122,4);tft.print(ck);strcpy(prev_clock,ck);
  }
  if((int)ntpSynced!=prev_ntp){
    tft.fillCircle(228,8,3,ntpSynced?C_NEON:C_RED);prev_ntp=ntpSynced;
    tft.fillRect(147,88,42,11,C_BLACK);
    tft.setTextColor(ntpSynced?C_NEON:C_RED);tft.setTextSize(1);
    tft.setCursor(150,90);tft.print(ntpSynced?"SYNC":"ERR");
  }
  if(strcmp(satName,prev_sat)!=0){
    tft.fillRect(30,21,207,11,C_PNLG);tft.setTextColor(C_AMBER);tft.setTextSize(1);
    tft.setCursor(32,23);snprintf(buf,sizeof(buf),"%.22s%s",satName,isGeoSat?" GEO":"");tft.print(buf);
    strncpy(prev_sat,satName,24);
  }
  float cAz=currentAz,tAz=targetAz,cEl=currentEl,tEl=targetEl;
  if(fabsf(cAz-prev_cAz)>0.05f||fabsf(tAz-prev_tAz)>0.05f){
    tft.fillRect(22,36,94,16,C_BLACK);tft.setTextColor(C_NEON);tft.setTextSize(2);
    tft.setCursor(22,36);snprintf(buf,sizeof(buf),"%05.1f",(double)cAz);tft.print(buf);
    float dAz=tAz-cAz;if(dAz>180)dAz-=360;if(dAz<-180)dAz+=360;
    tft.fillRect(6,58,110,9,C_BLACK);tft.setTextSize(1);tft.setTextColor(C_WHITE);
    tft.setCursor(6,58);snprintf(buf,sizeof(buf),">%05.1f",(double)tAz);tft.print(buf);
    tft.setTextColor(fabsf(dAz)>1.0f?C_AMBER:C_MGRAY);
    tft.setCursor(60,58);snprintf(buf,sizeof(buf),"%+06.1f",(double)dAz);tft.print(buf);
    prev_cAz=cAz;prev_tAz=tAz;
  }
  if(fabsf(cEl-prev_cEl)>0.05f||fabsf(tEl-prev_tEl)>0.05f){
    tft.fillRect(140,36,96,16,C_BLACK);tft.setTextColor(C_AMBER);tft.setTextSize(2);
    tft.setCursor(140,36);snprintf(buf,sizeof(buf),"%04.1f",(double)cEl);tft.print(buf);
    float dEl=tEl-cEl;
    tft.fillRect(124,58,110,9,C_BLACK);tft.setTextSize(1);tft.setTextColor(C_WHITE);
    tft.setCursor(124,58);snprintf(buf,sizeof(buf),">%04.1f",(double)tEl);tft.print(buf);
    tft.setTextColor(fabsf(dEl)>1.0f?C_AMBER:C_MGRAY);
    tft.setCursor(172,58);snprintf(buf,sizeof(buf),"%+05.1f",(double)dEl);tft.print(buf);
    prev_cEl=cEl;prev_tEl=tEl;
  }
  tft.setTextSize(1);
  int modeVal=onboardMode?1:0;
  if(modeVal!=prev_mode){tft.fillRect(3,88,42,11,C_BLACK);tft.setTextColor(onboardMode?C_NEON:C_AMBER);tft.setCursor(6,90);tft.print(onboardMode?"SGP4":"L4S");prev_mode=modeVal;}
  if((int)isMoving!=prev_moving){tft.fillRect(51,88,42,11,C_BLACK);tft.setTextColor(isMoving?C_AMBER:C_NEON);tft.setCursor(54,90);tft.print(isMoving?"SLEW":"IDLE");prev_moving=isMoving;}
  bool l4s=(tcpClient&&tcpClient.connected());
  if((int)l4s!=prev_l4s){tft.fillRect(99,88,42,11,C_BLACK);tft.setTextColor(l4s?C_NEON:C_MGRAY);tft.setCursor(102,90);tft.print(l4s?"LINK":"----");prev_l4s=l4s;}
  int heapK=ESP.getFreeHeap()/1024;
  if(abs(heapK-prev_heap)>2){tft.fillRect(195,88,42,11,C_BLACK);tft.setTextColor(heapK>50?C_MGRAY:C_RED);tft.setCursor(198,90);snprintf(buf,sizeof(buf),"%dK",heapK);tft.print(buf);prev_heap=heapK;}
  if(fabsf(satDistance-prev_dist)>0.5f){
    tft.fillRect(32,151,84,9,C_BLACK);tft.setTextColor(C_WHITE);tft.setCursor(32,151);
    if(satDistance>0)snprintf(buf,sizeof(buf),"%.0fkm",(double)satDistance);else strcpy(buf,"---");
    tft.print(buf);prev_dist=satDistance;
  }
  if(fabsf(dopplerFreq-prev_dop)>1.0f){
    tft.fillRect(150,151,86,9,C_BLACK);tft.setTextColor(C_NEON);tft.setCursor(150,151);
    snprintf(buf,sizeof(buf),"%+05.0fHz",(double)dopplerFreq);tft.print(buf);prev_dop=dopplerFreq;
  }
  int rssi=isAPMode?-100:WiFi.RSSI();
  if(abs(rssi-prev_rssi)>2){tft.fillRect(198,304,40,14,C_HDRBG);drawSigBars(200,306,rssi);prev_rssi=rssi;}

  unsigned long ne=timeClient.getEpochTime();
  int gs=isGeoSat?1:0;
  long cd=(nextAosTime>0)?(long)(nextAosTime-(time_t)ne):-999999;
  bool rdr=(gs!=prev_geo)||(gs&&fabsf(nextMaxEl-prev_maxElShown)>0.05f)||(!gs&&cd!=prev_countdown);
  if(rdr){
    tft.fillRect(32,120,84,9,C_BLACK);tft.fillRect(32,133,84,9,C_BLACK);
    tft.fillRect(150,120,86,9,C_BLACK);tft.fillRect(150,133,86,9,C_BLACK);
    if(gs){
      tft.setTextColor(C_NEON);tft.setCursor(32,120);tft.print("GEO ORBIT");tft.setCursor(32,133);tft.print("STATIC");
      tft.setTextColor(C_AMBER);tft.setCursor(150,120);snprintf(buf,sizeof(buf),"%.1f deg",(double)nextMaxEl);tft.print(buf);
      tft.setCursor(150,133);if(nextMaxEl>0){tft.setTextColor(C_NEON);tft.print("LOCKED");}else{tft.setTextColor(C_RED);tft.print("NO VIS");}
    } else if(nextAosTime>0){
      time_t at=nextAosTime,lt=nextLosTime;struct tm ta,tl;gmtime_r(&at,&ta);gmtime_r(&lt,&tl);
      tft.setTextColor(C_WHITE);tft.setCursor(32,120);snprintf(buf,sizeof(buf),"%02d:%02d:%02d",ta.tm_hour,ta.tm_min,ta.tm_sec);tft.print(buf);
      tft.setCursor(32,133);snprintf(buf,sizeof(buf),"%02d:%02d:%02d",tl.tm_hour,tl.tm_min,tl.tm_sec);tft.print(buf);
      tft.setTextColor(C_AMBER);tft.setCursor(150,120);snprintf(buf,sizeof(buf),"%.1f deg",(double)nextMaxEl);tft.print(buf);
      tft.setCursor(150,133);
      if(cd<=0&&(time_t)ne<lt){tft.setTextColor(C_NEON);tft.print("TRACKING");}
      else if(cd>0){tft.setTextColor(C_ORANGE);snprintf(buf,sizeof(buf),"-%02ld:%02ld:%02ld",cd/3600,(cd%3600)/60,cd%60);tft.print(buf);}
      else{tft.setTextColor(C_MGRAY);tft.print("ACQUIRING");}
    } else {
      tft.setTextColor(C_RED);tft.setCursor(32,120);tft.print(tleLoaded?"SEARCHING":"NO TLE");tft.setCursor(32,133);tft.print("--:--:--");
    }
    prev_geo=gs;prev_countdown=cd;prev_maxElShown=nextMaxEl;
  }
}

// ================= RADAR ENGINE =================
void tftRadarUpdate(){
  if(spiLock||isAPMode) return;
  auto toXY=[](float az,float el,int &x,int &y){
    float se=(el<0)?0:(el>90)?90:el;float rr=RR*(1-se/90.0f);float rad=(az-90)*PI/180.0f;
    x=clampX(RCX+(int)(rr*cosf(rad)));y=clampY(RCY+(int)(rr*sinf(rad)));
  };
  auto drawDash=[](int tx,int ty,uint16_t c){
    float dx=tx-RCX,dy=ty-RCY,len=sqrtf(dx*dx+dy*dy);if(len<1)return;
    float ux=dx/len,uy=dy/len,pos=0;bool on=true;
    while(pos<len){float end=pos+(on?6.0f:4.0f);if(end>len)end=len;
      if(on)tft.drawLine(clampX(RCX+(int)(ux*pos)),clampY(RCY+(int)(uy*pos)),clampX(RCX+(int)(ux*end)),clampY(RCY+(int)(uy*end)),c);
      pos=end;on=!on;}
  };
  auto eraseDash=[&drawDash](int tx,int ty){drawDash(tx,ty,C_BLACK);};
  auto drawDiamond=[](int cx,int cy,int r,uint16_t c){
    tft.drawLine(clampX(cx),clampY(cy-r),clampX(cx+r),clampY(cy),c);
    tft.drawLine(clampX(cx+r),clampY(cy),clampX(cx),clampY(cy+r),c);
    tft.drawLine(clampX(cx),clampY(cy+r),clampX(cx-r),clampY(cy),c);
    tft.drawLine(clampX(cx-r),clampY(cy),clampX(cx),clampY(cy-r),c);
  };
  if(newPathReady){
    tft.fillRect(RCX-RR-14,RCY-RR-14,(RR+14)*2,(RR+14)*2,C_BLACK);drawRadarBackground();
    prev_antX=prev_antY=prev_tgtX=prev_tgtY=prev_satX=prev_satY=-1;prev_alX[0]=prev_alX[1]=-1;newPathReady=false;
  } else {
    if(prev_antX>=0){tft.drawLine(RCX,RCY,prev_antX,prev_antY,C_BLACK);tft.fillCircle(prev_antX,prev_antY,4,C_BLACK);tft.fillRect(clampX(prev_antX+(prev_antX>=RCX?6:-26)),clampY(prev_antY+(prev_antY>=RCY?6:-10)),24,9,C_BLACK);}
    if(prev_tgtX>=0){eraseDash(prev_tgtX,prev_tgtY);drawDiamond(prev_tgtX,prev_tgtY,5,C_BLACK);tft.fillRect(clampX(prev_tgtX+(prev_tgtX>=RCX?8:-30)),clampY(prev_tgtY+(prev_tgtY>=RCY?7:-11)),24,9,C_BLACK);}
    if(prev_satX>=0){tft.fillCircle(prev_satX,prev_satY,4,C_BLACK);tft.drawCircle(prev_satX,prev_satY,7,C_BLACK);}
    for(int k=0;k<2;k++)if(prev_alX[k]>=0)tft.fillRect(prev_alX[k],prev_alY[k],7,9,C_BLACK);
    tft.drawCircle(RCX,RCY,RR,C_RING);tft.drawCircle(RCX,RCY,RR*2/3,C_RING);tft.drawCircle(RCX,RCY,RR/3,C_RING);
    tft.drawFastVLine(RCX,RCY-RR+1,RR*2-1,C_RING);tft.drawFastHLine(RCX-RR+1,RCY,RR*2-1,C_RING);
  }
  // Satellite footprint circle on radar (feature 13)
  if(targetEl>0.0f&&satFootprintKm>0){
    int sx,sy;toXY((float)targetAz,(float)targetEl,sx,sy);
    int fpPx=(int)((satFootprintKm/12000.0f)*RR);fpPx=constrain(fpPx,4,RR-2);
    tft.drawCircle(clampX(sx),clampY(sy),fpPx,C_DGRAY);
  }
  // Trajectory
  int len=(isGeoSat||predBusy)?0:globalPathLen;
  prev_alX[0]=prev_alX[1]=-1;
  if(len>1){
    int lx=-1,ly=-1;tft.setTextSize(1);
    for(int i=0;i<len;i++){
      int px,py;toXY(globalPath[i].az,globalPath[i].el,px,py);
      if(lx!=-1)tft.drawLine(lx,ly,px,py,C_AMBER);
      if(i==0){tft.setTextColor(C_NEON);int ax=clampX(px-3),ay=clampY(py-10);tft.setCursor(ax,ay);tft.print("A");prev_alX[0]=ax;prev_alY[0]=ay;}
      else if(i==len-1){tft.setTextColor(C_RED);int ax=clampX(px-3),ay=clampY(py-10);tft.setCursor(ax,ay);tft.print("L");prev_alX[1]=ax;prev_alY[1]=ay;}
      lx=px;ly=py;
    }
  }
  // Target arrow
  int tgtX=-1,tgtY=-1;
  if(targetEl>0.0f){
    toXY((float)targetAz,(float)targetEl,tgtX,tgtY);
    drawDash(tgtX,tgtY,C_AMBER);drawDiamond(tgtX,tgtY,5,C_AMBER);
    tft.setTextSize(1);tft.setTextColor(C_AMBER);
    tft.setCursor(clampX(tgtX+(tgtX>=RCX?8:-30)),clampY(tgtY+(tgtY>=RCY?7:-11)));tft.print("TGT");
  }
  prev_tgtX=tgtX;prev_tgtY=tgtY;
  // Actual needle
  float sae=(float)currentEl<0?0:(float)currentEl;int ax,ay;
  toXY((float)currentAz,sae,ax,ay);
  tft.drawLine(RCX,RCY,ax,ay,C_MGRAY);tft.fillCircle(ax,ay,4,C_WHITE);
  tft.setTextSize(1);tft.setTextColor(C_WHITE);
  tft.setCursor(clampX(ax+(ax>=RCX?6:-26)),clampY(ay+(ay>=RCY?6:-10)));tft.print("ANT");
  prev_antX=ax;prev_antY=ay;
  // Satellite dot
  if(targetEl>0.0f){
    int sx,sy;toXY((float)targetAz,(float)targetEl,sx,sy);
    uint16_t c=isGeoSat?C_AMBER:C_NEON;tft.fillCircle(sx,sy,4,c);tft.drawCircle(sx,sy,7,c);
    prev_satX=sx;prev_satY=sy;
  } else prev_satX=-1;
}

// ================= PASS LOG =================
void logPass(){
  if(passLogCount>=10){for(int i=0;i<9;i++)passLog[i]=passLog[i+1];passLogCount=9;}
  strncpy(passLog[passLogCount].sat,satName,24);
  strncpy(passLog[passLogCount].time,passStartBuf,19);
  passLog[passLogCount].maxEl=passMaxEl;passLogCount++;
  Serial.printf("[SGP4] Pass: %s MaxEl %.1f\n",satName,passMaxEl);
}

// ================= WEB SERVER =================
void setupWebServer(){
  webServer.on("/",HTTP_GET,[](AsyncWebServerRequest *r){r->send_P(200,"text/html",isAPMode?wifi_html:index_html);});

  webServer.on("/api/wifi",HTTP_POST,[](AsyncWebServerRequest *r){r->send(200,"application/json","{\"status\":\"ok\"}");},NULL,
    [](AsyncWebServerRequest *r,uint8_t *data,size_t len,size_t,size_t){
      StaticJsonDocument<256> doc;if(!deserializeJson(doc,String((char*)data,len))){
        String ns=doc["ssid"]|"",np=doc["pass"]|"";
        if(ns.length()>0){prefs.begin("sattracker",false);prefs.putString("wifi_ssid",ns);prefs.putString("wifi_pass",np);prefs.end();triggerReboot=true;}
      }
    });

  webServer.on("/api/status",HTTP_GET,[](AsyncWebServerRequest *r){r->send(200,"application/json",buildTelemetryJson());});

  webServer.on("/api/path",HTTP_GET,[](AsyncWebServerRequest *r){
    if(!tleLoaded||!ntpSynced||!onboardMode||predBusy){r->send(200,"application/json","[]");return;}
    String j="[";for(int i=0;i<globalPathLen;i++){if(i)j+=",";j+="{\"az\":"+String(globalPath[i].az,1)+",\"el\":"+String(globalPath[i].el,1)+"}";}
    r->send(200,"application/json",j+"]");
  });

  webServer.on("/api/passlog",HTTP_GET,[](AsyncWebServerRequest *r){
    String j="[";for(int i=0;i<passLogCount;i++){if(i)j+=",";j+="{\"sat\":\""+String(passLog[i].sat)+"\",\"time\":\""+String(passLog[i].time)+"\",\"maxEl\":"+String(passLog[i].maxEl,1)+"}";}
    r->send(200,"application/json",j+"]");
  });

  webServer.on("/api/schedule",HTTP_GET,[](AsyncWebServerRequest *r){
    if(!scheduleReady){r->send(200,"application/json","[]");return;}
    String j="[";for(int i=0;i<scheduleCount;i++){if(i)j+=",";j+="{\"sat\":\""+String(schedule[i].satName)+"\",\"aos\":"+String((long)schedule[i].aos)+",\"los\":"+String((long)schedule[i].los)+",\"maxEl\":"+String(schedule[i].maxEl,1)+",\"aosAz\":"+String(schedule[i].aosAz,1)+"}";}
    r->send(200,"application/json",j+"]");
  });

  webServer.on("/api/config",HTTP_POST,[](AsyncWebServerRequest *r){r->send(200,"application/json","{\"status\":\"ok\"}");},NULL,
    [](AsyncWebServerRequest *r,uint8_t *data,size_t len,size_t,size_t){
      StaticJsonDocument<256> doc;if(!deserializeJson(doc,String((char*)data,len))){
        if(doc.containsKey("grid")){maidenhead=doc["grid"].as<String>();maidenheadToLatLon(maidenhead,obsLat,obsLon);prefs.begin("sattracker",false);prefs.putString("grid",maidenhead);prefs.end();lastPathCalc=0;}
        if(doc.containsKey("obMode")){onboardMode=doc["obMode"].as<bool>();prefs.begin("sattracker",false);prefs.putBool("obMode",onboardMode);prefs.end();lastPathCalc=0;}
      }
    });

  webServer.on("/api/tle",HTTP_POST,[](AsyncWebServerRequest *r){r->send(200,"application/json","{\"status\":\"ok\"}");},NULL,
    [](AsyncWebServerRequest *r,uint8_t *data,size_t len,size_t,size_t){
      StaticJsonDocument<512> doc;if(!deserializeJson(doc,String((char*)data,len))){
        String name=doc["name"]|"UNKNOWN",l1=doc["line1"]|"",l2=doc["line2"]|"";
        if(l1.length()>0&&l2.length()>0){
          File f=LittleFS.open("/tle.txt","w");if(f){f.println(name);f.println(l1);f.println(l2);f.close();}
          name.toCharArray(satName,25);l1.toCharArray(tleLine1,70);l2.toCharArray(tleLine2,70);
          sat.site(obsLat,obsLon,obsAlt);sat.init(satName,tleLine1,tleLine2);
          isGeoSat=tleIsGeo();tleLoaded=true;nextAosTime=nextLosTime=0;lastPathCalc=0;newPathReady=true;
          // Also add to queue head. BUG FIX (A2): the old code did the
          // memmove WITHOUT bounding to MAX_QUEUED_SATS, so a full queue
          // wrote satQueue[8] out of bounds. Now we cap before inserting.
          if(queueCount>=MAX_QUEUED_SATS){
            queueCount=MAX_QUEUED_SATS-1;   // drop the tail slot to make room
          }
          memmove(&satQueue[1],&satQueue[0],sizeof(QueuedSat)*queueCount);
          strncpy(satQueue[0].name,satName,24);
          strncpy(satQueue[0].line1,tleLine1,69);
          strncpy(satQueue[0].line2,tleLine2,69);
          satQueue[0].valid=true;queueCount++;queueHead=0;
          scheduleDirty=true;   // (A3) queue changed — recompute schedule
          Serial.printf("[SGP4] TLE: %s %s\n",satName,isGeoSat?"(GEO)":"");
        }
      }
    });

  webServer.on("/api/queue",HTTP_POST,[](AsyncWebServerRequest *r){r->send(200,"application/json","{\"status\":\"ok\"}");},NULL,
    [](AsyncWebServerRequest *r,uint8_t *data,size_t len,size_t,size_t){
      if(queueCount>=MAX_QUEUED_SATS){
        AsyncWebServerResponse *res=r->beginResponse(200,"application/json","{\"status\":\"full\"}");r->send(res);return;
      }
      StaticJsonDocument<512> doc;if(!deserializeJson(doc,String((char*)data,len))){
        String name=doc["name"]|"",l1=doc["line1"]|"",l2=doc["line2"]|"";
        if(name.length()>0&&l1.length()>0){
          strncpy(satQueue[queueCount].name,name.c_str(),24);
          strncpy(satQueue[queueCount].line1,l1.c_str(),69);
          strncpy(satQueue[queueCount].line2,l2.c_str(),69);
          satQueue[queueCount].valid=true;queueCount++;
          scheduleDirty=true;   // (A3) recompute schedule for the new sat
          Serial.printf("[QUEUE] Added: %s (%d total)\n",satQueue[queueCount-1].name,queueCount);
        }
      }
      r->send(200,"application/json","{\"status\":\"ok\"}");
    });

  webServer.on("/api/queue/remove",HTTP_POST,[](AsyncWebServerRequest *r){r->send(200,"application/json","{\"status\":\"ok\"}");},NULL,
    [](AsyncWebServerRequest *r,uint8_t *data,size_t len,size_t,size_t){
      StaticJsonDocument<64> doc;if(!deserializeJson(doc,String((char*)data,len))){
        int idx=doc["idx"]|0;
        if(idx>=0&&idx<queueCount){for(int i=idx;i<queueCount-1;i++)satQueue[i]=satQueue[i+1];queueCount--;if(queueHead>=queueCount)queueHead=0;scheduleDirty=true;}
      }
    });

  webServer.on("/api/queue/clear",HTTP_POST,[](AsyncWebServerRequest *r){queueCount=0;queueHead=0;scheduleDirty=true;r->send(200,"application/json","{\"status\":\"ok\"}");});

  // (A3) web "REFRESH SCHEDULE" kicks a recompute; client then GETs /api/schedule.
  webServer.on("/api/schedule/refresh",HTTP_POST,[](AsyncWebServerRequest *r){scheduleDirty=true;r->send(200,"application/json","{\"status\":\"ok\"}");});

  webServer.on("/api/manual",HTTP_POST,[](AsyncWebServerRequest *r){r->send(200,"application/json","{\"status\":\"ok\"}");},NULL,
    [](AsyncWebServerRequest *r,uint8_t *data,size_t len,size_t,size_t){
      StaticJsonDocument<128> doc;if(!deserializeJson(doc,String((char*)data,len))&&!onboardMode&&!isAPMode){
        if(doc.containsKey("az"))targetAz=doc["az"].as<float>();if(doc.containsKey("el"))targetEl=doc["el"].as<float>();
      }
    });

  ws.onEvent([](AsyncWebSocket*,AsyncWebSocketClient *c,AwsEventType t,void*,uint8_t*,size_t){if(t==WS_EVT_CONNECT)c->text(buildTelemetryJson());});
  webServer.addHandler(&ws);webServer.begin();
}

// ================= PASS PREDICTION =================
void calculatePathPrediction(){
  if(!tleLoaded||!ntpSynced||!onboardMode||isAPMode){globalPathLen=0;newPathReady=true;return;}
  unsigned long nowT=timeClient.getEpochTime();
  if(nowT<1000000000UL)return;
  predBusy=true;predSat.site(obsLat,obsLon,obsAlt);predSat.init(satName,tleLine1,tleLine2);isGeoSat=tleIsGeo();
  if(isGeoSat){predSat.findsat(nowT);nextMaxEl=predSat.satEl;nextAosTime=(predSat.satEl>0)?(time_t)nowT:0;nextLosTime=0;globalPathLen=0;predBusy=false;newPathReady=true;return;}
  unsigned long t=nowT;predSat.findsat(t);bool found=false;
  if(predSat.satEl>0){int lim=0;while(predSat.satEl>0&&lim++<80){t-=30;predSat.findsat(t);if(lim%10==0)vTaskDelay(pdMS_TO_TICKS(2));}t+=30;found=true;}
  else{for(int i=0;i<1440&&!found;i++){t+=60;predSat.findsat(t);if(predSat.satEl>0){int lim=0;while(predSat.satEl>0&&lim++<12){t-=10;predSat.findsat(t);}t+=10;found=true;}if(i%30==0)vTaskDelay(pdMS_TO_TICKS(2));}}
  if(!found){if(nextLosTime>(time_t)nowT||nextAosTime>(time_t)nowT){predBusy=false;return;}nextAosTime=nextLosTime=0;nextMaxEl=0;globalPathLen=0;predBusy=false;newPathReady=true;return;}
  static PathPoint tmp[45];float mEl=0;int pl=0;unsigned long tt=t;
  for(int i=0;i<80;i++){predSat.findsat(tt);if(predSat.satEl<0&&i>0)break;if(predSat.satEl>=0){if(predSat.satEl>mEl)mEl=predSat.satEl;if(pl<45){tmp[pl].az=predSat.satAz;tmp[pl].el=predSat.satEl;pl++;}}tt+=30;if(i%10==0)vTaskDelay(pdMS_TO_TICKS(2));}
  memcpy((void*)globalPath,tmp,sizeof(PathPoint)*pl);nextAosTime=(time_t)t;nextLosTime=(time_t)tt;nextMaxEl=mEl;globalPathLen=pl;predBusy=false;newPathReady=true;
  Serial.printf("[SGP4] Pass AOS+%lds MaxEl %.1f (%d pts)\n",(long)(nextAosTime-nowT),mEl,pl);
}

// ================= CORE 0 TASK =================
void Core0TaskCode(void *pvParameters){
  static unsigned long lastWeather=0,lastSchedule=0,lastWifiCheck=0,lastWsPush=0,lastTft=0;
  for(;;){
    esp_task_wdt_reset();
    if(WiFi.status()==WL_CONNECTED&&!isAPMode)if(timeClient.update())ntpSynced=true;
    if(millis()-lastWifiCheck>10000&&!isAPMode){lastWifiCheck=millis();if(WiFi.status()!=WL_CONNECTED)WiFi.reconnect();}
    if(!tcpClient||!tcpClient.connected()){tcpClient=tcpServer.available();if(tcpClient)tcpClient.setNoDelay(true);}
    if(tcpClient&&tcpClient.available()){tcpClient.setTimeout(5);String cmd=tcpClient.readStringUntil('\n');if(cmd.length()>0){if(!onboardMode&&!isAPMode)parseEasyComm(cmd);tcpClient.println("OK");}}

    bool passExpired=(nextLosTime>0&&!isGeoSat&&(time_t)timeClient.getEpochTime()>nextLosTime&&millis()-lastPathCalc>10000);
    if((millis()-lastPathCalc>60000||passExpired)&&!isAPMode){calculatePathPrediction();lastPathCalc=millis();}

    // Auto-advance queue when pass ends
    if(passExpired&&queueCount>1){advanceQueue();}

    // Weather fetch
    if(millis()-lastWeather>WEATHER_INTERVAL_MS&&!isAPMode){fetchWeather();lastWeather=millis();}

    // Schedule compute every 30 min, OR immediately when the queue changed
    // or the web UI requested a refresh (scheduleDirty). BUG FIX (A3):
    // previously it only ran at boot + 30-min, so sats queued after boot
    // never appeared in the schedule — "only shows the current TLE".
    if((scheduleDirty||millis()-lastSchedule>1800000)&&!isAPMode&&queueCount>0){
      scheduleDirty=false;computeSchedule();lastSchedule=millis();
    }

    if(millis()-lastWsPush>1000&&!isAPMode){if(ws.count()>0)ws.textAll(buildTelemetryJson());lastWsPush=millis();}

    // TFT update — only screen 0 needs frequent updates; others are static
    if(millis()-lastTft>300&&!isAPMode){
      if(screenChanged){
        screenChanged=false;
        switch(currentScreen){
          case 0:tftDrawStaticFrame();break;
          case 1:tftDrawScheduleScreen();break;
          case 2:tftDrawDriftGraph();break;
          case 3:tftDrawFootprintScreen();break;
          case 4:tftDrawWeatherScreen();break;
        }
      }
      if(currentScreen==0){tftUpdateDynamic();tftRadarUpdate();}
      else if(currentScreen==2){tftDrawDriftGraph();}  // drift graph updates live
      lastTft=millis();
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ================= SETUP =================
void setup(){
  Serial.begin(115200);delay(500);
  Serial.println("\n====================================");
  Serial.println(" ESP32 Orbital Ops v10.0");
  Serial.println("====================================");
  esp_task_wdt_init(30,true);esp_task_wdt_add(NULL);

  // Button
  pinMode(BTN_PIN,INPUT_PULLUP);

  pinMode(ENABLE_PIN,OUTPUT);digitalWrite(ENABLE_PIN,HIGH);
  azStepper.setMaxSpeed(2000);azStepper.setAcceleration(1000);
  elStepper.setMaxSpeed(2000);elStepper.setAcceleration(1000);

  tft.init(240,320);tft.setRotation(0);tft.invertDisplay(false);
  drawBootScreen();delay(3000);
  // (C) switch to the Linux-tty boot log
  tft.fillScreen(C_BLACK);
  tft.fillRect(0,0,240,22,C_HDRBG);tft.fillRect(0,22,240,2,C_ACCENT);
  tft.setTextColor(C_ACCENT);tft.setTextSize(1);tft.setCursor(6,7);tft.print("orbital tty1");
  tft.setTextColor(C_MGRAY);tft.setCursor(170,7);tft.print("boot log");
  tft.drawFastHLine(0,24,240,C_DGRAY);
  terminalY=28;
  tft.setTextColor(C_PRIMARY);tft.setTextSize(1);
  tft.setCursor(4,terminalY);tft.print("orbital login: root");
  terminalY+=11;
  tft.setTextColor(C_MGRAY);tft.setCursor(4,terminalY);tft.print("Welcome to Orbital Ops v10.0");
  terminalY+=11;

  printBootLine("Initializing IMU...",2);
  if(mpuBegin()){
    for(int i=0;i<50;i++){mpuRead();delay(10);}
    printBootLine("IMU Kalman p="+String(imuPitch,1),0);
#if IMU_CORRECT_ENABLED
    float be=(imuPitch<0)?0:imuPitch;elStepper.setCurrentPosition((long)(be*STEPS_PER_DEG));
#endif
  } else {imuOK=false;printBootLine("IMU NOT FOUND",3);}

  printBootLine("Mounting LittleFS",2);
  if(!LittleFS.begin(false))LittleFS.begin(true);
  printBootLine("LittleFS mounted",0);

  printBootLine("Loading NVRAM prefs",2);
  prefs.begin("sattracker",true);
  maidenhead=prefs.getString("grid",maidenhead);onboardMode=prefs.getBool("obMode",false);
  String savedSSID=prefs.getString("wifi_ssid",ssid),savedPass=prefs.getString("wifi_pass",password);
  prefs.end();
  maidenheadToLatLon(maidenhead,obsLat,obsLon);sat.site(obsLat,obsLon,obsAlt);
  printBootLine("Prefs: grid "+maidenhead,0);

  if(LittleFS.exists("/tle.txt")){
    File f=LittleFS.open("/tle.txt","r");
    if(f){
      String n=f.readStringUntil('\n');n.trim();n.toCharArray(satName,25);
      String l1=f.readStringUntil('\n');l1.trim();l1.toCharArray(tleLine1,70);
      String l2=f.readStringUntil('\n');l2.trim();l2.toCharArray(tleLine2,70);f.close();
      if(strlen(tleLine1)>0&&strlen(tleLine2)>0){
        sat.init(satName,tleLine1,tleLine2);isGeoSat=tleIsGeo();tleLoaded=true;
        // Pre-populate queue with saved satellite
        strncpy(satQueue[0].name,satName,24);strncpy(satQueue[0].line1,tleLine1,69);strncpy(satQueue[0].line2,tleLine2,69);satQueue[0].valid=true;queueCount=1;
        printBootLine("TLE loaded: "+String(satName),0);
      }
    }
  } else {
    printBootLine("No saved TLE",3);
  }

  printBootLine("Coupling WiFi radio",2);
  WiFi.mode(WIFI_STA);WiFi.begin(savedSSID.c_str(),savedPass.c_str());WiFi.setSleep(false);
  unsigned long t0=millis();
  while(WiFi.status()!=WL_CONNECTED){if(millis()-t0>15000){isAPMode=true;break;}delay(500);}

  if(isAPMode){
    printBootLine("Network timeout",1);
    printBootLine("Setup gateway mode",3);
    WiFi.disconnect();WiFi.mode(WIFI_AP);WiFi.softAP("AEROSPACE-TRACKER","groundstation");
    setupWebServer();tcpServer.begin();delay(500);tftDrawAPMode();return;
  }
  printBootLine("WiFi link up",0);
  printBootLine("IP "+WiFi.localIP().toString(),0);
  if(MDNS.begin("sattracker")){MDNS.addService("http","tcp",80);MDNS.addService("easycomm","tcp",4533);printBootLine("mDNS sattracker.local",0);}

  printBootLine("Syncing reference clock",2);
  timeClient.begin();timeClient.update();
  if(timeClient.getEpochTime()>1000000){ntpSynced=true;printBootLine("NTP sync OK",0);}
  else printBootLine("NTP sync pending",3);

  ArduinoOTA.setHostname("sattracker");ArduinoOTA.begin();
  setupWebServer();tcpServer.begin();
  printBootLine("TCP/Web endpoints up",0);

  // Initial weather fetch
  printBootLine("Fetching weather",2);fetchWeather();lastWeatherFetch=millis();
  printBootLine(weather.valid?("WX "+String(weather.city)):String("WX fetch failed"),weather.valid?0:3);

  if(tleLoaded&&ntpSynced&&onboardMode){
    printBootLine("Compiling SGP4 matrix",2);calculatePathPrediction();lastPathCalc=millis();
    printBootLine("SGP4 matrix ready",0);
  }
  if(queueCount>0&&ntpSynced){
    printBootLine("Building pass schedule",2);computeSchedule();
    printBootLine(String(scheduleCount)+" passes queued",0);
  }

  printBootLine("Starting Ops HUD",0);delay(500);
  tftDrawStaticFrame();
  digitalWrite(ENABLE_PIN,LOW);
  lastActiveMs=millis();

  xTaskCreatePinnedToCore(Core0TaskCode,"Core0Task",32768,NULL,1,&Core0Task,0);
}

// ================= LOOP (CORE 1) =================
void loop(){
  esp_task_wdt_reset();
  if(triggerReboot){delay(1000);ESP.restart();}
  if(isAPMode)return;

  ArduinoOTA.handle();

  // Button handler — IO0 cycles screens
  if(digitalRead(BTN_PIN)==LOW&&millis()-lastBtnMs>300){
    lastBtnMs=millis();
    currentScreen=(currentScreen+1)%SCREEN_COUNT;
    screenChanged=true;
    Serial.printf("[BTN] Screen %d\n",currentScreen);
  }

  targetEl=constrain((float)targetEl,0.0f,90.0f);
  targetAz=fmodf((float)targetAz+360.0f,360.0f);
  azStepper.moveTo((long)(targetAz*STEPS_PER_DEG));
  elStepper.moveTo((long)(targetEl*STEPS_PER_DEG));
  azStepper.run();elStepper.run();
  if(elStepper.currentPosition()<0)elStepper.setCurrentPosition(0);
  currentAz=azStepper.currentPosition()/STEPS_PER_DEG;
  currentEl=elStepper.currentPosition()/STEPS_PER_DEG;
  if(currentEl<0)currentEl=0;
  isMoving=azStepper.isRunning()||elStepper.isRunning();

  // IMU at 20 Hz
  static unsigned long lastImuMs=0;
  if(imuOK&&millis()-lastImuMs>=50){mpuRead();mpuCorrect(currentEl,elStepper,STEPS_PER_DEG,isMoving);lastImuMs=millis();}

  // Auto-park check
  checkAutopark();

  static unsigned long lastLog=0;
  if(onboardMode){
    if(tleLoaded&&ntpSynced&&millis()-lastSGP4Update>1000){runSGP4();lastSGP4Update=millis();}
    if(millis()-lastLog>5000){
      if(!tleLoaded)Serial.println("[SGP4] Waiting for TLE...");
      else if(!ntpSynced)Serial.println("[NTP] Waiting...");
      else Serial.printf("[TRACK] AZ%.1f EL%.1f IMU p%.2f r%.2f WX %.1fm/s\n",(float)targetAz,(float)targetEl,imuPitch,imuRoll,weather.windMps);
      lastLog=millis();
    }
  } else {
    if(millis()-lastLog>10000){Serial.println("[NET] Listening 4533");lastLog=millis();}
  }
}

// ================= EASYCOMM =================
void parseEasyComm(String cmd){
  cmd.trim();if(cmd.length()<1)return;
  if(cmd.startsWith("P ")||cmd.startsWith("p ")){int fs=cmd.indexOf(' '),ss=cmd.indexOf(' ',fs+1);if(fs!=-1&&ss!=-1){targetAz=cmd.substring(fs+1,ss).toFloat();targetEl=cmd.substring(ss+1).toFloat();}}
  else if(cmd.indexOf("AZ")!=-1||cmd.indexOf("az")!=-1){
    int ai=cmd.indexOf("AZ");if(ai==-1)ai=cmd.indexOf("az");int ei=cmd.indexOf("EL");if(ei==-1)ei=cmd.indexOf("el");
    if(ai!=-1&&ei!=-1){targetAz=cmd.substring(ai+2,ei).toFloat();int ee=cmd.indexOf(' ',ei);if(ee==-1)ee=cmd.length();targetEl=cmd.substring(ei+2,ee).toFloat();}
  }
  else if(cmd=="p"||cmd=="P"){if(tcpClient)tcpClient.printf("AZ%.2f EL%.2f\n",(float)currentAz,(float)currentEl);}
}

// ================= SGP4 RUNTIME =================
void runSGP4(){
  unsigned long now=timeClient.getEpochTime();if(now<1000000000UL)return;
  sat.findsat(now);
  unsigned long cm=millis();
  if(lastSatDist>0&&cm>lastDopplerTime){double dt=(cm-lastDopplerTime)/1000.0;if(dt>0){double rr=(sat.satDist-lastSatDist)/dt;dopplerFreq=(float)(145800000.0*(-rr/299792.458));}}
  lastSatDist=sat.satDist;lastDopplerTime=cm;satDistance=(float)sat.satDist;
  targetAz=(float)sat.satAz;targetEl=(sat.satEl<0)?0:(float)sat.satEl;
  satAltitude  =(float)sat.satAlt;   // (A4) publish true altitude for the footprint screen
  satFootprintKm=computeFootprintKm(sat.satAlt);   // (A4) true altitude, not slant range
  if(isGeoSat)nextMaxEl=(float)sat.satEl;
  if(isMoving||targetEl>0)lastActiveMs=millis();
  if(sat.satEl>=0){
    if(!passInProgress){passInProgress=true;passMaxEl=(float)sat.satEl;time_t t=(time_t)now;struct tm tn;gmtime_r(&t,&tn);snprintf(passStartBuf,sizeof(passStartBuf),"%02d:%02d:%02d",tn.tm_hour,tn.tm_min,tn.tm_sec);}
    else if((float)sat.satEl>passMaxEl)passMaxEl=(float)sat.satEl;
  } else if(passInProgress){
    passInProgress=false;if(!isGeoSat)logPass();
    // Auto-advance queue after pass
    if(queueCount>1){Serial.println("[QUEUE] Pass ended, checking next...");advanceQueue();}
  }
}