// main.js

/* =====================================================================
   ACTIONS: add / delete / duplicate / layout / fit
   ===================================================================== */
function centerWorldPos(){ const r=canvas.getBoundingClientRect(); return screenToWorld(r.left+r.width/2-120,r.top+r.height/3); }
function addQuest(){ snapshot(); const q=newQuestDef(); const c=centerWorldPos(); q._x=snap(c.x); q._y=snap(c.y); q.__k="quest";
  state.quests.push(q); renderAll(); selectNode(q); save(); toast('Quest <span class="mono">'+q.QuestId+'</span> erstellt'); }
function addTrader(){ snapshot(); const t=newTraderDef(); const c=centerWorldPos(); t._x=snap(c.x); t._y=snap(c.y); t.__k="trader";
  state.traders.push(t); renderAll(); selectNode(t); save(); toast('Trader <span class="mono">'+t.id+'</span> erstellt'); }
function deleteSelection(){ const n=selectedNode(); if(!n)return; deleteNode(n); }
function deleteNode(n){ snapshot();
  if(n.__k==="trader"){ state.traders=state.traders.filter(x=>x!==n);
    state.quests.forEach(q=>{ if(q.TraderId===n.id)q.TraderId=""; }); }
  else { const qid=n.QuestId; state.quests=state.quests.filter(x=>x!==n);
    state.quests.forEach(q=>{ const pr=q.Prerequisites; ["RequiredCompletedQuests","BlockedByCompletedQuests"].forEach(k=>{
      pr[k]=pr[k].filter(v=>v!==qid); }); }); }
  selection=null; renderAll(); save(); toast("Gelöscht"); }
function duplicateSelection(){ const n=selectedNode(); if(!n)return; snapshot();
  if(n.__k==="trader"){ const c=clone(n); delete c.__k; c.id=uid("trader_"); c.DisplayName=n.DisplayName+" (Kopie)";
    c._x=(n._x||0)+30; c._y=(n._y||0)+30; c.__k="trader"; state.traders.push(c); renderAll(); selectNode(c); }
  else { const c=clone(n); delete c.__k; c.QuestId=uid("quest_"); c.Title=n.Title+" (Kopie)";
    c._x=(n._x||0)+30; c._y=(n._y||0)+30; c.__k="quest"; state.quests.push(c); renderAll(); selectNode(c); }
  save(); toast("Dupliziert"); }

function autoLayout(){ snapshot();
  // depth = length of longest prereq chain; traders placed above their first quest column
  const depth=new Map(); function d(qid,seen){ if(depth.has(qid))return depth.get(qid); if(seen.has(qid))return 0; seen.add(qid);
    const q=questById(qid); if(!q){return 0;} const reqs=(q.Prerequisites.RequiredCompletedQuests||[]);
    let m=0; reqs.forEach(r=>{ m=Math.max(m,d(r,seen)+1); }); depth.set(qid,m); return m; }
  state.quests.forEach(q=>d(q.QuestId,new Set()));
  const colW=300, rowH=210; const rowCount={};
  // group quests by trader, order by depth
  const byTrader={}; state.quests.forEach(q=>{ (byTrader[q.TraderId||"__none"]=byTrader[q.TraderId||"__none"]||[]).push(q); });
  let traderCol=0;
  Object.keys(byTrader).forEach(tid=>{ const list=byTrader[tid].sort((a,b)=>(depth.get(a.QuestId)||0)-(depth.get(b.QuestId)||0));
    const t=traderById(tid); const baseX=traderCol*colW;
    if(t){ t._x=snap(baseX); t._y=0; }
    list.forEach((q,i)=>{ const dp=depth.get(q.QuestId)||0; q._x=snap(baseX); q._y=snap(160+dp*rowH);
      // avoid overlap at same depth in same trader col
      const key=tid+"_"+dp; rowCount[key]=(rowCount[key]||0); q._x=snap(baseX+rowCount[key]*270); rowCount[key]++; });
    // width used by this trader = max lane count
    let maxLanes=1; Object.keys(rowCount).forEach(k=>{ if(k.startsWith(tid+"_"))maxLanes=Math.max(maxLanes,rowCount[k]); });
    traderCol += maxLanes;
  });
  renderAll(); fitView(); save(); toast("Auto-Layout angewendet"); }

function fitView(){ const nodes=[...state.traders,...state.quests]; if(!nodes.length){ view={x:120,y:90,k:1}; applyView(); return; }
  let minX=1e9,minY=1e9,maxX=-1e9,maxY=-1e9;
  nodes.forEach(n=>{ const w=n._w||248,h=n._h||120; minX=Math.min(minX,n._x||0); minY=Math.min(minY,n._y||0);
    maxX=Math.max(maxX,(n._x||0)+w); maxY=Math.max(maxY,(n._y||0)+h); });
  const r=canvas.getBoundingClientRect(); const pad=70;
  const k=Math.min(1.2,Math.min((r.width-2*pad)/(maxX-minX),(r.height-2*pad)/(maxY-minY)));
  view.k=Math.max(0.25,k||1); view.x=pad-minX*view.k; view.y=pad-minY*view.k; applyView(); save(); }

/* =====================================================================
   CONTEXT MENU / MODALS
   ===================================================================== */
function openContextMenu(x,y,n){ const m=$("#ctxmenu"); m.innerHTML="";
  if(n){ n.__k=n.__k||(n.QuestId?"quest":"trader");
    m.append(el("div",{class:"ctx-h",text:(n.__k==="trader"?"Trader: ":"Quest: ")+nodeId(n)}));
    m.append(el("button",{text:"Duplizieren  (Ctrl+D)",onClick:()=>{closeContextMenu();duplicateSelection();}}));
    m.append(el("button",{text:"JSON anzeigen",onClick:()=>{closeContextMenu();showCode();}}));
    if(n.__k==="trader") m.append(el("button",{text:"Datei exportieren",onClick:()=>{closeContextMenu();
      downloadBlob(new Blob([JSON.stringify(traderExport(n),null,"\t")],{type:"application/json"}),n.id+".json");}}));
    else m.append(el("button",{text:"Datei exportieren",onClick:()=>{closeContextMenu();
      downloadBlob(new Blob([JSON.stringify(questExport(n),null,"\t")],{type:"application/json"}),n.QuestId+".json");}}));
    m.append(el("div",{class:"sep"}));
    m.append(el("button",{class:"danger",text:"Löschen  (Entf)",onClick:()=>{closeContextMenu();deleteNode(n);}}));
  } else {
    m.append(el("div",{class:"ctx-h",text:"Neu erstellen"}));
    m.append(el("button",{text:"+ Quest  (N)",onClick:()=>{closeContextMenu();addQuest();}}));
    m.append(el("button",{text:"+ Trader  (T)",onClick:()=>{closeContextMenu();addTrader();}}));
    m.append(el("div",{class:"sep"}));
    m.append(el("button",{text:"Auto-Layout  (L)",onClick:()=>{closeContextMenu();autoLayout();}}));
    m.append(el("button",{text:"Ansicht einpassen  (F)",onClick:()=>{closeContextMenu();fitView();}}));
  }
  m.style.display="block"; const r=canvas.getBoundingClientRect();
  m.style.left=Math.min(x-r.left,r.width-190)+"px"; m.style.top=Math.min(y-r.top,r.height-260)+"px";
}
function closeContextMenu(){ $("#ctxmenu").style.display="none"; }
canvas.addEventListener("pointerdown",()=>closeContextMenu());

function openModal(sel){ $(sel).classList.add("show"); }
function closeModals(){ document.querySelectorAll(".modal-bg").forEach(m=>m.classList.remove("show")); }
document.querySelectorAll("[data-close]").forEach(b=>b.addEventListener("click",closeModals));
document.querySelectorAll(".modal-bg").forEach(m=>m.addEventListener("pointerdown",e=>{ if(e.target===m)closeModals(); }));
$("#modalCode").querySelector("[data-copy]").addEventListener("click",()=>{ const t=$("#codePre").textContent;
  navigator.clipboard.writeText(t).then(()=>toast("JSON kopiert")); });

/* =====================================================================
   TOOLBAR WIRING
   ===================================================================== */
$("#btnNewQuest").onclick=addQuest;
$("#btnNewTrader").onclick=addTrader;
$("#btnUndo").onclick=undo; $("#btnRedo").onclick=redo;
$("#btnLayout").onclick=autoLayout; $("#btnFit").onclick=fitView;
$("#btnImport").onclick=()=>$("#fileInput").click();
$("#btnExport").onclick=exportZip;
$("#btnCode").onclick=showCode;
$("#btnHelp").onclick=()=>openModal("#modalHelp");
$("#btnValidate").onclick=()=>{ const p=$("#valpanel"); p.classList.toggle("hidden"); };
$("#valHead").onclick=()=>$("#valpanel").classList.add("hidden");
$("#insp-close").onclick=()=>selectNode(null);
$("#btnClear").onclick=()=>{ if(confirm("Alle Trader und Quests löschen? (nur diese Session, Export vorher!)")){ snapshot();
  state={traders:[],quests:[]}; selection=null; renderAll(); save(); toast("Session geleert"); } };

// help content
(function(){ const kb=[["N","Neue Quest"],["T","Neuer Trader"],["F","Ansicht einpassen"],["L","Auto-Layout"],
  ["Space + Ziehen","Leinwand verschieben"],["Mausrad","Zoom"],["Ctrl+Z / Ctrl+Y","Rückgängig / Wiederholen"],
  ["Ctrl+D","Duplizieren"],["Entf","Auswahl löschen"],["Rechtsklick","Kontextmenü"],["?","Diese Hilfe"]];
  const g=$("#kbGrid"); kb.forEach(([k,d])=>{ g.append(el("span",{class:"k",text:k}),el("span",{class:"d",text:d})); }); })();
(function(){ const lg=$("#legend"); OBJ_TYPES.forEach(t=>lg.append(
  el("span",{class:"lg"},[el("i",{style:`background:${OBJ_COLOR[t]}`}),document.createTextNode(t)]))); })();

/* =====================================================================
   SEED (first run) + INIT
   ===================================================================== */
function seedDemo(){
  //! Beispiel-Content — 3 feste Questgeber (West / Ost / Handelsverband). Frei editierbar.
  function mkTrader(id,name,fac){ const t=newTraderDef(); t.id=id; t.DisplayName=name; t.Faction=fac;
    t.LoyaltyLevels=[{Level:1,RequiredPlayerLevel:0,RequiredReputation:0,RequiredTurnover:0},
                     {Level:2,RequiredPlayerLevel:10,RequiredReputation:0.2,RequiredTurnover:250000},
                     {Level:3,RequiredPlayerLevel:20,RequiredReputation:0.45,RequiredTurnover:1000000}];
    t.__k="trader"; t._x=0; t._y=0; return t; }
  function mkZone(zid,cx,cy,cz,r){ return {ZoneId:zid,CenterX:cx,CenterY:cy,CenterZ:cz,Radius:r,Height:120}; }
  function mkObj(type,id,extra){ const o=Object.assign(newObjective(type),{ObjectiveId:id},extra||{});
    if(!(extra&&("Zone" in extra))) delete o.Zone; return o; }
  function mkQuest(id,trader,cat,title,desc,prereqs,objs,xp,cur,rep,repeatable){ const q=newQuestDef();
    q.QuestId=id; q.TraderId=trader; q.Category=cat; q.Title=title; q.Description=desc;
    if(prereqs&&prereqs.length) q.Prerequisites.RequiredCompletedQuests=prereqs;
    q.Objectives=objs; q.Rewards.PlayerExperience=xp; q.Rewards.Currency=cur; q.Rewards.TraderReputation=rep;
    if(repeatable) q.Repeatable=true; q.__k="quest"; q._x=0; q._y=0; return q; }

  state.traders=[
    mkTrader("west_command","Quartiermeister Rask","WEST"),
    mkTrader("east_command","Major Sokolow","EAST"),
    mkTrader("union_broker","Händler Dima","TRADE_UNION")
  ];
  state.quests=[
    mkQuest("west_001","west_command","STORY","Willkommen im Feld",
      "Erkunde das verlassene Militärlager im Westen.",
      [], [mkObj("DISCOVER","discover_camp",{Zone:mkZone("mil_camp_west",2035,290,7295,150)})],
      1500,5000,0.02),
    mkQuest("west_002","west_command","STORY","Säubere das Lager",
      "Das Lager wimmelt von Infizierten. Räum es aus.",
      ["west_001"], [mkObj("KILL","kill_infected",{Amount:5,TargetCategory:"INFECTED",Zone:mkZone("mil_camp_west",2035,290,7295,150),RequiredZones:["mil_camp_west"]})],
      2500,7500,0.02),
    mkQuest("west_003","west_command","STORY","Beweise sichern",
      "Sammle drei Munitionskisten aus dem Lager und liefere sie ab.",
      ["west_002"], [mkObj("COLLECT","collect_ammo",{Amount:3,ClassNames:["AmmoBox_545x39_20Rnd"],FoundInWorldRequired:true}),
                     mkObj("HANDOVER","handover_ammo",{Amount:3,ReferencesObjective:"collect_ammo"})],
      3000,10000,0.03),
    mkQuest("west_004","west_command","FACTION","Anerkennung des Westens",
      "Beweise deine Treue: Schalte eine feindliche Patrouille aus und melde dich zurück.",
      ["west_003"], [mkObj("KILL","kill_patrol",{Amount:5,TargetCategory:"AI"}),
                     mkObj("RETURN_TO_TRADER","report_west",{TraderId:"west_command",MustBeAlive:true})],
      5000,20000,0.05),
    mkQuest("east_001","east_command","STORY","Grenzkontrolle",
      "Erreiche den östlichen Kontrollpunkt und sichere ihn.",
      [], [mkObj("TRAVEL","travel_checkpoint",{Zone:mkZone("east_checkpoint",13805,37,3120,60)})],
      1200,4000,0.02),
    mkQuest("east_002","east_command","STORY","Der Informant",
      "Geleite den Informanten sicher zum Treffpunkt.",
      ["east_001"], [mkObj("ESCORT","escort_informant",{Zone:mkZone("east_meet",166,427,10420,40)})],
      3500,12000,0.03),
    mkQuest("east_003","east_command","FACTION","Anerkennung des Ostens",
      "Eine feindliche Gruppe ist eingedrungen. Fang sie ab und berichte.",
      ["east_002"], [mkObj("KILL","kill_intruders",{Amount:6,TargetCategory:"AI"}),
                     mkObj("RETURN_TO_TRADER","report_east",{TraderId:"east_command",MustBeAlive:true})],
      5000,20000,0.05),
    mkQuest("union_001","union_broker","SIDE","Die Mutation",
      "Ein mutierter Bär treibt im Nordwesten sein Unwesen. Erlege die Bestie.",
      [], [mkObj("KILL","kill_bear",{Amount:1,ClassNames:["Animal_UrsusArctos"],Zone:mkZone("bear_zone",1563,452,14070,300),RequiredZones:["bear_zone"]})],
      2000,8000,0.02),
    mkQuest("union_002","union_broker","SIDE","Vergrabener Schatz",
      "Alte Koordinaten führen zu einem Versteck. Finde die markierte Stelle.",
      [], [mkObj("DISCOVER","find_stash",{Zone:mkZone("stash_zone",6478,312,11050,10)})],
      1800,6000,0.02),
    mkQuest("union_003","union_broker","DAILY","Tägliche Beute",
      "Besorge zwei Goldbarren und liefere sie beim Handelsverband ab.",
      [], [mkObj("COLLECT","collect_gold",{Amount:2,ClassNames:["ExpansionGoldBar"]}),
           mkObj("HANDOVER","handover_gold",{Amount:2,ReferencesObjective:"collect_gold"})],
      1500,5000,0.02,true)
  ];
}

function init(){ if(!load()){ seedDemo(); autoLayout(); } else { renderAll(); } applyView();
  setTimeout(()=>{ if(!localStorage.getItem(STORE_KEY+"_seen")){ toast('Willkommen im <span class="mono">DME Quest Editor</span> — <b>?</b> für Hilfe'); localStorage.setItem(STORE_KEY+"_seen","1"); } },400);
}
init();
