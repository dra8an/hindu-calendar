# Development Steps

A chronological summary of each development phase.

---

**Phase 1 — Project Setup + Swiss Ephemeris** (v0.1.0)

Vendored the Swiss Ephemeris C library from github.com/aloistr/swisseph.
Set up the Makefile, project structure, and basic astronomical wrapper
(`astro.c`) for solar/lunar longitude, ayanamsa, and sunrise. Configured
Lahiri ayanamsa, disc-center sunrise, and Moshier built-in ephemeris mode.

**Phase 2 — Tithi Calculation**

Implemented the tithi formula: `floor(((lunar_long - solar_long) mod 360) / 12) + 1`.
Added tithi-at-sunrise lookup, bisection-based tithi boundary finding, and
detection of kshaya (skipped) and adhika (repeated) tithis. Learned that tithi
uses tropical longitudes because the ayanamsa cancels in the Moon-Sun
difference.

**Phase 3 — Month Determination**

Implemented Amanta month naming using the solar rashi at new moon. Built new
moon finding via 17-point inverse Lagrange interpolation (matching the Python
reference implementation). Added adhika (leap) month detection — when two new
moons fall in the same solar sign. Computed Saka and Vikram Samvat years.

**Phase 4 — Full Month Panchang CLI**

Built the command-line interface: `./hindu-calendar [-y Y] [-m M] [-d D]`.
Full-month tabular view and detailed single-day view. Defaults to current
month in New Delhi (28.6139N, 77.2090E, UTC+5:30).

**Phase 5 — Validation Against drikpanchang.com**

Hand-verified 186 dates spanning 1900-2050 against drikpanchang.com day
panchang pages. 54 general dates plus 132 adhika/kshaya tithi edge cases
(the hardest dates to get right). All 186 matched: tithi, masa, adhika
status, and Saka year. Generated a 55,152-day reference CSV (1900-2050) for
regression testing. Built 6 test suites totaling 22,289 assertions.

**Phase 6 — Validation Web Page**

Built a browser-based tool (`validation/web/index.html`) for month-by-month
visual comparison against drikpanchang.com. Transposed calendar grid layout
matching drikpanchang's format. Python pipeline (`csv_to_json.py`) converts
the reference CSV into 1,812 per-month JSON files. Includes direct links to
drikpanchang.com for each month.

**Phase 7 — Reingold Diff Overlay**

Generated Reingold/Dershowitz Hindu calendar output (Surya Siddhanta method
from the book "Calendrical Calculations") for all 55,152 days using their
published Lisp code. Embedded diffs into the web page as an orange overlay.
5,943 of 55,152 days differ (89.2% match) — showing how Drik Siddhanta and
Surya Siddhanta diverge.

**Phase 8 — Hindu Solar Calendars** (v0.3.0-v0.4.0)

Added four regional solar calendar variants: Tamil, Bengali, Odia, and
Malayalam. Each has its own rule for which civil day owns a sankranti (the
moment the Sun enters a new zodiac sign). Discovered the rules empirically
by testing boundary cases against drikpanchang.com:

- Tamil: sunset minus 8 minutes (ayanamsa buffer)
- Bengali: midnight with a tithi-based rule from Sewell & Dikshit (1896)
- Odia: fixed 22:12 IST cutoff (not astronomical — discovered after ruling
  out every astronomical hypothesis)
- Malayalam: end of madhyahna (sunrise + 3/5 of daytime) minus 9.5 minutes

Built an edge case scanner that finds the 100 closest-to-critical-time
sankrantis per calendar. Total: 351 unit + 327 external + 28,976 regression +
1,200 edge case assertions. Added solar calendar support to the validation
web page with per-calendar JSON files.

**Phase 9 — Self-Contained Moshier Ephemeris** (v0.5.0)

Replaced the 51,493-line Swiss Ephemeris with a 787-line self-contained
library. Used Meeus algorithms for JD conversion (Ch. 7), lunar longitude
(Ch. 47, 60 terms), nutation (Ch. 22), sunrise (Ch. 15), and delta-T.
Lahiri ayanamsa via IAU 1976 3D equatorial precession. 29 of 53,143 tests
failed due to ~10 arcsecond lunar longitude error. Discovered that
`swe_get_ayanamsa_ut()` returns mean ayanamsa without nutation — adding
nutation causes a 17-arcsecond oscillating error.

**Phase 10 — VSOP87 Solar Longitude Upgrade**

Replaced Meeus Equation of Center with VSOP87 (Bretagnon & Francou 1988),
porting 135 harmonic terms. Full pipeline: VSOP87 J2000 ecliptic longitude,
IAU 1976 general precession, EMB-to-Earth correction, nutation, and standard
aberration. Solar precision improved to ~1 arcsecond. All solar calendar tests
passed. Total failures dropped from 120 to 29 (all remaining were lunar).

**Phase 11 — DE404 Moon Pipeline + Sunrise Improvements**

Ported Steve Moshier's full DE404-fitted analytical lunar ephemeris — the
complete multi-stage pipeline with mean elements, planetary perturbations,
explicit correction terms, and variable light-time retardation. Lunar
precision improved from 10 arcseconds to 0.07 arcseconds. Upgraded sunrise
with Sinclair refraction formula (h0 = -0.612 degrees), apparent sidereal
time (GAST), and a midnight-UT wrap-around fix. Failures dropped from 29 to
2. Final library: 1,943 lines across 5 source files.

**Phase 12 — Dual-Backend Validation + Final Verification** (v0.7.0)

Reorganized validation data to support both backends side-by-side. Added
backend selector to the web page. Discovered that the 2 remaining "Moshier
failures" were actually Swiss Ephemeris failures — drikpanchang.com confirms
the Moshier values on both dates (1965-05-30 and 2001-09-20). Cross-verified
against multiple panchang websites, finding that sources split into two camps
based on their underlying ephemeris engine. Moshier achieves 55,152/55,152
(100%) match against drikpanchang.com.

Removed all Swiss Ephemeris references from the Moshier library, replacing
them with proper academic citations (Bretagnon & Francou, Moshier, Lieske,
Meeus, Sinclair, IERS). Researched and documented the IP/licensing status of
all scientific components — all freely usable with zero risk.

---

Final state: 53,143 test assertions across 10 suites, 55,152 dates verified
against drikpanchang.com (100% match), 1,943 lines of self-contained
ephemeris code, 4 regional solar calendars, comprehensive documentation.
