//! Statische Helfer fuer idempotente Transaktionen auf DME_Tasks_PlayerState.Transactions.
//! Konzept §13: PENDING wird vor der Auszahlung persistiert (Reservierung),
//! COMPLETED nach der Auszahlung — HasCompleted schuetzt gegen Doppel-RPC.
//! Aufraeumen: es werden maximal MAX_COMPLETED_RECORDS COMPLETED-Records behalten;
//! aelteste zuerst entfernt (Transactions ist einfuege-geordnet, CreatedAt monoton).
class DME_Tasks_Transaction {
	private static const int MAX_COMPLETED_RECORDS = 50;

	static bool HasCompleted(DME_Tasks_PlayerState state, string txId) {
		DME_Tasks_TxRecord record = Find(state, txId);
		if (!record) {
			return false;
		}
		return record.State == EDME_Tasks_TxState.COMPLETED;
	}

	//! Existierenden Record liefern oder neuen PENDING-Record anlegen.
	static DME_Tasks_TxRecord BeginOrGet(DME_Tasks_PlayerState state, string txId, string questId) {
		if (!state || !state.Transactions) {
			DME_Tasks_Log.Error("Transaction.BeginOrGet: PlayerState/Transactions null (txId=%1)", txId);
			return null;
		}

		DME_Tasks_TxRecord record = Find(state, txId);
		if (record) {
			return record;
		}

		record = new DME_Tasks_TxRecord();
		record.TxId = txId;
		record.QuestId = questId;
		record.State = EDME_Tasks_TxState.PENDING;
		record.CreatedAt = CF_Date.Now(true).DateToEpoch();
		state.Transactions.Insert(record);
		return record;
	}

	static void Complete(DME_Tasks_PlayerState state, string txId) {
		DME_Tasks_TxRecord record = Find(state, txId);
		if (!record) {
			DME_Tasks_Log.Warn("Transaction.Complete: Record %1 nicht gefunden", txId);
			return;
		}

		record.State = EDME_Tasks_TxState.COMPLETED;
		PruneCompleted(state);
	}

	// ------------------------------------------------------------------
	// intern
	// ------------------------------------------------------------------

	private static DME_Tasks_TxRecord Find(DME_Tasks_PlayerState state, string txId) {
		if (!state || !state.Transactions) {
			return null;
		}
		foreach (DME_Tasks_TxRecord record : state.Transactions) {
			if (record && record.TxId == txId) {
				return record;
			}
		}
		return null;
	}

	//! Behaelt maximal MAX_COMPLETED_RECORDS COMPLETED-Records; aelteste fliegen zuerst.
	//! PENDING-Records werden nie entfernt.
	private static void PruneCompleted(DME_Tasks_PlayerState state) {
		if (!state || !state.Transactions) {
			return;
		}

		int completedCount = 0;
		foreach (DME_Tasks_TxRecord record : state.Transactions) {
			if (record && record.State == EDME_Tasks_TxState.COMPLETED) {
				completedCount++;
			}
		}

		int toRemove = completedCount - MAX_COMPLETED_RECORDS;
		if (toRemove <= 0) {
			return;
		}

		int index = 0;
		while (index < state.Transactions.Count() && toRemove > 0) {
			DME_Tasks_TxRecord candidate = state.Transactions[index];
			if (candidate && candidate.State == EDME_Tasks_TxState.COMPLETED) {
				state.Transactions.RemoveOrdered(index);
				toRemove--;
			} else {
				index++;
			}
		}
	}
}
