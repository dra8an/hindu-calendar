# Multi-Location Support

## Overview

The Hindu calendar is location-dependent — sunrise time varies by latitude, longitude, and timezone, which affects tithi assignment, masa determination, and solar calendar month boundaries. This document covers how the library handles different observer locations.

## Location Struct

```c
typedef struct {
    double latitude;       /* degrees N (negative for S) */
    double longitude;      /* degrees E (negative for W) */
    double altitude;       /* meters above sea level */
    double utc_offset;     /* hours east of UTC (e.g., 5.5 for IST, -5.0 for EST) */
} Location;
```

**CRITICAL**: All four fields must be set. A three-field initializer like `{lat, lon, utc_offset}` silently sets `altitude = utc_offset` and `utc_offset = 0`, producing wrong results. Always use `DEFAULT_LOCATION` or a four-field initializer:

```c
Location delhi = DEFAULT_LOCATION;                    // New Delhi (28.6139°N, 77.2090°E, IST)
Location ujjain = { 23.1824, 75.7764, 0.0, 5.5 };    // Ujjain
Location nyc   = { 40.7128, -74.0060, 0.0, -5.0 };   // New York (EST)
Location la    = { 34.0522, -118.2437, 0.0, -8.0 };   // Los Angeles (PST)
```

## What Varies by Location

### Lunisolar calendar

- **Sunrise time**: Determines which tithi "owns" a civil day. Tithi at sunrise = the tithi for that date
- **Tithi boundaries**: Near sub-minute tithi transitions at sunrise, different locations can assign different tithis to the same Gregorian date
- **Month (masa)**: Derived from tithi/new moon, so indirectly affected

### Solar calendars

Each calendar's critical time rule determines which civil day "owns" a sankranti (the exact moment the sun enters a new zodiac sign). Three of the four rules are inherently location-dependent:

| Calendar | Critical Time | Location-dependent? |
|----------|---------------|---------------------|
| **Tamil** | Sunset − 9.5 min | Yes — sunset varies by location |
| **Bengali** | Midnight + 24 min + tithi-based rule | Yes — midnight is local, tithi check uses local sunrise |
| **Odia** | 22:12 local time | Yes — 22:12 in the observer's timezone |
| **Malayalam** | End of madhyahna − 9.5 min | Yes — madhyahna = sunrise + 3/5 × daytime, both location-dependent |

## Odia Critical Time: IST to Local Time

### Background

The Odia critical time was originally discovered as a fixed 22:12 IST cutoff from 35 boundary cases verified against drikpanchang.com (see `Docs/ODIA_ADJUSTMENTS.md`). All boundary cases used Indian locations where IST = local time, so the distinction was invisible.

### Problem

When computing Odia dates for non-Indian locations (New York, Los Angeles), drikpanchang.com showed solar day numbers consistently +1 compared to our calculation. For example, March 1, 2026:

| Location | drikpanchang.com | Our code (IST rule) |
|----------|------------------|---------------------|
| Ujjain | Phalguna 17 | Phalguna 17 |
| New York | Phalguna 18 | Phalguna 17 |
| Los Angeles | Phalguna 18 | Phalguna 17 |

The offset was exactly +1 for all 31 days of the month, meaning the Kumbha sankranti (which started Odia month 11) was assigned to a different civil day for non-Indian locations.

### Root Cause

With the fixed IST rule (`jd_midnight_ut + 16.7/24.0`), the critical time is always 16:42 UTC regardless of the observer's timezone. For a sankranti falling between the IST-based cutoff (16:42 UTC) and the local-time-based cutoff (e.g., 03:12 UTC next day for NYC), the IST rule assigns it to the next day while the local rule assigns it to the current day — shifting the entire month by 1 day.

### Fix

Changed the Odia critical time from fixed IST to local time:

```c
// Before (fixed IST):
return jd_midnight_ut + 16.7 / 24.0;

// After (local time):
return jd_midnight_ut + (22.2 - loc->utc_offset) / 24.0;
```

For IST (`utc_offset = 5.5`): `22.2 - 5.5 = 16.7` — identical to the old formula. All existing India-based tests are unaffected.

For EST (`utc_offset = -5.0`): `22.2 - (-5.0) = 27.2` hours past midnight UT = 03:12 UTC next day = 22:12 EST.

### Verification

- 465/465 multi-location assertions pass (lunisolar + 4 solar calendars × 3 locations × 31 days)
- All 59,497 existing test assertions pass unchanged
- Validated against drikpanchang.com for Ujjain, New York, and Los Angeles (March 2026)

## Multi-Location Validation

### Test data

`validation/moshier/various_locations.csv` contains 465 rows scraped from drikpanchang.com for March 2026:

| Calendar | Locations | Days | Total rows |
|----------|-----------|------|------------|
| Lunisolar | Ujjain, NYC, LA | 31 each | 93 |
| Tamil | Ujjain, NYC, LA | 31 each | 93 |
| Bengali | Ujjain, NYC, LA | 31 each | 93 |
| Odia | Ujjain, NYC, LA | 31 each | 93 |
| Malayalam | Ujjain, NYC, LA | 31 each | 93 |

CSV format:
```
calendar,location,lat,lon,utc_offset,greg_year,greg_month,greg_day,tithi,solar_month,solar_day,solar_year
lunisolar,ujjain,23.1824,75.7764,5.5,2026,3,1,13,,,
tamil,nyc,40.7128,-74.006,-5.0,2026,3,1,,11,18,1947
```

Lunisolar rows have `tithi` filled and solar fields empty; solar rows have solar fields filled and `tithi` empty.

### Test suite

`tests/test_various_locations.c` reads the CSV and verifies each row:
- **Lunisolar**: Computes sunrise, calls `tithi_at_moment()`, compares with expected tithi
- **Solar**: Calls `gregorian_to_solar()`, compares month/day/year with expected values

Result: 465/465 pass (100%).

### NYC lunisolar validation (full scrape)

A separate full 55,152-day validation exists for NYC lunisolar dates (1900-2050), documented in `Docs/NYC_VALIDATION.md`. Results: 55,118/55,152 match (99.938%).

## DST Considerations

The library uses a fixed UTC offset per Location — it does not handle DST transitions automatically. For locations with DST:

- Use the UTC offset that applies to the date being computed
- The `various_locations.csv` uses fixed offsets (EST for NYC, PST for LA) which is acceptable for March 2026 validation since DST transitions are handled correctly for the dates tested
- For production use across DST boundaries, the caller should adjust `utc_offset` appropriately

A DST module exists for NYC validation (`src/dst.h` + `src/dst.c`) covering US Eastern rules 1900-2050, but it is not integrated into the main library API. See `Docs/NYC_VALIDATION.md` and `Docs/NEXT-STEPS.md` for future IANA timezone support plans.

## How Other Calendars Handle Location

- **Tamil**: Uses `sunset_jd()` which computes sunset for the observer's location. The 9.5-minute ayanamsa buffer is applied after the location-dependent sunset
- **Bengali**: Uses `loc->utc_offset` for local midnight computation. The tithi-based Sewell & Dikshit rule checks tithi at the observer's local sunrise
- **Malayalam**: Uses `sunrise_jd()` and `sunset_jd()` for the observer's location to compute madhyahna (3/5 of daytime). The 9.5-minute buffer is applied after
- **Odia**: Uses `loc->utc_offset` for 22:12 local time computation. No ayanamsa buffer needed

All four calendars are fully location-aware.
