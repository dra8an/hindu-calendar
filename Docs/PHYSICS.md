# Physics Behind the Hindu Calendar

This document explains the astronomical computations that underpin the calendar,
from orbital mechanics down to the actual code in `lib/moshier/moshier_sun.c`.

---

## Why astronomy matters for a calendar

The Hindu calendar is astronomically determined — not rule-based like the Gregorian
calendar. Every date depends on real-time positions of the Sun and Moon:

- **Tithi** (lunar day): angle between Moon and Sun, changing every ~24 hours
- **Masa** (month): which zodiac sign the Sun is in at the new moon
- **Sankranti** (solar month boundary): exact moment the Sun enters a new sign
- **Sunrise/sunset**: determines which civil day "owns" an astronomical event

All of these require knowing **where the Sun is** to sub-arcminute precision.
That's the job of the solar position pipeline.

---

## The two-body problem: Kepler's ideal

If the solar system were just the Sun and Earth, the orbit would be a perfect
ellipse (Kepler's first law). You could compute Earth's position with a simple
formula involving:

- Semi-major axis (average distance)
- Eccentricity (how elongated the ellipse is)
- Mean anomaly (angle swept since perihelion, increasing uniformly with time)
- Equation of center (correction from uniform motion to actual elliptical motion)

This gives ~1° accuracy. Not nearly enough — we need ~1 arcsecond (1/3600°).

---

## The N-body problem: gravitational perturbations

Earth doesn't orbit the Sun alone. Jupiter, Venus, Saturn, and every other planet
exert gravitational pulls on Earth, distorting its orbit from a perfect ellipse.
These distortions are called **gravitational perturbations**.

Each perturbation is periodic — it repeats at a frequency determined by the
relative orbital periods of the two bodies involved. For example:

- **Jupiter** (orbital period ~11.86 years) creates perturbations that repeat
  roughly every 12 years relative to Earth
- **Venus** (period ~0.615 years) creates faster perturbations
- **Jupiter + Saturn combined** create the "Great Inequality" — a ~900-year
  oscillation caused by their near 5:2 orbital resonance (Jupiter orbits ~5 times
  for every ~2 Saturn orbits)

These perturbations range from ~10 arcseconds (Jupiter) down to fractions of an
arcsecond (Uranus, Neptune). To reach 1-arcsecond precision, you need to account
for all of them.

---

## VSOP87: the harmonic series solution

VSOP87 (Variations Séculaires des Orbites Planétaires, Bretagnon & Francou 1988)
is the standard solution to this problem. Instead of simulating gravity in real
time (numerical integration), it expresses each planet's position as a **sum of
harmonic terms** — essentially a Fourier series:

```
L(t) = Σ [ C_i(t) × cos(angle_i(t)) + S_i(t) × sin(angle_i(t)) ]
```

Each term has:

1. **An angle** — a linear combination of "fundamental arguments" (the angular
   positions of the planets):
   ```
   angle = n₁×Mercury + n₂×Venus + n₃×Earth + n₄×Mars + n₅×Jupiter + ...
   ```
   The integers (n₁, n₂, ...) identify which gravitational interaction this term
   represents. For example, `2×Jupiter - 5×Saturn` captures the Great Inequality.

2. **Amplitude coefficients** C(t) and S(t) — polynomials in time that give the
   strength of this perturbation. Larger planets at closer distances have larger
   amplitudes.

The full VSOP87 theory has thousands of terms per planet. Our implementation uses
135 terms for Earth's longitude, selected from the Swiss Ephemeris implementation
(which itself derived them from VSOP87). This gives ~1 arcsecond precision over
the range 1900–2100.

### Why 135 terms?

We originally tried 60 terms (Meeus's simplified series) — it had 120 test
failures against drikpanchang.com. We then tried 118 terms from just the main
lunar table — paradoxically worse (179 failures) because partial series can
introduce systematic bias. The full 135-term set from Swiss Ephemeris, with all
correction pipelines, achieved 99.971% match. Fewer terms = less accuracy, and
we learned this the hard way.

---

## What `vsop87_earth_longitude()` computes

This function (`lib/moshier/moshier_sun.c:361`) is the computational bottleneck —
~63% of total runtime per the profiling results. Here's what it does:

### Step 1: Fundamental arguments (9 planets)

```c
for (int i = 0; i < 9; i++) {
    double sr = (mod_arcsec(planet_freq[i] * T) + planet_phase[i]) * ARCSEC_TO_RAD;
    precompute_sincos(i, sr, earth_max_harm[i]);
}
```

For each of 9 fundamental frequencies (Mercury through Neptune, plus a lunar
argument), compute the planet's angular position at time T:

```
argument = frequency × T + phase
```

The `planet_freq` array contains orbital frequencies in arcseconds per 10,000
Julian years. For example, Mercury's frequency (53,810,162,868.9) corresponds to
~414.9 revolutions per century — Mercury orbits the Sun roughly 4.15 times per year.

Then `precompute_sincos()` builds a table of sin(k×arg) and cos(k×arg) for
harmonics k = 1, 2, 3, ... using a Chebyshev recurrence relation. This is cheaper
than calling sin/cos for each harmonic individually — you compute sin(arg) and
cos(arg) once, then derive higher harmonics by multiplying:

```
sin(2θ) = 2·sin(θ)·cos(θ)
cos(2θ) = cos²(θ) - sin²(θ)
sin(3θ) = sin(θ)·cos(2θ) + cos(θ)·sin(2θ)
```

This turns 9 sin/cos calls into sufficient data for all 135 terms.

### Step 2: Walk the 135 terms

Each term is encoded as entries in two arrays:

- `earth_args[819]` (signed chars): which planets and which harmonics
- `earth_coeffs[460]` (doubles): amplitude coefficients

For each periodic term:

**2a. Build the combined angle:**
```c
for (int ip = 0; ip < np; ip++) {
    int j = *p++;   // harmonic number (e.g., +2 or -5)
    int m = *p++;   // which planet (0=Mercury, 4=Jupiter, etc.)
    // look up sin/cos from precomputed table, combine via addition formulas
    sv = su*cv + cu*sv;  // sin(A+B) = sin(A)cos(B) + cos(A)sin(B)
    cv = cu*cv - su*sv;  // cos(A+B) = cos(A)cos(B) - sin(A)sin(B)
}
```

This computes sin(angle) and cos(angle) where `angle = n₁×arg₁ + n₂×arg₂ + ...`
without ever calling sin/cos directly — it combines the precomputed values using
the angle addition identities.

**2b. Evaluate the amplitude polynomial:**
```c
double cu = *pl++;  // constant coefficient
double su = *pl++;
for (int ip = 0; ip < nt; ip++) {
    cu = cu * T + *pl++;  // Horner's method: c₀ + c₁T + c₂T² + ...
    su = su * T + *pl++;
}
```

The amplitude varies slowly with time (secular variation) — a polynomial in T,
typically degree 0–3.

**2c. Accumulate:**
```c
sl += cu * cv + su * sv;
```

This is `C(T)×cos(angle) + S(T)×sin(angle)` — one harmonic term's contribution
to Earth's longitude.

### Result

After all 135 terms, `sl` contains Earth's heliocentric ecliptic longitude in
arcseconds, referred to the J2000 ecliptic. This is where Earth is as seen from
the Sun, in the coordinate system of the year 2000.

---

## The full solar position pipeline

`vsop87_earth_longitude()` gives raw heliocentric Earth coordinates. Six more
steps transform this into the apparent geocentric solar longitude we need:

### Step 1: VSOP87 → heliocentric EMB longitude (J2000)

What we just described. "EMB" means Earth-Moon Barycenter — VSOP87 actually tracks
the center of mass of the Earth-Moon system, not Earth itself.

### Step 2: Precession (J2000 → ecliptic of date)

Earth's rotation axis wobbles like a spinning top (precession), completing one
cycle every ~25,800 years. This slowly shifts the coordinate system. The IAU 1976
precession formula advances the ecliptic from the J2000 reference frame to the
current date:

```
p_A = (5029.0966 + 1.11113×T − 0.000006×T²) × T  arcseconds
```

At ~50"/year, this is ~28' per century — significant.

### Step 3: EMB → Earth correction

The Moon orbits Earth at ~384,000 km. VSOP87 tracks the Earth-Moon Barycenter,
which is ~4,670 km from Earth's center (inside Earth, since Earth is 81× heavier
than the Moon). This creates a monthly oscillation of up to ~6" in the apparent
solar longitude.

The correction uses a simplified 6-term lunar series to compute the Moon's position
and derive the offset:

```
ΔL = −r_moon × cos(B) × sin(L_moon − L_emb) / ((M_earth/M_moon + 1) × R)
```

### Step 4: Geocentric flip (+180°)

VSOP87 gives heliocentric Earth longitude (where Earth is, seen from the Sun).
We need geocentric solar longitude (where the Sun is, seen from Earth). These
differ by exactly 180°.

### Step 5: Nutation (Δψ)

Earth's axis doesn't just precess smoothly — it also **nutates** (wobbles on
shorter timescales). The dominant term has an 18.6-year period caused by the
Moon's orbital plane precessing around the ecliptic. Our 13-term IAU 1980 model
computes nutation in longitude (Δψ) with sub-arcsecond precision:

```
Δψ = Σ (s₀ + s₁×T) × sin(n₁D + n₂M + n₃M' + n₄F + n₅Ω)
```

where D, M, M', F, Ω are lunar and solar fundamental arguments.

The largest term (Ω alone) contributes up to ±17.2 arcseconds. Nutation shifts
the apparent solar longitude and also affects sidereal time (equation of equinoxes).

### Step 6: Aberration (−20.496")

Light takes ~8.3 minutes to travel from Sun to Earth. During that time, Earth
moves ~20.5" along its orbit. So the Sun appears ~20.5" behind its true geometric
position. This constant correction (the "constant of aberration") is subtracted.

### Result

After all six steps, we have the **apparent geocentric solar longitude** — where
the Sun appears in the sky as seen from Earth, accounting for Earth's orbital
perturbations, the wobble of its axis, the Moon's influence, and the finite speed
of light. This is what determines tithi, masa, sankranti, and everything else in
the Hindu calendar.

---

## From solar longitude to sunrise

Knowing where the Sun is gets us the calendar date. But the Hindu calendar also
needs **sunrise and sunset** to determine which civil day owns each astronomical
event. This requires a different computation.

### Right Ascension and Declination

Solar longitude (ecliptic coordinates) must be converted to equatorial coordinates
(RA and Dec) for sunrise calculation:

```
RA = atan2(cos(ε) × sin(λ), cos(λ))
Dec = asin(sin(ε) × sin(λ))
```

where λ is the apparent solar longitude and ε is the true obliquity (the tilt of
Earth's axis, ~23.4°, slowly changing due to precession and nutation).

### Meeus iterative sunrise

The sunrise algorithm (Meeus Ch. 15, `lib/moshier/moshier_rise.c`) iterates:

1. Estimate transit time (when Sun crosses the meridian)
2. Compute hour angle at the desired altitude (horizon + refraction + semi-diameter)
3. Estimate rise/set time from transit ± hour angle
4. Recompute solar RA/Dec at the estimated time → refine
5. Repeat until convergence (~3–4 iterations, ~0.009 second precision)

Each iteration calls `solar_position()` once (via `moshier_solar_ra_dec()`).
So each sunrise costs ~4 full VSOP87 evaluations — the dominant per-day cost.

### Refraction and upper limb

"Sunrise" in the Hindu calendar means when the **upper edge** of the Sun's disc
first appears above the horizon. This requires:

- **Atmospheric refraction** at the horizon: the atmosphere bends light, making the
  Sun visible ~34' before it geometrically clears the horizon. We use the Sinclair
  (1982) formula: ~0.612° at standard conditions.
- **Solar semi-diameter**: the Sun's disc subtends ~32' (±0.5' seasonally). Upper
  limb = center + 16'.
- **Total**: the Sun appears to rise when its geometric center is ~0.88° below
  the horizon.

---

## Sidereal longitude and the ayanamsa

The Hindu calendar uses **sidereal** (nirayana) coordinates — positions relative to
the fixed stars — rather than the tropical coordinates used in Western astronomy.

The difference is the **ayanamsa**: the accumulated precession since the two systems
were aligned. We use the Lahiri ayanamsa (the Indian government standard):

```
sidereal_longitude = tropical_longitude − ayanamsa
```

The ayanamsa is currently ~24.2° and growing at ~50.3"/year. It's computed via
IAU 1976 3D equatorial precession (`lib/moshier/moshier_ayanamsa.c`), matching
the Swiss Ephemeris SE_SIDM_LAHIRI value to ±0.3".

**Important subtlety**: nutation cancels in sidereal coordinates. The tropical
longitude includes nutation (+Δψ), and the "true" ayanamsa would also include
nutation (+Δψ). So: sidereal = (tropical + Δψ) − (ayanamsa + Δψ) = tropical −
ayanamsa. This is why `moshier_ayanamsa()` returns the **mean** ayanamsa without
nutation — adding nutation would cause a ~17" oscillating error with an 18.6-year
period.

---

## The Moon pipeline

The Moon's position is needed for tithi (Moon−Sun elongation) and new moon finding
(month boundaries). It uses a completely different computation from the Sun.

### Why the Moon is hard

If the Sun's position requires 135 harmonic terms, you might expect the Moon to be
simpler — it's much closer. But the opposite is true. The Moon is the hardest body
in the solar system to model analytically because:

1. **The Sun's perturbation is enormous.** The Sun pulls on the Moon almost as
   strongly as Earth does. This creates large, complex variations in the orbit
   that don't occur for planets (which are far from other massive bodies).

2. **Multiple bodies matter simultaneously.** Jupiter and Venus both measurably
   perturb the Moon's orbit. The interplay between the Sun, Jupiter, and Venus
   creates combination terms (e.g., `18×Venus − 16×Earth − Moon_anomaly`) with
   surprisingly large amplitudes.

3. **The orbit precesses rapidly.** The Moon's orbital plane rotates around the
   ecliptic every 18.6 years (the "regression of the nodes"), and the major axis
   of the orbit rotates every 8.85 years. These create additional periodicities
   that must be tracked.

4. **Resonances amplify small effects.** Near-resonances between the Moon's orbital
   frequencies and planetary frequencies create terms that grow over centuries. The
   Venus-Earth-Moon quasi-resonance (the Saros-like cycle at ~18.03 years) is
   particularly important.

The result: the first complete analytical Moon theory (ELP2000, Chapront-Touzé &
Chapront 1988) contained over 30,000 terms. Even truncated to practical precision,
you need far more terms than for the Sun.

### JPL DE404 vs Moshier DE404

Our implementation doesn't use a pure analytical theory. Instead, it uses Steve
Moshier's **DE404 fit** — but to understand what that means, we need to distinguish
two very different things that share the "DE404" name.

#### JPL DE404: the numerical ephemeris

JPL is NASA's **Jet Propulsion Laboratory** — the organization that navigates
spacecraft to other planets. Their "DE" series (Development Ephemeris) are the
gold standard for solar system positions. DE404 specifically is a long-timespan
ephemeris covering 6,000 years (−3000 to +3000).

JPL produced DE404 by **numerically integrating** Newton's equations of motion.
Starting from precisely known modern positions (from radar ranging, laser
reflectors on the Moon, and spacecraft tracking), they stepped the equations
of motion backward and forward in tiny time increments, accounting for:

- Gravitational attraction between all major bodies (Sun, Moon, 8 planets)
- General relativistic corrections
- Tidal deformation of the Earth and Moon
- Perturbations from 300+ asteroids

The result is a ~35 MB binary file of **Chebyshev polynomial coefficients** —
essentially a giant lookup table. To find the Moon's position at any time, you
interpolate between tabulated values. This is the most accurate approach possible,
but it requires distributing and loading the binary file at runtime. Swiss
Ephemeris takes this approach — it repackages JPL data into its own format.

#### Moshier DE404: the analytical approximation

Moshier's contribution was creating a **compact analytical formula** that
approximates JPL DE404 without needing the binary file. He started with the
structure of the ELP2000-85 analytical lunar theory (Chapront-Touzé & Chapront,
1983) — which expresses the Moon's position as sums of sinusoidal terms with
known physical meanings (evection, variation, equation of center, etc.) — then
adjusted the coefficients via **least-squares fitting** against 34,247 positions
sampled from JPL DE404 across the full 6,000-year range.

The result is ~760 lines of C code with ~2 KB of coefficient tables, evaluating
in microseconds while achieving ~3–5 arcsecond accuracy over 6,000 years and
±0.07" agreement with Swiss Ephemeris over our 1900–2050 range.

#### Comparison

| | JPL DE404 | Moshier DE404 |
|---|---|---|
| **Type** | Numerical integration | Analytical series (fitted) |
| **Size** | ~35 MB binary file | ~2 KB coefficient tables |
| **Precision** | Sub-arcsecond (the "truth") | 0.018" RMS, 0.065" max vs JPL |
| **Dependency** | Needs binary file at runtime | Self-contained in source code |
| **Structure** | Chebyshev polynomials (opaque) | Named physical terms (evection, etc.) |
| **Speed** | Interpolation (fast) | Trigonometric series (fast) |

For our calendar, 0.065" max error is negligible — tithi boundaries span 12°
(43,200"), giving us ~660,000× margin over the worst-case Moshier error.

### The four fundamental arguments

The Moon's position is expressed in terms of four angles that change with time:

- **D** — Mean elongation: the average angular distance between the Moon and Sun.
  Increases by ~445,267° per century. New moon occurs when D = 0° (mod 360°).

- **l** (mean anomaly of the Moon) — the Moon's position along its elliptical orbit,
  measured from perigee (closest approach). Increases by ~477,199° per century.
  Controls the largest perturbation: the "equation of center" (~6.3° amplitude).

- **l'** (mean anomaly of the Sun) — the Sun's position along its orbit from
  perihelion. Increases by ~35,999° per century. Controls the "evection" and other
  solar perturbation terms.

- **F** — Argument of latitude: the Moon's angular distance from its ascending node
  (where the Moon's orbital plane crosses the ecliptic). Increases by ~483,202° per
  century. Controls latitude and some longitude terms.

These four arguments, combined in various integer multiples, generate all the
periodic terms in the Moon's longitude.

### The three perturbation tables

The heart of the Moon pipeline is three tables of periodic terms, each at a different
power of time T (Julian centuries from J2000):

#### `moon_lr` — Main table (118 terms)

```c
static const short moon_lr[8 * 118] = {
/*  D  l' l  F    lon_1"  lon_.0001"  rad_1km  rad_.0001km */
    0, 0, 1, 0,  22639,  5858,       -20905,  -3550,    /* l: equation of center */
    2, 0,-1, 0,   4586,  4383,        -3699,  -1109,    /* 2D-l: evection */
    2, 0, 0, 0,   2369,  9139,        -2955,  -9676,    /* 2D: variation */
    0, 0, 2, 0,    769,   257,         -569,  -9251,    /* 2l */
    ...
};
```

Each row encodes one periodic term. The first four columns are the integer
multipliers for D, l', l, F — defining the angle:

```
angle = n₁×D + n₂×l' + n₃×l + n₄×F
```

The next two columns give the longitude coefficient in arcseconds (split as
integer arcseconds + ten-thousandths for precision). The last two columns give
the radius coefficient (which we skip — we only need longitude).

The term `(0, 0, 1, 0)` with coefficient 22,639.5858" is the **equation of center**:
the largest single perturbation (~6.29°), caused by the elliptical shape of the
Moon's orbit.

The term `(2, 0, -1, 0)` with coefficient 4,586.4383" is the **evection** (~1.27°):
a modulation of the equation of center caused by the Sun's gravity varying the
Moon's orbital eccentricity over each synodic month.

The term `(2, 0, 0, 0)` with coefficient 2,369.9139" is the **variation** (~0.66°):
the Sun accelerates the Moon near new/full moon and decelerates it near quarters.

These 118 terms capture the constant (T⁰) part of the Moon's perturbations.

#### `moon_lr_t1` — First-order secular terms (38 terms)

```c
static const short moon_lr_t1[8 * 38] = {
/*  D  l' l  F   .1"  .00001"  .1km  .00001km — multiply by T */
    0, 1, 0, 0,   16, 7680,    -1, -2302,
    ...
};
```

Same format as the main table, but these coefficients are multiplied by T (centuries
from J2000). They capture the slow drift of perturbation amplitudes over time — for
example, the Sun's mean anomaly coefficient (l') changes because Earth's orbital
eccentricity is slowly decreasing. The amplitudes are ~100x smaller than the main
table (0.1" units vs 1" units).

#### `moon_lr_t2` — Second-order secular terms (25 terms)

```c
static const short moon_lr_t2[6 * 25] = {
/*  D  l' l  F  .00001"  .00001km — multiply by T² */
    0, 1, 0, 0,  487,   -36,
    ...
};
```

Multiplied by T². Even smaller corrections (0.00001" units) for second-order
secular changes. The reduced format (6 values per row instead of 8) reflects the
lower precision needed.

### How the tables combine

The total perturbation in longitude is:

```
ΔL = Σ moon_lr[i] × sin(angle_i)
   + Σ moon_lr_t1[i] × T × sin(angle_i)
   + Σ moon_lr_t2[i] × T² × sin(angle_i)
```

This is equivalent to each term having a time-dependent amplitude:

```
A(T) = A₀ + A₁×T + A₂×T²
```

where A₀ comes from the main table, A₁ from the T¹ table, and A₂ from the T²
table. Over the 150-year range we care about (1900–2050), T ranges from −1 to
+0.5, so the T¹ corrections contribute up to ~1.7" and T² corrections up to ~0.05".

### Additional corrections beyond the tables

The pipeline doesn't stop at the three tables. `lunar_perturbations()` also
computes:

1. **Explicit planetary perturbation terms** — 8 specific combinations involving
   Venus, Earth, Mars, Jupiter, and Saturn orbital angles. These are computed
   individually (not from a table) because they involve planetary longitudes that
   aren't part of the D/l'/l/F system. The largest is `18×Venus − 16×Earth − l`
   at ~12.7" amplitude — a near-resonance between Venus and the Moon.

2. **DE404 correction coefficients** (`de404_corr[25]`) — higher-degree secular
   corrections (T³, T⁴) to the mean elements and planetary perturbation amplitudes,
   derived from the least-squares fit to DE404. These ensure the mean elements
   track the numerical ephemeris over the full 6,000-year range.

3. **Variable light-time correction** — the Moon is close enough that its distance
   matters for the light-time delay. Five terms from the radius portion of the main
   table are used to compute the Moon's distance, which determines the light-time
   correction to the longitude (~0.7" at mean distance).

### The full pipeline

```
mean_elements()       → D, l', l, F, mean lunar longitude (polynomial in T)
mean_elements_pl()    → Venus, Earth, Mars, Jupiter, Saturn longitudes
lunar_perturbations() → combine all tables + explicit terms + DE404 corrections
                      → apply nutation
                      → result: apparent geocentric lunar longitude
```

The final result is the Moon's longitude in the same apparent geocentric coordinate
system as the Sun's — suitable for computing tithi (Moon − Sun elongation) and
finding new moons (where Moon longitude = Sun longitude).

### Precision

The full pipeline achieves ±0.07" RMS against Swiss Ephemeris over 1900–2050 (max
error 0.065"). This is actually better than our solar longitude (±1"), which makes
sense — the DE404 fit was specifically optimized for the Moon, while our VSOP87
implementation for the Sun uses a truncated series.

Despite its complexity (181 table terms + 8 explicit terms + DE404 corrections),
the Moon pipeline is only ~2.4% of total runtime. This is because the new moon
cache (hit rate ~97%) means it rarely needs to run — only ~2 times per month for
new moon finding. When it does run, it's fast: one pass through the tables with
precomputed sin/cos lookups, similar to the VSOP87 approach for the Sun.

---

## From astronomy to a Hindu date

Everything above describes how we compute where the Sun and Moon are. This section
explains how those positions become an actual Hindu calendar date: tithi, masa,
year, and the assignment to a civil (Gregorian) day.

### Tithi: the lunar day

A tithi is a 12° segment of the Moon–Sun elongation. The formula:

```
elongation = (lunar_longitude − solar_longitude) mod 360°
tithi = floor(elongation / 12°) + 1
```

This gives tithi 1–30. Tithis 1–15 are Shukla (bright half, waxing moon), 16–30
are Krishna (dark half, waning moon). Tithi 15 is Purnima (full moon, elongation
~180°), tithi 30 is Amavasya (new moon, elongation ~360°/0°).

Both longitudes are **tropical** (not sidereal) because the ayanamsa cancels:
`(moon_sid − sun_sid) = (moon_trop − ayan) − (sun_trop − ayan) = moon_trop − sun_trop`.

A tithi is not tied to a clock — it's purely angular. The Moon gains ~12° on the
Sun per day, so a tithi lasts ~23.5–25.5 hours (varying because the Moon's speed
fluctuates along its elliptical orbit).

**Implementation**: `src/tithi.c` — `tithi_num_at_jd(jd)` computes the tithi
number at any given Julian Date by evaluating `lunar_phase(jd)` (which calls both
the Moon and Sun pipelines) and dividing by 12.

### Which civil day owns a tithi: the sunrise rule

The Hindu calendar assigns each civil day the tithi that is **active at sunrise**.
This creates two special cases:

- **Adhika tithi** (repeated): if the same tithi is active at two consecutive
  sunrises, both days get the same tithi number. This happens when a tithi is
  longer than the time between two sunrises (~24h). Occurs roughly every 2 months.

- **Kshaya tithi** (skipped): if a tithi starts after one sunrise and ends before
  the next, no civil day ever "sees" it at sunrise. The tithi is skipped entirely.
  Also occurs roughly every 2 months.

This is why sunrise precision matters — a difference of 1–2 minutes at a tithi
boundary can change which tithi is active at sunrise, affecting the entire day's
calendar assignment.

**Implementation**: `src/panchang.c` — `gregorian_to_hindu()` computes sunrise for
the given date, evaluates the tithi at that moment, and checks the previous day's
tithi for adhika detection.

### Masa: the lunar month

The Hindu lunisolar calendar (Amanta scheme) runs from new moon to new moon. Each
month is named after the solar sign (rashi) in which the Sun resides at the new
moon that starts the month:

```
1. Find the two new moons bracketing the current day
2. Compute the Sun's sidereal longitude at the preceding new moon
3. The solar rashi = ceil(sidereal_longitude / 30°), giving rashi 1–12
4. Month name = next rashi (rashi + 1, wrapping 12→1)
```

For example, if the Sun is in Mesha (rashi 1, Aries) at the new moon, the month
is Vaishakha (month 2). The mapping:

```
Mesha(1)→Vaishakha, Vrishabha(2)→Jyeshtha, Mithuna(3)→Ashadha,
Karkata(4)→Shravana, Simha(5)→Bhadrapada, Kanya(6)→Ashvina,
Tula(7)→Kartika, Vrischika(8)→Margashirsha, Dhanu(9)→Pausha,
Makara(10)→Magha, Kumbha(11)→Phalguna, Meena(12)→Chaitra
```

**New moon finding**: we use inverse Lagrange interpolation on `lunar_phase()` —
sampling the Moon–Sun elongation at 9 points around the estimated new moon, then
interpolating to find where it crosses 360° (= 0°). The tithi number provides the
initial estimate: if we're at tithi T, the previous new moon was ~T days ago.

**Implementation**: `src/masa.c` — `new_moon_before()`, `new_moon_after()`, and
`masa_compute()`.

### Adhika masa: the leap month

The Sun takes ~30.4 days to traverse one rashi (30° of sidereal longitude). A
lunar month (new moon to new moon) is ~29.5 days. Usually, the Sun crosses into
a new rashi during each lunar month. But occasionally (~every 32.5 months), a
lunar month fits entirely within one rashi — no sankranti occurs.

When both new moons bracketing a month have the Sun in the **same rashi**, that
month is **adhika** (intercalary/leap). It takes the same name as the following
regular month but is prefixed with "Adhika." The regular month that follows is
called **nija** (true).

```
if solar_rashi(preceding_new_moon) == solar_rashi(following_new_moon):
    this month is adhika
```

Adhika months keep the lunar calendar synchronized with the solar year. Without
them, the months would drift through the seasons (like the Islamic calendar does).

The rare opposite — **kshaya masa** (lost month) — occurs when the Sun traverses
an entire rashi between two new moons, skipping a month name entirely. This happens
only once every 19–141 years.

**Implementation**: `src/masa.c` — `masa_compute()` checks if `rashi_last == rashi_next`.

### Hindu solar calendars: the sankranti rule

The four regional solar calendars (Tamil, Bengali, Odia, Malayalam) determine
months differently — by the Sun's actual position rather than the Moon's phase.
A solar month begins when the Sun enters a new sidereal rashi (a **sankranti**):

```
sidereal_solar_longitude = tropical_longitude − ayanamsa
rashi = ceil(sidereal_longitude / 30°)
```

The solar month boundary is the exact moment the Sun crosses a 30° boundary
(0°, 30°, 60°, ...). Finding this moment requires bisection: repeatedly narrow
the time interval where `solar_longitude_sidereal()` crosses the boundary.

**Which civil day owns a sankranti?** This is the subtle part — each regional
calendar has its own "critical time" rule:

| Calendar | Critical time | Meaning |
|----------|--------------|---------|
| Tamil | Sunset − 9.5 min | If sankranti is before this, the current day starts the new month |
| Bengali | Midnight + 24 min (with tithi-based exceptions) | Complex rule from Sewell & Dikshit (1896) |
| Odia | 22:12 IST | Fixed cutoff, no astronomical computation |
| Malayalam | Sunrise + 3/5 × daytime − 9.5 min | End of madhyahna (midday period) |

If a sankranti happens before the critical time, the current civil day is the
first day of the new solar month. If after, the next day is. The 9.5-minute
buffers in Tamil and Malayalam compensate for the ~24" ayanamsa difference
between our Lahiri value and drikpanchang.com's.

**Implementation**: `src/solar.c` — `gregorian_to_solar()` computes the sidereal
solar longitude at the critical time for the given calendar type, determines the
rashi, and handles sankranti boundary detection.

### Year determination

**Lunisolar (Saka era)**: The Hindu new year begins with Chaitra (month 1,
typically March/April). The Saka year is derived from the Kali Ahargana (day count
from the Kali epoch, 3102 BCE):

```
ahar = JD − 588465.5  (days since Kali epoch)
kali_year = (ahar + (4 − masa) × 30) / 365.25636
saka_year = kali_year − 3179
```

**Solar (regional eras)**: Each regional calendar has its own era and year-start:

- Tamil (Saka): year starts at Mesha (rashi 1, ~April), offset = gregorian_year − 78
- Bengali (Bangabda): year starts at Mesha, offset = gregorian_year − 593
- Odia (Amli): year starts at Kanya (rashi 6, ~September), offset = gregorian_year − 592
- Malayalam (Kollam): year starts at Simha (rashi 5, ~August), offset = gregorian_year − 824

### Putting it all together: `gregorian_to_hindu()`

Given a Gregorian date, computing the full Hindu date requires:

```
1. Convert Gregorian → JD (Julian Date)
2. Compute sunrise at the given location          → calls solar position pipeline
3. Compute tithi at sunrise                       → calls Moon + Sun pipelines
4. Compute tithi at previous day's sunrise        → adhika tithi detection
5. Find bracketing new moons                      → inverse Lagrange on lunar_phase()
6. Compute solar rashi at each new moon           → sidereal solar longitude
7. Derive masa name and adhika flag               → rashi comparison
8. Compute Saka year                              → Kali Ahargana formula
```

For the solar calendars, `gregorian_to_solar()` is simpler:

```
1. Convert Gregorian → JD
2. Compute critical time (may need sunrise/sunset) → solar position pipeline
3. Compute sidereal solar longitude at critical time → VSOP87 + ayanamsa
4. Determine rashi (solar month)
5. Check for sankranti boundary                     → bisection
6. Compute regional year from era offset
```

Each step involves the astronomical pipelines described in this document. The total
cost per date: ~23 μs for lunisolar, ~4–32 μs for solar (depending on whether
sunrise/sunset is needed), dominated by the VSOP87 harmonic evaluation inside
`solar_position()`.

---

## Surya Siddhanta vs Drik Siddhanta

There are two fundamentally different ways to compute a Hindu calendar date. Our
implementation uses the modern approach (Drik Siddhanta). The classical alternative
is the Surya Siddhanta, implemented by Reingold and Dershowitz in *Calendrical
Calculations*. They compute the same *kind* of thing (tithi, masa, adhika) using the
same *definitions* — but the underlying astronomy is completely different, which
leads to dates that disagree by up to a full day.

### Surya Siddhanta (Reingold/Dershowitz)

The Surya Siddhanta is an ancient Indian astronomical text (~400 CE). It models
the Sun and Moon using a geocentric system with epicycles — small circles riding on
larger circles — a technique shared with Ptolemy's Greek astronomy but with distinct
Indian parameters.

The key features:

**1. Fixed mean motions.** The Sun and Moon have constant average speeds, defined as
exact rational numbers of revolutions per *mahayuga* (4,320,000 years):

```
Sun:  4,320,000 revolutions / mahayuga
Moon: 57,753,336 revolutions / mahayuga
```

The mean longitude at any time is just `(revolutions × elapsed_days / days_per_mahayuga) × 360°`.
No perturbation tables, no harmonic series — a single division and multiply.

**2. Epicyclic correction (equation of center).** The mean longitude is corrected by
a single epicyclic term — the "manda" (slow) correction — which accounts for the
elliptical shape of the orbit. This uses a Hindu sine table (24 entries at 3.75°
intervals, linearly interpolated) rather than the modern `sin()` function:

```
anomaly = mean_longitude - apogee
equation = arcsin(epicycle_size × hindu_sin(anomaly) / 360)
true_longitude = mean_longitude - equation
```

The epicycle sizes are fixed constants: 14° for the Sun, ~31.5° for the Moon. There
is no equivalent of the evection, variation, or the hundreds of other perturbation
terms in modern lunar theory.

**3. Fixed epoch and location.** All calculations are referenced to the Kali Yuga
epoch (midnight at Ujjain, 3102 BCE Jan 23) and the location is fixed at Ujjain
(23°9'N, 75°46'E). There is no notion of computing sunrise for an arbitrary location.

**4. Approximate sunrise.** Sunrise is estimated using a formula involving the
"earth-sine" and "day radius" — geometric calculations based on the Sun's
declination and the observer's latitude. No atmospheric refraction, no iterative
convergence, no distinction between upper limb and disc center.

The entire pipeline — mean longitude, epicyclic correction, sunrise — runs in a
handful of arithmetic operations. No transcendental functions except the Hindu sine
table lookup. The Reingold/Dershowitz Lisp implementation runs all 72 Hindu calendar
functions in pure arithmetic.

### Drik Siddhanta (our approach)

"Drik" means "observational" — the calendar should match what you actually see in
the sky. This means using modern astronomical ephemerides:

**1. Perturbation series.** Instead of a single epicyclic term, we sum 135 VSOP87
harmonic terms for the Sun (gravitational effects of all planets) and 181+ DE404
terms for the Moon (including evection, variation, annual equation, parallactic
inequality, and dozens of smaller effects). See the previous sections for details.

**2. Nutation and precession.** The Earth's axis wobbles (nutation, ~18.6-year
period, ±9" amplitude) and slowly drifts (precession, ~50"/year). Both affect
where the ecliptic reference frame points. The Surya Siddhanta ignores nutation
entirely and uses a fixed precession rate that has accumulated ~20° of error over
1,600 years.

**3. Lahiri ayanamsa.** The sidereal reference point (where the zodiac "starts") is
defined by the Indian government's standard: the Lahiri ayanamsa, tied to the star
Spica (Chitra) via IAU 1976 precession. The Surya Siddhanta uses its own precession
model that gives a significantly different ayanamsa — currently off by several degrees.

**4. Accurate sunrise.** We compute sunrise by iterating the Meeus algorithm: find
when the Sun's apparent altitude equals the refraction-corrected horizon angle,
accounting for the observer's exact latitude/longitude, atmospheric refraction
(Sinclair formula), and the Sun's semi-diameter (upper limb). This requires ~10
full solar position evaluations per sunrise.

**5. Location-dependent.** Every calculation uses the observer's actual coordinates.
A tithi that spans sunrise in Delhi might not span sunrise in New York. The same
Gregorian date can map to different Hindu dates at different locations.

### Where they disagree

Our validation compared both approaches across 55,152 days (1900–2050):

- **Tithi**: Reingold/Dershowitz `hindu-lunar-from-fixed` disagrees on ~10.8% of
  days. Most disagreements are ±1 tithi at sunrise boundaries — the Surya Siddhanta's
  approximate sunrise and single-epicycle Moon longitude place the transition a few
  minutes earlier or later than reality.

- **Masa**: Disagreements on ~0.7% of days, concentrated around new moon boundaries
  where the Surya Siddhanta's solar longitude error shifts which rashi the new moon
  falls in, changing the month name or the adhika flag.

- **Adhika months**: The Surya Siddhanta occasionally assigns adhika status to a
  different month than Drik Siddhanta, because its solar longitude (and thus rashi
  boundaries) can be off by up to a degree.

The disagreements are not bugs — they're the expected consequence of using 1,600-year-old
astronomical constants vs modern ephemerides. When we spot-checked disagreement dates
against drikpanchang.com (the authoritative modern panchang), our implementation matched
every time.

### Why Surya Siddhanta still matters

Despite the astronomical inaccuracies, the Surya Siddhanta is historically significant:

- **It defined the Hindu calendar for over a millennium.** Every panchang published
  before ~1950 used some variant of Surya Siddhanta calculations.
- **Some traditional panchang publishers still use it** (or a blend), which is why
  you occasionally see two panchangs disagree on whether today is Ekadashi or Dvadashi.
- **Reingold/Dershowitz made it computationally accessible.** Their implementation is
  a valuable reference for understanding the classical approach and for comparing
  against modern results.
- **The conceptual framework is the same.** Both approaches compute tithi as lunar-solar
  elongation ÷ 12°, both use the rashi-at-new-moon rule for month naming, both handle
  adhika/kshaya by the same definitions. The only difference is the precision of the
  underlying positional astronomy.

### Cost comparison

| | Surya Siddhanta | Drik Siddhanta |
|---|---|---|
| **Sun longitude** | 1 division + 1 sine lookup | 135-term VSOP87 series |
| **Moon longitude** | 1 division + 1 sine lookup | 181-term DE404 series |
| **Ayanamsa** | fixed formula (~1 multiply) | IAU 1976 3D precession |
| **Sunrise** | geometric formula (~5 ops) | 10-iteration Meeus convergence |
| **Per-date cost** | < 1 μs | 4–32 μs |
| **Accuracy** | ±1 day (vs observation) | ±2 seconds (sunrise), ±0.07" (Moon) |
| **External deps** | None | Ephemeris coefficients (~2 KB) |

The ~30× speed difference is irrelevant in practice (both are sub-millisecond), but
the accuracy difference matters: a 1-day error means celebrating a festival on the
wrong day.

---

## Summary of precision

| Quantity | Precision | Source |
|----------|-----------|--------|
| Solar longitude | ±1" | VSOP87 (135 terms) |
| Lunar longitude | ±0.07" | DE404 Moshier pipeline |
| Ayanamsa (Lahiri) | ±0.3" | IAU 1976 precession |
| Sidereal solar longitude | ±0.5" | Solar + ayanamsa combined |
| Nutation | ±0.5" | IAU 1980 (13 terms) |
| Sunrise/sunset | ±2 seconds | Meeus iteration + Sinclair refraction |
| Delta-T | ±0.5s (1900–2025) | IERS lookup table |
| Delta-T | ~seconds (2025–2050) | Polynomial extrapolation |

All sufficient for the Hindu calendar, where the critical precision threshold is
~1 minute (the smallest timing difference that can change a tithi or sankranti
assignment to a different civil day).
