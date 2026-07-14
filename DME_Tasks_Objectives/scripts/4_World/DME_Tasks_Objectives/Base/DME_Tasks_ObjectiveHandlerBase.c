//! Basisklasse aller Objective-Handler (CONTRACTS §6.5 — API VERBINDLICH).
//!
//! HANDLER-KONTRAKT (Agenten 9-12):
//! - Handler-Klasse leitet von DME_Tasks_ObjectiveHandlerBase ab und hat einen parameterlosen Ctor
//!   (DME_Tasks_ObjectiveRegistry.RegisterAll erzeugt die EINZIGE Instanz — kein eigenes GetInstance noetig).
//! - ALLE drei Methoden mit `override` ueberschreiben:
//!   - CanHandle(objectiveType): true fuer jeden EDME_Tasks_ObjectiveType, den der Handler bedient.
//!   - GetEventTypesFor(objectiveType, outTypes): outTypes.Insert(EDME_Tasks_EventType....) je EventType,
//!     unter dem ein Objective dieses Typs im Active-Objective-Index einsortiert werden soll (mehrere erlaubt).
//!   - OnEvent(objectiveRef, evt): Event pruefen (Filter aus objectiveRef.Def), Fortschritt AUSSCHLIESSLICH
//!     via ReportProgress(...) melden. Event-Subtyp per <Subklasse>.Cast(evt) holen und null-checken!
//! - Handler laufen NUR auf dem Dedicated Server (EventBus/Modul garantieren das bereits; eigene Guards
//!   in OnEvent sind nicht noetig, schaden aber nicht).
class DME_Tasks_ObjectiveHandlerBase {
	//! true, wenn dieser Handler den EDME_Tasks_ObjectiveType bedient.
	bool CanHandle(int objectiveType) {
		return false;
	}

	//! Fuellt outTypes mit allen EDME_Tasks_EventType-Werten, unter denen Objectives dieses Typs
	//! indexiert werden sollen. Ein Objective kann unter MEHREREN EventTypes stehen.
	void GetEventTypesFor(int objectiveType, array<int> outTypes) {
	}

	//! Verarbeitet ein Event fuer genau einen Index-Eintrag. Fortschritt NUR via ReportProgress melden.
	void OnEvent(DME_Tasks_ObjectiveRef objectiveRef, DME_Tasks_Event evt) {
	}

	// ==================================================================
	// Gemeinsame Helfer fuer alle Handler
	// ==================================================================

	//! Tageszeit-Fenster gegen die IN-GAME-Stunde (g_Game.GetWorld().GetDate, World.c:33).
	//! fromHour/toHour aus DME_Tasks_ObjectiveDef (-1 = keine Einschraenkung; Default -1).
	//! Semantik: Stunde in [fromHour, toHour) — bei fromHour > toHour Mitternacht-Wrap
	//! (z. B. 20..4 = 20:00-23:59 oder 00:00-03:59). fromHour == toHour = keine Einschraenkung.
	protected bool IsInTimeWindow(int fromHour, int toHour) {
		if (fromHour < 0 || toHour < 0) {
			return true;
		}
		if (fromHour == toHour) {
			return true;
		}
		if (!g_Game) {
			return true;
		}
		World world = g_Game.GetWorld();
		if (!world) {
			return true;
		}

		int year;
		int month;
		int day;
		int hour;
		int minute;
		world.GetDate(year, month, day, hour, minute);

		if (fromHour < toHour) {
			return hour >= fromHour && hour < toHour;
		}
		//! Mitternacht-Wrap
		return hour >= fromHour || hour < toHour;
	}

	//! Klassennamen-Filter: leere/null Liste = alles erlaubt. Direkter Vergleich zuerst,
	//! danach Config-Vererbung via g_Game.IsKindOf(type, className) (verifiziert Game.c:1412).
	protected bool MatchesClassNames(string type, array<string> classNames) {
		if (!classNames || classNames.Count() == 0) {
			return true;
		}
		if (type == "") {
			return false;
		}

		foreach (string className : classNames) {
			if (className == "") {
				continue;
			}
			if (type == className) {
				return true;
			}
			if (g_Game && g_Game.IsKindOf(type, className)) {
				return true;
			}
		}
		return false;
	}

	//! Einziger erlaubter Fortschritts-Pfad: delegiert an QuestEngine.AddObjectiveProgress
	//! (klemmt auf [0..Required], setzt Done, prueft READY_TO_TURN_IN, sendet Delta-RPC, Dirty).
	protected void ReportProgress(DME_Tasks_ObjectiveRef objectiveRef, int delta) {
		if (!objectiveRef) {
			return;
		}
		DME_Tasks_QuestEngine.GetInstance().AddObjectiveProgress(objectiveRef.PlayerUid, objectiveRef.QuestId, objectiveRef.ObjectiveId, delta);
	}
}
