# Plan: Port SE Moshier Moon Pipeline to Eliminate 29 Lunar Failures

## Context

The self-contained Moshier library (`lib/moshier/moshier_moon.c`, 147 lines) uses 60 Meeus Ch.47 ELP-2000/82 terms for lunar longitude, achieving ~10 arcsecond precision. This causes 29 tithi boundary failures (25 in test_adhika_kshaya + 4 in test_csv_regression) where the tithi transition falls close to sunrise and the ~10" error flips the result.

Swiss Ephemeris achieves ~3-5 arcsecond precision using Moshier's own DE404-fitted lunar theory in `swemmoon.c` (1,930 lines). This is what SE uses internally in Moshier mode — the same code path we used before building our own library.

**Goal**: Port the SE Moshier moon longitude pipeline to `moshier_moon.c`, achieving the same ~3-5" precision and eliminating all 29 failures. We only need longitude (not latitude/radius/speed).

## Why the Current 60-Term Approach Can't Be Improved Incrementally

1. We already tried adding 118 SE terms → **worse** results (179 vs 120 failures) because SE terms require associated corrections (moon1/moon2 perturbations, z[] array) that form a self-consistent system
2. Meeus's 60 terms are self-consistent with E correction + A1/A2/A3 — but topped out at ~10"
3. No published intermediate set (60-120 terms) exists that works standalone

## What SE's Moshier Moon Pipeline Does (longitude only)

The pipeline in `swemmoon.c` (DE404 version, not DE200) computes ecliptic longitude of date:

```
mean_elements()     → D, M, MP, NF, SWELP (arcseconds) + z[] corrections
mean_elements_pl()  → Ve, Ea, Ma, Ju, Sa (planetary mean longitudes)
moon1()             → Venus-Jupiter-Earth-Mars perturbations with T^0-T^3 polynomial
                      LRT2 (25 terms), LRT (38 terms) multiplied by T^2 and T
moon2()             → 24 additional perturbation terms (fitted to DE404)
moon3()             → Main LR table summation (118 terms) + polynomial assembly
moon4()             → Convert arcseconds to radians, normalize
```

### Data required (longitude only):

| Table | Size | Description |
|-------|------|-------------|
| z[] | 26 doubles | DE404 mean element corrections + perturbation T^2 coefficients |
| LR[] | 118×8 shorts | Main longitude/radius terms (we use longitude half = 118×4) |
| LRT[] | 38×8 shorts | T^1 longitude/radius corrections |
| LRT2[] | 25×6 shorts | T^2 longitude corrections |
| moon1 explicit | ~20 terms | Venus-Jupiter perturbations with T^0-T^3 series |
| moon2 explicit | 24 terms | Additional DE404-fitted perturbations |

### Helper functions needed:

| Function | Purpose |
|----------|---------|
| `mods3600()` | Normalize arcseconds modulo 1,296,000 (360°) |
| `sscc()` | Build sin/cos lookup table for multiples of angles |
| `chewm()` | Step through perturbation table and accumulate |
| `mean_elements()` | Compute D, M, MP, NF, SWELP with z[] corrections |
| `mean_elements_pl()` | Compute Ve, Ea, Ma, Ju, Sa |

### What we DON'T need:

- MB[] latitude table (77 terms) — not needed for longitude
- BT[] latitude T^1 table (16 terms)
- BT2[] latitude T^2 table (12 terms)
- Latitude computation in moon1/moon2
- Radius computation
- ecldat_equ2000() equatorial conversion
- Speed computation
- Mean node/apsis corrections
- Interpolated apsides

## Implementation Plan

### Step 1: Rewrite `lib/moshier/moshier_moon.c`

Replace the current 147-line Meeus implementation with a port of SE's `swemmoon.c`, keeping only the longitude pipeline. Key design decisions:

- **Use DE404 version** (the `#else` branch, not `#ifdef MOSH_MOON_200`)
- **All computation in arcseconds** internally (matching SE), convert to degrees at the end
- **Skip latitude/radius/speed** — saves ~200 lines of tables and code
- **Static globals → function-local variables** where possible for cleaner code
- **Keep applying nutation externally** via existing `moshier_nutation_longitude()`

The function signature stays the same: `double moshier_lunar_longitude(double jd_ut)` → returns tropical longitude in degrees [0, 360).

### Step 2: Port data tables

From `swemmoon.c` (DE404 version):
```c
static const double z[26] = { ... };     // Mean element + perturbation corrections
static const short LR[8*118] = { ... };  // Main longitude+radius (use lon half)
static const short LRT[8*38] = { ... };  // T^1 longitude+radius
static const short LRT2[6*25] = { ... }; // T^2 longitude
```

### Step 3: Port helper functions

- `mods3600()` — 5 lines, trivial
- `sscc()` — 15 lines, Chebyshev recurrence for sin/cos multiples
- `chewm()` — 30 lines, table accumulation with type flags (only types 1 and 2 needed for longitude)

### Step 4: Port mean element computation

- `mean_elements()` — Fundamental arguments with z[] corrections
- `mean_elements_pl()` — Planetary mean longitudes (Venus through Saturn)

### Step 5: Port moon1() (longitude terms only)

- LRT2 summation (T^2 terms) via `chewm()`
- LRT summation (T^1 terms) via `chewm()`
- ~20 explicit Venus-Jupiter-Earth-Mars perturbation terms with T^0-T^3 polynomial
- Skip all latitude (moonpol[1]) and radius (moonpol[2]) terms

### Step 6: Port moon2() (longitude terms only)

- 24 additional perturbation terms for longitude (`l += ...`)
- Skip 8 latitude terms at the end

### Step 7: Port moon3() and moon4() (longitude assembly)

- Main LR table summation via `chewm()` (type 1)
- Polynomial assembly: `l = SWELP + l + (((l4*T + l3)*T + l2)*T + l1)*T*1e-5 + 1e-4*moonpol[0]`
- Convert from arcseconds to degrees: `lon = mods3600(result) * STR * (180/PI)`
- Apply nutation (from existing `moshier_nutation_longitude()`)

### Estimated size

- Data tables: ~200 lines (z[], LR[], LRT[], LRT2[])
- Helper functions: ~60 lines (mods3600, sscc, chewm)
- Mean elements: ~80 lines (mean_elements, mean_elements_pl)
- moon1 (longitude only): ~120 lines
- moon2 (longitude only): ~40 lines
- moon3+moon4+wrapper: ~40 lines
- **Total: ~540 lines** (up from 147, but still well under SE's 1,930)

## Files to Modify

| File | Change |
|------|--------|
| `lib/moshier/moshier_moon.c` | Complete rewrite — port SE Moshier moon pipeline (longitude only) |

No other files need changes — the interface (`moshier_lunar_longitude(jd_ut)` → degrees) stays the same.

## Verification

1. `make clean && make && make test` — all 53,143 assertions should pass (0 failures)
2. `make clean && make USE_SWISSEPH=1 && make test USE_SWISSEPH=1` — still 53,143/53,143
3. Spot-check: compare moshier vs SE lunar longitude at key dates (should agree within ~5")
4. Specifically verify the 29 previously-failing dates now pass
