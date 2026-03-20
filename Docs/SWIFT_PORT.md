# Swift Port of Hindu Calendar

Complete port of the Hindu calendar C project to Swift. Moshier-only backend (no Swiss Ephemeris). Produces identical output to the C implementation across all tested assertions — 55,152 lunisolar days (1,104 sampled, 0 failures), all 4 solar calendar regressions (7,244 months, 0 failures), and 186 drikpanchang.com validation dates. Includes upper limb sunrise, Odia Amli era, Bengali per-rashi tuning, Purnimanta scheme, and lunisolar/solar month start/length APIs.

## Quick Start

```bash
cd swift

# Build
swift build

# Run tests (62 tests)
swift test

# Single day
swift run hindu-calendar -- -y 2025 -m 1 -d 18

# Full month
swift run hindu-calendar -- -y 2025 -m 3

# Solar calendar
swift run hindu-calendar -- -s tamil -y 2025 -m 4 -d 14
```

## Project Structure

```
swift/
  Package.swift                          # SPM manifest: HinduCalendar library + hindu-calendar CLI
  Sources/
    HinduCalendar/                       # Library target
      Model/                             # Structs + enums (10 files, ~200 lines)
        Location.swift                   # Location struct + .newDelhi static
        Paksha.swift                     # Paksha enum (shukla/krishna)
        MasaName.swift                   # MasaName enum (12 months + display names)
        TithiInfo.swift                  # TithiInfo struct + static tithiNames
        MasaInfo.swift                   # MasaInfo struct
        HinduDate.swift                  # HinduDate struct
        PanchangDay.swift                # PanchangDay struct
        SolarCalendarType.swift          # SolarCalendarType enum (tamil/bengali/odia/malayalam)
        SolarDate.swift                  # SolarDate struct
        LunisolarScheme.swift            # LunisolarScheme enum (amanta/purnimanta)
      Ephemeris/                         # Moshier library (6 files, ~1,550 lines)
        JulianDay.swift                  # JD <-> Gregorian, day of week (58 lines)
        Sun.swift                        # VSOP87 solar longitude, nutation, delta-T (620 lines)
        Moon.swift                       # DE404 lunar longitude (644 lines)
        Ayanamsa.swift                   # IAU 1976 precession, Lahiri ayanamsa (91 lines)
        Rise.swift                       # Sinclair refraction, GAST, upper limb sunrise/sunset (128 lines)
        Ephemeris.swift                  # Facade class (68 lines)
      Core/                              # Calendar logic (5 files, ~850 lines)
        Tithi.swift                      # Lunar phase, tithi at sunrise, boundary finding (81 lines)
        Masa.swift                       # New moon, full moon (Lagrange), rashi, masa, year, month APIs (293 lines)
        Panchang.swift                   # Gregorian->Hindu, month generation, formatting (144 lines)
        Solar.swift                      # Sankranti, 4 regional critical-time rules, Bengali tuning, month APIs (296 lines)
        DateUtils.swift                  # Days in month, day-of-week names (34 lines)
    HinduCalendarCLI/
      main.swift                         # CLI entry point (158 lines) — same flags as Java/Rust
  Tests/
    HinduCalendarTests/
      EphemerisTests.swift               # JD, solar/lunar longitude, ayanamsa, sunrise/sunset (110 lines)
      TithiTests.swift                   # Tithi calculation, kshaya, adhika (48 lines)
      MasaTests.swift                    # Masa, month start/length — Amanta + Purnimanta (114 lines)
      SolarTests.swift                   # Solar calendar, Odia Amli era, month APIs (134 lines)
      SolarValidationTests.swift         # 109 drikpanchang.com solar dates (169 lines)
      SolarEdgeTests.swift               # 400 boundary sankranti edge cases (440 lines)
      ValidationTests.swift              # 186 drikpanchang.com dates (226 lines)
      AdhikaKshayaTests.swift            # 4,269 adhika/kshaya edge-case days (76 lines)
      LunisolarMonthTests.swift          # Month starts, lengths, roundtrip, CSV regression (255 lines)
      NycTests.swift                     # US Eastern DST + NYC validation (253 lines)
      VariousLocationsTests.swift        # Multi-location validation (112 lines)
      FullRegressionTests.swift          # 1,104 lunisolar days + 4 solar CSVs (165 lines)

Total: ~2,875 production lines, ~2,100 test lines
```

## Type Mapping (C to Swift)

### Model/ — Data Types

| Swift | C Source | Notes |
|-------|----------|-------|
| `Location` (struct) + `.newDelhi` static | `types.h` Location struct | Includes `utcOffset` |
| `Paksha` (enum, Int raw) | `types.h` Paksha enum | `.shukla` (0), `.krishna` (1) |
| `MasaName` (enum, Int raw, CaseIterable) | `types.h` MasaName + MASA_NAMES[] | `displayName`, `fromNumber()` |
| `TithiInfo` (struct) | `types.h` TithiInfo struct | Static `tithiNames` array |
| `MasaInfo` (struct) | `types.h` MasaInfo struct | |
| `HinduDate` (struct) | `types.h` HinduDate struct | |
| `PanchangDay` (struct) | `types.h` PanchangDay struct | |
| `SolarCalendarType` (enum, CaseIterable) | `types.h` + `solar.c` configs | `firstRashi`, `yearStartRashi`, offsets, `eraName`, `monthName()` |
| `SolarDate` (struct) | `types.h` SolarDate struct | |
| `LunisolarScheme` (enum) | `types.h` LunisolarScheme | `.amanta`, `.purnimanta` |

### Ephemeris/ — Moshier Ephemeris

| Swift | C Source | Lines |
|-------|----------|-------|
| `JulianDay` (enum, namespace) | `moshier_jd.c` (56) | 58 |
| `Sun` (class, mutable state) | `moshier_sun.c` (726) | 620 |
| `Moon` (class, mutable state) | `moshier_moon.c` (760) | 644 |
| `Ayanamsa` (class) | `moshier_ayanamsa.c` (147) | 91 |
| `Rise` (class) | `moshier_rise.c` (176) | 128 | Upper limb (solar semi-diameter) |
| `Ephemeris` (**public** facade class) | `astro.c` + `date_utils.c` | 68 |

`Ephemeris` public API:
- `gregorianToJd(year:month:day:)`, `jdToGregorian(_:)` → `(year: Int, month: Int, day: Int)`, `dayOfWeek(_:)`
- `solarLongitude(_:)`, `lunarLongitude(_:)`, `solarLongitudeSidereal(_:)`, `getAyanamsa(_:)`
- `sunriseJd(_:_:)`, `sunsetJd(_:_:)`

### Core/ — Calendar Logic

| Swift | C Source | Lines |
|-------|----------|-------|
| `DateUtils` (enum) | `date_utils.c` (partial) | 34 |
| `Tithi` (class) | `tithi.c` (96) | 81 |
| `Masa` (class) | `masa.c` (300+) | 293 | `fullMoonNear`, `lunisolarMonthStart`/`Length`, `Dictionary` cache |
| `Panchang` (class) | `panchang.c` (164) | 144 |
| `Solar` (class) | `solar.c` (448) | 296 | Bengali tuning, `solarMonthStart`/`Length`, `yearStartRashi` |

### CLI

| Swift | C Source | Lines |
|-------|----------|-------|
| `main.swift` | `main.c` (191) | 158 |

## Key Porting Decisions

### Numerical Equivalence

Swift `Double` and C `double` are both IEEE 754 64-bit. `Foundation.floor/sin/cos/atan2/asin/acos` match C `<math.h>`. Swift `truncatingRemainder(dividingBy:)` behaves like C `fmod`. The port produces bit-identical results for all tested dates.

### C Types to Swift

| C Type | Swift Type | Notes |
|--------|-----------|-------|
| `double` | `Double` | Identical (IEEE 754) |
| `signed char` | `Int` (from `Int8` literal) | EARARGS table uses `[Int]` for clean indexing |
| `short` | `Int16` | LR/LRT/LRT2 tables map directly |
| `int` | `Int` | Swift's default integer type |

### Pointer Arithmetic to Index Variables

Every C `*p++` pattern becomes `array[idx]; idx += 1` in Swift:

```c
// C (moshier_sun.c)
const signed char *pl = earargs;
double su = *p++;
for (int k = 0; k < 9; k++) { cv += ss[k][*pl++]; sv += cc[k][*pl++]; }
```

```swift
// Swift (Sun.swift)
var plIdx = argsOfs
let su = Sun.EARTABL[pIdx]; pIdx += 1
for k in 0..<9 {
    cv += ssTbl[k][Sun.EARARGS[plIdx]]; plIdx += 1
    sv += ccTbl[k][Sun.EARARGS[plIdx]]; plIdx += 1
}
```

### Output Parameters to Tuples

C functions with output pointers return Swift named tuples:

```c
// C
void jd_to_gregorian(double jd, int *y, int *m, int *d);
```

```swift
// Swift
func jdToGregorian(_ jd: Double) -> (year: Int, month: Int, day: Int)
```

### Value Types vs Reference Types

Swift structs (value types) for all model types. Classes (reference types) only for Ephemeris and its internal computation objects (Sun, Moon, Ayanamsa, Rise), which have mutable state that must be shared by reference. `JulianDay` and `DateUtils` are caseless enums (namespaces for static functions).

### Constructor Injection

`Ephemeris` owns `Sun`, `Moon`, `Ayanamsa`, and `Rise` instances. `Tithi`, `Masa`, `Panchang`, and `Solar` take an `Ephemeris` reference in their initializers.

### Thread Safety

NOT thread-safe, matching the C implementation. Moshier pipeline uses mutable instance properties (sin/cos tables, moon state variables). Each `Ephemeris` instance should be used from a single thread.

### No External Dependencies

Zero runtime or test dependencies. Uses only Swift standard library and Foundation. XCTest for testing (built into Swift toolchain). SPM (Swift Package Manager) for build.

## Key Porting Traps Encountered

1. **VSOP87 `sscc` loop guard**: The `sscc(k, arg, n)` function computes Chebyshev sin/cos multiples with a `for i in 2..<n` loop. When `n=1` (from `EAR_MAX_HARMONIC` values like `[1, 9, 14, 17, 5, 5, 2, 1, 0]`), Swift's `2..<1` crashes with "Range requires lowerBound <= upperBound". Fixed by wrapping in `if n > 2 { ... }`.

2. **VSOP87 pointer walking**: The nested loop in the VSOP87 summation walks two arrays (`EARTABL` for coefficients, `EARARGS` for argument indices) with different strides. Getting the index arithmetic wrong produces subtly wrong solar longitudes.

3. **DE404 mutable state**: The DE404 pipeline has ~20 mutable fields that persist across calls. These are stored as class properties in `Moon`, not local variables, because intermediate results (mean elements, planetary perturbations) are shared between `moon1()`...`moon4()`.

4. **Ayanamsa nutation**: `swe_get_ayanamsa_ut()` returns MEAN ayanamsa (without nutation). Our ayanamsa function must NOT add nutation. Nutation cancels in sidereal position: `sid = (trop + dpsi) - (ayan + dpsi) = trop - ayan`.

5. **JD noon convention**: Julian Day numbers are noon-based. When converting to local time, add 0.5 to get midnight-based, then add the UTC offset.

## Test Suite

**62 tests, 0 failures.**

| Test Class | Tests | What It Covers |
|------------|-------|----------------|
| `EphemerisTests` | 12 | JD epoch values, round-trip, day-of-week, solar/lunar longitude ranges, ayanamsa (range + reference epoch), Delhi sunrise/sunset, sidereal solar longitude |
| `TithiTests` | 4 | 7 known dates with tithi + paksha, kshaya detection, adhika detection, lunar phase at Purnima |
| `MasaTests` | 6 | 9 known dates with masa + adhika flag, Saka + Vikram year, Amanta month starts (14 cases), Amanta month lengths (12 months + consistency), Purnimanta month starts (12 months), Purnimanta month lengths (12 months) |
| `SolarTests` | 10 | Sankranti precision, month names, era names, Tamil (10 cases), Bengali (9 cases), Odia (7 + 11 boundary cases), Malayalam (7 cases), solar month start, solar month length |
| `SolarValidationTests` | 8 | 109 drikpanchang.com verified solar dates across all 4 calendars (Tamil, Bengali, Odia, Malayalam) |
| `SolarEdgeTests` | 4 | 400 closest-to-critical-time sankrantis (100 per calendar), boundary edge cases |
| `ValidationTests` | 3 | 186 dates × 4 assertions (tithi, masa, adhika, saka) = 744 checks. 54 spot-checked + 52 adhika tithi + 80 kshaya tithi |
| `AdhikaKshayaTests` | 1 | 4,269 adhika/kshaya tithi edge-case days from CSV (1900-2050) |
| `LunisolarMonthTests` | 6 | Amanta month starts (spot checks), month lengths, roundtrip, CSV regression (1,868 months), Purnimanta spot checks + lengths |
| `NycTests` | 2 | US Eastern DST rules + 18 NYC-location dates verified against drikpanchang.com |
| `VariousLocationsTests` | 1 | Multi-location CSV validation (Ujjain, NYC, LA — 465 assertions) |
| `FullRegressionTests` | 5 | 1,104 sampled lunisolar days (every 50th from 55,152) × 4 checks + 4 solar calendar regressions (1,811 months each × 4 checks) |
| **Total** | **62** | |

The 186 validation dates span 1900-2050 and include the hardest edge cases: adhika months, adhika tithis (repeated), kshaya tithis (skipped), new year boundaries, and Amavasya/Purnima days.

The full regression tests read C-generated reference CSVs (`validation/moshier/ref_1900_2050.csv` and `validation/moshier/solar/*.csv`) via `#filePath`-relative path resolution.

## Cross-Validation

Swift CLI output is identical to C CLI output:

```
# Single day — identical
$ swift run hindu-calendar -- -y 2025 -m 1 -d 18
Date:       2025-01-18 (Saturday)
Sunrise:    07:15:35 IST
Tithi:      Krishna Panchami (K-5)
Hindu Date: Pausha Krishna 5, Saka 1946 (Vikram 2081)

$ ./hindu-calendar -y 2025 -m 1 -d 18
Date:       2025-01-18 (Saturday)
Sunrise:    07:15:35 IST
Tithi:      Krishna Panchami (K-5)
Hindu Date: Pausha Krishna 5, Saka 1946 (Vikram 2081)

# Tamil solar — identical
$ swift run hindu-calendar -- -s tamil -y 2025 -m 4 -d 14
Tamil Solar Date: Chithirai 1, 1947 (Saka)

# Odia solar — identical
$ swift run hindu-calendar -- -s odia -y 2025 -m 9 -d 17
Odia Solar Date: Ashvina 1, 1433 (Amli)
```

## Build Notes

- **Swift version**: Requires Swift 5.9+ (swift-tools-version: 5.9). Builds on macOS and Linux.
- **Dependencies**: Zero runtime dependencies. XCTest for testing only (built into Swift toolchain).
- **Build system**: Swift Package Manager (SPM). No Xcode project file needed.
- **No Swiss Ephemeris**: The Swift port is Moshier-only. There is no `USE_SWISSEPH` equivalent.
- **Performance**: `swift test` runs all 62 tests in ~19 minutes (regression and validation tests dominate; unit tests complete in ~24s).

## Comparison with Other Ports

| | C (original) | Java | Rust | Swift |
|---|---|---|---|---|
| Production lines | ~4,900 | ~3,000 | ~3,100 | ~2,875 |
| Test lines | ~3,190 | ~2,600 | ~1,930 | ~2,100 |
| Tests | 15 suites | 257 tests | 27 tests | 62 tests |
| External deps | 0 | JUnit 5 (test only) | 0 | 0 |
| Build tool | Make | Gradle | Cargo | SPM |
| Backend | Moshier + SE | Moshier only | Moshier only | Moshier only |
| Upper limb sunrise | Yes | Yes | Yes | Yes |
| Purnimanta scheme | Yes | Yes | Yes | Yes |
| Thread-safe | No | No | No | No |
