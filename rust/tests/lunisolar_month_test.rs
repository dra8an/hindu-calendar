/// Lunisolar month start/length tests.
/// Ported from tests/test_lunisolar_month.c

use hindu_calendar::ephemeris::Ephemeris;
use hindu_calendar::model::*;
use hindu_calendar::core::{tithi, masa};

const LUNISOLAR_MONTHS_CSV: &str = include_str!("../../validation/moshier/lunisolar_months.csv");

// ----- Known month starts verified against drikpanchang.com -----

#[test]
fn test_known_month_starts() {
    let mut eph = Ephemeris::new();
    let delhi = Location::NEW_DELHI;

    struct Case {
        masa: MasaName,
        saka: i32,
        adhika: bool,
        y: i32, m: i32, d: i32,
        label: &'static str,
    }

    let cases = [
        // 2025 months
        Case { masa: MasaName::Chaitra, saka: 1947, adhika: false, y: 2025, m: 3, d: 30, label: "Chaitra 1947" },
        Case { masa: MasaName::Vaishakha, saka: 1947, adhika: false, y: 2025, m: 4, d: 28, label: "Vaishakha 1947" },
        Case { masa: MasaName::Jyeshtha, saka: 1947, adhika: false, y: 2025, m: 5, d: 28, label: "Jyeshtha 1947" },
        Case { masa: MasaName::Ashadha, saka: 1947, adhika: false, y: 2025, m: 6, d: 26, label: "Ashadha 1947" },
        Case { masa: MasaName::Shravana, saka: 1947, adhika: false, y: 2025, m: 7, d: 25, label: "Shravana 1947" },
        Case { masa: MasaName::Bhadrapada, saka: 1947, adhika: false, y: 2025, m: 8, d: 24, label: "Bhadrapada 1947" },
        Case { masa: MasaName::Ashvina, saka: 1947, adhika: false, y: 2025, m: 9, d: 22, label: "Ashvina 1947" },
        Case { masa: MasaName::Kartika, saka: 1947, adhika: false, y: 2025, m: 10, d: 22, label: "Kartika 1947" },
        Case { masa: MasaName::Margashirsha, saka: 1947, adhika: false, y: 2025, m: 11, d: 21, label: "Margashirsha 1947" },
        Case { masa: MasaName::Pausha, saka: 1947, adhika: false, y: 2025, m: 12, d: 21, label: "Pausha 1947" },
        // Adhika Bhadrapada 2012
        Case { masa: MasaName::Bhadrapada, saka: 1934, adhika: true, y: 2012, m: 8, d: 18, label: "Adhika Bhadrapada 1934" },
        Case { masa: MasaName::Bhadrapada, saka: 1934, adhika: false, y: 2012, m: 9, d: 17, label: "Nija Bhadrapada 1934" },
        // Adhika Ashadha 2015
        Case { masa: MasaName::Ashadha, saka: 1937, adhika: true, y: 2015, m: 6, d: 17, label: "Adhika Ashadha 1937" },
        Case { masa: MasaName::Ashadha, saka: 1937, adhika: false, y: 2015, m: 7, d: 17, label: "Nija Ashadha 1937" },
        // Year boundary
        Case { masa: MasaName::Phalguna, saka: 1946, adhika: false, y: 2025, m: 2, d: 28, label: "Phalguna 1946" },
        Case { masa: MasaName::Chaitra, saka: 1946, adhika: false, y: 2024, m: 4, d: 9, label: "Chaitra 1946" },
    ];

    for c in &cases {
        let jd = masa::lunisolar_month_start(&mut eph, c.masa, c.saka, c.adhika,
                                              LunisolarScheme::Amanta, &delhi);
        assert!(jd > 0.0, "{} start should be found", c.label);

        let (gy, gm, gd) = eph.jd_to_gregorian(jd);
        assert_eq!((gy, gm, gd), (c.y, c.m, c.d),
            "{} start date mismatch", c.label);

        // Roundtrip: masa_for_date on start date should give same masa
        let mi = masa::masa_for_date(&mut eph, gy, gm, gd, &delhi);
        assert_eq!(mi.name, c.masa, "{} masa roundtrip", c.label);
        assert_eq!(mi.is_adhika, c.adhika, "{} adhika roundtrip", c.label);
        assert_eq!(mi.year_saka, c.saka, "{} saka roundtrip", c.label);

        // Day before should belong to different month
        let jd_prev = jd - 1.0;
        let (py, pm, pd) = eph.jd_to_gregorian(jd_prev);
        let mi_prev = masa::masa_for_date(&mut eph, py, pm, pd, &delhi);
        assert!(mi_prev.name != c.masa || mi_prev.is_adhika != c.adhika,
            "{} prev day should be different masa", c.label);
    }
}

// ----- Month lengths -----

#[test]
fn test_month_lengths() {
    let mut eph = Ephemeris::new();
    let delhi = Location::NEW_DELHI;

    let months = [
        MasaName::Chaitra, MasaName::Vaishakha, MasaName::Jyeshtha, MasaName::Ashadha,
        MasaName::Shravana, MasaName::Bhadrapada, MasaName::Ashvina, MasaName::Kartika,
        MasaName::Margashirsha, MasaName::Pausha, MasaName::Magha, MasaName::Phalguna,
    ];
    for m in &months {
        let len = masa::lunisolar_month_length(&mut eph, *m, 1947, false,
                                                LunisolarScheme::Amanta, &delhi);
        assert!(len == 29 || len == 30,
            "{:?} 1947 length should be 29 or 30, got {}", m, len);
    }

    // Adhika month length
    let len = masa::lunisolar_month_length(&mut eph, MasaName::Bhadrapada, 1934, true,
                                            LunisolarScheme::Amanta, &delhi);
    assert!(len == 29 || len == 30,
        "Adhika Bhadrapada 1934 length should be 29 or 30, got {}", len);

    // start + length = next month start
    let jd_start = masa::lunisolar_month_start(&mut eph, MasaName::Vaishakha, 1947, false,
                                                LunisolarScheme::Amanta, &delhi);
    let len = masa::lunisolar_month_length(&mut eph, MasaName::Vaishakha, 1947, false,
                                            LunisolarScheme::Amanta, &delhi);
    let jd_next = masa::lunisolar_month_start(&mut eph, MasaName::Jyeshtha, 1947, false,
                                               LunisolarScheme::Amanta, &delhi);
    assert_eq!((jd_next - jd_start) as i32, len, "Vaishakha+length = Jyeshtha start");
}

// ----- Roundtrip test (sample every 10 years to keep time reasonable) -----

#[test]
fn test_roundtrip_sampled() {
    let mut eph = Ephemeris::new();
    let delhi = Location::NEW_DELHI;

    let mut months_tested = 0;
    let mut failures = 0;

    let years: Vec<i32> = (1900..=2050).step_by(10).collect();

    for &y in &years {
        let mut prev_masa: i32 = -1;
        let mut prev_adhika = false;
        let mut prev_saka: i32 = -1;
        let mut first_transition = true;

        for m in 1..=12 {
            let dim = days_in_month(y, m);
            for d in 1..=dim {
                let mi = masa::masa_for_date(&mut eph, y, m, d, &delhi);
                if mi.name.number() != prev_masa || mi.is_adhika != prev_adhika ||
                   mi.year_saka != prev_saka {
                    if first_transition {
                        first_transition = false;
                        prev_masa = mi.name.number();
                        prev_adhika = mi.is_adhika;
                        prev_saka = mi.year_saka;
                        continue;
                    }
                    let jd = masa::lunisolar_month_start(&mut eph, mi.name, mi.year_saka,
                                                          mi.is_adhika,
                                                          LunisolarScheme::Amanta, &delhi);
                    if jd > 0.0 {
                        let (ry, rm, rd) = eph.jd_to_gregorian(jd);
                        if ry != y || rm != m || rd != d {
                            eprintln!("FAIL: {:?}{} {} start: expected {:04}-{:02}-{:02}, got {:04}-{:02}-{:02}",
                                     if mi.is_adhika { "Adhika " } else { "" },
                                     mi.name, mi.year_saka,
                                     y, m, d, ry, rm, rd);
                            failures += 1;
                        }
                    } else {
                        eprintln!("FAIL: {:?}{} {} not found",
                                 if mi.is_adhika { "Adhika " } else { "" },
                                 mi.name, mi.year_saka);
                        failures += 1;
                    }
                    months_tested += 1;
                    prev_masa = mi.name.number();
                    prev_adhika = mi.is_adhika;
                    prev_saka = mi.year_saka;
                }
            }
        }
    }

    eprintln!("Roundtrip: {} months tested, {} failures", months_tested, failures);
    assert_eq!(failures, 0, "Roundtrip: {} failures out of {} months", failures, months_tested);
    assert!(months_tested > 100, "Expected >100 months tested, got {}", months_tested);
}

// ----- CSV regression test -----

#[test]
fn test_csv_regression() {
    let mut eph = Ephemeris::new();
    let delhi = Location::NEW_DELHI;

    let mut count = 0u32;
    let mut failures = 0u32;

    for line in LUNISOLAR_MONTHS_CSV.lines().skip(1) {
        let fields: Vec<&str> = line.split(',').collect();
        if fields.len() < 7 { continue; }

        let masa_num: i32 = fields[0].parse().unwrap();
        let is_adhika: i32 = fields[1].parse().unwrap();
        let saka_year: i32 = fields[2].parse().unwrap();
        let exp_length: i32 = fields[3].parse().unwrap();
        let gy: i32 = fields[4].parse().unwrap();
        let gm: i32 = fields[5].parse().unwrap();
        let gd: i32 = fields[6].parse().unwrap();

        let masa_name = MasaName::from_number(masa_num);
        let adhika = is_adhika != 0;

        // Verify month start
        let jd = masa::lunisolar_month_start(&mut eph, masa_name, saka_year, adhika,
                                              LunisolarScheme::Amanta, &delhi);
        count += 1;
        if jd > 0.0 {
            let (ry, rm, rd) = eph.jd_to_gregorian(jd);
            if ry != gy || rm != gm || rd != gd {
                if failures < 20 {
                    eprintln!("FAIL: {:?} {}: expected {:04}-{:02}-{:02}, got {:04}-{:02}-{:02}",
                             masa_name, saka_year, gy, gm, gd, ry, rm, rd);
                }
                failures += 1;
            }
        } else {
            if failures < 20 {
                eprintln!("FAIL: {:?} {} not found", masa_name, saka_year);
            }
            failures += 1;
        }

        // Verify length
        if exp_length > 0 {
            let calc_len = masa::lunisolar_month_length(&mut eph, masa_name, saka_year, adhika,
                                                         LunisolarScheme::Amanta, &delhi);
            count += 1;
            if calc_len != exp_length {
                if failures < 20 {
                    eprintln!("FAIL: {:?} {} length: expected {}, got {}",
                             masa_name, saka_year, exp_length, calc_len);
                }
                failures += 1;
            }
        }
    }

    eprintln!("CSV regression: {}/{} passed ({} failures)", count - failures, count, failures);
    assert_eq!(failures, 0, "CSV regression: {} failures out of {} checks", failures, count);
    assert!(count > 3000, "Expected >3000 checks from CSV, got {}", count);
}

// ----- Purnimanta month start spot checks -----

#[test]
fn test_purnimanta_month_starts() {
    let mut eph = Ephemeris::new();
    let delhi = Location::NEW_DELHI;

    let months = [
        MasaName::Chaitra, MasaName::Vaishakha, MasaName::Jyeshtha, MasaName::Ashadha,
        MasaName::Shravana, MasaName::Bhadrapada, MasaName::Ashvina, MasaName::Kartika,
        MasaName::Margashirsha, MasaName::Pausha, MasaName::Magha, MasaName::Phalguna,
    ];
    for m in &months {
        let jd_amanta = masa::lunisolar_month_start(&mut eph, *m, 1947, false,
                                                     LunisolarScheme::Amanta, &delhi);
        let jd_purni = masa::lunisolar_month_start(&mut eph, *m, 1947, false,
                                                    LunisolarScheme::Purnimanta, &delhi);

        assert!(jd_purni > 0.0, "Purnimanta {:?} 1947 should be found", m);

        // Purnimanta start should be ~13-17 days before Amanta start
        let diff = jd_amanta - jd_purni;
        assert!(diff >= 13.0 && diff <= 17.0,
            "Purnimanta {:?} 1947 offset {} should be in [13,17]", m, diff);

        // First day should have Krishna paksha tithi (>=16)
        let mut jr = eph.sunrise_jd(jd_purni, &delhi);
        if jr <= 0.0 { jr = jd_purni + 0.5 - delhi.utc_offset / 24.0; }
        let t = tithi::tithi_at_moment(&mut eph, jr);
        assert!(t >= 16 && t <= 30,
            "Purnimanta {:?} 1947 tithi {} should be in Krishna paksha", m, t);
    }
}

// ----- Purnimanta month lengths -----

#[test]
fn test_purnimanta_month_lengths() {
    let mut eph = Ephemeris::new();
    let delhi = Location::NEW_DELHI;

    let months = [
        MasaName::Chaitra, MasaName::Vaishakha, MasaName::Jyeshtha, MasaName::Ashadha,
        MasaName::Shravana, MasaName::Bhadrapada, MasaName::Ashvina, MasaName::Kartika,
        MasaName::Margashirsha, MasaName::Pausha, MasaName::Magha, MasaName::Phalguna,
    ];
    for m in &months {
        let len = masa::lunisolar_month_length(&mut eph, *m, 1947, false,
                                                LunisolarScheme::Purnimanta, &delhi);
        assert!(len == 29 || len == 30,
            "Purnimanta {:?} 1947 length should be 29 or 30, got {}", m, len);
    }

    // start + length = next month start
    let jd_start = masa::lunisolar_month_start(&mut eph, MasaName::Vaishakha, 1947, false,
                                                LunisolarScheme::Purnimanta, &delhi);
    let len = masa::lunisolar_month_length(&mut eph, MasaName::Vaishakha, 1947, false,
                                            LunisolarScheme::Purnimanta, &delhi);
    let jd_next = masa::lunisolar_month_start(&mut eph, MasaName::Jyeshtha, 1947, false,
                                               LunisolarScheme::Purnimanta, &delhi);
    assert_eq!((jd_next - jd_start) as i32, len,
        "Purnimanta Vaishakha+length = Jyeshtha start");
}

// ----- Helper -----

fn days_in_month(year: i32, month: i32) -> i32 {
    const MDAYS: [i32; 13] = [0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];
    if month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) {
        return 29;
    }
    MDAYS[month as usize]
}
