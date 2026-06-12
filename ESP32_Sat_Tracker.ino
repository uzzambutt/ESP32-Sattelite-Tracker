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

// ╔══════════════════════════════════════════════════════════╗
// ║  MPU6050 GYRO/ACCEL (GY-521) — UNCOMMENT TO ENABLE       ║
// ║  Wiring: VCC->3.3V  GND->GND  SDA->GPIO21  SCL->GPIO22   ║
// ╚══════════════════════════════════════════════════════════╝
// #include <Wire.h>
// #define MPU_ADDR      0x68     // AD0 low = 0x68, AD0 high = 0x69
// #define MPU_SDA       21
// #define MPU_SCL       22
// volatile float mpuPitch = 0;   // physical elevation of antenna (deg)
// volatile float mpuRoll  = 0;
// volatile bool  mpuOK    = false;
// unsigned long  lastMpuRead = 0;
// const float MPU_CORRECT_THRESH = 2.0;  // deg error before auto-correct
//
// bool mpuInit() {
//   Wire.begin(MPU_SDA, MPU_SCL);
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(0x6B); Wire.write(0x00);          // wake up
//   if (Wire.endTransmission(true) != 0) return false;
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(0x1C); Wire.write(0x00);          // accel ±2g
//   Wire.endTransmission(true);
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(0x1B); Wire.write(0x00);          // gyro ±250°/s
//   Wire.endTransmission(true);
//   return true;
// }
//
// void mpuRead() {
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(0x3B);                            // ACCEL_XOUT_H
//   if (Wire.endTransmission(false) != 0) { mpuOK = false; return; }
//   Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)14, (uint8_t)true);
//   if (Wire.available() < 14) { mpuOK = false; return; }
//   int16_t ax = (Wire.read() << 8) | Wire.read();
//   int16_t ay = (Wire.read() << 8) | Wire.read();
//   int16_t az = (Wire.read() << 8) | Wire.read();
//   Wire.read(); Wire.read();                    // temp (skip)
//   int16_t gx = (Wire.read() << 8) | Wire.read();
//   int16_t gy = (Wire.read() << 8) | Wire.read();
//   int16_t gz = (Wire.read() << 8) | Wire.read();
//   (void)gx; (void)gy; (void)gz;
//   // Accel-derived tilt angles
//   float accPitch = atan2f((float)ax, sqrtf((float)ay*ay + (float)az*az)) * 180.0f / PI;
//   float accRoll  = atan2f((float)ay, (float)az) * 180.0f / PI;
//   // Complementary smoothing (accel-only here; gyro fusion optional)
//   mpuPitch = 0.9f * mpuPitch + 0.1f * accPitch;
//   mpuRoll  = 0.9f * mpuRoll  + 0.1f * accRoll;
//   mpuOK = true;
// }
//
// // Wind/knock auto-correction: if the physical elevation measured by
// // the IMU disagrees with where the steppers THINK they are, re-zero
// // the elevation stepper to the IMU truth.
// void mpuStabilize() {
//   if (!mpuOK || isMoving) return;              // only correct when idle
//   float physEl = mpuPitch;                     // mount so pitch = elevation
//   float err = physEl - currentEl;
//   if (fabsf(err) > MPU_CORRECT_THRESH) {
//     Serial.printf("[IMU] Drift %.1f deg detected! Re-zeroing EL axis.\n", err);
//     elStepper.setCurrentPosition((long)(physEl * STEPS_PER_DEG));
//   }
// }

// ================= WEB DASHBOARD =================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>ORBITAL OPS</title><style>
@import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Rajdhani:wght@500;600;700&display=swap');
*{margin:0;padding:0;box-sizing:border-box}
:root{--grn:#00ff9d;--amb:#ffb347;--red:#ff4757;--dim:#3a4a52;--txt:#c8d6dd;--bg:#060a0d;--pnl:#0b1216}
body{font-family:'Rajdhani',sans-serif;background:var(--bg);color:var(--txt);min-height:100vh;display:flex;flex-direction:column;align-items:center;padding:16px;
background-image:linear-gradient(rgba(0,255,157,.03) 1px,transparent 1px),linear-gradient(90deg,rgba(0,255,157,.03) 1px,transparent 1px);
background-size:32px 32px}
body::after{content:'';position:fixed;inset:0;pointer-events:none;background:repeating-linear-gradient(0deg,transparent 0 2px,rgba(0,0,0,.08) 2px 4px)}
.topbar{width:100%;max-width:980px;display:flex;justify-content:space-between;align-items:center;border:1px solid var(--dim);
background:var(--pnl);padding:10px 16px;margin-bottom:14px;position:relative}
.topbar::before,.topbar::after{content:'';position:absolute;width:10px;height:10px;border:2px solid var(--grn)}
.topbar::before{top:-2px;left:-2px;border-right:0;border-bottom:0}
.topbar::after{bottom:-2px;right:-2px;border-left:0;border-top:0}
.logo{font-family:'Share Tech Mono',monospace;font-size:1.3rem;color:var(--grn);letter-spacing:4px;text-shadow:0 0 12px rgba(0,255,157,.5)}
.logo span{color:var(--amb)}
.clock{font-family:'Share Tech Mono',monospace;font-size:1.1rem;color:var(--amb)}
.badges{display:flex;gap:8px;margin-bottom:14px;flex-wrap:wrap;justify-content:center}
.bdg{padding:5px 14px;font-size:.7rem;font-weight:700;letter-spacing:2px;border:1px solid var(--dim);background:var(--pnl);text-transform:uppercase}
.bdg.ok{color:var(--grn);border-color:var(--grn);box-shadow:0 0 12px rgba(0,255,157,.2)}
.bdg.err{color:var(--red);border-color:var(--red);box-shadow:0 0 12px rgba(255,71,87,.2)}
.bdg.inf{color:var(--amb);border-color:var(--amb)}
.statstrip{display:grid;grid-template-columns:repeat(4,1fr);gap:10px;width:100%;max-width:980px;margin-bottom:14px}
.stat{background:var(--pnl);border:1px solid var(--dim);padding:12px;text-align:center;position:relative}
.stat::before{content:'';position:absolute;top:0;left:0;width:24px;height:2px;background:var(--grn)}
.stat .lbl{font-size:.6rem;letter-spacing:3px;color:#5a6e78;text-transform:uppercase}
.stat .val{font-family:'Share Tech Mono',monospace;font-size:1.9rem;color:var(--grn);text-shadow:0 0 14px rgba(0,255,157,.4)}
.stat.a .val{color:var(--amb);text-shadow:0 0 14px rgba(255,179,71,.4)}
.stat .unit{font-size:.8rem;color:#5a6e78}
.grid{display:grid;grid-template-columns:1fr;gap:14px;width:100%;max-width:980px}
@media(min-width:880px){.grid{grid-template-columns:1fr 1fr}.statstrip{grid-template-columns:repeat(4,1fr)}}
.panel{background:var(--pnl);border:1px solid var(--dim);padding:18px;position:relative}
.panel::before,.panel::after{content:'';position:absolute;width:8px;height:8px;border:2px solid var(--grn)}
.panel::before{top:-2px;left:-2px;border-right:0;border-bottom:0}
.panel::after{bottom:-2px;right:-2px;border-left:0;border-top:0}
.ph{font-family:'Share Tech Mono',monospace;font-size:.72rem;color:var(--grn);letter-spacing:3px;margin-bottom:14px;
border-bottom:1px solid var(--dim);padding-bottom:8px;display:flex;align-items:center;gap:8px}
.ph::before{content:'▸';color:var(--amb)}
.row{display:flex;justify-content:space-between;margin-bottom:8px;font-size:.9rem;border-bottom:1px dotted #1c272d;padding-bottom:4px}
.k{color:#5a6e78;letter-spacing:1px}.v{font-family:'Share Tech Mono',monospace;color:var(--txt)}
.vg{color:var(--grn)}.va{color:var(--amb)}.vr{color:var(--red)}
input,select,button{width:100%;padding:11px;margin-bottom:10px;background:#060c10;border:1px solid var(--dim);color:var(--txt);font-size:.85rem;outline:none;font-family:'Rajdhani',sans-serif;font-weight:600}
input:focus,select:focus{border-color:var(--grn);box-shadow:0 0 10px rgba(0,255,157,.2)}
button{background:rgba(0,255,157,.07);color:var(--grn);border:1px solid var(--grn);cursor:pointer;letter-spacing:2px;text-transform:uppercase;transition:.15s}
button:hover{background:var(--grn);color:#03110a;box-shadow:0 0 20px rgba(0,255,157,.5)}
.ig{display:flex;gap:8px}.ig input{margin-bottom:0}.ig button{margin-bottom:0;width:auto;flex-shrink:0}
.radwrap{display:flex;justify-content:center;margin:10px 0}
canvas{background:radial-gradient(circle,#08120e 0%,#040a07 100%);border-radius:50%;box-shadow:0 0 30px rgba(0,255,157,.15),inset 0 0 50px rgba(0,255,157,.05)}
.joy{display:grid;grid-template-columns:repeat(3,50px);gap:5px;justify-content:center;margin-top:12px}
.joy button{padding:13px 0;margin:0;background:#0b1216;border:1px solid var(--dim);color:#5a6e78}
.joy button:hover{background:var(--grn);color:#03110a}
.flt{border-color:var(--amb)!important;color:var(--amb)!important}
.flt::placeholder{color:rgba(255,179,71,.4)}
.lost{display:none;position:fixed;top:0;left:0;width:100%;background:var(--red);color:#fff;text-align:center;padding:8px;font-size:.8rem;font-weight:bold;letter-spacing:3px;z-index:999}
</style></head><body>
<div class="lost" id="connLost">⚠ DOWNLINK LOST — REACQUIRING</div>
<div class="topbar"><div class="logo">ORBITAL<span>OPS</span></div><div class="clock" id="utcClock">--:--:--</div></div>
<div class="badges">
<div class="bdg err" id="statusBadge">CONNECTING</div>
<div class="bdg inf" id="modeBadge">--</div>
<div class="bdg inf" id="satBadge">NO TARGET</div>
</div>
<div class="statstrip">
<div class="stat"><div class="lbl">Azimuth</div><div class="val" id="curAz">---</div><div class="unit">DEG</div></div>
<div class="stat a"><div class="lbl">Elevation</div><div class="val" id="curEl">---</div><div class="unit">DEG</div></div>
<div class="stat"><div class="lbl">Range</div><div class="val" id="satDist">---</div><div class="unit">KM</div></div>
<div class="stat a"><div class="lbl">Doppler</div><div class="val" id="doppler">---</div><div class="unit">HZ</div></div>
</div>
<div class="grid">
<div class="panel"><div class="ph">TACTICAL RADAR</div>
<div class="radwrap"><canvas id="radar" width="280" height="280"></canvas></div>
<div class="joy">
<div></div><button onclick="nudge(0,5)">▲</button><div></div>
<button onclick="nudge(-5,0)">◀</button><div></div><button onclick="nudge(5,0)">▶</button>
<div></div><button onclick="nudge(0,-5)">▼</button><div></div>
</div></div>
<div>
<div class="panel" style="margin-bottom:14px"><div class="ph">TELEMETRY</div>
<div class="row"><span class="k">TARGET AZ / EL</span><span class="v vg" id="tgt">--° / --°</span></div>
<div class="row"><span class="k">WIFI SIGNAL</span><span class="v" id="rssi">-- dBm</span></div>
<div class="row"><span class="k">UPTIME</span><span class="v" id="uptime">--</span></div>
<div class="row" style="border:none"><span class="k">FREE MEMORY</span><span class="v" id="ram">-- KB</span></div>
</div>
<div class="panel"><div class="ph">ORBITAL DATABASE</div>
<div class="row" style="border:none;margin-bottom:10px">
<span class="k" style="line-height:2.6">ENGINE</span>
<select id="mode-select" onchange="updateMode()" style="width:170px;margin:0">
<option value="0">EXTERNAL (LOOK4SAT)</option><option value="1">INTERNAL SGP4</option>
</select></div>
<div class="ig"><input type="text" id="grid-input" placeholder="Grid (MM71dl)" maxlength="6"><button onclick="updateGrid()">SET</button></div>
<button onclick="fetchCelestrak()" id="btn-fetch">1 ▸ DOWNLOAD CELESTRAK DB</button>
<input type="text" id="sat-filter" class="flt" placeholder="FILTER TARGETS (NOAA, ISS...)" oninput="filterSats()" disabled>
<select id="sat-select" size="4" style="height:84px"><option>AWAITING DATABASE</option></select>
<button onclick="pushTLE()" id="btn-push">2 ▸ UPLOAD TLE TO HARDWARE</button>
</div></div>
<div class="panel" style="grid-column:1/-1"><div class="ph">PASS LOG</div>
<button onclick="loadPassLog()" style="margin-bottom:10px">REFRESH LOG</button>
<div id="passLogBody" style="font-family:'Share Tech Mono',monospace;font-size:.8rem;color:#5a6e78">NO PASSES RECORDED</div>
</div></div>
<script>
let currentAz=0,currentEl=0,targetAz=0,targetEl=0;
let allTleData=[],tleData=[],satPath=[],isGeo=false;
let failCount=0,sweepAngle=0;
setInterval(()=>{document.getElementById('utcClock').innerText=new Date().toISOString().substr(11,8)+' UTC'},1000);
async function fetchTelemetry(){
 try{
  const r=await fetch('/api/status',{cache:'no-store'});
  if(!r.ok)throw 0;
  const d=await r.json();
  failCount=0;
  document.getElementById('connLost').style.display='none';
  currentAz=d.cAz;currentEl=d.cEl;targetAz=d.tAz;targetEl=d.tEl;isGeo=!!d.geo;
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
  b.className='bdg ok';b.innerText=d.isMoving?'⟳ SLEWING':'● ONLINE';
  document.getElementById('modeBadge').innerText=d.mode===1?(isGeo?'GEO LOCK':'SGP4 INT'):'EXT L4S';
  document.getElementById('satBadge').innerText=d.sat;
 }catch(e){
  if(++failCount>3){
   document.getElementById('connLost').style.display='block';
   const b=document.getElementById('statusBadge');
   b.className='bdg err';b.innerText='✕ OFFLINE';
  }
 }
}
async function fetchPath(){try{const r=await fetch('/api/path');if(r.ok)satPath=await r.json();}catch(e){}}
async function loadPassLog(){
 try{
  const r=await fetch('/api/passlog');const d=await r.json();
  const el=document.getElementById('passLogBody');
  if(!d.length){el.innerText='NO PASSES RECORDED';return;}
  el.innerHTML=d.reverse().map(p=>
   `<div style="margin-bottom:5px;border-bottom:1px dotted #1c272d;padding-bottom:4px">
   <span style="color:#00ff9d">${p.sat}</span> · <span style="color:#5a6e78">${p.time}</span> · 
   <span style="color:#ffb347">MAX EL ${parseFloat(p.maxEl).toFixed(1)}°</span></div>`).join('');
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
 btn.style.color='#00ff9d';
 document.getElementById('sat-filter').disabled=false;
 filterSats();
}
function fetchCelestrak(){
 const btn=document.getElementById('btn-fetch');
 const c=localStorage.getItem('celestrakDB');
 const ct=localStorage.getItem('celestrakTime');
 const now=Date.now();
 if(c&&ct&&(now-ct<14400000)){btn.innerText='LOADING CACHE...';setTimeout(()=>processTLEText(c,btn),400);return;}
 btn.innerText='DOWNLOADING...';
 fetch('https://celestrak.org/NORAD/elements/gp.php?GROUP=active&FORMAT=tle')
 .then(r=>{if(r.status===429)throw new Error("RL");if(!r.ok)throw new Error("NET");return r.text();})
 .then(t=>{localStorage.setItem('celestrakDB',t);localStorage.setItem('celestrakTime',now);processTLEText(t,btn);})
 .catch(e=>{
  btn.innerText=e.message==="RL"?'RATE LIMITED — USING CACHE':'FAILED — USING CACHE';
  if(c)setTimeout(()=>processTLEText(c,btn),1200);
  else{btn.style.color="#ff4757";btn.innerText='BLOCKED — NO CACHE';}
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
 const btn=document.getElementById('btn-push');
 btn.innerText='TRANSMITTING...';
 fetch('/api/tle',{method:'POST',headers:{'Content-Type':'application/json'},
  body:JSON.stringify({name:tleData[i].name,line1:tleData[i].l1,line2:tleData[i].l2})
 }).then(()=>{
  btn.innerText='UPLOAD OK';
  document.getElementById('mode-select').value='1';updateMode();
  setTimeout(()=>{btn.innerText='2 ▸ UPLOAD TLE TO HARDWARE';fetchPath();},3000);
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
function nudge(a,e){
 targetAz=(targetAz+a+360)%360;
 targetEl=Math.max(0,Math.min(90,targetEl+e));
 fetch('/api/manual',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({az:targetAz,el:targetEl})});
}
function animateRadar(){
 requestAnimationFrame(animateRadar);
 const cv=document.getElementById('radar'),ctx=cv.getContext('2d');
 const cx=cv.width/2,cy=cv.height/2,r=cx-22;
 ctx.clearRect(0,0,cv.width,cv.height);
 const toXY=(az,el)=>{
  const rr=r*(1-el/90),rad=(az-90)*Math.PI/180;
  return{x:cx+rr*Math.cos(rad),y:cy+rr*Math.sin(rad)};
 };
 if(satPath.length>1){
  ctx.strokeStyle='rgba(255,179,71,0.8)';ctx.lineWidth=2;ctx.setLineDash([5,4]);
  ctx.beginPath();
  satPath.forEach((p,i)=>{const q=toXY(p.az,p.el);i?ctx.lineTo(q.x,q.y):ctx.moveTo(q.x,q.y);});
  ctx.stroke();ctx.setLineDash([]);
  const a=toXY(satPath[0].az,satPath[0].el),l=toXY(satPath[satPath.length-1].az,satPath[satPath.length-1].el);
  ctx.font='10px "Share Tech Mono"';ctx.textAlign='center';
  ctx.fillStyle='#00ff9d';ctx.fillText('AOS',a.x,a.y-8);
  ctx.fillStyle='#ff4757';ctx.fillText('LOS',l.x,l.y-8);
 }
 ctx.strokeStyle='rgba(0,255,157,0.18)';ctx.lineWidth=1;
 [0.33,0.66,1].forEach(f=>{ctx.beginPath();ctx.arc(cx,cy,r*f,0,2*Math.PI);ctx.stroke();});
 ctx.beginPath();ctx.moveTo(cx,cy-r);ctx.lineTo(cx,cy+r);ctx.stroke();
 ctx.beginPath();ctx.moveTo(cx-r,cy);ctx.lineTo(cx+r,cy);ctx.stroke();
 ctx.textAlign='center';ctx.textBaseline='middle';
 for(let i=0;i<360;i+=30){
  const rad=(i-90)*Math.PI/180;
  ctx.strokeStyle=i%90===0?'#00ff9d':'#1c272d';ctx.lineWidth=i%90===0?2:1;
  ctx.beginPath();ctx.moveTo(cx+r*Math.cos(rad),cy+r*Math.sin(rad));
  ctx.lineTo(cx+(r+5)*Math.cos(rad),cy+(r+5)*Math.sin(rad));ctx.stroke();
  ctx.font=i%90===0?'bold 12px "Share Tech Mono"':'9px "Share Tech Mono"';
  ctx.fillStyle=i%90===0?'#00ff9d':'#3a4a52';
  if(i===0)ctx.fillText('N',cx,cy-r-13);
  else if(i===90)ctx.fillText('E',cx+r+13,cy);
  else if(i===180)ctx.fillText('S',cx,cy+r+13);
  else if(i===270)ctx.fillText('W',cx-r-13,cy);
  else ctx.fillText(i,cx+(r+14)*Math.cos(rad),cy+(r+14)*Math.sin(rad));
 }
 sweepAngle+=0.025;
 ctx.fillStyle='rgba(0,255,157,0.08)';
 ctx.beginPath();ctx.moveTo(cx,cy);ctx.arc(cx,cy,r,sweepAngle,sweepAngle+0.5);ctx.lineTo(cx,cy);ctx.fill();
 ctx.strokeStyle='rgba(0,255,157,0.6)';ctx.lineWidth=1.5;
 ctx.beginPath();ctx.moveTo(cx,cy);ctx.lineTo(cx+r*Math.cos(sweepAngle+0.5),cy+r*Math.sin(sweepAngle+0.5));ctx.stroke();
 const ap=toXY(currentAz,currentEl);
 ctx.strokeStyle='rgba(200,214,221,0.5)';ctx.lineWidth=2;
 ctx.beginPath();ctx.moveTo(cx,cy);ctx.lineTo(ap.x,ap.y);ctx.stroke();
 ctx.fillStyle='#c8d6dd';ctx.beginPath();ctx.arc(ap.x,ap.y,5,0,2*Math.PI);ctx.fill();
 if(targetEl>0){
  const sp=toXY(targetAz,targetEl);
  ctx.fillStyle=isGeo?'#ffb347':'#00ff9d';
  ctx.shadowBlur=16;ctx.shadowColor=ctx.fillStyle;
  ctx.beginPath();ctx.arc(sp.x,sp.y,6,0,2*Math.PI);ctx.fill();
  ctx.shadowBlur=0;
  if(isGeo){ctx.fillStyle='#ffb347';ctx.font='9px "Share Tech Mono"';ctx.fillText('GEO',sp.x,sp.y-12);}
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
<title>NETWORK SETUP</title><style>
@import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Rajdhani:wght@500;600&display=swap');
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Rajdhani',sans-serif;background:#060a0d;color:#c8d6dd;display:flex;align-items:center;justify-content:center;height:100vh;padding:20px;
background-image:linear-gradient(rgba(0,255,157,.03) 1px,transparent 1px),linear-gradient(90deg,rgba(0,255,157,.03) 1px,transparent 1px);background-size:32px 32px}
.panel{background:#0b1216;border:1px solid #3a4a52;padding:32px;width:100%;max-width:400px;text-align:center;position:relative}
.panel::before{content:'';position:absolute;top:-2px;left:-2px;width:12px;height:12px;border:2px solid #00ff9d;border-right:0;border-bottom:0}
.panel::after{content:'';position:absolute;bottom:-2px;right:-2px;width:12px;height:12px;border:2px solid #00ff9d;border-left:0;border-top:0}
h2{font-family:'Share Tech Mono',monospace;color:#00ff9d;margin-bottom:18px;letter-spacing:3px;text-shadow:0 0 12px rgba(0,255,157,.4)}
p{color:#5a6e78;font-size:.9rem;margin-bottom:22px;line-height:1.5}
input{width:100%;padding:14px;margin-bottom:14px;background:#060c10;border:1px solid #3a4a52;color:#c8d6dd;font-size:1rem;outline:none}
input:focus{border-color:#00ff9d}
button{width:100%;padding:14px;background:rgba(0,255,157,.07);color:#00ff9d;border:1px solid #00ff9d;cursor:pointer;font-weight:600;letter-spacing:2px;font-size:1rem;font-family:'Rajdhani',sans-serif}
button:hover{background:#00ff9d;color:#03110a}
</style></head><body>
<div class="panel" id="mainPanel">
<h2>NETWORK SETUP</h2>
<p>Downlink lost. Enter router credentials to restore the ground station connection.</p>
<input type="text" id="ssid" placeholder="WIFI SSID">
<input type="password" id="pass" placeholder="PASSWORD">
<button onclick="saveWifi()">SAVE & REBOOT</button>
</div>
<script>
function saveWifi(){
 const s=document.getElementById('ssid').value,p=document.getElementById('pass').value;
 if(!s||!p){alert("Enter both SSID and Password");return;}
 document.getElementById('mainPanel').innerHTML="<h2>REBOOTING</h2><p>Credentials saved. Return to your home network.</p>";
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
#define C_NEON   0x07F3   // bright spring green
#define C_GREEN  0x07E0
#define C_LIME   0xAFE5
#define C_AMBER  0xFD60
#define C_ORANGE 0xFD20
#define C_RED    0xF800
#define C_CYAN   0x07FF
#define C_MGRAY  0x7BEF
#define C_DGRAY  0x2945
#define C_FAINT  0x1084
#define C_HDRBG  0x01E2   // dark green header
#define C_PNLG   0x0162   // panel title dark green
#define C_PNLA   0x2940   // panel title dark amber
#define C_RING   0x0269   // radar ring dark green
#define C_DRED   0x6000

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

// BUG FIX: static prediction object — previously a local Sgp4 was created
// on the Core0 task stack every 60s. The object is several KB and was
// overflowing/corrupting the stack, silently wiping pass data & path.
static Sgp4 predSat;

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
volatile bool predBusy = false;   // guards path reads during recalc

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
float prev_cAz, prev_cEl, prev_tAz, prev_tEl, prev_dist, prev_dop;
int   prev_rssi, prev_heap;
int   prev_moving, prev_ntp, prev_l4s, prev_mode, prev_geo;
char  prev_sat[25], prev_clock[10];
long  prev_countdown;
float prev_maxElShown;
int prev_antX, prev_antY, prev_satX, prev_satY;
int prev_alX[2], prev_alY[2];
int terminalY = 30;
TaskHandle_t Core0Task;

// Radar geometry
const int RCX = 120, RCY = 228, RR = 55;

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
  // MPU6050: uncomment to expose IMU attitude over the API
  // doc["imuPitch"] = (float)mpuPitch;
  // doc["imuRoll"]  = (float)mpuRoll;
  // doc["imuOK"]    = mpuOK;
  String out;
  serializeJson(doc, out);
  return out;
}

// ================= BOOT SCREENS =================
void drawBootScreen() {
  tft.fillScreen(C_BLACK);
  tft.fillRect(0, 0, 240, 28, C_HDRBG);
  tft.fillRect(0, 28, 240, 2, C_NEON);
  tft.fillRect(0, 0, 4, 28, C_NEON);
  tft.fillRect(236, 0, 4, 28, C_AMBER);
  tft.setTextColor(C_NEON); tft.setTextSize(1);
  tft.setCursor(8, 5);  tft.print("ESP32 SATELLITE TRACKER");
  tft.setTextColor(C_AMBER);
  tft.setCursor(8, 16); tft.print("v9.0  //  ORBITAL OPS");
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
  tft.fillRect(0, 298, 240, 2, C_NEON);
  tft.setTextColor(C_NEON); tft.setCursor(8, 307);
  tft.print("INITIALIZING SYSTEMS...");
}

void printBootTerminal(String msg) {
  Serial.println("[BOOT] " + msg);
  if (terminalY > 300) {
    tft.fillScreen(C_BLACK);
    tft.fillRect(0, 0, 240, 18, C_HDRBG);
    tft.fillRect(0, 18, 240, 2, C_NEON);
    tft.setTextColor(C_NEON); tft.setTextSize(1);
    tft.setCursor(8, 5); tft.print("BOOT SEQUENCE");
    terminalY = 28;
  }
  tft.setTextSize(1);
  tft.setCursor(5, terminalY);
  tft.setTextColor(C_NEON); tft.print("> ");
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
  tft.fillRect(3, 59, 234, 12, C_PNLA);
  tft.setTextColor(C_AMBER); tft.setTextSize(1);
  tft.setCursor(6, 61); tft.print("[ SETUP INSTRUCTIONS ]");
  tft.drawFastHLine(3, 71, 234, C_DGRAY);
  tft.setTextColor(C_AMBER); tft.setCursor(6, 76);  tft.print("1.");
  tft.setTextColor(C_MGRAY); tft.setCursor(20, 76); tft.print("Join this WiFi network:");
  tft.setTextColor(C_NEON);  tft.setCursor(20, 88); tft.print("AEROSPACE-TRACKER");
  tft.setTextColor(C_AMBER); tft.setCursor(6, 104);  tft.print("2.");
  tft.setTextColor(C_MGRAY); tft.setCursor(20, 104); tft.print("WiFi Password:");
  tft.setTextColor(C_WHITE); tft.setCursor(20, 116); tft.print("groundstation");
  tft.setTextColor(C_AMBER); tft.setCursor(6, 132);  tft.print("3.");
  tft.setTextColor(C_MGRAY); tft.setCursor(20, 132); tft.print("Open browser, go to:");
  tft.setTextColor(C_LIME);  tft.setCursor(20, 144); tft.print("192.168.4.1");
  tft.setTextColor(C_AMBER); tft.setCursor(6, 160);  tft.print("4.");
  tft.setTextColor(C_MGRAY); tft.setCursor(20, 160); tft.print("Enter new credentials.");

  tft.drawRoundRect(2, 192, 236, 24, 4, C_DGRAY);
  tft.setTextColor(C_AMBER);
  tft.setCursor(6, 200); tft.print("WAITING FOR CREDENTIALS...");
  tft.fillRect(0, 300, 240, 20, C_DRED);
  tft.setTextColor(C_WHITE);
  tft.setCursor(8, 307); tft.print("AP MODE  |  192.168.4.1");
}

// ================= STATIC HUD FRAME =================
// Layout: HDR 0-17 | TARGET 20-74 | CHIPS 78-100 | PASS 104-146
//         RF row 150-160 | RADAR 164-293 | BAR 302-320
void tftDrawStaticFrame() {
  tft.fillScreen(C_BLACK);
  resetTftCache();

  // Header
  tft.fillRect(0, 0, 240, 16, C_HDRBG);
  tft.fillRect(0, 16, 240, 2, C_NEON);
  tft.fillRect(0, 0, 3, 16, C_AMBER);
  tft.setTextColor(C_NEON); tft.setTextSize(1);
  tft.setCursor(7, 4); tft.print("ORBITAL OPS");
  tft.setTextColor(C_MGRAY);
  tft.setCursor(96, 4); tft.print("UTC");
  // clock at (122,4); NTP dot at (228,8)

  // TARGET panel
  tft.drawRect(2, 20, 236, 54, C_NEON);
  tft.fillRect(3, 21, 234, 11, C_PNLG);
  tft.setTextColor(C_NEON);
  tft.setCursor(6, 23); tft.print("TGT");
  tft.drawFastHLine(3, 32, 234, C_FAINT);
  tft.drawFastVLine(118, 33, 40, C_FAINT);
  tft.setTextColor(C_MGRAY);
  tft.setCursor(6, 40);   tft.print("AZ");
  tft.setCursor(124, 40); tft.print("EL");

  // STATUS CHIPS row (5 chips, 46px each)
  const char* chipLbl[5] = {"ENG", "MOT", "L4S", "NTP", "MEM"};
  for (int i = 0; i < 5; i++) {
    int x = 2 + i * 48;
    tft.drawRect(x, 78, 44, 22, C_DGRAY);
    tft.fillRect(x+1, 79, 42, 8, C_FAINT);
    tft.setTextColor(C_MGRAY);
    tft.setCursor(x + 13, 79); tft.print(chipLbl[i]);
  }

  // PASS panel
  tft.drawRect(2, 104, 236, 42, C_AMBER);
  tft.fillRect(3, 105, 234, 11, C_PNLA);
  tft.setTextColor(C_AMBER);
  tft.setCursor(6, 107); tft.print("PASS PREDICTION");
  tft.drawFastHLine(3, 116, 234, C_FAINT);
  tft.drawFastVLine(118, 117, 28, C_FAINT);
  tft.setTextColor(C_MGRAY);
  tft.setCursor(6, 120);   tft.print("AOS:");
  tft.setCursor(124, 120); tft.print("MAX:");
  tft.setCursor(6, 133);   tft.print("LOS:");
  tft.setCursor(124, 133); tft.print("T- :");

  // RF info row labels
  tft.setTextColor(C_MGRAY);
  tft.setCursor(6, 151);   tft.print("DST:");
  tft.setCursor(124, 151); tft.print("DOP:");

  // Bottom bar
  tft.fillRect(0, 302, 240, 18, C_HDRBG);
  tft.fillRect(0, 300, 240, 2, C_NEON);
  tft.setTextColor(C_NEON); tft.setCursor(6, 307);
  tft.print(isAPMode ? "192.168.4.1" : WiFi.localIP().toString());
  tft.setTextColor(C_MGRAY);
  tft.setCursor(120, 307); tft.print(maidenhead);
  // RSSI bars at (200,305)

  newPathReady = true;
}

void drawRadarBackground() {
  tft.drawCircle(RCX, RCY, RR,     C_RING);
  tft.drawCircle(RCX, RCY, RR*2/3, C_RING);
  tft.drawCircle(RCX, RCY, RR/3,   C_RING);
  tft.drawFastVLine(RCX, RCY - RR, RR*2 + 1, C_RING);
  tft.drawFastHLine(RCX - RR, RCY, RR*2 + 1, C_RING);
  for (int i = 0; i < 360; i += 45) {
    float rad = i * PI / 180.0f;
    tft.drawLine(RCX + (int)((RR-4)*cosf(rad)), RCY + (int)((RR-4)*sinf(rad)),
                 RCX + (int)(RR*cosf(rad)),     RCY + (int)(RR*sinf(rad)),
                 (i % 90 == 0) ? C_NEON : C_DGRAY);
  }
  tft.setTextSize(1);
  tft.setTextColor(C_NEON);
  tft.setCursor(RCX-2,    RCY-RR-10); tft.print("N");
  tft.setCursor(RCX-2,    RCY+RR+4);  tft.print("S");
  tft.setCursor(RCX+RR+5, RCY-3);     tft.print("E");
  tft.setCursor(RCX-RR-11,RCY-3);     tft.print("W");
  tft.setTextColor(C_DGRAY);
  tft.setCursor(RCX+3, RCY-RR/3-4);   tft.print("60");
  tft.setCursor(RCX+3, RCY-RR*2/3-4); tft.print("30");
}

void drawSigBars(int x, int y, int rssi) {
  int lvl = (rssi > -55) ? 4 : (rssi > -65) ? 3 : (rssi > -75) ? 2 : (rssi > -85) ? 1 : 0;
  for (int i = 0; i < 4; i++) {
    int h = 3 + i * 2;
    uint16_t c = (i < lvl) ? ((lvl >= 3) ? C_NEON : (lvl == 2) ? C_AMBER : C_RED) : C_DGRAY;
    tft.fillRect(x + i * 5, y + (9 - h), 3, h, c);
  }
}

// ================= DYNAMIC HUD =================
void tftUpdateDynamic() {
  if (spiLock || isAPMode) return;
  char buf[32];

  // UTC clock + NTP dot
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
  if ((int)ntpSynced != prev_ntp) {
    tft.fillCircle(228, 8, 3, ntpSynced ? C_NEON : C_RED);
    prev_ntp = ntpSynced;
    // also update NTP chip below
    tft.fillRect(147, 88, 42, 11, C_BLACK);
    tft.setTextColor(ntpSynced ? C_NEON : C_RED); tft.setTextSize(1);
    tft.setCursor(150, 90); tft.print(ntpSynced ? "SYNC" : "ERR");
  }

  // Sat name in TARGET title
  if (strcmp(satName, prev_sat) != 0) {
    tft.fillRect(30, 21, 207, 11, C_PNLG);
    tft.setTextColor(C_AMBER); tft.setTextSize(1);
    tft.setCursor(32, 23);
    snprintf(buf, sizeof(buf), "%.22s%s", satName, isGeoSat ? " GEO" : "");
    tft.print(buf);
    strncpy(prev_sat, satName, 24);
  }

  float cAz = currentAz, tAz = targetAz, cEl = currentEl, tEl = targetEl;

  // AZ column
  if (fabsf(cAz - prev_cAz) > 0.05f || fabsf(tAz - prev_tAz) > 0.05f) {
    tft.fillRect(22, 36, 94, 16, C_BLACK);
    tft.setTextColor(C_NEON); tft.setTextSize(2);
    tft.setCursor(22, 36);
    snprintf(buf, sizeof(buf), "%05.1f", (double)cAz);
    tft.print(buf);
    float dAz = tAz - cAz;
    if (dAz >  180) dAz -= 360;
    if (dAz < -180) dAz += 360;
    tft.fillRect(6, 58, 110, 9, C_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(C_WHITE);
    tft.setCursor(6, 58);
    snprintf(buf, sizeof(buf), ">%05.1f", (double)tAz);
    tft.print(buf);
    tft.setTextColor(fabsf(dAz) > 1.0f ? C_AMBER : C_MGRAY);
    tft.setCursor(60, 58);
    snprintf(buf, sizeof(buf), "%+06.1f", (double)dAz);
    tft.print(buf);
    prev_cAz = cAz; prev_tAz = tAz;
  }

  // EL column
  if (fabsf(cEl - prev_cEl) > 0.05f || fabsf(tEl - prev_tEl) > 0.05f) {
    tft.fillRect(140, 36, 96, 16, C_BLACK);
    tft.setTextColor(C_AMBER); tft.setTextSize(2);
    tft.setCursor(140, 36);
    snprintf(buf, sizeof(buf), "%04.1f", (double)cEl);
    tft.print(buf);
    float dEl = tEl - cEl;
    tft.fillRect(124, 58, 110, 9, C_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(C_WHITE);
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

  // Chips: ENG / MOT / L4S / MEM
  int modeVal = onboardMode ? 1 : 0;
  if (modeVal != prev_mode) {
    tft.fillRect(3, 88, 42, 11, C_BLACK);
    tft.setTextColor(onboardMode ? C_NEON : C_AMBER);
    tft.setCursor(6, 90); tft.print(onboardMode ? "SGP4" : "L4S");
    prev_mode = modeVal;
  }
  if ((int)isMoving != prev_moving) {
    tft.fillRect(51, 88, 42, 11, C_BLACK);
    tft.setTextColor(isMoving ? C_AMBER : C_NEON);
    tft.setCursor(54, 90); tft.print(isMoving ? "SLEW" : "IDLE");
    prev_moving = isMoving;
  }
  bool l4sNow = (tcpClient && tcpClient.connected());
  if ((int)l4sNow != prev_l4s) {
    tft.fillRect(99, 88, 42, 11, C_BLACK);
    tft.setTextColor(l4sNow ? C_NEON : C_MGRAY);
    tft.setCursor(102, 90); tft.print(l4sNow ? "LINK" : "----");
    prev_l4s = l4sNow;
  }
  int heapK = ESP.getFreeHeap() / 1024;
  if (abs(heapK - prev_heap) > 2) {
    tft.fillRect(195, 88, 42, 11, C_BLACK);
    tft.setTextColor(heapK > 50 ? C_MGRAY : C_RED);
    tft.setCursor(198, 90);
    snprintf(buf, sizeof(buf), "%dK", heapK);
    tft.print(buf);
    prev_heap = heapK;
  }

  // RF row: DST / DOP
  if (fabsf(satDistance - prev_dist) > 0.5f) {
    tft.fillRect(32, 151, 84, 9, C_BLACK);
    tft.setTextColor(C_WHITE);
    tft.setCursor(32, 151);
    if (satDistance > 0) snprintf(buf, sizeof(buf), "%.0fkm", (double)satDistance);
    else strcpy(buf, "---");
    tft.print(buf);
    prev_dist = satDistance;
  }
  if (fabsf(dopplerFreq - prev_dop) > 1.0f) {
    tft.fillRect(150, 151, 86, 9, C_BLACK);
    tft.setTextColor(C_NEON);
    tft.setCursor(150, 151);
    snprintf(buf, sizeof(buf), "%+05.0fHz", (double)dopplerFreq);
    tft.print(buf);
    prev_dop = dopplerFreq;
  }

  // Bottom bar RSSI bars
  int rssi = isAPMode ? -100 : WiFi.RSSI();
  if (abs(rssi - prev_rssi) > 2) {
    tft.fillRect(198, 304, 40, 14, C_HDRBG);
    drawSigBars(200, 306, rssi);
    prev_rssi = rssi;
  }

  // Pass prediction
  unsigned long nowEpoch = timeClient.getEpochTime();
  int geoState = isGeoSat ? 1 : 0;
  long countdown = (nextAosTime > 0) ? (long)(nextAosTime - (time_t)nowEpoch) : -999999;
  bool redraw = (geoState != prev_geo) ||
                (geoState && fabsf(nextMaxEl - prev_maxElShown) > 0.05f) ||
                (!geoState && countdown != prev_countdown);

  if (redraw) {
    tft.fillRect(32, 120, 84, 9, C_BLACK);
    tft.fillRect(32, 133, 84, 9, C_BLACK);
    tft.fillRect(150, 120, 86, 9, C_BLACK);
    tft.fillRect(150, 133, 86, 9, C_BLACK);

    if (geoState) {
      tft.setTextColor(C_NEON);
      tft.setCursor(32, 120); tft.print("GEO ORBIT");
      tft.setCursor(32, 133); tft.print("STATIC");
      tft.setTextColor(C_AMBER);
      tft.setCursor(150, 120);
      snprintf(buf, sizeof(buf), "%.1f deg", (double)nextMaxEl);
      tft.print(buf);
      tft.setCursor(150, 133);
      if (nextMaxEl > 0) { tft.setTextColor(C_NEON); tft.print("LOCKED"); }
      else               { tft.setTextColor(C_RED);  tft.print("NO VIS"); }
    } else if (nextAosTime > 0) {
      time_t aosT = nextAosTime, losT = nextLosTime;
      struct tm tmAos, tmLos;
      gmtime_r(&aosT, &tmAos);
      gmtime_r(&losT, &tmLos);
      tft.setTextColor(C_WHITE);
      tft.setCursor(32, 120);
      snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tmAos.tm_hour, tmAos.tm_min, tmAos.tm_sec);
      tft.print(buf);
      tft.setCursor(32, 133);
      snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tmLos.tm_hour, tmLos.tm_min, tmLos.tm_sec);
      tft.print(buf);
      tft.setTextColor(C_AMBER);
      tft.setCursor(150, 120);
      snprintf(buf, sizeof(buf), "%.1f deg", (double)nextMaxEl);
      tft.print(buf);
      tft.setCursor(150, 133);
      if (countdown <= 0 && (time_t)nowEpoch < losT) {
        tft.setTextColor(C_NEON); tft.print("TRACKING");
      } else if (countdown > 0) {
        tft.setTextColor(C_ORANGE);
        snprintf(buf, sizeof(buf), "-%02ld:%02ld:%02ld",
                 countdown/3600, (countdown%3600)/60, countdown%60);
        tft.print(buf);
      } else {
        tft.setTextColor(C_MGRAY); tft.print("ACQUIRING");
      }
    } else {
      tft.setTextColor(C_RED);
      tft.setCursor(32, 120); tft.print(tleLoaded ? "SEARCHING" : "NO TLE");
      tft.setCursor(32, 133); tft.print("--:--:--");
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
    // Full bounding-box wipe (clears stale A/L labels too)
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
    tft.drawCircle(RCX, RCY, RR,     C_RING);
    tft.drawCircle(RCX, RCY, RR*2/3, C_RING);
    tft.drawCircle(RCX, RCY, RR/3,   C_RING);
    tft.drawFastVLine(RCX, RCY - RR + 1, RR*2 - 1, C_RING);
    tft.drawFastHLine(RCX - RR + 1, RCY, RR*2 - 1, C_RING);
  }

  // Trajectory (skipped for GEO)
  int len = (isGeoSat || predBusy) ? 0 : globalPathLen;
  prev_alX[0] = prev_alX[1] = -1;
  if (len > 1) {
    int lastX = -1, lastY = -1;
    tft.setTextSize(1);
    for (int i = 0; i < len; i++) {
      int px, py;
      toXY(globalPath[i].az, globalPath[i].el, px, py);
      if (lastX != -1) tft.drawLine(lastX, lastY, px, py, C_AMBER);
      if (i == 0) {
        tft.setTextColor(C_NEON);
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
  tft.drawLine(RCX, RCY, antX, antY, C_MGRAY);
  tft.fillCircle(antX, antY, 3, C_WHITE);
  prev_antX = antX; prev_antY = antY;

  // Satellite dot
  if (targetEl > 0.0f) {
    int sX, sY;
    toXY((float)targetAz, (float)targetEl, sX, sY);
    uint16_t c = isGeoSat ? C_AMBER : C_NEON;
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
    if (!tleLoaded || !ntpSynced || !onboardMode || predBusy) {
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
          nextAosTime = nextLosTime = 0;   // new target — clear old pass data
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

// ================= PASS PREDICTION (HARDENED) =================
void calculatePathPrediction() {
  if (!tleLoaded || !ntpSynced || !onboardMode || isAPMode) {
    globalPathLen = 0; newPathReady = true; return;
  }

  unsigned long nowT = timeClient.getEpochTime();

  // BUG FIX: never recalc with a bogus epoch (NTP hiccup used to wipe data)
  if (nowT < 1000000000UL) {
    Serial.println("[SGP4] Epoch invalid — skipping recalc, keeping last prediction.");
    return;
  }

  predBusy = true;
  predSat.site(obsLat, obsLon, obsAlt);
  predSat.init(satName, tleLine1, tleLine2);
  isGeoSat = tleIsGeo();

  // GEO: live position only
  if (isGeoSat) {
    predSat.findsat(nowT);
    nextMaxEl   = predSat.satEl;
    nextAosTime = (predSat.satEl > 0) ? (time_t)nowT : 0;
    nextLosTime = 0;
    globalPathLen = 0;
    predBusy = false;
    newPathReady  = true;
    return;
  }

  // LEO/MEO: find AOS (60s scan / 10s refine catches short passes)
  unsigned long t = nowT;
  predSat.findsat(t);
  bool found = false;

  if (predSat.satEl > 0) {
    int lim = 0;
    while (predSat.satEl > 0 && lim++ < 80) {
      t -= 30; predSat.findsat(t);
      if (lim % 10 == 0) vTaskDelay(pdMS_TO_TICKS(2));
    }
    t += 30;
    found = true;
  } else {
    for (int i = 0; i < 1440 && !found; i++) {
      t += 60; predSat.findsat(t);
      if (predSat.satEl > 0) {
        int lim = 0;
        while (predSat.satEl > 0 && lim++ < 12) { t -= 10; predSat.findsat(t); }
        t += 10;
        found = true;
      }
      if (i % 30 == 0) vTaskDelay(pdMS_TO_TICKS(2));
    }
  }

  if (!found) {
    // BUG FIX: if the previous prediction is STILL VALID (pass not over yet),
    // keep it instead of destroying good data on a transient search failure.
    if (nextLosTime > (time_t)nowT || nextAosTime > (time_t)nowT) {
      Serial.println("[SGP4] Recalc failed but existing prediction still valid — keeping it.");
      predBusy = false;
      return;
    }
    nextAosTime = nextLosTime = 0;
    nextMaxEl = 0;
    globalPathLen = 0;
    predBusy = false;
    newPathReady = true;
    return;
  }

  // Sample the pass into a temp buffer first, commit only on success
  // (prevents half-written path data being displayed)
  static PathPoint tmpPath[45];
  float maxEl = 0;
  int len = 0;
  unsigned long tt = t;
  for (int i = 0; i < 80; i++) {
    predSat.findsat(tt);
    if (predSat.satEl < 0 && i > 0) break;
    if (predSat.satEl >= 0) {
      if (predSat.satEl > maxEl) maxEl = predSat.satEl;
      if (len < 45) {
        tmpPath[len].az = predSat.satAz;
        tmpPath[len].el = predSat.satEl;
        len++;
      }
    }
    tt += 30;
    if (i % 10 == 0) vTaskDelay(pdMS_TO_TICKS(2));
  }

  // Atomic-ish commit
  memcpy((void*)globalPath, tmpPath, sizeof(PathPoint) * len);
  nextAosTime   = (time_t)t;
  nextLosTime   = (time_t)tt;
  nextMaxEl     = maxEl;
  globalPathLen = len;
  predBusy      = false;
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

    // Recalc every 60s, OR immediately when the tracked pass has just ended
    bool passExpired = (nextLosTime > 0 && !isGeoSat &&
                        (time_t)timeClient.getEpochTime() > nextLosTime &&
                        millis() - lastPathCalc > 10000);
    if ((millis() - lastPathCalc > 60000 || passExpired) && !isAPMode) {
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
  Serial.println(" ESP32 Orbital Ops v9.0 (ST7789)");
  Serial.println("====================================");

  esp_task_wdt_init(30, true);
  esp_task_wdt_add(NULL);

  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, HIGH);
  azStepper.setMaxSpeed(2000); azStepper.setAcceleration(1000);
  elStepper.setMaxSpeed(2000); elStepper.setAcceleration(1000);

  // ── MPU6050: uncomment to enable IMU ──
  // if (mpuInit()) {
  //   mpuOK = true;
  //   Serial.println("[IMU] MPU6050 online @ 0x68");
  //   // Optional: zero the elevation axis from the IMU at boot
  //   // for (int i = 0; i < 50; i++) { mpuRead(); delay(10); }
  //   // elStepper.setCurrentPosition((long)(mpuPitch * STEPS_PER_DEG));
  // } else {
  //   Serial.println("[IMU] MPU6050 NOT FOUND — check wiring");
  // }

  tft.init(240, 320);
  tft.setRotation(0);
  tft.invertDisplay(false);
  drawBootScreen();
  delay(3000);

  tft.fillScreen(C_BLACK);
  tft.fillRect(0, 0, 240, 18, C_HDRBG);
  tft.fillRect(0, 18, 240, 2, C_NEON);
  tft.setTextColor(C_NEON); tft.setTextSize(1);
  tft.setCursor(8, 5); tft.print("BOOT SEQUENCE");
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

  printBootTerminal("Starting Ops HUD...");
  delay(500);
  tftDrawStaticFrame();
  digitalWrite(ENABLE_PIN, LOW);

  // BUG FIX: stack raised 16K -> 24K. SGP4 prediction + TFT + JSON on this
  // task was running close to the limit and corrupting tracking state.
  xTaskCreatePinnedToCore(Core0TaskCode, "Core0Task", 24576, NULL, 1, &Core0Task, 0);
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

  // ── MPU6050: uncomment to enable wind-drift stabilization ──
  // if (millis() - lastMpuRead > 200) {        // 5 Hz IMU sampling
  //   mpuRead();
  //   mpuStabilize();                          // re-zeros EL axis on drift
  //   lastMpuRead = millis();
  // }

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
  if (now < 1000000000UL) return;   // BUG FIX: never track with bad epoch
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

  if (isGeoSat) nextMaxEl = sat.satEl;   // live GEO elevation

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