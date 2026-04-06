import XCTest
@testable import HinduCalendar

final class DebugTest: XCTestCase {
    func testDebug1923() {
        let sun = Sun()
        let jd0h = 2423463.5
        let lat = 28.6139  // New Delhi
        let lon = 77.2090
        let DEG2RAD = Double.pi / 180.0
        let RAD2DEG = 180.0 / Double.pi

        let phi = lat * DEG2RAD
        let jdNoon = jd0h + 0.5

        // Sidereal time
        let T = (jd0h - 2451545.0) / 36525.0
        var theta0 = 100.46061837 + 36000.770053608 * T + 0.000387933 * T * T - T * T * T / 38710000.0
        theta0 = theta0.truncatingRemainder(dividingBy: 360.0)
        if theta0 < 0 { theta0 += 360.0 }

        let dpsi = sun.nutationLongitude(jdNoon)
        let eps = sun.meanObliquityUt(jdNoon)
        theta0 += dpsi * cos(eps * DEG2RAD)

        let ra = sun.solarRa(jdNoon)
        let decl = sun.solarDeclination(jdNoon)

        // h0 for New Delhi (alt=216m)
        let alt = 216.0
        var atpress = 1013.25 * pow(1.0 - 0.0065 * alt / 288.0, 5.255)
        var r = 34.46
        r = ((atpress - 80.0) / 930.0 / (1.0 + 0.00008 * (r + 39.0) * (0.0 - 10.0)) * r) / 60.0
        var h0 = -r
        h0 -= 16.0 / 60.0
        h0 -= 0.0353 * sqrt(alt)

        let cosH0 = (sin(h0 * DEG2RAD) - sin(phi) * sin(decl * DEG2RAD))
            / (cos(phi) * cos(decl * DEG2RAD))
        let H0deg = acos(cosH0) * RAD2DEG

        var m0 = (ra - lon - theta0) / 360.0
        m0 = m0 - floor(m0)
        var m = m0 - H0deg / 360.0  // sunrise
        m = m - floor(m)

        print("HINDU SUNRISE DEBUG jd0h=\(jd0h) theta0=\(theta0) ra=\(ra) decl=\(decl) H0=\(H0deg) m0=\(m0) m_init=\(m)")

        for iter in 0..<10 {
            let jdTrial = jd0h + m
            let raI = sun.solarRa(jdTrial)
            let declI = sun.solarDeclination(jdTrial)
            let theta = theta0 + 360.985647 * m

            var H = (theta + lon - raI).truncatingRemainder(dividingBy: 360.0)
            if H < 0 { H += 360.0 }
            if H > 180.0 { H -= 360.0 }

            let sinH = sin(phi) * sin(declI * DEG2RAD)
                + cos(phi) * cos(declI * DEG2RAD) * cos(H * DEG2RAD)
            let h = asin(sinH) * RAD2DEG

            let denom = 360.0 * cos(declI * DEG2RAD) * cos(phi) * sin(H * DEG2RAD)
            if abs(denom) < 1e-12 { break }
            let dm = (h - h0) / denom
            m += dm

            print("  iter \(iter): m=\(m) raI=\(raI) declI=\(declI) H=\(H) h=\(h) dm=\(dm)")

            if abs(dm) < 0.0000001 { break }
        }
        print("HINDU final sunrise: JD \(jd0h + m)")
    }
}
