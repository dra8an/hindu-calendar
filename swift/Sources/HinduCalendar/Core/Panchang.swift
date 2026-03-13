import Foundation

public class Panchang {

    private let ephemeris: Ephemeris
    public let tithi: Tithi
    public let masa: Masa

    public init(ephemeris: Ephemeris) {
        self.ephemeris = ephemeris
        self.tithi = Tithi(ephemeris: ephemeris)
        self.masa = Masa(ephemeris: ephemeris, tithi: tithi)
    }

    public func gregorianToHindu(year: Int, month: Int, day: Int, loc: Location) -> HinduDate {
        let ti = tithi.tithiAtSunrise(year: year, month: month, day: day, loc: loc)
        let mi = masa.masaForDate(year: year, month: month, day: day, loc: loc)

        var isAdhikaTithi = false
        if day > 1 {
            let tiPrev = tithi.tithiAtSunrise(year: year, month: month, day: day - 1, loc: loc)
            isAdhikaTithi = (ti.tithiNum == tiPrev.tithiNum)
        }

        return HinduDate(yearSaka: mi.yearSaka, yearVikram: mi.yearVikram,
                        masa: mi.name, isAdhikaMasa: mi.isAdhika,
                        paksha: ti.paksha, tithi: ti.pakshaTithi,
                        isAdhikaTithi: isAdhikaTithi)
    }

    public func generateMonthPanchang(year: Int, month: Int, loc: Location) -> [PanchangDay] {
        let ndays = DateUtils.daysInMonth(year: year, month: month)
        var days = [PanchangDay]()
        days.reserveCapacity(ndays)

        for d in 1...ndays {
            let jd = ephemeris.gregorianToJd(year: year, month: month, day: d)
            let jdSunrise = ephemeris.sunriseJd(jd, loc)
            let ti = tithi.tithiAtSunrise(year: year, month: month, day: d, loc: loc)
            let hd = gregorianToHindu(year: year, month: month, day: d, loc: loc)

            days.append(PanchangDay(gregYear: year, gregMonth: month, gregDay: d,
                                   jdSunrise: jdSunrise, hinduDate: hd, tithi: ti))
        }

        return days
    }

    public static func jdToLocalTime(_ jdUt: Double, _ utcOffset: Double) -> (h: Int, m: Int, s: Int) {
        let localJd = jdUt + 0.5 + utcOffset / 24.0
        let frac = localJd - floor(localJd)
        let hours = frac * 24.0
        var h = Int(hours)
        var m = Int((hours - Double(h)) * 60.0)
        var s = Int(((hours - Double(h)) * 60.0 - Double(m)) * 60.0 + 0.5)
        if s == 60 { s = 0; m += 1 }
        if m == 60 { m = 0; h += 1 }
        return (h, m, s)
    }

    public func formatMonthPanchang(_ days: [PanchangDay], _ utcOffset: Double) -> String {
        if days.isEmpty { return "" }

        var sb = ""
        sb += String(format: "%-12s %-5s %-10s %-28s %s\n",
                     "Date", "Day", "Sunrise", "Tithi", "Hindu Date")
        sb += String(format: "%-12s %-5s %-10s %-28s %s\n",
                     "----------", "---", "--------", "----------------------------",
                     "----------------------------")

        for pd in days {
            let jd = ephemeris.gregorianToJd(year: pd.gregYear, month: pd.gregMonth, day: pd.gregDay)
            let dow = ephemeris.dayOfWeek(jd)

            let hms = Panchang.jdToLocalTime(pd.jdSunrise, utcOffset)

            let pakshaStr = (pd.tithi.paksha == .shukla) ? "Shukla" : "Krishna"
            let pt = pd.tithi.pakshaTithi
            let tithiName: String
            if pd.tithi.tithiNum == 30 {
                tithiName = "Amavasya"
            } else if pd.tithi.tithiNum == 15 {
                tithiName = "Purnima"
            } else {
                tithiName = TithiInfo.tithiNames[pt]
            }

            let masaStr = pd.hinduDate.masa.displayName
            let adhikaPrefix = pd.hinduDate.isAdhikaMasa ? "Adhika " : ""

            sb += String(format: "%04d-%02d-%02d   %-5s %02d:%02d:%02d   %-6s %-13s (%s-%d)   %@%@ %@ %d, Saka %d\n",
                        pd.gregYear, pd.gregMonth, pd.gregDay,
                        DateUtils.dayOfWeekShort(dow),
                        hms.h, hms.m, hms.s,
                        pakshaStr, tithiName,
                        (pd.tithi.paksha == .shukla) ? "S" : "K",
                        pt,
                        adhikaPrefix, masaStr, pakshaStr, pt,
                        pd.hinduDate.yearSaka)
        }

        return sb
    }

    public func formatDayPanchang(_ day: PanchangDay, _ utcOffset: Double) -> String {
        let jd = ephemeris.gregorianToJd(year: day.gregYear, month: day.gregMonth, day: day.gregDay)
        let dow = ephemeris.dayOfWeek(jd)

        let hms = Panchang.jdToLocalTime(day.jdSunrise, utcOffset)

        let pakshaStr = (day.tithi.paksha == .shukla) ? "Shukla" : "Krishna"
        let pt = day.tithi.pakshaTithi
        let tithiName: String
        if day.tithi.tithiNum == 30 {
            tithiName = "Amavasya"
        } else if day.tithi.tithiNum == 15 {
            tithiName = "Purnima"
        } else {
            tithiName = TithiInfo.tithiNames[pt]
        }

        let masaStr = day.hinduDate.masa.displayName
        let adhikaPrefix = day.hinduDate.isAdhikaMasa ? "Adhika " : ""

        var sb = ""
        sb += String(format: "Date:       %04d-%02d-%02d (%@)\n",
                    day.gregYear, day.gregMonth, day.gregDay,
                    DateUtils.dayOfWeekName(dow))
        sb += String(format: "Sunrise:    %02d:%02d:%02d IST\n", hms.h, hms.m, hms.s)
        sb += String(format: "Tithi:      %@ %@ (%@-%d)\n", pakshaStr, tithiName,
                    (day.tithi.paksha == .shukla) ? "S" : "K", pt)
        sb += String(format: "Hindu Date: %@%@ %@ %d, Saka %d (Vikram %d)\n",
                    adhikaPrefix, masaStr, pakshaStr, pt,
                    day.hinduDate.yearSaka, day.hinduDate.yearVikram)
        if day.tithi.isKshaya {
            sb += "Note:       Kshaya tithi (next tithi is skipped)\n"
        }
        if day.hinduDate.isAdhikaTithi {
            sb += "Note:       Adhika tithi (same tithi as previous day)\n"
        }

        return sb
    }
}
