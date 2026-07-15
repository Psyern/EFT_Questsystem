// render.js
/* =====================================================================
   RENDERING
   ===================================================================== */
const nodeEls = new Map();   // id -> element

function renderAll(){ world.querySelectorAll(".node").forEach(n=>n.remove()); nodeEls.clear();
  state.traders.forEach(t=>{ t.__k="trader"; renderNode(t); });
  state.quests.forEach(q=>{ q.__k="quest"; renderNode(q); });
  measureNodes(); renderEdges(); renderInspector(); runValidation(); applyView(); }

function measureNodes(){ nodeEls.forEach((elm,id)=>{ const n=elm.__node; n._w=elm.offsetWidth; n._h=elm.offsetHeight; }); }

function renderNode(n){
  const isTrader = n.__k==="trader";
  const id = nodeId(n);
  const wrap = el("div",{class:"node"+(isSel(n)?" sel":""),"data-id":id,"data-kind":n.__k});
  wrap.style.left=(n._x||0)+"px"; wrap.style.top=(n._y||0)+"px"; wrap.__node=n;

  const cat = isTrader ? (n.Faction||"NEUTRAL") : (n.Category||"SIDE");
  const accentColor = isTrader ? (FACTION_COLOR[n.Faction]||"#8A8F98") : (CATEGORY_COLOR[n.Category]||"#8A8F98");
  wrap.append(el("div",{class:"accent",style:`background:${accentColor}`}));

  const tag = el("span",{class:"ntag",text:isTrader?"TRADER":"QUEST",
    style:`color:${accentColor};border-color:${accentColor}55;background:${accentColor}14`});
  const title = el("div",{class:"ntitle",text:(isTrader?n.DisplayName:n.Title)||"(ohne Titel)"});
  const head = el("div",{class:"nhead"},[tag,title]);
  wrap.append(head);
  wrap.append(el("div",{class:"nid",text:id}));

  if(isTrader){
    const lv=(n.LoyaltyLevels||[]).length;
    wrap.append(el("div",{class:"nbadges"},[
      el("span",{class:"badge",text:n.Faction||"NEUTRAL"}),
      el("span",{class:"badge",text:lv+" LOYALTY"})
    ]));
    // output port: offers quests
    wrap.append(portEl("out-trader","BIETET AN"));
  } else {
    // badges
    const badges=[el("span",{class:"badge",text:n.Category||"SIDE"})];
    if(n.Repeatable) badges.push(el("span",{class:"badge",text:"REPEAT"}));
    if(n.TimeLimit>0) badges.push(el("span",{class:"badge",text:"⏱ "+n.TimeLimit+"s"}));
    if((n.Prerequisites&&n.Prerequisites.RequiredCompletedQuests||[]).length) badges.push(el("span",{class:"badge",text:"↳ CHAIN"}));
    wrap.append(el("div",{class:"nbadges"},badges));
    // objectives
    const objs=n.Objectives||[];
    if(objs.length){ const list=el("div",{class:"obj-list"});
      objs.slice(0,6).forEach(o=>{ const c=OBJ_COLOR[o.Type]||"#7A8290";
        list.append(el("div",{class:"obj-row"},[
          el("span",{class:"obj-dot",style:`background:${c}`}),
          el("span",{class:"obj-type",text:o.Type}),
          el("span",{class:"obj-txt",text:objSummary(o)})
        ])); });
      if(objs.length>6) list.append(el("div",{class:"obj-row"},[el("span",{class:"obj-txt",style:"color:var(--faint)",text:"+"+(objs.length-6)+" weitere…"})]));
      wrap.append(list);
    } else { wrap.classList.add("empty-obj"); wrap.append(el("div",{class:"obj-empty",text:"keine Objectives"})); }
    // footer: trader + validation flags
    const issues=nodeIssues(id);
    const foot=el("div",{class:"nfoot"});
    foot.append(el("span",{text:n.TraderId?("→ "+n.TraderId):"⚠ kein Trader"}));
    if(issues.length){ const e=issues.filter(i=>i.sev==="err").length, w=issues.length-e;
      foot.append(el("span",{class:"flags-badge"+(e?"":" warn"),title:issues.map(i=>i.msg).join("\n"),
        text:(e?e+" ERR":"")+(e&&w?" · ":"")+(w?w+" WARN":"")})); }
    wrap.append(foot);
    // ports
    wrap.append(portEl("out-prereq","FOLLOW-UP"));
  }

  head.addEventListener("pointerdown",e=>startNodeDrag(e,n,wrap));
  wrap.addEventListener("pointerdown",e=>{ if(e.button===0){ selectNode(n); } });
  wrap.addEventListener("contextmenu",e=>{ e.preventDefault(); selectNode(n); openContextMenu(e.clientX,e.clientY,n); });
  world.append(wrap); nodeEls.set(id,wrap);
  return wrap;
}
function portEl(cls,label){ const p=el("div",{class:"port "+cls}); p.append(el("span",{class:"plabel",text:label}));
  p.addEventListener("pointerdown",e=>startConnect(e,cls)); return p; }

function objSummary(o){
  if(o.Type==="KILL"||o.Type==="BOSS"){ const who=o.BossId?("Boss "+o.BossId):(o.TargetCategory||((o.ClassNames||[])[0])||"Ziel"); return o.Amount+"× "+who; }
  if(o.Type==="COLLECT"||o.Type==="CRAFT"||o.Type==="DELIVER"){ return o.Amount+"× "+((o.ClassNames||[])[0]||"Item"); }
  if(o.Type==="HANDOVER"){ return "abgeben → "+(o.ReferencesObjective||"?"); }
  if(o.Type==="TRAVEL"||o.Type==="DISCOVER"||o.Type==="ESCORT"||o.Type==="DEFEND"||o.Type==="EXTRACT"){ return o.Zone&&o.Zone.ZoneId?o.Zone.ZoneId:"Zone"; }
  if(o.Type==="RETURN_TO_TRADER"){ return "→ "+(o.TraderId||"Trader"); }
  if(o.Type==="SURVIVE"){ return o.Amount+"s"; }
  return o.ObjectiveId;
}

// ----- Edges -----
function nodePortPos(n,which){ const w=n._w||248, h=n._h||90;
  if(which==="trader-out") return {x:(n._x||0)+w, y:(n._y||0)+22};
  if(which==="trader-in")  return {x:(n._x||0),   y:(n._y||0)+22};
  if(which==="prereq-out") return {x:(n._x||0)+w/2, y:(n._y||0)+h};
  if(which==="prereq-in")  return {x:(n._x||0)+w/2, y:(n._y||0)}; }

function renderEdges(){
  while(edgesSvg.firstChild) edgesSvg.removeChild(edgesSvg.firstChild);
  const frag=document.createDocumentFragment();
  // trader -> quest (offers)
  state.quests.forEach(q=>{ if(!q.TraderId)return; const t=traderById(q.TraderId); if(!t)return;
    const a=nodePortPos(t,"trader-out"), b=nodePortPos(q,"trader-in");
    frag.append(edgePath(a,b,"h","#4A90D9",{kind:"trader",to:q.QuestId},"")); });
  // quest -> quest (prereq): A required by B  => edge A(bottom) -> B(top)
  state.quests.forEach(b=>{ const reqs=(b.Prerequisites&&b.Prerequisites.RequiredCompletedQuests)||[];
    reqs.forEach(reqId=>{ const a=questById(reqId); if(!a)return;
      const pa=nodePortPos(a,"prereq-out"), pb=nodePortPos(b,"prereq-in");
      frag.append(edgePath(pa,pb,"v","#C9A227",{kind:"prereq",from:reqId,to:b.QuestId},"")); }); });
  edgesSvg.append(frag);
}
function edgePath(a,b,dir,color,meta,label){
  const g=document.createElementNS("http://www.w3.org/2000/svg","g");
  let d;
  if(dir==="h"){ const dx=Math.max(46,Math.abs(b.x-a.x)*0.5); d=`M ${a.x} ${a.y} C ${a.x+dx} ${a.y}, ${b.x-dx} ${b.y}, ${b.x} ${b.y}`; }
  else { const dy=Math.max(46,Math.abs(b.y-a.y)*0.5); d=`M ${a.x} ${a.y} C ${a.x} ${a.y+dy}, ${b.x} ${b.y-dy}, ${b.x} ${b.y}`; }
  const hit=document.createElementNS("http://www.w3.org/2000/svg","path"); hit.setAttribute("d",d); hit.setAttribute("class","hit");
  const wire=document.createElementNS("http://www.w3.org/2000/svg","path"); wire.setAttribute("d",d);
  wire.setAttribute("class","wire"); wire.setAttribute("stroke",color); wire.setAttribute("stroke-dasharray",meta.kind==="prereq"?"6 4":"");
  wire.setAttribute("opacity","0.85");
  hit.addEventListener("click",()=>deleteEdge(meta));
  hit.addEventListener("contextmenu",e=>{e.preventDefault();deleteEdge(meta);});
  g.append(hit,wire);
  // label
  const mx=(a.x+b.x)/2, my=(a.y+b.y)/2;
  const tx=document.createElementNS("http://www.w3.org/2000/svg","text");
  tx.setAttribute("x",mx); tx.setAttribute("y",my-2); tx.setAttribute("text-anchor","middle");
  tx.setAttribute("class","elabel"); tx.textContent=meta.kind==="trader"?"bietet an":"setzt voraus";
  g.append(tx);
  return g;
}
function deleteEdge(meta){ snapshot();
  if(meta.kind==="trader"){ const q=questById(meta.to); if(q)q.TraderId=""; }
  else { const b=questById(meta.to); if(b){ const arr=b.Prerequisites.RequiredCompletedQuests;
    const i=arr.indexOf(meta.from); if(i>=0)arr.splice(i,1); } }
  renderAll(); save(); toast("Verbindung entfernt"); }

// ----- helpers for selection & node card refresh -----
function isSel(n){ return selection && selection.kind===n.__k && selection.id===nodeId(n); }
function refreshNode(n){ const id=nodeId(n); const old=nodeEls.get(id); const nw=renderNode(n);
  if(old){ nw.style.zIndex=old.style.zIndex; old.remove(); } measureOne(n); renderEdges(); }
function measureOne(n){ const e=nodeEls.get(nodeId(n)); if(e){ n._w=e.offsetWidth; n._h=e.offsetHeight; } }
