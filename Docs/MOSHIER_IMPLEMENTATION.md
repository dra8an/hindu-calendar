# Moshier Ephemeris Library — Implementation Notes

Self-contained astronomical ephemeris replacing Swiss Ephemeris (SE) for the Hindu calendar project.

## Overview

| Metric | Moshier | Swiss Ephemeris |
|--------|---------|----------------|
| Lines of code | 1,943 | 51,493 |
| External files | None | Optional .se1 files |
| Build time | ~0.5s | ~3s |
| Test pass rate | 53,143 / 53,143 (100%) | 53,143 / 53,143 (100%) |
| drikpanchang.com match | 1,071 / 1,071 (100%) | 1,071 / 1,071 (100%) |
| Reduction factor | **26x fewer lines** | — |

Both backends are selectable at compile time:
- `make` — moshier (default)
- `make USE_SWISSEPH=1` — Swiss Ephemeris

The Moshier library has been ported to [Java](JAVA_PORT.md) and [Rust](RUST_PORT.md), producing identical output.

## Architecture

```
lib/moshier/
├── moshier.h            35 lines   Public API (8 functions)
├── moshier_jd.c         56 lines   JD ↔ Gregorian, day of week
├── moshier_sun.c       739 lines   VSOP87 solar longitude, nutation, delta-T
├── moshier_moon.c      791 lines   Lunar longitude (DE404 Moshier theory)
├── moshier_ayanamsa.c  147 lines   Lahiri ayanamsa (IAU 1976 precession)
└── moshier_rise.c      175 lines   Sunrise / sunset (Sinclair refraction, GAST)
                       ──── ─────
                       1,943 total
```

The library replaces 8 SE functions called from `src/astro.c` and `src/date_utils.c`:

| SE Function | Moshier Replacement | File |
|-------------|-------------------|------|
| `swe_julday()` | `moshier_julday()` | moshier_jd.c |
| `swe_revjul()` | `moshier_revjul()` | moshier_jd.c |
| `swe_day_of_week()` | `moshier_day_of_week()` | moshier_jd.c |
| `swe_calc_ut(SE_SUN)` | `moshier_solar_longitude()` | moshier_sun.c |
| `swe_calc_ut(SE_MOON)` | `moshier_lunar_longitude()` | moshier_moon.c |
| `swe_get_ayanamsa_ut()` | `moshier_ayanamsa()` | moshier_ayanamsa.c |
| `swe_rise_trans(RISE)` | `moshier_sunrise()` | moshier_rise.c |
| `swe_rise_trans(SET)` | `moshier_sunset()` | moshier_rise.c |

## Algorithms Used

### JD ↔ Gregorian (`moshier_jd.c`)

Meeus Ch. 7, standard algorithm. Exact — no floating-point approximation.

Day-of-week formula matches SE: `(((int)floor(jd - 2433282 - 1.5) % 7) + 7) % 7` → 0=Mon..6=Sun.

### Solar Longitude (`moshier_sun.c`)

**VSOP87 pipeline** (ported from SE's `swemplan.c`):
```
JD (UT) → JD (TT) → VSOP87 harmonic summation (135 terms)
       → General precession IAU 1976 → EMB→Earth correction
       → Geocentric flip → Nutation → Aberration (−20.496″)
       → Apparent tropical solar longitude
```
See [VSOP87_IMPLEMENTATION.md](VSOP87_IMPLEMENTATION.md) for the full pipeline description.

**Nutation** (Meeus Ch. 22): 13 terms from IAU 1980 Table 22.A. SE uses the same model.

**Aberration**: Standard constant of aberration (−20.496″). This must be used with VSOP87 — the Meeus p.164 combined formula only works with the simplified 3-term equation of center.

**Delta-T**: SE yearly lookup table (1620–2025) for high-accuracy dates, Meeus polynomial fits for dates outside that range. The lookup table was essential for achieving ±2s sunrise precision — Meeus polynomials diverge up to ~4s from SE for certain historical periods.

### Lunar Longitude (`moshier_moon.c`)

Full DE404-fitted Moshier lunar theory, ported from SE's `swemmoon.c` (longitude pipeline only). This replaces the earlier 60-term Meeus Ch.47 implementation, achieving ~0.07″ RMS precision vs SE (was ~10″).

**Pipeline:**
```
mean_elements()     → D, M, MP, NF, LP + z[] corrections
mean_elements_pl()  → Ve, Ea, Ma, Ju, Sa (planetary mean longitudes)
moon1()             → Venus-Jupiter-Earth-Mars perturbations (moon_lr_t2, moon_lr_t1 tables + explicit terms)
moon2()             → 24 additional DE404-fitted perturbation terms
moon3()             → Main moon_lr table summation (118 terms) + polynomial assembly
moon4()             → Convert arcseconds to radians, normalize
```

**Data tables:**
- `z[26]`: Mean element corrections and perturbation T² coefficients
- `moon_lr[118×8]`: Main longitude/radius terms (longitude half used)
- `moon_lr_t1[38×8]`: T¹ longitude/radius corrections
- `moon_lr_t2[25×6]`: T² longitude corrections
- Variable light-time correction using 5 LR radius terms

**Helper functions:**
- `mods3600()`: Normalize arcseconds modulo 1,296,000 (360°)
- `sscc()`: Chebyshev recurrence for sin/cos multiples of angles
- `accum_series()`: Step through perturbation table and accumulate terms

Nutation in longitude (from `moshier_sun.c`) is added to get apparent longitude.

**Key insight**: Using only the 118 main LR terms without the full pipeline (moon1/moon2 corrections, z[] array) gave WORSE results than 60 Meeus terms (179 vs 120 failures). The full pipeline with ALL corrections is essential for accuracy.

### Lahiri Ayanamsa (`moshier_ayanamsa.c`)

Replicates the Lahiri ayanamsa exactly using the same 3D equatorial precession algorithm as Swiss Ephemeris:

1. Start with vernal point at target date: (1, 0, 0) in equatorial coordinates
2. Precess from target date → J2000 using IAU 1976 angles
3. Precess from J2000 → t₀ (Lahiri reference epoch, JD 2435553.5)
4. Convert to ecliptic of t₀, get polar longitude
5. Ayanamsa = −longitude + 23.245524743°
6. Add nutation in longitude

IAU 1976 precession angles (Lieske et al. 1977):
```
Z     = (2306.2181T + 0.30188T² + 0.017998T³) / 3600°
z     = (2306.2181T + 1.09468T² + 0.018203T³) / 3600°
θ     = (2004.3109T - 0.42665T² - 0.041833T³) / 3600°
```

SE Lahiri constants (from `sweph.h` ayanamsa table, index 1):
- t₀ = JD 2435553.5 (1956 September 22.0 TT)
- ayan_t₀ = 23.250182778 − 0.004658035 = 23.245524743°

### Sunrise / Sunset (`moshier_rise.c`)

Meeus Ch. 15 iterative method with SE-matching refinements:

1. Compute apparent sidereal time at 0h UT (GAST = GMST + equation of equinoxes)
2. Compute solar RA and declination at noon
3. Hour angle: cos(H₀) = (sin(h₀) − sin(φ)sin(δ)) / (cos(φ)cos(δ))
4. Initial estimate: m = m₀ ∓ H₀/360
5. Iterate up to 10 times: recompute solar position at trial time, correct via altitude residual (convergence threshold: 0.0000001 ≈ 0.009s)
6. Midnight UT wrap-around handling for near-midnight sunrises (e.g., Delhi in May)
7. Return JD of event

**Upper limb**: h₀ = −(Sinclair refraction) − (solar semi-diameter) ≈ −0.612° − 0.267° ≈ −0.879°. This matches drikpanchang.com's sunrise definition (top edge of the solar disc at the horizon).

**Sidereal time**: GAST (apparent sidereal time) = GMST + Δψ × cos(ε). SE's `swe_sidtime()` returns GAST; using GMST instead caused ~1″ hour angle error (~4s sunrise offset).

**Midnight UT wrap-around**: Delhi sunrise in May/June falls near 0h UT. The iteration can converge to m values near 1.0 instead of 0.0. Handled by: `if (is_rise && m > 0.75) m -= 1.0`.

Altitude adjustment for observer elevation: h₀ -= 0.0353 × √alt.

## Precision vs Swiss Ephemeris

Statistical comparison across 1900–2050 at representative dates:

| Quantity | Precision vs SE | Method |
|----------|----------------|--------|
| Tropical solar longitude | ±1″ | VSOP87 (135 harmonic terms) |
| Ayanamsa (mean) | ±0.3″ | IAU 1976 3D equatorial precession |
| Sidereal solar longitude | ±0.5″ | Combined solar + ayanamsa |
| Lunar longitude | ±0.07″ (RMS), 0.065″ max | DE404 Moshier theory (full pipeline) |
| Sunrise/sunset | ±2 seconds | Sinclair refraction + GAST |

All differences are well within the requirements for Hindu calendar calculation (tithi boundaries need ~1 arcminute, sankranti boundaries need ~1 minute).

## Zero Test Failures

All 53,143 assertions pass with the Moshier backend. The two dates that previously appeared as "failures" (1965-05-30 and 2001-09-20) were caused by `test_adhika_kshaya.c` being hardcoded to read the Swiss Ephemeris CSV even when testing with Moshier. Once fixed to select the backend-appropriate CSV, both dates pass — and are confirmed correct by drikpanchang.com (SE is wrong on those 2 dates, not Moshier).

## What We Tried and Why

### Lunar: Partial SE Terms (118 from LR table only)

We extracted 118 longitude terms from SE's `swemmoon.c` (the `LR[]` and `LRT[]` tables) without the full pipeline.

**Result: 179 failures (worse than 120)**

**Why**: SE's lunar tables require the full correction pipeline (moon1 Venus-Jupiter perturbations, moon2 DE404 terms, z[] corrections). Using raw terms without these corrections creates an imbalanced model. The eventual solution was porting the entire pipeline — all stages are essential.

### Lunar: Full DE404 Pipeline (current)

Ported the complete SE Moshier moon pipeline (mean_elements → moon1 → moon2 → moon3 → moon4), longitude only.

**Result: 29→2 failures, ±0.07″ precision**

This confirmed that the partial approach was fundamentally flawed — the pipeline is a self-consistent system.

### Nutation: 13 Terms vs Full 63-Term Table

We implemented the full 63-term IAU 1980 nutation table from Meeus Table 22.A.

**Result: 178 failures with 63 terms + clean aberration vs 120 failures with 13 terms + Meeus p.164 formula**

See [Error Cancellation](#error-cancellation-meeus-p164-formula) below.

### Removing Eccentricity Correction

Hypothesized that SE's `accum_series()` function doesn't apply the E correction (eccentricity factor) to lunar terms, so perhaps we shouldn't either.

**Result: 208 failures (much worse)**

The E correction is essential for the Meeus 60-term model. SE's `accum_series()` handles E differently (through its T-dependent LRT terms), but the standalone Meeus model requires explicit E multiplication.

### Solar Planetary Perturbation Terms

Added the Venus, Jupiter, Moon, and long-period correction terms from Meeus p. 164:
```
-0.00569° sin(M')
-0.01060° sin(2M')
-0.00317° sin(M' - 2F)
etc.
```

**Result: 208 failures (worse)**

These terms shift solar longitude in ways that cause more boundary crossings to flip unfavorably against SE. The simplified equation of center with the combined aberration formula already produces better alignment with SE's VSOP87 output.

### Error Cancellation: Meeus p.164 Formula

This was the most important discovery. The Meeus p. 164 apparent longitude correction:
```
aberration = -0.00569° - 0.00478° × sin(Ω)
```
is presented as combining aberration (−0.00569°) with a nutation-related adjustment (−0.00478° × sin(Ω)). However, for our implementation, this combined formula provides beneficial error cancellation:

1. Our equation of center has only 3 sine terms (vs SE's ~135 VSOP87 harmonics)
2. This means our geometric longitude has systematic errors of ~10–20 arcseconds
3. The −0.00478° × sin(Ω) term (~17" amplitude, ~18.6 year period) happens to partially compensate for these geometric longitude errors
4. Using the "correct" decomposition (full 63-term nutation + clean −20.4898" aberration) removes this compensation

**Result**: 13-term nutation + combined formula: **120 failures**. 63-term nutation + clean aberration: **178 failures**.

This is a textbook example of how simplified models can achieve better results through compensating errors, and why replacing individual components with "more accurate" versions can degrade overall system performance.

### Summary of All Configurations Tested

| Sun | Moon | Sunrise | Failures | Phase |
|-----|------|---------|----------|-------|
| Meeus 3-term EoC + p.164 aberration | 60 Meeus | h₀=−0.5667°, GMST | **120** | 9 |
| VSOP87 + clean aberration | 60 Meeus | h₀=−0.5667°, GMST | **29** | 10 |
| VSOP87 + clean aberration | DE404 full pipeline | Sinclair, GAST | **2** | 11 |

Earlier experiments with the Phase 9 solar code:

| Moon Terms | Nutation | Aberration | E | A1/A2/A3 | Failures |
|-----------|----------|------------|---|----------|----------|
| 60 Meeus | 13-term | Meeus p.164 combined | Yes | Yes | 120 |
| 60 Meeus | 63-term | Clean −20.5" | Yes | Yes | 178 |
| 118 SE (partial) | 13-term | Meeus p.164 combined | Yes | No | 179 |
| 118 SE (partial) | 63-term | Clean −20.5" | Yes | No | 179 |
| 118 SE (partial) | 13-term | Meeus p.164 combined | No | No | 208 |
| 60 Meeus | 13-term | Meeus p.164 + planetary | Yes | Yes | 208 |

## Evolution of the Library

### Phase 9: Initial Implementation (787 lines, 120 failures)

- Solar: Meeus 3-term equation of center + p.164 combined aberration formula
- Lunar: 60-term ELP-2000/82 (Meeus Ch.47)
- Sunrise: Meeus Ch.15, 5 iterations, h₀ = −0.5667°, GMST
- Precision: solar ±13″, lunar ±10″, sunrise ±14s
- Beneficial error cancellation between solar and ayanamsa errors

### Phase 10: VSOP87 Solar Upgrade (1,265 lines, 29 failures)

- Solar: Full VSOP87 (135 harmonic terms from SE), standard aberration (−20.496″)
- Eliminated all 91 solar-related failures
- Exposed and fixed ayanamsa nutation bug (was double-counting Δψ)

### Phase 11: DE404 Moon + Sunrise (1,943 lines, 0 failures)

- Lunar: Full DE404 Moshier pipeline (ported from SE's swemmoon.c, longitude only)
- Delta-T: SE yearly lookup table (1620–2025) replacing Meeus polynomials
- Sunrise: Sinclair refraction (h₀ ≈ −0.612°), GAST, 10 iterations, midnight-UT wrap-around fix
- Eliminated 27 of 29 remaining failures

## References

1. Meeus, Jean. "Astronomical Algorithms", 2nd ed. Willmann-Bell, 1998.
   - Ch. 7: Julian Day
   - Ch. 10: Delta-T
   - Ch. 12: Sidereal Time
   - Ch. 15: Rising, Transit, Setting
   - Ch. 22: Nutation and Obliquity
   - Ch. 25: Solar Coordinates
   - Ch. 47: Position of the Moon

2. Bretagnon, P. & Francou, G. "Planetary theories in rectangular and spherical variables. VSOP87 solutions." Astronomy & Astrophysics, 202, 309-315, 1988.

3. Chapront-Touzé, M. & Chapront, J. "ELP 2000-85: a semi-analytical lunar ephemeris adequate for historical times." Astronomy & Astrophysics, 190, 342-352, 1988.

4. Lieske, J.H. et al. "Expressions for the Precession Quantities Based upon the IAU (1976) System of Astronomical Constants." Astronomy & Astrophysics, 58, 1-16, 1977.

5. Swiss Ephemeris source code (v2.10): `swemmoon.c`, `swemplan.c`, `sweph.c`, `swephlib.c`.
