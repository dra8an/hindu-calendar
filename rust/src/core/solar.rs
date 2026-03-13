use crate::ephemeris::Ephemeris;
use crate::model::*;
use super::tithi;

// Regional month names
const BENGALI_MONTHS: [&str; 13] = [
    "", "Boishakh", "Joishtho", "Asharh", "Srabon", "Bhadro", "Ashshin",
    "Kartik", "Ogrohaeon", "Poush", "Magh", "Falgun", "Choitro",
];

const TAMIL_MONTHS: [&str; 13] = [
    "", "Chithirai", "Vaikaasi", "Aani", "Aadi", "Aavani", "Purattaasi",
    "Aippasi", "Karthikai", "Maargazhi", "Thai", "Maasi", "Panguni",
];

const ODIA_MONTHS: [&str; 13] = [
    "", "Baisakha", "Jyeshtha", "Ashadha", "Shravana", "Bhadrapada", "Ashvina",
    "Kartika", "Margashirsha", "Pausha", "Magha", "Phalguna", "Chaitra",
];

const MALAYALAM_MONTHS: [&str; 13] = [
    "", "Chingam", "Kanni", "Thulam", "Vrishchikam", "Dhanu", "Makaram",
    "Kumbham", "Meenam", "Medam", "Edavam", "Mithunam", "Karkadakam",
];

struct SolarCalendarConfig {
    first_rashi: i32,
    year_start_rashi: i32,
    gy_offset_on: i32,
    gy_offset_before: i32,
    months: &'static [&'static str; 13],
    era_name: &'static str,
}

fn get_config(cal_type: SolarCalendarType) -> &'static SolarCalendarConfig {
    match cal_type {
        SolarCalendarType::Tamil => &TAMIL_CONFIG,
        SolarCalendarType::Bengali => &BENGALI_CONFIG,
        SolarCalendarType::Odia => &ODIA_CONFIG,
        SolarCalendarType::Malayalam => &MALAYALAM_CONFIG,
    }
}

static TAMIL_CONFIG: SolarCalendarConfig = SolarCalendarConfig {
    first_rashi: 1, year_start_rashi: 1, gy_offset_on: 78, gy_offset_before: 79,
    months: &TAMIL_MONTHS, era_name: "Saka",
};
static BENGALI_CONFIG: SolarCalendarConfig = SolarCalendarConfig {
    first_rashi: 1, year_start_rashi: 1, gy_offset_on: 593, gy_offset_before: 594,
    months: &BENGALI_MONTHS, era_name: "Bangabda",
};
static ODIA_CONFIG: SolarCalendarConfig = SolarCalendarConfig {
    first_rashi: 1, year_start_rashi: 6, gy_offset_on: 592, gy_offset_before: 593,
    months: &ODIA_MONTHS, era_name: "Amli",
};
static MALAYALAM_CONFIG: SolarCalendarConfig = SolarCalendarConfig {
    first_rashi: 5, year_start_rashi: 5, gy_offset_on: 824, gy_offset_before: 825,
    months: &MALAYALAM_MONTHS, era_name: "Kollam",
};

// ---- Critical time computation ----

fn critical_time_jd(
    eph: &mut Ephemeris,
    jd_midnight_ut: f64,
    loc: &Location,
    cal_type: SolarCalendarType,
) -> f64 {
    match cal_type {
        SolarCalendarType::Tamil => {
            eph.sunset_jd(jd_midnight_ut, loc) - 9.5 / (24.0 * 60.0)
        }
        SolarCalendarType::Bengali => {
            jd_midnight_ut - loc.utc_offset / 24.0 + 24.0 / (24.0 * 60.0)
        }
        SolarCalendarType::Odia => {
            jd_midnight_ut + (22.2 - loc.utc_offset) / 24.0
        }
        SolarCalendarType::Malayalam => {
            let sr = eph.sunrise_jd(jd_midnight_ut, loc);
            let ss = eph.sunset_jd(jd_midnight_ut, loc);
            sr + 0.6 * (ss - sr) - 9.5 / (24.0 * 60.0)
        }
    }
}

// ---- Bengali per-rashi tuning ----

fn bengali_tuned_crit(cal_type: SolarCalendarType, base_crit_jd: f64, rashi: i32) -> f64 {
    if cal_type != SolarCalendarType::Bengali { return base_crit_jd; }
    let adjust_min = match rashi {
        4 => 8.0,    // Karkata
        7 => -1.0,   // Tula
        _ => 0.0,
    };
    base_crit_jd + adjust_min / (24.0 * 60.0)
}

fn bengali_day_edge_offset(cal_type: SolarCalendarType, rashi: i32) -> f64 {
    if cal_type != SolarCalendarType::Bengali { return 0.0; }
    match rashi {
        6 => 4.0 / (24.0 * 60.0),    // Kanya: 23:56
        7 => 21.0 / (24.0 * 60.0),   // Tula: 23:39
        9 => 11.0 / (24.0 * 60.0),   // Dhanu: 23:49
        _ => 0.0,
    }
}

fn bengali_rashi_correction(
    eph: &mut Ephemeris,
    cal_type: SolarCalendarType,
    jd_crit: f64,
    rashi: &mut i32,
    lon: &mut f64,
) {
    if cal_type != SolarCalendarType::Bengali { return; }
    let next_r = (*rashi % 12) + 1;
    let tuned = bengali_tuned_crit(cal_type, jd_crit, next_r);
    if tuned > jd_crit {
        let lon2 = eph.solar_longitude_sidereal(tuned);
        let mut r2 = (lon2 / 30.0).floor() as i32 + 1;
        if r2 > 12 { r2 = 12; }
        if r2 == next_r {
            *rashi = next_r;
            *lon = lon2;
        }
    }
}

// ---- Sankranti finding ----

pub fn sankranti_jd(eph: &mut Ephemeris, jd_approx: f64, target_longitude: f64) -> f64 {
    let mut lo = jd_approx - 20.0;
    let mut hi = jd_approx + 20.0;

    let lon_lo = eph.solar_longitude_sidereal(lo);
    let mut diff_lo = lon_lo - target_longitude;
    if diff_lo > 180.0 { diff_lo -= 360.0; }
    if diff_lo < -180.0 { diff_lo += 360.0; }
    if diff_lo >= 0.0 { lo -= 30.0; }

    let threshold = 1e-3 / 86400.0;

    for _ in 0..50 {
        let mid = (lo + hi) / 2.0;
        let lon = eph.solar_longitude_sidereal(mid);
        let mut diff = lon - target_longitude;
        if diff > 180.0 { diff -= 360.0; }
        if diff < -180.0 { diff += 360.0; }

        if diff >= 0.0 { hi = mid; } else { lo = mid; }

        if hi - lo < threshold { break; }
    }

    (lo + hi) / 2.0
}

#[allow(dead_code)]
fn sankranti_before(eph: &mut Ephemeris, jd_ut: f64) -> f64 {
    let lon = eph.solar_longitude_sidereal(jd_ut);
    let mut rashi = (lon / 30.0).floor() as i32 + 1;
    if rashi > 12 { rashi = 12; }
    if rashi < 1 { rashi = 1; }

    let target = (rashi - 1) as f64 * 30.0;
    let mut degrees_past = lon - target;
    if degrees_past < 0.0 { degrees_past += 360.0; }
    let jd_est = jd_ut - degrees_past;

    sankranti_jd(eph, jd_est, target)
}

fn sankranti_to_civil_day(
    eph: &mut Ephemeris,
    jd_sankranti: f64,
    loc: &Location,
    cal_type: SolarCalendarType,
    rashi: i32,
) -> (i32, i32, i32) {
    let day_edge = bengali_day_edge_offset(cal_type, rashi);
    let local_jd = jd_sankranti + loc.utc_offset / 24.0 + 0.5 + day_edge;
    let (sy, sm, sd) = eph.jd_to_gregorian(local_jd.floor());

    let jd_day = eph.gregorian_to_jd(sy, sm, sd);
    let crit = critical_time_jd(eph, jd_day, loc, cal_type);

    let crit = bengali_tuned_crit(cal_type, crit, rashi);

    if jd_sankranti <= crit {
        // Bengali tithi-based override
        if cal_type == SolarCalendarType::Bengali && rashi != 4 {
            let push_next = if rashi == 10 {
                true
            } else {
                let (py, pm, pd) = eph.jd_to_gregorian(jd_day - 1.0);
                let ti = tithi::tithi_at_sunrise(eph, py, pm, pd, loc);
                ti.jd_end <= jd_sankranti
            };
            if push_next {
                return eph.jd_to_gregorian(jd_day + 1.0);
            }
        }
        (sy, sm, sd)
    } else {
        eph.jd_to_gregorian(jd_day + 1.0)
    }
}

fn rashi_to_regional_month(rashi: i32, cal_type: SolarCalendarType) -> i32 {
    let cfg = get_config(cal_type);
    let mut m = rashi - cfg.first_rashi + 1;
    if m <= 0 { m += 12; }
    m
}

fn solar_year(
    eph: &mut Ephemeris,
    jd_ut: f64,
    loc: &Location,
    jd_greg_date: f64,
    cal_type: SolarCalendarType,
) -> i32 {
    let cfg = get_config(cal_type);
    let (gy, _, _) = eph.jd_to_gregorian(jd_ut);

    let target_long = (cfg.year_start_rashi - 1) as f64 * 30.0;
    let mut approx_greg_month = 3 + cfg.year_start_rashi;
    if approx_greg_month > 12 { approx_greg_month -= 12; }

    let jd_year_start_est = eph.gregorian_to_jd(gy, approx_greg_month, 14);
    let jd_year_start = sankranti_jd(eph, jd_year_start_est, target_long);

    let (ysy, ysm, ysd) = sankranti_to_civil_day(eph, jd_year_start, loc, cal_type, cfg.year_start_rashi);
    let jd_year_civil = eph.gregorian_to_jd(ysy, ysm, ysd);

    if jd_greg_date >= jd_year_civil {
        gy - cfg.gy_offset_on
    } else {
        gy - cfg.gy_offset_before
    }
}

// ---- Public API ----

pub fn gregorian_to_solar(
    eph: &mut Ephemeris,
    year: i32,
    month: i32,
    day: i32,
    loc: &Location,
    cal_type: SolarCalendarType,
) -> SolarDate {
    let jd = eph.gregorian_to_jd(year, month, day);
    let jd_crit = critical_time_jd(eph, jd, loc, cal_type);

    let mut lon = eph.solar_longitude_sidereal(jd_crit);
    let mut rashi = (lon / 30.0).floor() as i32 + 1;
    if rashi > 12 { rashi = 12; }
    if rashi < 1 { rashi = 1; }

    bengali_rashi_correction(eph, cal_type, jd_crit, &mut rashi, &mut lon);

    let target = (rashi - 1) as f64 * 30.0;
    let mut degrees_past = lon - target;
    if degrees_past < 0.0 { degrees_past += 360.0; }
    let jd_est = jd_crit - degrees_past;
    let jd_sankranti = sankranti_jd(eph, jd_est, target);

    let (sy, sm, s_day) = sankranti_to_civil_day(eph, jd_sankranti, loc, cal_type, rashi);
    let jd_month_start = eph.gregorian_to_jd(sy, sm, s_day);
    let mut sd_day = (jd - jd_month_start) as i32 + 1;

    let (rashi, jd_sankranti) = if sd_day <= 0 {
        let new_rashi = if rashi == 1 { 12 } else { rashi - 1 };
        let prev_target = (new_rashi - 1) as f64 * 30.0;
        let new_jd_sank = sankranti_jd(eph, jd_sankranti - 28.0, prev_target);
        let (sy2, sm2, sd2) = sankranti_to_civil_day(eph, new_jd_sank, loc, cal_type, new_rashi);
        let jd_ms = eph.gregorian_to_jd(sy2, sm2, sd2);
        sd_day = (jd - jd_ms) as i32 + 1;
        (new_rashi, new_jd_sank)
    } else {
        (rashi, jd_sankranti)
    };

    let reg_month = rashi_to_regional_month(rashi, cal_type);
    let year_val = solar_year(eph, jd_crit, loc, jd, cal_type);

    SolarDate {
        year: year_val,
        month: reg_month,
        day: sd_day,
        rashi,
        jd_sankranti,
    }
}

pub fn solar_to_gregorian(
    eph: &mut Ephemeris,
    sd: &SolarDate,
    cal_type: SolarCalendarType,
    loc: &Location,
) -> (i32, i32, i32) {
    let cfg = get_config(cal_type);
    let mut rashi = sd.month + cfg.first_rashi - 1;
    if rashi > 12 { rashi -= 12; }

    let mut gy = sd.year + cfg.gy_offset_on;

    let rashi_greg_month = 3 + rashi;
    let start_greg_month = 3 + cfg.year_start_rashi;

    if rashi_greg_month > 12 && start_greg_month <= 12 {
        gy += 1;
    } else if rashi_greg_month <= 12 && start_greg_month <= 12 &&
              rashi_greg_month < start_greg_month {
        gy += 1;
    }

    let target_long = (rashi - 1) as f64 * 30.0;
    let mut est_month = rashi_greg_month;
    if est_month > 12 { est_month -= 12; }
    let jd_est = eph.gregorian_to_jd(gy, est_month, 14);
    let jd_sank = sankranti_jd(eph, jd_est, target_long);

    let (sy, sm, s_day) = sankranti_to_civil_day(eph, jd_sank, loc, cal_type, rashi);
    let jd_result = eph.gregorian_to_jd(sy, sm, s_day) + (sd.day - 1) as f64;
    eph.jd_to_gregorian(jd_result)
}

pub fn solar_month_name(month: i32, cal_type: SolarCalendarType) -> &'static str {
    if month < 1 || month > 12 { return "???"; }
    let cfg = get_config(cal_type);
    cfg.months[month as usize]
}

pub fn solar_era_name(cal_type: SolarCalendarType) -> &'static str {
    get_config(cal_type).era_name
}

pub fn solar_month_start(
    eph: &mut Ephemeris,
    month: i32,
    year: i32,
    cal_type: SolarCalendarType,
    loc: &Location,
) -> f64 {
    let cfg = get_config(cal_type);
    let mut rashi = month + cfg.first_rashi - 1;
    if rashi > 12 { rashi -= 12; }

    // Compute approximate Gregorian year/month
    let year_start_month = ((cfg.year_start_rashi - cfg.first_rashi) % 12) + 1;
    let mut months_past = month - year_start_month;
    if months_past < 0 { months_past += 12; }

    let mut gy = year + cfg.gy_offset_on;
    let mut gm_start = 3 + cfg.year_start_rashi;
    if gm_start > 12 { gm_start -= 12; gy += 1; }
    let mut approx_gm = gm_start + months_past;
    while approx_gm > 12 { approx_gm -= 12; gy += 1; }

    let jd_approx = eph.gregorian_to_jd(gy, approx_gm, 14);

    let target = (rashi - 1) as f64 * 30.0;
    let jd_sk = sankranti_jd(eph, jd_approx, target);

    let (cy, cm, cd) = sankranti_to_civil_day(eph, jd_sk, loc, cal_type, rashi);
    let jd_civil = eph.gregorian_to_jd(cy, cm, cd);

    // Verify against gregorian_to_solar()
    let check = gregorian_to_solar(eph, cy, cm, cd, loc, cal_type);
    if check.month != month {
        // Try next day
        let (ny, nm, nd) = eph.jd_to_gregorian(jd_civil + 1.0);
        let check2 = gregorian_to_solar(eph, ny, nm, nd, loc, cal_type);
        if check2.month == month {
            return jd_civil + 1.0;
        }

        // Try previous day
        let (py, pm, pd) = eph.jd_to_gregorian(jd_civil - 1.0);
        let check3 = gregorian_to_solar(eph, py, pm, pd, loc, cal_type);
        if check3.month == month {
            return jd_civil - 1.0;
        }
    }

    jd_civil
}

pub fn solar_month_length(
    eph: &mut Ephemeris,
    month: i32,
    year: i32,
    cal_type: SolarCalendarType,
    loc: &Location,
) -> i32 {
    let jd_start = solar_month_start(eph, month, year, cal_type, loc);

    let next_month = if month == 12 { 1 } else { month + 1 };
    let cfg = get_config(cal_type);
    let year_start_month = ((cfg.year_start_rashi - cfg.first_rashi) % 12) + 1;
    let last_month = if year_start_month == 1 { 12 } else { year_start_month - 1 };
    let next_year = if month == last_month { year + 1 } else { year };

    let jd_end = solar_month_start(eph, next_month, next_year, cal_type, loc);

    (jd_end - jd_start) as i32
}
