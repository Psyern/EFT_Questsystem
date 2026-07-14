//! Text-Bau-Helfer der Tasks-UI: deutsche Anzeige-Texte aus Sync-Modellen (reine Anzeige-Logik).
//! Wird von TaskDetail, TaskListEntry und den Dialogen gemeinsam genutzt.
class DME_Tasks_UITextUtil {
	//! Lokalisiertes State-Label fuer den State-Chip (aufgeloest, kein '#'-Key).
	static string StateLabel(int state) {
		return DME_Tasks_Loc.Resolve(DME_Tasks_EnumUtil.StateDisplayKey(state));
	}

	//! ARGB-Farbe des State-Chips (Widget.SetColor).
	static int StateColor(int state) {
		if (state == EDME_Tasks_QuestState.LOCKED) return ARGB(255, 90, 90, 100);
		if (state == EDME_Tasks_QuestState.AVAILABLE) return ARGB(255, 185, 148, 50);
		if (state == EDME_Tasks_QuestState.ACTIVE) return ARGB(255, 46, 96, 140);
		if (state == EDME_Tasks_QuestState.READY_TO_TURN_IN) return ARGB(255, 60, 140, 70);
		if (state == EDME_Tasks_QuestState.COMPLETED) return ARGB(255, 58, 92, 66);
		if (state == EDME_Tasks_QuestState.FAILED) return ARGB(255, 150, 45, 45);
		if (state == EDME_Tasks_QuestState.EXPIRED) return ARGB(255, 160, 90, 40);
		if (state == EDME_Tasks_QuestState.ABANDONED) return ARGB(255, 110, 70, 70);
		if (state == EDME_Tasks_QuestState.COOLDOWN) return ARGB(255, 70, 110, 120);
		return ARGB(255, 90, 90, 100);
	}

	//! Lokalisiertes Kategorie-Label aus dem Sync-String (unbekannt -> Rohstring, kein Log-Spam).
	static string CategoryLabel(string category) {
		return DME_Tasks_Loc.Resolve(DME_Tasks_EnumUtil.CategoryDisplayKeyFromString(category));
	}

	//! Orts-Hinweis fuer die Listenzeile: erste Zone (ZoneId) aus den Objectives, sonst "".
	static string QuestLocationLabel(DME_Tasks_QuestSyncEntry entry) {
		if (!entry || !entry.Objectives) {
			return "";
		}
		for (int i = 0; i < entry.Objectives.Count(); i++) {
			DME_Tasks_ObjectiveDef def = entry.Objectives.Get(i);
			if (!def) {
				continue;
			}
			string zone = ZoneLabel(def);
			if (zone != "") {
				return zone;
			}
		}
		return "";
	}

	//! Zonen-Anzeige eines Objectives: Zone.ZoneId, sonst erste RequiredZone, sonst "".
	static string ZoneLabel(DME_Tasks_ObjectiveDef def) {
		if (!def) {
			return "";
		}
		if (def.Zone && def.Zone.ZoneId != "") {
			return def.Zone.ZoneId;
		}
		if (def.RequiredZones && def.RequiredZones.Count() > 0) {
			return def.RequiredZones.Get(0);
		}
		return "";
	}

	//! Anzeigename eines Items aus der Config (CfgVehicles/CfgWeapons/CfgMagazines/CfgAmmo),
	//! Fallback: Klassenname. ConfigGetText loest $STR-Lokalisierung bereits auf.
	static string ItemDisplayName(string className) {
		if (className == "") {
			return "";
		}
		if (!g_Game) {
			return className;
		}
		string displayName;
		string path = "CfgVehicles " + className + " displayName";
		if (g_Game.ConfigIsExisting(path)) {
			if (g_Game.ConfigGetText(path, displayName) && displayName != "") {
				return displayName;
			}
		}
		path = "CfgWeapons " + className + " displayName";
		if (g_Game.ConfigIsExisting(path)) {
			if (g_Game.ConfigGetText(path, displayName) && displayName != "") {
				return displayName;
			}
		}
		path = "CfgMagazines " + className + " displayName";
		if (g_Game.ConfigIsExisting(path)) {
			if (g_Game.ConfigGetText(path, displayName) && displayName != "") {
				return displayName;
			}
		}
		path = "CfgAmmo " + className + " displayName";
		if (g_Game.ConfigIsExisting(path)) {
			if (g_Game.ConfigGetText(path, displayName) && displayName != "") {
				return displayName;
			}
		}
		return className;
	}

	//! Ziel-Beschreibung (ClassNames -> Anzeigenamen, sonst TargetCategory, sonst BossId).
	static string TargetLabel(DME_Tasks_ObjectiveDef def) {
		if (!def) {
			return "";
		}
		if (def.ClassNames && def.ClassNames.Count() > 0) {
			string joined = ItemDisplayName(def.ClassNames.Get(0));
			if (def.ClassNames.Count() > 1) {
				joined = joined + " / " + ItemDisplayName(def.ClassNames.Get(1));
			}
			if (def.ClassNames.Count() > 2) {
				joined = joined + " ...";
			}
			return joined;
		}
		if (def.TargetCategory != "") {
			return TargetCategoryLabel(def.TargetCategory);
		}
		if (def.BossId != "") {
			return def.BossId;
		}
		return "";
	}

	//! Lokalisierte Bezeichnung gaengiger Zielkategorien (Konvention CONTRACTS 6.7).
	static string TargetCategoryLabel(string category) {
		if (category == "PLAYER") return DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_TARGET_PLAYERS);
		if (category == "INFECTED") return DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_TARGET_INFECTED);
		if (category == "ANIMAL") return DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_TARGET_ANIMALS);
		if (category == "BANDIT_AI") return DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_TARGET_BANDITS);
		return category;
	}

	//! Sekunden -> "2 h 5 min" / "3 min" / "45 s".
	static string FormatDuration(int seconds) {
		if (seconds <= 0) {
			return "0 s";
		}
		if (seconds >= 3600) {
			int hours = seconds / 3600;
			int restMinutes = (seconds % 3600) / 60;
			if (restMinutes == 0) {
				return hours.ToString() + " h";
			}
			return hours.ToString() + " h " + restMinutes.ToString() + " min";
		}
		if (seconds >= 60) {
			int minutes = seconds / 60;
			int restSeconds = seconds % 60;
			if (restSeconds == 0) {
				return minutes.ToString() + " min";
			}
			return minutes.ToString() + " min " + restSeconds.ToString() + " s";
		}
		return seconds.ToString() + " s";
	}

	//! Deutsche Objective-Beschreibung aus der Definition (Type + Amount + Ziel + Zone + Filter).
	static string ObjectiveText(DME_Tasks_ObjectiveDef def) {
		if (!def) {
			return "";
		}

		string target = TargetLabel(def);
		string zone = ZoneLabel(def);
		string amountText = def.Amount.ToString();
		int objType = DME_Tasks_EnumUtil.ObjectiveTypeFromString(def.Type);
		string text = "";

		if (objType == EDME_Tasks_ObjectiveType.KILL) {
			if (target == "") {
				target = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_TARGET_ENEMY);
			}
			text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_KILL, amountText, target);
		} else if (objType == EDME_Tasks_ObjectiveType.COLLECT) {
			text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_COLLECT, amountText, target);
		} else if (objType == EDME_Tasks_ObjectiveType.HANDOVER) {
			text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_HANDOVER, amountText, target);
		} else if (objType == EDME_Tasks_ObjectiveType.DELIVER) {
			text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_DELIVER, amountText, target);
			if (zone != "") {
				text = text + DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_TO_ZONE, zone);
			}
		} else if (objType == EDME_Tasks_ObjectiveType.TRAVEL) {
			if (zone != "") {
				text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_TRAVEL_ZONE, zone);
			} else {
				text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_TRAVEL);
			}
		} else if (objType == EDME_Tasks_ObjectiveType.DISCOVER) {
			if (zone != "") {
				text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_DISCOVER_ZONE, zone);
			} else {
				text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_DISCOVER);
			}
		} else if (objType == EDME_Tasks_ObjectiveType.MARK) {
			text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_MARK);
			if (target != "") {
				text = text + ": " + target;
			}
		} else if (objType == EDME_Tasks_ObjectiveType.STASH) {
			text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_STASH, amountText, target);
		} else if (objType == EDME_Tasks_ObjectiveType.INTERACT) {
			if (target == "") {
				target = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_TARGET_DEFAULT);
			}
			text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_INTERACT, target);
			if (def.Amount > 1) {
				text = text + " (" + amountText + "x)";
			}
		} else if (objType == EDME_Tasks_ObjectiveType.SURVIVE) {
			text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_SURVIVE, FormatDuration(def.Amount));
		} else if (objType == EDME_Tasks_ObjectiveType.RETURN_TO_TRADER) {
			if (def.MustBeAlive) {
				text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_RETURN_ALIVE);
			} else {
				text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_RETURN);
			}
		} else if (objType == EDME_Tasks_ObjectiveType.CRAFT) {
			text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_CRAFT, amountText, target);
		} else if (objType == EDME_Tasks_ObjectiveType.ESCORT) {
			text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_ESCORT);
			if (zone != "") {
				text = text + DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_TO_ZONE, zone);
			}
		} else if (objType == EDME_Tasks_ObjectiveType.DEFEND) {
			if (zone != "") {
				text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_DEFEND_ZONE, zone);
			} else {
				text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_DEFEND);
			}
			text = text + " (" + FormatDuration(def.Amount) + ")";
		} else if (objType == EDME_Tasks_ObjectiveType.USE_ITEM) {
			if (target == "") {
				target = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_TARGET_ITEM);
			}
			text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_USE_ITEM, target);
		} else if (objType == EDME_Tasks_ObjectiveType.SIGNAL) {
			text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_SIGNAL);
			if (zone != "") {
				text = text + " (" + zone + ")";
			}
		} else if (objType == EDME_Tasks_ObjectiveType.GROUP) {
			text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_GROUP);
		} else if (objType == EDME_Tasks_ObjectiveType.BOSS) {
			if (def.BossId != "") {
				text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_BOSS_NAMED, def.BossId);
			} else {
				text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_BOSS);
			}
		} else if (objType == EDME_Tasks_ObjectiveType.EXTRACT) {
			if (zone != "") {
				text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_EXTRACT_ZONE, zone);
			} else {
				text = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_EXTRACT);
			}
			if (def.Amount > 1) {
				text = text + DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_EXTRACT_HOLD, FormatDuration(def.Amount));
			}
		} else {
			text = def.Type + " x " + amountText;
		}

		return text + ObjectiveFilterSuffix(def);
	}

	//! Kurze Filter-Zusaetze (Distanz, Schalldaempfer, Trefferzone, Zeitfenster).
	static string ObjectiveFilterSuffix(DME_Tasks_ObjectiveDef def) {
		string suffix = "";
		if (def.MinimumDistance > 0) {
			int minDistance = Math.Round(def.MinimumDistance);
			suffix = suffix + DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_SUFFIX_MIN_DIST, minDistance.ToString());
		}
		if (def.MaximumDistance > 0) {
			int maxDistance = Math.Round(def.MaximumDistance);
			suffix = suffix + DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_SUFFIX_MAX_DIST, maxDistance.ToString());
		}
		if (def.SuppressorRequired) {
			suffix = suffix + DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_SUFFIX_SUPPRESSOR);
		}
		if (def.HitZone != "") {
			suffix = suffix + DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_SUFFIX_HITZONE, def.HitZone);
		}
		if (def.FromHour >= 0 && def.ToHour >= 0) {
			suffix = suffix + DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_OBJ_SUFFIX_TIME, def.FromHour.ToString(), def.ToHour.ToString());
		}
		return suffix;
	}

	//! Belohnungs-Zeilen (XP/Geld/Rep/Rival-Rep/Items/Skill/Season) als RichText mit <br/>.
	static string RewardText(DME_Tasks_QuestSyncEntry entry) {
		if (!entry || !entry.Rewards) {
			return DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_NONE);
		}

		DME_Tasks_RewardDef rewards = entry.Rewards;
		string result = "";

		if (rewards.PlayerExperience > 0) {
			result = AppendLine(result, DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_REWARD_XP, rewards.PlayerExperience.ToString()));
		}
		if (rewards.Currency > 0) {
			result = AppendLine(result, DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_REWARD_CURRENCY, rewards.Currency.ToString()));
		}
		if (rewards.TraderReputation != 0) {
			string repValue = rewards.TraderReputation.ToString();
			if (rewards.TraderReputation > 0) {
				repValue = "+ " + repValue;
			}
			result = AppendLine(result, DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_REWARD_REPUTATION, repValue));
		}
		if (rewards.RivalReputation) {
			for (int r = 0; r < rewards.RivalReputation.Count(); r++) {
				DME_Tasks_RivalRep rival = rewards.RivalReputation.Get(r);
				if (!rival || rival.Delta == 0) {
					continue;
				}
				string rivalDelta = rival.Delta.ToString();
				if (rival.Delta > 0) {
					rivalDelta = "+" + rivalDelta;
				}
				result = AppendLine(result, DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_REWARD_RIVAL_REP, rival.TraderId, rivalDelta));
			}
		}
		if (rewards.Items) {
			for (int i = 0; i < rewards.Items.Count(); i++) {
				DME_Tasks_ItemReward item = rewards.Items.Get(i);
				if (!item || item.ClassName == "") {
					continue;
				}
				result = AppendLine(result, DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_REWARD_ITEM, item.Amount.ToString(), ItemDisplayName(item.ClassName)));
			}
		}
		if (rewards.SkillPoints > 0) {
			result = AppendLine(result, DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_REWARD_SKILLPOINTS, rewards.SkillPoints.ToString()));
		}
		if (rewards.SeasonXp > 0) {
			result = AppendLine(result, DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_REWARD_SEASON_XP, rewards.SeasonXp.ToString()));
		}

		if (entry.Choices && entry.Choices.Count() > 0) {
			result = AppendLine(result, DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_REWARD_CHOICE_HINT));
		}

		if (result == "") {
			return DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_NONE);
		}
		return result;
	}

	//! "Penalties for failure"-Zeilen (FailOnDeath / TimeLimit / FailOnFactionChange).
	static string PenaltyText(DME_Tasks_QuestSyncEntry entry) {
		if (!entry) {
			return DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_NONE);
		}
		string result = "";
		if (entry.FailOnDeath) {
			result = AppendLine(result, DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_PENALTY_DEATH));
		}
		if (entry.TimeLimit > 0) {
			result = AppendLine(result, DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_PENALTY_TIMELIMIT, FormatDuration(entry.TimeLimit)));
		}
		if (entry.FailOnFactionChange) {
			result = AppendLine(result, DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_PENALTY_FACTION));
		}
		if (result == "") {
			return DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_NONE);
		}
		return result;
	}

	//! RichText-Zeilen mit <br/> verketten.
	static string AppendLine(string current, string line) {
		if (current == "") {
			return line;
		}
		return current + "<br/>" + line;
	}
}

//! Detail-Ansicht einer Quest (Paging: task_list_panel versteckt, task_detail_panel sichtbar).
//! Wird einmalig vom Menue in das task_detail_panel instanziert (CreateWidgets + SetHandler(this),
//! Muster Vanilla PlayerListEntryScriptedWidget.c) und bei jedem Cache-Update via
//! RefreshFromCache() neu aus dem Cache gefuellt — dadurch aktualisieren RPC_ObjectiveProgress/
//! RPC_QuestStateChanged Fortschrittsbalken und Button-Zustaende live.
//! Fortschrittsbalken = Eigenbau aus zwei PanelWidgets: objective_bar_bg_N (Hintergrund) +
//! objective_bar_fill_N (Fuellung, SetSize(fraction, 1.0)) — CONTRACTS 6.6.
class DME_Tasks_TaskDetail : ScriptedWidgetEventHandler {
	//! Anzahl vorgebauter Objective-Zeilen in task_detail.layout (objective_row_0..7).
	static const int OBJECTIVE_ROW_COUNT = 8;

	protected DME_Tasks_Menu m_DME_Menu;
	protected Widget m_DME_Root;
	protected ButtonWidget m_DME_BackButton;
	protected TextWidget m_DME_TitleText;
	protected Widget m_DME_StatePanel;
	protected TextWidget m_DME_StateText;
	protected TextWidget m_DME_CategoryText;
	protected RichTextWidget m_DME_BriefingText;
	protected RichTextWidget m_DME_RewardsText;
	protected RichTextWidget m_DME_PenaltiesText;
	protected TextWidget m_DME_LockText;

	protected ref array<Widget> m_DME_ObjectiveRows;
	protected ref array<TextWidget> m_DME_ObjectiveChecks;
	protected ref array<TextWidget> m_DME_ObjectiveTexts;
	protected ref array<TextWidget> m_DME_ObjectiveCounts;
	protected ref array<Widget> m_DME_ObjectiveBarFills;

	protected ButtonWidget m_DME_AcceptButton;
	protected ButtonWidget m_DME_TurninButton;
	protected ButtonWidget m_DME_ClaimButton;
	protected ButtonWidget m_DME_ChoiceButton;
	protected ButtonWidget m_DME_HandoverButton;
	protected ButtonWidget m_DME_TrackButton;
	protected ButtonWidget m_DME_AbandonButton;
	protected ButtonWidget m_DME_ReplaceButton;

	void DME_Tasks_TaskDetail(DME_Tasks_Menu menu, Widget parent) {
		m_DME_Menu = menu;
		m_DME_ObjectiveRows = new array<Widget>();
		m_DME_ObjectiveChecks = new array<TextWidget>();
		m_DME_ObjectiveTexts = new array<TextWidget>();
		m_DME_ObjectiveCounts = new array<TextWidget>();
		m_DME_ObjectiveBarFills = new array<Widget>();

		if (!g_Game || !parent) {
			return;
		}

		m_DME_Root = g_Game.GetWorkspace().CreateWidgets(DME_Tasks_UIConst.LAYOUT_TASK_DETAIL, parent);
		if (!m_DME_Root) {
			return;
		}

		m_DME_BackButton = ButtonWidget.Cast(m_DME_Root.FindAnyWidget("detail_back_button"));
		m_DME_TitleText = TextWidget.Cast(m_DME_Root.FindAnyWidget("detail_title_text"));
		m_DME_StatePanel = m_DME_Root.FindAnyWidget("detail_state_panel");
		m_DME_StateText = TextWidget.Cast(m_DME_Root.FindAnyWidget("detail_state_text"));
		m_DME_CategoryText = TextWidget.Cast(m_DME_Root.FindAnyWidget("detail_category_text"));
		m_DME_BriefingText = RichTextWidget.Cast(m_DME_Root.FindAnyWidget("detail_briefing_text"));
		m_DME_RewardsText = RichTextWidget.Cast(m_DME_Root.FindAnyWidget("detail_rewards_text"));
		m_DME_PenaltiesText = RichTextWidget.Cast(m_DME_Root.FindAnyWidget("detail_penalties_text"));
		m_DME_LockText = TextWidget.Cast(m_DME_Root.FindAnyWidget("detail_lock_text"));

		for (int i = 0; i < OBJECTIVE_ROW_COUNT; i++) {
			string indexSuffix = i.ToString();
			m_DME_ObjectiveRows.Insert(m_DME_Root.FindAnyWidget("objective_row_" + indexSuffix));
			m_DME_ObjectiveChecks.Insert(TextWidget.Cast(m_DME_Root.FindAnyWidget("objective_check_" + indexSuffix)));
			m_DME_ObjectiveTexts.Insert(TextWidget.Cast(m_DME_Root.FindAnyWidget("objective_text_" + indexSuffix)));
			m_DME_ObjectiveCounts.Insert(TextWidget.Cast(m_DME_Root.FindAnyWidget("objective_count_" + indexSuffix)));
			m_DME_ObjectiveBarFills.Insert(m_DME_Root.FindAnyWidget("objective_bar_fill_" + indexSuffix));
		}

		m_DME_AcceptButton = ButtonWidget.Cast(m_DME_Root.FindAnyWidget("detail_accept_button"));
		m_DME_TurninButton = ButtonWidget.Cast(m_DME_Root.FindAnyWidget("detail_turnin_button"));
		m_DME_ClaimButton = ButtonWidget.Cast(m_DME_Root.FindAnyWidget("detail_claim_button"));
		m_DME_ChoiceButton = ButtonWidget.Cast(m_DME_Root.FindAnyWidget("detail_choice_button"));
		m_DME_HandoverButton = ButtonWidget.Cast(m_DME_Root.FindAnyWidget("detail_handover_button"));
		m_DME_TrackButton = ButtonWidget.Cast(m_DME_Root.FindAnyWidget("detail_track_button"));
		m_DME_AbandonButton = ButtonWidget.Cast(m_DME_Root.FindAnyWidget("detail_abandon_button"));
		m_DME_ReplaceButton = ButtonWidget.Cast(m_DME_Root.FindAnyWidget("detail_replace_button"));

		m_DME_Root.SetHandler(this);
	}

	//! Detail aus dem Cache neu fuellen (Menue ruft das aus OnRefreshDetail bei jedem Update).
	void RefreshFromCache() {
		if (!m_DME_Root || !m_DME_Menu) {
			return;
		}

		string questId = m_DME_Menu.GetSelectedQuestId();
		if (questId == "") {
			return;
		}

		DME_Tasks_ClientCache cache = m_DME_Menu.GetCache();
		if (!cache) {
			return;
		}

		DME_Tasks_QuestSyncEntry entry = cache.GetQuest(questId);
		if (!entry) {
			return;
		}

		if (m_DME_TitleText) {
			string title = entry.Title;
			if (title == "") {
				title = entry.QuestId;
			}
			m_DME_TitleText.SetText(title);
		}

		if (m_DME_StateText) {
			m_DME_StateText.SetText(DME_Tasks_UITextUtil.StateLabel(entry.State));
		}
		if (m_DME_StatePanel) {
			m_DME_StatePanel.SetColor(DME_Tasks_UITextUtil.StateColor(entry.State));
		}

		if (m_DME_CategoryText) {
			string categoryText = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_CATEGORY_PREFIX, DME_Tasks_UITextUtil.CategoryLabel(entry.Category));
			if (entry.Repeatable) {
				categoryText = categoryText + DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_REPEATABLE);
			}
			m_DME_CategoryText.SetText(categoryText);
		}

		if (m_DME_BriefingText) {
			m_DME_BriefingText.SetText(entry.Description);
		}

		UpdateObjectiveRows(entry, cache);

		if (m_DME_RewardsText) {
			m_DME_RewardsText.SetText(DME_Tasks_UITextUtil.RewardText(entry));
		}
		if (m_DME_PenaltiesText) {
			m_DME_PenaltiesText.SetText(DME_Tasks_UITextUtil.PenaltyText(entry));
		}

		if (m_DME_LockText) {
			bool showLock = false;
			if (entry.State == EDME_Tasks_QuestState.LOCKED && entry.LockReasonKey != "") {
				showLock = true;
				string lockReason = DME_Tasks_Loc.Resolve(entry.LockReasonKey, entry.LockReasonP1, entry.LockReasonP2);
				m_DME_LockText.SetText(DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_LOCKED_REASON, lockReason));
			}
			m_DME_LockText.Show(showLock);
		}

		UpdateButtons(entry, cache);
	}

	//! Widget-Baum zerstoeren (Menue-Abbau).
	void Destroy() {
		if (m_DME_Root) {
			m_DME_Root.Unlink();
			m_DME_Root = null;
		}
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		if (!m_DME_Menu) {
			return false;
		}

		if (w == m_DME_BackButton) {
			m_DME_Menu.ShowList();
			m_DME_Menu.Refresh();
			return true;
		}

		string questId = m_DME_Menu.GetSelectedQuestId();
		if (questId == "") {
			return false;
		}

		if (w == m_DME_AcceptButton) {
			SendQuestRpc("RPC_AcceptQuest", questId);
			return true;
		}
		if (w == m_DME_TurninButton) {
			SendQuestRpc("RPC_RequestTurnIn", questId);
			return true;
		}
		if (w == m_DME_ClaimButton) {
			SendQuestRpc("RPC_ClaimReward", questId);
			return true;
		}
		if (w == m_DME_AbandonButton) {
			SendQuestRpc("RPC_AbandonQuest", questId);
			return true;
		}
		if (w == m_DME_HandoverButton) {
			m_DME_Menu.OpenHandoverDialog(questId);
			return true;
		}
		if (w == m_DME_ChoiceButton) {
			m_DME_Menu.OpenChoiceDialog(questId);
			return true;
		}
		if (w == m_DME_ReplaceButton) {
			m_DME_Menu.OpenReplaceDialog(questId);
			return true;
		}
		if (w == m_DME_TrackButton) {
			ToggleTracked(questId);
			return true;
		}

		return false;
	}

	// ------------------------------------------------------------------
	// Intern
	// ------------------------------------------------------------------

	//! Objective-Zeilen: Haekchen, Beschreibung, n/m-Zaehler, Fuellbalken (SetSize-Fraction).
	protected void UpdateObjectiveRows(DME_Tasks_QuestSyncEntry entry, DME_Tasks_ClientCache cache) {
		int objectiveCount = 0;
		if (entry.Objectives) {
			objectiveCount = entry.Objectives.Count();
		}

		for (int i = 0; i < OBJECTIVE_ROW_COUNT; i++) {
			Widget row = m_DME_ObjectiveRows.Get(i);
			if (!row) {
				continue;
			}
			if (i >= objectiveCount) {
				row.Show(false);
				continue;
			}
			DME_Tasks_ObjectiveDef def = entry.Objectives.Get(i);
			if (!def) {
				row.Show(false);
				continue;
			}
			row.Show(true);
			FillObjectiveRow(i, def, entry, cache);
		}

		// Mehr Objectives als vorgebaute Zeilen -> letzte Zeile als Sammel-Hinweis.
		if (objectiveCount > OBJECTIVE_ROW_COUNT) {
			int extraCount = objectiveCount - OBJECTIVE_ROW_COUNT + 1;
			TextWidget lastText = m_DME_ObjectiveTexts.Get(OBJECTIVE_ROW_COUNT - 1);
			if (lastText) {
				lastText.SetText(DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.MENU_MORE_OBJECTIVES, extraCount.ToString()));
			}
			TextWidget lastCheck = m_DME_ObjectiveChecks.Get(OBJECTIVE_ROW_COUNT - 1);
			if (lastCheck) {
				lastCheck.SetText("");
			}
			TextWidget lastCount = m_DME_ObjectiveCounts.Get(OBJECTIVE_ROW_COUNT - 1);
			if (lastCount) {
				lastCount.SetText("");
			}
			Widget lastFill = m_DME_ObjectiveBarFills.Get(OBJECTIVE_ROW_COUNT - 1);
			if (lastFill) {
				lastFill.SetSize(0.0, 1.0);
			}
		}
	}

	protected void FillObjectiveRow(int rowIndex, DME_Tasks_ObjectiveDef def, DME_Tasks_QuestSyncEntry entry, DME_Tasks_ClientCache cache) {
		int required = def.Amount;
		int current = 0;
		bool done = false;

		DME_Tasks_ObjectiveProgress progress = null;
		if (cache) {
			progress = cache.FindProgress(entry.QuestId, def.ObjectiveId);
		}
		if (progress) {
			current = progress.Current;
			required = progress.Required;
			done = progress.Done;
		}
		// Historisch abgeschlossene Quest: alle Ziele als erledigt anzeigen.
		if (entry.State == EDME_Tasks_QuestState.COMPLETED) {
			current = required;
			done = true;
		}

		TextWidget check = m_DME_ObjectiveChecks.Get(rowIndex);
		if (check) {
			if (done) {
				check.SetText(DME_Tasks_LocKeys.MENU_CHECK_DONE);
				check.SetColor(ARGB(255, 110, 200, 120));
			} else {
				check.SetText(DME_Tasks_LocKeys.MENU_CHECK_OPEN);
				check.SetColor(ARGB(255, 160, 160, 170));
			}
		}

		TextWidget description = m_DME_ObjectiveTexts.Get(rowIndex);
		if (description) {
			description.SetText(DME_Tasks_UITextUtil.ObjectiveText(def));
		}

		TextWidget counter = m_DME_ObjectiveCounts.Get(rowIndex);
		if (counter) {
			counter.SetText(current.ToString() + "/" + required.ToString());
		}

		Widget fill = m_DME_ObjectiveBarFills.Get(rowIndex);
		if (fill) {
			float fraction = 0.0;
			if (required > 0) {
				fraction = current;
				fraction = fraction / required;
			}
			if (fraction > 1.0) {
				fraction = 1.0;
			}
			if (fraction < 0.0) {
				fraction = 0.0;
			}
			fill.SetSize(fraction, 1.0);
			if (done) {
				fill.SetColor(ARGB(255, 110, 200, 120));
			} else {
				fill.SetColor(ARGB(255, 185, 148, 50));
			}
		}
	}

	//! Buttons je State ein-/ausblenden (SPEC Agent 14: Accept/Turn-in/Claim/Abandon/Handover/
	//! Track/Choice/Replace). Replace nur fuer DAILY/WEEKLY.
	protected void UpdateButtons(DME_Tasks_QuestSyncEntry entry, DME_Tasks_ClientCache cache) {
		int state = entry.State;
		bool running = false;
		if (state == EDME_Tasks_QuestState.ACTIVE || state == EDME_Tasks_QuestState.READY_TO_TURN_IN) {
			running = true;
		}
		bool hasChoices = false;
		if (entry.Choices && entry.Choices.Count() > 0) {
			hasChoices = true;
		}
		bool isDailyWeekly = false;
		if (entry.Category == "DAILY" || entry.Category == "WEEKLY") {
			isDailyWeekly = true;
		}
		bool replaceable = false;
		if (isDailyWeekly && (state == EDME_Tasks_QuestState.AVAILABLE || running)) {
			replaceable = true;
		}

		ShowButton(m_DME_AcceptButton, state == EDME_Tasks_QuestState.AVAILABLE);
		ShowButton(m_DME_TurninButton, state == EDME_Tasks_QuestState.READY_TO_TURN_IN);
		ShowButton(m_DME_ClaimButton, state == EDME_Tasks_QuestState.READY_TO_TURN_IN);
		ShowButton(m_DME_ChoiceButton, state == EDME_Tasks_QuestState.READY_TO_TURN_IN && hasChoices);
		ShowButton(m_DME_HandoverButton, state == EDME_Tasks_QuestState.ACTIVE && HasOpenHandoverObjective(entry, cache));
		ShowButton(m_DME_AbandonButton, running);
		ShowButton(m_DME_ReplaceButton, replaceable);

		UpdateTrackButton(entry, cache, running);
	}

	protected void ShowButton(ButtonWidget buttonWidget, bool visible) {
		if (!buttonWidget) {
			return;
		}
		buttonWidget.Show(visible);
		if (visible) {
			buttonWidget.Enable(true);
			buttonWidget.SetAlpha(1.0);
		}
	}

	//! Track-Button: Toggle-Beschriftung + Sperre bei vollem Tracker (max MAX_TRACKED_QUESTS).
	protected void UpdateTrackButton(DME_Tasks_QuestSyncEntry entry, DME_Tasks_ClientCache cache, bool visible) {
		if (!m_DME_TrackButton) {
			return;
		}
		m_DME_TrackButton.Show(visible);
		if (!visible) {
			return;
		}

		bool tracked = false;
		int trackedCount = 0;
		if (cache && cache.PlayerState) {
			tracked = cache.IsTracked(entry.QuestId);
			trackedCount = cache.PlayerState.TrackedQuests.Count();
		}

		int maxTracked = DME_Tasks_Const.MAX_TRACKED_QUESTS;
		if (tracked) {
			m_DME_TrackButton.SetText(DME_Tasks_LocKeys.BTN_UNTRACK);
			m_DME_TrackButton.Enable(true);
			m_DME_TrackButton.SetAlpha(1.0);
		} else if (trackedCount >= maxTracked) {
			m_DME_TrackButton.SetText(DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.BTN_TRACKER_FULL, maxTracked.ToString()));
			m_DME_TrackButton.Enable(false);
			m_DME_TrackButton.SetAlpha(0.35);
		} else {
			m_DME_TrackButton.SetText(DME_Tasks_LocKeys.BTN_TRACK);
			m_DME_TrackButton.Enable(true);
			m_DME_TrackButton.SetAlpha(1.0);
		}
	}

	//! true wenn mind. ein HANDOVER/DELIVER-Objective noch nicht erledigt ist.
	protected bool HasOpenHandoverObjective(DME_Tasks_QuestSyncEntry entry, DME_Tasks_ClientCache cache) {
		if (!entry.Objectives) {
			return false;
		}
		for (int i = 0; i < entry.Objectives.Count(); i++) {
			DME_Tasks_ObjectiveDef def = entry.Objectives.Get(i);
			if (!def) {
				continue;
			}
			if (def.Type != "HANDOVER" && def.Type != "DELIVER") {
				continue;
			}
			DME_Tasks_ObjectiveProgress progress = null;
			if (cache) {
				progress = cache.FindProgress(entry.QuestId, def.ObjectiveId);
			}
			if (progress && progress.Done) {
				continue;
			}
			return true;
		}
		return false;
	}

	//! Anfrage-RPC mit Param1<string>(questId) an den Server (nur Anzeige-Schicht, keine Logik).
	protected void SendQuestRpc(string rpcName, string questId) {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}
		GetRPCManager().SendRPC(DME_Tasks_Const.MOD_NAME, rpcName, new Param1<string>(questId), true);
	}

	//! Track/Untrack: neue CSV aus Cache.PlayerState.TrackedQuests bauen (max 3) und
	//! RPC_SetTracked senden; optimistische lokale Uebernahme (Server-Resync korrigiert).
	protected void ToggleTracked(string questId) {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}
		DME_Tasks_ClientCache cache = m_DME_Menu.GetCache();
		if (!cache || !cache.PlayerState) {
			return;
		}

		array<string> tracked = new array<string>();
		int i;
		for (i = 0; i < cache.PlayerState.TrackedQuests.Count(); i++) {
			tracked.Insert(cache.PlayerState.TrackedQuests.Get(i));
		}

		int existingIndex = tracked.Find(questId);
		if (existingIndex > -1) {
			tracked.Remove(existingIndex);
		} else {
			if (tracked.Count() >= DME_Tasks_Const.MAX_TRACKED_QUESTS) {
				return;
			}
			tracked.Insert(questId);
		}

		string csv = "";
		for (i = 0; i < tracked.Count(); i++) {
			if (csv != "") {
				csv = csv + ",";
			}
			csv = csv + tracked.Get(i);
		}

		GetRPCManager().SendRPC(DME_Tasks_Const.MOD_NAME, "RPC_SetTracked", new Param1<string>(csv), true);

		cache.PlayerState.TrackedQuests.Clear();
		for (i = 0; i < tracked.Count(); i++) {
			cache.PlayerState.TrackedQuests.Insert(tracked.Get(i));
		}

		m_DME_Menu.Refresh();
		DME_Tasks_Tracker.OnCacheUpdated();
	}
}
