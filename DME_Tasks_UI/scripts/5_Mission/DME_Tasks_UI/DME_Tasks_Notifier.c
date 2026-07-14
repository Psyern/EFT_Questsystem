//! Toast-Notifications oben rechts (client-only): max 4 gleichzeitig (aeltester faellt raus),
//! Auto-Hide nach 6 s via CallQueue (CALL_CATEGORY_GUI, kein Frame-Loop), Anti-Spam-Fenster 2 s.
//! Farbe nach EDME_Tasks_NotificationType (INFO grau / SUCCESS gruen / WARNING gelb / ERROR rot / PROGRESS blau).
//! Reine Anzeige — kein Gameplay-Code. Aufruf durch das UIModule (RPC_Notification) sowie durch
//! das DME_Tasks_NotifierModule unten (RPC_RewardGranted + QuestStateChanged→COMPLETED).

//! Ein aktiver Toast (nur Anzeige-Daten).
class DME_Tasks_NotificationEntry {
	string Title;
	string Message;
	int Type;

	void DME_Tasks_NotificationEntry() {
		Title = "";
		Message = "";
		Type = EDME_Tasks_NotificationType.INFO;
	}
}

class DME_Tasks_Notifier {
	//! Vorgebaute Slots im notification.layout (notification_slot_0..3).
	static const int MAX_VISIBLE = 4;
	//! Auto-Hide-Verzoegerung in Millisekunden.
	static const int AUTO_HIDE_MS = 6000;
	//! Identische Notification (Titel+Text) innerhalb dieses Fensters (Sekunden) wird verworfen.
	static const float DUPLICATE_WINDOW_SECONDS = 2.0;

	static const int COLOR_INFO = 0xFFB0B0B0;
	static const int COLOR_SUCCESS = 0xFF60BE60;
	static const int COLOR_WARNING = 0xFFD6B63C;
	static const int COLOR_ERROR = 0xFFCC4848;
	static const int COLOR_PROGRESS = 0xFF568CCC;

	private static ref DME_Tasks_Notifier s_DME_Instance;

	protected Widget m_DME_Root;
	protected ref array<Widget> m_DME_Slots;
	protected ref array<Widget> m_DME_Stripes;
	protected ref array<TextWidget> m_DME_Titles;
	protected ref array<TextWidget> m_DME_Messages;
	//! Aktive Toasts, aeltester zuerst; Anzeige neuester oben.
	protected ref array<ref DME_Tasks_NotificationEntry> m_DME_Active;

	protected string m_DME_LastTitle;
	protected string m_DME_LastMessage;
	protected float m_DME_LastShownAt;

	void DME_Tasks_Notifier() {
		m_DME_Slots = new array<Widget>();
		m_DME_Stripes = new array<Widget>();
		m_DME_Titles = new array<TextWidget>();
		m_DME_Messages = new array<TextWidget>();
		m_DME_Active = new array<ref DME_Tasks_NotificationEntry>();
		m_DME_LastTitle = "";
		m_DME_LastMessage = "";
		m_DME_LastShownAt = -1000.0;
		CreateRoot();
	}

	//! Einstiegspunkt (Signatur verbindlich, CONTRACTS 6.6 — das UIModule ruft sie bei RPC_Notification).
	//! type = EDME_Tasks_NotificationType.
	static void Show(string title, string message, int type) {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}
		DME_Tasks_Notifier notifier = GetInstance();
		if (!notifier) {
			return;
		}
		notifier.ShowInternal(title, message, type);
	}

	//! Lazy-Singleton; null auf Dedicated-Server.
	static DME_Tasks_Notifier GetInstance() {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return null;
		}
		if (!s_DME_Instance) {
			s_DME_Instance = new DME_Tasks_Notifier();
		}
		return s_DME_Instance;
	}

	// ------------------------------------------------------------------
	// Aufbau
	// ------------------------------------------------------------------

	protected void CreateRoot() {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}
		if (m_DME_Root) {
			return;
		}

		WorkspaceWidget workspace = g_Game.GetWorkspace();
		if (!workspace) {
			return;
		}

		m_DME_Root = workspace.CreateWidgets(DME_Tasks_UIConst.LAYOUT_NOTIFICATION, null);
		if (!m_DME_Root) {
			DME_Tasks_Log.Warn("Notifier: Layout %1 konnte nicht geladen werden", DME_Tasks_UIConst.LAYOUT_NOTIFICATION);
			return;
		}

		for (int i = 0; i < MAX_VISIBLE; i++) {
			string slotIdxText = i.ToString();
			Widget slot = m_DME_Root.FindAnyWidget("notification_slot_" + slotIdxText);
			m_DME_Slots.Insert(slot);
			Widget stripe = m_DME_Root.FindAnyWidget("notification_stripe_" + slotIdxText);
			m_DME_Stripes.Insert(stripe);
			TextWidget titleWidget = TextWidget.Cast(m_DME_Root.FindAnyWidget("notification_title_" + slotIdxText));
			m_DME_Titles.Insert(titleWidget);
			TextWidget messageWidget = TextWidget.Cast(m_DME_Root.FindAnyWidget("notification_message_" + slotIdxText));
			m_DME_Messages.Insert(messageWidget);
		}

		m_DME_Root.Show(false);
	}

	// ------------------------------------------------------------------
	// Anzeige
	// ------------------------------------------------------------------

	protected void ShowInternal(string title, string message, int type) {
		if (!m_DME_Root) {
			CreateRoot();
		}
		if (!m_DME_Root) {
			return;
		}

		// Anti-Spam: identischer Titel+Text innerhalb von 2 s wird verworfen.
		float now = g_Game.GetTickTime();
		if (title == m_DME_LastTitle && message == m_DME_LastMessage) {
			float elapsed = now - m_DME_LastShownAt;
			if (elapsed >= 0.0 && elapsed < DUPLICATE_WINDOW_SECONDS) {
				return;
			}
		}
		m_DME_LastTitle = title;
		m_DME_LastMessage = message;
		m_DME_LastShownAt = now;

		DME_Tasks_NotificationEntry entry = new DME_Tasks_NotificationEntry();
		entry.Title = title;
		entry.Message = message;
		entry.Type = type;
		m_DME_Active.Insert(entry);

		// Max 4 gleichzeitig — aeltester faellt raus.
		while (m_DME_Active.Count() > MAX_VISIBLE) {
			m_DME_Active.Remove(0);
		}

		RebuildSlots();

		// Auto-Hide nach 6 s; CallQueue haelt den Entry-Param am Leben (verifiziert: 2_GameLib tools.c).
		ScriptCallQueue queue = null;
		if (g_Game) {
			queue = g_Game.GetCallQueue(CALL_CATEGORY_GUI);
		}
		if (queue) {
			queue.CallLater(OnToastExpired, AUTO_HIDE_MS, false, entry);
		}
	}

	//! CallLater-Callback: entfernt genau diesen Toast (falls nicht schon durch Ueberlauf verdraengt).
	protected void OnToastExpired(DME_Tasks_NotificationEntry entry) {
		if (!entry) {
			return;
		}
		int idx = m_DME_Active.Find(entry);
		if (idx == -1) {
			return;
		}
		m_DME_Active.Remove(idx);
		RebuildSlots();
	}

	protected void RebuildSlots() {
		int activeCount = m_DME_Active.Count();

		for (int i = 0; i < MAX_VISIBLE; i++) {
			Widget slot = null;
			if (i < m_DME_Slots.Count()) {
				slot = m_DME_Slots.Get(i);
			}
			if (!slot) {
				continue;
			}

			if (i >= activeCount) {
				slot.Show(false);
				continue;
			}

			// Neuester Toast oben (Slot 0), aeltester unten.
			int entryIdx = activeCount - 1 - i;
			DME_Tasks_NotificationEntry entry = m_DME_Active.Get(entryIdx);
			if (!entry) {
				slot.Show(false);
				continue;
			}

			int typeColor = GetColorForType(entry.Type);

			Widget stripe = null;
			if (i < m_DME_Stripes.Count()) {
				stripe = m_DME_Stripes.Get(i);
			}
			if (stripe) {
				stripe.SetColor(typeColor);
			}

			TextWidget titleWidget = null;
			if (i < m_DME_Titles.Count()) {
				titleWidget = m_DME_Titles.Get(i);
			}
			if (titleWidget) {
				titleWidget.SetText(entry.Title);
				titleWidget.SetColor(typeColor);
			}

			TextWidget messageWidget = null;
			if (i < m_DME_Messages.Count()) {
				messageWidget = m_DME_Messages.Get(i);
			}
			if (messageWidget) {
				messageWidget.SetText(entry.Message);
			}

			slot.Show(true);
		}

		bool anyVisible = false;
		if (activeCount > 0) {
			anyVisible = true;
		}
		if (m_DME_Root) {
			m_DME_Root.Show(anyVisible);
		}
	}

	protected int GetColorForType(int type) {
		if (type == EDME_Tasks_NotificationType.SUCCESS) {
			return COLOR_SUCCESS;
		}
		if (type == EDME_Tasks_NotificationType.WARNING) {
			return COLOR_WARNING;
		}
		if (type == EDME_Tasks_NotificationType.ERROR) {
			return COLOR_ERROR;
		}
		if (type == EDME_Tasks_NotificationType.PROGRESS) {
			return COLOR_PROGRESS;
		}
		return COLOR_INFO;
	}
}

//! Abo-Traeger fuer die Notifier-eigenen ClientEvents (das UIModule delegiert RewardGranted NICHT):
//! RPC_RewardGranted → SUCCESS-Toast (Payload = DME_Tasks_RewardDef-JSON; gesendet vom Server in
//! QuestEngine.ClaimReward). Kein zusaetzlicher COMPLETED-Toast aus QuestStateChanged — der
//! Abschluss wird bereits durch RewardGranted-Toast + Server-RPC_Notification gemeldet.
[CF_RegisterModule(DME_Tasks_NotifierModule)]
class DME_Tasks_NotifierModule : CF_ModuleWorld {
	override void OnInit() {
		super.OnInit();

		DME_Tasks_ClientEvents.s_DME_OnRewardGranted.Insert(OnRewardGranted);
	}

	private void OnRewardGranted(string json) {
		if (!g_Game || g_Game.IsDedicatedServer()) {
			return;
		}

		string message = DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.NOTIF_REWARD_RECEIVED);
		if (json != "") {
			DME_Tasks_RewardDef reward = new DME_Tasks_RewardDef();
			bool parsed = DME_Tasks_Json<DME_Tasks_RewardDef>.FromJson(reward, json, "NotifierModule.OnRewardGranted");
			if (parsed) {
				string summary = BuildRewardSummary(reward);
				if (summary != "") {
					message = summary;
				}
			}
		}

		DME_Tasks_Notifier.Show(DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.NOTIF_TITLE_REWARD), message, EDME_Tasks_NotificationType.SUCCESS);
	}

	//! Kompakte Zusammenfassung eines RewardDef ("+8500 XP, +45000 Rubel, ..."); "" wenn leer.
	private string BuildRewardSummary(DME_Tasks_RewardDef reward) {
		string summary = "";

		if (reward.PlayerExperience > 0) {
			summary = AppendPart(summary, DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.NOTIF_REWARD_XP, reward.PlayerExperience.ToString()));
		}
		if (reward.Currency > 0) {
			summary = AppendPart(summary, DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.NOTIF_REWARD_RUBLES, reward.Currency.ToString()));
		}
		if (reward.TraderReputation > 0.0) {
			string positiveRep = "+" + reward.TraderReputation.ToString();
			summary = AppendPart(summary, DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.NOTIF_REWARD_REPUTATION, positiveRep));
		}
		if (reward.TraderReputation < 0.0) {
			summary = AppendPart(summary, DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.NOTIF_REWARD_REPUTATION, reward.TraderReputation.ToString()));
		}
		if (reward.Items) {
			int itemCount = reward.Items.Count();
			if (itemCount > 0) {
				summary = AppendPart(summary, DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.NOTIF_REWARD_ITEMS, itemCount.ToString()));
			}
		}
		if (reward.SkillPoints > 0) {
			summary = AppendPart(summary, DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.NOTIF_REWARD_SKILLPOINTS, reward.SkillPoints.ToString()));
		}
		if (reward.SeasonXp > 0) {
			summary = AppendPart(summary, DME_Tasks_Loc.Resolve(DME_Tasks_LocKeys.NOTIF_REWARD_SEASON_XP, reward.SeasonXp.ToString()));
		}

		return summary;
	}

	private string AppendPart(string summary, string part) {
		if (summary == "") {
			return part;
		}
		return summary + ", " + part;
	}
}
