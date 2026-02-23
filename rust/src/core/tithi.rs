use crate::ephemeris::Ephemeris;
use crate::model::*;

pub fn lunar_phase(eph: &mut Ephemeris, jd_ut: f64) -> f64 {
    let moon = eph.lunar_longitude(jd_ut);
    let sun = eph.solar_longitude(jd_ut);
    let mut phase = (moon - sun) % 360.0;
    if phase < 0.0 { phase += 360.0; }
    phase
}

pub fn tithi_at_moment(eph: &mut Ephemeris, jd_ut: f64) -> i32 {
    let phase = lunar_phase(eph, jd_ut);
    let t = (phase / 12.0) as i32 + 1;
    if t > 30 { 30 } else { t }
}

pub fn find_tithi_boundary(
    eph: &mut Ephemeris,
    jd_start: f64,
    jd_end: f64,
    target_tithi: i32,
) -> f64 {
    let target_phase = (target_tithi - 1) as f64 * 12.0;
    let mut lo = jd_start;
    let mut hi = jd_end;

    for _ in 0..50 {
        let mid = (lo + hi) / 2.0;
        let phase = lunar_phase(eph, mid);
        let mut diff = phase - target_phase;
        if diff > 180.0 { diff -= 360.0; }
        if diff < -180.0 { diff += 360.0; }

        if diff >= 0.0 {
            hi = mid;
        } else {
            lo = mid;
        }
    }

    (lo + hi) / 2.0
}

pub fn tithi_at_sunrise(
    eph: &mut Ephemeris,
    year: i32,
    month: i32,
    day: i32,
    loc: &Location,
) -> TithiInfo {
    let jd = eph.gregorian_to_jd(year, month, day);
    let mut jd_rise = eph.sunrise_jd(jd, loc);

    if jd_rise <= 0.0 {
        jd_rise = jd + 0.5 - loc.utc_offset / 24.0;
    }

    let t = tithi_at_moment(eph, jd_rise);

    let paksha = if t <= 15 { Paksha::Shukla } else { Paksha::Krishna };
    let paksha_tithi = if t <= 15 { t } else { t - 15 };

    let jd_start = find_tithi_boundary(eph, jd_rise - 2.0, jd_rise, t);

    let next_tithi = (t % 30) + 1;
    let jd_end = find_tithi_boundary(eph, jd_rise, jd_rise + 2.0, next_tithi);

    // Check for kshaya tithi
    let jd_tomorrow = jd + 1.0;
    let jd_rise_tmrw = eph.sunrise_jd(jd_tomorrow, loc);
    let is_kshaya = if jd_rise_tmrw > 0.0 {
        let t_tmrw = tithi_at_moment(eph, jd_rise_tmrw);
        let diff = ((t_tmrw - t) + 30) % 30;
        diff > 1
    } else {
        false
    };

    TithiInfo {
        tithi_num: t,
        paksha,
        paksha_tithi,
        jd_start,
        jd_end,
        is_kshaya,
    }
}
