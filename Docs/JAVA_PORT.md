# Java 21 Port of Hindu Calendar

Complete port of the Hindu calendar C project to Java. Moshier-only backend (no Swiss Ephemeris). Produces identical output to the C implementation across all 55,152 lunisolar days (1900-2050) and all 4 solar calendar regressions — 0 failures. Includes upper limb sunrise, Odia Amli era, Bengali per-rashi tuning, Purnimanta scheme, and lunisolar/solar month start/length APIs.

## Quick Start

```bash
cd java

# Build
./gradlew build

# Run tests (257 tests)
./gradlew test

# Single day
./gradlew run --args="-y 2025 -m 1 -d 18"

# Full month
./gradlew run --args="-y 2025 -m 3"

# Solar calendar
./gradlew run --args="-s tamil -y 2025 -m 4 -d 14"
```

## Project Structure

```
java/
  build.gradle.kts          # Java 18 toolchain, JUnit 5.9.3, application plugin
  settings.gradle.kts        # rootProject.name = "hindu-calendar"
  gradlew / gradlew.bat      # Gradle 8.10 wrapper
  src/
    main/java/com/hindu/calendar/
      model/                  # Records + enums (10 files, ~200 lines) — includes LunisolarScheme
      ephemeris/              # Moshier library (6 files, ~1,800 lines) — upper limb sunrise
      core/                   # Calendar logic (5 files, ~850 lines) — Purnimanta, month APIs
      cli/                    # CLI entry point (1 file, 139 lines)
    test/java/com/hindu/calendar/
      ephemeris/              # EphemerisTest (142 lines)
      core/                   # TithiTest, MasaTest, SolarTest (~500 lines) — month API tests
      validation/             # DrikPanchangValidationTest, FullRegressionTest (~440 lines)

Total: ~3,000 production lines, ~2,600 test lines
```

## Class Mapping (C to Java)

### model/ — Data Types

| Java | C Source | Notes |
|------|----------|-------|
| `Location` (record) | `types.h` Location struct | Includes `NEW_DELHI` constant |
| `Paksha` (enum) | `types.h` Paksha enum | SHUKLA(0), KRISHNA(1) |
| `MasaName` (enum) | `types.h` MasaName + MASA_NAMES[] | number, displayName, `fromNumber()` |
| `TithiInfo` (record) | `types.h` TithiInfo struct | Static `TITHI_NAMES[]` array |
| `MasaInfo` (record) | `types.h` MasaInfo struct | |
| `HinduDate` (record) | `types.h` HinduDate struct | |
| `PanchangDay` (record) | `types.h` PanchangDay struct | |
| `SolarCalendarType` (enum) | `types.h` + `solar.c` configs | firstRashi, yearStartRashi, offsets, eraName, months[] |
| `SolarDate` (record) | `types.h` SolarDate struct | |
| `LunisolarScheme` (enum) | `types.h` LunisolarScheme | AMANTA, PURNIMANTA |

### ephemeris/ — Moshier Ephemeris

| Java | C Source | Lines |
|------|----------|-------|
| `MoshierJulianDay` (pkg-private) | `moshier_jd.c` (56) | 77 |
| `MoshierSun` (pkg-private) | `moshier_sun.c` (726) | 668 |
| `MoshierMoon` (pkg-private) | `moshier_moon.c` (760) | 687 |
| `MoshierAyanamsa` (pkg-private) | `moshier_ayanamsa.c` (147) | 120 |
| `MoshierRise` (pkg-private) | `moshier_rise.c` (176) | 155 | Upper limb (solar semi-diameter) |
| `Ephemeris` (**public** facade) | `astro.c` + `date_utils.c` | 84 |

`Ephemeris` public API:
- `gregorianToJd(y, m, d)`, `jdToGregorian(jd)`, `dayOfWeek(jd)`
- `solarLongitude(jdUt)`, `lunarLongitude(jdUt)`, `solarLongitudeSidereal(jdUt)`, `getAyanamsa(jdUt)`
- `sunriseJd(jdUt, Location)`, `sunsetJd(jdUt, Location)`

### core/ — Calendar Logic

| Java | C Source | Lines |
|------|----------|-------|
| `DateUtils` | `date_utils.c` (partial) | 33 |
| `Tithi` | `tithi.c` (96) | 85 |
| `Masa` | `masa.c` (300+) | ~290 | fullMoonNear, lunisolarMonthStart/Length, cache |
| `Panchang` | `panchang.c` (164) | 154 |
| `Solar` | `solar.c` (448) | ~325 | Bengali tuning, solarMonthStart/Length, yearStartRashi |

### cli/

| Java | C Source | Lines |
|------|----------|-------|
| `HinduCalendarCli` | `main.c` (191) | 139 |

## Key Porting Decisions

### Numerical Equivalence

Java `double` and C `double` are both IEEE 754 64-bit. `Math.floor/sin/cos/atan2/asin/acos` match C `<math.h>`. Java `%` on doubles behaves like C `fmod`. The port produces bit-identical results for all tested dates.

### C Types to Java

| C Type | Java Type | Notes |
|--------|-----------|-------|
| `double` | `double` | Identical (IEEE 754) |
| `signed char` | `byte` | Both signed 8-bit; sign-extends to int identically |
| `short` | `short` | Both signed 16-bit |
| `int` | `int` | Both signed 32-bit |

### Pointer Arithmetic to Index Variables

Every C `*p++` pattern becomes `array[idx++]` in Java. This is the most common transformation in the VSOP87 and DE404 code:

```c
// C (moshier_sun.c)
const signed char *pl = earargs;
double su = *p++;
for (int k = 0; k < 9; k++) { cv += ss[k][*pl++]; sv += cc[k][*pl++]; }
```

```java
// Java (MoshierSun.java)
int plIdx = argsOfs;
double su = EARTABL[pIdx++];
for (int k = 0; k < 9; k++) { cv += ss[k][EARARGS[plIdx++]]; sv += cc[k][EARARGS[plIdx++]]; }
```

### Output Parameters to Return Values

C functions with output pointers return Java arrays or records:

```c
// C
void jd_to_gregorian(double jd, int *y, int *m, int *d);
```

```java
// Java
int[] jdToGregorian(double jd);  // returns {y, m, d}
```

### Constructor Injection

C `extern` declarations and global state become constructor injection in Java. MoshierMoon, MoshierAyanamsa, and MoshierRise all take a `MoshierSun` reference for shared delta-T, nutation, and obliquity computations.

### Thread Safety

NOT thread-safe, matching the C implementation. Moshier pipeline uses mutable instance fields (sin/cos tables, moon state variables like T, T2, LP, MP, D, NF, moonpol0, etc.). Each `Ephemeris` instance should be used from a single thread.

## Key Porting Traps Encountered

1. **VSOP87 pointer walking**: The nested loop in `calc_vsop_earth()` walks two arrays (`earth_coeffs` / `EARTABL` for coefficients, `earth_args` / `EARARGS` for argument indices) with different strides. Getting the index arithmetic wrong produces subtly wrong solar longitudes. Must carefully track `pIdx` and `plIdx` through the coefficient/harmonic structure.

2. **DE404 `accum_series()` function**: Accumulates table values into moonpol using pointer walking in C. Java version uses explicit index tracking through the lunar data arrays.

3. **Mutable state in moon pipeline**: The DE404 pipeline has ~25 mutable fields that persist across calls. These are instance fields in `MoshierMoon`, not local variables, because intermediate results (mean elements, planetary perturbations) are shared between `moon1()`...`moon4()`.

4. **Ayanamsa nutation**: `swe_get_ayanamsa_ut()` returns MEAN ayanamsa (without nutation). Our `MoshierAyanamsa.ayanamsa()` must NOT add nutation. Nutation cancels in sidereal position: `sid = (trop + dpsi) - (ayan + dpsi) = trop - ayan`.

5. **JD noon convention**: Julian Day numbers are noon-based. When converting to local time, add 0.5 to get midnight-based, then add the UTC offset.

## Test Suite

**257 tests, 0 failures.**

| Test Class | Tests | What It Covers |
|------------|-------|----------------|
| `EphemerisTest` | 12 | JD conversion, round-trips, day-of-week, solar/lunar longitude ranges, ayanamsa, Delhi sunrise/sunset |
| `TithiTest` | 4 | 7 known dates with tithi + paksha, kshaya detection, adhika detection, lunar phase at Purnima |
| `MasaTest` | 6 | 9 known dates with masa + adhika flag, Saka + Vikram year, Amanta month starts (14 cases), Amanta month lengths (12 months + consistency), Purnimanta month starts (12 months), Purnimanta month lengths (12 months) |
| `SolarTest` | 25 | Sankranti precision, month names, era names, Tamil/Bengali/Odia/Malayalam dates, round-trips, 11 Odia boundary cases, solar month start, solar month length |
| `SolarValidationTest` | 8 | 109 drikpanchang.com verified solar dates across all 4 calendars |
| `SolarEdgeTest` | 1 | 400 closest-to-critical-time sankrantis (100 per calendar), boundary edge cases |
| `DrikPanchangValidationTest` | 186 | 186 dates x 4 assertions (tithi, masa, adhika, saka) = 744 checks |
| `AdhikaKshayaTest` | 1 | 4,269 adhika/kshaya tithi edge-case days from CSV (1900-2050) |
| `LunisolarMonthTest` | 6 | Amanta month starts, month lengths, roundtrip, CSV regression, Purnimanta |
| `NycTest` | 2 | US Eastern DST rules + NYC-location dates verified against drikpanchang.com |
| `VariousLocationsTest` | 1 | Multi-location CSV validation (Ujjain, NYC, LA) |
| `FullRegressionTest` | 5 | 55,152-day lunisolar regression (0 failures) + 4 solar calendar regressions (0 failures) |
| **Total** | **257** | |

The 186 validation dates span 1900-2050 and include the hardest edge cases: adhika months, adhika tithis (repeated), kshaya tithis (skipped), new year boundaries, and Amavasya/Purnima days.

The full regression tests read C-generated reference CSVs (`validation/moshier/ref_1900_2050.csv` and `validation/moshier/solar/*.csv`). They verify that Java produces identical tithi, masa, adhika, saka, and solar date values for every date from 1900 to 2050.

## Cross-Validation

Java CLI output is identical to C CLI output:

```
# Single day — identical
$ ./gradlew run --args="-y 2025 -m 1 -d 18" --quiet
Date:       2025-01-18 (Saturday)
Sunrise:    07:15:35 IST
Tithi:      Krishna Panchami (K-5)
Hindu Date: Pausha Krishna 5, Saka 1946 (Vikram 2081)

$ ./hindu-calendar -y 2025 -m 1 -d 18
Date:       2025-01-18 (Saturday)
Sunrise:    07:15:35 IST
Tithi:      Krishna Panchami (K-5)
Hindu Date: Pausha Krishna 5, Saka 1946 (Vikram 2081)
```

Full month output (31 lines for March 2025) also matches line-for-line, including sunrise times to the second.

## Build Notes

- **Java version**: Targets Java 18 (build.gradle.kts uses `JavaLanguageVersion.of(18)`). The code itself is compatible with Java 16+ (uses records, which require Java 16).
- **Dependencies**: Zero runtime dependencies. JUnit 5.9.3 for testing only.
- **Gradle**: 8.10, Kotlin DSL. Wrapper included (`gradlew`).
- **No Swiss Ephemeris**: The Java port is Moshier-only. There is no `USE_SWISSEPH` equivalent.
