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

// ================= WEB DASHBOARD =================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>AEROSPACE CMD</title><style>
@import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Inter:wght@400;600;700&display=swap');
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Inter',sans-serif;color:#e2e8f0;min-height:100vh;display:flex;flex-direction:column;align-items:center;padding:20px;
background:#05060f;
background-image:radial-gradient(ellipse 80% 50% at 20% -10%,rgba(124,58,237,.18),transparent),
radial-gradient(ellipse 60% 40% at 90% 110%,rgba(34,211,238,.14),transparent),
radial-gradient(ellipse 50% 30% at 50% 50%,rgba(236,72,153,.05),transparent);
background-attachment:fixed}
h1{font-family:'Share Tech Mono',monospace;font-size:2rem;letter-spacing:5px;margin:15px 0 5px;text-align:center;
background:linear-gradient(90deg,#22d3ee,#a78bfa,#f472b6);-webkit-background-clip:text;background-clip:text;color:transparent;
filter:drop-shadow(0 0 14px rgba(34,211,238,.45))}
.subtitle{font-size:.7rem;color:#64748b;letter-spacing:4px;margin-bottom:22px;text-align:center}
.badge-container{display:flex;gap:10px;margin-bottom:25px;flex-wrap:wrap;justify-content:center}
.status-badge{padding:7px 18px;border-radius:20px;font-size:.72rem;font-weight:700;letter-spacing:2px;text-transform:uppercase;border:1px solid transparent}
.status-tracking{background:rgba(16,185,129,.12);color:#34d399;border-color:rgba(16,185,129,.45);box-shadow:0 0 16px rgba(16,185,129,.25)}
.status-offline{background:rgba(239,68,68,.12);color:#f87171;border-color:rgba(239,68,68,.45);box-shadow:0 0 16px rgba(239,68,68,.25)}
.status-mode{background:rgba(167,139,250,.12);color:#c4b5fd;border-color:rgba(167,139,250,.45);box-shadow:0 0 16px rgba(167,139,250,.2)}
.container{display:grid;grid-template-columns:1fr;gap:22px;width:100%;max-width:540px}
.panel{background:rgba(13,18,38,.78);backdrop-filter:blur(12px);border:1px solid rgba(148,163,184,.14);border-radius:16px;padding:22px;
box-shadow:0 10px 40px rgba(0,0,0,.5),inset 0 1px 0 rgba(255,255,255,.04);position:relative;overflow:hidden}
.panel::before{content:'';position:absolute;top:0;left:0;right:0;height:2px;background:linear-gradient(90deg,transparent,#22d3ee,#a78bfa,#f472b6,transparent);opacity:.7}
.panel-header{font-size:.72rem;color:#94a3b8;letter-spacing:3px;text-transform:uppercase;margin-bottom:16px;border-bottom:1px solid rgba(148,163,184,.12);
padding-bottom:9px;display:flex;align-items:center;gap:8px}
.panel-header::before{content:'';width:7px;height:7px;border-radius:50%;background:#22d3ee;box-shadow:0 0 10px #22d3ee;animation:pulse 2s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.35}}
.telemetry-grid{display:grid;grid-template-columns:1fr 1fr;gap:15px;margin-bottom:20px}
.data-card{background:rgba(2,6,23,.85);border:1px solid rgba(148,163,184,.12);border-radius:12px;padding:16px;text-align:center;position:relative;overflow:hidden}
.data-card::before{content:'';position:absolute;top:0;left:0;right:0;height:3px}
.data-card.az::before{background:linear-gradient(90deg,#06b6d4,#22d3ee);box-shadow:0 0 12px #22d3ee}
.data-card.el::before{background:linear-gradient(90deg,#d946ef,#f472b6);box-shadow:0 0 12px #f472b6}
.data-card.az .data-unit{color:#22d3ee}.data-card.el .data-unit{color:#f472b6}
.data-card.az .data-value{text-shadow:0 0 18px rgba(34,211,238,.5)}
.data-card.el .data-value{text-shadow:0 0 18px rgba(244,114,182,.5)}
.data-label{font-size:.62rem;letter-spacing:3px;color:#64748b;text-transform:uppercase;margin-bottom:6px}
.data-value{font-family:'Share Tech Mono',monospace;font-size:2.6rem;color:#f8fafc}
.data-unit{font-size:1rem;vertical-align:super}
.info-row{display:flex;justify-content:space-between;margin-bottom:9px;font-size:.85rem;border-bottom:1px dashed rgba(148,163,184,.12);padding-bottom:5px}
.info-key{color:#64748b}.info-val{font-family:'Share Tech Mono',monospace;color:#cbd5e1}
.highlight{color:#22d3ee;text-shadow:0 0 8px rgba(34,211,238,.4)}
.hl-pink{color:#f472b6;text-shadow:0 0 8px rgba(244,114,182,.4)}
.hl-lime{color:#a3e635;text-shadow:0 0 8px rgba(163,230,53,.4)}
input,select,button{width:100%;padding:13px;margin-bottom:12px;background:rgba(2,6,23,.85);border:1px solid rgba(148,163,184,.18);color:#e2e8f0;border-radius:10px;font-size:.85rem;outline:none;transition:.2s}
input:focus,select:focus{border-color:#22d3ee;box-shadow:0 0 14px rgba(34,211,238,.25)}
button{background:linear-gradient(135deg,rgba(34,211,238,.15),rgba(167,139,250,.15));color:#22d3ee;border:1px solid rgba(34,211,238,.5);cursor:pointer;font-weight:700;letter-spacing:1.5px;text-transform:uppercase}
button:hover{background:linear-gradient(135deg,#22d3ee,#a78bfa);color:#020617;box-shadow:0 0 24px rgba(34,211,238,.5)}
.input-group{display:flex;gap:8px}.input-group input{margin-bottom:0}.input-group button{margin-bottom:0;width:auto;flex-shrink:0}
.radar-wrapper{display:flex;justify-content:center;margin:20px 0}
canvas{background:radial-gradient(circle,#020a17 0%,#01040c 100%);border-radius:50%;box-shadow:0 0 40px rgba(34,211,238,.18),inset 0 0 60px rgba(34,211,238,.06)}
.joystick{display:grid;grid-template-columns:repeat(3,52px);gap:6px;justify-content:center;margin-top:15px}
.joystick button{padding:15px 0;margin:0;background:rgba(15,23,42,.9);border:1px solid rgba(148,163,184,.2);color:#94a3b8;border-radius:10px}
.joystick button:hover{background:linear-gradient(135deg,#22d3ee,#a78bfa);color:#020617}
.sat-filter{width:100%;padding:11px;margin-bottom:8px;background:rgba(2,6,23,.85);border:1px solid rgba(34,211,238,.4);color:#22d3ee;border-radius:10px;font-size:.85rem;outline:none}
.sat-filter::placeholder{color:rgba(34,211,238,.35)}
.conn-lost{display:none;position:fixed;top:0;left:0;width:100%;background:linear-gradient(90deg,#dc2626,#f43f5e);color:#fff;text-align:center;padding:9px;font-size:.8rem;font-weight:bold;letter-spacing:2px;z-index:999}
</style></head><body>
<div class="conn-lost" id="connLost">⚠ LINK LOST — REESTABLISHING...</div>
<h1>AEROSPACE CMD</h1><p class="subtitle">ESP32 ORBITAL GROUND STATION</p>
<div class="badge-container">
<div class="status-badge status-offline" id="statusBadge">CONNECTING...</div>
<div class="status-badge status-mode" id="modeBadge">--</div>
</div>
<div class="container">
<div class="panel"><div class="panel-header">Live Telemetry</div>
<div class="telemetry-grid">
<div class="data-card az"><div class="data-label">Azimuth</div><div class="data-value" id="curAz">--<span class="data-unit">°</span></div></div>
<div class="data-card el"><div class="data-label">Elevation</div><div class="data-value" id="curEl">--<span class="data-unit">°</span></div></div>
</div>
<div class="info-row"><span class="info-key">Target AZ</span><span class="info-val highlight" id="tgtAz">--°</span></div>
<div class="info-row"><span class="info-key">Target EL</span><span class="info-val hl-pink" id="tgtEl">--°</span></div>
<div class="info-row"><span class="info-key">Tracking Target</span><span class="info-val hl-lime" id="satName">--</span></div>
<div class="info-row"><span class="info-key">Doppler Shift</span><span class="info-val highlight" id="doppler">-- Hz</span></div>
<div class="info-row"><span class="info-key">Slant Range</span><span class="info-val" id="satDist">-- km</span></div>
<div class="info-row"><span class="info-key">WiFi Signal</span><span class="info-val" id="rssi">-- dBm</span></div>
<div class="info-row"><span class="info-key">System Uptime</span><span class="info-val" id="uptime">--</span></div>
<div class="info-row" style="border:none;margin-bottom:0"><span class="info-key">Free Memory</span><span class="info-val" id="ram">-- KB</span></div>
</div>
<div class="panel"><div class="panel-header">Tactical Radar</div>
<div class="radar-wrapper"><canvas id="radar" width="280" height="280"></canvas></div>
<div class="joystick">
<div></div><button onclick="nudge(0,5)">▲</button><div></div>
<button onclick="nudge(-5,0)">◀</button><div></div><button onclick="nudge(5,0)">▶</button>
<div></div><button onclick="nudge(0,-5)">▼</button><div></div>
</div></div>
<div class="panel"><div class="panel-header">Orbital Database — SGP4</div>
<div class="info-row" style="border:none;margin-bottom:12px">
<span class="info-key" style="line-height:2.5">Tracking Engine</span>
<select id="mode-select" onchange="updateMode()" style="width:170px;margin:0">
<option value="0">External (Look4Sat)</option><option value="1">Internal SGP4</option>
</select></div>
<div class="input-group">
<input type="text" id="grid-input" placeholder="Observer Grid (e.g. MM71dl)" maxlength="6">
<button onclick="updateGrid()">SET GRID</button></div>
<button onclick="fetchCelestrak()" id="btn-fetch">1. DOWNLOAD CELESTRAK DB</button>
<input type="text" id="sat-filter" class="sat-filter" placeholder="🔍 Filter satellites (e.g. NOAA, ISS)..." oninput="filterSats()" disabled>
<select id="sat-select" size="4" style="height:84px"><option>Awaiting Database...</option></select>
<button onclick="pushTLE()" id="btn-push">2. UPLOAD TLE TO HARDWARE</button>
</div>
<div class="panel"><div class="panel-header">Pass Log</div>
<button onclick="loadPassLog()" style="margin-bottom:12px">REFRESH PASS LOG</button>
<div id="passLogBody" style="font-family:monospace;font-size:.8rem;color:#94a3b8">No passes logged yet.</div>
</div></div>
<script>
let currentAz=0,currentEl=0,targetAz=0,targetEl=0;
let allTleData=[],tleData=[],satPath=[],isGeo=false;
let failCount=0,sweepAngle=0;
async function fetchTelemetry(){
 try{
  const r=await fetch('/api/status',{cache:'no-store'});
  if(!r.ok)throw new Error();
  const d=await r.json();
  failCount=0;
  document.getElementById('connLost').style.display='none';
  currentAz=d.cAz;currentEl=d.cEl;targetAz=d.tAz;targetEl=d.tEl;isGeo=!!d.geo;
  document.getElementById('curAz').innerHTML=currentAz.toFixed(1).padStart(5,'0')+'<span class="data-unit">°</span>';
  document.getElementById('curEl').innerHTML=currentEl.toFixed(1).padStart(4,'0')+'<span class="data-unit">°</span>';
  document.getElementById('tgtAz').innerText=targetAz.toFixed(1)+'°';
  document.getElementById('tgtEl').innerText=targetEl.toFixed(1)+'°';
  document.getElementById('satName').innerText=d.sat+(isGeo?' [GEO]':'');
  document.getElementById('ram').innerText=d.freeHeap+' KB';
  document.getElementById('doppler').innerText=(d.doppler||0).toFixed(0)+' Hz';
  document.getElementById('satDist').innerText=(d.dist||0).toFixed(0)+' km';
  document.getElementById('rssi').innerText=d.rssi+' dBm';
  const up=d.uptime;
  document.getElementById('uptime').innerText=`${Math.floor(up/3600)}h ${Math.floor((up%3600)/60)}m ${up%60}s`;
  document.getElementById('grid-input').placeholder=d.grid;
  document.getElementById('mode-select').value=d.mode===1?"1":"0";
  const badge=document.getElementById('statusBadge');
  badge.className='status-badge status-tracking';
  badge.innerText=d.isMoving?'⟳ SLEWING':'● LINK ACTIVE';
  document.getElementById('modeBadge').innerText=d.mode===1?(isGeo?'SGP4: GEO LOCK':'SGP4: ONBOARD'):'MODE: EXT L4S';
 }catch(e){
  failCount++;
  if(failCount>3){
   document.getElementById('connLost').style.display='block';
   document.getElementById('statusBadge').className='status-badge status-offline';
   document.getElementById('statusBadge').innerText='✕ OFFLINE';
  }
 }
}
async function fetchPath(){try{const r=await fetch('/api/path');if(r.ok)satPath=await r.json();}catch(e){}}
async function loadPassLog(){
 try{
  const r=await fetch('/api/passlog');const d=await r.json();
  const el=document.getElementById('passLogBody');
  if(!d.length){el.innerText='No passes logged yet.';return;}
  el.innerHTML=d.reverse().map(p=>
   `<div style="margin-bottom:6px;border-bottom:1px solid rgba(148,163,184,.12);padding-bottom:4px">
   <span style="color:#22d3ee">${p.sat}</span> &nbsp;
   <span style="color:#64748b">${p.time}</span> &nbsp;
   <span style="color:#34d399">MaxEl: ${parseFloat(p.maxEl).toFixed(1)}°</span></div>`).join('');
 }catch(e){document.getElementById('passLogBody').innerText='Failed to load.';}
}
function processTLEText(txt,btn){
 allTleData=[];
 const lines=txt.split('\n');
 for(let i=0;i<lines.length-2;i+=3){
  const name=lines[i].trim();
  if(name.length>0&&lines[i+1].startsWith('1 ')&&lines[i+2].startsWith('2 ')){
   if(!name.includes('STARLINK')&&!name.includes('ONEWEB')&&!name.includes('FLOCK'))
    allTleData.push({name,l1:lines[i+1].trim(),l2:lines[i+2].trim()});
  }
 }
 btn.innerText='DB LOADED ('+allTleData.length+' TARGETS)';
 btn.style.color='#34d399';btn.style.borderColor='#34d399';
 document.getElementById('sat-filter').disabled=false;
 filterSats();
}
function fetchCelestrak(){
 const btn=document.getElementById('btn-fetch');
 const cachedDB=localStorage.getItem('celestrakDB');
 const cacheTime=localStorage.getItem('celestrakTime');
 const now=Date.now();
 if(cachedDB&&cacheTime&&(now-cacheTime<14400000)){
  btn.innerText='LOADING FROM BROWSER CACHE...';
  setTimeout(()=>processTLEText(cachedDB,btn),500);return;
 }
 btn.innerText='DOWNLOADING (PLEASE WAIT)...';
 fetch('https://celestrak.org/NORAD/elements/gp.php?GROUP=active&FORMAT=tle')
 .then(r=>{
  if(r.status===429)throw new Error("RATE LIMITED");
  if(!r.ok)throw new Error("NETWORK ERROR");
  return r.text();
 })
 .then(txt=>{
  localStorage.setItem('celestrakDB',txt);
  localStorage.setItem('celestrakTime',now);
  processTLEText(txt,btn);
 })
 .catch(e=>{
  btn.innerText=e.message==="RATE LIMITED"?'RATE LIMITED! LOADING OLD CACHE...':'DOWNLOAD FAILED. LOADING CACHE...';
  if(cachedDB)setTimeout(()=>processTLEText(cachedDB,btn),1500);
  else{btn.style.borderColor="#ef4444";btn.style.color="#ef4444";btn.innerText='API BLOCKED. NO CACHE AVAILABLE.';}
 });
}
function filterSats(){
 const q=document.getElementById('sat-filter').value.toLowerCase();
 tleData=q?allTleData.filter(s=>s.name.toLowerCase().includes(q)):allTleData;
 document.getElementById('sat-select').innerHTML=
  tleData.slice(0,200).map((s,i)=>`<option value="${i}">${s.name}</option>`).join('');
}
function pushTLE(){
 const idx=document.getElementById('sat-select').value;
 if(!tleData[idx])return;
 const btn=document.getElementById('btn-push');
 btn.innerText='TRANSMITTING...';
 fetch('/api/tle',{method:'POST',headers:{'Content-Type':'application/json'},
  body:JSON.stringify({name:tleData[idx].name,line1:tleData[idx].l1,line2:tleData[idx].l2})
 }).then(()=>{
  btn.innerText='UPLOAD SUCCESSFUL';
  document.getElementById('mode-select').value='1';
  updateMode();
  setTimeout(()=>{btn.innerText='2. UPLOAD TLE TO HARDWARE';fetchPath();},3000);
 });
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
function nudge(az,el){
 targetAz=(targetAz+az+360)%360;
 targetEl=Math.max(0,Math.min(90,targetEl+el));
 fetch('/api/manual',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({az:targetAz,el:targetEl})});
}
function animateRadar(){
 requestAnimationFrame(animateRadar);
 const cv=document.getElementById('radar');
 const ctx=cv.getContext('2d');
 const cx=cv.width/2,cy=cv.height/2,r=cx-22;
 ctx.clearRect(0,0,cv.width,cv.height);
 const toXY=(az,el)=>{
  const rr=r*(1-el/90);
  const rad=(az-90)*Math.PI/180;
  return{x:cx+rr*Math.cos(rad),y:cy+rr*Math.sin(rad)};
 };
 if(satPath.length>1){
  ctx.strokeStyle='rgba(163,230,53,0.8)';ctx.lineWidth=2;ctx.setLineDash([4,4]);
  ctx.beginPath();
  satPath.forEach((pt,i)=>{
   const p=toXY(pt.az,pt.el);
   if(i===0)ctx.moveTo(p.x,p.y);else ctx.lineTo(p.x,p.y);
  });
  ctx.stroke();ctx.setLineDash([]);
  const aos=toXY(satPath[0].az,satPath[0].el);
  const los=toXY(satPath[satPath.length-1].az,satPath[satPath.length-1].el);
  ctx.fillStyle='#a3e635';ctx.font='10px "Share Tech Mono"';ctx.textAlign='center';
  ctx.fillText('AOS',aos.x,aos.y-8);
  ctx.fillStyle='#f87171';ctx.fillText('LOS',los.x,los.y-8);
 }
 ctx.strokeStyle='rgba(34,211,238,0.18)';ctx.lineWidth=1;
 [0.33,0.66,1].forEach(f=>{ctx.beginPath();ctx.arc(cx,cy,r*f,0,2*Math.PI);ctx.stroke();});
 ctx.beginPath();ctx.moveTo(cx,cy-r);ctx.lineTo(cx,cy+r);ctx.stroke();
 ctx.beginPath();ctx.moveTo(cx-r,cy);ctx.lineTo(cx+r,cy);ctx.stroke();
 ctx.textAlign='center';ctx.textBaseline='middle';
 for(let i=0;i<360;i+=30){
  const rad=(i-90)*Math.PI/180;
  ctx.strokeStyle=i%90===0?'#38bdf8':'#334155';ctx.lineWidth=i%90===0?2:1;
  ctx.beginPath();ctx.moveTo(cx+r*Math.cos(rad),cy+r*Math.sin(rad));
  ctx.lineTo(cx+(r+5)*Math.cos(rad),cy+(r+5)*Math.sin(rad));ctx.stroke();
  ctx.font=i%90===0?'bold 12px "Share Tech Mono"':'9px "Share Tech Mono"';
  ctx.fillStyle=i%90===0?'#38bdf8':'#64748b';
  if(i===0)ctx.fillText('N',cx,cy-r-14);
  else if(i===90)ctx.fillText('E',cx+r+14,cy);
  else if(i===180)ctx.fillText('S',cx,cy+r+14);
  else if(i===270)ctx.fillText('W',cx-r-14,cy);
  else ctx.fillText(i,cx+(r+15)*Math.cos(rad),cy+(r+15)*Math.sin(rad));
 }
 sweepAngle+=0.025;
 const g=ctx.createConicGradient?null:null;
 ctx.fillStyle='rgba(34,211,238,0.12)';
 ctx.beginPath();ctx.moveTo(cx,cy);ctx.arc(cx,cy,r,sweepAngle,sweepAngle+0.55);ctx.lineTo(cx,cy);ctx.fill();
 ctx.strokeStyle='rgba(34,211,238,0.7)';ctx.lineWidth=1.5;
 ctx.beginPath();ctx.moveTo(cx,cy);
 ctx.lineTo(cx+r*Math.cos(sweepAngle+0.55),cy+r*Math.sin(sweepAngle+0.55));ctx.stroke();
 const ap=toXY(currentAz,currentEl);
 ctx.strokeStyle='rgba(148,163,184,0.6)';ctx.lineWidth=2;
 ctx.beginPath();ctx.moveTo(cx,cy);ctx.lineTo(ap.x,ap.y);ctx.stroke();
 ctx.fillStyle='#38bdf8';ctx.beginPath();ctx.arc(ap.x,ap.y,5,0,2*Math.PI);ctx.fill();
 if(targetEl>0){
  const sp=toXY(targetAz,targetEl);
  ctx.fillStyle=isGeo?'#fbbf24':'#f472b6';
  ctx.shadowBlur=18;ctx.shadowColor=isGeo?'#fbbf24':'#f472b6';
  ctx.beginPath();ctx.arc(sp.x,sp.y,6,0,2*Math.PI);ctx.fill();
  ctx.shadowBlur=0;
  if(isGeo){ctx.fillStyle='#fbbf24';ctx.font='9px "Share Tech Mono"';ctx.fillText('GEO',sp.x,sp.y-12);}
 }
}
setInterval(fetchTelemetry,500);
setInterval(fetchPath,60000);
setTimeout(fetchPath,2000);
animateRadar();
</script></body></html>
)rawliteral";

// ================= WIFI PORTAL =================
const char wifi_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>WIFI CONFIGURATION</title><style>
@import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Inter:wght@400;600&display=swap');
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Inter',sans-serif;background:#05060f;background-image:radial-gradient(ellipse at 50% 0%,rgba(124,58,237,.2),transparent 60%);
color:#e2e8f0;display:flex;flex-direction:column;align-items:center;justify-content:center;height:100vh;padding:20px;text-align:center}
.panel{background:rgba(13,18,38,.85);border:1px solid rgba(148,163,184,.15);border-radius:16px;padding:32px;width:100%;max-width:400px;
box-shadow:0 10px 50px rgba(0,0,0,.6)}
h2{font-family:'Share Tech Mono',monospace;background:linear-gradient(90deg,#22d3ee,#a78bfa);-webkit-background-clip:text;background-clip:text;color:transparent;margin-bottom:20px;letter-spacing:2px}
p{color:#94a3b8;font-size:.9rem;margin-bottom:25px;line-height:1.5}
input{width:100%;padding:15px;margin-bottom:15px;background:rgba(2,6,23,.9);border:1px solid rgba(148,163,184,.2);color:#e2e8f0;border-radius:10px;font-size:1rem;outline:none}
input:focus{border-color:#22d3ee;box-shadow:0 0 14px rgba(34,211,238,.25)}
button{width:100%;padding:15px;background:linear-gradient(135deg,rgba(34,211,238,.15),rgba(167,139,250,.15));color:#22d3ee;border:1px solid rgba(34,211,238,.5);border-radius:10px;cursor:pointer;font-weight:600;letter-spacing:2px;font-size:1rem;transition:.2s}
button:hover{background:linear-gradient(135deg,#22d3ee,#a78bfa);color:#020617}
</style></head><body>
<div class="panel" id="mainPanel">
<h2>NETWORK SETUP</h2>
<p>The Aerospace Tracker lost its network link. Enter your router credentials below.</p>
<input type="text" id="ssid" placeholder="WiFi Network Name (SSID)">
<input type="password" id="pass" placeholder="WiFi Password">
<button onclick="saveWifi()">SAVE & REBOOT</button>
</div>
<script>
function saveWifi(){
 const s=document.getElementById('ssid').value;
 const p=document.getElementById('pass').value;
 if(!s||!p){alert("Please enter both SSID and Password");return;}
 document.getElementById('mainPanel').innerHTML="<h2>REBOOTING...</h2><p>Credentials saved. Return to your home WiFi network.</p>";
 fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:s,pass:p})});
}
</script></body></html>
)rawliteral";

// ================= PINS & COLORS =================
#define TFT_CS 15
#define TFT_DC 2
#define TFT_RST 4
#define AZ_STEP 32
#define AZ_DIR 14
#define EL_STEP 27
#define EL_DIR 26
#define ENABLE_PIN 25

#define C_BLACK  0x0000
#define C_WHITE  0xFFFF
#define C_CYAN   0x07FF
#define C_GREEN  0x07E0
#define C_LIME   0xAFE5
#define C_YELLOW 0xFFE0
#define C_AMBER  0xFD60
#define C_ORANGE 0xFD20
#define C_RED    0xF800
#define C_PINK   0xFB56
#define C_MAG    0xF81F
#define C_VIOLET 0xA15F
#define C_BLUE   0x34BF
#define C_DGRAY  0x2945
#define C_MGRAY  0x7BEF
#define C_FAINT  0x1084
#define C_HDRBG  0x0861   // dark slate
#define C_TEALBG 0x0149   // dark teal panel title
#define C_PURPBG 0x2808   // dark purple panel title
#define C_AMBBG  0x2940   // dark amber panel title
#define C_DRED   0x6000
#define C_DBLUE  0x0010

// ================= OBJECTS =================
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
AccelStepper azStepper(AccelStepper::DRIVER, AZ_STEP, AZ_DIR);
AccelStepper elStepper(AccelStepper::DRIVER, EL_STEP, EL_DIR);
AsyncWebServer webServer(80);
AsyncWebSocket ws("/ws");
WiFiServer tcpServer(4533);
WiFiClient tcpClient;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);
Preferences prefs;
Sgp4 sat;

// ================= STATE =================
char tleLine1[70] = "";
char tleLine2[70] = "";
char satName[25]  = "NO SAT";
volatile bool tleLoaded = false;
volatile bool spiLock = false;
volatile float currentAz = 0, currentEl = 0;
volatile float targetAz = 0, targetEl = 0;
volatile bool isMoving = false;
const float STEPS_PER_DEG = 8.88;
volatile bool onboardMode = false;
volatile bool isAPMode = false;
volatile bool triggerReboot = false;
volatile bool isGeoSat = false;
double obsLat = 31.52, obsLon = 74.35, obsAlt = 0.21;
String maidenhead = "MM71dl";
volatile bool ntpSynced = false;
volatile double lastSatDist = 0;
volatile float dopplerFreq = 0, satDistance = 0;
unsigned long lastDopplerTime = 0, lastSGP4Update = 0;
volatile time_t nextAosTime = 0, nextLosTime = 0;
volatile float nextMaxEl = 0;

struct PassLogEntry { char sat[25]; char time[20]; float maxEl; };
PassLogEntry passLog[10];
int passLogCount = 0;
bool passInProgress = false;
float passMaxEl = 0;
char passStartBuf[20] = "";

struct PathPoint { float az, el; };
PathPoint globalPath[45];
volatile int globalPathLen = 0;
unsigned long lastPathCalc = 0;
volatile bool newPathReady = false;

// ---- TFT dynamic cache ----
float prev_cAz, prev_cEl, prev_tAz, prev_tEl;
float prev_dist, prev_dop;
int   prev_rssi, prev_heap;
int   prev_moving, prev_ntp, prev_l4s, prev_mode, prev_geo;
char  prev_sat[25];
char  prev_clock[10];
long  prev_countdown;
float prev_maxElShown;
int prev_antX, prev_antY, prev_satX, prev_satY;
int prev_alX[2], prev_alY[2];
int terminalY = 30;
TaskHandle_t Core0Task;

// Radar geometry
const int RCX = 120, RCY = 232, RR = 56;

void parseEasyComm(String cmd);
void runSGP4();
void calculatePathPrediction();
void tftDrawStaticFrame();
void tftDrawAPMode();
void tftUpdateDynamic();
void tftRadarUpdate();
void setupWebServer();

// ================= HELPERS =================
void resetTftCache() {
  prev_cAz = prev_cEl = prev_tAz = prev_tEl = -999;
  prev_dist = prev_dop = -1e9;
  prev_rssi = 999; prev_heap = -1;
  prev_moving = prev_ntp = prev_l4s = prev_mode = prev_geo = -1;
  prev_sat[0] = '\0'; prev_clock[0] = '\0';
  prev_countdown = -999999;
  prev_maxElShown = -999;
  prev_antX = prev_satX = -1;
  prev_alX[0] = prev_alX[1] = -1;
}

void maidenheadToLatLon(String grid, double &lat, double &lon) {
  grid.toUpperCase();
  if (grid.length() < 4) return;
  lon = (grid[0]-'A')*20.0 - 180.0;
  lat = (grid[1]-'A')*10.0 - 90.0;
  lon += (grid[2]-'0')*2.0;
  lat += (grid[3]-'0')*1.0;
  if (grid.length() >= 6) {
    lon += ((tolower(grid[4])-'a')*5.0)/60.0;
    lat += ((tolower(grid[5])-'a')*2.5)/60.0;
  }
  lon += 1.0; lat += 0.5;
}

bool tleIsGeo() {
  if (strlen(tleLine2) < 63) return false;
  double mm = atof(tleLine2 + 52);
  return (mm > 0.1 && mm < 2.0);
}

String buildTelemetryJson() {
  StaticJsonDocument<512> doc;
  doc["tAz"] = (float)targetAz;   doc["tEl"] = (float)targetEl;
  doc["cAz"] = (float)currentAz;  doc["cEl"] = (float)currentEl;
  doc["isMoving"] = isMoving;
  doc["rssi"] = isAPMode ? 0 : WiFi.RSSI();
  doc["freeHeap"] = ESP.getFreeHeap() / 1024;
  doc["uptime"] = millis() / 1000;
  doc["mode"] = onboardMode ? 1 : 0;
  doc["sat"] = satName;
  doc["grid"] = maidenhead;
  doc["doppler"] = (float)dopplerFreq;
  doc["dist"] = (float)satDistance;
  doc["geo"] = isGeoSat;
  String out;
  serializeJson(doc, out);
  return out;
}

// Rainbow gradient strip (cyan -> violet -> magenta)
void drawGradientLine(int x, int y, int w, int h) {
  for (int i = 0; i < w; i++) {
    float f = (float)i / w;
    uint8_t r = (uint8_t)(f * 31);
    uint8_t g = (uint8_t)(63 - f * 63);
    tft.fillRect(x + i, y, 1, h, (r << 11) | (g << 5) | 31);
  }
}

// ================= BOOT SCREENS =================
void drawBootScreen() {
  tft.fillScreen(C_BLACK);
  tft.fillRect(0, 0, 240, 28, C_HDRBG);
  drawGradientLine(0, 28, 240, 2);
  tft.fillRect(0, 0, 4, 28, C_CYAN);
  tft.fillRect(236, 0, 4, 28, C_MAG);
  tft.setTextColor(C_CYAN); tft.setTextSize(1);
  tft.setCursor(8, 5);  tft.print("ESP32 SATELLITE TRACKER");
  tft.setTextColor(C_VIOLET);
  tft.setCursor(8, 16); tft.print("v8.5  //  SGP4 ENGINE");
  tft.setTextColor(C_MGRAY); tft.setCursor(8, 38); tft.print("DEVELOPER:");
  tft.setTextColor(C_WHITE); tft.setCursor(8, 50); tft.print("Muhammad Uzzam Butt");
  tft.drawFastHLine(4, 64, 232, C_DGRAY);
  tft.setTextColor(C_MGRAY); tft.setCursor(8, 70); tft.print("SCAN FOR SOURCE CODE:");

  const char* url = "https://github.com/uzzambutt/ESP32-Sattelite-Tracker";
  uint8_t *qrcode = (uint8_t *)malloc(qrcodegen_BUFFER_LEN_MAX);
  uint8_t *tempBuffer = (uint8_t *)malloc(qrcodegen_BUFFER_LEN_MAX);
  if (qrcode && tempBuffer) {
    if (qrcodegen_encodeText(url, tempBuffer, qrcode, qrcodegen_Ecc_LOW,
        qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true)) {
      int qs = qrcodegen_getSize(qrcode), ps = 3;
      int ox = (240 - qs*ps)/2, oy = 84;
      tft.fillRect(ox-4, oy-4, qs*ps+8, qs*ps+8, C_WHITE);
      for (int y = 0; y < qs; y++)
        for (int x = 0; x < qs; x++)
          if (qrcodegen_getModule(qrcode, x, y))
            tft.fillRect(ox + x*ps, oy + y*ps, ps, ps, C_BLACK);
    }
  }
  if (qrcode) free(qrcode);
  if (tempBuffer) free(tempBuffer);

  tft.fillRect(0, 300, 240, 20, C_HDRBG);
  drawGradientLine(0, 298, 240, 2);
  tft.setTextColor(C_CYAN); tft.setCursor(8, 307);
  tft.print("INITIALIZING SYSTEMS...");
}

void printBootTerminal(String msg) {
  Serial.println("[BOOT] " + msg);
  if (terminalY > 300) {
    tft.fillScreen(C_BLACK);
    tft.fillRect(0, 0, 240, 18, C_HDRBG);
    drawGradientLine(0, 18, 240, 2);
    tft.setTextColor(C_CYAN); tft.setTextSize(1);
    tft.setCursor(8, 5); tft.print("SYSTEM BOOT SEQUENCE...");
    terminalY = 28;
  }
  tft.setTextSize(1);
  tft.setCursor(5, terminalY);
  tft.setTextColor(C_LIME); tft.print("> ");
  tft.setTextColor(C_WHITE); tft.print(msg);
  terminalY += 12;
  delay(80);
}

void tftDrawAPMode() {
  tft.fillScreen(C_BLACK);
  tft.fillRect(0, 0, 240, 24, C_DRED);
  tft.fillRect(0, 0, 4, 24, C_RED);
  tft.fillRect(236, 0, 4, 24, C_RED);
  tft.setTextColor(C_WHITE); tft.setTextSize(1);
  tft.setCursor(8, 5);  tft.print("! NETWORK LINK FAILED !");
  tft.setTextColor(C_AMBER);
  tft.setCursor(8, 15); tft.print("ENTERING SETUP MODE");
  tft.drawFastHLine(0, 24, 240, C_RED);
  tft.setTextColor(C_RED); tft.setTextSize(2);
  tft.setCursor(10, 32); tft.print("WIFI FAIL");

  tft.drawRoundRect(2, 58, 236, 128, 4, C_AMBER);
  tft.fillRect(3, 59, 234, 12, C_AMBBG);
  tft.setTextColor(C_AMBER); tft.setTextSize(1);
  tft.setCursor(6, 61); tft.print("[ SETUP INSTRUCTIONS ]");
  tft.drawFastHLine(3, 71, 234, C_DGRAY);
  tft.setTextColor(C_AMBER); tft.setCursor(6, 76);  tft.print("1.");
  tft.setTextColor(C_MGRAY); tft.setCursor(20, 76); tft.print("Join this WiFi network:");
  tft.setTextColor(C_CYAN);  tft.setCursor(20, 88); tft.print("AEROSPACE-TRACKER");
  tft.setTextColor(C_AMBER); tft.setCursor(6, 104);  tft.print("2.");
  tft.setTextColor(C_MGRAY); tft.setCursor(20, 104); tft.print("WiFi Password:");
  tft.setTextColor(C_WHITE); tft.setCursor(20, 116); tft.print("groundstation");
  tft.setTextColor(C_AMBER); tft.setCursor(6, 132);  tft.print("3.");
  tft.setTextColor(C_MGRAY); tft.setCursor(20, 132); tft.print("Open browser, go to:");
  tft.setTextColor(C_LIME);  tft.setCursor(20, 144); tft.print("192.168.4.1");
  tft.setTextColor(C_AMBER); tft.setCursor(6, 160);  tft.print("4.");
  tft.setTextColor(C_MGRAY); tft.setCursor(20, 160); tft.print("Enter new credentials.");

  tft.drawRoundRect(2, 192, 236, 24, 4, C_DGRAY);
  tft.setTextColor(C_YELLOW);
  tft.setCursor(6, 200); tft.print("WAITING FOR CREDENTIALS...");
  tft.fillRect(0, 300, 240, 20, C_DRED);
  tft.setTextColor(C_WHITE);
  tft.setCursor(8, 307); tft.print("AP MODE  |  192.168.4.1");
}

// ================= STATIC HUD FRAME =================
// Layout: Header 0-17 | TARGET 20-74 | RF LINK 77-117 | PASS 120-160 | RADAR 163-301 | BAR 304-320
void tftDrawStaticFrame() {
  tft.fillScreen(C_BLACK);
  resetTftCache();

  // ── Header with gradient underline & clock zone ──
  tft.fillRect(0, 0, 240, 16, C_HDRBG);
  drawGradientLine(0, 16, 240, 2);
  tft.fillRect(0, 0, 3, 16, C_CYAN);
  tft.setTextColor(C_WHITE); tft.setTextSize(1);
  tft.setCursor(7, 4); tft.print("AEROSPACE CMD");
  tft.setTextColor(C_VIOLET);
  tft.setCursor(96, 4); tft.print("UTC");
  // clock drawn dynamically at (122,4); NTP dot at (228,7)

  // ── TARGET (cyan) ──
  tft.drawRoundRect(2, 20, 236, 54, 4, C_CYAN);
  tft.fillRect(3, 21, 234, 11, C_TEALBG);
  tft.setTextColor(C_CYAN);
  tft.setCursor(6, 23); tft.print("TARGET");
  tft.drawFastHLine(3, 32, 234, C_FAINT);
  tft.drawFastVLine(118, 33, 40, C_FAINT);
  tft.setTextColor(C_MGRAY);
  tft.setCursor(6, 40);   tft.print("AZ");
  tft.setCursor(124, 40); tft.print("EL");

  // ── RF LINK (magenta) ──
  tft.drawRoundRect(2, 77, 236, 40, 4, C_MAG);
  tft.fillRect(3, 78, 234, 11, C_PURPBG);
  tft.setTextColor(C_PINK);
  tft.setCursor(6, 80); tft.print("RF LINK");
  tft.setTextColor(C_VIOLET);
  tft.setCursor(170, 80); tft.print(maidenhead);
  tft.drawFastHLine(3, 89, 234, C_FAINT);
  tft.drawFastVLine(118, 90, 26, C_FAINT);
  tft.setTextColor(C_MGRAY);
  tft.setCursor(6, 93);    tft.print("DST:");
  tft.setCursor(124, 93);  tft.print("DOP:");
  tft.setCursor(6, 105);   tft.print("SIG:");
  tft.setCursor(124, 105); tft.print("L4S:");

  // ── PASS PREDICTION (amber) ──
  tft.drawRoundRect(2, 120, 236, 40, 4, C_AMBER);
  tft.fillRect(3, 121, 234, 11, C_AMBBG);
  tft.setTextColor(C_AMBER);
  tft.setCursor(6, 123); tft.print("PASS PREDICTION");
  tft.drawFastHLine(3, 132, 234, C_FAINT);
  tft.drawFastVLine(118, 133, 26, C_FAINT);
  tft.setTextColor(C_MGRAY);
  tft.setCursor(6, 136);   tft.print("AOS:");
  tft.setCursor(124, 136); tft.print("MAX:");
  tft.setCursor(6, 148);   tft.print("LOS:");
  tft.setCursor(124, 148); tft.print("T- :");

  // ── Bottom status bar ──
  tft.fillRect(0, 304, 240, 16, C_HDRBG);
  drawGradientLine(0, 302, 240, 2);
  tft.setTextColor(C_CYAN); tft.setCursor(6, 308);
  tft.print(isAPMode ? "192.168.4.1" : WiFi.localIP().toString());
  // dynamic: ENG@(110), SLEW@(154), MEM@(198)

  newPathReady = true; // force full radar draw
}

void drawRadarBackground() {
  tft.drawCircle(RCX, RCY, RR,     0x0339);  // deep teal rings
  tft.drawCircle(RCX, RCY, RR*2/3, 0x0339);
  tft.drawCircle(RCX, RCY, RR/3,   0x0339);
  tft.drawFastVLine(RCX, RCY - RR, RR*2 + 1, 0x0339);
  tft.drawFastHLine(RCX - RR, RCY, RR*2 + 1, 0x0339);
  for (int i = 0; i < 360; i += 45) {
    float rad = i * PI / 180.0f;
    tft.drawLine(RCX + (int)((RR-4)*cosf(rad)), RCY + (int)((RR-4)*sinf(rad)),
                 RCX + (int)(RR*cosf(rad)),     RCY + (int)(RR*sinf(rad)),
                 (i % 90 == 0) ? C_BLUE : C_DGRAY);
  }
  tft.setTextSize(1);
  tft.setTextColor(C_CYAN);
  tft.setCursor(RCX-2,    RCY-RR-10); tft.print("N");
  tft.setCursor(RCX-2,    RCY+RR+4);  tft.print("S");
  tft.setCursor(RCX+RR+5, RCY-3);     tft.print("E");
  tft.setCursor(RCX-RR-11,RCY-3);     tft.print("W");
  tft.setTextColor(C_DGRAY);
  tft.setCursor(RCX+3, RCY-RR/3-4);   tft.print("60");
  tft.setCursor(RCX+3, RCY-RR*2/3-4); tft.print("30");
}

// Signal strength bars (4 bars) at given origin
void drawSigBars(int x, int y, int rssi) {
  int lvl = (rssi > -55) ? 4 : (rssi > -65) ? 3 : (rssi > -75) ? 2 : (rssi > -85) ? 1 : 0;
  for (int i = 0; i < 4; i++) {
    int h = 3 + i * 2;
    uint16_t c = (i < lvl) ? ((lvl >= 3) ? C_LIME : (lvl == 2) ? C_AMBER : C_RED) : C_DGRAY;
    tft.fillRect(x + i * 5, y + (9 - h), 3, h, c);
  }
}

// ================= DYNAMIC HUD =================
void tftUpdateDynamic() {
  if (spiLock || isAPMode) return;
  char buf[32];

  // ── UTC clock in header ──
  time_t nowT = (time_t)timeClient.getEpochTime();
  struct tm tmN; gmtime_r(&nowT, &tmN);
  char ck[10];
  snprintf(ck, sizeof(ck), "%02d:%02d:%02d", tmN.tm_hour, tmN.tm_min, tmN.tm_sec);
  if (strcmp(ck, prev_clock) != 0) {
    tft.fillRect(122, 4, 50, 9, C_HDRBG);
    tft.setTextColor(C_WHITE); tft.setTextSize(1);
    tft.setCursor(122, 4); tft.print(ck);
    strcpy(prev_clock, ck);
  }
  // NTP dot in header
  if ((int)ntpSynced != prev_ntp) {
    tft.fillCircle(228, 8, 3, ntpSynced ? C_LIME : C_RED);
    prev_ntp = ntpSynced;
  }

  // ── Sat name in TARGET title bar ──
  if (strcmp(satName, prev_sat) != 0) {
    tft.fillRect(52, 21, 184, 11, C_TEALBG);
    tft.setTextColor(C_YELLOW); tft.setTextSize(1);
    tft.setCursor(54, 23);
    snprintf(buf, sizeof(buf), "%.22s%s", satName, isGeoSat ? " GEO" : "");
    tft.print(buf);
    strncpy(prev_sat, satName, 24);
  }

  float cAz = currentAz, tAz = targetAz, cEl = currentEl, tEl = targetEl;

  // ── AZ column ──
  if (fabsf(cAz - prev_cAz) > 0.05f || fabsf(tAz - prev_tAz) > 0.05f) {
    tft.fillRect(22, 36, 94, 16, C_BLACK);
    tft.setTextColor(C_CYAN); tft.setTextSize(2);
    tft.setCursor(22, 36);
    snprintf(buf, sizeof(buf), "%05.1f", (double)cAz);
    tft.print(buf);
    float dAz = tAz - cAz;
    if (dAz >  180) dAz -= 360;
    if (dAz < -180) dAz += 360;
    tft.fillRect(6, 58, 110, 9, C_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(C_LIME);
    tft.setCursor(6, 58);
    snprintf(buf, sizeof(buf), ">%05.1f", (double)tAz);
    tft.print(buf);
    tft.setTextColor(fabsf(dAz) > 1.0f ? C_AMBER : C_MGRAY);
    tft.setCursor(60, 58);
    snprintf(buf, sizeof(buf), "%+06.1f", (double)dAz);
    tft.print(buf);
    prev_cAz = cAz; prev_tAz = tAz;
  }

  // ── EL column ──
  if (fabsf(cEl - prev_cEl) > 0.05f || fabsf(tEl - prev_tEl) > 0.05f) {
    tft.fillRect(140, 36, 96, 16, C_BLACK);
    tft.setTextColor(C_PINK); tft.setTextSize(2);
    tft.setCursor(140, 36);
    snprintf(buf, sizeof(buf), "%04.1f", (double)cEl);
    tft.print(buf);
    float dEl = tEl - cEl;
    tft.fillRect(124, 58, 110, 9, C_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(C_LIME);
    tft.setCursor(124, 58);
    snprintf(buf, sizeof(buf), ">%04.1f", (double)tEl);
    tft.print(buf);
    tft.setTextColor(fabsf(dEl) > 1.0f ? C_AMBER : C_MGRAY);
    tft.setCursor(172, 58);
    snprintf(buf, sizeof(buf), "%+05.1f", (double)dEl);
    tft.print(buf);
    prev_cEl = cEl; prev_tEl = tEl;
  }

  tft.setTextSize(1);

  // ── RF: Distance ──
  if (fabsf(satDistance - prev_dist) > 0.5f) {
    tft.fillRect(32, 93, 84, 9, C_BLACK);
    tft.setTextColor(C_WHITE);
    tft.setCursor(32, 93);
    if (satDistance > 0) snprintf(buf, sizeof(buf), "%.0f km", (double)satDistance);
    else strcpy(buf, "---");
    tft.print(buf);
    prev_dist = satDistance;
  }
  // ── RF: Doppler ──
  if (fabsf(dopplerFreq - prev_dop) > 1.0f) {
    tft.fillRect(150, 93, 84, 9, C_BLACK);
    tft.setTextColor(C_VIOLET);
    tft.setCursor(150, 93);
    snprintf(buf, sizeof(buf), "%+05.0fHz", (double)dopplerFreq);
    tft.print(buf);
    prev_dop = dopplerFreq;
  }
  // ── RF: WiFi signal bars + dBm ──
  int rssi = isAPMode ? -100 : WiFi.RSSI();
  if (abs(rssi - prev_rssi) > 2) {
    tft.fillRect(32, 102, 84, 12, C_BLACK);
    drawSigBars(32, 103, rssi);
    tft.setTextColor(C_MGRAY);
    tft.setCursor(56, 105);
    snprintf(buf, sizeof(buf), "%ddBm", rssi);
    tft.print(buf);
    prev_rssi = rssi;
  }
  // ── RF: L4S link ──
  bool l4sNow = (tcpClient && tcpClient.connected());
  if ((int)l4sNow != prev_l4s) {
    tft.fillRect(150, 105, 84, 9, C_BLACK);
    tft.setTextColor(l4sNow ? C_LIME : C_MGRAY);
    tft.setCursor(150, 105);
    tft.print(l4sNow ? "LINKED" : "WAITING");
    prev_l4s = l4sNow;
  }

  // ── Bottom bar: ENG / motion / heap ──
  int modeVal = onboardMode ? 1 : 0;
  if (modeVal != prev_mode) {
    tft.fillRect(106, 308, 46, 9, C_HDRBG);
    tft.setTextColor(onboardMode ? C_CYAN : C_AMBER);
    tft.setCursor(106, 308);
    tft.print(onboardMode ? "SGP4" : "L4S");
    prev_mode = modeVal;
  }
  if ((int)isMoving != prev_moving) {
    tft.fillRect(150, 308, 44, 9, C_HDRBG);
    tft.setTextColor(isMoving ? C_AMBER : C_LIME);
    tft.setCursor(150, 308);
    tft.print(isMoving ? "SLEW" : "IDLE");
    prev_moving = isMoving;
  }
  int heapK = ESP.getFreeHeap() / 1024;
  if (abs(heapK - prev_heap) > 2) {
    tft.fillRect(194, 308, 44, 9, C_HDRBG);
    tft.setTextColor(C_MGRAY);
    tft.setCursor(194, 308);
    snprintf(buf, sizeof(buf), "%dKB", heapK);
    tft.print(buf);
    prev_heap = heapK;
  }

  // ── Pass prediction ──
  unsigned long nowEpoch = timeClient.getEpochTime();
  int geoState = isGeoSat ? 1 : 0;
  long countdown = (nextAosTime > 0) ? (long)(nextAosTime - (time_t)nowEpoch) : -999999;
  bool redraw = false;
  if (geoState != prev_geo) redraw = true;
  else if (geoState && fabsf(nextMaxEl - prev_maxElShown) > 0.05f) redraw = true;
  else if (!geoState && countdown != prev_countdown) redraw = true;

  if (redraw) {
    tft.fillRect(32, 136, 84, 9, C_BLACK);
    tft.fillRect(32, 148, 84, 9, C_BLACK);
    tft.fillRect(150, 136, 84, 9, C_BLACK);
    tft.fillRect(150, 148, 84, 9, C_BLACK);

    if (geoState) {
      tft.setTextColor(C_CYAN);
      tft.setCursor(32, 136); tft.print("GEO ORBIT");
      tft.setCursor(32, 148); tft.print("STATIC");
      tft.setTextColor(C_YELLOW);
      tft.setCursor(150, 136);
      snprintf(buf, sizeof(buf), "%.1f deg", (double)nextMaxEl);
      tft.print(buf);
      tft.setCursor(150, 148);
      if (nextMaxEl > 0) { tft.setTextColor(C_LIME); tft.print("LOCKED"); }
      else               { tft.setTextColor(C_RED);  tft.print("NO VIS"); }
    } else if (nextAosTime > 0) {
      time_t aosT = nextAosTime, losT = nextLosTime;
      struct tm tmAos, tmLos;
      gmtime_r(&aosT, &tmAos);
      gmtime_r(&losT, &tmLos);
      tft.setTextColor(C_WHITE);
      tft.setCursor(32, 136);
      snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tmAos.tm_hour, tmAos.tm_min, tmAos.tm_sec);
      tft.print(buf);
      tft.setCursor(32, 148);
      snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tmLos.tm_hour, tmLos.tm_min, tmLos.tm_sec);
      tft.print(buf);
      tft.setTextColor(C_YELLOW);
      tft.setCursor(150, 136);
      snprintf(buf, sizeof(buf), "%.1f deg", (double)nextMaxEl);
      tft.print(buf);
      tft.setCursor(150, 148);
      if (countdown <= 0 && (time_t)nowEpoch < losT) {
        tft.setTextColor(C_LIME); tft.print("TRACKING");
      } else if (countdown > 0) {
        tft.setTextColor(C_AMBER);
        snprintf(buf, sizeof(buf), "-%02ld:%02ld:%02ld",
                 countdown/3600, (countdown%3600)/60, countdown%60);
        tft.print(buf);
      } else {
        tft.setTextColor(C_MGRAY); tft.print("ACQUIRING");
      }
    } else {
      tft.setTextColor(C_RED);
      tft.setCursor(32, 136); tft.print(tleLoaded ? "NO PASS" : "NO TLE");
      tft.setCursor(32, 148); tft.print("--:--:--");
    }
    prev_geo = geoState;
    prev_countdown = countdown;
    prev_maxElShown = nextMaxEl;
  }
}

// ================= RADAR ENGINE =================
void tftRadarUpdate() {
  if (spiLock || isAPMode) return;

  auto toXY = [](float az, float el, int &x, int &y) {
    float rr  = RR * (1.0f - el / 90.0f);
    float rad = (az - 90.0f) * PI / 180.0f;
    x = RCX + (int)(rr * cosf(rad));
    y = RCY + (int)(rr * sinf(rad));
  };

  if (newPathReady) {
    // FULL bounding-box wipe — also clears stale A/L labels
    tft.fillRect(RCX - RR - 14, RCY - RR - 14, (RR + 14) * 2, (RR + 14) * 2, C_BLACK);
    drawRadarBackground();
    prev_antX = prev_satX = -1;
    prev_alX[0] = prev_alX[1] = -1;
    newPathReady = false;
  } else {
    if (prev_antX >= 0) {
      tft.drawLine(RCX, RCY, prev_antX, prev_antY, C_BLACK);
      tft.fillCircle(prev_antX, prev_antY, 3, C_BLACK);
    }
    if (prev_satX >= 0) {
      tft.fillCircle(prev_satX, prev_satY, 4, C_BLACK);
      tft.drawCircle(prev_satX, prev_satY, 7, C_BLACK);
    }
    for (int k = 0; k < 2; k++)
      if (prev_alX[k] >= 0)
        tft.fillRect(prev_alX[k], prev_alY[k], 7, 9, C_BLACK);
    tft.drawCircle(RCX, RCY, RR,     0x0339);
    tft.drawCircle(RCX, RCY, RR*2/3, 0x0339);
    tft.drawCircle(RCX, RCY, RR/3,   0x0339);
    tft.drawFastVLine(RCX, RCY - RR + 1, RR*2 - 1, 0x0339);
    tft.drawFastHLine(RCX - RR + 1, RCY, RR*2 - 1, 0x0339);
  }

  // Pass trajectory (skipped for GEO)
  int len = isGeoSat ? 0 : globalPathLen;
  prev_alX[0] = prev_alX[1] = -1;
  if (len > 1) {
    int lastX = -1, lastY = -1;
    tft.setTextSize(1);
    for (int i = 0; i < len; i++) {
      int px, py;
      toXY(globalPath[i].az, globalPath[i].el, px, py);
      if (lastX != -1) tft.drawLine(lastX, lastY, px, py, C_LIME);
      if (i == 0) {
        tft.setTextColor(C_LIME);
        tft.setCursor(px - 3, py - 10); tft.print("A");
        prev_alX[0] = px - 3; prev_alY[0] = py - 10;
      } else if (i == len - 1) {
        tft.setTextColor(C_RED);
        tft.setCursor(px - 3, py - 10); tft.print("L");
        prev_alX[1] = px - 3; prev_alY[1] = py - 10;
      }
      lastX = px; lastY = py;
    }
  }

  // Antenna needle
  int antX, antY;
  toXY((float)currentAz, (float)currentEl, antX, antY);
  tft.drawLine(RCX, RCY, antX, antY, C_BLUE);
  tft.fillCircle(antX, antY, 3, C_CYAN);
  prev_antX = antX; prev_antY = antY;

  // Satellite dot (magenta, amber if GEO)
  if (targetEl > 0.0f) {
    int sX, sY;
    toXY((float)targetAz, (float)targetEl, sX, sY);
    uint16_t c = isGeoSat ? C_AMBER : C_MAG;
    tft.fillCircle(sX, sY, 4, c);
    tft.drawCircle(sX, sY, 7, c);
    prev_satX = sX; prev_satY = sY;
  } else {
    prev_satX = -1;
  }
}

// ================= PASS LOG =================
void logPass() {
  if (passLogCount >= 10) {
    for (int i = 0; i < 9; i++) passLog[i] = passLog[i + 1];
    passLogCount = 9;
  }
  strncpy(passLog[passLogCount].sat,  satName,      24);
  strncpy(passLog[passLogCount].time, passStartBuf, 19);
  passLog[passLogCount].maxEl = passMaxEl;
  passLogCount++;
  Serial.printf("[SGP4] Pass logged: %s | MaxEl %.1f\n", satName, passMaxEl);
}

// ================= WEB SERVER =================
void setupWebServer() {
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", isAPMode ? wifi_html : index_html);
  });

  webServer.on("/api/wifi", HTTP_POST,
    [](AsyncWebServerRequest *request) { request->send(200, "application/json", "{\"status\":\"ok\"}"); },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      String body((char*)data, len);
      StaticJsonDocument<256> doc;
      if (!deserializeJson(doc, body)) {
        String newSSID = doc["ssid"] | "";
        String newPass = doc["pass"] | "";
        if (newSSID.length() > 0) {
          prefs.begin("sattracker", false);
          prefs.putString("wifi_ssid", newSSID);
          prefs.putString("wifi_pass", newPass);
          prefs.end();
          triggerReboot = true;
        }
      }
    });

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
      json += "{\"sat\":\"" + String(passLog[i].sat) +
              "\",\"time\":\"" + String(passLog[i].time) +
              "\",\"maxEl\":" + String(passLog[i].maxEl, 1) + "}";
    }
    json += "]";
    request->send(200, "application/json", json);
  });

  webServer.on("/api/config", HTTP_POST,
    [](AsyncWebServerRequest *request) { request->send(200, "application/json", "{\"status\":\"ok\"}"); },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      String body((char*)data, len);
      StaticJsonDocument<256> doc;
      if (!deserializeJson(doc, body)) {
        if (doc.containsKey("grid")) {
          maidenhead = doc["grid"].as<String>();
          maidenheadToLatLon(maidenhead, obsLat, obsLon);
          prefs.begin("sattracker", false);
          prefs.putString("grid", maidenhead);
          prefs.end();
          lastPathCalc = 0;
        }
        if (doc.containsKey("obMode")) {
          onboardMode = doc["obMode"].as<bool>();
          prefs.begin("sattracker", false);
          prefs.putBool("obMode", onboardMode);
          prefs.end();
          lastPathCalc = 0;
        }
      }
    });

  webServer.on("/api/tle", HTTP_POST,
    [](AsyncWebServerRequest *request) { request->send(200, "application/json", "{\"status\":\"ok\"}"); },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      String body((char*)data, len);
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
          isGeoSat = tleIsGeo();
          tleLoaded = true;
          lastPathCalc = 0;
          newPathReady = true;
          Serial.printf("[SGP4] TLE loaded: %s %s\n", satName, isGeoSat ? "(GEO)" : "");
        }
      }
    });

  webServer.on("/api/manual", HTTP_POST,
    [](AsyncWebServerRequest *request) { request->send(200, "application/json", "{\"status\":\"ok\"}"); },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      String body((char*)data, len);
      StaticJsonDocument<128> doc;
      if (!deserializeJson(doc, body)) {
        if (!onboardMode && !isAPMode) {
          if (doc.containsKey("az")) targetAz = doc["az"].as<float>();
          if (doc.containsKey("el")) targetEl = doc["el"].as<float>();
        }
      }
    });

  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client,
                AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) client->text(buildTelemetryJson());
  });
  webServer.addHandler(&ws);
  webServer.begin();
}

// ================= PASS PREDICTION =================
void calculatePathPrediction() {
  if (!tleLoaded || !ntpSynced || !onboardMode || isAPMode) {
    globalPathLen = 0; newPathReady = true; return;
  }

  Sgp4 p;
  p.site(obsLat, obsLon, obsAlt);
  p.init(satName, tleLine1, tleLine2);

  unsigned long nowT = timeClient.getEpochTime();
  isGeoSat = tleIsGeo();

  // GEO: live position only — correct elevation, no countdown
  if (isGeoSat) {
    p.findsat(nowT);
    nextMaxEl   = p.satEl;
    nextAosTime = (p.satEl > 0) ? (time_t)nowT : 0;
    nextLosTime = 0;
    globalPathLen = 0;
    newPathReady  = true;
    Serial.printf("[SGP4] GEO target. Live El: %.1f\n", (float)nextMaxEl);
    return;
  }

  // LEO/MEO: find AOS (60s scan, 10s refine — catches short passes)
  unsigned long t = nowT;
  p.findsat(t);
  bool found = false;

  if (p.satEl > 0) {
    int lim = 0;
    while (p.satEl > 0 && lim++ < 80) {
      t -= 30; p.findsat(t);
      if (lim % 10 == 0) vTaskDelay(pdMS_TO_TICKS(2));
    }
    t += 30;
    found = true;
  } else {
    for (int i = 0; i < 1440 && !found; i++) {
      t += 60; p.findsat(t);
      if (p.satEl > 0) {
        int lim = 0;
        while (p.satEl > 0 && lim++ < 12) { t -= 10; p.findsat(t); }
        t += 10;
        found = true;
      }
      if (i % 30 == 0) vTaskDelay(pdMS_TO_TICKS(2));
    }
  }

  if (!found) {
    nextAosTime = nextLosTime = 0;
    nextMaxEl = 0;
    globalPathLen = 0;
    newPathReady = true;
    return;
  }

  nextAosTime = (time_t)t;

  // Sample pass with independent counter (LOS != AOS guaranteed)
  float maxEl = 0;
  int len = 0;
  unsigned long tt = t;
  for (int i = 0; i < 80; i++) {
    p.findsat(tt);
    if (p.satEl < 0 && i > 0) break;
    if (p.satEl >= 0) {
      if (p.satEl > maxEl) maxEl = p.satEl;
      if (len < 45) {
        globalPath[len].az = p.satAz;
        globalPath[len].el = p.satEl;
        len++;
      }
    }
    tt += 30;
    if (i % 10 == 0) vTaskDelay(pdMS_TO_TICKS(2));
  }
  nextLosTime   = (time_t)tt;
  nextMaxEl     = maxEl;
  globalPathLen = len;
  newPathReady  = true;
  Serial.printf("[SGP4] Pass: AOS+%lds LOS+%lds MaxEl %.1f (%d pts)\n",
                (long)(nextAosTime - nowT), (long)(nextLosTime - nowT), maxEl, len);
}

// ================= CORE 0 TASK =================
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
        if (!onboardMode && !isAPMode) parseEasyComm(cmd);
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

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n====================================");
  Serial.println(" ESP32 Aerospace Tracker v8.5 (ST7789)");
  Serial.println("====================================");

  esp_task_wdt_init(30, true);
  esp_task_wdt_add(NULL);

  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, HIGH);
  azStepper.setMaxSpeed(2000); azStepper.setAcceleration(1000);
  elStepper.setMaxSpeed(2000); elStepper.setAcceleration(1000);

  tft.init(240, 320);
  tft.setRotation(0);
  tft.invertDisplay(false);
  drawBootScreen();
  delay(3000);

  tft.fillScreen(C_BLACK);
  tft.fillRect(0, 0, 240, 18, C_HDRBG);
  drawGradientLine(0, 18, 240, 2);
  tft.setTextColor(C_CYAN); tft.setTextSize(1);
  tft.setCursor(8, 5); tft.print("SYSTEM BOOT SEQUENCE...");
  terminalY = 28;

  printBootTerminal("Initializing LittleFS...");
  if (!LittleFS.begin(false)) LittleFS.begin(true);

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
      String n  = f.readStringUntil('\n'); n.trim();  n.toCharArray(satName, 25);
      String l1 = f.readStringUntil('\n'); l1.trim(); l1.toCharArray(tleLine1, 70);
      String l2 = f.readStringUntil('\n'); l2.trim(); l2.toCharArray(tleLine2, 70);
      f.close();
      if (strlen(tleLine1) > 0 && strlen(tleLine2) > 0) {
        sat.init(satName, tleLine1, tleLine2);
        isGeoSat = tleIsGeo();
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
    delay(500);
    tftDrawAPMode();
    return;
  }

  printBootTerminal("IP: " + WiFi.localIP().toString());
  if (MDNS.begin("sattracker")) {
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("easycomm", "tcp", 4533);
    printBootTerminal("mDNS: sattracker.local");
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
    lastPathCalc = millis();
  }

  printBootTerminal("Starting Tactical HUD...");
  delay(500);
  tftDrawStaticFrame();
  digitalWrite(ENABLE_PIN, LOW);

  xTaskCreatePinnedToCore(Core0TaskCode, "Core0Task", 16384, NULL, 1, &Core0Task, 0);
}

// ================= LOOP (CORE 1) =================
void loop() {
  esp_task_wdt_reset();
  if (triggerReboot) { delay(1000); ESP.restart(); }
  if (isAPMode) return;

  ArduinoOTA.handle();

  targetEl = constrain((float)targetEl, 0.0f, 90.0f);
  targetAz = fmodf((float)targetAz + 360.0f, 360.0f);

  azStepper.moveTo((long)((float)targetAz * STEPS_PER_DEG));
  elStepper.moveTo((long)((float)targetEl * STEPS_PER_DEG));
  azStepper.run();
  elStepper.run();

  currentAz = azStepper.currentPosition() / STEPS_PER_DEG;
  currentEl = elStepper.currentPosition() / STEPS_PER_DEG;
  isMoving  = azStepper.isRunning() || elStepper.isRunning();

  static unsigned long lastLog = 0;
  if (onboardMode) {
    if (tleLoaded && ntpSynced && millis() - lastSGP4Update > 1000) {
      runSGP4();
      lastSGP4Update = millis();
    }
    if (millis() - lastLog > 5000) {
      if (!tleLoaded)      Serial.println("[SGP4] Waiting on TLE upload...");
      else if (!ntpSynced) Serial.println("[NTP]  Syncing reference time...");
      else Serial.printf("[SGP4] AZ %.1f | EL %.1f | %.0fkm %s\n",
                         (float)targetAz, (float)targetEl, (float)satDistance,
                         isGeoSat ? "[GEO]" : "");
      lastLog = millis();
    }
  } else {
    if (millis() - lastLog > 10000) {
      Serial.println("[NET] Port 4533 listening for Look4Sat...");
      lastLog = millis();
    }
  }
}

// ================= EASYCOMM PARSER =================
void parseEasyComm(String cmd) {
  cmd.trim();
  if (cmd.length() < 1) return;
  if (cmd.startsWith("P ") || cmd.startsWith("p ")) {
    int fs = cmd.indexOf(' ');
    int ss = cmd.indexOf(' ', fs + 1);
    if (fs != -1 && ss != -1) {
      targetAz = cmd.substring(fs + 1, ss).toFloat();
      targetEl = cmd.substring(ss + 1).toFloat();
    }
  } else if (cmd.indexOf("AZ") != -1 || cmd.indexOf("az") != -1) {
    int ai = cmd.indexOf("AZ"); if (ai == -1) ai = cmd.indexOf("az");
    int ei = cmd.indexOf("EL"); if (ei == -1) ei = cmd.indexOf("el");
    if (ai != -1 && ei != -1) {
      targetAz = cmd.substring(ai + 2, ei).toFloat();
      int ee = cmd.indexOf(' ', ei); if (ee == -1) ee = cmd.length();
      targetEl = cmd.substring(ei + 2, ee).toFloat();
    }
  } else if (cmd == "p" || cmd == "P") {
    if (tcpClient) tcpClient.printf("AZ%.2f EL%.2f\n", (float)currentAz, (float)currentEl);
  }
}

// ================= SGP4 RUNTIME =================
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
  lastSatDist = sat.satDist;
  lastDopplerTime = curMs;
  satDistance = (float)sat.satDist;

  targetAz = sat.satAz;
  targetEl = sat.satEl;

  if (isGeoSat) nextMaxEl = sat.satEl;  // live GEO elevation

  if (sat.satEl >= 0.0) {
    if (!passInProgress) {
      passInProgress = true;
      passMaxEl = sat.satEl;
      time_t t = (time_t)now;
      struct tm tmNow;
      gmtime_r(&t, &tmNow);
      snprintf(passStartBuf, sizeof(passStartBuf), "%02d:%02d:%02d",
               tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec);
    } else if ((float)sat.satEl > passMaxEl) {
      passMaxEl = sat.satEl;
    }
  } else if (passInProgress) {
    passInProgress = false;
    if (!isGeoSat) logPass();
  }
}