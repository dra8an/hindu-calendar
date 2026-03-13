import Foundation

public class Ephemeris {

    private let sun: Sun
    private let moon: Moon
    private let ayanamsaCalc: Ayanamsa
    private let rise: Rise

    public init() {
        self.sun = Sun()
        self.moon = Moon(sun: sun)
        self.ayanamsaCalc = Ayanamsa(sun: sun)
        self.rise = Rise(sun: sun)
    }

    // ===== Julian Day =====

    public func gregorianToJd(year: Int, month: Int, day: Int) -> Double {
        return JulianDay.julday(year: year, month: month, day: day)
    }

    public func jdToGregorian(_ jd: Double) -> (year: Int, month: Int, day: Int) {
        return JulianDay.revjul(jd)
    }

    public func dayOfWeek(_ jd: Double) -> Int {
        return JulianDay.dayOfWeek(jd)
    }

    // ===== Solar =====

    public func solarLongitude(_ jdUt: Double) -> Double {
        return sun.solarLongitude(jdUt)
    }

    public func solarLongitudeSidereal(_ jdUt: Double) -> Double {
        let sayana = solarLongitude(jdUt)
        let ayan = ayanamsaCalc.ayanamsa(jdUt)
        var nirayana = (sayana - ayan).truncatingRemainder(dividingBy: 360.0)
        if nirayana < 0 { nirayana += 360.0 }
        return nirayana
    }

    // ===== Lunar =====

    public func lunarLongitude(_ jdUt: Double) -> Double {
        return moon.lunarLongitude(jdUt)
    }

    // ===== Ayanamsa =====

    public func getAyanamsa(_ jdUt: Double) -> Double {
        return ayanamsaCalc.ayanamsa(jdUt)
    }

    // ===== Sunrise / Sunset =====

    public func sunriseJd(_ jdUt: Double, _ loc: Location) -> Double {
        return rise.sunrise(jdUt - loc.utcOffset / 24.0,
                           loc.longitude, loc.latitude, loc.altitude)
    }

    public func sunsetJd(_ jdUt: Double, _ loc: Location) -> Double {
        return rise.sunset(jdUt - loc.utcOffset / 24.0,
                          loc.longitude, loc.latitude, loc.altitude)
    }
}
