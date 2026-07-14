//! The single client-side localization resolver (the '#' rule):
//! any string with a leading '#' is a stringtable key — translate it via Widget.TranslateString
//! and fill %1/%2 with p1/p2. Anything else is shown verbatim (plain admin content, dynamic values).
//! Server code never builds display prose — it sends DME_Tasks_LocKeys constants plus params.
class DME_Tasks_Loc {
	static string Resolve(string raw, string p1 = "", string p2 = "", string p3 = "") {
		if (raw == "") {
			return "";
		}

		if (raw.IndexOf("#") != 0) {
			return raw;
		}

		string translated = Widget.TranslateString(raw);

		//! Always format (even with empty params) so a parameterized key never
		//! leaks a literal "%1" into the UI. Params follow the '#' rule too —
		//! a param may itself be a key (e.g. a fail reason nested inside a message).
		if (p1.IndexOf("#") == 0) {
			p1 = Widget.TranslateString(p1);
		}
		if (p2.IndexOf("#") == 0) {
			p2 = Widget.TranslateString(p2);
		}
		if (p3.IndexOf("#") == 0) {
			p3 = Widget.TranslateString(p3);
		}

		return string.Format(translated, p1, p2, p3);
	}
}
