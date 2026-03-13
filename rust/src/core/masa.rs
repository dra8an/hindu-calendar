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
    let mut x = [0.0f64; 9];
    let mut y = [0.0f64; 9];
    for i in 0..9 {
        x[i] = -2.0 + i as f64 * 0.5;
        y[i] = tithi::lunar_phase(eph, start + x[i]);
    }
    unwrap_angles(&mut y);
    let y0 = inverse_lagrange(&x, &y, 9, 360.0);
    start + y0
}

pub fn new_moon_after(eph: &mut Ephemeris, jd_ut: f64, tithi_hint: i32) -> f64 {
    let start = jd_ut + (30 - tithi_hint) as f64;
    let mut x = [0.0f64; 9];
    let mut y = [0.0f64; 9];
    for i in 0..9 {
        x[i] = -2.0 + i as f64 * 0.5;
        y[i] = tithi::lunar_phase(eph, start + x[i]);
    }
    unwrap_angles(&mut y);
    let y0 = inverse_lagrange(&x, &y, 9, 360.0);
    start + y0
}

pub fn full_moon_near(eph: &mut Ephemeris, jd_ut: f64) -> f64 {
    let mut x = [0.0f64; 9];
    let mut y = [0.0f64; 9];
    for i in 0..9 {
        x[i] = -2.0 + i as f64 * 0.5;
        y[i] = tithi::lunar_phase(eph, jd_ut + x[i]);
    }
    unwrap_angles(&mut y);
    let y0 = inverse_lagrange(&x, &y, 9, 180.0);
    jd_ut + y0
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
    let _ = eph.jd_to_gregorian(jd_ut);
    let sidereal_year = 365.25636;
    let ahar = jd_ut - 588465.5;
    let kali = ((ahar + (4 - masa_num) as f64 * 30.0) / sidereal_year) as i32;
    kali - 3179
}

pub fn hindu_year_vikram(saka_year: i32) -> i32 {
    saka_year + 135
}

// ===== Lunisolar month start/length =====

fn amanta_month_start(
    eph: &mut Ephemeris,
    masa: MasaName,
    saka_year: i32,
    is_adhika: bool,
    loc: &Location,
) -> f64 {
    // Estimate approximate Gregorian date
    let mut gy = saka_year + 78;
    let mut approx_gm = masa.number() + 3;
    if approx_gm > 12 {
        approx_gm -= 12;
        gy += 1;
    }

    let (mut est_y, mut est_m, mut est_d) = (gy, approx_gm, 15);
    let mut mi = masa_for_date(eph, est_y, est_m, est_d, loc);

    // Navigate using new moon boundaries
    for _ in 0..14 {
        if mi.name == masa && mi.is_adhika == is_adhika && mi.year_saka == saka_year {
            break;
        }

        let is_adhika_int = if is_adhika { 0 } else { 1 };
        let mi_adhika_int = if mi.is_adhika { 0 } else { 1 };
        let target_ord = saka_year * 13 + masa.number() + is_adhika_int;
        let cur_ord = mi.year_saka * 13 + mi.name.number() + mi_adhika_int;

        let jd_nav = if target_ord > cur_ord {
            mi.jd_end + 1.0
        } else {
            mi.jd_start - 1.0
        };
        let ymd = eph.jd_to_gregorian(jd_nav);
        est_y = ymd.0; est_m = ymd.1; est_d = ymd.2;
        mi = masa_for_date(eph, est_y, est_m, est_d, loc);
    }

    if mi.name != masa || mi.is_adhika != is_adhika || mi.year_saka != saka_year {
        return 0.0;
    }

    // Find the first civil day of this month
    let (nm_y, nm_m, nm_d) = eph.jd_to_gregorian(mi.jd_start);

    let check = masa_for_date(eph, nm_y, nm_m, nm_d, loc);
    if check.name == masa && check.is_adhika == is_adhika && check.year_saka == saka_year {
        return eph.gregorian_to_jd(nm_y, nm_m, nm_d);
    }

    let jd_next = eph.gregorian_to_jd(nm_y, nm_m, nm_d) + 1.0;
    let (ny, nm, nd) = eph.jd_to_gregorian(jd_next);
    let check = masa_for_date(eph, ny, nm, nd, loc);
    if check.name == masa && check.is_adhika == is_adhika && check.year_saka == saka_year {
        return jd_next;
    }

    let jd_next = jd_next + 1.0;
    let (ny, nm, nd) = eph.jd_to_gregorian(jd_next);
    let check = masa_for_date(eph, ny, nm, nd, loc);
    if check.name == masa && check.is_adhika == is_adhika && check.year_saka == saka_year {
        return jd_next;
    }

    0.0
}

pub fn lunisolar_month_start(
    eph: &mut Ephemeris,
    masa: MasaName,
    saka_year: i32,
    is_adhika: bool,
    scheme: LunisolarScheme,
    loc: &Location,
) -> f64 {
    if scheme == LunisolarScheme::Purnimanta {
        let amanta_start = amanta_month_start(eph, masa, saka_year, is_adhika, loc);
        if amanta_start == 0.0 { return 0.0; }

        let (ay, am, ad) = eph.jd_to_gregorian(amanta_start);
        let mi = masa_for_date(eph, ay, am, ad, loc);

        // Find full moon ~15 days before this new moon
        let jd_full = full_moon_near(eph, mi.jd_start - 15.0);

        // Find first civil day on/after full moon with Krishna paksha tithi
        let (fm_y, fm_m, fm_d) = eph.jd_to_gregorian(jd_full);
        for offset in 0..=2 {
            let jd_try = eph.gregorian_to_jd(fm_y, fm_m, fm_d) + offset as f64;
            let mut jr = eph.sunrise_jd(jd_try, loc);
            if jr <= 0.0 { jr = jd_try + 0.5 - loc.utc_offset / 24.0; }
            let t = tithi::tithi_at_moment(eph, jr);
            if t >= 16 {
                return jd_try;
            }
        }
        return 0.0;
    }

    // Amanta
    amanta_month_start(eph, masa, saka_year, is_adhika, loc)
}

pub fn lunisolar_month_length(
    eph: &mut Ephemeris,
    masa: MasaName,
    saka_year: i32,
    is_adhika: bool,
    scheme: LunisolarScheme,
    loc: &Location,
) -> i32 {
    let jd_start = lunisolar_month_start(eph, masa, saka_year, is_adhika, scheme, loc);
    if jd_start == 0.0 { return 0; }

    if scheme == LunisolarScheme::Purnimanta {
        let next_masa = if masa == MasaName::Phalguna { MasaName::Chaitra }
                        else { MasaName::from_number(masa.number() + 1) };
        let (actual_next_masa, next_saka) = if is_adhika {
            (masa, saka_year)
        } else {
            (next_masa, if masa == MasaName::Phalguna { saka_year + 1 } else { saka_year })
        };
        let next_adhika = false;
        let jd_next = lunisolar_month_start(eph, actual_next_masa, next_saka, next_adhika, scheme, loc);
        if jd_next > 0.0 {
            return (jd_next - jd_start) as i32;
        }
        return 0;
    }

    // Amanta: scan days 28-31
    for d in 28..=31 {
        let jd = jd_start + d as f64;
        let (gy, gm, gd) = eph.jd_to_gregorian(jd);
        let mi = masa_for_date(eph, gy, gm, gd, loc);
        if mi.name != masa || mi.is_adhika != is_adhika {
            return d;
        }
    }

    0
}
