import Foundation

public class Tithi {

    private let ephemeris: Ephemeris

    public init(ephemeris: Ephemeris) {
        self.ephemeris = ephemeris
    }

    public func lunarPhase(_ jdUt: Double) -> Double {
        let moon = ephemeris.lunarLongitude(jdUt)
        let sun = ephemeris.solarLongitude(jdUt)
        var phase = (moon - sun).truncatingRemainder(dividingBy: 360.0)
        if phase < 0 { phase += 360.0 }
        return phase
    }

    public func tithiAtMoment(_ jdUt: Double) -> Int {
        let phase = lunarPhase(jdUt)
        var t = Int(phase / 12.0) + 1
        if t > 30 { t = 30 }
        return t
    }

    public func findTithiBoundary(_ jdStart: Double, _ jdEnd: Double, _ targetTithi: Int) -> Double {
        let targetPhase = Double(targetTithi - 1) * 12.0

        var lo = jdStart
        var hi = jdEnd

        for _ in 0..<50 {
            let mid = (lo + hi) / 2.0
            let phase = lunarPhase(mid)

            var diff = phase - targetPhase
            if diff > 180.0 { diff -= 360.0 }
            if diff < -180.0 { diff += 360.0 }

            if diff >= 0 {
                hi = mid
            } else {
                lo = mid
            }
        }

        return (lo + hi) / 2.0
    }

    public func tithiAtSunrise(year: Int, month: Int, day: Int, loc: Location) -> TithiInfo {
        let jd = ephemeris.gregorianToJd(year: year, month: month, day: day)
        var jdRise = ephemeris.sunriseJd(jd, loc)

        if jdRise <= 0 {
            jdRise = jd + 0.5 - loc.utcOffset / 24.0
        }

        let riseUt = jdRise
        let t = tithiAtMoment(riseUt)

        let paksha: Paksha = (t <= 15) ? .shukla : .krishna
        let pakshaTithi = (t <= 15) ? t : t - 15

        let jdStart = findTithiBoundary(riseUt - 2.0, riseUt, t)

        let nextTithi = (t % 30) + 1
        let jdEnd = findTithiBoundary(riseUt, riseUt + 2.0, nextTithi)

        var isKshaya = false
        let jdTomorrow = jd + 1.0
        let jdRiseTmrw = ephemeris.sunriseJd(jdTomorrow, loc)
        if jdRiseTmrw > 0 {
            let tTmrw = tithiAtMoment(jdRiseTmrw)
            let diff = ((tTmrw - t) % 30 + 30) % 30
            isKshaya = (diff > 1)
        }

        return TithiInfo(tithiNum: t, paksha: paksha, pakshaTithi: pakshaTithi,
                        jdStart: jdStart, jdEnd: jdEnd, isKshaya: isKshaya)
    }
}
