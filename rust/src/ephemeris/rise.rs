/// Sunrise and sunset calculation
/// Meeus Ch. 15, iterative method with Sinclair refraction and GAST.

use std::f64::consts::PI;
use super::sun::{self, SunState};
use super::julian_day;

const DEG2RAD: f64 = PI / 180.0;
const RAD2DEG: f64 = 180.0 / PI;

fn normalize_deg(d: f64) -> f64 {
    let d = d % 360.0;
    if d < 0.0 { d + 360.0 } else { d }
}

/// Sinclair refraction at horizon (degrees)
fn sinclair_refraction_horizon(atpress: f64, attemp: f64) -> f64 {
    let r = 34.46; // arcminutes at horizon
    ((atpress - 80.0) / 930.0 / (1.0 + 0.00008 * (r + 39.0) * (attemp - 10.0)) * r) / 60.0
}

/// Mean sidereal time at Greenwich at 0h UT, in degrees
fn sidereal_time_0h(jd_0h: f64) -> f64 {
    let t = (jd_0h - 2451545.0) / 36525.0;
    let t2 = t * t;
    let t3 = t2 * t;
    let theta = 100.46061837 + 36000.770053608 * t + 0.000387933 * t2 - t3 / 38710000.0;
    normalize_deg(theta)
}

fn rise_set_for_date(
    state: &mut SunState,
    jd_0h: f64,
    lon: f64,
    lat: f64,
    h0: f64,
    is_rise: bool,
) -> f64 {
    let phi = lat * DEG2RAD;

    // Apparent sidereal time at 0h UT (GAST)
    let mut theta0 = sidereal_time_0h(jd_0h);
    let jd_noon = jd_0h + 0.5;
    let dpsi = sun::nutation_longitude(jd_noon);
    let eps = sun::mean_obliquity(sun::jd_ut_to_tt(jd_noon));
    theta0 += dpsi * (eps * DEG2RAD).cos();

    let ra = sun::solar_ra(state, jd_noon);
    let decl = sun::solar_declination(state, jd_noon);

    // Hour angle
    let cos_h0 = ((h0 * DEG2RAD).sin() - phi.sin() * (decl * DEG2RAD).sin())
        / (phi.cos() * (decl * DEG2RAD).cos());

    if cos_h0 < -1.0 || cos_h0 > 1.0 {
        return 0.0; // circumpolar
    }

    let h0_deg = cos_h0.acos() * RAD2DEG;

    // Approximate transit
    let mut m0 = (ra - lon - theta0) / 360.0;
    m0 = m0 - m0.floor();

    let mut m = if is_rise {
        m0 - h0_deg / 360.0
    } else {
        m0 + h0_deg / 360.0
    };

    m = m - m.floor();

    // Iterate
    for _ in 0..10 {
        let jd_trial = jd_0h + m;
        let ra_i = sun::solar_ra(state, jd_trial);
        let decl_i = sun::solar_declination(state, jd_trial);

        let theta = theta0 + 360.985647 * m;
        let mut h = normalize_deg(theta + lon - ra_i);
        if h > 180.0 { h -= 360.0; }

        let sin_h = phi.sin() * (decl_i * DEG2RAD).sin()
            + phi.cos() * (decl_i * DEG2RAD).cos() * (h * DEG2RAD).cos();
        let alt = sin_h.asin() * RAD2DEG;

        let denom = 360.0 * (decl_i * DEG2RAD).cos() * phi.cos() * (h * DEG2RAD).sin();
        if denom.abs() < 1e-12 { break; }
        let dm = (alt - h0) / denom;
        m += dm;

        if dm.abs() < 0.0000001 { break; }
    }

    // Midnight UT wrap-around
    if is_rise && m > 0.75 { m -= 1.0; }
    if !is_rise && m < 0.25 { m += 1.0; }

    jd_0h + m
}

fn rise_set(
    state: &mut SunState,
    jd_ut: f64,
    lon: f64,
    lat: f64,
    alt: f64,
    is_rise: bool,
) -> f64 {
    let mut atpress = 1013.25f64;
    if alt > 0.0 {
        atpress = 1013.25 * (1.0 - 0.0065 * alt / 288.0).powf(5.255);
    }
    let mut h0 = -sinclair_refraction_horizon(atpress, 0.0);
    if alt > 0.0 {
        h0 -= 0.0353 * alt.sqrt();
    }

    let (_, yr, mo, dy) = julian_day::revjul(jd_ut);
    let jd_0h = julian_day::julday(yr, mo, dy, 0.0);

    let result = rise_set_for_date(state, jd_0h, lon, lat, h0, is_rise);
    if result > 0.0 && result >= jd_ut - 0.0001 {
        return result;
    }

    rise_set_for_date(state, jd_0h + 1.0, lon, lat, h0, is_rise)
}

pub fn sunrise(state: &mut SunState, jd_ut: f64, lon: f64, lat: f64, alt: f64) -> f64 {
    rise_set(state, jd_ut, lon, lat, alt, true)
}

pub fn sunset(state: &mut SunState, jd_ut: f64, lon: f64, lat: f64, alt: f64) -> f64 {
    rise_set(state, jd_ut, lon, lat, alt, false)
}
