# Plan: Rust Port of Hindu Calendar

## Context

Port the Hindu calendar C project to Rust. Moshier-only backend (no Swiss Ephemeris). The C implementation (lib/moshier/ + src/) is ~3,500 lines. The Java port proved this is pure math — 275,649 days across 5 calendars, 0 differences. Rust `f64` = C `double` = IEEE 754, so identical results are guaranteed.

## Project Structure

```
rust/
  Cargo.toml
  src/
    lib.rs                  # Library crate root — re-exports public API
    model.rs                # Structs + enums (Location, Paksha, MasaName, TithiInfo, etc.)
    ephemeris/
      mod.rs                # Public Ephemeris struct (facade)
      julian_day.rs         # JD ↔ Gregorian, day of week
      sun.rs                # VSOP87 solar longitude, nutation, delta-T, obliquity
      moon.rs               # DE404 lunar longitude
      ayanamsa.rs           # IAU 1976 precession, Lahiri ayanamsa
      rise.rs               # Sinclair refraction, GAST, iterative sunrise/sunset
    core/
      mod.rs                # Re-exports
      tithi.rs              # Lunar phase, tithi at sunrise, boundary finding
      masa.rs               # New moon (Lagrange), rashi, masa, year
      panchang.rs           # Gregorian→Hindu, month generation, formatting
      solar.rs              # Sankranti, 4 regional critical-time rules, conversion
    bin/
      hindu_calendar.rs     # CLI binary (argument parsing, output)
  tests/
    validation_test.rs      # 186 drikpanchang.com dates (from C test_validation.c)
    full_regression_test.rs # 55,152 lunisolar + 4×55,124 solar days from CSV
```

## Module Mapping (C → Rust)

### model.rs (~120 lines)

| Rust | C Source |
|------|----------|
| `Location` struct + `Location::NEW_DELHI` const | `types.h` Location |
| `Paksha` enum | `types.h` Paksha |
| `MasaName` enum with `number()`, `display_name()` | `types.h` MasaName + MASA_NAMES[] |
| `TithiInfo` struct + `TITHI_NAMES: [&str; 30]` | `types.h` TithiInfo |
| `MasaInfo` struct | `types.h` MasaInfo |
| `HinduDate` struct | `types.h` HinduDate |
| `PanchangDay` struct | `types.h` PanchangDay |
| `SolarCalendarType` enum with config methods | `types.h` + `solar.c` |
| `SolarDate` struct | `types.h` SolarDate |

### ephemeris/ (~1,800 lines)

| Rust | C Source | Est. Lines |
|------|----------|------------|
| `julian_day.rs` | `moshier_jd.c` (56) | ~60 |
| `sun.rs` — VSOP87 tables, nutation, delta-T | `moshier_sun.c` (726) | ~700 |
| `moon.rs` — DE404 tables, pipeline | `moshier_moon.c` (760) | ~750 |
| `ayanamsa.rs` — IAU 1976 precession | `moshier_ayanamsa.c` (147) | ~140 |
| `rise.rs` — Sinclair, GAST, iterative | `moshier_rise.c` (176) | ~170 |
| `mod.rs` — `Ephemeris` pub struct | `astro.c` facade | ~80 |

### core/ (~600 lines)

| Rust | C Source | Est. Lines |
|------|----------|------------|
| `tithi.rs` | `tithi.c` (96) | ~90 |
| `masa.rs` | `masa.c` (150) | ~140 |
| `panchang.rs` | `panchang.c` (164) | ~160 |
| `solar.rs` | `solar.c` (448) | ~400 |

### CLI (~140 lines)

| Rust | C Source |
|------|----------|
| `bin/hindu_calendar.rs` | `main.c` (191) |

**Estimated totals: ~2,700 production lines, ~800 test lines.**

## Implementation Steps

### Step 1: Cargo Project Setup
- `cargo init --lib` in `rust/`
- Add `[[bin]]` section in Cargo.toml for CLI
- Zero external runtime dependencies (only `std`)
- No dev-deps needed — Rust has built-in `#[test]`

### Step 2: Data Types (model.rs)
- All structs with `#[derive(Debug, Clone, Copy)]`
- Enums with `impl` blocks for name/number methods
- `SolarCalendarType` with match-based config (month names, era, offsets)

### Step 3: Moshier Julian Day (julian_day.rs)
- `gregorian_to_jd(y, m, d) -> f64`
- `jd_to_gregorian(jd) -> (i32, i32, i32)`
- `day_of_week(jd) -> i32`
- Unit tests in `#[cfg(test)]` module

### Step 4: Moshier Sun (sun.rs)
- `const EARTABL: [f64; 460]`, `const EARARGS: [i8; 819]`, delta-T table
- Mutable state: `ss_tbl`, `cc_tbl` arrays in `Ephemeris` struct
- VSOP87 pipeline, nutation, obliquity, aberration, RA/Dec
- C `*p++` → `data[idx]; idx += 1;` (same as Java port)

### Step 5: Moshier Moon (moon.rs)
- `const Z: [f64; 25]`, `const LR: [i16; 944]`, `const LRT: [i16; 304]`, `const LRT2: [i16; 150]`
- ~25 mutable fields in struct (T, T2, SWELP, MP, D, NF, moonpol, etc.)
- Borrows `&mut` sun state for delta-T/nutation
- `mean_elements()`, `moon1()`..`moon4()` pipeline

### Step 6: Moshier Ayanamsa (ayanamsa.rs)
- IAU 1976 precession matrices
- Borrows `&` sun state for delta-T/nutation/obliquity

### Step 7: Moshier Sunrise (rise.rs)
- Sinclair refraction, GAST, Meeus Ch.15 iterative
- Borrows `&mut` sun state for RA/Dec
- Returns `Option<f64>` (sunrise can fail for polar regions)

### Step 8: Ephemeris Facade (mod.rs)
- `pub struct Ephemeris` owns all mutable state (sun, moon fields)
- `&mut self` on all methods
- Public API mirrors C/Java: `gregorian_to_jd`, `solar_longitude`, `lunar_longitude`, `sunrise_jd`, etc.

### Step 9: Calendar Logic (core/)
- `tithi.rs`: free functions taking `&mut Ephemeris`
- `masa.rs`: inverse Lagrange, new moon, rashi, masa, year
- `panchang.rs`: gregorian_to_hindu, month generation, formatting
- `solar.rs`: sankranti, 4 regional critical-time rules, conversion

### Step 10: CLI (bin/hindu_calendar.rs)
- Same flags: `-y`, `-m`, `-d`, `-s`, `-l`, `-u`, `-h`
- Manual arg parsing with `std::env::args()` (C version is simple enough)

### Step 11: Tests
- Unit tests: `#[cfg(test)]` in each source file
- Integration tests in `tests/`:
  - `validation_test.rs`: 186 drikpanchang.com dates (hardcoded from C test_validation.c)
  - `full_regression_test.rs`: read C-generated CSVs, check every day 1900-2050

## Key Porting Notes

- **`f64` = C `double`**: IEEE 754 identical. `f64::floor()/sin()/cos()/atan2()` match C.
- **`f64 % f64`**: Rust `%` = C `fmod` (same sign as dividend). Use `.rem_euclid()` only when positive modulo needed.
- **`i8` = C `signed char`**: Both signed 8-bit. EARARGS table maps directly.
- **`i16` = C `short`**: Both signed 16-bit. LR/LRT/LRT2 tables map directly.
- **C pointers → Rust indices**: `*p++` becomes `data[idx]; idx += 1;`
- **C output params → Rust tuples**: `jd_to_gregorian(jd, &y, &m, &d)` → `fn jd_to_gregorian(jd: f64) -> (i32, i32, i32)`
- **Mutability**: `&mut self` on Ephemeris. Not `Send`/`Sync` (mutable shared state).
- **No heap allocation**: All computation is stack-based f64 math. Data tables are `const` statics.
- **Error handling**: `Option<f64>` for sunrise/sunset. Everything else infallible.

## Verification

1. `cargo build` — zero warnings
2. `cargo test` — all tests pass
3. Cross-validate CLI: `cargo run -- -y 2025 -m 3` vs `./hindu-calendar -y 2025 -m 3` — identical
4. 186 drikpanchang.com dates: 744/744 assertions pass
5. Full regression: 275,649 days from C CSVs, 0 failures
6. `cargo run -- -s tamil -y 2025 -m 4` — matches C solar output
