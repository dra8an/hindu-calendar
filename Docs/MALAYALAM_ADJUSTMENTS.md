# Malayalam Solar Calendar — Critical Time Adjustment

## Problem

The Malayalam solar calendar determines month boundaries by sankranti — the exact moment the sun enters a new sidereal zodiac sign (rashi). When a sankranti occurs during a given civil day, a "critical time" rule decides whether that day is the first day of the new month or the last day of the old month.

Our initial implementation used **apparent noon** (midpoint of sunrise and sunset) as the critical time. This produced incorrect results for dates where the sankranti fell between noon and approximately 1:45 PM IST.

Example: June 15, 2026 — the Mithuna sankranti occurs at 12:49:56 IST, which is 28 minutes after apparent noon (12:21:39 IST). Our noon rule assigned it to the next day, outputting **Edavam 32**. Drikpanchang.com shows **Mithunam 1**.

## Investigation

We identified 33 dates where a sankranti falls close to the critical time boundary and manually verified each one against drikpanchang.com. The investigation proceeded in stages:

### Stage 1: Fixed 13:12 IST hypothesis

We tested whether a fixed IST time (1:12 PM) could serve as the cutoff. Found 12 dates closest to 13:12 IST (6 before, 6 after) and verified them. Result: 10 of 12 matched, but two November dates (2021-11-16 and 1982-11-16) at ~13:09 IST were "last day" on drikpanchang while non-November dates at similar IST times were "day 1". This ruled out a fixed IST cutoff — the boundary is season-dependent (day-length-dependent).

### Stage 2: End of madhyahna (3/5 of daytime) hypothesis

The traditional Hindu 5-fold division of daytime splits sunrise-to-sunset into equal parts:

1. Pratahkala (early morning)
2. Sangava (forenoon)
3. Madhyahna (midday)
4. Aparahna (afternoon)
5. Sayahna (evening)

The end of madhyahna = sunrise + 3/5 × (sunset − sunrise). We tested this against all 13 previously verified dates. It correctly assigned 11 of 13; the two November cases were within 1 minute of the cutoff.

### Stage 3: Boundary scanning

We scanned all 1,812 Malayalam sankrantis (1900–2050) and found 74 that fall within 30 minutes of the 3/5 cutoff. We then verified three batches of dates against drikpanchang.com:

- **Batch 2** (12 dates, 0–5 min from cutoff): All showed "last day" on drikpanchang, meaning the 3/5 cutoff was slightly too late.
- **Batch 3** (8 dates, 5–15 min from cutoff): 6 showed "last day", 2 showed "day 1". The flip from "last day" to "day 1" occurs between −9.3 min and −10.0 min from the 3/5 mark.

### Stage 4: Fraction-of-daytime analysis

We computed the exact fraction of daytime (sunrise-to-sunset) at which each sankranti occurs and sorted all 33 verified cases:

```
Fraction   Drikpanchang
0.534      day 1 (current)
0.554      day 1
...
0.577      day 1
0.580      day 1
0.587      next (last day)    ← inconsistent pair:
0.588      day 1 (current)    ← no fixed fraction separates these two
0.589      next
0.593      next
...
0.600      next
0.604      next
0.607      next
```

The key finding: **no single fixed fraction perfectly separates all cases**. Specifically, 1973-02-12 (fraction 0.586, February) is "next" while 2035-05-15 (fraction 0.588, May) is "day 1". These are 0.002 apart in fraction but on opposite sides of the boundary.

## Root Cause

The inconsistency is explained by a ~10 minute systematic offset between our sankranti times and drikpanchang's. This offset corresponds to approximately 24 arcseconds of difference in the Lahiri ayanamsa value. Our code uses Swiss Ephemeris SE_SIDM_LAHIRI; drikpanchang likely uses a slightly different Lahiri computation (possibly the Indian Astronomical Ephemeris value).

Since sankranti is defined as the moment the sidereal solar longitude crosses a multiple of 30 degrees, even a tiny ayanamsa difference shifts the sankranti time. At the sun's typical speed of ~1 degree/day, 24 arcseconds ≈ 10 minutes.

This means: the rule IS 3/5 of daytime, but drikpanchang's sankranti times are ~10 minutes later than ours for every date. Cases well before or well after the cutoff match perfectly. Cases within ~10 minutes of the cutoff may disagree.

## Solution (v0.3.1)

Changed the Malayalam critical time from apparent noon to 3/5 of daytime:

```c
// Before (apparent noon):
return (sr + ss) / 2.0;

// After (end of madhyahna):
return sr + 0.6 * (ss - sr);
```

This change is in `src/solar.c`, function `critical_time_jd()`, case `SOLAR_CAL_MALAYALAM`.

## Ayanamsa Buffer Fix (v0.3.2)

In v0.3.2, a comprehensive edge case scan of all 1,812 Malayalam sankrantis (1900–2050) found 100 cases closest to the madhyahna cutoff. Manual verification against drikpanchang.com revealed:

- **Delta > 0** (sankranti after critical time): all correct
- **Delta ≤ −10.0 min**: all correct
- **0 > Delta ≥ −9.3 min**: all 15 entries wrong (our code shows day 1 of new month, drikpanchang shows last day of previous month)

The 9.3–10.0 min gap corresponds exactly to the ~24 arcsecond Lahiri ayanamsa difference between SE_SIDM_LAHIRI and drikpanchang.com. Applied a −9.5 min buffer to the critical time:

```c
// v0.3.2: end of madhyahna minus ayanamsa buffer
return sr + 0.6 * (ss - sr) - 9.5 / (24.0 * 60.0);
```

This splits the 9.3–10.0 min gap cleanly and fixes all 15 boundary dates. The same approach was applied to Tamil (−8.0 min buffer from sunset).

## Validation Results

- **33 manually verified boundary cases** from the initial investigation (v0.3.1)
- **100 edge case assertions** from the comprehensive scan (v0.3.2), with 15 corrected entries
- All 33 original data points stored in `validation/malayalam_boundary_cases.csv`
- Edge case test data in `tests/test_solar_edge.c`

Total test suite: 53,143 assertions across 10 suites, all passing.
