#ifndef WEB_PAGES_H
#define WEB_PAGES_H

#include <Arduino.h>

// ================= WEB DASHBOARD =================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>ORBITAL OPS</title><style>
@import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&family=JetBrains+Mono:wght@500;700&display=swap');
*{margin:0;padding:0;box-sizing:border-box;font-family:'Inter',sans-serif}
:root{
 --pri:#6366f1; --pri-glow:rgba(99,102,241,0.4);
 --acc:#2dd4bf; --acc-glow:rgba(45,212,191,0.4);
 --grn:#10b981; --amb:#f59e0b; --red:#f43f5e;
 --bg:#020617; --pnl:rgba(15,23,42,0.6); --dim:#1e293b; --txt:#f8fafc; --txt-dim:#94a3b8;
}
body{
 background:var(--bg); color:var(--txt); min-height:100vh; display:flex; flex-direction:column; align-items:center; padding:20px;
 background-image:radial-gradient(circle at 15% 50%,rgba(99,102,241,0.08),transparent 25%),radial-gradient(circle at 85% 30%,rgba(45,212,191,0.08),transparent 25%);
 background-attachment:fixed;
}
.topbar{
 width:100%; max-width:1000px; display:flex; justify-content:space-between; align-items:center;
 background:var(--pnl); backdrop-filter:blur(12px); -webkit-backdrop-filter:blur(12px);
 border:1px solid rgba(255,255,255,0.05); border-radius:16px; padding:16px 24px; margin-bottom:20px;
 box-shadow:0 10px 30px rgba(0,0,0,0.2);
}
.logo{font-weight:700; font-size:1.4rem; color:var(--txt); letter-spacing:2px; display:flex; align-items:center; gap:8px}
.logo span{color:var(--acc)}
.logo svg{width:20px; height:20px; fill:var(--acc)}
.clock{font-family:'JetBrains Mono',monospace; font-size:1.1rem; color:var(--txt-dim); font-weight:500}
.badges{display:flex; gap:10px; margin-bottom:20px; flex-wrap:wrap; justify-content:center}
.bdg{
 padding:6px 16px; font-size:0.75rem; font-weight:600; letter-spacing:1px; border-radius:20px;
 background:rgba(255,255,255,0.03); border:1px solid rgba(255,255,255,0.1); text-transform:uppercase;
 transition:0.3s;
}
.bdg.ok{color:var(--acc); border-color:var(--acc); background:rgba(45,212,191,0.1); box-shadow:0 0 15px var(--acc-glow)}
.bdg.err{color:var(--red); border-color:var(--red); background:rgba(244,63,94,0.1); box-shadow:0 0 15px rgba(244,63,94,0.3); animation:pulse-err 1.5s infinite}
@keyframes pulse-err { 0%,100%{box-shadow:0 0 15px rgba(244,63,94,0.3)} 50%{box-shadow:0 0 30px rgba(244,63,94,0.6)} }
.bdg.inf{color:var(--pri); border-color:var(--pri); background:rgba(99,102,241,0.1)}
.statstrip{display:grid; grid-template-columns:repeat(4,1fr); gap:16px; width:100%; max-width:1000px; margin-bottom:20px}
.stat{
 background:var(--pnl); backdrop-filter:blur(12px); border:1px solid rgba(255,255,255,0.05); border-radius:16px;
 padding:20px; text-align:center; position:relative; overflow:hidden; transition:transform 0.2s, box-shadow 0.2s;
}
.stat:hover{transform:translateY(-2px); box-shadow:0 10px 30px rgba(0,0,0,0.3), 0 0 0 1px var(--pri-glow)}
.stat::before{content:''; position:absolute; top:0; left:0; width:100%; height:2px; background:linear-gradient(90deg,var(--pri),var(--acc)); opacity:0.5}
.stat .lbl{font-size:0.7rem; font-weight:600; letter-spacing:1.5px; color:var(--txt-dim); text-transform:uppercase; margin-bottom:8px}
.stat .val{
 font-family:'JetBrains Mono',monospace; font-size:2.2rem; font-weight:700;
 background:linear-gradient(to right, #fff, #94a3b8); -webkit-background-clip:text; -webkit-text-fill-color:transparent;
}
.stat.a .val{background:linear-gradient(to right, var(--acc), var(--pri)); -webkit-background-clip:text; -webkit-text-fill-color:transparent}
.stat .unit{font-size:0.8rem; color:var(--txt-dim); font-weight:500}
.grid{display:grid; grid-template-columns:1fr; gap:20px; width:100%; max-width:1000px}
@media(min-width:880px){.grid{grid-template-columns:1fr 1fr}}
.panel{
 background:var(--pnl); backdrop-filter:blur(12px); border:1px solid rgba(255,255,255,0.05); border-radius:16px;
 padding:24px; position:relative; transition:transform 0.2s;
}
.panel:hover{box-shadow:0 10px 40px rgba(0,0,0,0.2)}
.ph{
 font-weight:600; font-size:0.85rem; color:var(--txt); letter-spacing:2px; text-transform:uppercase;
 margin-bottom:20px; border-bottom:1px solid rgba(255,255,255,0.05); padding-bottom:12px; display:flex; align-items:center; gap:8px
}
.ph::before{content:''; display:block; width:8px; height:8px; border-radius:50%; background:var(--acc); box-shadow:0 0 10px var(--acc-glow)}
.row{display:flex; justify-content:space-between; align-items:center; margin-bottom:12px; font-size:0.9rem; border-bottom:1px solid rgba(255,255,255,0.03); padding-bottom:8px}
.k{color:var(--txt-dim); font-weight:500; letter-spacing:0.5px}
.v{font-family:'JetBrains Mono',monospace; color:var(--txt); font-weight:500}
.vg{color:var(--acc)}.va{color:var(--pri)}.vr{color:var(--red)}
input,select,button{
 width:100%; padding:12px 16px; margin-bottom:12px; background:rgba(0,0,0,0.3); border:1px solid rgba(255,255,255,0.1);
 color:var(--txt); border-radius:8px; font-size:0.9rem; outline:none; font-weight:500; transition:0.2s
}
input:focus,select:focus{border-color:var(--pri); box-shadow:0 0 0 3px var(--pri-glow); background:rgba(0,0,0,0.5)}
button{
 background:var(--dim); color:var(--txt); border:none; cursor:pointer; font-weight:600; letter-spacing:1px;
 display:flex; justify-content:center; align-items:center; gap:8px
}
button:hover{background:var(--pri); transform:translateY(-1px); box-shadow:0 4px 15px var(--pri-glow)}
button:active{transform:translateY(1px)}
.ig{display:flex; gap:10px}.ig input,.ig button{margin-bottom:0}.ig button{width:auto; flex-shrink:0}
.radwrap{display:flex; justify-content:center; margin:10px 0; position:relative}
canvas{border-radius:50%; box-shadow:0 0 40px rgba(0,0,0,0.5), inset 0 0 40px rgba(0,0,0,0.5)}
#worldMap{border-radius:12px; box-shadow:none}
.joy{display:grid; grid-template-columns:repeat(3,44px); gap:8px; justify-content:center; margin-top:20px}
.joy button{padding:12px 0; margin:0; background:rgba(255,255,255,0.05); border-radius:12px; color:var(--txt-dim)}
.joy button:hover{background:var(--acc); color:#000; box-shadow:0 0 15px var(--acc-glow)}
.flt{border-color:var(--pri)!important; color:var(--pri)!important}
.flt::placeholder{color:rgba(99,102,241,0.5)}
.lost{display:none; position:fixed; top:0; left:0; width:100%; background:var(--red); color:#fff; text-align:center; padding:10px; font-size:0.85rem; font-weight:700; letter-spacing:2px; z-index:999; box-shadow:0 0 20px rgba(244,63,94,0.5)}
.sched-row{display:flex; justify-content:space-between; align-items:center; padding:10px 0; border-bottom:1px solid rgba(255,255,255,0.03); font-size:0.85rem}
.sched-sat{color:var(--txt); font-weight:600; min-width:120px}
.sched-time{color:var(--txt-dim); font-family:'JetBrains Mono',monospace}
.sched-el{color:var(--acc); font-family:'JetBrains Mono',monospace; font-weight:700; text-align:right}
.wx-grid{display:grid; grid-template-columns:1fr 1fr; gap:12px; margin-top:16px}
.wx-stat{background:rgba(0,0,0,0.2); border:1px solid rgba(255,255,255,0.05); border-radius:12px; padding:16px; text-align:center}
.wx-val{font-family:'JetBrains Mono',monospace; font-size:1.5rem; font-weight:700; color:var(--txt)}
.wx-lbl{font-size:0.65rem; color:var(--txt-dim); font-weight:600; letter-spacing:1px; text-transform:uppercase; margin-top:4px}
.queue-item{display:flex; justify-content:space-between; align-items:center; padding:8px 12px; margin-bottom:8px; background:rgba(0,0,0,0.2); border-radius:8px; border:1px solid rgba(255,255,255,0.03); transition:0.2s}
.queue-item:hover{background:rgba(255,255,255,0.05)}
.queue-item.active{border-color:var(--acc); background:rgba(45,212,191,0.05); box-shadow:0 0 15px rgba(45,212,191,0.1)}
.queue-item button{width:auto; padding:6px 12px; margin:0; font-size:0.75rem; border-radius:6px; background:transparent; border:1px solid var(--red); color:var(--red)}
.queue-item button:hover{background:var(--red); color:#fff; box-shadow:0 0 15px rgba(244,63,94,0.4)}
.grade{display:inline-flex; align-items:center; justify-content:center; width:22px; height:22px; font-size:0.7rem; font-weight:700; border-radius:6px; font-family:'JetBrains Mono',monospace; margin-left:8px}
.grade-a{background:rgba(16,185,129,0.2); color:var(--grn); border:1px solid var(--grn)}
.grade-b{background:rgba(245,158,11,0.2); color:var(--amb); border:1px solid var(--amb)}
.grade-c{background:rgba(244,63,94,0.2); color:var(--red); border:1px solid var(--red)}
.grade-f{background:rgba(244,63,94,0.1); color:var(--red); border:1px dotted var(--red)}
.vis-dot{display:inline-block; width:8px; height:8px; border-radius:50%; margin-right:10px; box-shadow:0 0 8px currentColor}
.raw-tle-area{background:rgba(0,0,0,0.3); color:var(--txt); border:1px solid rgba(255,255,255,0.1); border-radius:8px; width:100%; padding:12px; font-family:'JetBrains Mono',monospace; font-size:0.8rem; resize:vertical; min-height:80px; margin-bottom:12px}
.raw-tle-area:focus{border-color:var(--acc); box-shadow:0 0 0 3px var(--acc-glow); outline:none}
/* Night Mode Overrides */
body.night{filter:none!important}
body.night .panel, body.night .stat, body.night .topbar{background:#050000!important; border-color:#330000!important; box-shadow:none!important}
body.night .ph{color:#ef4444!important; border-color:#330000!important}
body.night .ph::before{background:#ef4444!important; box-shadow:none!important}
body.night .val, body.night .vg, body.night .va, body.night .sched-sat, body.night .sched-el{color:#ef4444!important; background:none!important; -webkit-text-fill-color:#ef4444!important; text-shadow:0 0 10px rgba(239,68,68,0.5)!important}
body.night .logo, body.night .logo span{color:#ef4444!important}
body.night canvas{filter:sepia(1) saturate(5) hue-rotate(320deg) brightness(0.6)}
body.night button{border:1px solid #550000!important; color:#ef4444!important; background:#1a0000!important; box-shadow:none!important}
body.night button:hover{background:#ef4444!important; color:#000!important}
body.night #nightBtn{background:#ef4444!important; color:#000!important}
</style></head><body>
<div class="lost" id="connLost">⚠ DOWNLINK LOST — REACQUIRING</div>
<div class="topbar">
  <div class="logo">ORBITAL<span>OPS</span></div>
  <div style="display:flex;align-items:center;gap:10px">
   <span id="weatherBadge" style="font-size:.75rem;color:#7a7493;font-family:'Share Tech Mono',monospace"></span>
   <button id="nightBtn" onclick="toggleNightMode()" style="width:auto;padding:5px 12px;margin:0;font-size:.65rem;letter-spacing:2px">NIGHT</button>
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
</div>
<div style="border-top:1px solid #1c2733;margin:12px 0 10px;padding-top:10px">
<div style="font-size:.65rem;letter-spacing:2px;color:#7a7493;margin-bottom:6px">PASTE RAW TLE (3 LINES)</div>
<textarea id="raw-tle-input" class="raw-tle-area" rows="3" placeholder="Satellite Name&#10;1 NNNNNC 00000A ...&#10;2 NNNNN ..."></textarea>
<button onclick="pushRawTLE()" style="border-color:#b9a0dc;color:#b9a0dc">UPLOAD RAW TLE</button>
</div></div>

<div class="panel" style="margin-bottom:14px"><div class="ph">ORBITAL ELEMENTS</div>
<div id="elemBody">
<div style="color:#7a7493;font-size:.8rem">Load a TLE to view elements</div>
</div></div>

<div class="panel" style="margin-bottom:14px"><div class="ph">SATELLITE QUEUE</div>
<div id="queueBody"><div style="color:#7a7493;font-size:.8rem">Queue empty</div></div>
<button onclick="clearQueue()" style="margin-top:8px;border-color:#ff6b8a;color:#ff6b8a">CLEAR QUEUE</button>
</div>

<div class="panel"><div class="ph" style="justify-content:space-between">
<span>PASS SCHEDULE (24H)</span>
<button onclick="downloadScheduleCSV()" style="width:auto;padding:3px 10px;margin:0;font-size:.65rem">CSV</button>
</div>
<button onclick="loadSchedule()" style="margin-bottom:10px">REFRESH SCHEDULE</button>
<div id="schedBody" style="font-family:'Share Tech Mono',monospace;font-size:.75rem;color:#7a7493;max-height:200px;overflow-y:auto">
Press REFRESH to compute
</div>
<div class="panel" style="margin-top:10px;padding:12px"><div class="ph" style="font-size:.65rem;margin-bottom:8px">PASS ELEVATION PROFILE</div>
<canvas id="passGraph" width="380" height="110" style="width:100%;border-radius:4px;background:#080614"></canvas>
<button onclick="loadPassGraph()" style="margin-top:6px;font-size:.7rem">LOAD GRAPH</button>
</div></div>
</div>
</div>

<div class="panel" style="width:100%;max-width:980px;margin-top:14px"><div class="ph">GROUND TRACK</div>
<div style="position:relative;width:100%;height:220px;border-radius:4px;overflow:hidden;background:#08060e">
 <div style="position:absolute;inset:0;background:url('https://upload.wikimedia.org/wikipedia/commons/e/ec/World_map_blank_without_borders.svg') center/100% 100% no-repeat;filter:invert(1) opacity(0.15) hue-rotate(180deg)"></div>
 <canvas id="worldMap" width="700" height="220" style="position:absolute;inset:0;width:100%;height:100%;background:transparent;box-shadow:inset 0 0 50px rgba(120,230,154,.05)"></canvas>
</div>
</div>

<div class="panel" style="width:100%;max-width:980px;margin-top:14px"><div class="ph">PASS LOG</div>
<button onclick="loadPassLog()" style="margin-bottom:10px">REFRESH LOG</button>
<div id="passLogBody" style="font-family:'Share Tech Mono',monospace;font-size:.8rem;color:#7a7493">NO PASSES RECORDED</div>
</div>

<div class="panel" style="width:100%;max-width:980px;margin-top:14px"><div class="ph">QSO CONTACT LOG</div>
<div class="ig" style="margin-bottom:8px">
 <input type="text" id="qso-call" placeholder="Callsign" style="margin:0;flex:1">
 <input type="text" id="qso-notes" placeholder="Notes" style="margin:0;flex:2">
 <button onclick="logQSO()" style="width:auto;flex-shrink:0;padding:11px 14px;margin:0">LOG</button>
 <button onclick="clearQSOLog()" style="width:auto;flex-shrink:0;padding:11px 10px;margin:0;border-color:#ff6b8a;color:#ff6b8a">CLR</button>
</div>
<div id="qsoLogBody" style="font-family:'Share Tech Mono',monospace;font-size:.75rem;color:#7a7493;max-height:160px;overflow-y:auto">No contacts logged.</div>
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
  if(d.wxDesc) document.getElementById('weatherBadge').innerText=d.wxDesc+' '+parseFloat(d.wxTemp||0).toFixed(0)+'C';
  renderWeather(d);
  renderQueue(d.queue||[]);
  renderOrbitalElements(d);
  lastSatLat=d.satLat||0;lastSatLon=d.satLon||0;
  drawWorldMap();
 }catch(e){
  if(++failCount>3){
   document.getElementById('connLost').style.display='block';
   const b=document.getElementById('statusBadge');
   b.className='bdg err';b.innerText='X OFFLINE';
  }
 }
}

function renderWeather(d){
 if(!d.wxValid){document.getElementById('wxBody').innerHTML='<div style="color:#7a7493;font-size:.8rem">No weather data</div>';return;}
 const COMP=['N','NNE','NE','ENE','E','ESE','SE','SSE','S','SSW','SW','WSW','W','WNW','NW','NNW'];
 const wdeg=parseFloat(d.wxWindDeg||0);
 const wcomp=COMP[((Math.round(wdeg/22.5))%16+16)%16];
 const kpLevel=d.kpValid?(d.kp<3?'QUIET':d.kp<5?'UNSETTLED':d.kp<7?'STORM':'SEVERE STORM'):'--';
 const kpColor=d.kpValid?(d.kp<3?'var(--acc)':d.kp<5?'var(--pri)':d.kp<7?'#ff9100':'var(--red)'):'#7a7493';
 const tleAgeStr=(d.tleAge&&d.tleAge>0)?d.tleAge.toFixed(1)+' d':'--';
 const tleAgeColor=(d.tleAge&&d.tleAge>14)?'var(--red)':(d.tleAge&&d.tleAge>7)?'#ff9100':'var(--acc)';
 document.getElementById('wxBody').innerHTML=
  '<div style="color:#cabfe6;margin-bottom:4px;font-size:.9rem">'+d.wxCity+'</div>'+
  '<div style="color:#7a7493;margin-bottom:8px;font-size:.78rem;letter-spacing:1px;text-transform:uppercase">'+d.wxDesc+'</div>'+
  '<div class="wx-grid">'+
  '<div class="wx-stat"><div class="wx-val">'+parseFloat(d.wxTemp||0).toFixed(1)+'C</div><div class="wx-lbl">TEMP</div></div>'+
  '<div class="wx-stat"><div class="wx-val" style="color:var(--pri)">'+parseFloat(d.wxFeels||0).toFixed(1)+'C</div><div class="wx-lbl">FEELS</div></div>'+
  '<div class="wx-stat"><div class="wx-val">'+parseFloat(d.wind||0).toFixed(1)+'</div><div class="wx-lbl">WIND m/s</div></div>'+
  '<div class="wx-stat"><div class="wx-val">'+wdeg.toFixed(0)+' '+wcomp+'</div><div class="wx-lbl">WIND DIR</div></div>'+
  '<div class="wx-stat"><div class="wx-val">'+(d.wxHum||'--')+'%</div><div class="wx-lbl">HUMIDITY</div></div>'+
  '<div class="wx-stat"><div class="wx-val" style="color:var(--pri)">'+(d.wxPressure||'--')+'hPa</div><div class="wx-lbl">PRESSURE</div></div>'+
  '</div>'+
  '<div class="row" style="margin-top:10px;border-top:1px solid #1c2733;padding-top:8px">'+
  '<span class="k">Kp INDEX</span>'+
  '<span style="font-family:monospace;color:'+kpColor+'">'+(d.kpValid?d.kp.toFixed(1):'--')+' ('+kpLevel+')</span></div>'+
  '<div class="row" style="border:none"><span class="k">TLE AGE</span>'+
  '<span style="font-family:monospace;color:'+tleAgeColor+'">'+tleAgeStr+((d.tleAge&&d.tleAge>7)?' (STALE)':'')+'</span></div>';
}


let lastSchedule=[];
let lastSatLat=0,lastSatLon=0;

function passGrade(el){
 if(el>=60) return '<span class="grade grade-a">A</span>';
 if(el>=30) return '<span class="grade grade-b">B</span>';
 if(el>=10) return '<span class="grade grade-c">C</span>';
 return '<span class="grade grade-f">F</span>';
}

function toggleNightMode(){
 document.body.classList.toggle('night');
}

function maidenheadToLatLon(g){
 if(!g||g.length<2)return{lat:0,lon:0};
 g=g.toUpperCase();
 let lon=(g.charCodeAt(0)-65)*20-180;
 let lat=(g.charCodeAt(1)-65)*10-90;
 if(g.length>=4){lon+=(parseInt(g[2]))*2;lat+=parseInt(g[3]);}
 if(g.length>=6){lon+=((g.charCodeAt(4)-65)*5)/60+2.5/60;lat+=((g.charCodeAt(5)-65)*2.5)/60+1.25/60;}
 lon+=1;lat+=0.5;
 return{lat,lon};
}

function drawWorldMap(){
 const c=document.getElementById('worldMap');
 if(!c)return;
 const W=c.width,H=c.height;
 const ctx=c.getContext('2d');
 ctx.clearRect(0,0,W,H);
 // Lat/lon grid
 function proj(lat,lon){return{x:Math.round((lon+180)/360*W),y:Math.round((90-lat)/180*H)};}
 ctx.strokeStyle='rgba(26,21,48,0.5)';ctx.lineWidth=0.5;
 for(let lon=-180;lon<=180;lon+=30){const p1=proj(-90,lon),p2=proj(90,lon);ctx.beginPath();ctx.moveTo(p1.x,p1.y);ctx.lineTo(p2.x,p2.y);ctx.stroke();}
 for(let lat=-60;lat<=60;lat+=30){
 ctx.strokeStyle=(lat===0)?'#2a2060':'#1a1530';
 const p1=proj(lat,-180),p2=proj(lat,180);ctx.beginPath();ctx.moveTo(p1.x,p1.y);ctx.lineTo(p2.x,p2.y);ctx.stroke();
 }
 // Labels
 ctx.fillStyle='#2a2060';ctx.font='9px monospace';
 ctx.fillText('EQ',2,proj(0,0).y-2);
 ctx.fillText('N',2,proj(60,-180).y);ctx.fillText('S',2,proj(-60,-180).y);
 // Footprint circle
 if(satFootprintRadius>0&&lastSatLat!==0){
 const fp=proj(lastSatLat,lastSatLon);
 const fpPxRadius=(satFootprintRadius/6371)*W*(180/360)*2;
 ctx.beginPath();ctx.arc(fp.x,fp.y,fpPxRadius,0,2*Math.PI);
 ctx.strokeStyle='rgba(120,230,154,0.15)';ctx.lineWidth=1;ctx.stroke();
 ctx.fillStyle='rgba(120,230,154,0.04)';ctx.fill();
 }
 // Observer position
 const obs=maidenheadToLatLon(document.getElementById('grid-input').placeholder||'MM71');
 const op=proj(obs.lat,obs.lon);
 ctx.beginPath();
 ctx.moveTo(op.x,op.y-6);ctx.lineTo(op.x+5,op.y+4);ctx.lineTo(op.x-5,op.y+4);ctx.closePath();
 ctx.fillStyle='#b9a0dc';ctx.fill();
 // Satellite dot
 if(lastSatLat!==0||lastSatLon!==0){
 const sp=proj(lastSatLat,lastSatLon);
 const grad=ctx.createRadialGradient(sp.x,sp.y,0,sp.x,sp.y,8);
 grad.addColorStop(0,'rgba(120,230,154,.9)');grad.addColorStop(1,'rgba(120,230,154,0)');
 ctx.beginPath();ctx.arc(sp.x,sp.y,8,0,2*Math.PI);ctx.fillStyle=grad;ctx.fill();
 ctx.beginPath();ctx.arc(sp.x,sp.y,3,0,2*Math.PI);ctx.fillStyle='#78e69a';ctx.fill();
 ctx.fillStyle='#78e69a';ctx.font='bold 9px monospace';ctx.fillText('SAT',sp.x+6,sp.y-4);
 }
}

async function loadPassGraph(){
 try{
  const r=await fetch('/api/path');
  const pts=await r.json();
  if(!pts||!pts.length)return;
  const c=document.getElementById('passGraph');
  const W=c.width,H=c.height;
  const ctx=c.getContext('2d');
  const PAD={l:28,r:10,t:8,b:22};
  const gW=W-PAD.l-PAD.r,gH=H-PAD.t-PAD.b;
  ctx.fillStyle='#080614';ctx.fillRect(0,0,W,H);
  // Grid lines
  ctx.strokeStyle='#1a1530';ctx.lineWidth=0.5;ctx.setLineDash([2,4]);
  [30,60].forEach(el=>{
   const y=PAD.t+gH*(1-el/90);
   ctx.beginPath();ctx.moveTo(PAD.l,y);ctx.lineTo(PAD.l+gW,y);ctx.stroke();
   ctx.fillStyle='#3a3460';ctx.font='8px monospace';ctx.fillText(el+'',2,y+3);
  });
  ctx.setLineDash([]);
  // Axes
  ctx.strokeStyle='#3a2a55';ctx.lineWidth=1;
  ctx.beginPath();ctx.moveTo(PAD.l,PAD.t);ctx.lineTo(PAD.l,PAD.t+gH);ctx.lineTo(PAD.l+gW,PAD.t+gH);ctx.stroke();
  // Fill + line
  const grad=ctx.createLinearGradient(0,PAD.t,0,PAD.t+gH);
  grad.addColorStop(0,'rgba(120,230,154,0.35)');grad.addColorStop(1,'rgba(120,230,154,0.03)');
  ctx.beginPath();
  pts.forEach((p,i)=>{
   const x=PAD.l+(i/(pts.length-1||1))*gW;
   const y=PAD.t+gH*(1-Math.min(p.el,90)/90);
   i===0?ctx.moveTo(x,y):ctx.lineTo(x,y);
  });
  ctx.lineTo(PAD.l+gW,PAD.t+gH);ctx.lineTo(PAD.l,PAD.t+gH);
  ctx.closePath();ctx.fillStyle=grad;ctx.fill();
  ctx.beginPath();
  pts.forEach((p,i)=>{
   const x=PAD.l+(i/(pts.length-1||1))*gW;
   const y=PAD.t+gH*(1-Math.min(p.el,90)/90);
   i===0?ctx.moveTo(x,y):ctx.lineTo(x,y);
  });
  ctx.strokeStyle='#78e69a';ctx.lineWidth=1.5;ctx.stroke();
  // Peak label
  const maxEl=Math.max(...pts.map(p=>p.el));
  const maxIdx=pts.findIndex(p=>p.el===maxEl);
  const px=PAD.l+(maxIdx/(pts.length-1||1))*gW;
  const py=PAD.t+gH*(1-maxEl/90);
  ctx.fillStyle='#78e69a';ctx.font='bold 9px monospace';
  ctx.fillText(maxEl.toFixed(1),px-10,py-4);
  // X axis labels
  ctx.fillStyle='#7a7493';ctx.font='8px monospace';
  [0,0.25,0.5,0.75,1].forEach(f=>{
   const mins=((pts.length-1)*f*30/60).toFixed(0);
   const x=PAD.l+f*gW;
   ctx.fillText(mins+'m',x-6,H-5);
  });
 }catch(e){console.log('passGraph err',e);}
}

function renderOrbitalElements(d){
 const el=d.tleElem;
 const b=document.getElementById('elemBody');
 if(!b)return;
 if(!el){b.innerHTML='<div style="color:#7a7493;font-size:.8rem">Load a TLE to view elements</div>';return;}
 const age=d.tleAge||0;
 const ageColor=age>14?'var(--red)':age>7?'#ff9100':'var(--acc)';
 const ageLabel=age>7?' (STALE)':'';
 const orb=el.perigeeAlt>35000?'GEO':el.perigeeAlt>2000?'MEO':el.perigeeAlt>200?'LEO':'SPL';
 b.innerHTML=
  '<div class="row"><span class="k">NORAD</span><span class="v vg">'+el.catalogNum+'</span></div>'+
  '<div class="row"><span class="k">INCLINATION</span><span class="v">'+parseFloat(el.inclination).toFixed(3)+'</span></div>'+
  '<div class="row"><span class="k">RAAN</span><span class="v">'+parseFloat(el.raan).toFixed(2)+'</span></div>'+
  '<div class="row"><span class="k">ECCENTRICITY</span><span class="v">'+parseFloat(el.eccentricity).toFixed(6)+'</span></div>'+
  '<div class="row"><span class="k">PERIGEE</span><span class="v">'+Math.round(el.perigeeAlt)+' km</span></div>'+
  '<div class="row"><span class="k">APOGEE</span><span class="v">'+Math.round(el.apogeeAlt)+' km</span></div>'+
  '<div class="row"><span class="k">PERIOD</span><span class="v vg">'+parseFloat(el.period).toFixed(1)+' min</span></div>'+
  '<div class="row"><span class="k">ORBIT TYPE</span><span class="v va">'+orb+'</span></div>'+
  '<div class="row" style="border:none"><span class="k">TLE AGE</span><span style="font-family:monospace;color:'+ageColor+'">'+age.toFixed(1)+'d'+ageLabel+'</span></div>';
}

function renderQueue(q){
 const now=Date.now()/1000;
 const el=document.getElementById('queueBody');
 if(!q||!q.length){el.innerHTML='<div style="color:#7a7493;font-size:.8rem">Queue empty</div>';return;}
 el.innerHTML=q.map((s,i)=>{
  let dotColor='#3a3460';
  const pass=lastSchedule.find(p=>p.sat===s.name);
  if(pass){
   if(now>=pass.aos&&now<=pass.los)dotColor='#78e69a';
   else if(pass.aos-now<600)dotColor='#ffd600';
  }
  return '<div class="queue-item'+(s.active?' active':'')+'">'+
  '<span style="display:flex;align-items:center"><span class="vis-dot" style="background:'+dotColor+'" title="Visibility status"></span>'+s.name+'</span>'+
  '<button onclick="removeFromQueue('+i+')">REMOVE</button></div>';
 }).join('');
}

async function loadSchedule(){
 try{
  await fetch('/api/schedule/refresh',{method:'POST'});
  await new Promise(r=>setTimeout(r,2000));
  const r=await fetch('/api/schedule');
  const data=await r.json();
  lastSchedule=data||[];
  if(!lastSchedule.length){document.getElementById('schedBody').innerHTML='<span style="color:#7a7493">No passes in next 24h</span>';return;}
  document.getElementById('schedBody').innerHTML=lastSchedule.map(p=>{
   const aosD=new Date(p.aos*1000);
   const losD=new Date(p.los*1000);
   const fmt=d=>d.toUTCString().substr(17,5);
   const g=passGrade(p.maxEl);
   return '<div class="sched-row"><span class="sched-sat">'+p.sat+'</span>'+
   '<span class="sched-time">'+fmt(aosD)+'-'+fmt(losD)+'</span>'+
   '<span class="sched-el">'+p.maxEl.toFixed(1)+''+g+'</span></div>';
  }).join('');
 }catch(e){document.getElementById('schedBody').innerHTML='Error';}
}

function downloadScheduleCSV(){
 if(!lastSchedule.length){alert('Refresh schedule first.');return;}
 let csv='Satellite,AOS_UTC,LOS_UTC,MaxEl_deg,AOS_Az_deg\n';
 lastSchedule.forEach(p=>{
  const ao=new Date(p.aos*1000).toISOString();
  const lo=new Date(p.los*1000).toISOString();
  csv+=p.sat+','+ao+','+lo+','+p.maxEl.toFixed(1)+','+(p.aosAz||0).toFixed(1)+'\n';
 });
 const a=document.createElement('a');
 a.href='data:text/csv;charset=utf-8,'+encodeURIComponent(csv);
 a.download='pass_schedule.csv';a.click();
}

async function pushRawTLE(){
 const raw=document.getElementById('raw-tle-input').value;
 const lines=raw.split('\n').map(s=>s.trim()).filter(s=>s);
 if(lines.length<3){alert('Need 3 lines: name, TLE line 1, TLE line 2');return;}
 if(!lines[1].startsWith('1 ')||!lines[2].startsWith('2 ')){alert('Lines 2 and 3 must start with "1 " and "2 "');return;}
 try{
  const r=await fetch('/api/tle',{method:'POST',headers:{'Content-Type':'application/json'},
   body:JSON.stringify({name:lines[0],line1:lines[1],line2:lines[2]})});
  const j=await r.json();
  if(j.status==='ok'){alert('TLE uploaded: '+lines[0]);document.getElementById('raw-tle-input').value='';}
  else alert('Upload failed.');
 }catch(e){alert('Error: '+e);}
}

let _qsoSat='';
async function fetchTelForQSO(){try{const r=await fetch('/api/status',{cache:'no-store'});const d=await r.json();_qsoSat=d.sat||'';}catch(e){}}
fetchTelForQSO();

function logQSO(){
 const call=document.getElementById('qso-call').value.trim().toUpperCase();
 if(!call){alert('Enter a callsign.');return;}
 const notes=document.getElementById('qso-notes').value.trim();
 const entry={call,notes,sat:_qsoSat,time:new Date().toISOString()};
 const stored=JSON.parse(localStorage.getItem('sattracker_qso')||'[]');
 stored.unshift(entry);
 localStorage.setItem('sattracker_qso',JSON.stringify(stored.slice(0,200)));
 document.getElementById('qso-call').value='';
 document.getElementById('qso-notes').value='';
 renderQSOLog();
}

function clearQSOLog(){
 if(!confirm('Clear all QSO contacts?'))return;
 localStorage.removeItem('sattracker_qso');
 renderQSOLog();
}

function renderQSOLog(){
 const data=JSON.parse(localStorage.getItem('sattracker_qso')||'[]');
 const el=document.getElementById('qsoLogBody');
 if(!data.length){el.innerHTML='No contacts logged.';return;}
 el.innerHTML=data.slice(0,20).map(e=>{
  const dt=new Date(e.time).toUTCString().substr(4,20);
  return '<div style="border-bottom:1px dotted #1c2733;padding:3px 0">'+
  '<span style="color:var(--acc)">'+e.call+'</span> via '+
  '<span style="color:var(--pri)">'+e.sat+'</span> '+
  '<span style="color:#7a7493">['+dt+']</span>'+
  (e.notes?' <span style="color:#9a93b0">'+e.notes+'</span>':'')+
  '</div>';
 }).join('');
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
 // Fade for sweep trail effect
 ctx.fillStyle='rgba(2,6,23,0.15)';
 ctx.fillRect(0,0,cv.width,cv.height);
 
 const toXY=(az,el)=>{
  const safeEl=Math.max(0,Math.min(90,el));
  const rr=r*(1-safeEl/90),rad=(az-90)*Math.PI/180;
  return{x:cx+rr*Math.cos(rad),y:cy+rr*Math.sin(rad)};
 };

 // Grid
 ctx.strokeStyle='rgba(255,255,255,0.05)';ctx.lineWidth=1;ctx.setLineDash([]);
 [0.33,0.66,1].forEach(f=>{ctx.beginPath();ctx.arc(cx,cy,r*f,0,2*Math.PI);ctx.stroke();});
 ctx.beginPath();ctx.moveTo(cx,cy-r);ctx.lineTo(cx,cy+r);ctx.stroke();
 ctx.beginPath();ctx.moveTo(cx-r,cy);ctx.lineTo(cx+r,cy);ctx.stroke();
 ctx.textAlign='center';ctx.textBaseline='middle';
 for(let i=0;i<360;i+=30){
  const rad=(i-90)*Math.PI/180;
  const isCard = i%90===0;
  ctx.strokeStyle=isCard?'rgba(45,212,191,0.5)':'rgba(255,255,255,0.1)';ctx.lineWidth=isCard?2:1;
  ctx.beginPath();ctx.moveTo(cx+r*Math.cos(rad),cy+r*Math.sin(rad));
  ctx.lineTo(cx+(r+(isCard?6:4))*Math.cos(rad),cy+(r+(isCard?6:4))*Math.sin(rad));ctx.stroke();
  ctx.font=isCard?'bold 12px "JetBrains Mono"':'9px "JetBrains Mono"';
  ctx.fillStyle=isCard?'#2dd4bf':'#64748b';
  if(i===0)ctx.fillText('N',cx,cy-r-14);
  else if(i===90)ctx.fillText('E',cx+r+14,cy);
  else if(i===180)ctx.fillText('S',cx,cy+r+14);
  else if(i===270)ctx.fillText('W',cx-r-14,cy);
  else ctx.fillText(i,cx+(r+14)*Math.cos(rad),cy+(r+14)*Math.sin(rad));
 }

 // Trajectory (Ghost Trail)
 if(satPath.length>1){
  ctx.strokeStyle='rgba(99,102,241,0.4)';ctx.lineWidth=2;ctx.setLineDash([4,4]);
  ctx.beginPath();
  satPath.forEach((p,i)=>{const q=toXY(p.az,p.el);i?ctx.lineTo(q.x,q.y):ctx.moveTo(q.x,q.y);});
  ctx.stroke();ctx.setLineDash([]);
  const a=toXY(satPath[0].az,satPath[0].el),l=toXY(satPath[satPath.length-1].az,satPath[satPath.length-1].el);
  ctx.font='10px "JetBrains Mono"';
  ctx.fillStyle='#10b981';ctx.fillText('AOS',a.x,a.y-10);
  ctx.fillStyle='#f43f5e';ctx.fillText('LOS',l.x,l.y-10);
 }

 // Sweep
 sweepAngle+=0.04;
 ctx.fillStyle='rgba(45,212,191,0.1)';
 ctx.beginPath();ctx.moveTo(cx,cy);ctx.arc(cx,cy,r,sweepAngle,sweepAngle+0.4);ctx.lineTo(cx,cy);ctx.fill();
 ctx.strokeStyle='rgba(45,212,191,0.8)';ctx.lineWidth=2;
 ctx.beginPath();ctx.moveTo(cx,cy);ctx.lineTo(cx+r*Math.cos(sweepAngle+0.4),cy+r*Math.sin(sweepAngle+0.4));ctx.stroke();

 // Satellite footprint circle
 if(targetEl>0&&satFootprintRadius>0&&satDist>0){
  const sp=toXY(targetAz,targetEl);
  const angRad=Math.atan2(satFootprintRadius,satDist);
  const angDeg=angRad*(180/Math.PI);
  const fpPx=Math.max(6,Math.min(r*0.9,(angDeg/90)*r));
  const pulse = 1 + 0.1 * Math.sin(Date.now()/200); // pulsing effect
  
  ctx.strokeStyle='rgba(45,212,191,0.05)';ctx.lineWidth=10;
  ctx.beginPath();ctx.arc(sp.x,sp.y,fpPx*pulse,0,2*Math.PI);ctx.stroke();
  
  ctx.strokeStyle='rgba(45,212,191,0.3)';ctx.lineWidth=1.5;ctx.setLineDash([4,4]);
  ctx.beginPath();ctx.arc(sp.x,sp.y,fpPx,0,2*Math.PI);ctx.stroke();
  ctx.setLineDash([]);
 }

 const safeEl=Math.max(0,currentEl);
 const ap=toXY(currentAz,safeEl);
 
 // Lock Beam & Target
 if(targetEl>0){
  const tp=toXY(targetAz,targetEl);
  const isLocked = Math.abs(currentAz-targetAz)<3 && Math.abs(currentEl-targetEl)<3;
  
  // Connection Beam
  ctx.strokeStyle = isLocked ? 'rgba(16,185,129,0.5)' : 'rgba(99,102,241,0.3)';
  ctx.lineWidth = isLocked ? 2 : 1;
  ctx.setLineDash(isLocked ? [] : [4,4]);
  ctx.beginPath();ctx.moveTo(ap.x,ap.y);ctx.lineTo(tp.x,tp.y);ctx.stroke();
  ctx.setLineDash([]);

  // Target Crosshair
  const ds=7;
  ctx.strokeStyle='rgba(99,102,241,0.9)';ctx.lineWidth=1.5;
  ctx.beginPath();ctx.moveTo(tp.x,tp.y-ds);ctx.lineTo(tp.x+ds,tp.y);
  ctx.lineTo(tp.x,tp.y+ds);ctx.lineTo(tp.x-ds,tp.y);ctx.closePath();ctx.stroke();
  ctx.fillStyle='rgba(99,102,241,0.2)';ctx.fill();
  
  // Satellite Dot
  const sc=isGeo?'#818cf8':'#2dd4bf';
  ctx.fillStyle=sc;ctx.shadowBlur=15;ctx.shadowColor=sc;
  ctx.beginPath();ctx.arc(tp.x,tp.y,4,0,2*Math.PI);ctx.fill();
  ctx.shadowBlur=0;
  
  // Lock Status Text
  ctx.font='bold 10px "JetBrains Mono"';ctx.textAlign='center';
  ctx.fillStyle = isLocked ? '#10b981' : '#818cf8';
  ctx.fillText(isLocked ? 'LOCKED' : (isGeo?'GEO':'TGT'),tp.x,tp.y-14);
 }

 // Antenna Needle
 ctx.strokeStyle='rgba(226,232,240,0.8)';ctx.lineWidth=2.5;
 ctx.shadowBlur=10; ctx.shadowColor='rgba(255,255,255,0.5)';
 ctx.beginPath();ctx.moveTo(cx,cy);ctx.lineTo(ap.x,ap.y);ctx.stroke();
 ctx.fillStyle='#ffffff';ctx.beginPath();ctx.arc(ap.x,ap.y,4,0,2*Math.PI);ctx.fill();
 ctx.shadowBlur=0;
 ctx.font='9px "JetBrains Mono"';ctx.textAlign='left';ctx.textBaseline='top';
 ctx.fillStyle='#e2e8f0';
 ctx.fillText('ANT',ap.x+(ap.x>=cx?8:-22),ap.y+(ap.y>=cy?8:-14));
 // Parked indicator
 if(isParked){
  ctx.fillStyle='rgba(244,63,94,0.15)';ctx.beginPath();ctx.arc(cx,cy,r,0,2*Math.PI);ctx.fill();
  ctx.font='bold 11px "JetBrains Mono"';ctx.textAlign='center';ctx.textBaseline='middle';
  ctx.fillStyle='#f43f5e';ctx.fillText('PARKED',cx,cy+r*0.6);
 }
}

setInterval(fetchTelemetry,500);
renderQSOLog();
drawWorldMap();
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
