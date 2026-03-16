/// NYC validation tests with DST helper function.
/// Ported from tests/test_nyc.c and src/dst.c

use hindu_calendar::ephemeris::Ephemeris;
use hindu_calendar::model::*;
use hindu_calendar::core::{tithi, masa};

// ----- DST helper: US Eastern offset -----

fn dow(y: i32, m: i32, d: i32) -> i32 {
    // Tomohiko Sakamoto's algorithm: 0=Sun, 1=Mon, ..., 6=Sat
    let t = [0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4];
    let y = if m < 3 { y - 1 } else { y };
    ((y + y / 4 - y / 100 + y / 400 + t[(m - 1) as usize] + d) % 7) as i32
}

fn nth_weekday(y: i32, m: i32, nth: i32, wday: i32) -> i32 {
    if nth > 0 {
        let d1_dow = dow(y, m, 1);
        return 1 + ((wday - d1_dow + 7) % 7) + (nth - 1) * 7;
    }
    // Last occurrence
    let mdays = [0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];
    let mut last = mdays[m as usize];
    if m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0) {
        last = 29;
    }
    let last_dow = dow(y, m, last);
    last - ((last_dow - wday + 7) % 7)
}

fn day_of_year(y: i32, m: i32, d: i32) -> i32 {
    let cum = [0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334];
    let mut doy = cum[m as usize] + d;
    if m > 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0) {
        doy += 1;
    }
    doy
}

fn in_dst(y: i32, m: i32, d: i32, sm: i32, sd: i32, em: i32, ed: i32) -> bool {
    let doy = day_of_year(y, m, d);
    let start = day_of_year(y, sm, sd);
    let end = day_of_year(y, em, ed);
    doy >= start && doy < end
}

fn us_eastern_offset(y: i32, m: i32, d: i32) -> f64 {
    // 1900-1917: No DST
    if y < 1918 { return -5.0; }

    // 1918-1919: Last Sun March - Last Sun October
    if y <= 1919 {
        let sd = nth_weekday(y, 3, -1, 0);
        let ed = nth_weekday(y, 10, -1, 0);
        return if in_dst(y, m, d, 3, sd, 10, ed) { -4.0 } else { -5.0 };
    }

    // 1920-1941: No federal DST
    if y <= 1941 { return -5.0; }

    // 1942-1945: War Time
    if y >= 1942 && y <= 1945 {
        if y == 1942 {
            return if m > 2 || (m == 2 && d >= 9) { -4.0 } else { -5.0 };
        }
        if y <= 1944 { return -4.0; }
        // 1945: War Time ended Sep 30
        return if m < 10 || (m == 9 && d <= 30) { -4.0 } else { -5.0 };
    }

    // 1946-1966: Last Sun April - Last Sun September
    if y <= 1966 {
        let sd = nth_weekday(y, 4, -1, 0);
        let ed = nth_weekday(y, 9, -1, 0);
        return if in_dst(y, m, d, 4, sd, 9, ed) { -4.0 } else { -5.0 };
    }

    // 1967-1973: Last Sun April - Last Sun October
    if y <= 1973 {
        let sd = nth_weekday(y, 4, -1, 0);
        let ed = nth_weekday(y, 10, -1, 0);
        return if in_dst(y, m, d, 4, sd, 10, ed) { -4.0 } else { -5.0 };
    }

    // 1974: Energy crisis - Jan 6 start
    if y == 1974 {
        let ed = nth_weekday(y, 10, -1, 0);
        return if in_dst(y, m, d, 1, 6, 10, ed) { -4.0 } else { -5.0 };
    }

    // 1975: Last Sun Feb - Last Sun October
    if y == 1975 {
        let sd = nth_weekday(y, 2, -1, 0);
        let ed = nth_weekday(y, 10, -1, 0);
        return if in_dst(y, m, d, 2, sd, 10, ed) { -4.0 } else { -5.0 };
    }

    // 1976-1986: Last Sun April - Last Sun October
    if y <= 1986 {
        let sd = nth_weekday(y, 4, -1, 0);
        let ed = nth_weekday(y, 10, -1, 0);
        return if in_dst(y, m, d, 4, sd, 10, ed) { -4.0 } else { -5.0 };
    }

    // 1987-2006: First Sun April - Last Sun October
    if y <= 2006 {
        let sd = nth_weekday(y, 4, 1, 0);
        let ed = nth_weekday(y, 10, -1, 0);
        return if in_dst(y, m, d, 4, sd, 10, ed) { -4.0 } else { -5.0 };
    }

    // 2007+: Second Sun March - First Sun November
    let sd = nth_weekday(y, 3, 2, 0);
    let ed = nth_weekday(y, 11, 1, 0);
    if in_dst(y, m, d, 3, sd, 11, ed) { -4.0 } else { -5.0 }
}

// ----- DST rule tests -----

#[test]
fn test_dst_rules() {
    // 1900-1917: No DST
    assert_eq!(us_eastern_offset(1910, 6, 15), -5.0, "1910-06-15 = EST");

    // 1918: Last Sun March (Mar 31) - Last Sun Oct (Oct 27)
    assert_eq!(us_eastern_offset(1918, 3, 30), -5.0, "1918-03-30 = EST");
    assert_eq!(us_eastern_offset(1918, 3, 31), -4.0, "1918-03-31 = EDT");
    assert_eq!(us_eastern_offset(1918, 10, 26), -4.0, "1918-10-26 = EDT");
    assert_eq!(us_eastern_offset(1918, 10, 27), -5.0, "1918-10-27 = EST");

    // 1920-1941: No federal DST
    assert_eq!(us_eastern_offset(1935, 7, 1), -5.0, "1935-07-01 = EST");

    // 1942-1945: War Time
    assert_eq!(us_eastern_offset(1942, 2, 8), -5.0, "1942-02-08 = EST");
    assert_eq!(us_eastern_offset(1942, 2, 9), -4.0, "1942-02-09 = EDT (War Time)");
    assert_eq!(us_eastern_offset(1943, 1, 1), -4.0, "1943-01-01 = EDT (War Time)");
    assert_eq!(us_eastern_offset(1944, 12, 31), -4.0, "1944-12-31 = EDT (War Time)");
    assert_eq!(us_eastern_offset(1945, 9, 30), -4.0, "1945-09-30 = EDT (War Time ends)");
    assert_eq!(us_eastern_offset(1945, 10, 1), -5.0, "1945-10-01 = EST");

    // 1946-1966: Last Sun April - Last Sun September
    // 1960: Last Sun April = Apr 24, Last Sun Sep = Sep 25
    assert_eq!(us_eastern_offset(1960, 4, 23), -5.0, "1960-04-23 = EST");
    assert_eq!(us_eastern_offset(1960, 4, 24), -4.0, "1960-04-24 = EDT");
    assert_eq!(us_eastern_offset(1960, 7, 15), -4.0, "1960-07-15 = EDT");
    assert_eq!(us_eastern_offset(1960, 9, 24), -4.0, "1960-09-24 = EDT");
    assert_eq!(us_eastern_offset(1960, 9, 25), -5.0, "1960-09-25 = EST");

    // 1967-1973: Last Sun April - Last Sun October
    // 1970: Last Sun April = Apr 26, Last Sun Oct = Oct 25
    assert_eq!(us_eastern_offset(1970, 4, 25), -5.0, "1970-04-25 = EST");
    assert_eq!(us_eastern_offset(1970, 4, 26), -4.0, "1970-04-26 = EDT");
    assert_eq!(us_eastern_offset(1970, 10, 24), -4.0, "1970-10-24 = EDT");
    assert_eq!(us_eastern_offset(1970, 10, 25), -5.0, "1970-10-25 = EST");

    // 1974: Energy crisis - Jan 6 start
    assert_eq!(us_eastern_offset(1974, 1, 5), -5.0, "1974-01-05 = EST");
    assert_eq!(us_eastern_offset(1974, 1, 6), -4.0, "1974-01-06 = EDT");

    // 1975: Last Sun Feb (Feb 23)
    assert_eq!(us_eastern_offset(1975, 2, 22), -5.0, "1975-02-22 = EST");
    assert_eq!(us_eastern_offset(1975, 2, 23), -4.0, "1975-02-23 = EDT");

    // 1976-1986: Last Sun April - Last Sun October
    // 1980: Last Sun April = Apr 27, Last Sun Oct = Oct 26
    assert_eq!(us_eastern_offset(1980, 4, 26), -5.0, "1980-04-26 = EST");
    assert_eq!(us_eastern_offset(1980, 4, 27), -4.0, "1980-04-27 = EDT");
    assert_eq!(us_eastern_offset(1980, 10, 25), -4.0, "1980-10-25 = EDT");
    assert_eq!(us_eastern_offset(1980, 10, 26), -5.0, "1980-10-26 = EST");

    // 1987-2006: First Sun April - Last Sun October
    // 2000: First Sun April = Apr 2, Last Sun Oct = Oct 29
    assert_eq!(us_eastern_offset(2000, 4, 1), -5.0, "2000-04-01 = EST");
    assert_eq!(us_eastern_offset(2000, 4, 2), -4.0, "2000-04-02 = EDT");
    assert_eq!(us_eastern_offset(2000, 10, 28), -4.0, "2000-10-28 = EDT");
    assert_eq!(us_eastern_offset(2000, 10, 29), -5.0, "2000-10-29 = EST");

    // 2007+: Second Sun March - First Sun November
    // 2025: DST starts Mar 9, ends Nov 2
    assert_eq!(us_eastern_offset(2025, 3, 8), -5.0, "2025-03-08 = EST");
    assert_eq!(us_eastern_offset(2025, 3, 9), -4.0, "2025-03-09 = EDT");
    assert_eq!(us_eastern_offset(2025, 7, 4), -4.0, "2025-07-04 = EDT");
    assert_eq!(us_eastern_offset(2025, 11, 1), -4.0, "2025-11-01 = EDT");
    assert_eq!(us_eastern_offset(2025, 11, 2), -5.0, "2025-11-02 = EST");
    assert_eq!(us_eastern_offset(2025, 12, 25), -5.0, "2025-12-25 = EST");
}

// ----- NYC tithi/masa validation against drikpanchang.com -----

#[test]
fn test_nyc_validation() {
    let mut eph = Ephemeris::new();

    struct NycCase {
        y: i32, m: i32, d: i32,
        exp_tithi: i32,
        exp_masa: i32,
        exp_adhika: bool,
        exp_saka: i32,
        note: &'static str,
    }

    let cases = [
        // Winter (EST)
        NycCase { y: 2025, m: 1, d: 1, exp_tithi: 2, exp_masa: 10, exp_adhika: false, exp_saka: 1946, note: "Jan 1 Pausha S-2" },
        NycCase { y: 2025, m: 1, d: 13, exp_tithi: 15, exp_masa: 10, exp_adhika: false, exp_saka: 1946, note: "Pausha Purnima" },
        NycCase { y: 2025, m: 1, d: 29, exp_tithi: 30, exp_masa: 10, exp_adhika: false, exp_saka: 1946, note: "Pausha Amavasya" },
        NycCase { y: 2025, m: 2, d: 12, exp_tithi: 15, exp_masa: 11, exp_adhika: false, exp_saka: 1946, note: "Magha Purnima" },
        NycCase { y: 2025, m: 2, d: 28, exp_tithi: 1, exp_masa: 12, exp_adhika: false, exp_saka: 1946, note: "Phalguna S-1" },
        // Around DST transition
        NycCase { y: 2025, m: 3, d: 8, exp_tithi: 10, exp_masa: 12, exp_adhika: false, exp_saka: 1946, note: "Mar 8 last EST" },
        NycCase { y: 2025, m: 3, d: 9, exp_tithi: 11, exp_masa: 12, exp_adhika: false, exp_saka: 1946, note: "Mar 9 first EDT" },
        NycCase { y: 2025, m: 3, d: 10, exp_tithi: 12, exp_masa: 12, exp_adhika: false, exp_saka: 1946, note: "Mar 10 EDT" },
        // Hindu New Year
        NycCase { y: 2025, m: 3, d: 29, exp_tithi: 30, exp_masa: 12, exp_adhika: false, exp_saka: 1946, note: "Phalguna Amavasya" },
        NycCase { y: 2025, m: 3, d: 30, exp_tithi: 2, exp_masa: 1, exp_adhika: false, exp_saka: 1947, note: "Chaitra S-2 (NYC)" },
        // Summer (EDT)
        NycCase { y: 2025, m: 6, d: 21, exp_tithi: 26, exp_masa: 3, exp_adhika: false, exp_saka: 1947, note: "Jun 21 solstice" },
        NycCase { y: 2025, m: 7, d: 4, exp_tithi: 9, exp_masa: 4, exp_adhika: false, exp_saka: 1947, note: "Jul 4" },
        NycCase { y: 2025, m: 8, d: 15, exp_tithi: 22, exp_masa: 5, exp_adhika: false, exp_saka: 1947, note: "Aug 15" },
        // Around DST end
        NycCase { y: 2025, m: 11, d: 1, exp_tithi: 11, exp_masa: 8, exp_adhika: false, exp_saka: 1947, note: "Nov 1 last EDT" },
        NycCase { y: 2025, m: 11, d: 2, exp_tithi: 12, exp_masa: 8, exp_adhika: false, exp_saka: 1947, note: "Nov 2 first EST" },
        NycCase { y: 2025, m: 11, d: 3, exp_tithi: 13, exp_masa: 8, exp_adhika: false, exp_saka: 1947, note: "Nov 3 EST" },
        // Winter again
        NycCase { y: 2025, m: 12, d: 25, exp_tithi: 6, exp_masa: 10, exp_adhika: false, exp_saka: 1947, note: "Dec 25" },
        NycCase { y: 2025, m: 12, d: 31, exp_tithi: 12, exp_masa: 10, exp_adhika: false, exp_saka: 1947, note: "Dec 31" },
    ];

    let mut failures = 0;

    for c in &cases {
        let offset = us_eastern_offset(c.y, c.m, c.d);
        let loc = Location { latitude: 40.7128, longitude: -74.0060, altitude: 0.0, utc_offset: offset };

        let ti = tithi::tithi_at_sunrise(&mut eph, c.y, c.m, c.d, &loc);
        let mi = masa::masa_for_date(&mut eph, c.y, c.m, c.d, &loc);

        if ti.tithi_num != c.exp_tithi {
            eprintln!("FAIL {:04}-{:02}-{:02} tithi: got {}, expected {} [{}]",
                     c.y, c.m, c.d, ti.tithi_num, c.exp_tithi, c.note);
            failures += 1;
        }
        if mi.name.number() != c.exp_masa {
            eprintln!("FAIL {:04}-{:02}-{:02} masa: got {}, expected {}",
                     c.y, c.m, c.d, mi.name.number(), c.exp_masa);
            failures += 1;
        }
        if mi.is_adhika != c.exp_adhika {
            eprintln!("FAIL {:04}-{:02}-{:02} adhika: got {}, expected {}",
                     c.y, c.m, c.d, mi.is_adhika, c.exp_adhika);
            failures += 1;
        }
        if mi.year_saka != c.exp_saka {
            eprintln!("FAIL {:04}-{:02}-{:02} saka: got {}, expected {}",
                     c.y, c.m, c.d, mi.year_saka, c.exp_saka);
            failures += 1;
        }
    }

    assert_eq!(failures, 0, "NYC validation: {} failures out of {} dates", failures, cases.len());
}
