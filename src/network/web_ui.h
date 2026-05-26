#pragma once

static const char WEB_UI_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>CoreS3 AI Pet</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'SF Pro Display',sans-serif;
background:#000;color:#f5f5f7;min-height:100vh;padding:20px 16px;max-width:480px;margin:0 auto}
h1{font-size:1.6em;font-weight:600;text-align:center;margin-bottom:16px;
background:linear-gradient(135deg,#64d2ff,#5e5ce6);-webkit-background-clip:text;
-webkit-text-fill-color:transparent}
.face-wrap{background:#1c1c1e;border-radius:20px;padding:12px;margin-bottom:16px;
box-shadow:0 4px 24px rgba(0,0,0,0.6)}
#faceCanvas{display:block;width:100%;border-radius:12px;background:#000}
.card{background:rgba(28,28,30,0.8);backdrop-filter:blur(20px);-webkit-backdrop-filter:blur(20px);
border-radius:16px;padding:16px;margin-bottom:12px;border:1px solid rgba(255,255,255,0.06)}
.card h2{font-size:0.75em;font-weight:500;color:rgba(255,255,255,0.4);
text-transform:uppercase;letter-spacing:0.5px;margin-bottom:10px}
.grid{display:grid;gap:8px}
.g4{grid-template-columns:repeat(4,1fr)}
.g3{grid-template-columns:repeat(3,1fr)}
.g2{grid-template-columns:1fr 1fr}
.btn{background:rgba(99,99,102,0.24);border:none;color:#f5f5f7;padding:12px 6px;
border-radius:12px;font-size:0.8em;font-weight:500;cursor:pointer;text-align:center;
transition:all 0.2s ease;-webkit-tap-highlight-color:transparent}
.btn:hover{background:rgba(99,99,102,0.48);transform:scale(1.02)}
.btn:active{transform:scale(0.96);background:rgba(99,99,102,0.6)}
.btn-em{background:linear-gradient(135deg,rgba(94,92,230,0.3),rgba(100,210,255,0.2));
border:1px solid rgba(100,210,255,0.2)}
.btn-em:hover{background:linear-gradient(135deg,rgba(94,92,230,0.5),rgba(100,210,255,0.4))}
.btn-act{background:linear-gradient(135deg,rgba(255,159,10,0.25),rgba(255,55,95,0.2));
border:1px solid rgba(255,159,10,0.2)}
.btn-act:hover{background:linear-gradient(135deg,rgba(255,159,10,0.45),rgba(255,55,95,0.4))}
.status-bar{display:flex;gap:8px;flex-wrap:wrap;margin-bottom:12px}
.status-pill{background:rgba(44,44,46,0.8);border-radius:20px;padding:4px 10px;
font-size:0.7em;color:rgba(255,255,255,0.6)}
.status-pill span{color:#64d2ff}
</style>
</head>
<body>
<h1>CoreS3 AI Pet</h1>
<div class="status-bar" id="status">
<div class="status-pill"><span id="s-page">-</span></div>
<div class="status-pill">WiFi: <span id="s-wifi">-</span></div>
<div class="status-pill">AI: <span id="s-ai">-</span></div>
</div>
<div class="face-wrap"><canvas id="faceCanvas" width="320" height="240"></canvas></div>
)rawhtml"
R"rawhtml(
<div class="card">
<h2>Emotions</h2>
<div class="grid g4">
<div class="btn btn-em" onclick="emo('normal')">Normal</div>
<div class="btn btn-em" onclick="emo('happy')">Happy</div>
<div class="btn btn-em" onclick="emo('curious')">Curious</div>
<div class="btn btn-em" onclick="emo('shy')">Shy</div>
<div class="btn btn-em" onclick="emo('surprised')">Surprise</div>
<div class="btn btn-em" onclick="emo('sleepy')">Sleepy</div>
<div class="btn btn-em" onclick="emo('thinking')">Think</div>
<div class="btn btn-em" onclick="emo('sick')">Sick</div>
</div>
</div>
<div class="card">
<h2>Actions</h2>
<div class="grid g4">
<div class="btn btn-act" onclick="act('nod')">Nod</div>
<div class="btn btn-act" onclick="act('shake')">Shake</div>
<div class="btn btn-act" onclick="act('dance')">Dance</div>
<div class="btn btn-act" onclick="act('wink')">Wink</div>
</div>
</div>
<div class="card">
<h2>Pages</h2>
<div class="grid g4">
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
<h2>Controls</h2>
<div class="grid g3">
<div class="btn" onclick="bright('dim')">Dim</div>
<div class="btn" onclick="bright('normal')">Normal</div>
<div class="btn" onclick="bright('bright')">Bright</div>
</div>
<div class="grid g3" style="margin-top:8px">
<div class="btn" onclick="vol('quiet')">Quiet</div>
<div class="btn" onclick="vol('normal')">Normal</div>
<div class="btn" onclick="vol('loud')">Loud</div>
</div>
<div class="grid g2" style="margin-top:8px">
<div class="btn" onclick="slp('sleep')">Sleep</div>
<div class="btn" onclick="slp('wake')">Wake</div>
</div>
</div>
)rawhtml"
R"rawhtml(
<div class="card">
<h2>Servo</h2>
<div class="grid g4">
<div class="btn" onclick="servo('left')">Left</div>
<div class="btn" onclick="servo('up')">Up</div>
<div class="btn" onclick="servo('down')">Down</div>
<div class="btn" onclick="servo('right')">Right</div>
</div>
<div class="grid g4" style="margin-top:8px">
<div class="btn" onclick="servo('center')">Center</div>
<div class="btn" onclick="servo('nod')">Nod</div>
<div class="btn" onclick="servo('shake')">Shake</div>
<div class="btn" onclick="servo('dance')">Dance</div>
</div>
</div>
<div class="card">
<h2>Camera</h2>
<div style="text-align:center">
<img id="stream" style="width:100%;border-radius:12px;display:none">
<div class="grid g2" style="margin-top:8px">
<div class="btn" onclick="toggleStream(true)">Start Preview</div>
<div class="btn" onclick="toggleStream(false)">Stop</div>
</div>
</div>
</div>
<div class="card">
<h2>OTA Update</h2>
<input type="file" id="otaFile" accept=".bin" style="color:#aaa;font-size:0.8em;margin-bottom:8px;width:100%">
<div class="btn" onclick="doOta()" style="width:100%">Upload Firmware</div>
<div id="ota-progress" style="margin-top:8px;font-size:0.75em;color:#64d2ff;display:none"></div>
</div>
<div class="card">
<h2>Photos</h2>
<div class="grid g3" id="photos"></div>
</div>
<div id="toast" style="position:fixed;bottom:30px;left:50%;transform:translateX(-50%);
background:rgba(99,99,102,0.9);backdrop-filter:blur(10px);color:#fff;padding:10px 20px;
border-radius:24px;font-size:0.8em;display:none;z-index:99"></div>
)rawhtml"
R"rawhtml(
<script>
const C=document.getElementById('faceCanvas'),X=C.getContext('2d');
let face={emotion:0,gazeX:0,gazeY:0,blinking:false,speaking:false,mouthOpen:0};
let particles=[];
const EMOTIONS=['normal','happy','curious','listening','thinking','speaking','surprised','sleepy','tracking','shy','sick'];

function toast(m){let t=document.getElementById('toast');t.textContent=m;t.style.display='block';setTimeout(()=>t.style.display='none',2000)}
async function post(u,d){let r=await fetch(u,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(d)});return r.json()}
async function nav(p){let r=await post('/api/page',{page:p});toast(r.ok?'OK':'Failed');refreshStatus()}
async function bright(l){await post('/api/brightness',{level:l});toast(l)}
async function vol(l){await post('/api/volume',{level:l});toast(l)}
async function slp(a){await post('/api/sleep',{action:a});toast(a)}
async function servo(a){let r=await post('/api/servo',{action:a});toast(r.ok?a:'Failed')}
async function emo(e){let r=await post('/api/emotion',{emotion:e});toast(r.ok?e:'Failed')}
async function act(a){let r=await post('/api/action',{action:a});toast(r.ok?a:'Failed')}
function toggleStream(on){let img=document.getElementById('stream');if(on){img.src='/api/stream';img.style.display='block'}else{img.src='';img.style.display='none'}}
async function doOta(){let f=document.getElementById('otaFile').files[0];if(!f){toast('Select .bin');return}
let p=document.getElementById('ota-progress');p.style.display='block';p.textContent='Uploading...';
try{let r=await fetch('/api/ota',{method:'POST',headers:{'Content-Type':'application/octet-stream'},body:f});
let j=await r.json();p.textContent=j.ok?'Rebooting...':j.msg;toast(j.ok?'OTA OK':'Failed')}catch(e){p.textContent='Error';toast('Failed')}}
async function delPhoto(n){if(!confirm('Delete?'))return;await fetch('/api/photos',{method:'DELETE',headers:{'Content-Type':'application/json'},body:JSON.stringify({name:n})});loadPhotos()}
async function loadPhotos(){try{let r=await fetch('/api/photos');let l=await r.json();let g=document.getElementById('photos');g.innerHTML='';
l.forEach(n=>{let d=document.createElement('div');d.style.cssText='position:relative';
d.innerHTML='<img src="/api/photos/'+n+'" style="width:100%;aspect-ratio:4/3;object-fit:cover;border-radius:8px;cursor:pointer" onclick="window.open(this.src)"><button onclick="delPhoto(\''+n+'\')" style="position:absolute;top:4px;right:4px;background:rgba(255,55,95,0.8);color:#fff;border:none;border-radius:50%;width:20px;height:20px;font-size:11px;cursor:pointer">x</button>';
g.appendChild(d)})}catch(e){}}
async function refreshStatus(){try{let r=await fetch('/api/status');let s=await r.json();
document.getElementById('s-page').textContent=s.page||'-';
document.getElementById('s-wifi').textContent=s.wifi||'-';
document.getElementById('s-ai').textContent=s.ai||'-'}catch(e){}}
async function refreshFace(){try{let r=await fetch('/api/face');face=await r.json()}catch(e){}}
)rawhtml"
R"rawhtml(
function drawFace(){
X.fillStyle='#000';X.fillRect(0,0,320,240);
let cx=160,cy=120,em=face.emotion||0;
let eyeW=50,eyeH=58,pupilR=12,gazeMax=14;
let gx=face.gazeX*gazeMax,gy=face.gazeY*gazeMax;
if(em==1){eyeH=39}else if(em==2){eyeW=56;eyeH=64;pupilR=14}
else if(em==3){eyeH=62}else if(em==5){eyeW=54}
else if(em==6){eyeW=60;eyeH=68;pupilR=10}
else if(em==7){eyeH=19;pupilR=0}
else if(em==9){eyeH=29;pupilR=11}else if(em==10){eyeH=29;pupilR=10}
if(face.blinking){eyeH=3;pupilR=0}
let lx=cx-35,rx=cx+35,ey=cy-20;
// Eyes
X.fillStyle='#fff';
if(em==1&&!face.blinking){
  X.beginPath();X.moveTo(lx-18,ey);
  for(let i=-18;i<=18;i++){let y=-(i*i)/(36)+9;X.lineTo(lx+i,ey-y)}
  X.closePath();X.fill();
  X.beginPath();X.moveTo(rx-18,ey);
  for(let i=-18;i<=18;i++){let y=-(i*i)/(36)+9;X.lineTo(rx+i,ey-y)}
  X.closePath();X.fill();
}else{
  X.beginPath();X.ellipse(lx,ey,eyeW/2,eyeH/2,0,0,Math.PI*2);X.fill();
  X.beginPath();X.ellipse(rx,ey,eyeW/2,eyeH/2,0,0,Math.PI*2);X.fill();
}
// Pupils
if(pupilR>0){
  let px=gx,py=gy;
  let maxOff=eyeW/2-pupilR-2;
  let d=Math.sqrt(px*px+py*py);if(d>maxOff){px*=maxOff/d;py*=maxOff/d}
  X.fillStyle='#1a0a1a';
  X.beginPath();X.arc(lx+px,ey+py,pupilR+3,0,Math.PI*2);X.fill();
  X.beginPath();X.arc(rx+px,ey+py,pupilR+3,0,Math.PI*2);X.fill();
  X.fillStyle='#000';
  X.beginPath();X.arc(lx+px,ey+py,pupilR,0,Math.PI*2);X.fill();
  X.beginPath();X.arc(rx+px,ey+py,pupilR,0,Math.PI*2);X.fill();
  X.fillStyle='#fff';
  X.beginPath();X.arc(lx+px-3,ey+py-3,pupilR/3,0,Math.PI*2);X.fill();
  X.beginPath();X.arc(rx+px-3,ey+py-3,pupilR/3,0,Math.PI*2);X.fill();
  X.fillStyle='#ffd700';
  X.beginPath();X.arc(lx+px+2,ey+py+2,pupilR/5,0,Math.PI*2);X.fill();
  X.beginPath();X.arc(rx+px+2,ey+py+2,pupilR/5,0,Math.PI*2);X.fill();
}
// Eyebrows
let browY=cy-38,browW=44;
X.strokeStyle='#eee';X.lineWidth=3;X.lineCap='round';
let lbx=cx-35,rbx=cx+35;
if(em==1)browY-=5;else if(em==6){browY-=10;X.lineWidth=4}else if(em==7)browY+=8;
X.beginPath();X.moveTo(lbx-browW/2,browY);X.lineTo(lbx+browW/2,browY);X.stroke();
X.beginPath();X.moveTo(rbx-browW/2,browY);X.lineTo(rbx+browW/2,browY);X.stroke();
// Mouth
let my=cy+30;X.strokeStyle='#fff';X.fillStyle='#fff';X.lineWidth=2;
if(em==0||em==3||em==8){X.beginPath();X.moveTo(cx-18,my);X.lineTo(cx+18,my);X.stroke()}
else if(em==1){X.beginPath();X.moveTo(cx-14,my-4);X.quadraticCurveTo(cx,my+10,cx+14,my-4);X.stroke()}
else if(em==2){X.beginPath();X.arc(cx,my,7,0,Math.PI*2);X.stroke()}
else if(em==5||face.speaking){let h=face.mouthOpen*18||8;
  X.beginPath();X.roundRect(cx-9,my-h/2,18,h,4);X.fill()}
else if(em==6){X.beginPath();X.arc(cx,my,12,0,Math.PI*2);X.stroke();
  X.fillStyle='#000';X.beginPath();X.arc(cx,my,8,0,Math.PI*2);X.fill()}
else if(em==7){X.strokeStyle='#555';X.beginPath();
  for(let i=-12;i<=12;i++){X.lineTo(cx+i,my+Math.sin(i/3)*2)}X.stroke()}
else if(em==9){X.beginPath();X.moveTo(cx-8,my);X.quadraticCurveTo(cx,my+5,cx+8,my);X.stroke()}
else if(em==10){X.strokeStyle='#7f7';X.beginPath();
  let t=Date.now()/300;for(let i=-9;i<=9;i++){X.lineTo(cx+i,my+Math.sin(i/2.5+t)*3+3)}X.stroke()}
else{X.beginPath();X.moveTo(cx-18,my);X.lineTo(cx+18,my);X.stroke()}
// Cheeks
if(em==1||em==6){X.fillStyle='rgba(180,40,40,0.4)';X.beginPath();X.arc(cx-55,ey+30,10,0,Math.PI*2);X.fill();X.beginPath();X.arc(cx+55,ey+30,10,0,Math.PI*2);X.fill()}
else if(em==9){X.fillStyle='rgba(200,60,100,0.4)';X.beginPath();X.arc(cx-55,ey+30,12,0,Math.PI*2);X.fill();X.beginPath();X.arc(cx+55,ey+30,12,0,Math.PI*2);X.fill()}
// Particles
updateParticles(em);drawParticles();
}
)rawhtml"
R"rawhtml(
function updateParticles(em){
if(em==1||em==7||em==9||em==6){
  if(particles.length<8&&Math.random()<0.15){
    let p={x:160+Math.random()*120-60,y:120+Math.random()*80-40,
      vx:Math.random()*2-1,vy:-Math.random()*1.5-0.3,life:1,em:em};
    if(em==7){p.x=200+Math.random()*30;p.vy=-0.8-Math.random()*0.5;p.vx=0.3+Math.random()*0.3}
    particles.push(p)}}
for(let i=particles.length-1;i>=0;i--){
  let p=particles[i];p.x+=p.vx;p.y+=p.vy;p.life-=0.02;
  if(p.life<=0)particles.splice(i,1)}
}
function drawParticles(){
particles.forEach(p=>{
  let a=p.life;X.globalAlpha=a;
  if(p.em==1){X.fillStyle='#ffd700';drawStar(p.x,p.y,3+a*3)}
  else if(p.em==7){X.fillStyle='#7bf';X.font='bold '+(10+a*6)+'px sans-serif';X.fillText('Z',p.x,p.y)}
  else if(p.em==9){X.fillStyle='#f6f';drawHeart(p.x,p.y,4+a*3)}
  else if(p.em==6){X.fillStyle='#fff';drawStar(p.x,p.y,2+a*2)}
  X.globalAlpha=1})}
function drawStar(x,y,r){X.beginPath();for(let i=0;i<5;i++){let a=i*Math.PI*2/5-Math.PI/2;
X.lineTo(x+Math.cos(a)*r,y+Math.sin(a)*r);let b=a+Math.PI/5;X.lineTo(x+Math.cos(b)*r*0.4,y+Math.sin(b)*r*0.4)}X.closePath();X.fill()}
function drawHeart(x,y,s){X.beginPath();X.moveTo(x,y+s*0.4);
X.bezierCurveTo(x-s,y-s*0.4,x-s*0.5,y-s,x,y-s*0.4);
X.bezierCurveTo(x+s*0.5,y-s,x+s,y-s*0.4,x,y+s*0.4);X.fill()}

function renderLoop(){drawFace();requestAnimationFrame(renderLoop)}
renderLoop();
refreshStatus();loadPhotos();refreshFace();
setInterval(refreshStatus,5000);setInterval(loadPhotos,15000);setInterval(refreshFace,100);
</script>
</body>
</html>
)rawhtml"
;
