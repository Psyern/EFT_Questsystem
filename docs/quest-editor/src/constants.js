// constants.js
// ---------- Constants ----------
const FACTIONS = ["NEUTRAL","WEST","EAST","BANDITS","MONSTERS","TRADE_UNION"];
const FACTION_COLOR = {NEUTRAL:"#8A8F98",WEST:"#4A90D9",EAST:"#D94A4A",BANDITS:"#E0982E",MONSTERS:"#9B59B6",TRADE_UNION:"#3DD68C"};
const CATEGORIES = ["STORY","SIDE","FACTION","BOSS","EXPEDITION","DAILY","WEEKLY"];
const CATEGORY_COLOR = {STORY:"#C9A227",SIDE:"#4AA8E0",FACTION:"#D94A4A",BOSS:"#F76808",EXPEDITION:"#66C2A5",DAILY:"#3DD68C",WEEKLY:"#8B7BE8"};
const OBJ_TYPES = ["KILL","BOSS","COLLECT","HANDOVER","DELIVER","TRAVEL","DISCOVER","MARK","STASH","INTERACT","SURVIVE","RETURN_TO_TRADER","CRAFT","ESCORT","DEFEND","USE_ITEM","SIGNAL","GROUP","EXTRACT"];
const OBJ_COLOR = {KILL:"#E5484D",BOSS:"#F76808",COLLECT:"#3DD68C",HANDOVER:"#E0982E",DELIVER:"#E0982E",TRAVEL:"#4AA8E0",DISCOVER:"#2BC4C4",MARK:"#8B7BE8",STASH:"#8B7BE8",INTERACT:"#C9A227",SURVIVE:"#D08770",RETURN_TO_TRADER:"#B48EAD",CRAFT:"#A3BE8C",ESCORT:"#F0A868",DEFEND:"#F0A868",USE_ITEM:"#C9A227",SIGNAL:"#56B6C2",GROUP:"#7A8290",EXTRACT:"#66C2A5"};
const TARGET_CATS = ["","INFECTED","PLAYER","ANIMAL","AI"];
const STORE_KEY = "dme_quest_editor_v2";

// which extra fields each objective type surfaces (beyond Id/Type/Amount)
const OBJ_FIELDS = {
  KILL:["TargetCategory","ClassNames","Zone","RequiredZones","BossId"],
  BOSS:["BossId","ClassNames","Zone"],
  COLLECT:["ClassNames","FoundInWorldRequired","AllowedOrigins"],
  HANDOVER:["ReferencesObjective","AllowPartialHandover"],
  DELIVER:["ClassNames","ReferencesObjective","Zone"],
  TRAVEL:["Zone"],
  DISCOVER:["Zone"],
  MARK:["Zone","ClassNames"],
  STASH:["ClassNames","Zone"],
  INTERACT:["ClassNames","Zone"],
  SURVIVE:["Zone"],
  RETURN_TO_TRADER:["TraderId","MustBeAlive"],
  CRAFT:["ClassNames"],
  ESCORT:["Zone"],
  DEFEND:["Zone"],
  USE_ITEM:["ClassNames","Zone"],
  SIGNAL:["Zone"],
  GROUP:[],
  EXTRACT:["Zone"]
};
