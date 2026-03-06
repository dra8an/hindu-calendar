# Future: Rename moshier_ prefix

## Why

After removing all Moshier-original variable/function names and SE provenance
references, the `moshier_` prefix on file names and public functions is the
largest remaining tie to the original author. The code should be named for
what it *does* (ephemeris computation), not where it came *from*.

## Scope

### Directory

`lib/moshier/` → `lib/ephemeris/`

### Files

| Current | New |
|---------|-----|
| `moshier_sun.c` | `ephem_sun.c` |
| `moshier_moon.c` | `ephem_moon.c` |
| `moshier_jd.c` | `ephem_jd.c` |
| `moshier_ayanamsa.c` | `ephem_ayanamsa.c` |
| `moshier_rise.c` | `ephem_rise.c` |

### Public functions (grep for `moshier_` across src/)

All `moshier_*()` functions → `ephem_*()`, e.g.:

- `moshier_solar_longitude()` → `ephem_solar_longitude()`
- `moshier_lunar_longitude()` → `ephem_lunar_longitude()`
- `moshier_ayanamsa()` → `ephem_ayanamsa()`
- `moshier_sunrise()` / `moshier_sunset()` → `ephem_sunrise()` / `ephem_sunset()`
- `moshier_julian_day()` → `ephem_julian_day()`
- etc.

### Callers to update

- `src/astro.c` / `src/astro.h` — main consumer of ephemeris functions
- `src/date_utils.c` — JD/Gregorian conversions
- `Makefile` — source paths, build rules
- `tests/` — all test files referencing moshier_ functions
- `#include` paths everywhere

### Also update

- Java port (`java/`)
- Rust port (`rust/`)
- Docs referencing `moshier_` (MEMORY.md, LICENSING.md, RENAMING.md, etc.)
