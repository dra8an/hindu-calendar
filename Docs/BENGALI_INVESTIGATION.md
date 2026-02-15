# Bengali Solar Calendar — Critical Time Investigation

## Problem

The Bengali solar calendar has 23 out of 100 edge case entries that disagree with drikpanchang.com. Unlike Tamil (clean 7.7–8.7 min gap) and Malayalam (clean 9.3–10.0 min gap), the Bengali pattern has no obvious delta threshold — wrong and correct entries are interleaved across a wide range of deltas.

## What We Know

### Current rule: IST midnight + 24 minutes (00:24 IST)

From the Reingold/Dershowitz book, Bengali uses a "midnight" rule with a special edge-case zone between 11:36 PM and 12:24 AM. Our implementation uses 00:24 IST as the critical time (= IST midnight + 24 min buffer).

### Edge case results (from v0.3.2 scan)

- 100 closest sankrantis to 00:24 IST scanned (1900–2050)
- All positive delta entries: correct
- Negative delta entries: mixed — some correct, some wrong, no clean threshold
- After delta = −22.9 min, all remaining entries have positive delta (weird asymmetry)
- 23 entries disagree with drikpanchang.com

### Verified entries (37 total)

23 wrong (W = our code says day 1, drikpanchang says last day of old month):
```
1908-05-14(ra2), 1976-10-17(ra7), 2022-03-15(ra12), 1983-03-15(ra12),
2047-01-15(ra10), 1935-08-17(ra5), 1937-10-17(ra7), 1944-03-14(ra12),
2015-10-18(ra7), 1935-09-17(ra6), 1981-06-15(ra3), 1903-06-15(ra3),
1969-01-14(ra10), 1930-01-14(ra10), 1986-05-15(ra2), 1976-11-16(ra8),
1924-02-13(ra11), 2036-12-16(ra9), 2008-01-15(ra10), 2044-04-14(ra1),
2013-08-17(ra5), 1974-08-17(ra5), 1937-11-16(ra8)
```

14 correct (C = both our code and drikpanchang agree on day 1):
```
1963-02-13(ra11), 1909-07-16(ra4), 1947-05-15(ra2), 1927-04-14(ra1),
1905-03-14(ra12), 1991-07-17(ra4), 2005-04-14(ra1), 2025-05-15(ra2),
1942-06-15(ra3), 2030-07-17(ra4), 1997-12-16(ra9), 2015-11-17(ra8),
2020-06-15(ra3), 1948-07-16(ra4)
```

All 37 re-verified against drikpanchang.com — 100% confirmed.

## Hypotheses

### H1: Location — Kolkata instead of Delhi ❌ RULED OUT

Bengali calendar is for West Bengal. Kolkata nishita scores 23/37 (23W, 0C) — trivially classifies everything as W. No buffer helps.

### H2: Different buffer value ❌ RULED OUT

Swept IST midnight buffers from -10 to +40 min. Best score: 23/37 (trivial).
With ayanamsa-shifted times: still max 23/37 for any fixed IST cutoff.

### H3: Sunrise-based rule ❌ RULED OUT

All sankrantis are around midnight (00:10–00:34 IST). Sunrise is 05:30–06:30 IST. All would be assigned the same way — can't distinguish W from C.

### H4: Ayanamsa offset + midnight rule ❌ RULED OUT

Measured exact ayanamsa difference from 5 drikpanchang data points:
```
1908-05-14: SE_SIDM_LAHIRI=22.577312  drikpanchang=22.583950  diff=+23.90"
1948-07-16: SE_SIDM_LAHIRI=23.138298  drikpanchang=23.144971  diff=+24.02"
1963-02-13: SE_SIDM_LAHIRI=23.341912  drikpanchang=23.348599  diff=+24.07"
1976-10-17: SE_SIDM_LAHIRI=23.532918  drikpanchang=23.539616  diff=+24.11"
2022-03-15: SE_SIDM_LAHIRI=24.167208  drikpanchang=24.173941  diff=+24.24"
```

Model: da(year) = 24.10 + 0.003*(year-2000) arcsec → ~10 min later sankranti times for drikpanchang.

Applied this shift to all 37 entries and tested against every cutoff:
- Fixed IST cutoffs: max 23/37 (trivial)
- Nishita Delhi + shift: 22/37 (15 failures)
- Nishita + buffer at any location: max 23/37
- **No combination reaches 24/37**

### H5: Apparent midnight (nishita) ❌ RULED OUT

Nishita at Delhi: 22/37 (with shifted times). Varies by ~30 min across the year but doesn't help — W and C entries are interleaved at every seasonal nishita.

### H5b: Equation of Time midnight ❌ RULED OUT

R/D Lisp code computes "midnight" using equation of time (EoT), not nishita. Tested EoT midnight at IST, Ujjain, Delhi, Kolkata longitudes:
- EoT midnight IST: 20/37 (best without shift), 23/37 (with shift, trivial)
- EoT ≈ nishita at same location (±0.2 min difference)
- No location or buffer achieves separation

## Definitive Conclusion: The Data Is Inseparable

### Proof by contradiction within rashis

**Rashi 8 (Vrischika, November)** with shifted dp_IST times:
- 1937-11-16 W at 00:10:57
- 2015-11-17 C at 00:14:00
- 1976-11-16 W at 00:17:39

This is **W–C–W ordering** — impossible to separate with any single cutoff.

**Rashi 2 (Vrishabha, May)** with shifted dp_IST times:
- 1986-05-15 W at 00:18:50
- 2025-05-15 C at 00:21:53
- 1947-05-15 C at 00:28:02
- 1908-05-14 W at 00:33:42

W at 00:18:50 < C at 00:21:53 → any cutoff ≤ 00:18:50 fails the C entry at 00:28:02.

### Implications

drikpanchang's Bengali calendar **cannot** be explained by any rule of the form:
> "if sankranti time [before|after] [fixed cutoff | nishita | EoT midnight | mean midnight] at [any location] ± [any buffer], assign to [current|next] day"

This was tested exhaustively:
- 51 fixed IST cutoffs (from -10 to +40 min, 1-min steps)
- 4 locations (IST, Delhi, Kolkata, Ujjain)
- 3 midnight variants (mean, nishita, EoT)
- ±30 min buffer sweep on each
- With and without ayanamsa shift (~24 arcsec, ~10 min)

**Maximum non-trivial score: 22/37** (Nishita Delhi with ayanamsa shift).

## Possible Explanations

1. **Published almanac**: Bengali calendar may follow Vishuddha Siddhanta Panjika (published by the University of Calcutta) which uses its own traditional computational methods that don't map to a simple modern astronomical formula.

2. **More complex algorithm**: drikpanchang may use a multi-factor decision rule that considers aspects we haven't tested (planetary positions, specific traditional texts, etc.).

3. **drikpanchang inconsistency**: Unlikely for 23 entries, but possible that their Bengali implementation has edge case bugs or uses a lookup table rather than algorithmic computation.

## Current Status

**Decision needed**: Whether to:
- Accept the 23/100 error rate for Bengali (77% accuracy in edge cases, ~99% overall)
- Continue investigating (e.g., try to obtain the Vishuddha Siddhanta Panjika's algorithm)
- Use a different approach for Bengali (e.g., accept drikpanchang's results as authoritative and hardcode corrections)

## Files

| File | Purpose |
|------|---------|
| `tools/bengali_diag.c` | Diagnostic tool — 100 edge cases with multiple candidate rules |
| `tools/bengali_analysis.c` | Analysis of 37 verified entries against nishita/midnight variants |
| `tools/bengali_eot.c` | Equation of time midnight analysis |
| `tools/bengali_ayanamsa.c` | Ayanamsa comparison with drikpanchang |
| `tools/bengali_shifted.c` | Shifted sankranti (with ayanamsa correction) vs all cutoff rules |
| `validation/solar_edge_cases.csv` | Original edge case scan (Bengali section) |
| `tests/test_solar_edge.c` | Edge case tests (Bengali entries not yet corrected) |
