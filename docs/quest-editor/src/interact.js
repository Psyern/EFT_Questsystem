// interact.js
/* =====================================================================
   INTERACTION: pan / zoom / drag / connect
   ===================================================================== */
let dragState=null, panState=null, connectState=null, spaceDown=false;

canvas.addEventListener("pointerdown",e=>{
  if(e.target===canvas||e.target===world||e.target===edgesSvg){
    if(e.button===0&&!spaceDown){ selectNode(null); }
    if(e.button===1||e.button===2||spaceDown||e.button===0){ startPan(e); }
    if(e.button===2){ /* handled in contextmenu */ }
  }
});
canvas.addEventListener("contextmenu",e=>{ if(e.target===canvas||e.target===world||e.target===edgesSvg){ e.preventDefault(); openContextMenu(e.clientX,e.clientY,null); } });

function startPan(e){ if(e.button===2)return; panState={sx:e.clientX,sy:e.clientY,ox:view.x,oy:view.y};
  canvas.classList.add("panning"); canvas.setPointerCapture(e.pointerId); }

function startNodeDrag(e,n,wrap){ if(e.button!==0)return; e.stopPropagation();
  const w0=screenToWorld(e.clientX,e.clientY); dragState={n,wrap,dx:w0.x-(n._x||0),dy:w0.y-(n._y||0),moved:false};
  wrap.style.zIndex=1000; canvas.setPointerCapture(e.pointerId); }

function startConnect(e,cls){ e.stopPropagation(); e.preventDefault();
  const wrap=e.target.closest(".node"); const n=wrap.__node;
  const from = cls==="out-trader" ? nodePortPos(n,"trader-out") : nodePortPos(n,"prereq-out");
  connectState={kind:cls==="out-trader"?"trader":"prereq", src:n, from};
  const p=document.createElementNS("http://www.w3.org/2000/svg","path");
  p.setAttribute("class","wire"); p.setAttribute("stroke",cls==="out-trader"?"#4A90D9":"#C9A227");
  p.setAttribute("stroke-dasharray","5 4"); p.setAttribute("opacity","0.9"); p.id="tempwire";
  edgesSvg.append(p); canvas.classList.add("connecting"); canvas.setPointerCapture(e.pointerId); }

window.addEventListener("pointermove",e=>{
  if(panState){ view.x=panState.ox+(e.clientX-panState.sx); view.y=panState.oy+(e.clientY-panState.sy); applyView(); return; }
  if(dragState){ const w=screenToWorld(e.clientX,e.clientY); const nx=w.x-dragState.dx, ny=w.y-dragState.dy;
    dragState.n._x=snap(nx); dragState.n._y=snap(ny); dragState.wrap.style.left=dragState.n._x+"px";
    dragState.wrap.style.top=dragState.n._y+"px"; dragState.moved=true; renderEdges(); return; }
  if(connectState){ const w=screenToWorld(e.clientX,e.clientY); const a=connectState.from;
    const p=$("#tempwire"); if(p){ const dir=connectState.kind==="trader"?"h":"v";
      let d; if(dir==="h"){const dx=Math.max(46,Math.abs(w.x-a.x)*0.5);d=`M ${a.x} ${a.y} C ${a.x+dx} ${a.y}, ${w.x-dx} ${w.y}, ${w.x} ${w.y}`;}
      else{const dy=Math.max(46,Math.abs(w.y-a.y)*0.5);d=`M ${a.x} ${a.y} C ${a.x} ${a.y+dy}, ${w.x} ${w.y-dy}, ${w.x} ${w.y}`;}
      p.setAttribute("d",d); } return; }
});
window.addEventListener("pointerup",e=>{
  if(panState){ panState=null; canvas.classList.remove("panning"); }
  if(dragState){ if(dragState.moved){ snapshot(); save(); } dragState.wrap.style.zIndex=""; dragState=null; }
  if(connectState){ finishConnect(e); }
});

function finishConnect(e){ const tmp=$("#tempwire"); if(tmp)tmp.remove(); canvas.classList.remove("connecting");
  const target=document.elementFromPoint(e.clientX,e.clientY); const wrap=target&&target.closest(".node");
  const cs=connectState; connectState=null;
  if(!wrap){ return; } const tn=wrap.__node;
  if(cs.kind==="trader"){ if(tn.__k!=="quest"){ toast("Trader-Kante nur auf eine Quest",true); return; }
    snapshot(); tn.TraderId=cs.src.id; renderAll(); save(); toast("Trader bietet Quest an"); }
  else { if(tn.__k!=="quest"){ toast("Voraussetzung nur zwischen Quests",true); return; }
    if(tn===cs.src){ return; }
    if(wouldCycle(cs.src.QuestId,tn.QuestId)){ toast("Zyklus verhindert",true); return; }
    const arr=tn.Prerequisites.RequiredCompletedQuests; if(arr.includes(cs.src.QuestId)){ toast("Bereits verknüpft"); return; }
    snapshot(); arr.push(cs.src.QuestId); renderAll(); save(); toast("Voraussetzung gesetzt"); }
}
function wouldCycle(fromId,toId){ // adding edge from->to; cycle if 'from' is reachable from 'to' via prereqs
  const seen=new Set(); const stack=[toId];
  while(stack.length){ const cur=stack.pop(); if(cur===fromId)return true; if(seen.has(cur))continue; seen.add(cur);
    const q=questById(cur); if(q) (q.Prerequisites.RequiredCompletedQuests||[]).forEach(r=>stack.push(r)); }
  return false; }

// zoom
canvas.addEventListener("wheel",e=>{ e.preventDefault(); const r=canvas.getBoundingClientRect();
  const mx=e.clientX-r.left, my=e.clientY-r.top; const wx=(mx-view.x)/view.k, wy=(my-view.y)/view.k;
  const factor=e.deltaY<0?1.12:1/1.12; view.k=Math.min(2.4,Math.max(0.25,view.k*factor));
  view.x=mx-wx*view.k; view.y=my-wy*view.k; applyView(); save(); },{passive:false});

// keyboard
window.addEventListener("keydown",e=>{
  if(e.code==="Space"&&!isTyping(e)){ spaceDown=true; }
  if(isTyping(e)) return;
  const ctrl=e.ctrlKey||e.metaKey;
  if(ctrl&&e.key.toLowerCase()==="z"){ e.preventDefault(); undo(); return; }
  if(ctrl&&(e.key.toLowerCase()==="y"||(e.shiftKey&&e.key.toLowerCase()==="z"))){ e.preventDefault(); redo(); return; }
  if(ctrl&&e.key.toLowerCase()==="d"){ e.preventDefault(); duplicateSelection(); return; }
  if(e.key==="n"||e.key==="N"){ e.preventDefault(); addQuest(); }
  else if(e.key==="t"||e.key==="T"){ e.preventDefault(); addTrader(); }
  else if(e.key==="f"||e.key==="F"){ e.preventDefault(); fitView(); }
  else if(e.key==="l"||e.key==="L"){ e.preventDefault(); autoLayout(); }
  else if(e.key==="?"){ e.preventDefault(); openModal("#modalHelp"); }
  else if(e.key==="Delete"||e.key==="Backspace"){ e.preventDefault(); deleteSelection(); }
  else if(e.key==="Escape"){ selectNode(null); closeModals(); closeContextMenu(); }
});
window.addEventListener("keyup",e=>{ if(e.code==="Space")spaceDown=false; });
function isTyping(e){ const t=e.target; return t&&(t.tagName==="INPUT"||t.tagName==="TEXTAREA"||t.tagName==="SELECT"||t.isContentEditable); }
