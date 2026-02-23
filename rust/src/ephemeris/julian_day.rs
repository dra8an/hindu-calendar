/// Julian Day <-> Gregorian calendar conversion
/// Algorithms from Meeus, "Astronomical Algorithms", 2nd ed., Ch. 7.

pub fn gregorian_to_jd(year: i32, month: i32, day: i32) -> f64 {
    julday(year, month, day, 0.0)
}

pub fn julday(year: i32, month: i32, day: i32, hour: f64) -> f64 {
    let (y, m) = if month <= 2 {
        (year - 1, month + 12)
    } else {
        (year, month)
    };
    let a = y / 100;
    let b = 2 - a + a / 4;
    (365.25 * (y + 4716) as f64).floor()
        + (30.6001 * (m + 1) as f64).floor()
        + day as f64
        + hour / 24.0
        + b as f64
        - 1524.5
}

pub fn jd_to_gregorian(jd: f64) -> (i32, i32, i32) {
    let (_, year, month, day) = revjul(jd);
    (year, month, day)
}

pub fn revjul(jd: f64) -> (f64, i32, i32, i32) {
    let jd2 = jd + 0.5;
    let z = jd2.floor();
    let f = jd2 - z;
    let a = if z < 2299161.0 {
        z
    } else {
        let alpha = ((z - 1867216.25) / 36524.25).floor();
        z + 1.0 + alpha - (alpha / 4.0).floor()
    };
    let b = a + 1524.0;
    let c = ((b - 122.1) / 365.25).floor();
    let d = (365.25 * c).floor();
    let e = ((b - d) / 30.6001).floor();
    let day_f = b - d - (30.6001 * e).floor() + f;
    let day = day_f as i32;
    let hour = (day_f - day as f64) * 24.0;
    let month = if e < 14.0 { e as i32 - 1 } else { e as i32 - 13 };
    let year = if month > 2 { c as i32 - 4716 } else { c as i32 - 4715 };
    (hour, year, month, day)
}

/// Day of week: 0=Monday, 1=Tuesday, ..., 6=Sunday (ISO convention)
pub fn day_of_week(jd: f64) -> i32 {
    (((jd - 2433282.0 - 1.5).floor() as i64 % 7 + 7) % 7) as i32
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_jd_j2000() {
        let jd = gregorian_to_jd(2000, 1, 1);
        assert!((jd - 2451544.5).abs() < 1e-10);
    }

    #[test]
    fn test_roundtrip() {
        let jd = gregorian_to_jd(2025, 3, 15);
        let (y, m, d) = jd_to_gregorian(jd);
        assert_eq!((y, m, d), (2025, 3, 15));
    }

    #[test]
    fn test_day_of_week() {
        // 2025-01-01 is Wednesday = 2
        let jd = gregorian_to_jd(2025, 1, 1);
        assert_eq!(day_of_week(jd), 2);
    }
}
