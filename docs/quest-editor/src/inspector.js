// inspector.js
/* =====================================================================
   SELECTION + INSPECTOR
   ===================================================================== */
function selectNode(n){ closeContextMenu();
  const prev=selection; selection = n? {kind:n.__k,id:nodeId(n)} : null;
  // update sel classes cheaply
  nodeEls.forEach((elm,id)=>elm.classList.remove("sel"));
  if(n){ const e=nodeEls.get(nodeId(n)); if(e)e.classList.add("sel"); }
  renderInspector();
}
function selectedNode(){ if(!selection)return null;
  return selection.kind==="trader"?traderById(selection.id):questById(selection.id); }

function renderInspector(){
  const n=selectedNode();
  if(!n){ inspHead.classList.add("hidden"); inspEmpty.classList.remove("hidden");
    inspBody.querySelectorAll(".sect").forEach(s=>s.remove()); return; }
  inspEmpty.classList.add("hidden"); inspHead.classList.remove("hidden");
  $("#ih-tag").textContent = n.__k==="trader"?"TRADER":"QUEST";
  $("#ih-title").textContent = (n.__k==="trader"?n.DisplayName:n.Title)||"(ohne Titel)";
  $("#ih-sub").textContent = n.__k==="trader"
    ? nodeId(n)+" · "+(n.LoyaltyLevels||[]).length+" Loyalty-Level"
    : nodeId(n)+" · "+(n.Objectives||[]).length+" Obj · "+(n.Rewards.Items||[]).length+" Item-Rewards";
  inspBody.querySelectorAll(".sect").forEach(s=>s.remove());
  if(n.__k==="trader") buildTraderInspector(n); else buildQuestInspector(n);
}

// section helper
function section(title,count,open,build){ const s=el("div",{class:"sect"+(open?" open":"")});
  const head=el("div",{class:"sect-head"},[el("span",{class:"chev",text:"▶"}),el("span",{class:"s-name",text:title}),
    count!=null?el("span",{class:"s-count",text:count}):null]);
  head.addEventListener("click",()=>s.classList.toggle("open"));
  const body=el("div",{class:"sect-body"}); s.append(head,body); build(body); inspBody.append(s); return s; }

// bound controls
function txt(obj,key,label,ph,onEdit){ const f=el("div",{class:"f"});
  f.append(el("label",{text:label})); const i=el("input",{class:"ctrl",type:"text",value:obj[key]==null?"":obj[key]});
  if(ph)i.placeholder=ph; i.addEventListener("input",()=>{ obj[key]=i.value; (onEdit||liveRefresh)(); });
  f.append(i); return f; }
function area(obj,key,label){ const f=el("div",{class:"f"}); f.append(el("label",{text:label}));
  const i=el("textarea",{class:"ctrl"}); i.value=obj[key]||""; i.addEventListener("input",()=>{obj[key]=i.value;liveRefresh();});
  f.append(i); return f; }
function num(obj,key,label,step){ const f=el("div",{class:"f"}); f.append(el("label",{text:label}));
  const i=el("input",{class:"ctrl",type:"number",value:obj[key]}); if(step)i.step=step;
  i.addEventListener("input",()=>{ const v=i.value===""?0:(step?parseFloat(i.value):parseInt(i.value,10)); obj[key]=isNaN(v)?0:v; liveRefresh(); });
  f.append(i); return f; }
function sel(obj,key,label,opts,onEdit){ const f=el("div",{class:"f"}); f.append(el("label",{text:label}));
  const s=el("select",{class:"ctrl"}); opts.forEach(o=>{ const op=el("option",{value:o,text:o||"(keine)"}); if((obj[key]||"")===o)op.selected=true; s.append(op); });
  s.addEventListener("change",()=>{ obj[key]=s.value; (onEdit||liveRefresh)(); }); f.append(s); return f; }
function chk(obj,key,label){ const l=el("label",{class:"chk"}); const i=el("input",{type:"checkbox"});
  i.checked=!!obj[key]; i.addEventListener("change",()=>{obj[key]=i.checked;liveRefresh();});
  l.append(i,document.createTextNode(label)); return l; }
function strList(arr,label,ph){ const f=el("div",{class:"f"}); f.append(el("label",{text:label}));
  const list=el("div",{class:"taglist"});
  function redraw(){ list.innerHTML="";
    arr.forEach((v,idx)=>{ const tg=el("span",{class:"tag"},[document.createTextNode(v)]);
      tg.append(el("button",{text:"×",onClick:()=>{arr.splice(idx,1);redraw();liveRefresh();}})); list.append(tg); });
    const add=el("span",{class:"tag add"}); const inp=el("input",{type:"text",placeholder:ph||"hinzufügen…"});
    inp.addEventListener("keydown",e=>{ if(e.key==="Enter"&&inp.value.trim()){ arr.push(inp.value.trim()); redraw(); liveRefresh(); } });
    add.append(inp); list.append(add); }
  redraw(); f.append(list); return f; }

let liveTimer=null;
function liveRefresh(){ const n=selectedNode(); if(n){ n.__k=n.__k||(n.QuestId?"quest":"trader"); refreshNode(n);
  $("#ih-title").textContent=(n.__k==="trader"?n.DisplayName:n.Title)||"(ohne Titel)"; }
  runValidation(); clearTimeout(liveTimer); liveTimer=setTimeout(save,250); }

// ---- Trader inspector ----
function buildTraderInspector(t){
  section("Identität",null,true,b=>{
    b.append(idField(t,"id","Trader-ID",true));
    b.append(txt(t,"DisplayName","Anzeigename","z.B. Quartiermeister Rask"));
    b.append(hintFor("DisplayName mit führendem # = Stringtable-Key; sonst Klartext."));
    b.append(sel(t,"Faction","Fraktion",FACTIONS));
  });
  section("Loyalty-Level",(t.LoyaltyLevels||[]).length,true,b=>{
    (t.LoyaltyLevels||[]).forEach((lv,i)=>{ const card=itemCard(FACTION_COLOR[t.Faction]||"#8A8F98","Level "+lv.Level,
      ()=>{ t.LoyaltyLevels.splice(i,1); renderInspector(); liveRefresh(); });
      const bd=card.querySelector(".ic-body");
      bd.append(row2(num(lv,"Level","Level"),num(lv,"RequiredPlayerLevel","Min. Spielerlevel")));
      bd.append(row2(num(lv,"RequiredReputation","Min. Reputation","0.01"),num(lv,"RequiredTurnover","Min. Umsatz")));
      b.append(card); });
    b.append(el("button",{class:"add-btn",text:"+ Loyalty-Level",onClick:()=>{ const lv=(t.LoyaltyLevels[t.LoyaltyLevels.length-1]||{}).Level||0;
      t.LoyaltyLevels.push({Level:lv+1,RequiredPlayerLevel:0,RequiredReputation:0,RequiredTurnover:0}); renderInspector(); liveRefresh(); }}));
  });
}

// ---- Quest inspector ----
function buildQuestInspector(q){
  section("Identität",null,true,b=>{
    b.append(idField(q,"QuestId","Quest-ID",false));
    b.append(txt(q,"Title","Titel","z.B. Der verlorene Konvoi"));
    b.append(area(q,"Description","Beschreibung"));
    b.append(hintFor("Titel/Beschreibung mit # = Stringtable-Key; sonst Klartext (DME_Tasks_Loc zeigt verbatim)."));
    b.append(sel(q,"Category","Kategorie",CATEGORIES));
    b.append(traderSelect(q));
  });
  section("Flags",null,false,b=>{
    b.append(chk(q,"Repeatable","Wiederholbar"));
    b.append(chk(q,"FailOnDeath","Bei Tod fehlschlagen"));
    b.append(chk(q,"FailOnFactionChange","Bei Fraktionswechsel fehlschlagen"));
    b.append(num(q,"TimeLimit","Zeitlimit (Sek, -1 = keins)"));
  });
  section("Objectives",(q.Objectives||[]).length,true,b=>buildObjectives(q,b));
  section("Belohnungen",null,false,b=>buildRewards(q,b));
  section("Voraussetzungen",(q.Prerequisites.RequiredCompletedQuests||[]).length,false,b=>buildPrereqs(q,b));
  section("Freischaltungen",null,false,b=>{
    b.append(strList(q.Unlocks.QuestIds,"Quest-IDs freischalten","quest_x"));
    b.append(strList(q.Unlocks.MarketItems,"Market-Items freischalten","AK74"));
    b.append(strList(q.Unlocks.Keys,"Keys","key_x"));
    b.append(txt(q.Unlocks,"BossAccess","Boss-Zugang",""));
  });
}

function buildObjectives(q,b){
  (q.Objectives||[]).forEach((o,i)=>{ const color=OBJ_COLOR[o.Type]||"#7A8290";
    const card=itemCard(color,o.Type+" · "+o.ObjectiveId,()=>{ q.Objectives.splice(i,1); renderInspector(); liveRefresh(); });
    const bd=card.querySelector(".ic-body");
    const head=card.querySelector(".ic-head");
    // reorder controls
    const mv=el("span",{style:"display:flex;gap:2px;margin-right:2px"});
    mv.append(el("button",{class:"ic-x",text:"↑",title:"nach oben",style:"font-size:12px",onClick:e=>{e.stopPropagation();if(i>0){[q.Objectives[i-1],q.Objectives[i]]=[q.Objectives[i],q.Objectives[i-1]];renderInspector();liveRefresh();}}}));
    mv.append(el("button",{class:"ic-x",text:"↓",title:"nach unten",style:"font-size:12px",onClick:e=>{e.stopPropagation();if(i<q.Objectives.length-1){[q.Objectives[i+1],q.Objectives[i]]=[q.Objectives[i],q.Objectives[i+1]];renderInspector();liveRefresh();}}}));
    head.insertBefore(mv,head.querySelector(".ic-x"));
    // fields
    bd.append(row2(txt(o,"ObjectiveId","Objective-ID"),
      sel(o,"Type","Typ",OBJ_TYPES,()=>{ ensureObjFields(o); renderInspector(); liveRefresh(); })));
    bd.append(num(o,"Amount","Anzahl"));
    const extra=OBJ_FIELDS[o.Type]||[];
    if(extra.includes("TargetCategory")) bd.append(sel(o,"TargetCategory","Ziel-Kategorie",TARGET_CATS));
    if(extra.includes("ClassNames")){ o.ClassNames=o.ClassNames||[]; bd.append(strList(o.ClassNames,"Klassennamen","ZmbM_… / AK74")); }
    if(extra.includes("BossId")) bd.append(txt(o,"BossId","Boss-ID",""));
    if(extra.includes("ReferencesObjective")) bd.append(refObjSelect(o,q));
    if(extra.includes("AllowPartialHandover")) bd.append(chk(o,"AllowPartialHandover","Teilabgabe erlauben"));
    if(extra.includes("FoundInWorldRequired")) bd.append(chk(o,"FoundInWorldRequired","Muss in der Welt gefunden sein"));
    if(extra.includes("AllowedOrigins")){ o.AllowedOrigins=o.AllowedOrigins||[]; bd.append(strList(o.AllowedOrigins,"Erlaubte Herkünfte","WORLD_LOOT")); }
    if(extra.includes("MustBeAlive")) bd.append(chk(o,"MustBeAlive","Muss am Leben sein"));
    if(extra.includes("TraderId")) bd.append(txt(o,"TraderId","Ziel-Trader-ID",""));
    if(extra.includes("RequiredZones")){ o.RequiredZones=o.RequiredZones||[]; bd.append(strList(o.RequiredZones,"Erforderliche Zonen (ZoneId)","zone_id")); }
    if(extra.includes("Zone")){ o.Zone=o.Zone||newZone(); bd.append(zoneEditor(o.Zone)); }
    b.append(card);
  });
  const add=el("div",{class:"row2"});
  const s=el("select",{class:"ctrl"}); OBJ_TYPES.forEach(t=>s.append(el("option",{value:t,text:t})));
  add.append(s);
  add.append(el("button",{class:"mini-btn",style:"flex:0 0 auto",text:"+ Objective",onClick:()=>{ snapshot();
    q.Objectives.push(newObjective(s.value)); renderInspector(); liveRefresh(); }}));
  b.append(add);
}
function ensureObjFields(o){ const f=OBJ_FIELDS[o.Type]||[];
  if(f.includes("Zone")&&!o.Zone)o.Zone=newZone();
  if(f.includes("ClassNames")&&!o.ClassNames)o.ClassNames=[]; }
function refObjSelect(o,q){ const f=el("div",{class:"f"}); f.append(el("label",{text:"Referenziertes COLLECT-Objective"}));
  const s=el("select",{class:"ctrl"}); s.append(el("option",{value:"",text:"(keins)"}));
  (q.Objectives||[]).filter(x=>x.Type==="COLLECT").forEach(x=>{ const op=el("option",{value:x.ObjectiveId,text:x.ObjectiveId});
    if(o.ReferencesObjective===x.ObjectiveId)op.selected=true; s.append(op); });
  s.addEventListener("change",()=>{o.ReferencesObjective=s.value;liveRefresh();}); f.append(s); return f; }
function zoneEditor(z){ const wrap=el("div",{class:"item-card",style:"margin:8px 0"});
  wrap.append(el("div",{class:"ic-head"},[el("span",{class:"ic-dot",style:"background:#4AA8E0"}),el("span",{class:"ic-name",text:"ZONE"})]));
  const bd=el("div",{class:"ic-body"});
  bd.append(txt(z,"ZoneId","Zone-ID","mil_camp_west"));
  bd.append(row2(num(z,"CenterX","Center X","0.01"),num(z,"CenterY","Center Y","0.01")));
  bd.append(row2(num(z,"CenterZ","Center Z","0.01"),num(z,"Radius","Radius","0.1")));
  bd.append(num(z,"Height","Höhe","0.1")); wrap.append(bd); return wrap; }

function buildRewards(q,b){ const r=q.Rewards;
  b.append(row2(num(r,"PlayerExperience","Erfahrung (XP)"),num(r,"Currency","Währung")));
  b.append(row2(num(r,"TraderReputation","Trader-Reputation","0.01"),num(r,"SkillPoints","Skillpunkte")));
  b.append(num(r,"SeasonXp","Season-XP"));
  b.append(el("div",{class:"eyebrow",style:"margin-top:10px",text:"Item-Belohnungen"}));
  (r.Items||[]).forEach((it,i)=>{ const card=itemCard("#3DD68C",(it.ClassName||"Item")+" ×"+(it.Amount||1),
    ()=>{ r.Items.splice(i,1); renderInspector(); liveRefresh(); });
    const bd=card.querySelector(".ic-body");
    bd.append(row2(txt(it,"ClassName","Klassenname"),num(it,"Amount","Anzahl")));
    it.Attachments=it.Attachments||[]; bd.append(strList(it.Attachments,"Anbauteile","Mag_AK74_30Rnd"));
    b.append(card); });
  b.append(el("button",{class:"add-btn",text:"+ Item-Belohnung",onClick:()=>{ r.Items.push({ClassName:"",Amount:1,Attachments:[]}); renderInspector(); liveRefresh(); }}));
  b.append(el("div",{class:"eyebrow",style:"margin-top:12px",text:"Rivalen-Reputation"}));
  (r.RivalReputation||[]).forEach((rv,i)=>{ const card=itemCard("#D94A4A",(rv.TraderId||"Trader")+" "+(rv.Delta>=0?"+":"")+rv.Delta,
    ()=>{ r.RivalReputation.splice(i,1); renderInspector(); liveRefresh(); });
    const bd=card.querySelector(".ic-body"); bd.append(row2(traderIdSelect(rv,"TraderId","Trader"),num(rv,"Delta","Delta","0.01")));
    b.append(card); });
  b.append(el("button",{class:"add-btn",text:"+ Rivalen-Reputation",onClick:()=>{ r.RivalReputation.push({TraderId:"",Delta:0}); renderInspector(); liveRefresh(); }}));
}

function buildPrereqs(q,b){ const p=q.Prerequisites;
  b.append(el("div",{class:"eyebrow",text:"Quest-Kette"}));
  b.append(strList(p.RequiredCompletedQuests,"Erfordert abgeschlossene Quests","quest_x"));
  b.append(hintFor("Auch per Ziehen der Messing-Kante von Quest zu Quest setzbar."));
  b.append(strList(p.BlockedByCompletedQuests,"Blockiert durch abgeschlossene Quests","quest_x"));
  b.append(el("div",{class:"eyebrow",style:"margin-top:10px",text:"Anforderungen"}));
  b.append(row2(num(p,"MinimumPlayerLevel","Min. Spielerlevel"),num(p,"RequiredTraderLevel","Min. Trader-Level")));
  b.append(num(p,"MinimumTraderReputation","Min. Trader-Reputation","0.01"));
  b.append(sel(p,"RequiredFaction","Erforderliche Fraktion",["",...FACTIONS]));
  b.append(txt(p,"RequiredItem","Erforderliches Item (Klassenname)",""));
  b.append(row2(num(p,"FromHour","Ab Stunde (-1)"),num(p,"ToHour","Bis Stunde (-1)")));
  b.append(txt(p,"RequiredBossProgress","Erforderlicher Boss-Fortschritt",""));
}

// small building blocks
function idField(obj,key,label,isTrader){ const f=el("div",{class:"f"}); f.append(el("label",{text:label}));
  const i=el("input",{class:"ctrl mono",type:"text",value:obj[key]}); i.addEventListener("change",()=>{
    const nv=i.value.trim(); if(!nv){ i.value=obj[key]; return; }
    if(nv!==obj[key]){ if(idExists(nv,obj)){ toast("ID bereits vergeben",true); i.value=obj[key]; return; }
      snapshot(); renameNode(obj,key,nv); } });
  f.append(i); f.append(hintNode("Eindeutige ID = Dateiname beim Export. Umbenennen aktualisiert Verweise.")); return f; }
function idExists(id,except){ return state.traders.some(t=>t!==except&&t.id===id)||state.quests.some(q=>q!==except&&q.QuestId===id); }
function renameNode(obj,key,nv){ const old=obj[key];
  if(key==="id"){ state.quests.forEach(q=>{ if(q.TraderId===old)q.TraderId=nv; (q.Objectives||[]).forEach(o=>{if(o.TraderId===old)o.TraderId=nv;});
      (q.Rewards.RivalReputation||[]).forEach(rv=>{if(rv.TraderId===old)rv.TraderId=nv;}); }); obj.id=nv; }
  else { state.quests.forEach(q=>{ const pr=q.Prerequisites; ["RequiredCompletedQuests","BlockedByCompletedQuests"].forEach(k=>{
      const arr=pr[k]; const idx=arr.indexOf(old); if(idx>=0)arr[idx]=nv; }); (q.Unlocks.QuestIds||[]).forEach((v,ix,a)=>{if(v===old)a[ix]=nv;}); });
    obj.QuestId=nv; }
  selection={kind:obj.__k,id:nv}; renderAll(); save(); }

function traderSelect(q){ const f=el("div",{class:"f"}); f.append(el("label",{text:"Trader (Questgeber)"}));
  const s=el("select",{class:"ctrl"}); s.append(el("option",{value:"",text:"(kein Trader)"}));
  state.traders.forEach(t=>{ const op=el("option",{value:t.id,text:t.DisplayName+"  ["+t.id+"]"}); if(q.TraderId===t.id)op.selected=true; s.append(op); });
  s.addEventListener("change",()=>{ q.TraderId=s.value; liveRefresh(); }); f.append(s); return f; }
function traderIdSelect(obj,key,label){ const f=el("div",{class:"f"}); f.append(el("label",{text:label}));
  const s=el("select",{class:"ctrl"}); s.append(el("option",{value:"",text:"(kein)"}));
  state.traders.forEach(t=>{ const op=el("option",{value:t.id,text:t.id}); if(obj[key]===t.id)op.selected=true; s.append(op); });
  s.addEventListener("change",()=>{obj[key]=s.value;liveRefresh();}); f.append(s); return f; }

function itemCard(color,name,onDel){ const c=el("div",{class:"item-card"});
  const h=el("div",{class:"ic-head"},[el("span",{class:"ic-dot",style:`background:${color}`}),el("span",{class:"ic-name",text:name})]);
  h.append(el("button",{class:"ic-x",text:"×",title:"entfernen",onClick:e=>{e.stopPropagation();onDel();}}));
  c.append(h,el("div",{class:"ic-body"})); return c; }
function row2(a,b){ const r=el("div",{class:"row2"}); r.append(a,b); return r; }
function hintFor(t){ return el("div",{class:"f"},[el("div",{class:"hint",text:t})]); }
function hintNode(t){ return el("div",{class:"hint",text:t}); }
