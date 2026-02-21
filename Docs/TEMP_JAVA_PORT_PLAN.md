# Plan: Java 21 Port of Hindu Calendar

## Context

Port the entire Hindu calendar C project to Java 21. The C project has a self-contained Moshier ephemeris library (1,943 lines) and calendar logic (~1,550 lines) that produces 100% match against drikpanchang.com across 55,152 dates. The Java port targets Moshier-only (no Swiss Ephemeris), with both a reusable library and a CLI.

**Decisions:** Java 21 LTS, Gradle (Kotlin DSL), Moshier-only, Library + CLI, JUnit 5, zero external runtime dependencies.

## Project Structure

```
java/
  build.gradle.kts
  settings.gradle.kts
  gradlew / gradlew.bat / gradle/wrapper/
  src/
    main/java/com/hindu/calendar/
      model/          # Records + enums (Location, Paksha, MasaName, TithiInfo, etc.)
      ephemeris/      # Moshier library (package-private internals + public Ephemeris facade)
      core/           # Calendar logic (Tithi, Masa, Panchang, Solar, DateUtils)
      cli/            # HinduCalendarCli (main method)
    test/java/com/hindu/calendar/
      ephemeris/      # Moshier unit tests
      core/           # Calendar logic tests
      validation/     # Ported drikpanchang + CSV regression tests
    test/resources/validation/
      ref_1900_2050.csv
      adhika_kshaya_tithis.csv
      solar/*.csv
```

## Class Mapping (C → Java)

### model/ — Data Types (C structs → Java records, C enums → Java enums)

| Java Class | C Source | Lines |
|---|---|---|
| `Location.java` (record, includes `NEW_DELHI` constant) | `types.h` Location | ~25 |
| `Paksha.java` (enum: SHUKLA, KRISHNA) | `types.h` Paksha | ~10 |
| `MasaName.java` (enum with number + displayName + `fromNumber()`) | `types.h` MasaName + MASA_NAMES[] | ~40 |
| `TithiInfo.java` (record, includes `TITHI_NAMES[]`) | `types.h` TithiInfo | ~25 |
| `MasaInfo.java` (record) | `types.h` MasaInfo | ~15 |
| `HinduDate.java` (record) | `types.h` HinduDate | ~15 |
| `PanchangDay.java` (record) | `types.h` PanchangDay | ~15 |
| `SolarCalendarType.java` (enum with config: firstRashi, offsets, eraName, months[]) | `types.h` + `solar.c` configs | ~80 |
| `SolarDate.java` (record) | `types.h` SolarDate | ~15 |

### ephemeris/ — Moshier Ephemeris (package-private internals + public facade)

| Java Class | C Source | Lines |
|---|---|---|
| `MoshierJulianDay.java` (pkg-private) | `moshier_jd.c` (56) | ~70 |
| `MoshierSun.java` (pkg-private) — VSOP87 data tables, nutation, delta-T | `moshier_sun.c` (726) | ~800 |
| `MoshierMoon.java` (pkg-private) — DE404 pipeline, z/LR/LRT tables | `moshier_moon.c` (760) | ~850 |
| `MoshierAyanamsa.java` (pkg-private) — IAU 1976 precession | `moshier_ayanamsa.c` (147) | ~160 |
| `MoshierRise.java` (pkg-private) — Sinclair refraction, GAST, iterative | `moshier_rise.c` (176) | ~200 |
| `Ephemeris.java` (**public** facade) — wires all Moshier classes | `astro.c` + `date_utils.c` | ~90 |

`Ephemeris.java` public API:
- `gregorianToJd(y, m, d)`, `jdToGregorian(jd)`, `dayOfWeek(jd)`
- `solarLongitude(jdUt)`, `lunarLongitude(jdUt)`, `solarLongitudeSidereal(jdUt)`, `getAyanamsa(jdUt)`
- `sunriseJd(jdUt, Location)`, `sunsetJd(jdUt, Location)`

Thread-safety: NOT thread-safe (matches C). Moshier pipeline uses mutable instance fields (ss/cc tables, moon state variables). Single-threaded use, same as C.

### core/ — Calendar Logic

| Java Class | C Source | Lines |
|---|---|---|
| `DateUtils.java` — day-of-week names, daysInMonth | `date_utils.c` (partial) | ~40 |
| `Tithi.java` — lunar phase, tithi at moment/sunrise, boundary finding | `tithi.c` (96) | ~120 |
| `Masa.java` — new moon finding (Lagrange), rashi, masa, year | `masa.c` (150) | ~180 |
| `Panchang.java` — gregorian→hindu, month generation, formatting | `panchang.c` (164) | ~200 |
| `Solar.java` — sankranti, 4 regional critical-time rules, conversion | `solar.c` (448) | ~520 |

Dependencies: `Panchang` → `Tithi`, `Masa`. `Solar` → `Tithi` (Bengali rule). All → `Ephemeris`.

### cli/ — CLI

| Java Class | C Source | Lines |
|---|---|---|
| `HinduCalendarCli.java` — argument parsing, output | `main.c` (191) | ~260 |

Same flags as C: `-y YEAR -m MONTH -d DAY -s tamil|bengali|odia|malayalam -l LAT,LON -u OFFSET -h`

**Totals: ~3,730 production lines, ~3,140 test lines.**

## Implementation Steps

### Step 1: Gradle Project Setup
- `gradle init` or manual wrapper generation
- `build.gradle.kts`: Java 21 toolchain, JUnit 5, application plugin (mainClass), no runtime deps
- `settings.gradle.kts`: `rootProject.name = "hindu-calendar"`
- Create all package directories

### Step 2: Data Types (model/)
- All 9 records/enums listed above
- No tests needed (trivially correct)

### Step 3: Moshier Julian Day
- Port `moshier_jd.c` → `MoshierJulianDay.java`
- Test: J2000 epoch, Unix epoch, round-trips, day-of-week

### Step 4: Moshier Solar
- Port `moshier_sun.c` → `MoshierSun.java`
- Largest file: VSOP87 data tables (`eartabl[460]` as `double[]`, `earargs[819]` as `byte[]`), delta-T table, nutation series
- **Key porting trap**: C pointer arithmetic in VSOP87 loop (`*p++`, `*pl++`) → Java index variables (`pIdx++`, `plIdx++`)
- Test: solar longitude, delta-T, nutation at specific JDs, cross-validate vs C output

### Step 5: Moshier Moon
- Port `moshier_moon.c` → `MoshierMoon.java`
- Data tables: `z[26]` double, `LR[944]`/`LRT[304]`/`LRT2[150]` short
- ~25 mutable instance fields (T, T2, SWELP, MP, D, NF, moonpol0, etc.)
- `MoshierMoon` takes `MoshierSun` reference (for deltaT, nutation)
- **Key porting trap**: `chewm()` pointer walking → index-based. `sscc()` is separate from sun's version (different array sizes)
- Test: lunar longitude at 20 specific JDs, cross-validate vs C output

### Step 6: Moshier Ayanamsa
- Port `moshier_ayanamsa.c` → `MoshierAyanamsa.java`
- IAU 1976 precession matrices, ecliptic rotation
- Takes `MoshierSun` reference (for deltaT, nutation, obliquity)
- Test: ayanamsa at known epochs

### Step 7: Moshier Sunrise/Sunset
- Port `moshier_rise.c` → `MoshierRise.java`
- Sinclair refraction, GAST, Meeus Ch.15 iterative, midnight UT wrap-around
- Takes `MoshierSun` reference (for RA, declination, nutation, obliquity)
- Test: Delhi sunrise/sunset at known dates

### Step 8: Ephemeris Facade
- Create `Ephemeris.java` wiring all Moshier classes together
- Integration test: full pipeline through public API

### Step 9: DateUtils + Tithi
- Port `DateUtils.java` (day names, daysInMonth)
- Port `Tithi.java` (lunar phase, bisection, tithi at sunrise)
- Test: port assertions from `test_tithi.c`

### Step 10: Masa
- Port `Masa.java` (inverse Lagrange interpolation, new moon finding, rashi, masa, year)
- Test: port assertions from `test_masa.c`

### Step 11: Panchang
- Port `Panchang.java` (gregorian→hindu, month generation, formatting)
- Test: single-day conversion, month generation

### Step 12: Solar
- Port `Solar.java` (sankranti finding, 4 regional critical-time rules, conversion)
- Test: port key assertions from `test_solar.c`

### Step 13: CLI
- Port `HinduCalendarCli.java` (argument parsing, output)
- Test: manual comparison of Java vs C CLI output

### Step 14: Validation Tests
- Copy CSVs from `validation/moshier/` to `src/test/resources/validation/`
- Port `DrikPanchangValidationTest.java` — 186 dates x 4 checks (hardcoded)
- Port `CsvRegressionTest.java` — sampled CSV (JUnit `@CsvFileSource` or manual reader)
- Port `AdhikaKshayaTest.java` — 4,269 edge-case days
- Port `SolarValidationTest.java`, `SolarRegressionTest.java`, `SolarEdgeTest.java`

## Key Porting Notes

- **Numerical equivalence**: Java `double` = C `double` (IEEE 754). `Math.floor/sin/cos/atan2/asin/acos` match C `<math.h>`. Java `%` on doubles = C `fmod`.
- **C `signed char` → Java `byte`**: Both signed 8-bit, sign-extends to int identically.
- **C `short` → Java `short`**: Both signed 16-bit, maps directly.
- **C pointers → Java indices**: Every `*p++` becomes `array[idx++]`.
- **C output params → Java return values**: `jd_to_gregorian(jd, &y, &m, &d)` → `int[] jdToGregorian(jd)`.
- **C `extern` declarations → Java constructor injection**: Moon/Ayanamsa/Rise receive MoshierSun reference.
- **C `#ifdef USE_SWISSEPH` → eliminated**: Moshier-only, no conditional compilation needed.

## Verification

1. `./gradlew build` — compiles with zero warnings
2. `./gradlew test` — all JUnit 5 tests pass
3. Cross-validate: run Java CLI and C CLI for same dates, diff output
4. Regression: 186 drikpanchang.com dates must pass 100% (744/744 assertions)
5. Full regression: all CSV tests pass (53,143 assertion equivalent)
6. `./gradlew run --args="-y 2025 -m 3"` — matches C `./hindu-calendar -y 2025 -m 3`
7. `./gradlew run --args="-s tamil -y 2025 -m 4"` — matches C solar output
