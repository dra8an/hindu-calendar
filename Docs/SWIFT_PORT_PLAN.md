# Plan: Port Hindu Calendar to Swift

## Context

The user wants the Hindu calendar ported to Swift as an SPM package with both a library target and a CLI executable (same flags as Java/Rust). The project has existing C, Java, and Rust implementations that all produce identical output. The Swift port will follow the same Moshier-only approach.

## Package Structure

```
swift/
  Package.swift                          # SPM manifest: HinduCalendar library + hindu-calendar CLI
  Sources/
    HinduCalendar/                       # Library target
      Model/
        Location.swift                   # Location struct + NEW_DELHI default
        Paksha.swift                     # Paksha enum (shukla/krishna)
        MasaName.swift                   # MasaName enum (12 months + display names)
        TithiInfo.swift                  # TithiInfo struct
        MasaInfo.swift                   # MasaInfo struct
        HinduDate.swift                  # HinduDate struct
        PanchangDay.swift                # PanchangDay struct
        SolarCalendarType.swift          # SolarCalendarType enum (tamil/bengali/odia/malayalam)
        SolarDate.swift                  # SolarDate struct
        LunisolarScheme.swift            # LunisolarScheme enum (amanta/purnimanta)
      Ephemeris/
        JulianDay.swift                  # JD <-> Gregorian, day of week
        Sun.swift                        # VSOP87 solar longitude, nutation, delta-T (~676 lines)
        Moon.swift                       # DE404 lunar longitude (~662 lines)
        Ayanamsa.swift                   # IAU 1976 precession, Lahiri ayanamsa
        Rise.swift                       # Upper limb sunrise/sunset, Sinclair refraction, GAST
        Ephemeris.swift                  # Facade class (mutable state via Sun/Moon instances)
      Core/
        Tithi.swift                      # Lunar phase, tithi at sunrise, boundary finding
        Masa.swift                       # New moon, full moon (Lagrange), rashi, masa, year, month APIs
        Panchang.swift                   # Gregorian->Hindu, month generation, formatting
        Solar.swift                      # Sankranti, 4 regional critical-time rules, Bengali tuning, month APIs
    HinduCalendarCLI/
      main.swift                         # CLI entry point (same flags as Java/Rust)
  Tests/
    HinduCalendarTests/
      EphemerisTests.swift               # JD, day of week, roundtrip
      TithiTests.swift                   # Tithi calculation
      MasaTests.swift                    # Masa, month start/length (Amanta + Purnimanta)
      SolarTests.swift                   # Solar calendar, Odia Amli era, month APIs
      ValidationTests.swift              # 186 drikpanchang.com dates
      FullRegressionTests.swift          # 55,152 lunisolar + 4 solar CSVs
```

## Key Porting Decisions

| Aspect | Swift Approach | Source |
|--------|---------------|--------|
| Mutable state | `Ephemeris` is a `class` owning `Sun` and `Moon` (also classes). All other types are structs/enums | Rust `&mut self` pattern |
| Data tables | Static `let` arrays (VSOP87 coefficients, DE404 tables, delta-T) | Rust `const` statics |
| Floating point | `Double` (IEEE 754 f64, identical to C/Java/Rust) | All ports |
| Pointer arithmetic | Index variables (`var idx = 0; ... idx += 1`) | Rust pattern |
| Output params | Return tuples `-> (Int, Int, Int)` | Rust pattern |
| Optional | `Double?` for sunrise/sunset (polar failure) | Rust `Option<f64>` |
| Error handling | No throws — same as Rust (infallible except sunrise) | Rust pattern |
| CLI argument parsing | Manual parsing (no ArgumentParser dep — zero deps) | Java/Rust manual parsing |
| CSV test data | `Bundle.module` resource or `#filePath`-relative path | Rust `include_str!` equivalent |
| Thread safety | NOT thread-safe (mutable state in Ephemeris) | All ports |

## Critical Files to Port From (Java sources — closest to Swift idioms)

| Java Source | Swift Target | Lines |
|-------------|-------------|-------|
| `model/Location.java` | `Model/Location.swift` | ~20 |
| `model/Paksha.java` | `Model/Paksha.swift` | ~10 |
| `model/MasaName.java` | `Model/MasaName.swift` | ~50 |
| `model/TithiInfo.java` + constants | `Model/TithiInfo.swift` | ~50 |
| `model/MasaInfo.java` | `Model/MasaInfo.swift` | ~15 |
| `model/HinduDate.java` | `Model/HinduDate.swift` | ~15 |
| `model/PanchangDay.java` | `Model/PanchangDay.swift` | ~15 |
| `model/SolarCalendarType.java` | `Model/SolarCalendarType.swift` | ~80 |
| `model/SolarDate.java` | `Model/SolarDate.swift` | ~15 |
| `model/LunisolarScheme.java` | `Model/LunisolarScheme.swift` | ~5 |
| `ephemeris/MoshierJulianDay.java` | `Ephemeris/JulianDay.swift` | ~80 |
| `ephemeris/MoshierSun.java` | `Ephemeris/Sun.swift` | ~680 |
| `ephemeris/MoshierMoon.java` | `Ephemeris/Moon.swift` | ~660 |
| `ephemeris/MoshierAyanamsa.java` | `Ephemeris/Ayanamsa.swift` | ~100 |
| `ephemeris/MoshierRise.java` | `Ephemeris/Rise.swift` | ~140 |
| `ephemeris/Ephemeris.java` | `Ephemeris/Ephemeris.swift` | ~90 |
| `core/Tithi.java` | `Core/Tithi.swift` | ~90 |
| `core/Masa.java` | `Core/Masa.swift` | ~280 |
| `core/Panchang.java` | `Core/Panchang.swift` | ~170 |
| `core/Solar.java` | `Core/Solar.swift` | ~420 |
| `cli/HinduCalendarCli.java` | `HinduCalendarCLI/main.swift` | ~220 |

**Estimated total: ~3,200 production lines + ~600 test lines**

## Implementation Steps

### Step 1: Package.swift + Model types
- Create `swift/Package.swift` with `HinduCalendar` library and `hindu-calendar` executable targets
- Port all 10 model files (structs and enums)
- Verify: `swift build` compiles

### Step 2: JulianDay.swift
- Port Meeus Ch.7 JD↔Gregorian and day-of-week
- Add basic test: J2000.0 epoch, roundtrip, day of week

### Step 3: Sun.swift (VSOP87 + nutation + delta-T)
- Port all data tables (earth_coeffs, earth_args, frequencies, phases)
- Port VSOP87 pipeline, nutation, obliquity, delta-T
- Port RA/Dec helper functions
- Verify: solar longitude at known JD matches Java/Rust

### Step 4: Moon.swift (DE404)
- Port all lunar data tables (moon_lr, moon_lr_t1, moon_lr_t2, etc.)
- Port full pipeline: mean_elements → moon1-4, accum_series, sscc
- Verify: lunar longitude at known JD matches

### Step 5: Ayanamsa.swift + Rise.swift
- Port IAU 1976 precession (Lahiri ayanamsa)
- Port Sinclair refraction, GAST, upper limb sunrise/sunset
- Verify: ayanamsa at known JD, sunrise for Delhi

### Step 6: Ephemeris.swift (facade)
- Wire Sun, Moon, Ayanamsa, Rise, JulianDay into facade class
- Public API: `solarLongitude()`, `lunarLongitude()`, `solarLongitudeSidereal()`, `getAyanamsa()`, `sunriseJd()`, `sunsetJd()`, `gregorianToJd()`, `jdToGregorian()`, `dayOfWeek()`

### Step 7: Tithi.swift
- Port tithi calculation, boundary finding (bisection)
- Port tithi-at-sunrise
- Add tithi unit tests

### Step 8: Masa.swift
- Port new moon finding (9-point Lagrange), `fullMoonNear()`
- Port masa determination, Saka/Vikram year
- Port `lunisolarMonthStart()` / `lunisolarMonthLength()` (Amanta + Purnimanta)
- Add masa + month start/length tests

### Step 9: Panchang.swift
- Port `gregorianToHindu()`, month panchang generation
- Port formatting (single-day and full-month views)

### Step 10: Solar.swift
- Port sankranti finding (bisection on sidereal longitude)
- Port 4 critical time rules (Tamil, Bengali, Odia, Malayalam)
- Port Bengali per-rashi tuning
- Port solar year/era calculation (including Odia Amli, yearStartRashi)
- Port `solarMonthStart()` / `solarMonthLength()`
- Add solar tests (including Odia Amli era)

### Step 11: CLI (main.swift)
- Port argument parsing (same flags: -y, -m, -d, -s, -l, -u, -h)
- Port output formatting (identical to C/Java/Rust)
- Verify: `swift run hindu-calendar -y 2025 -m 1 -d 18` matches other ports

### Step 12: Validation + Regression tests
- Port 186 drikpanchang.com validation dates
- Port 55,152-day lunisolar CSV regression
- Port 4 solar calendar CSV regressions
- CSV loading: copy CSVs to test resources, read via `#filePath`-relative paths

### Step 13: Cross-validation
- Compare CLI output against C/Java/Rust for 5+ months and single-day views
- Verify all tests pass: `swift test`

## Verification

- `cd swift && swift build` — compiles without errors
- `cd swift && swift test` — all tests pass (target: ~240 tests matching Java)
- CLI spot-checks identical to C:
  - `swift run hindu-calendar -y 2025 -m 1 -d 18` → Pausha Krishna 5, Saka 1946
  - `swift run hindu-calendar -s tamil -y 2025 -m 4 -d 14` → Chithirai 1, 1947
  - `swift run hindu-calendar -s odia -y 2026 -m 3` → Amli era year
