/// Adhika/Kshaya tithi validation test.
/// Loads ~4,269 rows from adhika_kshaya_tithis.csv and verifies each.

use hindu_calendar::ephemeris::Ephemeris;
use hindu_calendar::model::*;
use hindu_calendar::core::{tithi, masa};

const CSV: &str = include_str!("../../validation/moshier/adhika_kshaya_tithis.csv");

#[test]
fn test_adhika_kshaya_csv() {
    let mut eph = Ephemeris::new();
    let delhi = Location::NEW_DELHI;

    let mut count = 0u32;
    let mut failures = 0u32;

    for line in CSV.lines().skip(1) {
        let fields: Vec<&str> = line.split(',').collect();
        if fields.len() < 7 { continue; }

        let year: i32 = fields[0].parse().unwrap();
        let month: i32 = fields[1].parse().unwrap();
        let day: i32 = fields[2].parse().unwrap();
        let exp_tithi: i32 = fields[3].parse().unwrap();
        let exp_masa: i32 = fields[4].parse().unwrap();
        let exp_adhika: i32 = fields[5].parse().unwrap();
        let exp_saka: i32 = fields[6].parse().unwrap();

        let ti = tithi::tithi_at_sunrise(&mut eph, year, month, day, &delhi);
        let mi = masa::masa_for_date(&mut eph, year, month, day, &delhi);

        let got_tithi = ti.tithi_num;
        let got_masa = mi.name.number();
        let got_adhika = if mi.is_adhika { 1 } else { 0 };
        let got_saka = mi.year_saka;

        if got_tithi != exp_tithi || got_masa != exp_masa ||
           got_adhika != exp_adhika || got_saka != exp_saka {
            if failures < 20 {
                eprintln!(
                    "FAIL {}-{:02}-{:02}: tithi={}/{} masa={}/{} adhika={}/{} saka={}/{}",
                    year, month, day,
                    got_tithi, exp_tithi,
                    got_masa, exp_masa,
                    got_adhika, exp_adhika,
                    got_saka, exp_saka
                );
            }
            failures += 1;
        }
        count += 1;
    }

    eprintln!("Adhika/Kshaya regression: {}/{} passed ({} failures)", count - failures, count, failures);
    assert_eq!(failures, 0, "Adhika/Kshaya regression: {} failures out of {} rows", failures, count);
    assert!(count > 4000, "Expected ~4,269 rows in CSV, got {}", count);
}
