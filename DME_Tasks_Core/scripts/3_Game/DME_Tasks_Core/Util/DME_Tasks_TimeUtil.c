//! Zeit-Helfer (CONTRACTS §1): Real-Zeit ausschliesslich via CF_Date (UTC).
//! NowEpoch = Epoch-Sekunden, TodayKey = "YYYY-MM-DD" (UTC), WeekKey = ISO-Jahr+Woche "YYYY-Www" (UTC).
class DME_Tasks_TimeUtil {
	static int NowEpoch() {
		CF_Date now = CF_Date.Now(true);
		return now.DateToEpoch();
	}

	static string TodayKey() {
		CF_Date now = CF_Date.Now(true);
		return now.Format(CF_Date.DATE);
	}

	static string WeekKey() {
		CF_Date now = CF_Date.Now(true);
		int year = now.GetYear();
		int month = now.GetMonth();
		int day = now.GetDay();

		int isoDow = IsoDayOfWeek(year, month, day);
		int dayOfYear = DayOfYear(year, month, day);
		int week = (dayOfYear - isoDow + 10) / 7;

		int isoYear = year;
		if (week < 1) {
			isoYear = year - 1;
			week = IsoWeeksInYear(isoYear);
		} else {
			int weeksThisYear = IsoWeeksInYear(year);
			if (week > weeksThisYear) {
				isoYear = year + 1;
				week = 1;
			}
		}

		string weekPart = week.ToStringLen(2);
		return isoYear.ToString() + "-W" + weekPart;
	}

	//! Wochentag nach Sakamoto (0 = Sonntag) — identische Formel wie CF_Date.GetDayOfWeek, aber fuer beliebige Daten.
	private static int DayOfWeekSundayZero(int year, int month, int day) {
		int y = year;
		int d = day;
		if (month < 3) {
			d = d + y;
			y = y - 1;
		} else {
			d = d + y - 2;
		}
		return (23 * month / 9 + d + 4 + y / 4 - y / 100 + y / 400) % 7;
	}

	//! ISO-Wochentag: Montag = 1 .. Sonntag = 7.
	private static int IsoDayOfWeek(int year, int month, int day) {
		int dow = DayOfWeekSundayZero(year, month, day);
		if (dow == 0) {
			return 7;
		}
		return dow;
	}

	private static int DayOfYear(int year, int month, int day) {
		int result = day;
		if (month > 1) result += 31;
		if (month > 2) result += 28;
		if (month > 3) result += 31;
		if (month > 4) result += 30;
		if (month > 5) result += 31;
		if (month > 6) result += 30;
		if (month > 7) result += 31;
		if (month > 8) result += 31;
		if (month > 9) result += 30;
		if (month > 10) result += 31;
		if (month > 11) result += 30;
		if (month > 2 && CF_Date.IsLeapYear(year)) {
			result += 1;
		}
		return result;
	}

	//! ISO 8601: 53 Wochen, wenn der 1. Januar ein Donnerstag ist — oder ein Mittwoch in einem Schaltjahr.
	private static int IsoWeeksInYear(int year) {
		int jan1 = IsoDayOfWeek(year, 1, 1);
		if (jan1 == 4) {
			return 53;
		}
		if (jan1 == 3 && CF_Date.IsLeapYear(year)) {
			return 53;
		}
		return 52;
	}
}
