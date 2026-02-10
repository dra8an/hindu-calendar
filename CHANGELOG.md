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
- **Test suite**: 157 tests across 4 suites (test_astro, test_tithi, test_masa, test_validation), all passing
- **Validation**: 26 reference dates across 2012-2025 verified against drikpanchang.com — tithi, masa, adhika status, and Saka year all match
- **Build system**: Makefile with `make`, `make test`, `make clean` targets
