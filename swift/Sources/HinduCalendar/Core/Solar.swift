import Foundation

public class Solar {

    private let ephemeris: Ephemeris
    private let tithi: Tithi

    public init(ephemeris: Ephemeris) {
        self.ephemeris = ephemeris
        self.tithi = Tithi(ephemeris: ephemeris)
    }

    // ===== Critical time computation =====

    private func criticalTimeJd(_ jdMidnightUt: Double, _ loc: Location,
                                 _ type: SolarCalendarType) -> Double {
        switch type {
        case .tamil:
            return ephemeris.sunsetJd(jdMidnightUt, loc) - 9.5 / (24.0 * 60.0)
        case .bengali:
            return jdMidnightUt - loc.utcOffset / 24.0 + 24.0 / (24.0 * 60.0)
        case .odia:
            return jdMidnightUt + (22.2 - loc.utcOffset) / 24.0
        case .malayalam:
            let sr = ephemeris.sunriseJd(jdMidnightUt, loc)
            let ss = ephemeris.sunsetJd(jdMidnightUt, loc)
            return sr + 0.6 * (ss - sr) - 9.5 / (24.0 * 60.0)
        }
    }

    // ===== Sankranti finding =====

    public func sankrantiJd(_ jdApprox: Double, _ targetLongitude: Double) -> Double {
        var lo = jdApprox - 20.0
        let hi0 = jdApprox + 20.0
        var hi = hi0

        let lonLo = ephemeris.solarLongitudeSidereal(lo)
        var diffLo = lonLo - targetLongitude
        if diffLo > 180.0 { diffLo -= 360.0 }
        if diffLo < -180.0 { diffLo += 360.0 }

        if diffLo >= 0 { lo -= 30.0 }

        let threshold = 1e-3 / 86400.0

        for _ in 0..<50 {
            let mid = (lo + hi) / 2.0
            let lon = ephemeris.solarLongitudeSidereal(mid)

            var diff = lon - targetLongitude
            if diff > 180.0 { diff -= 360.0 }
            if diff < -180.0 { diff += 360.0 }

            if diff >= 0 {
                hi = mid
            } else {
                lo = mid
            }

            if hi - lo < threshold { break }
        }

        return (lo + hi) / 2.0
    }

    public func sankrantiBefore(_ jdUt: Double) -> Double {
        let lon = ephemeris.solarLongitudeSidereal(jdUt)
        var rashi = Int(floor(lon / 30.0)) + 1
        if rashi > 12 { rashi = 12 }
        if rashi < 1 { rashi = 1 }

        let target = Double(rashi - 1) * 30.0
        var degreesPast = lon - target
        if degreesPast < 0 { degreesPast += 360.0 }
        let jdEst = jdUt - degreesPast

        return sankrantiJd(jdEst, target)
    }

    // ===== Bengali per-rashi tuning =====

    private func bengaliTunedCrit(_ type: SolarCalendarType, _ baseCritJd: Double, _ rashi: Int) -> Double {
        guard type == .bengali else { return baseCritJd }
        let adjustMin: Double
        switch rashi {
        case 4: adjustMin = 8.0    // Karkata
        case 7: adjustMin = -1.0   // Tula
        default: adjustMin = 0.0
        }
        return baseCritJd + adjustMin / (24.0 * 60.0)
    }

    private func bengaliDayEdgeOffset(_ type: SolarCalendarType, _ rashi: Int) -> Double {
        guard type == .bengali else { return 0.0 }
        switch rashi {
        case 6: return 4.0 / (24.0 * 60.0)    // Kanya: 23:56
        case 7: return 21.0 / (24.0 * 60.0)   // Tula: 23:39
        case 9: return 11.0 / (24.0 * 60.0)   // Dhanu: 23:49
        default: return 0.0
        }
    }

    private func bengaliRashiCorrection(_ type: SolarCalendarType, _ jdCrit: Double,
                                         _ rashi: inout Int, _ lon: inout Double) {
        guard type == .bengali else { return }
        let nextR = (rashi % 12) + 1
        let tuned = bengaliTunedCrit(type, jdCrit, nextR)
        if tuned > jdCrit {
            let lon2 = ephemeris.solarLongitudeSidereal(tuned)
            var r2 = Int(floor(lon2 / 30.0)) + 1
            if r2 > 12 { r2 = 12 }
            if r2 == nextR {
                rashi = nextR
                lon = lon2
            }
        }
    }

    // ===== Sankranti to civil day =====

    private func sankrantiToCivilDay(_ jdSankranti: Double, _ loc: Location,
                                      _ type: SolarCalendarType, _ rashi: Int)
        -> (year: Int, month: Int, day: Int) {
        let dayEdge = bengaliDayEdgeOffset(type, rashi)
        let localJd = jdSankranti + loc.utcOffset / 24.0 + 0.5 + dayEdge
        let ymd = ephemeris.jdToGregorian(floor(localJd))
        let sy = ymd.year, sm = ymd.month, sd = ymd.day

        let jdDay = ephemeris.gregorianToJd(year: sy, month: sm, day: sd)
        var crit = criticalTimeJd(jdDay, loc, type)

        crit = bengaliTunedCrit(type, crit, rashi)

        if jdSankranti <= crit {
            if bengaliTithiPushNext(type, jdSankranti, jdDay, rashi, loc) {
                return ephemeris.jdToGregorian(jdDay + 1.0)
            }
            return (sy, sm, sd)
        } else {
            return ephemeris.jdToGregorian(jdDay + 1.0)
        }
    }

    private func bengaliTithiPushNext(_ type: SolarCalendarType, _ jdSankranti: Double,
                                       _ jdDay: Double, _ rashi: Int, _ loc: Location) -> Bool {
        guard type == .bengali else { return false }
        if rashi == 4 { return false }  // Karkata: always this day
        if rashi == 10 { return true }  // Makara: always next day

        let prevYmd = ephemeris.jdToGregorian(jdDay - 1.0)
        let ti = tithi.tithiAtSunrise(year: prevYmd.year, month: prevYmd.month,
                                       day: prevYmd.day, loc: loc)
        return ti.jdEnd <= jdSankranti
    }

    // ===== Rashi to regional month number =====

    private static func rashiToRegionalMonth(_ rashi: Int, _ type: SolarCalendarType) -> Int {
        var m = rashi - type.firstRashi + 1
        if m <= 0 { m += 12 }
        return m
    }

    // ===== Solar year =====

    private func solarYear(_ jdUt: Double, _ loc: Location, _ jdGregDate: Double,
                            _ type: SolarCalendarType) -> Int {
        let ymd = ephemeris.jdToGregorian(jdUt)
        let gy = ymd.year

        let targetLong = Double(type.yearStartRashi - 1) * 30.0
        var approxGregMonth = 3 + type.yearStartRashi
        if approxGregMonth > 12 { approxGregMonth -= 12 }

        let jdYearStartEst = ephemeris.gregorianToJd(year: gy, month: approxGregMonth, day: 14)
        let jdYearStart = sankrantiJd(jdYearStartEst, targetLong)

        let ysYmd = sankrantiToCivilDay(jdYearStart, loc, type, type.yearStartRashi)
        let jdYearCivil = ephemeris.gregorianToJd(year: ysYmd.year, month: ysYmd.month, day: ysYmd.day)

        if jdGregDate >= jdYearCivil {
            return gy - type.gyOffsetOn
        } else {
            return gy - type.gyOffsetBefore
        }
    }

    // ===== Public API =====

    public func gregorianToSolar(year: Int, month: Int, day: Int,
                                  loc: Location, type: SolarCalendarType) -> SolarDate {
        let jd = ephemeris.gregorianToJd(year: year, month: month, day: day)
        let jdCrit = criticalTimeJd(jd, loc, type)

        var lon = ephemeris.solarLongitudeSidereal(jdCrit)

        var rashi = Int(floor(lon / 30.0)) + 1
        if rashi > 12 { rashi = 12 }
        if rashi < 1 { rashi = 1 }

        bengaliRashiCorrection(type, jdCrit, &rashi, &lon)

        let target = Double(rashi - 1) * 30.0
        var degreesPast = lon - target
        if degreesPast < 0 { degreesPast += 360.0 }
        let jdEst = jdCrit - degreesPast
        var jdSankranti = sankrantiJd(jdEst, target)

        var civilDay = sankrantiToCivilDay(jdSankranti, loc, type, rashi)
        var jdMonthStart = ephemeris.gregorianToJd(year: civilDay.year, month: civilDay.month,
                                                    day: civilDay.day)
        var solarDay = Int(jd - jdMonthStart) + 1

        if solarDay <= 0 {
            rashi = (rashi == 1) ? 12 : rashi - 1
            let prevTarget = Double(rashi - 1) * 30.0
            jdSankranti = sankrantiJd(jdSankranti - 28.0, prevTarget)
            civilDay = sankrantiToCivilDay(jdSankranti, loc, type, rashi)
            jdMonthStart = ephemeris.gregorianToJd(year: civilDay.year, month: civilDay.month,
                                                    day: civilDay.day)
            solarDay = Int(jd - jdMonthStart) + 1
        }

        let regionalMonth = Solar.rashiToRegionalMonth(rashi, type)
        let solarYr = solarYear(jdCrit, loc, jd, type)

        return SolarDate(year: solarYr, month: regionalMonth, day: solarDay,
                        rashi: rashi, jdSankranti: jdSankranti)
    }

    // ===== Solar month start/length =====

    public func solarMonthStart(_ month: Int, _ year: Int, _ type: SolarCalendarType,
                                 _ loc: Location) -> Double {
        var rashi = month + type.firstRashi - 1
        if rashi > 12 { rashi -= 12 }

        let yearStartMonth = ((type.yearStartRashi - type.firstRashi) % 12) + 1
        var monthsPast = month - yearStartMonth
        if monthsPast < 0 { monthsPast += 12 }

        var gy = year + type.gyOffsetOn
        var gmStart = 3 + type.yearStartRashi
        if gmStart > 12 { gmStart -= 12; gy += 1 }
        var approxGm = gmStart + monthsPast
        while approxGm > 12 { approxGm -= 12; gy += 1 }

        let jdApprox = ephemeris.gregorianToJd(year: gy, month: approxGm, day: 14)

        let target = Double(rashi - 1) * 30.0
        let jdSk = sankrantiJd(jdApprox, target)

        let civilDay = sankrantiToCivilDay(jdSk, loc, type, rashi)
        let cy = civilDay.year, cm = civilDay.month, cd = civilDay.day
        let jdCivil = ephemeris.gregorianToJd(year: cy, month: cm, day: cd)

        let check = gregorianToSolar(year: cy, month: cm, day: cd, loc: loc, type: type)
        if check.month != month {
            let next = ephemeris.jdToGregorian(jdCivil + 1.0)
            let check2 = gregorianToSolar(year: next.year, month: next.month,
                                           day: next.day, loc: loc, type: type)
            if check2.month == month {
                return jdCivil + 1.0
            }

            let prev = ephemeris.jdToGregorian(jdCivil - 1.0)
            let check3 = gregorianToSolar(year: prev.year, month: prev.month,
                                           day: prev.day, loc: loc, type: type)
            if check3.month == month {
                return jdCivil - 1.0
            }
        }

        return jdCivil
    }

    public func solarMonthLength(_ month: Int, _ year: Int, _ type: SolarCalendarType,
                                  _ loc: Location) -> Int {
        let jdStart = solarMonthStart(month, year, type, loc)

        let nextMonth = (month == 12) ? 1 : month + 1
        let yearStartMonth = ((type.yearStartRashi - type.firstRashi) % 12) + 1
        let lastMonth = (yearStartMonth == 1) ? 12 : yearStartMonth - 1
        let nextYear = (month == lastMonth) ? year + 1 : year

        let jdEnd = solarMonthStart(nextMonth, nextYear, type, loc)

        return Int(jdEnd - jdStart)
    }

    public static let rashiNames = [
        "", "Mesha", "Vrishabha", "Mithuna", "Karka", "Simha", "Kanya",
        "Tula", "Vrishchika", "Dhanu", "Makara", "Kumbha", "Meena"
    ]
}
