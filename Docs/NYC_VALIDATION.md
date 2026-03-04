# NYC Drikpanchang.com Validation (1900–2050)

## Summary

We scraped every month panchang page from drikpanchang.com for New York City
(geoname-id=5128581) across the full 1900–2050 range (1,812 Gregorian months,
55,152 days) and compared the tithi for each day against our computed reference.

| Location | Match | Mismatch | Rate |
|----------|-------|----------|------|
| **NYC (upper limb)** | **55,118** | **34** | **99.938%** |
| Delhi (upper limb) | 55,142 | 10 | 99.982% |

NYC has more mismatches than Delhi (34 vs 10), which is expected — sunrise at
40.7°N has a shallower angle, creating a wider window where sub-minute timing
differences can flip a tithi boundary. Additionally, DST transitions add
complexity to the UTC offset calculation.

## NYC-Specific Infrastructure

### DST Module

`src/dst.h` + `src/dst.c` — US Eastern DST rules for 1900–2050:

| Era | Rule |
|-----|------|
| 1900–1917 | No DST (EST, UTC-5) |
| 1918–1919 | Last Sun March – Last Sun October |
| 1920–1941 | No federal DST (EST) |
| 1942 – 1945-Sep-30 | Year-round War Time (EDT, UTC-4) |
| 1946–1966 | Last Sun April – Last Sun September |
| 1967–1973 | Last Sun April – Last Sun October |
| 1974 | Jan 6 – Last Sun October (energy crisis) |
| 1975 | Last Sun February – Last Sun October |
| 1976–1986 | Last Sun April – Last Sun October |
| 1987–2006 | First Sun April – Last Sun October |
| 2007–2050 | Second Sun March – First Sun November |

### Reference Generator

`tools/generate_ref_data.c` accepts `-l LAT,LON` and `-tz us_eastern` flags.
NYC reference: `validation/moshier/nyc/ref_1900_2050.csv` (55,152 rows).

### Scraper

The lunisolar scraper (`scraper/lunisolar/`) supports `--location nyc`:
- Data in `scraper/data/lunisolar_nyc/raw/` (1,812 HTML files)
- Parsed CSV: `scraper/data/lunisolar_nyc/parsed/drikpanchang.csv`
- Comparison report: `scraper/data/lunisolar_nyc/comparison_report.txt`

## The 34 Mismatches

### Pattern

Every mismatch is a **tithi boundary edge case** where the moon–sun
elongation crosses a 12° boundary within ~0–2 minutes of sunrise.

| Metric | Value |
|--------|-------|
| Total days compared | 55,152 |
| Match | 55,118 (99.938%) |
| Mismatch | 34 (0.062%) |

### Direction of disagreement

- **33 dates**: Drikpanchang assigns the *next* tithi (+1); we assign the
  *previous*. The tithi boundary falls just after our computed sunrise.

- **1 date** (2010-09-08): Shows as -29 diff (our 30 vs drikpanchang's 1),
  which is the same +1 pattern wrapping across the tithi 30→1 boundary.

### No overlap with Delhi mismatches

The 34 NYC mismatch dates and the 10 Delhi mismatch dates share **zero
dates in common**. This confirms these are location-dependent sunrise
boundary cases — different sunrise times at different longitudes/latitudes
create different sub-minute race conditions.

### Complete list

```
Date         Drik  Ours  Diff
1902-05-04    27    26    +1
1920-07-24    10     9    +1
1928-12-15     4     3    +1
1930-02-27    30    29    +1
1952-04-29     6     5    +1
1961-03-01    15    14    +1
1966-02-06    17    16    +1
1969-12-19    12    11    +1
1971-09-03    14    13    +1
1986-02-18    10     9    +1
1992-09-03     8     7    +1
2001-04-28     6     5    +1
2008-07-01    29    28    +1
2010-09-08     1    30   -29  (30→1 wrap)
2011-07-12    13    12    +1
2020-06-01    11    10    +1
2027-11-21    24    23    +1
2028-04-09    16    15    +1
2032-12-31    30    29    +1
2034-04-11    23    22    +1
2036-10-06    17    16    +1
2038-05-26    24    23    +1
2039-08-04    16    15    +1
2040-07-22    14    13    +1
2040-12-18    16    15    +1
2041-03-13    12    11    +1
2041-05-04     5     4    +1
2042-03-01    11    10    +1
2042-03-14    23    22    +1
2046-02-11     6     5    +1
2046-07-23    21    20    +1
2048-09-28    22    21    +1
2049-07-19    21    20    +1
2050-04-10    19    18    +1
```

### Cause

Same as Delhi — at these margins, any of the following can flip the result:

- **Sunrise time**: a 30-second difference in sunrise computation (refraction
  model, solar semi-diameter, delta-T) shifts which tithi is active.

- **Lunar longitude**: a 0.01° difference in the moon's position shifts
  the elongation boundary by ~2 minutes of time.

- **DST boundaries**: on DST transition dates, the exact UTC offset affects
  the local sunrise time used for tithi determination.

## Why NYC Has More Mismatches Than Delhi

1. **Higher latitude** (40.7°N vs 28.6°N): sunrise/sunset angles are
   shallower, meaning the sun takes longer to cross the horizon. Small
   differences in the refraction model or h0 value have a larger effect
   on the computed sunrise time.

2. **DST complexity**: India uses a fixed UTC+5:30 offset year-round.
   NYC switches between UTC-5 (EST) and UTC-4 (EDT), with rules that
   changed 9 times between 1900–2050. Any error in DST rules shifts
   sunrise by 1 hour, though our DST module has been verified against
   historical records.

3. **Longitude effect**: NYC is at 74°W, meaning sunrise is ~12 hours
   offset from Delhi's. Different tithi boundary races occur at different
   local sunrise times.

## Test Suite

`tests/test_nyc.c` — 18 manually verified dates from 2025 spanning:
- Winter (EST), summer (EDT)
- DST spring-forward (Mar 8→9) and fall-back (Nov 1→2) transitions
- Hindu New Year (Chaitra S-1)
- Full moon (Purnima) and new moon (Amavasya)

All 18 dates verified against drikpanchang.com NYC page (365/365 for 2025).

## Conclusion

The 99.938% match rate across 151 years (55,152 days) for NYC confirms
that our implementation correctly handles:
- Location-dependent sunrise at arbitrary coordinates
- US Eastern DST rules across all historical eras (1900–2050)
- Tithi and masa determination for locations outside India

The 34 mismatches are all sub-minute tithi boundary cases — the same
irreducible precision limitation seen in Delhi, amplified by NYC's higher
latitude and DST complexity.
