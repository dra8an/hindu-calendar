/*
 * surya_siddhanta.h - Classical Surya Siddhanta solar and lunar positions
 *
 * Implements the geocentric epicyclic model of the Surya Siddhanta, a Sanskrit
 * astronomical treatise (c. 4th-12th century CE).  This is NOT an ephemeris in
 * the modern sense and is deliberately far less accurate than the Moshier
 * library in lib/moshier: its purpose is to reproduce a calendar that is itself
 * computed from these classical rules.
 *
 * Used by the Bengali "Suryasiddhanta panjika" (see Docs/SURYASIDDHANTA_PANJIKA.md),
 * which drikpanchang.com publishes alongside the modern Bisuddhasiddhanta
 * calendar.  The two disagree on 26.32% of month starts.
 *
 * Provenance
 * ----------
 * Every constant here is a historical figure stated in the Surya Siddhanta and
 * published in Burgess's 1860 English translation (public domain).  Nothing is
 * derived from any third-party implementation.  The whole model reduces to five
 * integers from the text (see SS_* constants in the .c file) plus a 24-entry
 * sine table given verbatim in II.15-16.
 *
 * Conventions
 * -----------
 * All functions take Julian Day in UT and an observer Location, matching the
 * rest of the project.  Internally the Surya Siddhanta reckons in local time at
 * the reference meridian, so the observer's longitude is applied before the
 * position is computed -- a 5-hour shift at Delhi's longitude moves the sun by
 * ~0.2 degrees, so this is not optional.
 *
 * Longitudes returned are SIDEREAL (nirayana) and must NOT have an ayanamsa
 * subtracted.  The Surya Siddhanta zodiac is anchored to its own epoch.
 */
#ifndef SURYA_SIDDHANTA_H
#define SURYA_SIDDHANTA_H

#include "types.h"

/*
 * surya_solar_longitude - True sidereal solar longitude.
 *
 *   jd_ut: Julian Day in Universal Time.
 *   loc:   Observer location (longitude is used to set local time).
 *   Returns: degrees [0, 360), sidereal.  Do NOT subtract an ayanamsa.
 */
double surya_solar_longitude(double jd_ut, const Location *loc);

/*
 * surya_lunar_longitude - True sidereal lunar longitude.
 *
 *   jd_ut: Julian Day in Universal Time.
 *   loc:   Observer location.
 *   Returns: degrees [0, 360), sidereal.
 */
double surya_lunar_longitude(double jd_ut, const Location *loc);

/*
 * surya_lunar_phase - Moon-sun elongation.
 *
 *   Returns: degrees [0, 360).  0 = new moon, 180 = full moon.
 *
 * The ayanamsa cancels in the difference, as in the drik implementation.
 */
double surya_lunar_phase(double jd_ut, const Location *loc);

/*
 * surya_tithi_at - Tithi number at a moment.
 *
 *   Returns: 1-30.  tithi = floor(lunar_phase / 12) + 1.
 */
int surya_tithi_at(double jd_ut, const Location *loc);

/*
 * surya_tithi_end - Moment the tithi current at jd_ut ends.
 *
 *   Returns: JD (UT) of the next 12-degree elongation boundary.
 *
 * Used by the Bengali tithi rule, which asks whether the tithi prevailing at
 * a given sunrise has already ended by the time of a sankranti.
 */
double surya_tithi_end(double jd_ut, const Location *loc);

/*
 * surya_sankranti - Moment the sun enters a sidereal zodiac sign.
 *
 *   jd_approx:        Starting estimate (search runs forward and back).
 *   target_longitude: Sign boundary in degrees (0, 30, 60, ... 330).
 *   loc:              Observer location.
 *   Returns: JD (UT) of the crossing.
 *
 * Bisection on the sidereal solar longitude, mirroring sankranti_jd() in
 * solar.c but using the Surya Siddhanta sun.
 */
double surya_sankranti(double jd_approx, double target_longitude,
                       const Location *loc);

#endif /* SURYA_SIDDHANTA_H */
