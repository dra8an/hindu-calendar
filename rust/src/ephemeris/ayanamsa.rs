/// Lahiri ayanamsa — IAU 1976 precession

use std::f64::consts::PI;
use super::sun;

const DEG2RAD: f64 = PI / 180.0;
const RAD2DEG: f64 = 180.0 / PI;
const J2000: f64 = 2451545.0;
const LAHIRI_T0: f64 = 2435553.5;
const LAHIRI_AYAN_T0: f64 = 23.245524743;

fn iau1976_precession_angles(t: f64) -> (f64, f64, f64) {
    let z_big = ((0.017998 * t + 0.30188) * t + 2306.2181) * t * DEG2RAD / 3600.0;
    let z = ((0.018203 * t + 1.09468) * t + 2306.2181) * t * DEG2RAD / 3600.0;
    let theta = ((-0.041833 * t - 0.42665) * t + 2004.3109) * t * DEG2RAD / 3600.0;
    (z_big, z, theta)
}

/// Precess Cartesian equatorial coordinates.
/// direction > 0: from J to J2000
/// direction < 0: from J2000 to J
fn precess_equatorial(x: &mut [f64; 3], j: f64, direction: i32) {
    if j == J2000 { return; }

    let t = (j - J2000) / 36525.0;
    let (z_big, z, theta) = iau1976_precession_angles(t);

    let costh = theta.cos();
    let sinth = theta.sin();
    let cos_z = z_big.cos();
    let sin_z = z_big.sin();
    let cosz = z.cos();
    let sinz = z.sin();
    let a = cos_z * costh;
    let b = sin_z * costh;

    let r;
    if direction > 0 {
        r = [
            (a * cosz - sin_z * sinz) * x[0] + (a * sinz + sin_z * cosz) * x[1] + cos_z * sinth * x[2],
            -(b * cosz + cos_z * sinz) * x[0] - (b * sinz - cos_z * cosz) * x[1] - sin_z * sinth * x[2],
            -sinth * cosz * x[0] - sinth * sinz * x[1] + costh * x[2],
        ];
    } else {
        r = [
            (a * cosz - sin_z * sinz) * x[0] - (b * cosz + cos_z * sinz) * x[1] - sinth * cosz * x[2],
            (a * sinz + sin_z * cosz) * x[0] - (b * sinz - cos_z * cosz) * x[1] - sinth * sinz * x[2],
            cos_z * sinth * x[0] - sin_z * sinth * x[1] + costh * x[2],
        ];
    }
    x[0] = r[0];
    x[1] = r[1];
    x[2] = r[2];
}

fn obliquity_iau1976(jd_tt: f64) -> f64 {
    let t = (jd_tt - J2000) / 36525.0;
    let u = t / 100.0;
    (23.0 + 26.0 / 60.0 + 21.448 / 3600.0
        + (-4680.93 * u - 1.55 * u * u + 1999.25 * u.powi(3)
            - 51.38 * u.powi(4) - 249.67 * u.powi(5)
            - 39.05 * u.powi(6) + 7.12 * u.powi(7)
            + 27.87 * u.powi(8) + 5.79 * u.powi(9)
            + 2.45 * u.powi(10))
            / 3600.0)
        * DEG2RAD
}

fn equatorial_to_ecliptic(x: &mut [f64; 3], eps: f64) {
    let c = eps.cos();
    let s = eps.sin();
    let y1 = c * x[1] + s * x[2];
    let z1 = -s * x[1] + c * x[2];
    x[1] = y1;
    x[2] = z1;
}

/// Lahiri ayanamsa in degrees (MEAN, without nutation)
pub fn ayanamsa(jd_ut: f64) -> f64 {
    let jd_tt = jd_ut + sun::delta_t_days(jd_ut);

    let mut x = [1.0f64, 0.0, 0.0];

    // Precess from target date to J2000
    precess_equatorial(&mut x, jd_tt, 1);

    // Precess from J2000 to t0
    precess_equatorial(&mut x, LAHIRI_T0, -1);

    // Convert to ecliptic of t0
    let eps_t0 = obliquity_iau1976(LAHIRI_T0);
    equatorial_to_ecliptic(&mut x, eps_t0);

    // Get polar longitude
    let lon = x[1].atan2(x[0]) * RAD2DEG;

    let mut ayan = -lon + LAHIRI_AYAN_T0;

    // Normalize to [0, 360)
    ayan = ayan % 360.0;
    if ayan < 0.0 { ayan += 360.0; }

    ayan
}
