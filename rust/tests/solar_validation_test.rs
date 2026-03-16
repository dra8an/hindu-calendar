/// Solar calendar external validation against drikpanchang.com.
/// Ported from tests/test_solar_validation.c — 109 entries.

use hindu_calendar::ephemeris::Ephemeris;
use hindu_calendar::model::*;
use hindu_calendar::core::solar;

struct SolarRef {
    gy: i32, gm: i32, gd: i32,
    cal: SolarCalendarType,
    exp_month: i32,
    exp_day: i32,
    exp_year: i32,
}

const REF_DATA: &[SolarRef] = &[
    // ==== Tamil: Chithirai 1 across 1950-2050, every 5 years ====
    SolarRef { gy: 1950, gm: 4, gd: 14, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1872 },
    SolarRef { gy: 1955, gm: 4, gd: 14, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1877 },
    SolarRef { gy: 1960, gm: 4, gd: 13, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1882 },
    SolarRef { gy: 1965, gm: 4, gd: 13, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1887 },
    SolarRef { gy: 1970, gm: 4, gd: 14, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1892 },
    SolarRef { gy: 1975, gm: 4, gd: 14, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1897 },
    SolarRef { gy: 1980, gm: 4, gd: 13, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1902 },
    SolarRef { gy: 1985, gm: 4, gd: 14, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1907 },
    SolarRef { gy: 1990, gm: 4, gd: 14, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1912 },
    SolarRef { gy: 1995, gm: 4, gd: 14, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1917 },
    SolarRef { gy: 2000, gm: 4, gd: 13, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1922 },
    SolarRef { gy: 2005, gm: 4, gd: 14, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1927 },
    SolarRef { gy: 2010, gm: 4, gd: 14, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1932 },
    SolarRef { gy: 2015, gm: 4, gd: 14, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1937 },
    SolarRef { gy: 2020, gm: 4, gd: 14, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1942 },
    SolarRef { gy: 2025, gm: 4, gd: 14, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1947 },
    SolarRef { gy: 2030, gm: 4, gd: 14, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1952 },
    SolarRef { gy: 2035, gm: 4, gd: 14, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1957 },
    SolarRef { gy: 2040, gm: 4, gd: 14, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1962 },
    SolarRef { gy: 2045, gm: 4, gd: 14, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1967 },
    SolarRef { gy: 2050, gm: 4, gd: 14, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1972 },

    // ==== Tamil: all 12 months of 2025 ====
    SolarRef { gy: 2025, gm: 1, gd: 14, cal: SolarCalendarType::Tamil, exp_month: 10, exp_day: 1, exp_year: 1946 },
    SolarRef { gy: 2025, gm: 2, gd: 13, cal: SolarCalendarType::Tamil, exp_month: 11, exp_day: 1, exp_year: 1946 },
    SolarRef { gy: 2025, gm: 3, gd: 15, cal: SolarCalendarType::Tamil, exp_month: 12, exp_day: 1, exp_year: 1946 },
    SolarRef { gy: 2025, gm: 4, gd: 14, cal: SolarCalendarType::Tamil, exp_month: 1, exp_day: 1, exp_year: 1947 },
    SolarRef { gy: 2025, gm: 5, gd: 15, cal: SolarCalendarType::Tamil, exp_month: 2, exp_day: 1, exp_year: 1947 },
    SolarRef { gy: 2025, gm: 6, gd: 15, cal: SolarCalendarType::Tamil, exp_month: 3, exp_day: 1, exp_year: 1947 },
    SolarRef { gy: 2025, gm: 7, gd: 16, cal: SolarCalendarType::Tamil, exp_month: 4, exp_day: 1, exp_year: 1947 },
    SolarRef { gy: 2025, gm: 8, gd: 17, cal: SolarCalendarType::Tamil, exp_month: 5, exp_day: 1, exp_year: 1947 },
    SolarRef { gy: 2025, gm: 9, gd: 17, cal: SolarCalendarType::Tamil, exp_month: 6, exp_day: 1, exp_year: 1947 },
    SolarRef { gy: 2025, gm: 10, gd: 17, cal: SolarCalendarType::Tamil, exp_month: 7, exp_day: 1, exp_year: 1947 },
    SolarRef { gy: 2025, gm: 11, gd: 16, cal: SolarCalendarType::Tamil, exp_month: 8, exp_day: 1, exp_year: 1947 },
    SolarRef { gy: 2025, gm: 12, gd: 16, cal: SolarCalendarType::Tamil, exp_month: 9, exp_day: 1, exp_year: 1947 },

    // ==== Bengali: Boishakh 1 across 1950-2050 ====
    SolarRef { gy: 1950, gm: 4, gd: 14, cal: SolarCalendarType::Bengali, exp_month: 1, exp_day: 1, exp_year: 1357 },
    SolarRef { gy: 1960, gm: 4, gd: 14, cal: SolarCalendarType::Bengali, exp_month: 1, exp_day: 1, exp_year: 1367 },
    SolarRef { gy: 1970, gm: 4, gd: 15, cal: SolarCalendarType::Bengali, exp_month: 1, exp_day: 1, exp_year: 1377 },
    SolarRef { gy: 1980, gm: 4, gd: 14, cal: SolarCalendarType::Bengali, exp_month: 1, exp_day: 1, exp_year: 1387 },
    SolarRef { gy: 1990, gm: 4, gd: 15, cal: SolarCalendarType::Bengali, exp_month: 1, exp_day: 1, exp_year: 1397 },
    SolarRef { gy: 2000, gm: 4, gd: 14, cal: SolarCalendarType::Bengali, exp_month: 1, exp_day: 1, exp_year: 1407 },
    SolarRef { gy: 2010, gm: 4, gd: 15, cal: SolarCalendarType::Bengali, exp_month: 1, exp_day: 1, exp_year: 1417 },
    SolarRef { gy: 2015, gm: 4, gd: 15, cal: SolarCalendarType::Bengali, exp_month: 1, exp_day: 1, exp_year: 1422 },
    SolarRef { gy: 2025, gm: 4, gd: 15, cal: SolarCalendarType::Bengali, exp_month: 1, exp_day: 1, exp_year: 1432 },
    SolarRef { gy: 2030, gm: 4, gd: 15, cal: SolarCalendarType::Bengali, exp_month: 1, exp_day: 1, exp_year: 1437 },
    SolarRef { gy: 2040, gm: 4, gd: 14, cal: SolarCalendarType::Bengali, exp_month: 1, exp_day: 1, exp_year: 1447 },
    SolarRef { gy: 2050, gm: 4, gd: 15, cal: SolarCalendarType::Bengali, exp_month: 1, exp_day: 1, exp_year: 1457 },

    // ==== Bengali: all 12 months of 2025 ====
    SolarRef { gy: 2025, gm: 1, gd: 15, cal: SolarCalendarType::Bengali, exp_month: 10, exp_day: 1, exp_year: 1431 },
    SolarRef { gy: 2025, gm: 2, gd: 13, cal: SolarCalendarType::Bengali, exp_month: 11, exp_day: 1, exp_year: 1431 },
    SolarRef { gy: 2025, gm: 3, gd: 15, cal: SolarCalendarType::Bengali, exp_month: 12, exp_day: 1, exp_year: 1431 },
    SolarRef { gy: 2025, gm: 4, gd: 15, cal: SolarCalendarType::Bengali, exp_month: 1, exp_day: 1, exp_year: 1432 },
    SolarRef { gy: 2025, gm: 5, gd: 15, cal: SolarCalendarType::Bengali, exp_month: 2, exp_day: 1, exp_year: 1432 },
    SolarRef { gy: 2025, gm: 6, gd: 16, cal: SolarCalendarType::Bengali, exp_month: 3, exp_day: 1, exp_year: 1432 },
    SolarRef { gy: 2025, gm: 7, gd: 17, cal: SolarCalendarType::Bengali, exp_month: 4, exp_day: 1, exp_year: 1432 },
    SolarRef { gy: 2025, gm: 8, gd: 18, cal: SolarCalendarType::Bengali, exp_month: 5, exp_day: 1, exp_year: 1432 },
    SolarRef { gy: 2025, gm: 9, gd: 18, cal: SolarCalendarType::Bengali, exp_month: 6, exp_day: 1, exp_year: 1432 },
    SolarRef { gy: 2025, gm: 10, gd: 18, cal: SolarCalendarType::Bengali, exp_month: 7, exp_day: 1, exp_year: 1432 },
    SolarRef { gy: 2025, gm: 11, gd: 17, cal: SolarCalendarType::Bengali, exp_month: 8, exp_day: 1, exp_year: 1432 },
    SolarRef { gy: 2025, gm: 12, gd: 17, cal: SolarCalendarType::Bengali, exp_month: 9, exp_day: 1, exp_year: 1432 },

    // ==== Odia: all 12 months of 2025 ====
    SolarRef { gy: 2025, gm: 1, gd: 14, cal: SolarCalendarType::Odia, exp_month: 10, exp_day: 1, exp_year: 1432 },
    SolarRef { gy: 2025, gm: 2, gd: 12, cal: SolarCalendarType::Odia, exp_month: 11, exp_day: 1, exp_year: 1432 },
    SolarRef { gy: 2025, gm: 3, gd: 14, cal: SolarCalendarType::Odia, exp_month: 12, exp_day: 1, exp_year: 1432 },
    SolarRef { gy: 2025, gm: 4, gd: 14, cal: SolarCalendarType::Odia, exp_month: 1, exp_day: 1, exp_year: 1432 },
    SolarRef { gy: 2025, gm: 5, gd: 15, cal: SolarCalendarType::Odia, exp_month: 2, exp_day: 1, exp_year: 1432 },
    SolarRef { gy: 2025, gm: 6, gd: 15, cal: SolarCalendarType::Odia, exp_month: 3, exp_day: 1, exp_year: 1432 },
    SolarRef { gy: 2025, gm: 7, gd: 16, cal: SolarCalendarType::Odia, exp_month: 4, exp_day: 1, exp_year: 1432 },
    SolarRef { gy: 2025, gm: 8, gd: 17, cal: SolarCalendarType::Odia, exp_month: 5, exp_day: 1, exp_year: 1432 },
    SolarRef { gy: 2025, gm: 9, gd: 17, cal: SolarCalendarType::Odia, exp_month: 6, exp_day: 1, exp_year: 1433 },
    SolarRef { gy: 2025, gm: 10, gd: 17, cal: SolarCalendarType::Odia, exp_month: 7, exp_day: 1, exp_year: 1433 },
    SolarRef { gy: 2025, gm: 11, gd: 16, cal: SolarCalendarType::Odia, exp_month: 8, exp_day: 1, exp_year: 1433 },
    SolarRef { gy: 2025, gm: 12, gd: 16, cal: SolarCalendarType::Odia, exp_month: 9, exp_day: 1, exp_year: 1433 },

    // ==== Odia: all 12 months of 2030 ====
    SolarRef { gy: 2030, gm: 1, gd: 14, cal: SolarCalendarType::Odia, exp_month: 10, exp_day: 1, exp_year: 1437 },
    SolarRef { gy: 2030, gm: 2, gd: 13, cal: SolarCalendarType::Odia, exp_month: 11, exp_day: 1, exp_year: 1437 },
    SolarRef { gy: 2030, gm: 3, gd: 15, cal: SolarCalendarType::Odia, exp_month: 12, exp_day: 1, exp_year: 1437 },
    SolarRef { gy: 2030, gm: 4, gd: 14, cal: SolarCalendarType::Odia, exp_month: 1, exp_day: 1, exp_year: 1437 },
    SolarRef { gy: 2030, gm: 5, gd: 15, cal: SolarCalendarType::Odia, exp_month: 2, exp_day: 1, exp_year: 1437 },
    SolarRef { gy: 2030, gm: 6, gd: 15, cal: SolarCalendarType::Odia, exp_month: 3, exp_day: 1, exp_year: 1437 },
    SolarRef { gy: 2030, gm: 7, gd: 17, cal: SolarCalendarType::Odia, exp_month: 4, exp_day: 1, exp_year: 1437 },
    SolarRef { gy: 2030, gm: 8, gd: 17, cal: SolarCalendarType::Odia, exp_month: 5, exp_day: 1, exp_year: 1437 },
    SolarRef { gy: 2030, gm: 9, gd: 17, cal: SolarCalendarType::Odia, exp_month: 6, exp_day: 1, exp_year: 1438 },
    SolarRef { gy: 2030, gm: 10, gd: 17, cal: SolarCalendarType::Odia, exp_month: 7, exp_day: 1, exp_year: 1438 },
    SolarRef { gy: 2030, gm: 11, gd: 16, cal: SolarCalendarType::Odia, exp_month: 8, exp_day: 1, exp_year: 1438 },
    SolarRef { gy: 2030, gm: 12, gd: 16, cal: SolarCalendarType::Odia, exp_month: 9, exp_day: 1, exp_year: 1438 },

    // ==== Malayalam: Chingam 1 across 1950-2030, every 5 years ====
    SolarRef { gy: 1950, gm: 8, gd: 17, cal: SolarCalendarType::Malayalam, exp_month: 1, exp_day: 1, exp_year: 1126 },
    SolarRef { gy: 1955, gm: 8, gd: 17, cal: SolarCalendarType::Malayalam, exp_month: 1, exp_day: 1, exp_year: 1131 },
    SolarRef { gy: 1960, gm: 8, gd: 16, cal: SolarCalendarType::Malayalam, exp_month: 1, exp_day: 1, exp_year: 1136 },
    SolarRef { gy: 1965, gm: 8, gd: 17, cal: SolarCalendarType::Malayalam, exp_month: 1, exp_day: 1, exp_year: 1141 },
    SolarRef { gy: 1970, gm: 8, gd: 17, cal: SolarCalendarType::Malayalam, exp_month: 1, exp_day: 1, exp_year: 1146 },
    SolarRef { gy: 1975, gm: 8, gd: 17, cal: SolarCalendarType::Malayalam, exp_month: 1, exp_day: 1, exp_year: 1151 },
    SolarRef { gy: 1985, gm: 8, gd: 17, cal: SolarCalendarType::Malayalam, exp_month: 1, exp_day: 1, exp_year: 1161 },
    SolarRef { gy: 1990, gm: 8, gd: 17, cal: SolarCalendarType::Malayalam, exp_month: 1, exp_day: 1, exp_year: 1166 },
    SolarRef { gy: 1995, gm: 8, gd: 17, cal: SolarCalendarType::Malayalam, exp_month: 1, exp_day: 1, exp_year: 1171 },
    SolarRef { gy: 2000, gm: 8, gd: 17, cal: SolarCalendarType::Malayalam, exp_month: 1, exp_day: 1, exp_year: 1176 },
    SolarRef { gy: 2005, gm: 8, gd: 17, cal: SolarCalendarType::Malayalam, exp_month: 1, exp_day: 1, exp_year: 1181 },
    SolarRef { gy: 2010, gm: 8, gd: 17, cal: SolarCalendarType::Malayalam, exp_month: 1, exp_day: 1, exp_year: 1186 },
    SolarRef { gy: 2015, gm: 8, gd: 17, cal: SolarCalendarType::Malayalam, exp_month: 1, exp_day: 1, exp_year: 1191 },
    SolarRef { gy: 2020, gm: 8, gd: 17, cal: SolarCalendarType::Malayalam, exp_month: 1, exp_day: 1, exp_year: 1196 },
    SolarRef { gy: 2025, gm: 8, gd: 17, cal: SolarCalendarType::Malayalam, exp_month: 1, exp_day: 1, exp_year: 1201 },
    SolarRef { gy: 2030, gm: 8, gd: 17, cal: SolarCalendarType::Malayalam, exp_month: 1, exp_day: 1, exp_year: 1206 },

    // ==== Malayalam: all 12 months of 2025 ====
    SolarRef { gy: 2025, gm: 1, gd: 14, cal: SolarCalendarType::Malayalam, exp_month: 6, exp_day: 1, exp_year: 1200 },
    SolarRef { gy: 2025, gm: 2, gd: 13, cal: SolarCalendarType::Malayalam, exp_month: 7, exp_day: 1, exp_year: 1200 },
    SolarRef { gy: 2025, gm: 3, gd: 15, cal: SolarCalendarType::Malayalam, exp_month: 8, exp_day: 1, exp_year: 1200 },
    SolarRef { gy: 2025, gm: 4, gd: 14, cal: SolarCalendarType::Malayalam, exp_month: 9, exp_day: 1, exp_year: 1200 },
    SolarRef { gy: 2025, gm: 5, gd: 15, cal: SolarCalendarType::Malayalam, exp_month: 10, exp_day: 1, exp_year: 1200 },
    SolarRef { gy: 2025, gm: 6, gd: 15, cal: SolarCalendarType::Malayalam, exp_month: 11, exp_day: 1, exp_year: 1200 },
    SolarRef { gy: 2025, gm: 7, gd: 17, cal: SolarCalendarType::Malayalam, exp_month: 12, exp_day: 1, exp_year: 1200 },
    SolarRef { gy: 2025, gm: 8, gd: 17, cal: SolarCalendarType::Malayalam, exp_month: 1, exp_day: 1, exp_year: 1201 },
    SolarRef { gy: 2025, gm: 9, gd: 17, cal: SolarCalendarType::Malayalam, exp_month: 2, exp_day: 1, exp_year: 1201 },
    SolarRef { gy: 2025, gm: 10, gd: 18, cal: SolarCalendarType::Malayalam, exp_month: 3, exp_day: 1, exp_year: 1201 },
    SolarRef { gy: 2025, gm: 11, gd: 17, cal: SolarCalendarType::Malayalam, exp_month: 4, exp_day: 1, exp_year: 1201 },
    SolarRef { gy: 2025, gm: 12, gd: 16, cal: SolarCalendarType::Malayalam, exp_month: 5, exp_day: 1, exp_year: 1201 },
];

#[test]
fn test_solar_validation() {
    let mut eph = Ephemeris::new();
    let delhi = Location::NEW_DELHI;

    let mut failures = 0;

    for (i, r) in REF_DATA.iter().enumerate() {
        let sd = solar::gregorian_to_solar(&mut eph, r.gy, r.gm, r.gd, &delhi, r.cal);

        if sd.month != r.exp_month {
            eprintln!("FAIL [{}] {:?} {:04}-{:02}-{:02}: month got {}, expected {}",
                     i, r.cal, r.gy, r.gm, r.gd, sd.month, r.exp_month);
            failures += 1;
        }
        if sd.day != r.exp_day {
            eprintln!("FAIL [{}] {:?} {:04}-{:02}-{:02}: day got {}, expected {}",
                     i, r.cal, r.gy, r.gm, r.gd, sd.day, r.exp_day);
            failures += 1;
        }
        if sd.year != r.exp_year {
            eprintln!("FAIL [{}] {:?} {:04}-{:02}-{:02}: year got {}, expected {}",
                     i, r.cal, r.gy, r.gm, r.gd, sd.year, r.exp_year);
            failures += 1;
        }
    }

    assert_eq!(failures, 0, "Solar validation: {} failures out of {} entries", failures, REF_DATA.len());
}
