pub mod julian_day;
pub mod sun;
pub mod moon;
pub mod ayanamsa;
pub mod rise;

use crate::model::Location;

/// Ephemeris facade — owns all mutable computation state.
pub struct Ephemeris {
    sun_state: sun::SunState,
    moon_state: moon::MoonState,
}

impl Ephemeris {
    pub fn new() -> Self {
        Ephemeris {
            sun_state: sun::SunState::new(),
            moon_state: moon::MoonState::new(),
        }
    }

    pub fn gregorian_to_jd(&self, year: i32, month: i32, day: i32) -> f64 {
        julian_day::gregorian_to_jd(year, month, day)
    }

    pub fn jd_to_gregorian(&self, jd: f64) -> (i32, i32, i32) {
        julian_day::jd_to_gregorian(jd)
    }

    pub fn day_of_week(&self, jd: f64) -> i32 {
        julian_day::day_of_week(jd)
    }

    pub fn solar_longitude(&mut self, jd_ut: f64) -> f64 {
        sun::solar_longitude(&mut self.sun_state, jd_ut)
    }

    pub fn lunar_longitude(&mut self, jd_ut: f64) -> f64 {
        self.moon_state.lunar_longitude(jd_ut)
    }

    pub fn solar_longitude_sidereal(&mut self, jd_ut: f64) -> f64 {
        let sayana = self.solar_longitude(jd_ut);
        let ayan = ayanamsa::ayanamsa(jd_ut);
        let mut nirayana = (sayana - ayan) % 360.0;
        if nirayana < 0.0 { nirayana += 360.0; }
        nirayana
    }

    pub fn ayanamsa(&self, jd_ut: f64) -> f64 {
        ayanamsa::ayanamsa(jd_ut)
    }

    pub fn sunrise_jd(&mut self, jd_ut: f64, loc: &Location) -> f64 {
        rise::sunrise(
            &mut self.sun_state,
            jd_ut - loc.utc_offset / 24.0,
            loc.longitude,
            loc.latitude,
            loc.altitude,
        )
    }

    pub fn sunset_jd(&mut self, jd_ut: f64, loc: &Location) -> f64 {
        rise::sunset(
            &mut self.sun_state,
            jd_ut - loc.utc_offset / 24.0,
            loc.longitude,
            loc.latitude,
            loc.altitude,
        )
    }

    pub fn solar_declination(&mut self, jd_ut: f64) -> f64 {
        sun::solar_declination(&mut self.sun_state, jd_ut)
    }

    pub fn solar_ra(&mut self, jd_ut: f64) -> f64 {
        sun::solar_ra(&mut self.sun_state, jd_ut)
    }

    pub fn nutation_longitude(&self, jd_ut: f64) -> f64 {
        sun::nutation_longitude(jd_ut)
    }
}
