// state.js

// ---------- State ----------
let state = { traders:[], quests:[] };   // nodes carry editor-only _x,_y
let view = { x:120, y:90, k:1 };
let selection = null;                     // {kind:'quest'|'trader', id}
let undoStack = [], redoStack = [];
let validationCache = [];

// ---------- DOM ----------
const $ = s => document.querySelector(s);
const canvas = $("#canvas"), world = $("#world"), edgesSvg = $("#edges");
const inspBody = $("#insp-body"), inspHead = $("#insp-head"), inspEmpty = $("#insp-empty");

// ---------- Utils ----------
function toast(msg,isErr){ const t=$("#toast"); t.innerHTML=msg; t.className=isErr?"show err":"show";
  clearTimeout(t._t); t._t=setTimeout(()=>t.className="",2200); }
function el(tag,props,kids){ const e=document.createElement(tag);
  if(props) for(const k in props){ if(k==="class")e.className=props[k]; else if(k==="text")e.textContent=props[k];
    else if(k==="html")e.innerHTML=props[k]; else if(k.startsWith("on"))e.addEventListener(k.slice(2).toLowerCase(),props[k]);
    else if(k==="style")e.setAttribute("style",props[k]); else e.setAttribute(k,props[k]); }
  if(kids) (Array.isArray(kids)?kids:[kids]).forEach(c=>{ if(c!=null) e.append(c.nodeType?c:document.createTextNode(c)); });
  return e; }
function snap(v){ return Math.round(v/13)*13; }

// ---------- Persistence & history ----------
function save(){ localStorage.setItem(STORE_KEY, JSON.stringify({state,view})); }
function load(){ try{ const raw=localStorage.getItem(STORE_KEY); if(!raw)return false;
  const d=JSON.parse(raw); if(d&&d.state){ state=d.state; if(d.view)view=d.view;
    state.traders=state.traders||[]; state.quests=state.quests||[]; return true; } }catch(e){} return false; }
function snapshot(){ undoStack.push(JSON.stringify(state)); if(undoStack.length>60)undoStack.shift(); redoStack.length=0; }
function undo(){ if(!undoStack.length)return; redoStack.push(JSON.stringify(state));
  state=JSON.parse(undoStack.pop()); selection=null; renderAll(); save(); toast("Rückgängig"); }
function redo(){ if(!redoStack.length)return; undoStack.push(JSON.stringify(state));
  state=JSON.parse(redoStack.pop()); selection=null; renderAll(); save(); toast("Wiederholt"); }

// ---------- Coordinate transform ----------
function applyView(){ world.style.transform=`translate(${view.x}px,${view.y}px) scale(${view.k})`;
  $("#zoomLabel").textContent=Math.round(view.k*100)+"%"; }
function screenToWorld(sx,sy){ const r=canvas.getBoundingClientRect();
  return {x:(sx-r.left-view.x)/view.k, y:(sy-r.top-view.y)/view.k}; }
