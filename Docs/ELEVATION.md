# Elevation and Horizon Dip Investigation

## Background

The default location for this project is New Delhi (28.6139°N, 77.2090°E), which sits at **216 meters** above sea level on the Indo-Gangetic Plain. The original implementation used altitude=0, which was incorrect.

## The Problem

When altitude was changed from 0 to 216m, the sunrise calculation applied two adjustments:

1. **Atmospheric pressure correction** (minor): Lower pressure at altitude → slightly less refraction. At 216m, pressure drops from 1013.25 to ~988 hPa, shifting refraction by ~1-2 arcseconds.

2. **Horizon dip** (major): The formula `h₀ -= 0.0353 × √alt` adds a depression angle of **0.52°** (~31 arcminutes) for 216m. This shifted sunrise ~2.5 minutes earlier.

The result: lunisolar tithi mismatches against drikpanchang.com jumped from **16 to 111** (all `-1`, meaning our tithi was one less — sunrise was too early).

## Why Horizon Dip Is Wrong for Delhi

The dip-of-the-horizon formula models an observer who is elevated **above the surrounding terrain** — standing on a cliff overlooking the sea, or on a tower. From such a vantage point, the geometric horizon is depressed below the horizontal plane, so the sun becomes visible earlier.

New Delhi is on a flat plain at 216m elevation. The surrounding terrain is at the same elevation. An observer at ground level in Delhi sees a horizon that is **not depressed** — the visible horizon is at eye level, same as if they were at sea level.

The Swiss Ephemeris `calc_dip()` function (swecl.c:3150) explicitly documents this assumption:

> "refracted height of ocean if visible at horizon"

There is no ocean visible from Delhi.

## The Fix

**Keep altitude=216m** for the atmospheric pressure correction, but **do not apply horizon dip**. The `rise_set()` function in `moshier_rise.c` now:

- Adjusts atmospheric pressure using the barometric formula (standard atmosphere, lapse rate 0.0065 K/m)
- Uses the pressure-adjusted Sinclair refraction for h₀
- Does NOT add the dip term

```c
double atpress = 1013.25;
if (alt > 0)
    atpress = 1013.25 * pow(1.0 - 0.0065 * alt / 288.0, 5.255);
double h0 = -sinclair_refraction_horizon(atpress, 0.0);
h0 -= SOLAR_SEMIDIAM_ARCMIN / 60.0;  /* upper limb */
/* No dip: surrounding terrain is at the same elevation */
```

## Why We Didn't Notice Before

With altitude=0, the dip formula evaluates to `0.0353 × √0 = 0` — a no-op. The formula was present in the code but had no effect. It only became apparent when the altitude was corrected to 216m.

## Results

| Configuration | Drikpanchang mismatches | Match rate |
|---------------|------------------------|------------|
| Altitude=0 (original) | 16 | 99.971% |
| Altitude=216, with dip | 111 | 99.799% |
| **Altitude=216, pressure only** | **15** | **99.973%** |

The pressure-only adjustment at 216m actually **improved** the match rate by one — the tiny refraction change nudged one boundary case (1919-10-13) to the correct side.

## Independent Confirmation from Drikpanchang

The no-dip conclusion above was reached empirically — applying dip blew the
mismatch count from 16 to 111, so we removed it. Drikpanchang's own
configuration independently agrees.

A scrape request returns the site's default settings as cookies, and among
them (verified 2026-07-27):

```
drik-geo-elevation-status = disabled
drik-sunrise-type         = edges
```

`drik-geo-elevation-status=disabled` means drikpanchang does not apply a
geometric elevation correction to the horizon by default — the same choice we
made for the opposite-seeming reason (we kept altitude for pressure but dropped
the dip term). `drik-sunrise-type=edges` separately confirms the upper-limb
sunrise convention documented in
[DRIKPANCHANG_VALIDATION.md](DRIKPANCHANG_VALIDATION.md).

This does not change any code — it explains *why* the empirical result came out
the way it did, and means the choice is a match to the reference implementation
rather than a fitted parameter.

## When Dip SHOULD Be Applied

The horizon dip is physically appropriate when the observer is genuinely elevated above the surrounding terrain:

- Observer on a hilltop or mountain overlooking a valley
- Observer on a coastal cliff overlooking the sea
- Observer in a tall building with an unobstructed view to a distant, lower horizon

For general-purpose panchang calculations at a city location, the altitude field represents elevation above sea level (for pressure), not height above local terrain.

## Impact on Solar Calendars

The solar calendar critical times depend on sunset (Tamil) and madhyahna (Malayalam), both of which use sunrise/sunset. The pressure-only correction has negligible effect on these (~1-2 arcseconds), so all solar calendar results are unchanged:

- Tamil: 1,811/1,811 (100%)
- Bengali: 1,811/1,811 (100%)
- Odia: 1,811/1,811 (100%)
- Malayalam: 1,811/1,811 (100%)

The Tamil (-9.5 min) and Malayalam (-9.5 min) empirical buffers were NOT compensating for a missing altitude correction. They compensate for the ~24 arcsecond difference between our Lahiri ayanamsa and drikpanchang.com's Lahiri ayanamsa.

## Swiss Ephemeris Comparison

The SE `calc_dip()` function uses a more rigorous formula (Thom 1973):

```
dip = -180/π × arccos(1/(1 + alt/R_earth)) × sqrt(d)
```

where `d` accounts for atmospheric refraction variation with altitude. For 216m, SE gives 0.512° vs our simplified formula's 0.520° — nearly identical, and both equally inappropriate for a flat-terrain location.

Our SE backend in `astro.c` passes `loc->altitude` to `swe_rise_trans()`, which also applies dip internally. With altitude=0, both backends were consistent (no dip). The Moshier backend now correctly avoids dip; if using the SE backend with altitude>0, be aware that SE will still apply dip.
