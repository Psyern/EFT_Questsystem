// model.js
// ---------- Utils ----------
function uid(prefix){ let n=1; const ids=new Set([...state.traders.map(t=>t.id),...state.quests.map(q=>q.QuestId)]);
  while(ids.has(prefix+n)) n++; return prefix+n; }
function clone(o){ return JSON.parse(JSON.stringify(o)); }

// ---------- Model factories ----------
function newTraderDef(){ return {Version:1,TraderId:uid("trader_"),DisplayName:"Neuer Trader",Faction:"NEUTRAL",
  LoyaltyLevels:[{Level:1,RequiredPlayerLevel:0,RequiredReputation:0.0,RequiredTurnover:0}]}; }
function newQuestDef(){ return {Version:1,QuestId:uid("quest_"),TraderId:"",Category:"SIDE",Title:"Neue Quest",
  Description:"",Repeatable:false,FailOnDeath:false,FailOnFactionChange:false,TimeLimit:-1,
  Prerequisites:emptyPrereq(),Objectives:[],Rewards:emptyReward(),Unlocks:emptyUnlock(),Choices:[]}; }
function emptyPrereq(){ return {MinimumPlayerLevel:0,RequiredFaction:"",MinimumTraderReputation:0,RequiredTraderLevel:0,
  RequiredCompletedQuests:[],BlockedByCompletedQuests:[],RequiredDecisions:[],BlockedByDecisions:[],RequiredSkills:[],
  RequiredSeasonLevel:0,RequiredBossProgress:"",RequiredItem:"",FromHour:-1,ToHour:-1}; }
function emptyReward(){ return {PlayerExperience:0,Currency:0,TraderReputation:0,RivalReputation:[],Items:[],SkillPoints:0,SeasonXp:0}; }
function emptyUnlock(){ return {QuestIds:[],MarketItems:[],Keys:[],BossAccess:""}; }
function newObjective(type){ const o={ObjectiveId:"obj_"+Math.random().toString(36).slice(2,6),Type:type||"TRAVEL",Amount:1};
  if(OBJ_FIELDS[o.Type].includes("Zone")) o.Zone=newZone(); return o; }
function newZone(){ return {ZoneId:"",CenterX:0,CenterY:0,CenterZ:0,Radius:50,Height:100}; }

function traderById(id){ return state.traders.find(t=>t.id===id); }
function questById(id){ return state.quests.find(q=>q.QuestId===id); }
// node accessors that hide the id-field difference
function nodeId(n){ return n.__k==="trader"?n.id:n.QuestId; }
