use crate::ephemeris::Ephemeris;
use crate::model::*;
use super::tithi;

/// Inverse Lagrange interpolation
fn inverse_lagrange(x: &[f64], y: &[f64], n: usize, ya: f64) -> f64 {
    let mut total = 0.0;
    for i in 0..n {
        let mut numer = 1.0;
        let mut denom = 1.0;
        for j in 0..n {
            if j != i {
                numer *= ya - y[j];
                denom *= y[i] - y[j];
            }
        }
        total += numer * x[i] / denom;
    }
    total
}

fn unwrap_angles(angles: &mut [f64]) {
    for i in 1..angles.len() {
        if angles[i] < angles[i - 1] {
            angles[i] += 360.0;
        }
    }
}

pub fn new_moon_before(eph: &mut Ephemeris, jd_ut: f64, tithi_hint: i32) -> f64 {
    let start = jd_ut - tithi_hint as f64;
    let mut x = [0.0f64; 17];
    let mut y = [0.0f64; 17];
    for i in 0..17 {
        x[i] = -2.0 + i as f64 * 0.25;
        y[i] = tithi::lunar_phase(eph, start + x[i]);
    }
    unwrap_angles(&mut y);
    let y0 = inverse_lagrange(&x, &y, 17, 360.0);
    start + y0
}

pub fn new_moon_after(eph: &mut Ephemeris, jd_ut: f64, tithi_hint: i32) -> f64 {
    let start = jd_ut + (30 - tithi_hint) as f64;
    let mut x = [0.0f64; 17];
    let mut y = [0.0f64; 17];
    for i in 0..17 {
        x[i] = -2.0 + i as f64 * 0.25;
        y[i] = tithi::lunar_phase(eph, start + x[i]);
    }
    unwrap_angles(&mut y);
    let y0 = inverse_lagrange(&x, &y, 17, 360.0);
    start + y0
}

pub fn solar_rashi(eph: &mut Ephemeris, jd_ut: f64) -> i32 {
    let nirayana = eph.solar_longitude_sidereal(jd_ut);
    let mut rashi = (nirayana / 30.0).ceil() as i32;
    if rashi <= 0 { rashi = 12; }
    if rashi > 12 { rashi = rashi % 12; }
    if rashi == 0 { rashi = 12; }
    rashi
}

pub fn masa_for_date(
    eph: &mut Ephemeris,
    year: i32,
    month: i32,
    day: i32,
    loc: &Location,
) -> MasaInfo {
    let jd = eph.gregorian_to_jd(year, month, day);
    let mut jd_rise = eph.sunrise_jd(jd, loc);
    if jd_rise <= 0.0 {
        jd_rise = jd + 0.5 - loc.utc_offset / 24.0;
    }

    let t = tithi::tithi_at_moment(eph, jd_rise);

    let last_nm = new_moon_before(eph, jd_rise, t);
    let next_nm = new_moon_after(eph, jd_rise, t);

    let rashi_last = solar_rashi(eph, last_nm);
    let rashi_next = solar_rashi(eph, next_nm);

    let is_adhika = rashi_last == rashi_next;

    let mut masa_num = rashi_last + 1;
    if masa_num > 12 { masa_num -= 12; }
    let name = MasaName::from_number(masa_num);

    let year_saka = hindu_year_saka(eph, jd_rise, masa_num);
    let year_vikram = hindu_year_vikram(year_saka);

    MasaInfo {
        name,
        is_adhika,
        year_saka,
        year_vikram,
        jd_start: last_nm,
        jd_end: next_nm,
    }
}

pub fn hindu_year_saka(eph: &mut Ephemeris, jd_ut: f64, masa_num: i32) -> i32 {
    let _ = eph.jd_to_gregorian(jd_ut); // for consistency, but we just need the year formula
    let sidereal_year = 365.25636;
    let ahar = jd_ut - 588465.5;
    let kali = ((ahar + (4 - masa_num) as f64 * 30.0) / sidereal_year) as i32;
    kali - 3179
}

pub fn hindu_year_vikram(saka_year: i32) -> i32 {
    saka_year + 135
}
