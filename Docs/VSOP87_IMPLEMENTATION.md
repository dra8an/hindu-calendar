# VSOP87 Solar Longitude Implementation

## Overview

The self-contained Moshier ephemeris library (`lib/moshier/`, 1,265 lines) replaces the 51,493-line Swiss Ephemeris as the default backend. The solar longitude computation uses the VSOP87 planetary theory (Bretagnon & Francou, 1988) ported from Swiss Ephemeris source code, achieving ~1 arcsecond precision vs SE's output.

**Build options:**
- `make` — uses moshier backend (default)
- `make USE_SWISSEPH=1` — uses Swiss Ephemeris backend

## Library Structure

```
lib/moshier/
├── moshier.h           (35 lines)   Public API — 8 replacement functions
├── moshier_jd.c        (56 lines)   JD ↔ Gregorian, day of week
├── moshier_sun.c       (726 lines)  VSOP87 solar longitude, nutation, delta-T
├── moshier_moon.c      (146 lines)  Lunar longitude (60 ELP-2000/82 terms)
├── moshier_ayanamsa.c  (147 lines)  Lahiri ayanamsa (IAU 1976 precession)
└── moshier_rise.c      (155 lines)  Iterative sunrise/sunset
```

Total: **1,265 lines** (41x reduction from Swiss Ephemeris).

## VSOP87 Solar Longitude Pipeline

The solar position computation in `moshier_sun.c` follows this pipeline:

```
JD (UT) → JD (TT)
       → VSOP87 harmonic summation → EMB heliocentric ecliptic longitude (J2000, arcseconds)
       → General precession IAU 1976 → ecliptic of date (arcseconds → radians)
       → EMB→Earth correction (simplified Moon series)
       → Geocentric flip (+180°)
       → Nutation in longitude (13-term model)
       → Aberration (−20.496″ constant)
       → Normalize to [0°, 360°) → apparent tropical solar longitude
```

### Step 1: Time Conversion

```c
double jd_tt = jd_ut + delta_t(jd_ut);     // UT → Terrestrial Time
double T = (jd_tt - 2451545.0) / 36525.0;  // Julian centuries from J2000
```

Delta-T uses the polynomial approximations from Meeus & Simons (2000).

### Step 2: VSOP87 Harmonic Summation

The VSOP87 theory computes the heliocentric ecliptic longitude of the Earth-Moon Barycenter (EMB) as a sum of 135 harmonic terms. The data was extracted from Swiss Ephemeris source files `swemptab.h` and `swemplan.c`.

**Time variable:** `T_vsop = (jd_tt - 2451545.0) / 3652500.0` (in units of 10,000 Julian years, not centuries).

**Fundamental arguments:** 9 planetary mean longitudes (Simon et al., 1994):

| Index | Body | Frequency (″/10kyr) | Phase at J2000 |
|-------|------|---------------------|----------------|
| 0 | Mercury | 53,810,162,868.90 | 252.251° |
| 1 | Venus | 21,066,413,643.35 | 181.980° |
| 2 | Earth | 12,959,774,228.34 | 100.466° |
| 3 | Mars | 6,890,507,749.40 | 355.433° |
| 4 | Jupiter | 1,092,566,037.80 | 34.352° |
| 5 | Saturn | 439,960,985.54 | 50.077° |
| 6 | Uranus | 154,248,119.39 | 314.055° |
| 7 | Neptune | 78,655,032.07 | 304.349° |
| 8 | Precession | 52,272,245.18 | 860,492.15″ |

Each fundamental angle is: `θ_i = mods3600(freq_i × T_vsop) + phase_i` (in arcseconds), then converted to radians. The `sscc()` function pre-computes `sin(k×θ_i)` and `cos(k×θ_i)` for multiples k = 1..max_harmonic[i] using Chebyshev recurrence.

**Harmonic specification:** The `earargs[]` table (819 signed chars) encodes each term as:
- `np` = number of planetary arguments (0 = polynomial term, −1 = end)
- For each argument: harmonic multiplier `j` and planet index `m` (1-based)
- `nt` = maximum power of T for this term
- Corresponding coefficient pairs (cos, sin) × (nt+1) read from `eartabl[]`

**Data sizes:**
- `eartabl[]`: 460 doubles (longitude coefficients in arcseconds)
- `earargs[]`: 819 signed chars (harmonic specification)
- `freqs[]`: 9 doubles
- `phases[]`: 9 doubles

The first term is a polynomial of degree 3 (mean longitude):
```
L₀ = ((−65.547 × T − 232.750) × T + 12,959,774,227.576) × T + 361,678.596
```
where L₀ is in arcseconds. The linear coefficient matches Earth's mean motion; the constant matches the J2000 mean longitude.

**Output:** EMB heliocentric ecliptic longitude in arcseconds, referred to the ecliptic of J2000.

### Step 3: General Precession (IAU 1976)

The J2000 ecliptic longitude is precessed to the ecliptic of date using the general precession in longitude:

```
p_A = (5029.0966 + 1.11113 × T − 0.000006 × T²) × T   (arcseconds)
L_date = L_j2000 + p_A
```

Both are then converted from arcseconds to radians.

### Step 4: EMB→Earth Correction

VSOP87 computes the Earth-Moon Barycenter, not Earth. A simplified 6-term Moon longitude series (ported from SE's `embofs_mosh()`) computes the correction:

```c
// Simplified Moon position: mean anomaly, elongation, argument of latitude
// → 6-term series for Moon ecliptic longitude, latitude, distance
// First-order ecliptic correction:
ΔL = −r_moon × cos(B_moon) × sin(L_moon − L_emb) / (EARTH_MOON_MRAT + 1.0)
```

where `EARTH_MOON_MRAT = 81.30057` (Earth/Moon mass ratio from DE431), and `R_emb ≈ 1.0 AU` is assumed (±1.7% → ±0.11″ in correction, negligible). The correction magnitude is up to ~6.5″.

### Step 5: Geocentric Flip

```c
L_sun = L_earth + π     // heliocentric Earth → geocentric Sun
```

### Step 6: Nutation

A 13-term nutation model computes the nutation in longitude (Δψ), based on the 5 fundamental Delaunay arguments. The dominant term (lunar node, 18.6-year period) contributes ~17.2″.

```c
L_apparent = L_sun + Δψ
```

### Step 7: Aberration

A constant aberration correction of −20.496″ is applied (the "constant of aberration"):

```c
L_apparent −= 20.496 × STR   // STR = arcseconds to radians
```

**Important:** With VSOP87, the standard constant aberration must be used. The Meeus p.164 combined formula (−0.00569° − 0.00478° × sin(Ω)) that worked with the simplified equation of center must NOT be used — it provides error cancellation that only works with the Meeus 3-term EoC, not with VSOP87's more accurate geometric longitude.

## Ayanamsa Implementation

The Lahiri ayanamsa is computed using full 3D equatorial precession matching SE's algorithm:

1. Start with vernal point `(1, 0, 0)` in equatorial coordinates at target date
2. Precess from target date → J2000 using IAU 1976 precession angles (Lieske et al., 1977)
3. Precess from J2000 → t₀ (Lahiri reference epoch, JD 2435553.5 = 1956 Sept 22)
4. Rotate to ecliptic of t₀ using Laskar's obliquity polynomial
5. Compute polar longitude
6. Ayanamsa = −longitude + ayan_t₀ (where ayan_t₀ = 23.245524743°)

**Lahiri constants** (from SE's `sweph.h`, index 1):
- t₀ = JD 2435553.5 (1956 September 22.0 TT)
- ayan_t₀ = 23.250182778° − 0.004658035° = 23.245524743°
- Precession model: IAU 1976

### Critical Discovery: Nutation Must NOT Be Added

`swe_get_ayanamsa_ut()` returns the **mean** ayanamsa (without nutation in longitude). This was discovered during VSOP87 development when the ayanamsa showed a ~17″ oscillating error with 18.6-year period — exactly matching the dominant nutation term.

**Why nutation cancels in sidereal positions:**
```
sidereal_longitude = (tropical + Δψ) − (ayanamsa + Δψ) = tropical − ayanamsa
```

If nutation (Δψ) is added to the ayanamsa, it gets double-counted. The mean ayanamsa is correct because the tropical longitude already includes nutation, and subtracting the mean ayanamsa gives the correct sidereal position.

## Precision vs Swiss Ephemeris

Comparison at representative dates (1900–2050):

| Quantity | Moshier vs SE | Notes |
|----------|--------------|-------|
| Tropical solar longitude | ±1″ | VSOP87 harmonic summation |
| Ayanamsa (mean) | ±0.3″ | IAU 1976 3D precession |
| Sidereal solar longitude | ±0.5″ | Combined solar + ayanamsa |
| Lunar longitude | ±10″ | Meeus Ch.47 (60-term ELP-2000/82) |
| Sunrise/sunset | ±14 seconds | Meeus Ch.15 iterative |

The sidereal solar longitude precision of ±0.5″ translates to ~2 seconds of sankranti timing error — well within the empirical buffer margins for Tamil (−8.0 min) and Malayalam (−9.5 min) solar calendars.

## Test Results

### Before VSOP87 (Meeus 3-term Equation of Center)

| Suite | Passed | Failed |
|-------|--------|--------|
| Solar | 330/351 | 21 |
| Solar edge | 1,176/1,200 | 24 |
| Solar Regression | 28,930/28,976 | 46 |
| Other (tithi-based) | — | 29 |
| **Total** | **53,023/53,143** | **120** |

### After VSOP87 + Ayanamsa Fix

| Suite | Passed | Failed |
|-------|--------|--------|
| Solar | 351/351 | **0** |
| Solar edge | 1,200/1,200 | **0** |
| Solar Regression | 28,976/28,976 | **0** |
| Other (tithi-based) | — | 29 |
| **Total** | **53,114/53,143** | **29** |

All 91 solar-related failures eliminated. The remaining 29 failures are tithi boundary edge cases from the ~10″ lunar longitude precision (Moon-Sun elongation, independent of ayanamsa/solar longitude).

## Evolution of Solar Longitude Approaches

### v1: Meeus 3-term Equation of Center (787 lines total)

- Solar longitude: 3-term EoC + combined aberration formula (Meeus p.164)
- Error vs SE: ~13″ in solar longitude
- 120 test failures, but had **beneficial error cancellation**: the ~13″ solar error partially cancelled the ~15″ ayanamsa error (from IAU 1976 vs Vondrak 2011 precession), resulting in only ~2″ sidereal error near J2000

### v2: VSOP87 + Nutation Bug (726 lines in moshier_sun.c)

- Solar longitude: VSOP87 (±1″)
- Ayanamsa: still adding nutation (bug)
- Error vs SE: ~17″ oscillating (nutation double-counted)
- 145 test failures — worse than v1! Removing the cancellation exposed the nutation bug

### v3: VSOP87 + Ayanamsa Fix (current, 1,265 lines total)

- Solar longitude: VSOP87 (±1″)
- Ayanamsa: mean only, no nutation (correct)
- Error vs SE: ~0.5″ sidereal
- 29 test failures (all tithi-related, not solar)

## Key Lessons Learned

1. **Error cancellation can mask bugs.** The Meeus EoC had ~13″ solar error that partially cancelled the ~15″ ayanamsa nutation error. When VSOP87 fixed the solar error, test results got *worse* — revealing the underlying ayanamsa bug.

2. **`swe_get_ayanamsa_ut()` returns MEAN ayanamsa.** This is not documented prominently in the SE manual. Nutation in longitude cancels in sidereal position calculations: `sid = (trop + Δψ) − (ayan + Δψ) = trop − ayan`.

3. **SE uses Vondrak 2011 precession, not IAU 1976.** For Lahiri ayanamsa (defined with IAU 1976), SE applies a correction via `get_aya_correction()`. Our implementation uses IAU 1976 directly, avoiding the need for this correction. The residual precession model difference at the target date is only ~0.3″.

4. **VSOP87 uses 10,000-year timescale, not Julian centuries.** The time variable is `T = (JD − J2000) / 3652500.0`, not the usual `T = (JD − J2000) / 36525.0` used for nutation and precession.

5. **Standard aberration with VSOP87.** The Meeus p.164 combined formula (aberration + nutation adjustment) that worked with the simplified EoC must be replaced with the standard constant aberration (−20.496″) when using VSOP87. The combined formula provides error cancellation specific to the 3-term EoC.

6. **SE's 118 lunar terms gave worse results than Meeus's 60.** SE's ELP-2000/82 terms require additional corrections (Venus-Jupiter perturbations, moon2 terms) that are embedded in SE's full pipeline. Meeus's 60 terms are self-consistent with the E correction and A1/A2/A3 additional terms.

## Data Provenance

All VSOP87 data was extracted from Swiss Ephemeris source files:
- **Frequencies and phases**: `lib/swisseph/swemplan.c` lines 88–114
- **Harmonic coefficients** (`eartabl[]`): `lib/swisseph/swemptab.h` lines 1925–2216
- **Argument specification** (`earargs[]`): `lib/swisseph/swemptab.h` lines 2802–2938
- **Summation algorithm**: `lib/swisseph/swemplan.c` `swi_moshplan2()` lines 134–264
- **EMB→Earth correction**: `lib/swisseph/swemplan.c` `embofs_mosh()` lines 416–492
- **Mass ratio**: `lib/swisseph/sweph.h` `EARTH_MOON_MRAT = 1/0.0123000383`

The `ear404` table structure: `max_harmonic = {1,9,14,17,5,5,2,1,0}`, `max_power_of_t = 4`, 135 total terms (134 periodic + 1 polynomial).

Only the longitude coefficients (`eartabl[]`, 460 doubles) are used. Latitude (`eartabb[]`) and radius (`eartabr[]`) are not needed for solar longitude, saving 920 doubles of data.
