import Foundation

class Rise {

    private static let DEG2RAD = Double.pi / 180.0
    private static let RAD2DEG = 180.0 / Double.pi
    private static let SOLAR_SEMIDIAM_ARCMIN = 16.0

    private let sun: Sun

    init(sun: Sun) {
        self.sun = sun
    }

    private static func normalizeDeg(_ d: Double) -> Double {
        var d = d.truncatingRemainder(dividingBy: 360.0)
        if d < 0 { d += 360.0 }
        return d
    }

    private static func sinclairRefractionHorizon(_ atpress: Double, _ attemp: Double) -> Double {
        var r = 34.46
        r = ((atpress - 80.0) / 930.0 / (1.0 + 0.00008 * (r + 39.0) * (attemp - 10.0)) * r) / 60.0
        return r
    }

    private static func siderealTime0h(_ jd0h: Double) -> Double {
        let T = (jd0h - 2451545.0) / 36525.0
        let T2 = T * T
        let T3 = T2 * T
        let theta = 100.46061837 + 36000.770053608 * T + 0.000387933 * T2 - T3 / 38710000.0
        return normalizeDeg(theta)
    }

    private func riseSetForDate(_ jd0h: Double, _ lon: Double, _ lat: Double,
                                 _ h0: Double, _ isRise: Bool) -> Double {
        let phi = lat * Rise.DEG2RAD

        var theta0 = Rise.siderealTime0h(jd0h)
        let jdNoon = jd0h + 0.5
        let dpsi = sun.nutationLongitude(jdNoon)
        let eps = sun.meanObliquityUt(jdNoon)
        theta0 += dpsi * cos(eps * Rise.DEG2RAD)

        let ra = sun.solarRa(jdNoon)
        let decl = sun.solarDeclination(jdNoon)

        let cosH0 = (sin(h0 * Rise.DEG2RAD) - sin(phi) * sin(decl * Rise.DEG2RAD))
            / (cos(phi) * cos(decl * Rise.DEG2RAD))

        if cosH0 < -1.0 || cosH0 > 1.0 {
            return 0.0
        }

        let H0deg = acos(cosH0) * Rise.RAD2DEG

        var m0 = (ra - lon - theta0) / 360.0
        m0 = m0 - floor(m0)

        var m: Double
        if isRise {
            m = m0 - H0deg / 360.0
        } else {
            m = m0 + H0deg / 360.0
        }
        m = m - floor(m)

        for _ in 0..<10 {
            let jdTrial = jd0h + m

            let raI = sun.solarRa(jdTrial)
            let declI = sun.solarDeclination(jdTrial)

            let theta = theta0 + 360.985647 * m

            var H = Rise.normalizeDeg(theta + lon - raI)
            if H > 180.0 { H -= 360.0 }

            let sinH = sin(phi) * sin(declI * Rise.DEG2RAD)
                + cos(phi) * cos(declI * Rise.DEG2RAD) * cos(H * Rise.DEG2RAD)
            let h = asin(sinH) * Rise.RAD2DEG

            let denom = 360.0 * cos(declI * Rise.DEG2RAD) * cos(phi) * sin(H * Rise.DEG2RAD)
            if abs(denom) < 1e-12 { break }
            let dm = (h - h0) / denom
            m += dm

            if abs(dm) < 0.0000001 { break }
        }

        if isRise && m > 0.75 { m -= 1.0 }
        if !isRise && m < 0.25 { m += 1.0 }

        return jd0h + m
    }

    private func riseSet(_ jdUt: Double, _ lon: Double, _ lat: Double,
                          _ alt: Double, _ isRise: Bool) -> Double {
        var atpress = 1013.25
        if alt > 0 {
            atpress = 1013.25 * pow(1.0 - 0.0065 * alt / 288.0, 5.255)
        }
        var h0 = -Rise.sinclairRefractionHorizon(atpress, 0.0)
        h0 -= Rise.SOLAR_SEMIDIAM_ARCMIN / 60.0
        if alt > 0 {
            h0 -= 0.0353 * sqrt(alt)
        }

        let ymd = JulianDay.revjul(jdUt)
        let jd0h = JulianDay.julday(year: ymd.year, month: ymd.month, day: ymd.day, hour: 0.0)

        var result = riseSetForDate(jd0h, lon, lat, h0, isRise)
        if result > 0 && result >= jdUt - 0.0001 {
            return result
        }

        result = riseSetForDate(jd0h + 1.0, lon, lat, h0, isRise)
        return result
    }

    func sunrise(_ jdUt: Double, _ lon: Double, _ lat: Double, _ alt: Double) -> Double {
        return riseSet(jdUt, lon, lat, alt, true)
    }

    func sunset(_ jdUt: Double, _ lon: Double, _ lat: Double, _ alt: Double) -> Double {
        return riseSet(jdUt, lon, lat, alt, false)
    }
}
