package com.hindu.calendar.ephemeris;

import com.hindu.calendar.model.Location;

/**
 * Public facade for the Moshier ephemeris library.
 *
 * NOT thread-safe. Moshier pipeline uses mutable instance fields.
 * Create one instance per thread or synchronize externally.
 */
public class Ephemeris {

    private final MoshierSun sun;
    private final MoshierMoon moon;
    private final MoshierAyanamsa ayanamsa;
    private final MoshierRise rise;

    public Ephemeris() {
        this.sun = new MoshierSun();
        this.moon = new MoshierMoon(sun);
        this.ayanamsa = new MoshierAyanamsa(sun);
        this.rise = new MoshierRise(sun);
    }

    // ===== Julian Day =====

    public double gregorianToJd(int year, int month, int day) {
        return MoshierJulianDay.julday(year, month, day, 0.0);
    }

    /** Returns [year, month, day] */
    public int[] jdToGregorian(double jd) {
        return MoshierJulianDay.revjul(jd);
    }

    /** ISO convention: 0=Mon, 1=Tue, ..., 6=Sun */
    public int dayOfWeek(double jd) {
        return MoshierJulianDay.dayOfWeek(jd);
    }

    // ===== Solar =====

    /** Apparent tropical solar longitude in degrees [0, 360) */
    public double solarLongitude(double jdUt) {
        return sun.solarLongitude(jdUt);
    }

    /** Sidereal solar longitude in degrees [0, 360) */
    public double solarLongitudeSidereal(double jdUt) {
        double sayana = solarLongitude(jdUt);
        double ayan = ayanamsa.ayanamsa(jdUt);
        double nirayana = (sayana - ayan) % 360.0;
        if (nirayana < 0) nirayana += 360.0;
        return nirayana;
    }

    // ===== Lunar =====

    /** Apparent tropical lunar longitude in degrees [0, 360) */
    public double lunarLongitude(double jdUt) {
        return moon.lunarLongitude(jdUt);
    }

    // ===== Ayanamsa =====

    /** Lahiri ayanamsa in degrees (mean, without nutation) */
    public double getAyanamsa(double jdUt) {
        return ayanamsa.ayanamsa(jdUt);
    }

    // ===== Sunrise / Sunset =====

    /** JD (UT) of sunrise for a given JD and location. Returns 0 on error. */
    public double sunriseJd(double jdUt, Location loc) {
        return rise.sunrise(jdUt - loc.utcOffset() / 24.0,
                loc.longitude(), loc.latitude(), loc.altitude());
    }

    /** JD (UT) of sunset for a given JD and location. Returns 0 on error. */
    public double sunsetJd(double jdUt, Location loc) {
        return rise.sunset(jdUt - loc.utcOffset() / 24.0,
                loc.longitude(), loc.latitude(), loc.altitude());
    }
}
