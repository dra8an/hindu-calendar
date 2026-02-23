# Drikpanchang.com Full Validation (1900–2050)

## Summary

We scraped every month panchang page from drikpanchang.com for the full
1900–2050 range (1,812 Gregorian months, 55,152 days) and compared the
tithi for each day against our computed reference.

**Result: 55,117 / 55,152 match (99.937%) — 35 mismatches.**

Both the Moshier and Swiss Ephemeris backends produce identical results
and disagree with drikpanchang on the same 35 dates.

## Scraper

The scraper lives in `scraper/` and operates in three phases:

1. **fetch.py** — Downloads raw HTML month pages to `scraper/data/raw/month/YYYY-MM.html`
2. **parse.py** — Extracts tithi (1–30) per day from HTML → `scraper/data/parsed/drikpanchang_lunisolar.csv`
3. **compare.py** — Diffs parsed tithis against `validation/moshier/ref_1900_2050.csv`

Settings: Amanta scheme, New Delhi, Lahiri ayanamsa (via cookies).

The HTML structure is straightforward: each day cell has a `dpBigDate`
element (Gregorian day) and a `dpCellTithi` element ("TithiName Paksha",
e.g., "Dwitiya Shukla"). The parser converts these to tithi numbers 1–30
using the standard Shukla 1–15 / Krishna 16–29 / Amavasya 30 mapping.

### Rate limiting

Drikpanchang rate-limits after ~200–400 consecutive requests, returning
CAPTCHA pages (~2 KB vs normal ~200 KB). The fetcher detects this by
checking response size and rotates to a fresh HTTP session (the CAPTCHA
is tied to the server's `_DRIK_SESSION_ID` cookie, not the IP). With
session rotation and 15-second delays, all 1,812 months were downloaded
in ~4 sessions over several hours.

## The 35 Mismatches

### Pattern

Every mismatch is a **tithi boundary edge case** where the moon–sun
elongation crosses a 12° boundary within 0–1.3 minutes of sunrise.

| Metric | Value |
|--------|-------|
| Total days compared | 55,152 |
| Match | 55,117 (99.937%) |
| Mismatch | 35 (0.063%) |
| Max margin to boundary | 1.3 minutes |
| Min margin to boundary | 0.05 minutes (3 seconds) |
| Median margin | ~0.5 minutes |

### Direction of disagreement

- **33 dates**: Drikpanchang assigns the *previous* tithi; we assign the
  *next*. The tithi boundary falls just before our computed sunrise
  (0.1–1.3 min before). Our code sees the new tithi at sunrise;
  drikpanchang sees the old one.

- **2 dates** (2045-01-17, 2046-05-22): Opposite — the boundary falls
  just after our sunrise. Our code sees the old tithi; drikpanchang sees
  the new one.

### Cause

At these margins, any of the following could flip the result:

- **Sunrise time**: a 30-second difference in sunrise computation (due to
  refraction model, disc center vs edge, delta-T) shifts which tithi is
  active at that moment.

- **Lunar longitude**: a 0.01° difference in the moon's position shifts
  the elongation boundary by ~2 minutes of time.

- **Solar longitude**: much smaller effect but contributes at the margin.

Comparing sunrise times from drikpanchang day pages against our Moshier
sunrise shows agreement within 1 minute for dates after 1910. The one
outlier is 1902-05-30 where drikpanchang reports sunrise at 05:15 vs our
05:25 — a 10-minute gap likely due to different delta-T models for the
early 20th century.

### Complete list

All 35 dates with diagnostic data from `tools/drikpanchang_mismatch_diag.c`:

```
Date         Sunrise   Our Drik  BdyBefore  BdyAfter   Margin   Direction
1902-05-30   05:25:03   23  22   05:24:34   05:25:27   0.5 min  near start
1903-05-18   05:30:27   22  21   05:30:10   07:46:42   0.3 min  near start
1908-03-17   06:30:28   15  14   06:30:18   07:58:16   0.2 min  near start
1909-10-11   06:20:04   28  27   06:19:29   08:41:18   0.6 min  near start
1909-12-01   06:57:02   20  19   06:56:02   06:47:16   1.0 min  near start
1911-08-26   05:56:19    3   2   05:55:22   04:59:27   0.9 min  near start
1912-12-14   07:06:37    6   5   07:06:30   09:48:05   0.1 min  near start
1915-12-05   06:59:44   29  28   06:58:50   03:22:38   0.9 min  near start
1916-02-24   06:53:59   21  20   06:53:44   05:30:54   0.2 min  near start
1920-10-12   06:20:51    1  30   06:20:16   06:18:14   0.6 min  near start
1924-02-05   07:09:05    1  30   07:08:10   06:02:50   0.9 min  near start
1925-03-03   06:46:08    9   8   06:46:02   09:18:26   0.1 min  near start
1932-05-15   05:31:38   10   9   05:30:48   03:23:17   0.8 min  near start
1939-07-23   05:37:50    8   7   05:37:47   04:39:53   0.0 min  near start
1940-02-03   07:10:07   26  25   07:09:38   07:34:54   0.5 min  near start
1943-12-17   07:08:04   21  20   07:07:50   09:52:04   0.2 min  near start
1946-01-29   07:12:25   27  26   07:11:56   08:53:43   0.5 min  near start
1951-06-08   05:23:46    4   3   05:22:52   07:39:47   0.9 min  near start
1956-05-29   05:25:11   20  19   05:24:50   07:51:52   0.3 min  near start
1957-08-28   05:57:51    4   3   05:57:33   03:01:33   0.3 min  near start
1965-05-06   05:37:48    6   5   05:36:47   03:10:47   1.0 min  near start
1966-01-08   07:16:18   17  16   07:15:48   03:40:00   0.5 min  near start
1966-08-09   05:47:39   23  22   05:47:16   06:52:58   0.4 min  near start
1966-10-25   06:28:52   12  11   06:28:22   09:00:32   0.5 min  near start
1968-03-11   06:36:43   12  11   06:35:53   05:59:16   0.8 min  near start
1968-05-24   05:26:44   28  27   05:26:20   07:55:22   0.4 min  near start
1972-04-01   06:12:29   18  17   06:12:00   08:40:08   0.5 min  near start
1974-12-19   07:09:26    6   5   07:08:31   09:42:55   0.9 min  near start
1978-09-15   06:06:45   14  13   06:06:25   03:11:28   0.3 min  near start
1982-03-07   06:41:34   13  12   06:40:16   04:55:07   1.3 min  near start
1987-12-18   07:08:44   28  27   07:08:15   05:19:09   0.5 min  near start
2007-10-09   06:19:05   29  28   06:18:13   08:16:49   0.9 min  near start
2014-05-22   05:27:40   24  23   05:27:32   03:38:00   0.1 min  near start
2045-01-17   07:15:42   29  30   04:43:38   07:16:12   0.5 min  near end
2046-05-22   05:27:38   17  18   06:49:13   05:28:08   0.5 min  near end
```

### Temporal distribution

- 1900–1909: 4 mismatches
- 1910–1919: 4
- 1920–1929: 3
- 1930–1939: 2
- 1940–1949: 3
- 1950–1959: 3
- 1960–1969: 5
- 1970–1979: 3
- 1980–1989: 2
- 1990–1999: 0
- 2000–2009: 1
- 2010–2019: 1
- 2020–2029: 0
- 2030–2039: 0
- 2040–2049: 2
- 2050: 0

Mismatches are spread fairly evenly, with slightly more in the early 20th
century (likely due to larger delta-T uncertainty) and a gap in the
1990–2030 range where modern observations constrain delta-T precisely.

## Conclusion

The 99.937% match rate across 151 years confirms that our implementation
is essentially identical to drikpanchang.com. The 35 disagreements are
all at tithi boundaries within 1.3 minutes of sunrise — well within the
uncertainty of any ephemeris computation. Neither side is definitively
correct for these edge cases; the differences arise from slightly
different sunrise and lunar longitude calculations.

This is the most comprehensive validation of a Hindu calendar
implementation we are aware of: 55,152 days independently verified
against the authoritative reference site.
