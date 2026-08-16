#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>

const char* ssid = "ESP32-igra";
const char* password ="12345678";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
/* ===== подсказки =====
// Состояние игры: 'idle' = меню, 'play' = играем, 'over' = проигрыш
  // счёт (score) — сколько очков набрано
  // номер текущей волны waveNum
  // сколько раз враги добрались до базы damages
  // максимум ударов по базе до проигрыша MAX_DMG
  // массив всех врагов на экране enemies
  // массив всех снарядов в полёте bullets
  // массив частиц ба-бах explosions 
  // массив взрывов (анимация) particles
  // анимационный цикл animId
  // счётчик паузы между волнами waveTimer
  // кд между появлением врагов spawnTimer
  // сколько врагов ещё нужно заспавнить spawnLeft
  // кулдаун основной турели shootCD (0 - это стреляем!)
  // таймер показа сообщения на экране msgTimer
  // цвет последнего сообщения msgCol
  // уровни прокачки: {skill, reload, damage, radar, tech, ally}
  // skill=0 новобранец, skill=4 легенда и т.д. - это все upg
  // ресурсы игрока: {metal=металл, fuel=топливо, stars=звёзды} res
  CFG.MAX_WAVE_SIZE// максимум врагов за волну
  CFG.WAVE_BASE// база для расчёта размера волны
  CFG.BASE_HP// жизни базы
  CFG.START_METAL// стартовый металл
  CFG.START_FUEL// стартовое топливо
  CFG.WAVE_BONUS_M/MK // бонус металла за волну
  CFG.WAVE_BONUS_F/FK // бонус топлива за волну
  CFG.ENEMY_SPD_SCALE // насколько быстрее враги с каждой волной
  CFG.ENEMY_HP_SCALE  // насколько больше HP врагов с каждой волной
  CFG.SPAWN_BASE/STEP/MIN // интервал появления врагов
  CFG.WAVE_PAUSE// пауза между волнами в кадрах
  CFG.SKILL_LEAD// коэффициент упреждения по уровню навыка [0..4]
  CFG.SKILL_SPREAD// разброс по уровню навыка [0..4]
  CFG.RELOAD_FRAMES// время перезарядки по уровню [0..4]
  CFG.DAMAGE_VAL// урон по уровню [0..3]
  CFG.RADAR_RANGE// дальность радара по уровню [0..3]
  CFG.TECH_NAMES// названия техники [0..3]
  CFG.BULLET_SPD// скорость снаряда по технике [0..3]
  CFG.ALLY_RANGE// радар союзника по уровню [0..1]
  CFG.ALLY_RELOAD// перезарядка союзника [0..1]
  CFG.ALLY_SPD// скорость снаряда союзника [0..1]
  CFG.ALLY_LEAD// упреждение союзника [0..1]
  const CITIES// массив из 20 городов
  let currentCity  // текущий город — меняется каждые 5 волн

  // Поля каждого города:
  city.name// название ("Киев", "Москва"...)
  city.river// название реки ("Днепр", "Волга"...)
  city.riverX// где река по X (0.0=левый край, 1.0=правый)
  city.riverBend// 12 чисел — контрольные точки кривой Безье для реки
  city.bgColor// цвет земли/фона
  city.blockColor// цвет кварталов
  city.streetColor // цвет улиц
  city.districts// массив названий районов (не используется сейчас)
  e.x, e.y// текущая позиция
  e.vx, e.vy// скорость по X и Y (пикселей за кадр)
  e.angle// угол поворота силуэта (atan2 от скорости)
  e.hp// текущее здоровье
  e.maxHp// максимальное здоровье
  e.w, e.h// размер (ширина, высота) силуэта
  e.spd// базовая скорость (до умножения на волну)
  e.color// цвет силуэта
  e.shape// название функции отрисовки ('jet', 'drone' и т.д.)
  e.era// эпоха (0=ВОВ, 1=холодная война, 2=современность, 3=∞)
  e.reward// награда за уничтожение {m=металл, f=топливо, s=звёзды}
  b.x, b.y// текущая позиция
  b.vx, b.vy// скорость полёта
  b.life// оставшееся время жизни в кадрах (при 0 — удаляется)
  b.fromAlly// true если снаряд от союзника (не используется для логики)
  ex.x, ex.y// позиция
  ex.color// цвет
  ex.maxR// максимальный радиус
  ex.life// оставшееся время жизни
  ex.maxLife// начальное время жизни (для расчёта прозрачности)
*/
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0">
<title>ПВО</title>
<style>
* { margin:0; padding:0; box-sizing:border-box; }
body { background:#0d1208; color:#e8f0e0; font-family:'Segoe UI',sans-serif;
       display:flex; flex-direction:column; align-items:center; min-height:100vh; gap:0; }
h1 { margin:8px 0 4px; font-size:1.1rem; color:#6fc; letter-spacing:3px; font-weight:400; opacity:0.9; }
#hud { display:grid; grid-template-columns:1fr 1fr; gap:4px 12px;
       margin:0 0 4px; font-size:0.78rem; width:100%; max-width:480px; padding:0 6px; }
#score { color:#f0b840; font-weight:600; }
#wave  { color:#6fc; text-align:right; }
#era   { color:#88b8ff; font-size:0.72rem; }
#res   { color:#88ccaa; font-size:0.72rem; text-align:right; }
canvas { border:1px solid #2a4a22; border-radius:6px; display:block; touch-action:none; max-width:100%; }
#shopPanel { margin-top:6px; width:100%; max-width:480px; display:none; background:#0f1a0a;
             border:1px solid #2a4a22; border-radius:8px; padding:10px; }
#shopPanel h2 { color:#6fc; margin-bottom:8px; font-size:0.85rem; font-weight:400; letter-spacing:1px; }
.shopRow { display:grid; grid-template-columns:1fr 1fr; gap:5px; margin-bottom:5px; }
.shopRow.single { grid-template-columns:1fr; }
.shopBtn { padding:6px 8px; border-radius:4px; border:1px solid #2a4a22;
           background:#0d1a0a; color:#6fc; font-size:0.7rem; cursor:pointer; text-align:left;
           line-height:1.4; transition:border-color 0.15s; }
.shopBtn:hover:not(:disabled) { border-color:#4a8a44; }
.shopBtn:disabled { border-color:#1a2a16; color:#334; cursor:default; }
.shopBtn.maxed { border-color:#f0b840; color:#f0b840; }
.shopBtn span { display:block; color:#556650; font-size:0.65rem; margin-top:2px; }
#msg { font-size:0.8rem; color:#6fc; min-height:16px; text-align:center;
       padding:2px 0; width:100%; max-width:480px; }
#ui { margin:5px 0; display:flex; gap:6px; flex-wrap:wrap; justify-content:center; width:100%; max-width:480px; padding:0 6px; }
button.main { padding:6px 14px; border-radius:4px; border:1px solid #2a4a22; cursor:pointer;
              font-size:0.82rem; background:#0d1a0a; color:#6fc; font-weight:400;
              letter-spacing:0.5px; transition:border-color 0.15s, background 0.15s; }
button.main:hover { background:#122010; border-color:#4a8a44; }
#lb { margin-top:8px; width:100%; max-width:480px; display:none; padding:0 6px; }
#lb h2 { color:#f0b840; margin-bottom:4px; font-size:0.85rem; font-weight:400; }
table { width:100%; border-collapse:collapse; font-size:0.75rem; }
th,td { padding:4px 6px; border-bottom:1px solid #1a2a16; text-align:center; }
th { color:#6fc; font-weight:400; }
#nameInput { padding:6px 10px; border-radius:4px; border:1px solid #2a4a22;
             background:#0d1a0a; color:#e8f0e0; font-size:0.82rem; width:120px;
             outline:none; }
#nameInput:focus { border-color:#4a8a44; }
#shopRes { font-size:0.68rem; color:#445544; margin-top:6px; }
</style>
</head>
<body>
<h1>ПВО</h1>
<div id="hud">
  <div id="score">💀 0</div>
  <div id="wave">волна 1</div>
  <div id="era">ВОВ 1941</div>
  <div id="res">🔩50 ⛽30</div>
</div>
<canvas id="c"></canvas>

<div id="msg"></div>
<div id="ui">
  <input id="nameInput" placeholder="позывной" maxlength="16" value="Командир1">
  <button class="main" onclick="startGame()"> старт</button>
  <button class="main" onclick="toggleShop()">прокачка</button>
  <button class="main" onclick="showLb()"> рекорды</button>
</div>
<div id="shopPanel">
  <h2>центр прокачки</h2>
  <div class="shopRow">
    <button class="shopBtn" id="btnSkill"  onclick="upgrade('skill')" >🎯 навык бойца<br><span id="sklInfo"></span></button>
    <button class="shopBtn" id="btnReload" onclick="upgrade('reload')">⚡ перезарядка<br><span id="relInfo"></span></button>
  </div>
  <div class="shopRow">
    <button class="shopBtn" id="btnDamage" onclick="upgrade('damage')">💥 урон<br><span id="dmgInfo"></span></button>
    <button class="shopBtn" id="btnRadar"  onclick="upgrade('radar')" >📡 радар<br><span id="radInfo"></span></button>
  </div>
  <div class="shopRow">
    <button class="shopBtn" id="btnTech"   onclick="upgrade('tech')"  >🚀 техника<br><span id="tecInfo"></span></button>
    <button class="shopBtn" id="btnAlly"   onclick="upgrade('ally')"  >🤝 союзник<br><span id="alyInfo"></span></button>
  </div>
  <div id="shopRes"></div>
</div>
<div id="lb">
  <h2>🏆 Рекорды</h2>
  <table><thead><tr><th>#</th><th>Игрок</th><th>Счёт</th><th>Волна</th></tr></thead>
  <tbody id="lbBody"></tbody></table>
</div>

<script>
const canvas = document.getElementById('c');
const ctx = canvas.getContext('2d');
const maxW = Math.min(window.innerWidth - 12, 480);
const ratio = 500 / 360;
canvas.width  = maxW;
canvas.height = Math.round(maxW * ratio);
const W = canvas.width, H = canvas.height;

let sock;
function connectWS() {
  sock = new WebSocket('ws://' + location.hostname + '/ws');
  sock.onmessage = e => { try { const d=JSON.parse(e.data); if(d.type==='leaderboard') renderLb(d.data); } catch(x){} };
  sock.onclose = () => setTimeout(connectWS, 2000);
}
connectWS();

/* ===== ПРОКАЧКА =====*/
const UPGRADES = {
  skill:{ name:'Навык бойца',costs:[{m:10,f:0},{m:20,f:5},{m:40,f:15},{m:80,f:30}],  desc:['новобранец — стреляет куда был враг','боец — 25% упреждения','снайпер — 55% упреждения','ас — 80% упреждения','легенда — точное наведение'] },
  reload:{ name:'Перезарядка',costs:[{m:8,f:3},{m:18,f:8},{m:35,f:18},{m:70,f:35}],  desc:['медленно (50 кадров)','быстрее (38)','быстро (28)','очень быстро (20)','авто (13)'] },
  damage:{ name:'Урон снаряда',costs:[{m:12,f:5},{m:30,f:12},{m:65,f:28}],  desc:['слабый — 1 ед.','средний — 1.5 ед.','сильный — 2.5 ед.','мощный — 4 ед.'] },
  radar:{ name:'Радар',costs:[{m:8,f:8},{m:18,f:18},{m:45,f:40}], desc:['слепой — 150px','короткий — 210px','дальний — 290px','максимум — весь экран'] },
  tech:{ name:'Техника ПВО', costs:[{m:30,f:20},{m:70,f:40},{m:140,f:80}], desc:['29К / ГАЗ-АА — пулемёт','ЗСУ-23-4 Шилка — 4 ствола','С-75 — ракета на рампе','С-300 — 3 ракеты залпом'] },
  ally:{ name:'Союзник ПВО', costs:[{m:80,f:60},{m:160,f:100}], desc:['нет','С-75 авто — средний радар','С-300 авто — дальний радар'] }
};
let upg = { skill:0, reload:0, damage:0, radar:0, tech:0, ally:0 };
let res  = { metal:0, fuel:0, stars:0 };

function canAfford(c) { return res.metal>=c.m && res.fuel>=c.f; }
function upgrade(key) {
  const u=UPGRADES[key], lvl=upg[key];
  if (lvl>=u.costs.length) return;
  const c=u.costs[lvl];
  if (!canAfford(c)) { showMsg('Не хватает ресурсов!','#f84'); return; }
  res.metal-=c.m; res.fuel-=c.f; upg[key]++;
  showMsg(u.name+' → '+u.desc[upg[key]],'#4f8');
  updateShop(); updateResUI();
}
function updateShop() {
  const cfg = [
    ['skill','btnSkill','sklInfo'],['reload','btnReload','relInfo'],
    ['damage','btnDamage','dmgInfo'],['radar','btnRadar','radInfo'],
    ['tech','btnTech','tecInfo'],['ally','btnAlly','alyInfo']
  ];
  cfg.forEach(([k,bid,iid]) => {
    const u=UPGRADES[k], lvl=upg[k], btn=document.getElementById(bid), inf=document.getElementById(iid);
    if (lvl>=u.costs.length) { btn.classList.add('maxed'); btn.disabled=false; inf.textContent='МАКС: '+u.desc[lvl]; }
    else { btn.classList.remove('maxed'); const c=u.costs[lvl]; inf.textContent=u.desc[lvl]+' → '+u.desc[lvl+1]+' 🔩'+c.m+' ⛽'+c.f; btn.disabled=!canAfford(c); }
  });
  document.getElementById('shopRes').textContent='🔩'+res.metal+' ⛽'+res.fuel+' ⭐'+res.stars;
}
function toggleShop() { const p=document.getElementById('shopPanel'); p.style.display=p.style.display==='block'?'none':'block'; updateShop(); }

/* ===== ЭПОХИ =====
// Волна 1-5: ВОВ, 6-12: Холодная война, 13-20: Современность, 21+: Бесконечность (смешанные, сложнее)
*/
function getEra(w) {
  if (w<=5)  return 0;
  if (w<=12) return 1;
  if (w<=20) return 2;
  return 3;
}
const ERA_NAMES = ['ВОВ 1941-45','Холодная война','Современность','Бесконечная война'];

/* ===== ТИПЫ ВРАГОВ по эпохам =====
// era 0: По-2(кукурузник), Ju-88, He-111
// era 1: МиГ-17, B-52, крылатая ракета
// era 2: Дрон Байрактар, Томагавк, Су-34
// era 3: всё + гиперзвук
*/
const ENEMY_DEFS = [
{ era:0, id:'po2',    name:'По-2',      color:'#8a6',  spd:0.9,  hp:1, w:34, h:17, reward:{m:8,f:0,s:1},   shape:'biplane'   },
{ era:0, id:'ju88',   name:'Ju-88',     color:'#a84',  spd:1.3,  hp:3, w:48, h:24, reward:{m:14,f:2,s:2},  shape:'bomber_ww2'},
{ era:0, id:'he111',  name:'He-111',    color:'#a96',  spd:1.1,  hp:4, w:54, h:20, reward:{m:18,f:2,s:2},  shape:'he111'     },
{ era:1, id:'mig17',  name:'МиГ-17',   color:'#48f',  spd:2.4,  hp:2, w:38, h:15, reward:{m:12,f:3,s:2},  shape:'jet'       },
{ era:1, id:'b52',    name:'B-52',      color:'#888',  spd:1.0,  hp:8, w:68, h:24, reward:{m:30,f:5,s:4},  shape:'b52'       },
{ era:1, id:'cr1',    name:'Крылатая',  color:'#f84',  spd:3.2,  hp:1, w:24, h:9,  reward:{m:8,f:5,s:2},   shape:'cruise'    },
{ era:2, id:'byr',    name:'Байрактар', color:'#aaf',  spd:2.0,  hp:2, w:40, h:14, reward:{m:10,f:8,s:2},  shape:'drone'     },
{ era:2, id:'tgk',    name:'Томагавк',  color:'#f44',  spd:3.8,  hp:1, w:24, h:9,  reward:{m:6,f:8,s:3},   shape:'cruise'    },
{ era:2, id:'su34',   name:'Су-34',     color:'#46a',  spd:3.0,  hp:3, w:40, h:17, reward:{m:15,f:5,s:3},  shape:'jet'       },
{ era:3, id:'hyp',    name:'Гиперзвук', color:'#f4f',  spd:5.5,  hp:2, w:27, h:10, reward:{m:20,f:15,s:5}, shape:'cruise'    },
{ era:3, id:'swarm',  name:'Рой дронов',color:'#aaf',  spd:2.5,  hp:1, w:17, h:10, reward:{m:5,f:4,s:2},   shape:'drone'     },
];

function pickEnemy(wave) {
  const era = getEra(wave);
  let pool = ENEMY_DEFS.filter(e => e.era <= era);
  // В поздних волнах больше быстрых
  if (era >= 2) pool = pool.filter(e => e.era >= 1);
  if (era >= 3) pool = ENEMY_DEFS; // всё подряд
  return pool[Math.floor(Math.random()*pool.length)];
}

/* ===== ПАРАМЕТРЫ ИЗ ПРОКАЧКИ =====*/
/* skill: 0=новобранец(0% упреждения,разброс 30px), 1=боец(25%,20px), 2=снайпер(55%,12px), 3=ас(80%,5px), 4=легенда(100%,1px)*/
const CFG = {
  // Сколько врагов в волне: 4+волна, максимум MAX_WAVE_SIZE
  MAX_WAVE_SIZE: 18,
  WAVE_BASE: 3, // waveSize = WAVE_BASE + wave
  // Здоровье базы
  BASE_HP: 12,
  // Стартовые ресурсы при каждом старте
  START_METAL: 50,
  START_FUEL:  30,
  // Бонус ресурсов за волну: metal += WAVE_BONUS_M + wave*WAVE_BONUS_MK
  WAVE_BONUS_M: 20,
  WAVE_BONUS_MK: 6,
  WAVE_BONUS_F:8,
  WAVE_BONUS_FK: 3,
  // Нарастание скорости врагов с волной: spd * (1 + wave * ENEMY_SPD_SCALE)
  ENEMY_SPD_SCALE: 0.06,
  // Нарастание HP врагов с волной: ceil(base * (1 + wave * ENEMY_HP_SCALE))
  ENEMY_HP_SCALE: 0.12,
  // Задержка между спавнами: max(SPAWN_MIN, SPAWN_BASE - wave*SPAWN_STEP) кадров
  SPAWN_BASE: 75,
  SPAWN_STEP: 4,
  SPAWN_MIN:  18,
  // Пауза между волнами (кадры)
  WAVE_PAUSE: 110,
  // Навык бойца: коэффициент упреждения [ур0..ур4]
  SKILL_LEAD:   [0, 0.25, 0.55, 0.80, 1.0],
  // Навык бойца: разброс пикселей [ур0..ур4]
  SKILL_SPREAD: [32, 22, 13, 6, 1],
  // Перезарядка в кадрах [ур0..ур4]
  RELOAD_FRAMES: [55, 42, 30, 22, 14],
  // Урон снаряда [ур0..ур3]
  DAMAGE_VAL: [1, 1.5, 2.5, 4],
  // Дальность радара в пикселях [ур0..ур3] (W = весь экран)
  RADAR_RANGE: [140, 200, 280, 9999],
  // Название техники ПВО [ур0..ур3]
  TECH_NAMES: ['29К / ГАЗ-АА', 'ЗСУ-23-4', 'С-75', 'С-300'],
  // Скорость снаряда по технике [ур0..ур3]
  BULLET_SPD: [6, 8, 11, 15],
  // Союзник: радар, перезарядка, скорость по уровню [ур1, ур2]
  ALLY_RANGE:   [200, 320],
  ALLY_RELOAD:  [35, 22],
  ALLY_SPD:     [10, 13],
  ALLY_LEAD:    [0.6, 0.8],
};

function getLead()    { return CFG.SKILL_LEAD[upg.skill]; }
function getSpread()  { return CFG.SKILL_SPREAD[upg.skill]; }
function getReload()  { return CFG.RELOAD_FRAMES[upg.reload]; }
function getDmg()     { return CFG.DAMAGE_VAL[upg.damage]; }
function getRadar()   { return Math.min(CFG.RADAR_RANGE[upg.radar], W); }
function getTechName(){ return CFG.TECH_NAMES[upg.tech]; }
function getBspd()    { return CFG.BULLET_SPD[upg.tech]; }
function getEnemyHP(base,wave){ return Math.ceil(base*(1+wave*CFG.ENEMY_HP_SCALE)); }

let state='idle', score=0, waveNum=1, damages=0;
const MAX_DMG = CFG.BASE_HP;
let enemies=[], bullets=[], explosions=[], particles=[];
let animId, waveTimer=0, spawnTimer=0, spawnLeft=0, shootCD=0, msgTimer=0;

function startGame() {
  /* стартовые ресуры */
  state='play'; score=0; waveNum=1; damages=0;
  pickCity(1);
  upg = { skill:0, reload:0, damage:0, radar:0, tech:0, ally:0 };
  res.metal=CFG.START_METAL; res.fuel=CFG.START_FUEL; res.stars=0;
  enemies=[]; bullets=[]; explosions=[]; particles=[];
  waveTimer=0; spawnTimer=0; spawnLeft=waveSize(1); shootCD=0;
  ally=null;
  document.getElementById('lb').style.display='none';
  document.getElementById('shopPanel').style.display='none';
  updateHUD();
  cancelAnimationFrame(animId);
  animId=requestAnimationFrame(loop);
}
function waveSize(w) { return Math.min(CFG.WAVE_BASE+w, CFG.MAX_WAVE_SIZE); } /* максимум врагов за волну */

function updateHUD() {
  document.getElementById('score').textContent='Сбито: '+score;
  document.getElementById('wave').textContent='Волна: '+waveNum;
  document.getElementById('era').textContent=ERA_NAMES[getEra(waveNum)];
  updateResUI();
}
function updateResUI() { document.getElementById('res').textContent='🔩'+res.metal+' ⛽'+res.fuel+' ⭐'+res.stars; }

let msgCol='#4f8';
function showMsg(t,c='#4f8') { document.getElementById('msg').textContent=t; document.getElementById('msg').style.color=c; msgCol=c; msgTimer=140; }

/*===== СТРЕЛЬБА С УПРЕЖДЕНИЕМ =====*/
const TURRET={x:W/2, y:H-42}; // Основная турель игрока 
const TURRET2 = { x: W*0.22, y: H*0.72 }; // вторая турель
let ally = null; // { active, lvl, shootCD }
function computeAimPoint(e) {
  const lead = getLead();
  const spd  = getBspd();
  /* Время полёта до текущей позиции*/
  const dist = Math.hypot(e.x-TURRET.x, e.y-TURRET.y);
  const tof  = dist / spd; // кадров до цели
  /*упреждения*/
  const predX = e.x + e.vx * tof * lead;
  const predY = e.y + e.vy * tof * lead;
  return { x: predX, y: predY };
}

function shoot(tx, ty) {
  if (shootCD>0) return;
  shootCD = getReload();
  const spread = getSpread();
  const dx = tx - TURRET.x + (Math.random()-0.5)*spread;
  const dy = ty - TURRET.y + (Math.random()-0.5)*spread;
  const len = Math.hypot(dx,dy)||1;
  const spd = getBspd();
  const bul = { x:TURRET.x, y:TURRET.y, vx:dx/len*spd, vy:dy/len*spd, life:90 };
  bullets.push(bul);
  /* С-300: 3 ракеты веером*/
  if (upg.tech>=3) {
    [-0.12, 0.12].forEach(da => {
      bullets.push({ x:TURRET.x, y:TURRET.y,
        vx: dx/len*spd*Math.cos(da)-dy/len*spd*Math.sin(da),
        vy: dx/len*spd*Math.sin(da)+dy/len*spd*Math.cos(da), life:90 });
    });
  }
  addParticles(TURRET.x, TURRET.y, '#ff8', 5, 2);
}
function updateAlly() {
  if (upg.ally===0) return;
  if (!ally) ally={shootCD:0};
  ally.shootCD=Math.max(0,ally.shootCD-1);
  if (ally.shootCD>0) return;
  const ai = upg.ally-1;
  const range=CFG.ALLY_RANGE[ai];
  const reloadTime=CFG.ALLY_RELOAD[ai];
  const spd=CFG.ALLY_SPD[ai];
  let best=null, bd=range;
  enemies.forEach(e=>{const d=Math.hypot(e.x-TURRET2.x,e.y-TURRET2.y);if(d<bd){bd=d;best=e;}});
  if (!best) return;
  ally.shootCD=reloadTime;
  const dist=Math.hypot(best.x-TURRET2.x,best.y-TURRET2.y);
  const tof=dist/spd;
  const tx=best.x+best.vx*tof*CFG.ALLY_LEAD[ai];
  const ty=best.y+best.vy*tof*CFG.ALLY_LEAD[ai];
  const dx=tx-TURRET2.x, dy=ty-TURRET2.y, len=Math.hypot(dx,dy)||1;
  bullets.push({x:TURRET2.x,y:TURRET2.y,vx:dx/len*spd,vy:dy/len*spd,life:90,fromAlly:true});
  addParticles(TURRET2.x,TURRET2.y,'#4ff',4,2);
}
function autoAim() {
  if (shootCD>0) return;
  const range=getRadar();
  let best=null, bd=range;
  enemies.forEach(e => { const d=Math.hypot(e.x-TURRET.x,e.y-TURRET.y); if(d<bd){bd=d;best=e;} });
  if (!best) return;
  const aim=computeAimPoint(best);
  shoot(aim.x, aim.y);
}

/* ===== ФИЗИКА =====*/
function updateBullets() {
  shootCD=Math.max(0,shootCD-1);
  bullets=bullets.filter(b=>{
    b.x+=b.vx; b.y+=b.vy; b.life--;
    if (b.life<=0||b.x<-10||b.x>W+10||b.y<-10||b.y>H+10) return false;
    for (let i=enemies.length-1;i>=0;i--) {
      const e=enemies[i];
      if (Math.hypot(b.x-e.x,b.y-e.y)<e.w/2+5) {
        e.hp-=getDmg();
        addExplosion(b.x,b.y,'#fa4',6);
        if (e.hp<=0) killEnemy(i);
        return false;
      }
    }
    return true;
  });
}

function killEnemy(idx) {
  const e=enemies[idx];
  addExplosion(e.x,e.y,e.id==='hyp'?'#f4f':e.era<=0?'#f84':'#fa4',20);
  addParticles(e.x,e.y,'#fa4',12,3);
  res.metal+=e.reward.m; res.fuel+=e.reward.f; res.stars+=e.reward.s;
  score+=e.reward.s*10+waveNum*5;
  enemies.splice(idx,1);
  updateHUD();
}

function updateEnemies() {
  enemies=enemies.filter(e=>{
    e.x+=e.vx; e.y+=e.vy;
    e.angle=Math.atan2(e.vy,e.vx);
    if (e.y>H-25||Math.hypot(e.x-TURRET.x,e.y-TURRET.y)<30) {
      addExplosion(TURRET.x,TURRET.y,'#f44',25);
      addParticles(TURRET.x,TURRET.y,'#f44',10,4);
      damages++;
      showMsg('База атакована! HP:'+(MAX_DMG-damages),'#f44');
      if (damages>=MAX_DMG) { endGame(); return false; }
      return false;
    }
    return !(e.x<-60||e.x>W+60||e.y<-60);
  });
}

function addExplosion(x,y,color,r) { explosions.push({x,y,color,r,maxR:r,life:22,maxLife:22}); }
function updateExplosions() { explosions=explosions.filter(ex=>{ex.life--;return ex.life>0;}); }

function addParticles(x,y,color,n,spd) {
  for (let i=0;i<n;i++) {
    const a=Math.random()*Math.PI*2, s=Math.random()*spd+1;
    particles.push({x,y,vx:Math.cos(a)*s,vy:Math.sin(a)*s,color,life:18,maxLife:18});
  }
}
function updateParticles() {
  particles=particles.filter(p=>{p.x+=p.vx;p.y+=p.vy;p.vy+=0.1;p.life--;return p.life>0;});
}

/* волны противников*/
function updateWaves() {
  if (spawnLeft>0) {
    spawnTimer--;
    if (spawnTimer<=0) {
      spawnEnemy();
      spawnLeft--;
      spawnTimer=Math.max(CFG.SPAWN_MIN, CFG.SPAWN_BASE-waveNum*CFG.SPAWN_STEP);
    }
  } else if (enemies.length===0) {
    waveTimer++;
    if (waveTimer>CFG.WAVE_PAUSE) {
      waveNum++;
      spawnLeft=waveSize(waveNum);
      spawnTimer=55;
      waveTimer=0;
      const bonus_m=CFG.WAVE_BONUS_M+waveNum*CFG.WAVE_BONUS_MK;
      const bonus_f=CFG.WAVE_BONUS_F+waveNum*CFG.WAVE_BONUS_FK;
      res.metal+=bonus_m; res.fuel+=bonus_f;
      pickCity(waveNum);
      if (waveNum%5===1) showMsg('Переходим в '+currentCity.name+'! Волна '+waveNum,'#adf');
      else showMsg('Волна '+waveNum+'! Бонус: 🔩'+bonus_m+' ⛽'+bonus_f,'#4f8');
      updateHUD();
    }
  }
}

function spawnEnemy() {
  const t=pickEnemy(waveNum);
  const side=Math.floor(Math.random()*3);
  let x,y,vx,vy;
  const spd=t.spd*(1+waveNum*CFG.ENEMY_SPD_SCALE);
  if (side===0) { x=Math.random()*W; y=-25; vx=(Math.random()-0.5)*spd*0.6; vy=spd; }
  else if (side===1) { x=W+25; y=30+Math.random()*H*0.6; vx=-spd; vy=(Math.random()-0.2)*spd*0.4; }
  else { x=-25; y=30+Math.random()*H*0.6; vx=spd; vy=(Math.random()-0.2)*spd*0.4; }
  enemies.push({...t, x, y, vx, vy, angle:Math.atan2(vy,vx), hp:getEnemyHP(t.hp,waveNum), maxHp:getEnemyHP(t.hp,waveNum)});
}

/* отрисовка лохов*/

function drawShape_biplane(ctx, w, h) {
  // По-2: бипланные крылья
  ctx.fillStyle='currentColor';
  // Фюзеляж
  ctx.fillRect(-w/2,-h/5,w,h*0.4);
  // Верхнее крыло
  ctx.fillRect(-w/2,-h*0.9,w,h*0.28);
  // Нижнее крыло
  ctx.fillRect(-w/2, h*0.18,w,h*0.22);
  // Стойки
  ctx.fillRect(-w/4,-h*0.65,h*0.15,h*0.8);
  ctx.fillRect( w/4-h*0.15,-h*0.65,h*0.15,h*0.8);
  // Хвост
  ctx.fillRect(w*0.3,-h*0.6,w*0.2,h*0.5);
}

function drawShape_bomber_ww2(ctx, w, h) {
  // Ju-88: широкий фюзеляж, два мотора
  ctx.fillRect(-w/2,-h/3,w,h*0.65);
  // Крылья
  ctx.fillRect(-w*0.55,-h*0.15,w*1.1,h*0.28);
  // Гондолы двигателей
  ctx.fillRect(-w*0.35,-h*0.4,w*0.18,h*0.25);
  ctx.fillRect( w*0.17,-h*0.4,w*0.18,h*0.25);
  // Хвост
  ctx.fillRect(w*0.28,-h*0.7,w*0.24,h*0.36);
  ctx.fillRect(w*0.28, h*0.16,w*0.24,h*0.28);
}

function drawShape_he111(ctx, w, h) {
  // He-111: эллиптические крылья
  ctx.beginPath();
  ctx.ellipse(0,0,w/2,h/3,0,0,Math.PI*2); ctx.fill();
  // Крылья эллипс
  ctx.beginPath();
  ctx.ellipse(0,-h*0.05,w*0.55,h*0.16,0,0,Math.PI*2); ctx.fill();
  // Двигатели
  ctx.beginPath(); ctx.ellipse(-w*0.25,-h*0.2,h*0.13,h*0.22,0,0,Math.PI*2); ctx.fill();
  ctx.beginPath(); ctx.ellipse( w*0.25,-h*0.2,h*0.13,h*0.22,0,0,Math.PI*2); ctx.fill();
}

function drawShape_jet(ctx, w, h) {
  // МиГ-17/Су-34: стреловидные крылья
  // Фюзеляж
  ctx.beginPath();
  ctx.moveTo(w/2,0); ctx.lineTo(-w/2,-h/4); ctx.lineTo(-w*0.3,0); ctx.lineTo(-w/2,h/4); ctx.closePath(); ctx.fill();
  // Крыло стреловидное
  ctx.beginPath();
  ctx.moveTo(w*0.05,-h*0.1); ctx.lineTo(-w*0.4,-h*0.8);
  ctx.lineTo(-w*0.5,-h*0.65); ctx.lineTo(-w*0.05,0); ctx.closePath(); ctx.fill();
  ctx.beginPath();
  ctx.moveTo(w*0.05,h*0.1); ctx.lineTo(-w*0.4,h*0.8);
  ctx.lineTo(-w*0.5,h*0.65); ctx.lineTo(-w*0.05,0); ctx.closePath(); ctx.fill();
  // Хвостовое оперение
  ctx.fillRect(-w/2,-h*0.5,w*0.22,h*0.22);
  ctx.fillRect(-w/2, h*0.28,w*0.22,h*0.22);
}

function drawShape_b52(ctx, w, h) {
  // B-52: 8 двигателей под стреловидным крылом
  ctx.fillRect(-w/2,-h/4,w,h*0.5);
  // Стреловидное крыло
  ctx.beginPath();
  ctx.moveTo(w*0.1,-h*0.25); ctx.lineTo(-w*0.45,-h*1.1);
  ctx.lineTo(-w*0.5,-h*0.9); ctx.lineTo(w*0.0,0); ctx.closePath(); ctx.fill();
  ctx.beginPath();
  ctx.moveTo(w*0.1,h*0.25); ctx.lineTo(-w*0.45,h*1.1);
  ctx.lineTo(-w*0.5,h*0.9); ctx.lineTo(w*0.0,0); ctx.closePath(); ctx.fill();
  // Двигатели (4 пары)
  [-0.38,-0.25,-0.15,-0.05].forEach(ox => {
    ctx.fillRect(ox*w-h*0.08,-h*1.05,h*0.16,h*0.28);
    ctx.fillRect(ox*w-h*0.08, h*0.77,h*0.16,h*0.28);
  });
  // Хвост высокий
  ctx.fillRect(w*0.28,-h*1.0,w*0.22,h*0.45);
}

function drawShape_cruise(ctx, w, h) {
  // Крылатая ракета / Томагавк: вытянутый корпус + маленькое крыло
  ctx.beginPath();
  ctx.moveTo(w/2,0); ctx.lineTo(w*0.2,-h/3); ctx.lineTo(-w/2,-h/3);
  ctx.lineTo(-w/2,h/3); ctx.lineTo(w*0.2,h/3); ctx.closePath(); ctx.fill();
  // Крылышки
  ctx.fillRect(-w*0.1,-h*0.9,w*0.35,h*0.28);
  ctx.fillRect(-w*0.1, h*0.62,w*0.35,h*0.28);
  // Хвостовые рули
  ctx.fillRect(-w*0.45,-h*0.7,w*0.15,h*0.18);
  ctx.fillRect(-w*0.45, h*0.52,w*0.15,h*0.18);
}

function drawShape_drone(ctx, w, h) {
  // Байрактар/дрон: перевёрнутое V-крыло, толкающий винт сзади
  // Фюзеляж
  ctx.fillRect(-w*0.38,-h*0.2,w*0.76,h*0.4);
  // V-образное крыло
  ctx.beginPath();
  ctx.moveTo(-w*0.05,-h*0.18); ctx.lineTo(-w*0.55,-h*0.85);
  ctx.lineTo(-w*0.45,-h*0.95); ctx.lineTo(w*0.0,-h*0.28); ctx.closePath(); ctx.fill();
  ctx.beginPath();
  ctx.moveTo(-w*0.05,h*0.18); ctx.lineTo(-w*0.55,h*0.85);
  ctx.lineTo(-w*0.45,h*0.95); ctx.lineTo(w*0.0,h*0.28); ctx.closePath(); ctx.fill();
  // Хвостовые балки
  ctx.fillRect(-w*0.5,-h*0.08,w*0.15,h*0.16);
  // Толкающий винт (круг)
  ctx.beginPath(); ctx.arc(-w*0.42,0,h*0.28,0,Math.PI*2); ctx.strokeStyle=ctx.fillStyle; ctx.lineWidth=1.5; ctx.stroke();
}

const SHAPE_FNS = {
  biplane: drawShape_biplane,
  bomber_ww2: drawShape_bomber_ww2,
  he111: drawShape_he111,
  jet: drawShape_jet,
  b52: drawShape_b52,
  cruise: drawShape_cruise,
  drone: drawShape_drone,
};

function drawEnemy(e) {
  ctx.save();
  ctx.translate(e.x, e.y);
  ctx.rotate(e.angle);
  ctx.fillStyle = e.color;
  const fn = SHAPE_FNS[e.shape];
  if (fn) fn(ctx, e.w, e.h);
  else { ctx.fillRect(-e.w/2,-e.h/2,e.w,e.h); }
  ctx.restore();

  // HP бар
  if (e.maxHp>1) {
    const bw=e.w*1.5;
    ctx.fillStyle='#300'; ctx.fillRect(e.x-bw/2,e.y-e.h-12,bw,4);
    ctx.fillStyle='#f44'; ctx.fillRect(e.x-bw/2,e.y-e.h-12,bw*(Math.max(0,e.hp)/e.maxHp),4);
  }
  // Метка
  ctx.fillStyle='rgba(255,160,80,0.85)'; ctx.font='8px Arial'; ctx.textAlign='center';
  ctx.fillText(e.name, e.x, e.y-e.h-14);
  ctx.textAlign='left';
}

// ===== ОТРИСОВКА ПВО =====
function drawAlly() {
  if (upg.ally===0) return;
  const tx=TURRET2.x, ty=TURRET2.y;
  ctx.fillStyle='#1a3a4a';
  ctx.fillRect(tx-18,ty-5,36,11);
  ctx.fillStyle='#111';
  [-10,10].forEach(ox=>{ctx.beginPath();ctx.arc(tx+ox,ty+6,4,0,Math.PI*2);ctx.fill();});
  let ang=-Math.PI/2;
  if (enemies.length>0) {
    let best=enemies[0],bd=Infinity;
    enemies.forEach(e=>{const d=Math.hypot(e.x-tx,e.y-ty);if(d<bd){bd=d;best=e;}});
    ang=Math.atan2(best.y-ty,best.x-tx);
  }
  ctx.save();ctx.translate(tx,ty-3);ctx.rotate(ang);
  ctx.fillStyle='#4ff';ctx.fillRect(0,-2,upg.ally>=2?18:13,4);
  ctx.restore();
  const range=upg.ally>=2?320:200;
  ctx.strokeStyle='rgba(0,220,255,0.1)';ctx.lineWidth=1;
  ctx.beginPath();ctx.arc(tx,ty,range,0,Math.PI*2);ctx.stroke();
  ctx.fillStyle='rgba(0,200,220,0.5)';ctx.font='7px Arial';ctx.textAlign='center';
  ctx.fillText(upg.ally>=2?'С-300':'С-75',tx,ty+17);
  ctx.textAlign='left';
}
function drawPVO() {
  const tx=TURRET.x, ty=TURRET.y;

  if (upg.tech===0) {
    // 29К / ГАЗ-АА: грузовик + зенитный пулемёт
    // Кузов ГАЗ-АА
    ctx.fillStyle='#3a4a2a';
    ctx.fillRect(tx-22,ty-8,44,16);
    ctx.fillStyle='#2a3a1a';
    ctx.fillRect(tx-18,ty-14,20,10); // кабина
    // Колёса
    ctx.fillStyle='#111';
    [-14,14].forEach(ox => { ctx.beginPath(); ctx.arc(tx+ox,ty+8,5,0,Math.PI*2); ctx.fill(); });
    // Турель пулемёта (крутится)
    drawBarrel(tx,ty-10,1,12);

  } else if (upg.tech===1) {
    // ЗСУ-23-4 "Шилка": гусеничная, башня с 4 стволами
    ctx.fillStyle='#3a5a2a';
    ctx.fillRect(tx-24,ty-7,48,14);
    // Гусеницы
    ctx.fillStyle='#222';
    ctx.fillRect(tx-26,ty,52,7);
    ctx.fillRect(tx-26,ty-7,52,5);
    // Башня
    ctx.fillStyle='#4a6a3a';
    ctx.beginPath(); ctx.arc(tx,ty-10,12,0,Math.PI*2); ctx.fill();
    // 4 ствола
    drawBarrel(tx,ty-10,2,18,-4);
    drawBarrel(tx,ty-10,2,18,-1.5);
    drawBarrel(tx,ty-10,2,18,1.5);
    drawBarrel(tx,ty-10,2,18,4);

  } else if (upg.tech===2) {
    // С-75: пусковая рампа
    ctx.fillStyle='#3a4a2a';
    ctx.fillRect(tx-28,ty-5,56,12);
    // Гусеницы
    ctx.fillStyle='#222'; ctx.fillRect(tx-30,ty+2,60,6);
    // Рампа
    ctx.fillStyle='#5a6a4a';
    drawBarrel(tx,ty-5,5,28);
    // Ракета на рампе
    ctx.fillStyle='#8af';
    ctx.save(); ctx.translate(tx,ty-5);
    const ang=getAimAngle(); ctx.rotate(ang);
    ctx.fillRect(2,-2,20,4);
    ctx.beginPath(); ctx.moveTo(22,-3); ctx.lineTo(28,0); ctx.lineTo(22,3); ctx.closePath(); ctx.fill();
    ctx.restore();

  } else {
    // С-300: транспортно-пусковой контейнер на шасси
    ctx.fillStyle='#3a5a3a';
    ctx.fillRect(tx-30,ty-8,60,16);
    ctx.fillStyle='#222';
    [-20,-5,10,22].forEach(ox => { ctx.beginPath(); ctx.arc(tx+ox,ty+8,5,0,Math.PI*2); ctx.fill(); });
    // 4 пусковых контейнера
    ctx.fillStyle='#6a8a5a';
    [-12,-4,4,12].forEach(ox => {
      ctx.save(); ctx.translate(tx+ox,ty-8);
      const ang=getAimAngle(); ctx.rotate(ang);
      ctx.fillRect(0,-3,22,6);
      ctx.beginPath(); ctx.moveTo(22,-4); ctx.lineTo(28,0); ctx.lineTo(22,4); ctx.closePath(); ctx.fill();
      ctx.restore();
    });
  }

  // Полоска HP базы
  const lifeW=160, lifeH=7, lx=W/2-lifeW/2, ly=H-14;
  ctx.fillStyle='#1a3a1a'; ctx.fillRect(lx,ly,lifeW,lifeH);
  const ratio=Math.max(0,1-damages/MAX_DMG);
  ctx.fillStyle=ratio>0.5?'#4f8':ratio>0.25?'#fa4':'#f44';
  ctx.fillRect(lx,ly,lifeW*ratio,lifeH);
  ctx.strokeStyle='#2a5a2a'; ctx.lineWidth=1; ctx.strokeRect(lx,ly,lifeW,lifeH);

  // Название системы
  ctx.fillStyle='#4f8'; ctx.font='8px Arial'; ctx.textAlign='center';
  ctx.fillText(getTechName(), tx, ty+24);
  ctx.textAlign='left';
}

function getAimAngle() {
  if (enemies.length===0) return -Math.PI/2;
  let best=enemies[0], bd=Infinity;
  enemies.forEach(e=>{ const d=Math.hypot(e.x-TURRET.x,e.y-TURRET.y); if(d<bd){bd=d;best=e;} });
  return Math.atan2(best.y-TURRET.y, best.x-TURRET.x);
}

function drawBarrel(bx,by,w,len,offsetY=0) {
  const ang=getAimAngle();
  ctx.save(); ctx.translate(bx,by+offsetY); ctx.rotate(ang);
  ctx.fillStyle='#8fa'; ctx.fillRect(0,-w/2,len,w);
  ctx.restore();
}

// ===== ОСТАЛЬНАЯ ОТРИСОВКА =====
const CITIES = [
  { name:'Киев',river:'Днепр',   riverX:0.82, riverBend:[0.78,0.2,0.85,0.4,0.80,0.6,0.76,0.8,0.82,0.9,0.80,1.0], districts:['Оболонь','Подол','Центр','Печерск','Лавра','Голосеев.','Теремки','Борщаг.','Святошин','Нивки','Дарниця','Троєщина'], bgColor:'#2d4a1e', blockColor:'#3a5a28', streetColor:'#1e3012' },
  { name:'Москва',river:'Москва-р.',riverX:0.45, riverBend:[0.35,0.3,0.50,0.5,0.45,0.7,0.40,0.85,0.48,0.95,0.46,1.0], districts:['Хамовники','Арбат','Тверской','Сокол','Бутово','Митино','Марьино','Люблино','Выхино','Ховрино','Медведково','Крылатск.'], bgColor:'#2a3a1a', blockColor:'#364828', streetColor:'#1a2810' },
  { name:'Санкт-Петербург', river:'Нева', riverX:0.60, riverBend:[0.55,0.0,0.65,0.2,0.60,0.4,0.55,0.6,0.62,0.8,0.58,1.0], districts:['Василеостр.','Петроград.','Центр','Адмирал.','Невский','Московск.','Калинин.','Красноглин.','Выборгск.','Фрунзенск.','Красносел.','Приморск.'], bgColor:'#283818', blockColor:'#324820', streetColor:'#182410' },
  { name:'Новосибирск', river:'Обь',    riverX:0.55, riverBend:[0.50,0.0,0.58,0.3,0.53,0.5,0.50,0.7,0.55,0.9,0.52,1.0], districts:['Центр','Дзержинск.','Ленинск.','Калинин.','Октябрьск.','Советск.','Кировск.','Первомайск.','Заельцовск.','Железнодор.','Новосибирск','Академгор.'], bgColor:'#2a3820', blockColor:'#364830', streetColor:'#1a2812' },
  { name:'Екатеринбург',river:'Исеть', riverX:0.48, riverBend:[0.44,0.1,0.50,0.3,0.46,0.5,0.50,0.7,0.44,0.9,0.48,1.0], districts:['Центр','Верх-Исетск.','Чкалов.','Орджоник.','Кировск.','Ленинск.','Железнод.','Октябрьск.','Синарск.','Тагилск.','Сортиров.','Уралмаш'], bgColor:'#2c3c1c', blockColor:'#384c28', streetColor:'#1c2c10' },
  { name:'Казань',river:'Казанка', riverX:0.72, riverBend:[0.68,0.0,0.75,0.25,0.70,0.45,0.68,0.65,0.74,0.85,0.70,1.0], districts:['Кремль','Вахитов.','Советск.','Приволжск.','Московск.','Кировск.','Авиастр.','Ново-Савин.','Нижнекамск','Зеленодол.','Дербышки','Царицыно'], bgColor:'#2d3f1f', blockColor:'#3a502a', streetColor:'#1d2f12' },
  { name:'Нижний Новгород',river:'Волга',riverX:0.78, riverBend:[0.74,0.0,0.80,0.3,0.76,0.5,0.74,0.7,0.80,0.9,0.76,1.0], districts:['Кремль','Нижегород.','Советск.','Приокск.','Автозавод.','Ленинск.','Канавин.','Сормовск.','Московск.','Борск.','Дзержинск','Кстово'], bgColor:'#2a3c1c', blockColor:'#364e28', streetColor:'#1a2c10' },
  { name:'Волгоград',  river:'Волга',   riverX:0.85, riverBend:[0.82,0.0,0.87,0.2,0.83,0.45,0.86,0.65,0.82,0.85,0.85,1.0], districts:['Центр','Тракторозав.','Краснооктябр.','Дзержинск.','Ворошилов.','Советск.','Кировск.','Красноармейск.','Светлоярск.','Городище','Камышин','Михайловка'], bgColor:'#2e4220', blockColor:'#3c542c', streetColor:'#1e3214' },
  { name:'Самара',river:'Волга',   riverX:0.80, riverBend:[0.76,0.1,0.82,0.3,0.78,0.55,0.76,0.75,0.82,0.9,0.78,1.0], districts:['Самарск.','Ленинск.','Железнод.','Октябрьск.','Промышл.','Советск.','Куйбышев.','Красноглин.','Кинель','Новокуйб.','Чапаевск','Отрадный'], bgColor:'#2c4020', blockColor:'#38522c', streetColor:'#1c301a' },
  { name:'Ростов-на-Дону',river:'Дон', riverX:0.50, riverBend:[0.46,0.6,0.52,0.7,0.48,0.8,0.52,0.9,0.46,0.95,0.50,1.0], districts:['Центр','Ленинск.','Октябрьск.','Советск.','Первомайск.','Пролетарск.','Железнод.','Орджоник.','Аксай','Батайск','Новочеркасск','Таганрог'], bgColor:'#304422', blockColor:'#3c562e', streetColor:'#203414' },
  { name:'Уфа',river:'Белая',   riverX:0.65, riverBend:[0.60,0.1,0.68,0.3,0.63,0.55,0.60,0.75,0.66,0.9,0.63,1.0], districts:['Центр','Советск.','Калинин.','Октябрьск.','Ленинск.','Орджоник.','Демский','Шакша','Сипайлово','Инорс','Зеленая Роща','Черниковка'], bgColor:'#2a3e1c', blockColor:'#365028', streetColor:'#1a2e10' },
  { name:'Минск',  river:'Свислочь',riverX:0.55, riverBend:[0.52,0.2,0.57,0.4,0.53,0.6,0.56,0.8,0.52,0.92,0.55,1.0], districts:['Центр','Советск.','Первомайск.','Партизанск.','Заводск.','Ленинск.','Октябрьск.','Фрунзенск.','Малиновка','Серебрянка','Уручье','Зеленый Луг'], bgColor:'#2c3e1e', blockColor:'#38502a', streetColor:'#1c2e10' },
  { name:'Берлин', river:'Шпрее',   riverX:0.52, riverBend:[0.48,0.3,0.55,0.45,0.50,0.6,0.53,0.75,0.48,0.9,0.52,1.0], districts:['Митте','Потсдамер','Тиргартен','Шарлоттенб.','Нойкёльн','Кройцберг','Марцан','Лихтенберг','Панков','Шпандау','Зеленсдорф','Трептов'], bgColor:'#28361a', blockColor:'#344826', streetColor:'#181e0e' },
  { name:'Варшава', river:'Висла',   riverX:0.75, riverBend:[0.72,0.0,0.78,0.25,0.73,0.5,0.76,0.72,0.72,0.88,0.75,1.0], districts:['Центр','Воля','Прага','Жолибож','Охота','Мокотув','Вислане','Урсус','Бялоленка','Таргувек','Влохы','Берново'], bgColor:'#2e401e', blockColor:'#3a5228', streetColor:'#1e3010' },
  { name:'Лондон',  river:'Темза',   riverX:0.50, riverBend:[0.46,0.55,0.54,0.62,0.48,0.70,0.53,0.78,0.47,0.88,0.52,1.0], districts:['Сити','Вестминст.','Ламбет','Саутварк','Хакни','Кэмден','Брент','Харингей','Гринвич','Льюишем','Тауэр','Ислингтон'], bgColor:'#263418', blockColor:'#324424', streetColor:'#16240e' },
  { name:'Париж',  river:'Сена',    riverX:0.50, riverBend:[0.44,0.45,0.54,0.55,0.46,0.65,0.53,0.75,0.46,0.88,0.52,1.0], districts:['Сите','Марэ','Монмартр','Бастилия','Монпарнас','Латинский','Дефанс','Венсен','Сен-Дени','Клиши','Левалуа','Булонь'], bgColor:'#2a3818', blockColor:'#364a24', streetColor:'#1a280e' },
  { name:'Стамбул', river:'Босфор',  riverX:0.55, riverBend:[0.52,0.0,0.58,0.2,0.53,0.4,0.57,0.6,0.52,0.8,0.56,1.0], districts:['Бейоглу','Фатих','Кадыкей','Ускюдар','Бешикташ','Сарыер','Бакыркёй','Авджылар','Малтепе','Карталь','Пендик','Султангази'], bgColor:'#30421e', blockColor:'#3c5428', streetColor:'#202c10' },
  { name:'Пекин', river:'Чаоян',   riverX:0.62, riverBend:[0.58,0.1,0.65,0.3,0.60,0.5,0.63,0.7,0.58,0.88,0.62,1.0], districts:['Дунчэн','Сичэн','Чаоян','Хайдянь','Фэнтай','Шицзиншань','Тунчжоу','Шуньи','Дасин','Фаншань','Мэньтоугоу','Пинань'], bgColor:'#2e3e18', blockColor:'#3a5022', streetColor:'#1e2e08' },
  { name:'Дели',  river:'Джамна',  riverX:0.78, riverBend:[0.74,0.0,0.80,0.25,0.76,0.5,0.74,0.72,0.80,0.88,0.76,1.0], districts:['Центр','Конн.Плейс','Паш.Виха','Двар.Кала','Ром.Нагар','Шахдара','Норт-Ист','Норт-Вест','Нью-Дели','Саут-Вест','Саут-Ист','Саут'], bgColor:'#324018', blockColor:'#3e5222', streetColor:'#222e08' },
  { name:'Нью-Йорк',   river:'Гудзон',  riverX:0.15, riverBend:[0.13,0.0,0.17,0.25,0.13,0.5,0.16,0.72,0.12,0.88,0.15,1.0], districts:['Манхэттен','Бруклин','Квинс','Бронкс','Стат.Айленд','Гарлем','Бедфорд','Флашинг','Байсайд','Астория','Джамейка','Фарр.Рок.'], bgColor:'#283618', blockColor:'#344822', streetColor:'#18260e' },
];
let currentCity = null;

function pickCity(wave) {
  // Меняем город каждые 5 волн
  const idx = Math.floor((wave-1)/5) % CITIES.length;
  currentCity = CITIES[idx];
}

function drawBg() {
  if (!currentCity) return;
  const city = currentCity;
  const scaleX = W / 360, scaleY = H / 500;

  // Земля — основной фон города
  ctx.fillStyle = city.bgColor;
  ctx.fillRect(0, 0, W, H);

  // Кварталы города
  ctx.fillStyle = city.blockColor;
  const blocks = [
    [0,0,80,60],[90,0,80,60],[180,0,80,60],[270,0,90,60],
    [0,70,70,55],[80,70,90,55],[180,70,80,55],[270,70,90,55],
    [0,135,75,60],[85,135,85,60],[180,135,75,60],[265,135,95,60],
    [0,205,80,60],[90,205,80,60],[180,205,85,60],[275,205,85,60],
    [0,275,70,60],[80,275,90,60],[180,275,80,60],[270,275,90,60],
    [0,345,80,55],[90,345,80,55],[180,345,80,55],[270,345,90,55],
  ];
  blocks.forEach(([bx,by,bw,bh]) =>
    ctx.fillRect(bx*scaleX, by*scaleY, bw*scaleX-2, bh*scaleY-2)
  );

  // Улицы — горизонтальные
  ctx.fillStyle = city.streetColor;
  [62,130,198,268,338].forEach(y =>
    ctx.fillRect(0, y*scaleY, W, 7*scaleY)
  );
  // Улицы — вертикальные
  [78,172,262].forEach(x =>
    ctx.fillRect(x*scaleX, 0, 7*scaleX, H)
  );

  // Река или море — синяя полоса с изгибом
  ctx.fillStyle = '#1a3a5c';
  const curve = city.riverBend;
  ctx.beginPath();
  ctx.moveTo(W * city.riverX, 0);
  ctx.bezierCurveTo(
    W*curve[0], H*curve[1], W*curve[2], H*curve[3],
    W*curve[4], H*curve[5]
  );
  ctx.bezierCurveTo(
    W*curve[6], H*curve[7], W*curve[8], H*curve[9],
    W*curve[10], H*curve[11]
  );
  // Заливаем до края экрана
  const riverSide = city.riverX > 0.5 ? 1 : 0;
  ctx.lineTo(W * riverSide, H);
  ctx.lineTo(W * riverSide, 0);
  ctx.closePath();
  ctx.fill();

  // Название реки / моря
  ctx.fillStyle = 'rgba(120,180,240,0.8)';
  ctx.font = Math.round(10*scaleX) + 'px Arial';
  ctx.textAlign = 'center';
  const riverLabelX = city.riverX > 0.5 ? W*city.riverX + 16 : W*city.riverX - 16;
  ctx.fillText(city.river, riverLabelX, H * 0.38);

  // Название города — крупно вверху
  ctx.fillStyle = 'rgba(255,240,180,0.7)';
  ctx.font = 'bold ' + Math.round(14*scaleX) + 'px Arial';
  ctx.fillText(city.name, W/2, 20*scaleY);
  ctx.textAlign = 'left';

  // Зона радара основной турели — зелёный круг
  const radarRange = getRadar();
  ctx.strokeStyle = 'rgba(0,255,80,0.15)';
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  ctx.arc(TURRET.x, TURRET.y, radarRange, 0, Math.PI*2);
  ctx.stroke();
  ctx.fillStyle = 'rgba(0,255,80,0.04)';
  ctx.beginPath();
  ctx.arc(TURRET.x, TURRET.y, radarRange, 0, Math.PI*2);
  ctx.fill();
}

function drawBullet(b) {
  ctx.beginPath(); ctx.arc(b.x,b.y,upg.tech>=2?4:2.5,0,Math.PI*2);
  ctx.fillStyle=upg.tech>=3?'#4ff':upg.tech>=2?'#ff4':'#cfa'; ctx.fill();
}
function drawExplosion(ex) {
  const a=ex.life/ex.maxLife;
  const r=ex.maxR*(1.5-a)*1.5;
  ctx.globalAlpha=a*0.7;
  ctx.beginPath(); ctx.arc(ex.x,ex.y,r,0,Math.PI*2);
  ctx.fillStyle=ex.color; ctx.fill();
  ctx.globalAlpha=1;
}
function drawParticle(p) {
  const a=p.life/p.maxLife;
  ctx.globalAlpha=a;
  ctx.beginPath(); ctx.arc(p.x,p.y,2,0,Math.PI*2);
  ctx.fillStyle=p.color; ctx.fill();
  ctx.globalAlpha=1;
}

function drawIdle() {
  ctx.fillStyle='rgba(0,10,0,0.84)'; ctx.fillRect(0,0,W,H);
  ctx.textAlign='center';
  ctx.fillStyle='#4f8'; ctx.font='bold 24px Arial'; ctx.fillText('🛡 ПВО',W/2,H/2-70);
  ctx.fillStyle='#fa4'; ctx.font='14px Arial'; ctx.fillText('Защити базу от воздушных угроз',W/2,H/2-40);
  ctx.fillStyle='#adf'; ctx.font='12px Arial';
  ctx.fillText('Тап/клик по врагу = выстрел',W/2,H/2-16);
  ctx.fillText('Прокачивай навык → лучше упреждение',W/2,H/2+4);
  ctx.fillStyle='#888'; ctx.font='11px Arial';
  ctx.fillText('ВОВ → Холодная война → Современность → ∞',W/2,H/2+26);
  ctx.fillStyle='#4f8'; ctx.font='11px Arial';
  ctx.fillText('По-2, Ju-88, He-111 → МиГ-17, B-52 → Байрактар, Томагавк',W/2,H/2+46);
  ctx.textAlign='left';
}

function drawOver() {
  ctx.fillStyle='rgba(0,5,0,0.9)'; ctx.fillRect(0,0,W,H);
  ctx.textAlign='center';
  ctx.fillStyle='#f44'; ctx.font='bold 24px Arial'; ctx.fillText('База уничтожена!',W/2,H/2-55);
  ctx.fillStyle='#fa4'; ctx.font='17px Arial'; ctx.fillText('Счёт: '+score,W/2,H/2-22);
  ctx.fillStyle='#4f8'; ctx.font='15px Arial'; ctx.fillText('Волна: '+waveNum+' | '+ERA_NAMES[getEra(waveNum)],W/2,H/2+8);
  ctx.fillStyle='#adf'; ctx.font='12px Arial'; ctx.fillText('Прокачай технику и попробуй снова',W/2,H/2+34);
  ctx.textAlign='left';
}

// ===== ГЛАВНЫЙ ЦИКЛ =====
function loop() {
  if (state==='play') {
    updateEnemies(); updateBullets(); updateExplosions(); updateParticles();
    updateWaves(); autoAim(); updateAlly();
    if (msgTimer>0){ msgTimer--; if(msgTimer===0) document.getElementById('msg').textContent=''; }
  }
  drawBg();
  enemies.forEach(drawEnemy);
  bullets.forEach(drawBullet);
  explosions.forEach(drawExplosion);
  particles.forEach(drawParticle);
  drawAlly();
  drawPVO();
  if (state==='idle') drawIdle();
  if (state==='over') drawOver();
  animId=requestAnimationFrame(loop);
}

// ===== УПРАВЛЕНИЕ =====
function getPos(e) {
  const r=canvas.getBoundingClientRect(), t=e.touches?e.touches[0]:e;
  return { x:(t.clientX-r.left)*(W/r.width), y:(t.clientY-r.top)*(H/r.height) };
}
canvas.addEventListener('mousedown', e=>{
  if (state!=='play') return;
  const p=getPos(e);
  let best=null, bd=70;
  enemies.forEach(en=>{ const d=Math.hypot(en.x-p.x,en.y-p.y); if(d<bd){bd=d;best=en;} });
  if (best) { const aim=computeAimPoint(best); shoot(aim.x,aim.y); }
  else shoot(p.x,p.y);
});
canvas.addEventListener('touchstart',e=>{
  e.preventDefault();
  if (state!=='play') return;
  const p=getPos(e);
  let best=null, bd=80;
  enemies.forEach(en=>{ const d=Math.hypot(en.x-p.x,en.y-p.y); if(d<bd){bd=d;best=en;} });
  if (best) { const aim=computeAimPoint(best); shoot(aim.x,aim.y); }
  else shoot(p.x,p.y);
},{passive:false});

// ===== КОНЕЦ =====
function endGame() {
  state='over';
  showMsg('База уничтожена! Счёт: '+score,'#f44');
  const name=document.getElementById('nameInput').value||'Командир';
  if (sock&&sock.readyState===1)
    sock.send(JSON.stringify({action:'game_over',name,won:waveNum>=20,scored:score,assists:waveNum}));
}

function showLb() {
  document.getElementById('lb').style.display='block';
  if (sock&&sock.readyState===1) sock.send(JSON.stringify({action:'get_leaderboard'}));
}
function renderLb(data) {
  const sorted=[...data].sort((a,b)=>b.goals-a.goals);
  document.getElementById('lbBody').innerHTML=sorted.map((p,i)=>
    `<tr><td>${i+1}</td><td>${p.name}</td><td>${p.goals}</td><td>${p.assists||0}</td></tr>`
  ).join('');
  document.getElementById('lb').style.display='block';
}

loop();
</script>
</body>
</html>
)rawliteral";

// турнирная сетка
struct Player {
  String name;
  int wins;
  int losses;
  int goals;
  int assists;

};

Player tyrnirka[10];
int kolvoIgrok = 0;

void addOrUpdatePlayer(String name, bool won, int scored, int assists) {
  for (int i = 0; i < kolvoIgrok; i++) {
    if (tyrnirka[i].name == name) {
      if (won) tyrnirka[i].wins++;
      else tyrnirka[i].losses++;
      tyrnirka[i].goals += scored;
      tyrnirka[i].assists += assists;
      return;
    }
  }
  if (kolvoIgrok < 10) {
    tyrnirka[kolvoIgrok++] = {name, won ? 1 : 0, won ? 0 : 1, scored, assists};
  }
}

String getLeaderboardJson() {
  StaticJsonDocument<1024> doc;
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < kolvoIgrok; i++) {
    JsonObject p = arr.createNestedObject();
    p["name"]   = tyrnirka[i].name;
    p["wins"]   = tyrnirka[i].wins;
    p["losses"] = tyrnirka[i].losses;
    p["goals"]  = tyrnirka[i].goals;
    p["assists"] = tyrnirka[i].assists;
  }
  String out;
  serializeJson(doc, out);
  return out;
}
void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_DATA) {
    String msg = String((char*)data).substring(0, len);
    StaticJsonDocument<256> doc;
    if (!deserializeJson(doc, msg)) {
      String action = doc["action"].as<String>();
      if (action == "game_over") {
        String name   = doc["name"]   | "Player";
        bool   won    = doc["won"]    | false;
        int    scored = doc["scored"] | 0;
        int assists = doc["assists"] | 0;
        addOrUpdatePlayer(name, won, scored, assists);
        // Отправить таблицу всем
        ws.textAll("{\"type\":\"leaderboard\",\"data\":" + getLeaderboardJson() + "}");
      }
      if (action == "get_leaderboard") {
        client->text("{\"type\":\"leaderboard\",\"data\":" + getLeaderboardJson() + "}");
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  WiFi.softAP(ssid, password);
  Serial.println("AP IP: " + WiFi.softAPIP().toString());

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send_P(200, "text/html", INDEX_HTML);
  });
  server.begin();
}

void loop() {
  ws.cleanupClients();
}