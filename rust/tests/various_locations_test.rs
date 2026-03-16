/// Various locations validation test.
/// Loads validation/moshier/various_locations.csv and verifies lunisolar tithi
/// and solar calendar month/day/year for Ujjain, NYC, LA.
/// Ported from tests/test_various_locations.c

use hindu_calendar::ephemeris::Ephemeris;
use hindu_calendar::model::*;
use hindu_calendar::core::{tithi, solar};

const CSV: &str = include_str!("../../validation/moshier/various_locations.csv");

fn parse_solar_type(s: &str) -> Option<SolarCalendarType> {
    match s {
        "tamil" => Some(SolarCalendarType::Tamil),
        "bengali" => Some(SolarCalendarType::Bengali),
        "odia" => Some(SolarCalendarType::Odia),
        "malayalam" => Some(SolarCalendarType::Malayalam),
        _ => None,
    }
}

#[test]
fn test_various_locations() {
    let mut eph = Ephemeris::new();

    let mut count = 0u32;
    let mut failures = 0u32;

    for line in CSV.lines().skip(1) {
        // Parse CSV manually to handle empty fields
        let fields: Vec<&str> = line.split(',').collect();
        if fields.len() < 8 { continue; }

        let calendar = fields[0].trim();
        let _location = fields[1].trim();
        let lat: f64 = fields[2].parse().unwrap_or(0.0);
        let lon: f64 = fields[3].parse().unwrap_or(0.0);
        let utc_offset: f64 = fields[4].parse().unwrap_or(0.0);
        let gy: i32 = fields[5].parse().unwrap_or(0);
        let gm: i32 = fields[6].parse().unwrap_or(0);
        let gd: i32 = fields[7].parse().unwrap_or(0);

        let loc = Location { latitude: lat, longitude: lon, altitude: 0.0, utc_offset };

        if calendar == "lunisolar" {
            let tithi_str = if fields.len() > 8 { fields[8].trim() } else { "" };
            let expected_tithi: i32 = tithi_str.parse().unwrap_or(0);
            if expected_tithi == 0 { continue; }

            let jd = eph.gregorian_to_jd(gy, gm, gd);
            let mut jd_rise = eph.sunrise_jd(jd, &loc);
            if jd_rise <= 0.0 {
                jd_rise = jd + 0.5 - utc_offset / 24.0;
            }
            let actual_tithi = tithi::tithi_at_moment(&mut eph, jd_rise);

            count += 1;
            if actual_tithi != expected_tithi {
                if failures < 20 {
                    eprintln!("FAIL: lunisolar {} {:04}-{:02}-{:02}: tithi expected {}, got {}",
                             _location, gy, gm, gd, expected_tithi, actual_tithi);
                }
                failures += 1;
            }
        } else {
            let sol_month_str = if fields.len() > 9 { fields[9].trim() } else { "" };
            let sol_day_str = if fields.len() > 10 { fields[10].trim() } else { "" };
            let sol_year_str = if fields.len() > 11 { fields[11].trim() } else { "" };

            let expected_month: i32 = sol_month_str.parse().unwrap_or(0);
            let expected_day: i32 = sol_day_str.parse().unwrap_or(0);
            let expected_year: i32 = sol_year_str.parse().unwrap_or(0);
            if expected_month == 0 { continue; }

            let cal_type = match parse_solar_type(calendar) {
                Some(t) => t,
                None => continue,
            };

            let sd = solar::gregorian_to_solar(&mut eph, gy, gm, gd, &loc, cal_type);

            count += 1;
            if sd.month != expected_month || sd.day != expected_day || sd.year != expected_year {
                if failures < 20 {
                    eprintln!("FAIL: {} {} {:04}-{:02}-{:02}: expected m{} d{} y{}, got m{} d{} y{}",
                             calendar, _location, gy, gm, gd,
                             expected_month, expected_day, expected_year,
                             sd.month, sd.day, sd.year);
                }
                failures += 1;
            }
        }
    }

    eprintln!("Various locations: {}/{} passed ({} failures)", count - failures, count, failures);
    assert_eq!(failures, 0, "Various locations: {} failures out of {} checks", failures, count);
    assert!(count > 100, "Expected >100 entries in CSV, got {}", count);
}
