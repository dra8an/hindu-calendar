# Odia Solar Calendar — Critical Time Adjustment

## Problem

The Odia solar calendar determines month boundaries by sankranti — the exact moment the sun enters a new sidereal zodiac sign (rashi). When a sankranti occurs during a given civil day, a "critical time" rule decides whether that day is the first day of the new month or the last day of the old month.

Our initial implementation used **end of civil day (midnight)** as the critical time. This produced incorrect results for sankrantis falling in the late evening (roughly 22:00–00:00 IST).

Example: July 16, 2026 — the Karka sankranti occurs at 23:35 IST. The midnight rule assigned it to the current day, but drikpanchang.com shows it as **Ashadha 32** (last day), with Shravana 1 on July 17.

## Investigation

### Stage 1: Midnight rule

Initial implementation: if the sankranti falls before midnight (00:00 IST of the next civil day), assign to the current day. This failed for multiple late-evening sankrantis that drikpanchang assigns to the next day.

### Stage 2: Apparent midnight (nishita) hypothesis

We hypothesized the cutoff might be **apparent midnight** — the midpoint between sunset and the next day's sunrise, which varies by season. Built diagnostic tool `tools/odia_nishita.c` to test this.

Result: failed. The distance from apparent midnight to the sankranti did not consistently predict the assignment. Cases at the same distance from apparent midnight received different assignments on drikpanchang.

### Stage 3: Fixed offset before apparent midnight

We tried a fixed offset before apparent midnight (2 hours 2 minutes), based on initial boundary cases. Built `tools/odia_cutoff_scan.c` to scan all sankrantis in the 21:30–22:30 IST range.

Result: failed. When we verified 12 new cases near this cutoff against drikpanchang, 7 mismatched. The critical pair: a case at 2h01m38s before apparent midnight was "current" on drikpanchang, while a case at 2h06m32s was "next". No single before-apparent-midnight offset could separate both correctly.

### Stage 4: Fixed IST cutoff discovery

We plotted all verified cases by their IST time and found a clean separation:

- All sankrantis at **22:11 IST or earlier** → current day (day 1 of new month)
- All sankrantis at **22:12 IST or later** → next day (last day of old month)

This holds for all 35 verified boundary cases spanning 1900–2050, across all seasons and day lengths. Unlike the apparent-midnight model, the fixed IST cutoff has zero inconsistencies.

## Why fixed IST works for Odia but not other calendars

Tamil uses sunset (season-dependent), Malayalam uses 3/5 of daytime (season-dependent), but Odia uses a fixed clock time. This is consistent with IST being the civil standard in Odisha. The 22:12 IST cutoff likely represents a traditional rule expressed in clock time rather than astronomical time.

## Solution

Changed the Odia critical time from midnight to fixed 22:12 IST:

```c
// Before (midnight):
return jd_midnight_ut - loc->utc_offset / 24.0;

// After (22:12 IST = 16:42 UTC):
return jd_midnight_ut + 16.7 / 24.0;
```

This change is in `src/solar.c`, function `critical_time_jd()`, case `SOLAR_CAL_ODIA`.

## Verified Boundary Cases

35 boundary cases in `tests/test_solar.c`, all verified against drikpanchang.com. Grouped by distance from the 22:12 cutoff:

**Tightest cases (within 3 minutes of cutoff):**

| Date | Sankranti IST | Offset from 22:12 | Drikpanchang |
|------|--------------|-------------------|--------------|
| 1915-04-13 | 22:11:18 | −42 seconds | Baisakha 1 (current) |
| 1946-12-15 | 22:08:53 | −3m07s | Pausha 1 (current) |
| 1957-01-13 | 22:09:03 | −2m57s | Magha 1 (current) |
| 1918-01-13 | 22:09:30 | −2m30s | Magha 1 (current) |
| 1974-05-14 | 22:09:42 | −2m18s | Jyeshtha 1 (current) |
| 1907-12-15 | 22:12:24 | +24 seconds | Margashirsha 30 (next) |
| 2040-09-16 | 22:14:02 | +2m02s | Bhadrapada 31 (next) |
| 1971-03-14 | 22:14:36 | +2m36s | Phalguna 30 (next) |
| 2042-11-16 | 22:15:11 | +3m11s | Kartika 30 (next) |

The tightest case is 1915-04-13 at 22:11:18 IST — only 42 seconds before the cutoff, correctly assigned to the current day.

**All 35 cases confirm:** ≤22:11 = current day, ≥22:12 = next day. Zero exceptions.

## Diagnostic Tools

Five investigation tools were built during the process (all in `tools/`):

- `odia_diag.c` — prints all 12 sankrantis for a year with IST times and multiple rule comparisons
- `odia_boundary.c` — scans sankrantis 1900–2050 in the 18:00–02:00 IST range
- `odia_midnight_scan.c` — scans sankrantis near midnight, compares midnight vs sunset rules
- `odia_nishita.c` — tests apparent midnight hypothesis against 11 confirmed cases
- `odia_cutoff_scan.c` — scans 21:30–22:30 IST range, computes distance to apparent midnight
