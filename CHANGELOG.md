# Changelog

## 0.1.0 — 2026-02-09

Initial implementation of the Hindu lunisolar calendar (panchang).

### Added

- **Swiss Ephemeris integration**: Vendored C source from github.com/aloistr/swisseph, using Moshier built-in ephemeris with Lahiri ayanamsa (SE_SIDM_LAHIRI)
- **Tithi calculation** (`tithi.c`): Lunar phase computation, tithi at sunrise, tithi boundary finding via bisection, kshaya (skipped) and adhika (repeated) tithi detection
- **Month determination** (`masa.c`): New moon finding via 17-point inverse Lagrange interpolation, solar rashi from sidereal longitude, Amanta month naming, adhika (leap) month detection, Saka and Vikram Samvat year calculation
- **Panchang generation** (`panchang.c`): Gregorian-to-Hindu date conversion, full month panchang generation, tabular and detailed single-day display
- **CLI** (`main.c`): Options for year (`-y`), month (`-m`), day (`-d`), location (`-l LAT,LON`), UTC offset (`-u`), defaults to current month in New Delhi
- **Astronomical wrapper** (`astro.c`): Tropical and sidereal solar/lunar longitude, ayanamsa, sunrise/sunset using disc-center method
- **Test suite**: 21,761 assertions across 6 suites (test_astro, test_tithi, test_masa, test_validation, test_csv_regression, test_adhika_kshaya), all passing
- **Validation**: 54 dates spanning 1950-2045 spot-checked against drikpanchang.com — tithi, masa, adhika status, and Saka year all match
- **CSV reference data**: 55,152-day dataset (1900-2050) with regression testing and dedicated adhika/kshaya tithi coverage (4,269 edge-case days)
- **Build system**: Makefile with `make`, `make test`, `make clean` targets
