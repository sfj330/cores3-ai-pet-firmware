#pragma once

static const char WEB_UI_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>CoreS3 Pet Control</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,sans-serif;background:#1a1a2e;color:#eee;padding:16px;max-width:600px;margin:0 auto}
h1{font-size:1.3em;color:#4bf;margin-bottom:12px;text-align:center}
.card{background:#16213e;border-radius:12px;padding:14px;margin-bottom:12px}
.card h2{font-size:0.9em;color:#888;margin-bottom:8px}
.status-grid{display:grid;grid-template-columns:1fr 1fr;gap:6px}
.status-item{font-size:0.85em;padding:4px 0}
.status-item span{color:#4bf}
.btn-grid{display:grid;grid-template-columns:repeat(4,1fr);gap:8px}
.btn{background:#0f3460;border:1px solid #4bf3;color:#eee;padding:10px 4px;border-radius:8px;
font-size:0.8em;cursor:pointer;text-align:center;transition:background 0.2s}
.btn:hover{background:#4bf;color:#000}
.btn.active{background:#4bf;color:#000}
.ctrl-row{display:flex;gap:8px;margin-top:8px}
.ctrl-row .btn{flex:1}
.photos-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:6px;margin-top:8px}
.photo-thumb{width:100%;aspect-ratio:4/3;object-fit:cover;border-radius:6px;cursor:pointer}
.photo-item{position:relative}
.photo-del{position:absolute;top:2px;right:2px;background:rgba(255,0,80,0.8);color:#fff;
border:none;border-radius:50%;width:20px;height:20px;font-size:12px;cursor:pointer;line-height:20px;text-align:center}
#toast{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);background:#4bf;color:#000;
padding:8px 16px;border-radius:20px;font-size:0.85em;display:none;z-index:99}
</style>
</head>
<body>
<h1>CoreS3 Pet Control</h1>
<div class="card">
<h2>Status</h2>
<div class="status-grid" id="status">
<div class="status-item">Page: <span id="s-page">-</span></div>
<div class="status-item">WiFi: <span id="s-wifi">-</span></div>
<div class="status-item">AI: <span id="s-ai">-</span></div>
<div class="status-item">SD: <span id="s-sd">-</span></div>
<div class="status-item">Heap: <span id="s-heap">-</span></div>
<div class="status-item">PSRAM: <span id="s-psram">-</span></div>
</div>
</div>
)rawhtml"
R"rawhtml(
<div class="card">
<h2>Pages</h2>
<div class="btn-grid">
<div class="btn" onclick="nav('face')">Face</div>
<div class="btn" onclick="nav('menu')">Menu</div>
<div class="btn" onclick="nav('camera')">Camera</div>
<div class="btn" onclick="nav('music')">Music</div>
<div class="btn" onclick="nav('pomodoro')">Timer</div>
<div class="btn" onclick="nav('album')">Album</div>
<div class="btn" onclick="nav('ai')">AI</div>
<div class="btn" onclick="nav('system')">System</div>
</div>
</div>
<div class="card">
<h2>Brightness</h2>
<div class="ctrl-row">
<div class="btn" onclick="bright('dim')">Dim</div>
<div class="btn" onclick="bright('normal')">Normal</div>
<div class="btn" onclick="bright('bright')">Bright</div>
</div>
<h2 style="margin-top:10px">Volume</h2>
<div class="ctrl-row">
<div class="btn" onclick="vol('quiet')">Quiet</div>
<div class="btn" onclick="vol('normal')">Normal</div>
<div class="btn" onclick="vol('loud')">Loud</div>
</div>
<div class="ctrl-row" style="margin-top:10px">
<div class="btn" onclick="slp('sleep')">Sleep</div>
<div class="btn" onclick="slp('wake')">Wake</div>
</div>
</div>
<div class="card">
<h2>Servo</h2>
<div class="btn-grid">
<div class="btn" onclick="servo('left')">Left</div>
<div class="btn" onclick="servo('up')">Up</div>
<div class="btn" onclick="servo('down')">Down</div>
<div class="btn" onclick="servo('right')">Right</div>
</div>
<div class="btn-grid" style="margin-top:8px">
<div class="btn" onclick="servo('center')">Center</div>
<div class="btn" onclick="servo('nod')">Nod</div>
<div class="btn" onclick="servo('shake')">Shake</div>
<div class="btn" onclick="servo('dance')">Dance</div>
</div>
<div class="ctrl-row" style="margin-top:8px">
<div class="btn" onclick="servo('release')">Release</div>
</div>
</div>
<div class="card">
<h2>Photos</h2>
<div class="photos-grid" id="photos"></div>
</div>
<div id="toast"></div>
<script>
function toast(msg){let t=document.getElementById('toast');t.textContent=msg;t.style.display='block';setTimeout(()=>t.style.display='none',2000)}
async function post(url,data){let r=await fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data)});return r.json()}
async function nav(p){let r=await post('/api/page',{page:p});toast(r.ok?'OK':'Failed');refresh()}
async function bright(l){await post('/api/brightness',{level:l});toast('Brightness: '+l)}
async function vol(l){await post('/api/volume',{level:l});toast('Volume: '+l)}
async function slp(a){await post('/api/sleep',{action:a});toast(a)}
async function servo(a){let r=await post('/api/servo',{action:a});toast(r.ok?'Servo: '+a:'Servo failed')}
async function delPhoto(n){if(!confirm('Delete '+n+'?'))return;let r=await fetch('/api/photos',{method:'DELETE',headers:{'Content-Type':'application/json'},body:JSON.stringify({name:n})});let j=await r.json();toast(j.ok?'Deleted':'Failed');loadPhotos()}
async function loadPhotos(){try{let r=await fetch('/api/photos');let list=await r.json();let g=document.getElementById('photos');g.innerHTML='';list.forEach(n=>{let d=document.createElement('div');d.className='photo-item';d.innerHTML='<img class="photo-thumb" src="/api/photos/'+n+'" onclick="window.open(this.src)"><button class="photo-del" onclick="delPhoto(\''+n+'\')">x</button>';g.appendChild(d)})}catch(e){}}
async function refresh(){try{let r=await fetch('/api/status');let s=await r.json();document.getElementById('s-page').textContent=s.page||'-';document.getElementById('s-wifi').textContent=s.wifi||'-';document.getElementById('s-ai').textContent=s.ai||'-';document.getElementById('s-sd').textContent=s.sd||'-';document.getElementById('s-heap').textContent=(s.heap||'-')+'KB';document.getElementById('s-psram').textContent=(s.psram||'-')+'KB'}catch(e){}}
refresh();loadPhotos();setInterval(refresh,3000);setInterval(loadPhotos,10000);
</script>
</body>
</html>
)rawhtml"
;
