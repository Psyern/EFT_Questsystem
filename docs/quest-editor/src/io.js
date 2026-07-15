// io.js
/* =====================================================================
   IMPORT / EXPORT
   ===================================================================== */
function isTraderJson(o){ return o && o.TraderId!==undefined && o.DisplayName!==undefined && o.QuestId===undefined; }
function isQuestJson(o){ return o && o.QuestId!==undefined; }

function importObjects(objs){ let nt=0,nq=0,skip=0; snapshot();
  objs.forEach(o=>{ if(isTraderJson(o)){ const t=normalizeTrader(o); const ex=traderById(t.id);
      if(ex){ Object.assign(ex,t); } else { t._x=snap(60+nt*280); t._y=snap(0); state.traders.push(t); } nt++; }
    else if(isQuestJson(o)){ const q=normalizeQuest(o); const ex=questById(q.QuestId);
      if(ex){ const x=ex._x,y=ex._y; Object.assign(ex,q); ex._x=x; ex._y=y; } else { state.quests.push(q); } nq++; }
    else skip++; });
  // place any quests without coords
  state.quests.forEach((q,i)=>{ if(q._x==null){ q._x=snap(60+(i%5)*280); q._y=snap(200+Math.floor(i/5)*220); } });
  renderAll(); autoLayout(); save();
  toast("Import: "+nt+" Trader, "+nq+" Quests"+(skip?(" ("+skip+" übersprungen)"):"")); }

function normalizeTrader(o){ const t=newTraderDef(); t.Version=o.Version||1; t.id=o.TraderId||t.id;
  t.DisplayName=o.DisplayName||""; t.Faction=o.Faction||"NEUTRAL";
  t.LoyaltyLevels=(o.LoyaltyLevels||[]).map(l=>({Level:l.Level||0,RequiredPlayerLevel:l.RequiredPlayerLevel||0,
    RequiredReputation:l.RequiredReputation||0,RequiredTurnover:l.RequiredTurnover||0})); t.__k="trader"; delete t.TraderId; return t; }
function normalizeQuest(o){ const q=newQuestDef(); Object.keys(newQuestDef()).forEach(k=>{ if(o[k]!==undefined)q[k]=o[k]; });
  q.QuestId=o.QuestId; q.Prerequisites=Object.assign(emptyPrereq(),o.Prerequisites||{});
  q.Rewards=Object.assign(emptyReward(),o.Rewards||{}); q.Unlocks=Object.assign(emptyUnlock(),o.Unlocks||{});
  q.Objectives=(o.Objectives||[]).map(x=>Object.assign(newObjective(x.Type||"TRAVEL"),x)); q.Choices=o.Choices||[]; q.__k="quest"; return q; }

// strip editor-only fields for export
function traderExport(t){ return {Version:t.Version||1,TraderId:t.id,DisplayName:t.DisplayName,Faction:t.Faction,
  LoyaltyLevels:t.LoyaltyLevels}; }
function questExport(q){ const o={Version:q.Version||1,QuestId:q.QuestId,TraderId:q.TraderId,Category:q.Category,
  Title:q.Title,Description:q.Description,Repeatable:q.Repeatable,FailOnDeath:q.FailOnDeath,
  FailOnFactionChange:q.FailOnFactionChange,TimeLimit:q.TimeLimit,Prerequisites:q.Prerequisites,
  Objectives:q.Objectives.map(cleanObjective),Rewards:q.Rewards,Unlocks:q.Unlocks,Choices:q.Choices||[]};
  return o; }
function cleanObjective(o){ const c=clone(o); if(c.Zone&&!c.Zone.ZoneId&&!c.Zone.CenterX&&!c.Zone.CenterZ&&!c.Zone.Radius) delete c.Zone; return c; }

function exportZip(){ const files=[];
  state.traders.forEach(t=>files.push(["DME_Tasks/Traders/"+t.id+".json",JSON.stringify(traderExport(t),null,"\t")+"\n"]));
  state.quests.forEach(q=>files.push(["DME_Tasks/Quests/"+q.QuestId+".json",JSON.stringify(questExport(q),null,"\t")+"\n"]));
  if(!files.length){ toast("Nichts zu exportieren",true); return; }
  const blob=makeZip(files); downloadBlob(blob,"DME_Tasks_"+ts()+".zip");
  toast("ZIP exportiert: "+files.length+" Dateien"); }

function ts(){ const d=new Date(); const p=x=>String(x).padStart(2,"0");
  return d.getFullYear()+p(d.getMonth()+1)+p(d.getDate())+"_"+p(d.getHours())+p(d.getMinutes()); }
function downloadBlob(blob,name){ const a=document.createElement("a"); a.href=URL.createObjectURL(blob); a.download=name;
  document.body.append(a); a.click(); setTimeout(()=>{URL.revokeObjectURL(a.href);a.remove();},500); }

// ----- Minimal ZIP writer (STORE method, no compression) with CRC32 -----
const CRC_TABLE=(()=>{ const t=new Uint32Array(256); for(let n=0;n<256;n++){ let c=n;
  for(let k=0;k<8;k++) c=c&1?(0xEDB88320^(c>>>1)):(c>>>1); t[n]=c>>>0; } return t; })();
function crc32(bytes){ let c=0xFFFFFFFF; for(let i=0;i<bytes.length;i++) c=CRC_TABLE[(c^bytes[i])&0xFF]^(c>>>8);
  return (c^0xFFFFFFFF)>>>0; }
function makeZip(files){ const enc=new TextEncoder(); const chunks=[]; const central=[]; let offset=0;
  const u16=v=>[v&0xFF,(v>>8)&0xFF]; const u32=v=>[v&0xFF,(v>>8)&0xFF,(v>>16)&0xFF,(v>>24)&0xFF];
  files.forEach(([name,content])=>{ const nameB=enc.encode(name); const data=enc.encode(content);
    const crc=crc32(data); const size=data.length;
    const local=[].concat(u32(0x04034b50),u16(20),u16(0),u16(0),u16(0),u16(0),u32(crc),u32(size),u32(size),u16(nameB.length),u16(0));
    const lh=new Uint8Array(local.length+nameB.length+data.length); lh.set(local,0); lh.set(nameB,local.length);
    lh.set(data,local.length+nameB.length); chunks.push(lh);
    const cen=[].concat(u32(0x02014b50),u16(20),u16(20),u16(0),u16(0),u16(0),u16(0),u32(crc),u32(size),u32(size),
      u16(nameB.length),u16(0),u16(0),u16(0),u16(0),u32(0),u32(offset));
    const ch=new Uint8Array(cen.length+nameB.length); ch.set(cen,0); ch.set(nameB,cen.length); central.push(ch);
    offset+=lh.length; });
  let cenSize=0; central.forEach(c=>cenSize+=c.length); const cenOffset=offset;
  const end=[].concat(u32(0x06054b50),u16(0),u16(0),u16(files.length),u16(files.length),u32(cenSize),u32(cenOffset),u16(0));
  const parts=[...chunks,...central,new Uint8Array(end)]; return new Blob(parts,{type:"application/zip"}); }

// ----- Import from files (JSON multi-select or ZIP) -----
$("#fileInput").addEventListener("change",async e=>{ const files=[...e.target.files]; if(!files.length)return;
  const collected=[];
  for(const f of files){ if(f.name.toLowerCase().endsWith(".zip")){ try{ const objs=await readZip(f); collected.push(...objs); }
      catch(err){ toast("ZIP-Fehler: "+err.message,true); } }
    else { try{ const txt=await f.text(); const o=JSON.parse(txt); collected.push(o); }catch(err){ toast("JSON-Fehler in "+f.name,true); } } }
  if(collected.length) importObjects(collected);
  e.target.value=""; });

async function readZip(file){ const buf=new Uint8Array(await file.arrayBuffer()); const dv=new DataView(buf.buffer);
  // find End Of Central Directory
  let eocd=-1; for(let i=buf.length-22;i>=0;i--){ if(dv.getUint32(i,true)===0x06054b50){ eocd=i; break; } }
  if(eocd<0) throw new Error("kein ZIP");
  const count=dv.getUint16(eocd+10,true); let ptr=dv.getUint32(eocd+16,true); const out=[]; const dec=new TextDecoder();
  for(let n=0;n<count;n++){ if(dv.getUint32(ptr,true)!==0x02014b50) break;
    const method=dv.getUint16(ptr+10,true); const compSize=dv.getUint32(ptr+20,true); const nameLen=dv.getUint16(ptr+28,true);
    const extraLen=dv.getUint16(ptr+30,true); const commentLen=dv.getUint16(ptr+32,true); const lho=dv.getUint32(ptr+42,true);
    const name=dec.decode(buf.subarray(ptr+46,ptr+46+nameLen));
    // local header
    const lNameLen=dv.getUint16(lho+26,true); const lExtra=dv.getUint16(lho+28,true);
    const dataStart=lho+30+lNameLen+lExtra; const dataEnd=dataStart+compSize; let bytes=buf.subarray(dataStart,dataEnd);
    let text;
    if(method===0){ text=dec.decode(bytes); }
    else if(method===8){ try{ const ds=new DecompressionStream("deflate-raw");
        const stream=new Blob([bytes]).stream().pipeThrough(ds); text=await new Response(stream).text(); }
      catch(err){ ptr+=46+nameLen+extraLen+commentLen; continue; } }
    else { ptr+=46+nameLen+extraLen+commentLen; continue; }
    if(name.toLowerCase().endsWith(".json")){ try{ out.push(JSON.parse(text)); }catch(e){} }
    ptr+=46+nameLen+extraLen+commentLen; }
  return out; }

function showCode(){ const bundle={traders:state.traders.map(traderExport),quests:state.quests.map(questExport)};
  const n=selectedNode(); let title="Bundle (alle Nodes)"; let obj=bundle;
  if(n){ if(n.__k==="trader"){ title="Traders/"+n.id+".json"; obj=traderExport(n); } else { title="Quests/"+n.QuestId+".json"; obj=questExport(n); } }
  $("#codeTitle").textContent=title; $("#codePre").textContent=JSON.stringify(obj,null,2); openModal("#modalCode"); }
