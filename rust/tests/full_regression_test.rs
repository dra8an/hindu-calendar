/// Full regression test: reads C-generated CSVs and verifies every day 1900-2050.
/// Lunisolar: 55,152 days × 4 checks (tithi, masa, adhika, saka)
/// Solar: 4 calendars × ~1,811 month-start entries (month, year, greg start date)

use hindu_calendar::ephemeris::Ephemeris;
use hindu_calendar::model::*;
use hindu_calendar::core::{panchang, solar};

const REF_CSV: &str = include_str!("../../validation/moshier/ref_1900_2050.csv");
const TAMIL_CSV: &str = include_str!("../../validation/moshier/solar/tamil_months_1900_2050.csv");
const BENGALI_CSV: &str = include_str!("../../validation/moshier/solar/bengali_months_1900_2050.csv");
const ODIA_CSV: &str = include_str!("../../validation/moshier/solar/odia_months_1900_2050.csv");
const MALAYALAM_CSV: &str = include_str!("../../validation/moshier/solar/malayalam_months_1900_2050.csv");

#[test]
fn test_lunisolar_55152_days() {
    let mut eph = Ephemeris::new();
    let loc = Location::NEW_DELHI;

    let mut count = 0u32;
    let mut failures = 0u32;

    for line in REF_CSV.lines().skip(1) {
        let fields: Vec<&str> = line.split(',').collect();
        if fields.len() < 7 { continue; }

        let year: i32 = fields[0].parse().unwrap();
        let month: i32 = fields[1].parse().unwrap();
        let day: i32 = fields[2].parse().unwrap();
        let exp_tithi: i32 = fields[3].parse().unwrap();
        let exp_masa: i32 = fields[4].parse().unwrap();
        let exp_adhika: i32 = fields[5].parse().unwrap();
        let exp_saka: i32 = fields[6].parse().unwrap();

        let ti = hindu_calendar::core::tithi::tithi_at_sunrise(&mut eph, year, month, day, &loc);
        let hd = panchang::gregorian_to_hindu(&mut eph, year, month, day, &loc);

        let got_tithi = ti.tithi_num;
        let got_masa = hd.masa.number();
        let got_adhika = if hd.is_adhika_masa { 1 } else { 0 };
        let got_saka = hd.year_saka;

        if got_tithi != exp_tithi || got_masa != exp_masa || got_adhika != exp_adhika || got_saka != exp_saka {
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

    eprintln!("Lunisolar regression: {}/{} passed ({} failures)", count - failures, count, failures);
    assert_eq!(failures, 0, "Lunisolar regression: {} failures out of {} days", failures, count);
    assert_eq!(count, 55152, "Expected 55,152 days in CSV");
}

fn test_solar_month_starts(csv: &str, cal_type: SolarCalendarType, cal_name: &str) {
    let mut eph = Ephemeris::new();
    let loc = Location::NEW_DELHI;

    let mut count = 0u32;
    let mut failures = 0u32;

    for line in csv.lines().skip(1) {
        let fields: Vec<&str> = line.split(',').collect();
        if fields.len() < 7 { continue; }

        let exp_month: i32 = fields[0].parse().unwrap();
        let exp_year: i32 = fields[1].parse().unwrap();
        // fields[2] = length (not checked here)
        let greg_year: i32 = fields[3].parse().unwrap();
        let greg_month: i32 = fields[4].parse().unwrap();
        let greg_day: i32 = fields[5].parse().unwrap();
        let exp_month_name: &str = fields[6].trim();

        let sd = solar::gregorian_to_solar(&mut eph, greg_year, greg_month, greg_day, &loc, cal_type);
        let got_name = solar::solar_month_name(sd.month, cal_type);

        if sd.month != exp_month || sd.year != exp_year || got_name != exp_month_name {
            if failures < 20 {
                eprintln!(
                    "FAIL {} {}-{:02}-{:02}: month={}/{} year={}/{} name={}/{}",
                    cal_name, greg_year, greg_month, greg_day,
                    sd.month, exp_month,
                    sd.year, exp_year,
                    got_name, exp_month_name
                );
            }
            failures += 1;
        }

        // Also verify that this is day 1 of the solar month
        if sd.day != 1 {
            if failures < 20 {
                eprintln!(
                    "FAIL {} {}-{:02}-{:02}: expected solar day 1, got {}",
                    cal_name, greg_year, greg_month, greg_day, sd.day
                );
            }
            failures += 1;
        }

        count += 1;
    }

    eprintln!("{} solar regression: {}/{} passed ({} failures)", cal_name, count - failures, count, failures);
    assert_eq!(failures, 0, "{} solar regression: {} failures out of {} entries", cal_name, failures, count);
    assert!(count > 1800, "Expected ~1811 {} solar entries, got {}", cal_name, count);
}

#[test]
fn test_tamil_solar_months() {
    test_solar_month_starts(TAMIL_CSV, SolarCalendarType::Tamil, "Tamil");
}

#[test]
fn test_bengali_solar_months() {
    test_solar_month_starts(BENGALI_CSV, SolarCalendarType::Bengali, "Bengali");
}

#[test]
fn test_odia_solar_months() {
    test_solar_month_starts(ODIA_CSV, SolarCalendarType::Odia, "Odia");
}

#[test]
fn test_malayalam_solar_months() {
    test_solar_month_starts(MALAYALAM_CSV, SolarCalendarType::Malayalam, "Malayalam");
}
