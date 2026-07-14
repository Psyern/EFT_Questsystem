//! Typkonkrete JSON-String-(De-)Serialisierung.
//! WICHTIG: JsonSerializer.WriteToString/ReadFromString (gameplay.c:49ff) arbeiten mit dem
//! STATISCHEN Typ des Arguments — eine als Basisklasse (Class/Managed) typisierte Variable
//! serialisiert zu "{}". Deshalb MUSS jede Nutzung ueber dieses Template mit konkretem T laufen.
class DME_Tasks_Json<Class T> {
	//! Liefert "" bei Fehler (bereits geloggt) — Aufrufer prueft auf leeren String.
	static string ToJson(T data, string context = "") {
		if (!data) {
			DME_Tasks_Log.Error("JSON-Serialisierung: null-Daten (%1)", context);
			return "";
		}
		JsonSerializer serializer = new JsonSerializer();
		string result;
		bool ok = serializer.WriteToString(data, false, result);
		if (!ok) {
			DME_Tasks_Log.Error("JSON-Serialisierung fehlgeschlagen: %1", context);
			return "";
		}
		return result;
	}

	//! data muss eine bereits allozierte Instanz sein; false bei Fehler (bereits geloggt).
	static bool FromJson(T data, string json, string context = "") {
		if (!data) {
			DME_Tasks_Log.Error("JSON-Deserialisierung: null-Ziel (%1)", context);
			return false;
		}
		JsonSerializer serializer = new JsonSerializer();
		string error;
		bool ok = serializer.ReadFromString(data, json, error);
		if (!ok) {
			DME_Tasks_Log.Error("JSON-Deserialisierung fehlgeschlagen (%1): %2", context, error);
			return false;
		}
		return true;
	}
}
