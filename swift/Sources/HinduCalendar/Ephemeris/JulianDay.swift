import Foundation

public enum JulianDay {

    public static func julday(year: Int, month: Int, day: Int, hour: Double = 0.0) -> Double {
        var y = year
        var m = month
        if m <= 2 {
            y -= 1
            m += 12
        }
        let A = y / 100
        let B = 2 - A + A / 4
        return floor(365.25 * Double(y + 4716)) + floor(30.6001 * Double(m + 1))
            + Double(day) + hour / 24.0 + Double(B) - 1524.5
    }

    public static func revjul(_ jd: Double) -> (year: Int, month: Int, day: Int) {
        let jd2 = jd + 0.5
        let Z = floor(jd2)
        let F = jd2 - Z

        let A: Double
        if Z < 2299161.0 {
            A = Z
        } else {
            let alpha = floor((Z - 1867216.25) / 36524.25)
            A = Z + 1 + alpha - floor(alpha / 4.0)
        }

        let B = A + 1524
        let C = floor((B - 122.1) / 365.25)
        let D = floor(365.25 * C)
        let E = floor((B - D) / 30.6001)
        let d = B - D - floor(30.6001 * E) + F
        let day = Int(d)

        let month: Int
        if E < 14 {
            month = Int(E) - 1
        } else {
            month = Int(E) - 13
        }

        let year: Int
        if month > 2 {
            year = Int(C) - 4716
        } else {
            year = Int(C) - 4715
        }

        return (year, month, day)
    }

    public static func dayOfWeek(_ jd: Double) -> Int {
        return (((Int(floor(jd - 2433282 - 1.5)) % 7) + 7) % 7)
    }
}
