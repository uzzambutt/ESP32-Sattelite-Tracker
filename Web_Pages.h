#ifndef WEB_PAGES_H
#define WEB_PAGES_H

#include <Arduino.h>

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
<div class="ig" style="margin-bottom:10px">
 <input type="text" id="wx-city-input" placeholder="City name (e.g. Lahore)" style="margin:0" maxlength="47">
 <button onclick="setWeatherCity()" style="width:auto;flex-shrink:0;padding:11px 12px;margin:0">SET</button>
 <button onclick="clearWeatherCity()" style="width:auto;flex-shrink:0;padding:11px 10px;margin:0;border-color:#ff6b8a;color:#ff6b8a">CLR</button>
</div>
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
let satFootprintRadius=0,satDist=1;

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
  satDist=d.dist||1;
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
  // Pre-fill city input with the currently active override (don't overwrite if user is typing)
  if(d.wxCityOverride!==undefined){
   const inp=document.getElementById('wx-city-input');
   if(document.activeElement!==inp)inp.value=d.wxCityOverride||'';
  }
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
 if(!q||!q.length){el.innerHTML='<div style="color:#7a7493;font-size:.8rem">Queue empty</div>';return;}
 el.innerHTML=q.map((s,i)=>`
 <div class="queue-item ${s.active?'active':''}" id="qitem-${i}">
  <span style="font-family:'Share Tech Mono',monospace;font-size:.8rem">${s.active?'▶ ':''} ${s.name}</span>
  <button onclick="removeFromQueue(${i})" style="border-color:#ff6b8a;color:#ff6b8a">✕</button>
 </div>`).join('');
}

async function loadSchedule(){
 document.getElementById('schedBody').innerText='Computing...';
 try{
  await fetch('/api/schedule/refresh',{method:'POST'});
  let d=null, tries=0;
  do{
   await new Promise(r=>setTimeout(r,500));
   const r=await fetch('/api/schedule'); d=await r.json();
   if(d.length) break;
  }while(++tries<20);   // up to 10s for Core0 to finish all queued sats
  if(!d||!d.length){document.getElementById('schedBody').innerText='No passes in next 24h';return;}
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
 // Optimistic UI update: remove the row immediately
 const el=document.getElementById('qitem-'+idx);
 if(el)el.remove();
 // If queue body is now empty, show placeholder
 const body=document.getElementById('queueBody');
 if(body&&!body.querySelector('.queue-item'))body.innerHTML='<div style="color:#7a7493;font-size:.8rem">Queue empty</div>';
 fetch('/api/queue/remove',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({idx})});
}

function clearQueue(){
 document.getElementById('queueBody').innerHTML='<div style="color:#7a7493;font-size:.8rem">Queue empty</div>';
 fetch('/api/queue/clear',{method:'POST'});
}

function setWeatherCity(){
 const city=document.getElementById('wx-city-input').value.trim();
 if(!city)return;
 fetch('/api/weather/city',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({city})});
}
function clearWeatherCity(){
 document.getElementById('wx-city-input').value='';
 fetch('/api/weather/city',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({city:''})});
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
 // Satellite footprint circle (feature 13) — physics-based angular radius
 if(targetEl>0&&satFootprintRadius>0&&satDist>0){
  const sp=toXY(targetAz,targetEl);
  // Angular half-angle of footprint as seen from observer = atan(fp_km / slant_km)
  const angRad=Math.atan2(satFootprintRadius,satDist); // radians
  const angDeg=angRad*(180/Math.PI);                   // degrees of sky
  const fpPx=Math.max(6,Math.min(r*0.9,(angDeg/90)*r));
  // Outer glow
  ctx.strokeStyle='rgba(120,230,154,0.07)';ctx.lineWidth=6;ctx.setLineDash([]);
  ctx.beginPath();ctx.arc(sp.x,sp.y,fpPx,0,2*Math.PI);ctx.stroke();
  // Main dashed circle
  ctx.strokeStyle='rgba(120,230,154,0.30)';ctx.lineWidth=1.5;ctx.setLineDash([4,3]);
  ctx.beginPath();ctx.arc(sp.x,sp.y,fpPx,0,2*Math.PI);ctx.stroke();
  ctx.setLineDash([]);
  // Label
  ctx.font='8px "Share Tech Mono"';ctx.textAlign='center';ctx.fillStyle='rgba(120,230,154,0.5)';
  ctx.fillText(Math.round(satFootprintRadius)+'km',sp.x,sp.y-fpPx-4);
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

#endif // WEB_PAGES_H
