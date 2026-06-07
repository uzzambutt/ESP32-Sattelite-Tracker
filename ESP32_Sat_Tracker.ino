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
#include "qrcodegen.h" 
#include "secrets.h"

// =============================================
//   PROGMEM: MAIN TACTICAL DASHBOARD
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
      <div id="passLogBody" style="font-family:monospace;font-size:0.8rem;color:#94a3b8">No passes logged yet.</div>
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

  function processTLEText(txt, btn) {
    allTleData = [];
    const lines = txt.split('\n');
    for(let i=0; i<lines.length-2; i+=3){
      const name = lines[i].trim();
      if(name.length > 0 && lines[i+1].startsWith('1 ') && lines[i+2].startsWith('2 ')){
        if(!name.includes('STARLINK') && !name.includes('ONEWEB') && !name.includes('FLOCK')) {
          allTleData.push({name, l1:lines[i+1].trim(), l2:lines[i+2].trim()});
        }
      }
    }
    btn.innerText = 'DB LOADED (' + allTleData.length + ' TARGETS)';
    btn.style.color = '#10b981'; btn.style.borderColor = '#10b981';
    document.getElementById('sat-filter').disabled = false;
    filterSats();
  }

  function fetchCelestrak() {
    const btn = document.getElementById('btn-fetch');
    const cachedDB = localStorage.getItem('celestrakDB');
    const cacheTime = localStorage.getItem('celestrakTime');
    const now = new Date().getTime();
    
    if (cachedDB && cacheTime && (now - cacheTime < 14400000)) {
       btn.innerText = 'LOADING FROM BROWSER CACHE...';
       setTimeout(() => processTLEText(cachedDB, btn), 500);
       return;
    }

    btn.innerText = 'DOWNLOADING (PLEASE WAIT)...';
    fetch('https://celestrak.org/NORAD/elements/gp.php?GROUP=active&FORMAT=tle')
    .then(r => {
      if(r.status === 429) throw new Error("RATE LIMITED");
      if(!r.ok) throw new Error("NETWORK ERROR");
      return r.text();
    })
    .then(txt => {
      localStorage.setItem('celestrakDB', txt);
      localStorage.setItem('celestrakTime', now);
      processTLEText(txt, btn);
    })
    .catch(e => {
      if (e.message === "RATE LIMITED") {
         btn.innerText = 'RATE LIMITED! LOADING OLD CACHE...';
      } else {
         btn.innerText = 'DOWNLOAD FAILED. LOADING CACHE...';
      }
      
      if (cachedDB) {
         setTimeout(() => processTLEText(cachedDB, btn), 1500);
      } else {
         btn.style.borderColor = "#ef4444";
         btn.style.color = "#ef4444";
         btn.innerText = 'API BLOCKED. NO CACHE AVAILABLE.';
      }
    });
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
//   PROGMEM: WIFI PROVISIONING PORTAL
// =============================================
const char wifi_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>WIFI CONFIGURATION</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Inter:wght@400;600&display=swap');
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: 'Inter', sans-serif; background: #030712; color: #e2e8f0; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; padding: 20px; text-align: center; }
    .panel { background: #0f172a; border: 1px solid #1e293b; border-radius: 8px; padding: 30px; width: 100%; max-width: 400px; box-shadow: 0 4px 6px rgba(0,0,0,0.5); }
    h2 { font-family: 'Share Tech Mono', monospace; color: #22d3ee; margin-bottom: 20px; letter-spacing: 2px; }
    p { color: #94a3b8; font-size: 0.9rem; margin-bottom: 25px; line-height: 1.5; }
    input { width: 100%; padding: 15px; margin-bottom: 15px; background: #020617; border: 1px solid #1e293b; color: #e2e8f0; border-radius: 4px; font-size: 1rem; outline: none; }
    input:focus { border-color: #22d3ee; }
    button { width: 100%; padding: 15px; background: rgba(34,211,238,0.1); color: #22d3ee; border: 1px solid #22d3ee; border-radius: 4px; cursor: pointer; font-weight: 600; letter-spacing: 2px; font-size: 1rem; transition: 0.2s; }
    button:hover { background: #22d3ee; color: #020617; }
  </style>
</head>
<body>
  <div class="panel" id="mainPanel">
    <h2>NETWORK SETUP</h2>
    <p>The Aerospace Tracker lost its network link. Enter your router credentials below to restore the SGP4 engine connection.</p>
    <input type="text" id="ssid" placeholder="WiFi Network Name (SSID)">
    <input type="password" id="pass" placeholder="WiFi Password">
    <button onclick="saveWifi()">SAVE & REBOOT</button>
  </div>
<script>
  function saveWifi() {
    const s = document.getElementById('ssid').value;
    const p = document.getElementById('pass').value;
    if(!s || !p) { alert("Please enter both SSID and Password"); return; }
    document.getElementById('mainPanel').innerHTML = "<h2>REBOOTING...</h2><p>Credentials saved. Please disconnect from this Access Point and return to your home WiFi network. The tracker will be available there shortly.</p>";
    fetch('/api/wifi', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ ssid: s, pass: p })
    });
  }
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
//   TFT COLOUR DEFINES
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
#define C_MGRAY   0x7BEF
#define C_DBLUE   0x0010
#define C_DRED    0x6000

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
//   STATE & CONFIG
// =============================================
char tleLine1[70] = "";
char tleLine2[70] = "";
char satName[25]  = "NO SAT";
volatile bool tleLoaded = false;
volatile bool spiLock   = false;

volatile float currentAz = 0.0, currentEl = 0.0;
volatile float targetAz  = 0.0, targetEl  = 0.0;
volatile bool  isMoving  = false;

const float STEPS_PER_DEGREE_AZ = 8.88;
const float STEPS_PER_DEGREE_EL = 8.88;

volatile bool onboardMode  = false;
volatile bool isAPMode     = false;
volatile bool triggerReboot = false;
double obsLat = 31.52, obsLon = 74.35, obsAlt = 0.21;
String maidenhead = "MM71dl";

volatile bool   ntpSynced    = false;
volatile double lastSatDist  = 0;
volatile float  dopplerFreq  = 0.0;
volatile float  satDistance  = 0.0;
unsigned long   lastDopplerTime = 0;
unsigned long   lastSGP4Update  = 0;

volatile time_t nextAosTime = 0;
volatile time_t nextLosTime = 0;
volatile float  nextMaxEl   = 0.0;

struct PassLog { char sat[25]; char time[20]; float maxEl; };
PassLog passLog[10];
int  passLogCount   = 0;
bool passInProgress = false;
float passMaxEl     = 0.0;
char  passStartBuf[20] = "";

struct PathPoint { float az, el; };
PathPoint globalPath[45];
volatile int  globalPathLen = 0;
unsigned long lastPathCalc  = 0;
volatile bool newPathReady  = false;

// Dynamic Cache
float  prev_cAz = -999, prev_cEl = -999;
float  prev_tAz = -999, prev_tEl = -999;
float  prev_dAz = -999, prev_dEl = -999;
float  prev_doppler = -999;
float  prev_dist    = -999;
int    prev_rssi    = 0;
int    prev_moving  = -1;
int    prev_ntp     = -1;
int    prev_l4s     = -1;
int    prev_mode    = -1;
char   prev_sat[25] = "";
unsigned long prev_uptime   = 0;
long          prev_countdown = -999;

int prev_antX = -1, prev_antY = -1;
int prev_satX = -1, prev_satY = -1;

int terminalY = 30;
TaskHandle_t Core0Task;

// =============================================
//   FORWARD DECLARATIONS
// =============================================
void parseEasyComm(String cmd);
void runSGP4();
void calculatePathPrediction();
void tftDrawStaticFrame();
void tftDrawAPMode();
void tftUpdateDynamic();
void tftRadarUpdate();
void setupWebServer();
void drawBootScreen();
void printBootTerminal(String msg);

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

String buildTelemetryJson() {
  StaticJsonDocument<512> doc;
  doc["tAz"]      = (float)targetAz;
  doc["tEl"]      = (float)targetEl;
  doc["cAz"]      = (float)currentAz;
  doc["cEl"]      = (float)currentEl;
  doc["isMoving"] = isMoving;
  doc["rssi"]     = isAPMode ? 0 : WiFi.RSSI();
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
//   TFT — BOOT SCREEN WITH QR CODE
// =============================================
void drawBootScreen() {
  tft.fillScreen(C_BLACK);

  // ── TOP HEADER ───────────────────────────────────────────
  tft.fillRect(0, 0, 240, 28, C_DBLUE);
  tft.fillRect(0, 0, 4, 28, C_CYAN);       // left accent
  tft.fillRect(236, 0, 4, 28, C_CYAN);     // right accent
  tft.setTextColor(C_CYAN);
  tft.setTextSize(1);
  tft.setCursor(8, 5);
  tft.print("ESP32 SATELLITE TRACKER");
  tft.setTextColor(C_MGRAY);
  tft.setCursor(8, 16);
  tft.print("v8.3  //  SGP4 ENGINE");

  // ── SEPARATOR ────────────────────────────────────────────
  tft.drawFastHLine(0, 28, 240, C_DGRAY);

  // ── DEVELOPER BLOCK ──────────────────────────────────────
  tft.setTextColor(C_MGRAY);
  tft.setTextSize(1);
  tft.setCursor(8, 36);
  tft.print("DEVELOPER:");
  tft.setTextColor(C_WHITE);
  tft.setCursor(8, 48);
  tft.print("Muhammad Uzzam Butt");

  // ── DIVIDER ──────────────────────────────────────────────
  tft.drawFastHLine(4, 62, 232, C_DGRAY);

  // ── QR CODE LABEL ────────────────────────────────────────
  tft.setTextColor(C_MGRAY);
  tft.setTextSize(1);
  tft.setCursor(8, 68);
  tft.print("SCAN FOR SOURCE CODE:");

  // ── QR CODE ──────────────────────────────────────────────
  const char* url = "https://github.com/uzzambutt/ESP32-Sattelite-Tracker";
  uint8_t *qrcode     = (uint8_t *)malloc(qrcodegen_BUFFER_LEN_MAX);
  uint8_t *tempBuffer = (uint8_t *)malloc(qrcodegen_BUFFER_LEN_MAX);

  if (qrcode && tempBuffer) {
    bool ok = qrcodegen_encodeText(url, tempBuffer, qrcode, qrcodegen_Ecc_LOW,
              qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
    if (ok) {
      int qrSize    = qrcodegen_getSize(qrcode);
      int pixelSize = 3;
      int offsetX   = (240 - (qrSize * pixelSize)) / 2;
      int offsetY   = 82;
      // White border behind QR
      tft.fillRect(offsetX - 4, offsetY - 4,
                   (qrSize * pixelSize) + 8,
                   (qrSize * pixelSize) + 8, C_WHITE);
      for (int y = 0; y < qrSize; y++) {
        for (int x = 0; x < qrSize; x++) {
          if (qrcodegen_getModule(qrcode, x, y)) {
            tft.fillRect(offsetX + (x * pixelSize),
                         offsetY + (y * pixelSize),
                         pixelSize, pixelSize, C_BLACK);
          }
        }
      }
    }
  }
  if (qrcode)     free(qrcode);
  if (tempBuffer) free(tempBuffer);

  // ── BOTTOM STATUS BAR ────────────────────────────────────
  tft.fillRect(0, 300, 240, 20, C_DBLUE);
  tft.fillRect(0, 300, 4, 20, C_CYAN);
  tft.setTextColor(C_MGRAY);
  tft.setTextSize(1);
  tft.setCursor(8, 307);
  tft.print("INITIALIZING SYSTEMS...");
}

// =============================================
//   TFT — BOOT TERMINAL
// =============================================
void printBootTerminal(String msg) {
  Serial.println("[BOOT] " + msg);
  if (terminalY > 300) {
    tft.fillScreen(C_BLACK);
    tft.fillRect(0, 0, 240, 20, C_DBLUE);
    tft.fillRect(0, 0, 4, 20, C_CYAN);
    tft.setTextColor(C_CYAN);
    tft.setTextSize(1);
    tft.setCursor(8, 6);
    tft.print("SYSTEM BOOT SEQUENCE...");
    tft.drawFastHLine(0, 20, 240, C_DGRAY);
    terminalY = 30;
  }
  tft.setTextSize(1);
  tft.setCursor(5, terminalY);
  tft.setTextColor(C_CYAN);
  tft.print("> ");
  tft.setTextColor(C_WHITE);
  tft.print(msg);
  terminalY += 12;
  delay(100);
}

// =============================================
//   TFT — AP SETUP MODE SCREEN
// =============================================
void tftDrawAPMode() {
  tft.fillScreen(C_BLACK);

  // ── HEADER ───────────────────────────────────────────────
  tft.fillRect(0, 0, 240, 24, C_DRED);
  tft.fillRect(0, 0, 4, 24, C_RED);
  tft.fillRect(236, 0, 4, 24, C_RED);
  tft.setTextColor(C_WHITE);
  tft.setTextSize(1);
  tft.setCursor(8, 5);
  tft.print("! NETWORK LINK FAILED !");
  tft.setCursor(8, 15);
  tft.setTextColor(0xFBE0);   // light orange-yellow
  tft.print("ENTERING SETUP MODE");

  tft.drawFastHLine(0, 24, 240, C_RED);

  // ── ERROR LABEL ──────────────────────────────────────────
  tft.setTextColor(C_RED);
  tft.setTextSize(2);
  tft.setCursor(10, 32);
  tft.print("WIFI FAIL");

  // ── INSTRUCTIONS BOX ─────────────────────────────────────
  tft.drawRoundRect(2, 58, 236, 128, 3, C_DGRAY);
  tft.fillRect(3, 59, 234, 12, C_DBLUE);
  tft.setTextColor(C_CYAN);
  tft.setTextSize(1);
  tft.setCursor(6, 61);
  tft.print("[ SETUP INSTRUCTIONS ]");
  tft.drawFastHLine(3, 71, 234, C_DGRAY);

  // Step 1
  tft.setTextColor(C_ORANGE);
  tft.setCursor(6, 76);  tft.print("1.");
  tft.setTextColor(C_MGRAY);
  tft.setCursor(20, 76); tft.print("Join this WiFi network:");
  tft.setTextColor(C_CYAN);
  tft.setCursor(20, 88); tft.print("AEROSPACE-TRACKER");

  // Step 2
  tft.setTextColor(C_ORANGE);
  tft.setCursor(6, 104);  tft.print("2.");
  tft.setTextColor(C_MGRAY);
  tft.setCursor(20, 104); tft.print("WiFi Password:");
  tft.setTextColor(C_WHITE);
  tft.setCursor(20, 116); tft.print("groundstation");

  // Step 3
  tft.setTextColor(C_ORANGE);
  tft.setCursor(6, 132);  tft.print("3.");
  tft.setTextColor(C_MGRAY);
  tft.setCursor(20, 132); tft.print("Open browser, go to:");
  tft.setTextColor(C_GREEN);
  tft.setCursor(20, 144); tft.print("192.168.4.1");

  // Step 4
  tft.setTextColor(C_ORANGE);
  tft.setCursor(6, 160);  tft.print("4.");
  tft.setTextColor(C_MGRAY);
  tft.setCursor(20, 160); tft.print("Enter new credentials.");

  // ── WAITING BOX ──────────────────────────────────────────
  tft.drawRoundRect(2, 192, 236, 24, 3, C_DGRAY);
  tft.fillRect(3, 193, 234, 22, 0x0820);
  tft.setTextColor(C_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(6, 199);
  tft.print("WAITING FOR CREDENTIALS...");

  // ── BOTTOM STATUS BAR ────────────────────────────────────
  tft.fillRect(0, 300, 240, 20, C_DRED);
  tft.fillRect(0, 300, 4, 20, C_RED);
  tft.setTextColor(C_MGRAY);
  tft.setTextSize(1);
  tft.setCursor(8, 307);
  tft.print("AP MODE  |  192.168.4.1");
}

// =============================================
//   TFT — STATIC HUD FRAME
// =============================================
void tftDrawStaticFrame() {
  tft.fillScreen(C_BLACK);

  // ── TOP HEADER BAR ───────────────────────────────────────
  tft.fillRect(0, 0, 240, 22, C_DBLUE);
  tft.fillRect(0, 0, 4, 22, C_CYAN);        // left accent
  tft.fillRect(236, 0, 4, 22, C_CYAN);      // right accent
  tft.setTextColor(C_WHITE);
  tft.setTextSize(1);
  tft.setCursor(8, 5);
  tft.print("AEROSPACE TACTICAL HUD");
  tft.setTextColor(C_MGRAY);
  tft.setCursor(8, 14);
  tft.print("SGP4 ENGINE  |  L4S READY");
  tft.drawFastHLine(0, 22, 240, C_DGRAY);

  // ── SECTION 1: TARGET ────────────────────────────────────
  // Outer border
  tft.drawRoundRect(2, 25, 236, 72, 3, C_CYAN);
  // Title bar
  tft.fillRect(3, 26, 234, 12, C_DBLUE);
  tft.setTextColor(C_CYAN);
  tft.setTextSize(1);
  tft.setCursor(6, 28);
  tft.print("[ TARGET ]");
  tft.drawFastHLine(3, 38, 234, C_DGRAY);

  // Satellite name row
  tft.setTextColor(C_MGRAY);
  tft.setCursor(6, 41);
  tft.print("SAT:");

  // AZ / EL row
  tft.setCursor(6,  56); tft.print("AZ :");
  tft.setCursor(120, 56); tft.print("EL :");
  tft.drawFastHLine(3, 52, 234, 0x1084);    // faint row divider

  // Delta row
  tft.setCursor(6,  70); tft.print("dAZ:");
  tft.setCursor(120, 70); tft.print("dEL:");
  tft.drawFastHLine(3, 66, 234, 0x1084);

  // Vertical column divider between left/right values
  tft.drawFastVLine(114, 39, 58, 0x1084);

  // ── SECTION 2: SYSTEM STATUS ─────────────────────────────
  tft.drawRoundRect(2, 100, 236, 44, 3, C_DGRAY);
  tft.fillRect(3, 101, 234, 12, 0x0820);
  tft.setTextColor(C_ORANGE);
  tft.setTextSize(1);
  tft.setCursor(6, 103);
  tft.print("[ SYSTEM ]");
  tft.drawFastHLine(3, 113, 234, C_DGRAY);
  tft.drawFastVLine(114, 101, 43, 0x1084);

  tft.setTextColor(C_MGRAY);
  tft.setCursor(6,   116); tft.print("ENG:");
  tft.setCursor(120,  116); tft.print("SYS:");
  tft.drawFastHLine(3, 127, 234, 0x1084);
  tft.setCursor(6,   130); tft.print("L4S:");
  tft.setCursor(120,  130); tft.print("NTP:");

  // ── SECTION 3: PASS PREDICTION ───────────────────────────
  tft.drawRoundRect(2, 147, 236, 44, 3, C_DGRAY);
  tft.fillRect(3, 148, 234, 12, 0x0820);
  tft.setTextColor(C_ORANGE);
  tft.setTextSize(1);
  tft.setCursor(6, 150);
  tft.print("[ PASS PREDICTION ]");
  tft.drawFastHLine(3, 160, 234, C_DGRAY);
  tft.drawFastVLine(114, 148, 43, 0x1084);

  tft.setTextColor(C_MGRAY);
  tft.setCursor(6,   163); tft.print("AOS:");
  tft.setCursor(120,  163); tft.print("MAX:");
  tft.drawFastHLine(3, 174, 234, 0x1084);
  tft.setCursor(6,   177); tft.print("LOS:");
  tft.setCursor(120,  177); tft.print("T- :");

  // ── SECTION 4: RADAR ─────────────────────────────────────
  tft.drawRoundRect(2, 194, 236, 124, 3, C_DGRAY);
  tft.fillRect(3, 195, 234, 12, 0x0820);
  tft.setTextColor(C_CYAN);
  tft.setTextSize(1);
  tft.setCursor(6, 197);
  tft.print("[ TACTICAL RADAR ]");
  tft.drawFastHLine(3, 207, 234, C_DGRAY);

  // Draw radar rings and crosshairs
  int cx = 120, cy = 253, r = 52;
  tft.drawCircle(cx, cy, r,       C_DGRAY);
  tft.drawCircle(cx, cy, r*2/3,   C_DGRAY);
  tft.drawCircle(cx, cy, r*1/3,   C_DGRAY);
  tft.drawFastVLine(cx, cy - r, r * 2 + 1, C_DGRAY);
  tft.drawFastHLine(cx - r, cy, r * 2 + 1, C_DGRAY);

  // Tick marks at 45° intervals
  for (int i = 0; i < 360; i += 45) {
    float rad = i * PI / 180.0;
    tft.drawLine(
      cx + (int)((r - 4) * cos(rad)), cy + (int)((r - 4) * sin(rad)),
      cx + (int)(r       * cos(rad)), cy + (int)(r       * sin(rad)),
      C_MGRAY
    );
  }

  // Cardinal labels
  tft.setTextColor(C_CYAN);
  tft.setCursor(cx - 3,  cy - r - 10); tft.print("N");
  tft.setCursor(cx - 3,  cy + r + 3);  tft.print("S");
  tft.setCursor(cx + r + 4, cy - 3);   tft.print("E");
  tft.setCursor(cx - r - 9, cy - 3);   tft.print("W");

  // Elevation ring labels (30°, 60°)
  tft.setTextColor(C_DGRAY);
  tft.setCursor(cx + 2, cy - r*1/3 - 4); tft.print("60");
  tft.setCursor(cx + 2, cy - r*2/3 - 4); tft.print("30");

  // ── BOTTOM STATUS BAR ────────────────────────────────────
  tft.fillRect(0, 300, 240, 20, C_DBLUE);
  tft.fillRect(0, 300, 4, 20, C_ORANGE);
  tft.setTextColor(C_MGRAY);
  tft.setTextSize(1);
  tft.setCursor(8, 307);
  tft.print("NET: ");
  // IP will be filled in by tftUpdateDynamic on first run
}

// =============================================
//   TFT — DYNAMIC HUD UPDATE
// =============================================
void tftUpdateDynamic() {
  if (spiLock || isAPMode) return;
  char buf[32];

  // ── Satellite Name ────────────────────────────────────────
  if (strcmp(satName, prev_sat) != 0) {
    tft.fillRect(32, 41, 200, 9, C_BLACK);
    tft.setTextColor(C_YELLOW);
    tft.setTextSize(1);
    tft.setCursor(32, 41);
    snprintf(buf, sizeof(buf), "%.26s", satName);
    tft.print(buf);
    strncpy(prev_sat, satName, 24);
  }

  float cAz = currentAz, tAz = targetAz;
  float cEl = currentEl, tEl = targetEl;

  // ── Azimuth Column ───────────────────────────────────────
  if (fabsf(cAz - prev_cAz) > 0.05f || fabsf(tAz - prev_tAz) > 0.05f) {
    tft.fillRect(25, 56, 86, 28, C_BLACK);
    tft.setTextColor(C_WHITE);
    tft.setTextSize(2);
    tft.setCursor(25, 56);
    snprintf(buf, sizeof(buf), "%05.1f", (double)cAz);
    tft.print(buf);
    tft.setTextColor(C_GREEN);
    tft.setTextSize(1);
    tft.setCursor(25, 70);
    snprintf(buf, sizeof(buf), ">%05.1f", (double)tAz);
    tft.print(buf);

    // delta AZ
    tft.fillRect(32, 70, 78, 9, C_BLACK);
    float dAz = tAz - cAz;
    if (dAz >  180) dAz -= 360;
    if (dAz < -180) dAz += 360;
    tft.setTextColor(fabsf(dAz) > 1.0f ? C_ORANGE : C_MGRAY);
    tft.setTextSize(1);
    tft.setCursor(32, 70);
    snprintf(buf, sizeof(buf), "%+06.1f", (double)dAz);
    tft.print(buf);

    prev_cAz = cAz; prev_tAz = tAz;
  }

  // ── Elevation Column ─────────────────────────────────────
  if (fabsf(cEl - prev_cEl) > 0.05f || fabsf(tEl - prev_tEl) > 0.05f) {
    tft.fillRect(144, 56, 90, 28, C_BLACK);
    tft.setTextColor(C_WHITE);
    tft.setTextSize(2);
    tft.setCursor(144, 56);
    snprintf(buf, sizeof(buf), "%04.1f", (double)cEl);
    tft.print(buf);
    tft.setTextColor(C_GREEN);
    tft.setTextSize(1);
    tft.setCursor(144, 70);
    snprintf(buf, sizeof(buf), ">%04.1f", (double)tEl);
    tft.print(buf);

    // delta EL
    tft.fillRect(146, 70, 78, 9, C_BLACK);
    float dEl = tEl - cEl;
    tft.setTextColor(fabsf(dEl) > 1.0f ? C_ORANGE : C_MGRAY);
    tft.setTextSize(1);
    tft.setCursor(146, 70);
    snprintf(buf, sizeof(buf), "%+05.1f", (double)dEl);
    tft.print(buf);

    prev_cEl = cEl; prev_tEl = tEl;
  }

  // ── Engine Mode ──────────────────────────────────────────
  int modeVal = onboardMode ? 1 : 0;
  if (modeVal != prev_mode) {
    tft.fillRect(32, 116, 78, 9, C_BLACK);
    tft.setTextColor(onboardMode ? C_CYAN : C_YELLOW);
    tft.setTextSize(1);
    tft.setCursor(32, 116);
    tft.print(onboardMode ? "INT SGP4" : "EXT L4S");
    prev_mode = modeVal;
  }

  // ── System / Moving ──────────────────────────────────────
  if ((int)isMoving != prev_moving) {
    tft.fillRect(146, 116, 86, 9, C_BLACK);
    tft.setTextColor(isMoving ? C_ORANGE : C_GREEN);
    tft.setTextSize(1);
    tft.setCursor(146, 116);
    tft.print(isMoving ? "SLEWING" : "IDLE");
    prev_moving = isMoving;
  }

  // ── L4S TCP Link ─────────────────────────────────────────
  bool l4sNow = (tcpClient && tcpClient.connected());
  if ((int)l4sNow != prev_l4s) {
    tft.fillRect(32, 130, 78, 9, C_BLACK);
    tft.setTextColor(l4sNow ? C_GREEN : C_MGRAY);
    tft.setTextSize(1);
    tft.setCursor(32, 130);
    tft.print(l4sNow ? "LINKED" : "WAIT TCP");
    prev_l4s = l4sNow;
  }

  // ── NTP Status ───────────────────────────────────────────
  if ((int)ntpSynced != prev_ntp) {
    tft.fillRect(146, 130, 86, 9, C_BLACK);
    tft.setTextColor(ntpSynced ? C_GREEN : C_RED);
    tft.setTextSize(1);
    tft.setCursor(146, 130);
    tft.print(ntpSynced ? "SYNC OK" : "NO SYNC");
    prev_ntp = ntpSynced;
  }

  // ── IP Address (bottom bar, drawn once) ──────────────────
  static bool ipDrawn = false;
  if (!ipDrawn) {
    tft.fillRect(40, 307, 196, 9, C_BLACK);
    tft.setTextColor(C_WHITE);
    tft.setTextSize(1);
    tft.setCursor(40, 307);
    if (isAPMode) {
      tft.print("AP: 192.168.4.1");
    } else {
      tft.print(WiFi.localIP().toString());
    }
    ipDrawn = true;
  }

  // ── Pass Prediction ──────────────────────────────────────
  unsigned long nowEpoch = timeClient.getEpochTime();
  long countdown = (long)(nextAosTime - nowEpoch);

  if (countdown != prev_countdown) {
    tft.fillRect(32, 163, 78, 9,  C_BLACK);
    tft.fillRect(32, 177, 78, 9,  C_BLACK);
    tft.fillRect(146, 163, 86, 9, C_BLACK);
    tft.fillRect(146, 177, 86, 9, C_BLACK);

    tft.setTextSize(1);
    if (nextAosTime > 0) {
      struct tm *tmAos = gmtime((const time_t *)&nextAosTime);
      struct tm *tmLos = gmtime((const time_t *)&nextLosTime);

      tft.setTextColor(C_WHITE);
      tft.setCursor(32, 163);
      snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tmAos->tm_hour, tmAos->tm_min, tmAos->tm_sec);
      tft.print(buf);

      tft.setCursor(32, 177);
      snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tmLos->tm_hour, tmLos->tm_min, tmLos->tm_sec);
      tft.print(buf);

      tft.setTextColor(C_YELLOW);
      tft.setCursor(146, 163);
      snprintf(buf, sizeof(buf), "%.1f*", (double)nextMaxEl);
      tft.print(buf);

      tft.setCursor(146, 177);
      if (countdown < 0 && nowEpoch < nextLosTime) {
        tft.setTextColor(C_GREEN);
        tft.print("TRACKING");
      } else if (countdown >= 0) {
        tft.setTextColor(C_ORANGE);
        snprintf(buf, sizeof(buf), "-%02ld:%02ld:%02ld",
                 countdown / 3600, (countdown % 3600) / 60, countdown % 60);
        tft.print(buf);
      } else {
        tft.setTextColor(C_MGRAY);
        tft.print("ACQUIRING");
      }
    } else {
      tft.setTextColor(C_RED);
      tft.setCursor(32, 163); tft.print("NO TLE");
      tft.setCursor(32, 177); tft.print("NO DATA");
    }
    prev_countdown = countdown;
  }
}

// =============================================
//   TFT — RADAR ENGINE
// =============================================
void tftRadarUpdate() {
  if (spiLock || isAPMode) return;
  int cx = 120, cy = 253, r = 52;

  auto toXY = [cx, cy, r](float az, float el, int &x, int &y) {
    float rr  = r * (1.0f - el / 90.0f);
    float rad = (az - 90.0f) * PI / 180.0f;
    x = cx + (int)(rr * cosf(rad));
    y = cy + (int)(rr * sinf(rad));
  };

  if (newPathReady) {
    // Full radar wipe and redraw background
    tft.fillCircle(cx, cy, r - 1, C_BLACK);
    tft.drawCircle(cx, cy, r,       C_DGRAY);
    tft.drawCircle(cx, cy, r * 2/3, C_DGRAY);
    tft.drawCircle(cx, cy, r * 1/3, C_DGRAY);
    tft.drawFastVLine(cx, cy - r + 1, r * 2 - 1, C_DGRAY);
    tft.drawFastHLine(cx - r + 1, cy, r * 2 - 1, C_DGRAY);
    for (int i = 0; i < 360; i += 45) {
      float rad = i * PI / 180.0;
      tft.drawLine(
        cx + (int)((r - 4) * cos(rad)), cy + (int)((r - 4) * sin(rad)),
        cx + (int)(r       * cos(rad)), cy + (int)(r       * sin(rad)),
        C_MGRAY
      );
    }
    prev_antX = -1; prev_satX = -1;
    newPathReady = false;
  } else {
    // Erase old antenna needle and satellite dot
    if (prev_antX >= 0) {
      tft.drawLine(cx, cy, prev_antX, prev_antY, C_BLACK);
      tft.fillCircle(prev_antX, prev_antY, 3, C_BLACK);
    }
    if (prev_satX >= 0) {
      tft.fillCircle(prev_satX, prev_satY, 4, C_BLACK);
      tft.drawCircle(prev_satX, prev_satY, 7, C_BLACK);
    }
    // Redraw rings after erase
    tft.drawCircle(cx, cy, r * 2/3, C_DGRAY);
    tft.drawCircle(cx, cy, r * 1/3, C_DGRAY);
    tft.drawFastVLine(cx, cy - r + 1, r * 2 - 1, C_DGRAY);
    tft.drawFastHLine(cx - r + 1, cy, r * 2 - 1, C_DGRAY);
  }

  // Draw pass trajectory path
  int len = globalPathLen;
  if (len > 1) {
    int lastX = -1, lastY = -1;
    for (int i = 0; i < len; i++) {
      int px, py;
      toXY(globalPath[i].az, globalPath[i].el, px, py);
      if (lastX != -1) tft.drawLine(lastX, lastY, px, py, C_GREEN);
      if (i == 0) {
        tft.setTextColor(C_GREEN); tft.setTextSize(1);
        tft.setCursor(px - 6, py - 8); tft.print("A");
      } else if (i == len - 1) {
        tft.setTextColor(C_RED); tft.setTextSize(1);
        tft.setCursor(px - 6, py - 8); tft.print("L");
      }
      lastX = px; lastY = py;
    }
  }

  // Draw antenna needle (current position)
  int antX, antY;
  toXY((float)currentAz, (float)currentEl, antX, antY);
  tft.drawLine(cx, cy, antX, antY, C_BLUE);
  tft.fillCircle(antX, antY, 3, C_CYAN);
  prev_antX = antX; prev_antY = antY;

  // Draw satellite target dot
  float tEl = targetEl;
  if (tEl > 0.0f) {
    int satX, satY;
    toXY((float)targetAz, tEl, satX, satY);
    tft.fillCircle(satX, satY, 4, C_CYAN);
    tft.drawCircle(satX, satY, 7, C_CYAN);
    prev_satX = satX; prev_satY = satY;
  } else {
    prev_satX = -1; prev_satY = -1;
  }
}

// =============================================
//   PASS LOG
// =============================================
void logPass() {
  if (passLogCount >= 10) {
    for (int i = 0; i < 9; i++) passLog[i] = passLog[i + 1];
    passLogCount = 9;
  }
  strncpy(passLog[passLogCount].sat,  satName,      24);
  strncpy(passLog[passLogCount].time, passStartBuf, 19);
  passLog[passLogCount].maxEl = passMaxEl;
  passLogCount++;
  Serial.printf("[SGP4] Pass Log Entry Committed for %s | MaxEl: %.1f\n", satName, passMaxEl);
}

// =============================================
//   WEB SERVER ROUTING
// =============================================
void setupWebServer() {
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (isAPMode) request->send_P(200, "text/html", wifi_html);
    else          request->send_P(200, "text/html", index_html);
  });

  webServer.on("/api/wifi", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      String body = "";
      for (size_t i = 0; i < len; i++) body += (char)data[i];
      StaticJsonDocument<256> doc;
      if (!deserializeJson(doc, body)) {
        String newSSID = doc["ssid"] | "";
        String newPass = doc["pass"] | "";
        if (newSSID.length() > 0) {
          prefs.begin("sattracker", false);
          prefs.putString("wifi_ssid", newSSID);
          prefs.putString("wifi_pass", newPass);
          prefs.end();
          Serial.printf("[WIFI] New Credentials Provisioned: %s\n", newSSID.c_str());
          triggerReboot = true;
        }
      }
    }
  );

  webServer.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", buildTelemetryJson());
  });

  webServer.on("/api/path", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!tleLoaded || !ntpSynced || !onboardMode) {
      request->send(200, "application/json", "[]"); return;
    }
    String json = "[";
    int len = globalPathLen;
    for (int i = 0; i < len; i++) {
      if (i > 0) json += ",";
      json += "{\"az\":" + String(globalPath[i].az, 1) +
              ",\"el\":" + String(globalPath[i].el, 1) + "}";
    }
    json += "]";
    request->send(200, "application/json", json);
  });

  webServer.on("/api/passlog", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "[";
    for (int i = 0; i < passLogCount; i++) {
      if (i > 0) json += ",";
      json += "{\"sat\":\"" + String(passLog[i].sat)  +
              "\",\"time\":\"" + String(passLog[i].time) +
              "\",\"maxEl\":"  + String(passLog[i].maxEl, 1) + "}";
    }
    json += "]";
    request->send(200, "application/json", json);
  });

  webServer.on("/api/config", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      String body = "";
      for (size_t i = 0; i < len; i++) body += (char)data[i];
      StaticJsonDocument<256> doc;
      if (!deserializeJson(doc, body)) {
        if (doc.containsKey("grid")) {
          maidenhead = doc["grid"].as<String>();
          maidenheadToLatLon(maidenhead, obsLat, obsLon);
          prefs.begin("sattracker", false);
          prefs.putString("grid", maidenhead);
          prefs.end();
          lastPathCalc = 0;
          Serial.printf("[CONFIG] Observer Ground Coordinates Restructured: %s\n", maidenhead.c_str());
        }
        if (doc.containsKey("obMode")) {
          onboardMode = doc["obMode"].as<bool>();
          prefs.begin("sattracker", false);
          prefs.putBool("obMode", onboardMode);
          prefs.end();
          lastPathCalc = 0;
          Serial.printf("[CONFIG] Engine Handover Executed. Mode -> %s\n",
                        onboardMode ? "INTERNAL SGP4" : "EXTERNAL INTERFACES");
        }
      }
    }
  );

  webServer.on("/api/tle", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      String body = "";
      for (size_t i = 0; i < len; i++) body += (char)data[i];
      StaticJsonDocument<512> doc;
      if (!deserializeJson(doc, body)) {
        String name = doc["name"]  | "UNKNOWN";
        String l1   = doc["line1"] | "";
        String l2   = doc["line2"] | "";
        if (l1.length() > 0 && l2.length() > 0) {
          File f = LittleFS.open("/tle.txt", "w");
          if (f) { f.println(name); f.println(l1); f.println(l2); f.close(); }
          name.toCharArray(satName, 25);
          l1.toCharArray(tleLine1, 70);
          l2.toCharArray(tleLine2, 70);
          sat.site(obsLat, obsLon, obsAlt);
          sat.init(satName, tleLine1, tleLine2);
          tleLoaded    = true;
          lastPathCalc = 0;
          prev_sat[0]  = '\0';
          newPathReady = true;
          Serial.printf("[SGP4] Local Keplerian Elements Updated for: %s\n", satName);
        }
      }
    }
  );

  webServer.on("/api/manual", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      String body = "";
      for (size_t i = 0; i < len; i++) body += (char)data[i];
      StaticJsonDocument<128> doc;
      if (!deserializeJson(doc, body)) {
        if (!onboardMode && !isAPMode) {
          spiLock = true;
          if (doc.containsKey("az")) targetAz = doc["az"].as<float>();
          if (doc.containsKey("el")) targetEl = doc["el"].as<float>();
          spiLock = false;
        }
      }
    }
  );

  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client,
                AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
      Serial.printf("[NET] WebSocket Client Connected: %u\n", client->id());
      client->text(buildTelemetryJson());
    }
  });

  webServer.addHandler(&ws);
  webServer.begin();
}

// =============================================
//   PATH PREDICTION
// =============================================
void calculatePathPrediction() {
  if (!tleLoaded || !ntpSynced || !onboardMode || isAPMode) {
    globalPathLen = 0; return;
  }

  Sgp4 pathSat;
  pathSat.site(obsLat, obsLon, obsAlt);
  pathSat.init(satName, tleLine1, tleLine2);

  unsigned long t = timeClient.getEpochTime();
  bool inPass = false, isGeo = false;

  pathSat.findsat(t);
  if (pathSat.satEl > 0) {
    inPass = true;
    int rewindLimit = 0;
    while (pathSat.satEl > 0 && rewindLimit < 1440) {
      t -= 60; pathSat.findsat(t);
      vTaskDelay(pdMS_TO_TICKS(2));
      rewindLimit++;
    }
    if (rewindLimit >= 1440) isGeo = true;
    t += 60;
  } else {
    for (int i = 0; i < 1440; i += 2) {
      t += 120; pathSat.findsat(t);
      if (pathSat.satEl > 0) { inPass = true; break; }
      if (i % 20 == 0) vTaskDelay(pdMS_TO_TICKS(2));
    }
    if (inPass) {
      int rewindLimit = 0;
      while (pathSat.satEl > 0 && rewindLimit < 1440) {
        t -= 60; pathSat.findsat(t);
        vTaskDelay(pdMS_TO_TICKS(2));
        rewindLimit++;
      }
      if (rewindLimit >= 1440) isGeo = true;
      t += 60;
    }
  }

  float tempMaxEl = 0.0;
  int   tempLen   = 0;

  if (inPass) {
    nextAosTime = t;
    if (isGeo) {
      Serial.println("[SGP4] GEO/High-Orbit detected. Skipping trajectory math.");
      nextLosTime = t + 86400;
      tempMaxEl   = pathSat.satEl;
    } else {
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
        vTaskDelay(pdMS_TO_TICKS(2));
      }
      nextLosTime = t;
    }
  } else {
    nextAosTime = 0; nextLosTime = 0;
  }

  nextMaxEl     = tempMaxEl;
  globalPathLen = tempLen;
  newPathReady  = true;
}

// =============================================
//   CORE 0 TASK
// =============================================
void Core0TaskCode(void *pvParameters) {
  for (;;) {
    esp_task_wdt_reset();

    if (WiFi.status() == WL_CONNECTED && !isAPMode) {
      if (timeClient.update()) ntpSynced = true;
    }

    static unsigned long lastWifiCheck = 0;
    if (millis() - lastWifiCheck > 10000 && !isAPMode) {
      lastWifiCheck = millis();
      if (WiFi.status() != WL_CONNECTED) WiFi.reconnect();
    }

    if (!tcpClient || !tcpClient.connected()) {
      tcpClient = tcpServer.available();
      if (tcpClient) tcpClient.setNoDelay(true);
    }

    if (tcpClient && tcpClient.available()) {
      tcpClient.setTimeout(5);
      String cmd = tcpClient.readStringUntil('\n');
      if (cmd.length() > 0) {
        if (!onboardMode && !isAPMode) {
          spiLock = true;
          parseEasyComm(cmd);
          spiLock = false;
        }
        tcpClient.println("OK");
      }
    }

    if (millis() - lastPathCalc > 60000 && !isAPMode) {
      calculatePathPrediction();
      lastPathCalc = millis();
    }

    static unsigned long lastWsPush = 0;
    if (millis() - lastWsPush > 1000 && !isAPMode) {
      if (ws.count() > 0) ws.textAll(buildTelemetryJson());
      lastWsPush = millis();
    }

    static unsigned long lastTft = 0;
    if (millis() - lastTft > 300 && !isAPMode) {
      tftUpdateDynamic();
      tftRadarUpdate();
      lastTft = millis();
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// =============================================
//   SETUP
// =============================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n====================================");
  Serial.println(" ESP32 Aerospace Tracker v8.3 (ST7789)");
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

  drawBootScreen();
  delay(5000);

  tft.fillScreen(C_BLACK);
  tft.fillRect(0, 0, 240, 20, C_DBLUE);
  tft.fillRect(0, 0, 4, 20, C_CYAN);
  tft.setTextColor(C_CYAN);
  tft.setTextSize(1);
  tft.setCursor(8, 6);
  tft.print("SYSTEM BOOT SEQUENCE...");
  tft.drawFastHLine(0, 20, 240, C_DGRAY);
  terminalY = 30;

  printBootTerminal("Initializing LittleFS Engine...");
  if (!LittleFS.begin(false)) { LittleFS.begin(true); }

  printBootTerminal("Loading NVRAM Prefs...");
  prefs.begin("sattracker", true);
  maidenhead  = prefs.getString("grid", maidenhead);
  onboardMode = prefs.getBool("obMode", false);
  String savedSSID = prefs.getString("wifi_ssid", ssid);
  String savedPass = prefs.getString("wifi_pass", password);
  prefs.end();

  maidenheadToLatLon(maidenhead, obsLat, obsLon);
  sat.site(obsLat, obsLon, obsAlt);

  if (LittleFS.exists("/tle.txt")) {
    File f = LittleFS.open("/tle.txt", "r");
    if (f) {
      String n  = f.readStringUntil('\n'); n.trim();  n.toCharArray(satName,  25);
      String l1 = f.readStringUntil('\n'); l1.trim(); l1.toCharArray(tleLine1, 70);
      String l2 = f.readStringUntil('\n'); l2.trim(); l2.toCharArray(tleLine2, 70);
      f.close();
      if (strlen(tleLine1) > 0 && strlen(tleLine2) > 0) {
        sat.init(satName, tleLine1, tleLine2);
        tleLoaded = true;
      }
    }
  }

  printBootTerminal("Coupling WiFi Radio...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPass.c_str());
  WiFi.setSleep(false);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startAttempt > 15000) { isAPMode = true; break; }
    delay(500);
  }

  if (isAPMode) {
    printBootTerminal("WARN: Network Timeout.");
    printBootTerminal("Executing Setup Gateway...");
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAP("AEROSPACE-TRACKER", "groundstation");
    setupWebServer();
    tcpServer.begin();
    delay(1000);
    tftDrawAPMode();
    return;
  }

  printBootTerminal("IP Assigned: " + WiFi.localIP().toString());
  if (MDNS.begin("sattracker")) {
    MDNS.addService("http",      "tcp", 80);
    MDNS.addService("easycomm", "tcp", 4533);
    printBootTerminal("mDNS Active: sattracker.local");
  }

  printBootTerminal("Syncing Reference Clock...");
  timeClient.begin();
  timeClient.update();
  if (timeClient.getEpochTime() > 1000000) {
    ntpSynced = true;
    printBootTerminal("NTP Sync Successful");
  }

  ArduinoOTA.setHostname("sattracker");
  ArduinoOTA.begin();

  setupWebServer();
  tcpServer.begin();
  printBootTerminal("TCP/Web Endpoints Online");

  if (tleLoaded && ntpSynced && onboardMode) {
    printBootTerminal("Compiling SGP4 Matrix...");
    calculatePathPrediction();
  }

  printBootTerminal("Transferring to Core 1 UI...");
  delay(1000);

  tftDrawStaticFrame();
  digitalWrite(ENABLE_PIN, LOW);

  xTaskCreatePinnedToCore(Core0TaskCode, "Core0Task", 16384, NULL, 1, &Core0Task, 0);
}

// =============================================
//   LOOP — CORE 1
// =============================================
void loop() {
  esp_task_wdt_reset();

  if (triggerReboot) { delay(1000); ESP.restart(); }
  if (isAPMode) return;

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
    if (tleLoaded && ntpSynced && millis() - lastSGP4Update > 1000) {
      spiLock = true;
      runSGP4();
      spiLock = false;
      lastSGP4Update = millis();
    }
    if (millis() - lastLog > 5000) {
      if (!tleLoaded)       Serial.println("[SGP4] Waiting on Web Interface Keplerian Upload...");
      else if (!ntpSynced)  Serial.println("[NTP]  Starved. Syncing Reference Time...");
      else Serial.printf("[SGP4] Vector -> AZ: %.1f | EL: %.1f | Range: %.0fkm\n",
                         (float)targetAz, (float)targetEl, (float)satDistance);
      lastLog = millis();
    }
  } else {
    if (millis() - lastLog > 10000) {
      Serial.println("[NET] Port 4533 Listening for Look4Sat Packets...");
      lastLog = millis();
    }
  }
}

// =============================================
//   EASYCOMM PARSER
// =============================================
void parseEasyComm(String cmd) {
  cmd.trim();
  if (cmd.length() < 2) return;
  if (cmd.startsWith("P ") || cmd.startsWith("p ")) {
    int fs = cmd.indexOf(" ");
    int ss = cmd.indexOf(" ", fs + 1);
    if (fs != -1 && ss != -1) {
      targetAz = cmd.substring(fs + 1, ss).toFloat();
      targetEl = cmd.substring(ss + 1).toFloat();
    }
  } else if (cmd.indexOf("AZ") != -1 || cmd.indexOf("az") != -1) {
    int ai = cmd.indexOf("AZ"); if (ai == -1) ai = cmd.indexOf("az");
    int ei = cmd.indexOf("EL"); if (ei == -1) ei = cmd.indexOf("el");
    if (ai != -1 && ei != -1) {
      targetAz = cmd.substring(ai + 2, ei).toFloat();
      int ee   = cmd.indexOf(" ", ei); if (ee == -1) ee = cmd.length();
      targetEl = cmd.substring(ei + 2, ee).toFloat();
    }
  } else if (cmd == "p" || cmd == "P") {
    if (tcpClient) tcpClient.printf("AZ%.2f EL%.2f\n", (float)currentAz, (float)currentEl);
  }
}

// =============================================
//   SGP4 RUNTIME
// =============================================
void runSGP4() {
  unsigned long now = timeClient.getEpochTime();
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

  targetAz = sat.satAz;
  targetEl = sat.satEl;

  if (sat.satEl >= 0.0) {
    if (!passInProgress) {
      passInProgress = true;
      passMaxEl      = sat.satEl;
      time_t t       = (time_t)now;
      struct tm *tm  = gmtime(&t);
      sprintf(passStartBuf, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
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