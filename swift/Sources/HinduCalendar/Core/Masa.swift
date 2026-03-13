import Foundation

public class Masa {

    private let ephemeris: Ephemeris
    private let tithi: Tithi

    private struct CacheKey: Hashable {
        let masa: MasaName
        let sakaYear: Int
        let isAdhika: Bool
        let scheme: LunisolarScheme
    }

    private struct CacheEntry {
        let jdStart: Double
        var length: Int
    }

    private var monthCache: [CacheKey: CacheEntry] = [:]

    public init(ephemeris: Ephemeris, tithi: Tithi) {
        self.ephemeris = ephemeris
        self.tithi = tithi
    }

    private static func inverseLagrange(_ x: [Double], _ y: [Double], _ n: Int, _ ya: Double) -> Double {
        var total = 0.0
        for i in 0..<n {
            var numer = 1.0
            var denom = 1.0
            for j in 0..<n {
                if j != i {
                    numer *= (ya - y[j])
                    denom *= (y[i] - y[j])
                }
            }
            total += numer * x[i] / denom
        }
        return total
    }

    private static func unwrapAngles(_ angles: inout [Double], _ n: Int) {
        for i in 1..<n {
            if angles[i] < angles[i - 1] {
                angles[i] += 360.0
            }
        }
    }

    public func newMoonBefore(_ jdUt: Double, _ tithiHint: Int) -> Double {
        let start = jdUt - Double(tithiHint)

        var x = [Double](repeating: 0, count: 9)
        var y = [Double](repeating: 0, count: 9)
        for i in 0..<9 {
            x[i] = -2.0 + Double(i) * 0.5
            y[i] = tithi.lunarPhase(start + x[i])
        }
        Masa.unwrapAngles(&y, 9)

        let y0 = Masa.inverseLagrange(x, y, 9, 360.0)
        return start + y0
    }

    public func newMoonAfter(_ jdUt: Double, _ tithiHint: Int) -> Double {
        let start = jdUt + Double(30 - tithiHint)

        var x = [Double](repeating: 0, count: 9)
        var y = [Double](repeating: 0, count: 9)
        for i in 0..<9 {
            x[i] = -2.0 + Double(i) * 0.5
            y[i] = tithi.lunarPhase(start + x[i])
        }
        Masa.unwrapAngles(&y, 9)

        let y0 = Masa.inverseLagrange(x, y, 9, 360.0)
        return start + y0
    }

    public func fullMoonNear(_ jdUt: Double) -> Double {
        var x = [Double](repeating: 0, count: 9)
        var y = [Double](repeating: 0, count: 9)
        for i in 0..<9 {
            x[i] = -2.0 + Double(i) * 0.5
            y[i] = tithi.lunarPhase(jdUt + x[i])
        }
        Masa.unwrapAngles(&y, 9)

        let y0 = Masa.inverseLagrange(x, y, 9, 180.0)
        return jdUt + y0
    }

    public func solarRashi(_ jdUt: Double) -> Int {
        let nirayana = ephemeris.solarLongitudeSidereal(jdUt)
        var rashi = Int(ceil(nirayana / 30.0))
        if rashi <= 0 { rashi = 12 }
        if rashi > 12 { rashi = rashi % 12 }
        if rashi == 0 { rashi = 12 }
        return rashi
    }

    public func masaForDate(year: Int, month: Int, day: Int, loc: Location) -> MasaInfo {
        let jd = ephemeris.gregorianToJd(year: year, month: month, day: day)
        var jdRise = ephemeris.sunriseJd(jd, loc)
        if jdRise <= 0 {
            jdRise = jd + 0.5 - loc.utcOffset / 24.0
        }

        let t = tithi.tithiAtMoment(jdRise)

        let lastNm = newMoonBefore(jdRise, t)
        let nextNm = newMoonAfter(jdRise, t)

        let rashiLast = solarRashi(lastNm)
        let rashiNext = solarRashi(nextNm)

        let isAdhika = (rashiLast == rashiNext)

        var masaNum = rashiLast + 1
        if masaNum > 12 { masaNum -= 12 }
        let name = MasaName.fromNumber(masaNum)

        let yearSaka = hinduYearSaka(jdRise, masaNum)
        let yearVikram = hinduYearVikram(yearSaka)

        return MasaInfo(name: name, isAdhika: isAdhika, yearSaka: yearSaka,
                       yearVikram: yearVikram, jdStart: lastNm, jdEnd: nextNm)
    }

    public func hinduYearSaka(_ jdUt: Double, _ masaNum: Int) -> Int {
        let siderealYear = 365.25636
        let ahar = jdUt - 588465.5
        let kali = Int((ahar + Double(4 - masaNum) * 30) / siderealYear)
        return kali - 3179
    }

    public func hinduYearVikram(_ sakaYear: Int) -> Int {
        return sakaYear + 135
    }

    // ===== Lunisolar month start/length =====

    private func amantaMonthStart(_ masa: MasaName, _ sakaYear: Int, _ isAdhika: Bool,
                                   _ loc: Location) -> Double {
        var gy = sakaYear + 78
        var approxGm = masa.rawValue + 3
        if approxGm > 12 {
            approxGm -= 12
            gy += 1
        }

        var estY = gy, estM = approxGm, estD = 15
        var mi = masaForDate(year: estY, month: estM, day: estD, loc: loc)

        for _ in 0..<14 {
            if mi.name == masa && mi.isAdhika == isAdhika && mi.yearSaka == sakaYear {
                break
            }

            let isAdhikaInt = isAdhika ? 0 : 1
            let miAdhikaInt = mi.isAdhika ? 0 : 1
            let targetOrd = sakaYear * 13 + masa.rawValue + isAdhikaInt
            let curOrd = mi.yearSaka * 13 + mi.name.rawValue + miAdhikaInt

            let jdNav: Double
            if targetOrd > curOrd {
                jdNav = mi.jdEnd + 1.0
            } else {
                jdNav = mi.jdStart - 1.0
            }
            let ymd = ephemeris.jdToGregorian(jdNav)
            estY = ymd.year; estM = ymd.month; estD = ymd.day
            mi = masaForDate(year: estY, month: estM, day: estD, loc: loc)
        }

        if mi.name != masa || mi.isAdhika != isAdhika || mi.yearSaka != sakaYear {
            return 0
        }

        let nmYmd = ephemeris.jdToGregorian(mi.jdStart)

        var check = masaForDate(year: nmYmd.year, month: nmYmd.month, day: nmYmd.day, loc: loc)
        if check.name == masa && check.isAdhika == isAdhika && check.yearSaka == sakaYear {
            return ephemeris.gregorianToJd(year: nmYmd.year, month: nmYmd.month, day: nmYmd.day)
        }

        let jdNext = ephemeris.gregorianToJd(year: nmYmd.year, month: nmYmd.month, day: nmYmd.day) + 1
        let next = ephemeris.jdToGregorian(jdNext)
        check = masaForDate(year: next.year, month: next.month, day: next.day, loc: loc)
        if check.name == masa && check.isAdhika == isAdhika && check.yearSaka == sakaYear {
            return jdNext
        }

        let jdNext2 = jdNext + 1
        let next2 = ephemeris.jdToGregorian(jdNext2)
        check = masaForDate(year: next2.year, month: next2.month, day: next2.day, loc: loc)
        if check.name == masa && check.isAdhika == isAdhika && check.yearSaka == sakaYear {
            return jdNext2
        }

        return 0
    }

    public func lunisolarMonthStart(_ masa: MasaName, _ sakaYear: Int, _ isAdhika: Bool,
                                     _ scheme: LunisolarScheme, _ loc: Location) -> Double {
        let key = CacheKey(masa: masa, sakaYear: sakaYear, isAdhika: isAdhika, scheme: scheme)
        if let cached = monthCache[key], cached.jdStart > 0 {
            return cached.jdStart
        }

        let result: Double

        if scheme == .purnimanta {
            let amantaStart = amantaMonthStart(masa, sakaYear, isAdhika, loc)
            if amantaStart == 0 { return 0 }

            let amYmd = ephemeris.jdToGregorian(amantaStart)
            let mi = masaForDate(year: amYmd.year, month: amYmd.month, day: amYmd.day, loc: loc)

            let jdFull = fullMoonNear(mi.jdStart - 15.0)

            let fmYmd = ephemeris.jdToGregorian(jdFull)
            for offset in 0...2 {
                let jdTry = ephemeris.gregorianToJd(year: fmYmd.year, month: fmYmd.month,
                                                     day: fmYmd.day) + Double(offset)
                var jr = ephemeris.sunriseJd(jdTry, loc)
                if jr <= 0 { jr = jdTry + 0.5 - loc.utcOffset / 24.0 }
                let t = tithi.tithiAtMoment(jr)
                if t >= 16 {
                    monthCache[key] = CacheEntry(jdStart: jdTry, length: 0)
                    return jdTry
                }
            }
            return 0
        } else {
            result = amantaMonthStart(masa, sakaYear, isAdhika, loc)
            if result == 0 { return 0 }
        }

        monthCache[key] = CacheEntry(jdStart: result, length: 0)
        return result
    }

    public func lunisolarMonthLength(_ masa: MasaName, _ sakaYear: Int, _ isAdhika: Bool,
                                      _ scheme: LunisolarScheme, _ loc: Location) -> Int {
        let key = CacheKey(masa: masa, sakaYear: sakaYear, isAdhika: isAdhika, scheme: scheme)
        if let cached = monthCache[key], cached.length > 0 {
            return cached.length
        }

        let jdStart = lunisolarMonthStart(masa, sakaYear, isAdhika, scheme, loc)
        if jdStart == 0 { return 0 }

        var length = 0

        if scheme == .purnimanta {
            var nextMasa: MasaName
            var nextSaka: Int
            let nextAdhika = false
            if isAdhika {
                nextMasa = masa
                nextSaka = sakaYear
            } else {
                nextMasa = (masa == .phalguna) ? .chaitra : MasaName.fromNumber(masa.rawValue + 1)
                nextSaka = (masa == .phalguna) ? sakaYear + 1 : sakaYear
            }
            let jdNext = lunisolarMonthStart(nextMasa, nextSaka, nextAdhika, scheme, loc)
            if jdNext > 0 {
                length = Int(jdNext - jdStart)
            }
        } else {
            for d in 28...31 {
                let jd = jdStart + Double(d)
                let ymd = ephemeris.jdToGregorian(jd)
                let mi = masaForDate(year: ymd.year, month: ymd.month, day: ymd.day, loc: loc)
                if mi.name != masa || mi.isAdhika != isAdhika {
                    length = d
                    break
                }
            }
        }

        if length > 0 {
            if var existing = monthCache[key] {
                existing.length = length
                monthCache[key] = existing
            }
        }

        return length
    }
}
