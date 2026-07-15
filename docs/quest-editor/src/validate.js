// validate.js
/* =====================================================================
   VALIDATION
   ===================================================================== */
function nodeIssues(id){ return validationCache.filter(v=>v.id===id); }
function runValidation(){
  const out=[]; const push=(sev,id,who,msg)=>out.push({sev,id,who,msg});
  const tIds={}, qIds={};
  state.traders.forEach(t=>{ if(!t.id) push("err",nodeId(t),"Trader","Trader-ID fehlt");
    else { if(tIds[t.id]) push("err",t.id,t.id,"Doppelte Trader-ID"); tIds[t.id]=1; }
    if(!(t.LoyaltyLevels||[]).length) push("warn",t.id,t.id,"Keine Loyalty-Level definiert"); });
  state.quests.forEach(q=>{ const id=q.QuestId;
    if(!id){ push("err","","Quest","Quest-ID fehlt"); return; }
    if(qIds[id]) push("err",id,id,"Doppelte Quest-ID"); qIds[id]=1;
    if(!q.Title) push("warn",id,id,"Kein Titel");
    if(!q.TraderId) push("warn",id,id,"Kein Trader zugewiesen");
    else if(!traderById(q.TraderId)) push("err",id,id,"Trader '"+q.TraderId+"' existiert nicht");
    if(!(q.Objectives||[]).length) push("warn",id,id,"Keine Objectives");
    (q.Prerequisites.RequiredCompletedQuests||[]).forEach(r=>{ if(!questById(r)) push("err",id,id,"Voraussetzung '"+r+"' existiert nicht"); });
    if(hasCycle(id)) push("err",id,id,"Zyklische Voraussetzung");
    (q.Objectives||[]).forEach(o=>{ const oid=o.ObjectiveId;
      if((o.Type==="KILL"||o.Type==="BOSS")&&!o.TargetCategory&&!(o.ClassNames||[]).length&&!o.BossId)
        push("warn",id,id,oid+": KILL ohne Filter (TargetCategory/ClassNames/BossId)");
      if(o.Type==="HANDOVER"){ const ref=o.ReferencesObjective; const match=(q.Objectives||[]).find(x=>x.ObjectiveId===ref&&x.Type==="COLLECT");
        if(!match) push("err",id,id,oid+": HANDOVER ohne passendes COLLECT-Objective"); }
      if((o.RequiredZones||[]).length){ if(!o.Zone||o.RequiredZones.indexOf(o.Zone.ZoneId)<0)
        push("warn",id,id,oid+": RequiredZones ohne passende Zone"); }
    });
  });
  validationCache=out; renderValidation();
}
function hasCycle(startId){ const seen=new Set(); const stack=[[startId,new Set()]];
  function dfs(id,path){ if(path.has(id))return true; path.add(id); const q=questById(id);
    if(q){ for(const r of (q.Prerequisites.RequiredCompletedQuests||[])){ if(dfs(r,new Set(path)))return true; } } return false; }
  return dfs(startId,new Set()); }
function renderValidation(){ const errs=validationCache.filter(v=>v.sev==="err").length, warns=validationCache.length-errs;
  const pill=$("#valPill"), pill2=$("#valPill2");
  const cls=errs?"err":(warns?"warn":"ok"); const txt=validationCache.length?(errs+"E "+warns+"W"):"OK";
  pill.className="count-pill "+cls; pill.textContent=txt; pill2.className="count-pill "+cls; pill2.textContent=txt;
  const body=$("#valBody"); body.innerHTML="";
  if(!validationCache.length){ body.append(el("div",{class:"vp-empty",text:"✓ Keine Probleme gefunden"})); return; }
  validationCache.forEach(v=>{ const row=el("div",{class:"vitem "+v.sev},[
    el("span",{class:"vmark",text:v.sev==="err"?"ERR":"WRN"}),
    el("span",{},[document.createTextNode(v.msg),el("div",{class:"vwho",text:v.who||""})])
  ]); row.addEventListener("click",()=>{ const n=questById(v.id)||traderById(v.id); if(n){ n.__k=n.__k||(n.QuestId?"quest":"trader"); selectNode(n); focusNode(n); } });
  body.append(row); });
}
function focusNode(n){ const r=canvas.getBoundingClientRect(); view.x=r.width/2-((n._x||0)+120)*view.k; view.y=r.height/2-((n._y||0)+40)*view.k; applyView(); save(); }
