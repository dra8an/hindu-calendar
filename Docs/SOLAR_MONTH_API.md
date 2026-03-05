# Solar Month API

## Overview

Two new public functions provide programmatic access to solar month boundaries
and lengths for all four regional calendars (Tamil, Bengali, Odia, Malayalam).

Both functions are declared in `src/solar.h` and implemented in `src/solar.c`.

## API Reference

### `solar_month_start()`

```c
double solar_month_start(int month, int year, SolarCalendarType type,
                         const Location *loc);
```

Returns the Julian Day number of the **first civil day** of the given solar
month.

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `month` | `int` | Regional month number (1-12). Month 1 is the first month of the calendar (Chithirai for Tamil, Boishakh for Bengali/Odia, Chingam for Malayalam). |
| `year` | `int` | Regional year in the calendar's era (Saka for Tamil, Bangabda for Bengali, Amli for Odia, Kollam for Malayalam). |
| `type` | `SolarCalendarType` | One of `SOLAR_CAL_TAMIL`, `SOLAR_CAL_BENGALI`, `SOLAR_CAL_ODIA`, `SOLAR_CAL_MALAYALAM`. |
| `loc` | `const Location *` | Observer location. Use `DEFAULT_LOCATION` for New Delhi (28.6139°N, 77.2090°E, IST). |

**Returns:** Julian Day (UT) of the first day of the month. Convert to
Gregorian with `jd_to_gregorian()`.

**Example:**

```c
Location loc = DEFAULT_LOCATION;
double jd = solar_month_start(1, 1947, SOLAR_CAL_TAMIL, &loc);
int gy, gm, gd;
jd_to_gregorian(jd, &gy, &gm, &gd);
// gy=2025, gm=4, gd=14 → Chithirai 1, 1947 starts on April 14, 2025
```

### `solar_month_length()`

```c
int solar_month_length(int month, int year, SolarCalendarType type,
                       const Location *loc);
```

Returns the number of days in the given solar month (29-32).

Internally defined as:

```
solar_month_start(next_month, next_year) - solar_month_start(month, year)
```

The year-wrapping logic correctly handles all calendars, including Odia where
the year increments at month 6 (Ashvina, ~September) rather than month 12.

**Parameters:** Same as `solar_month_start()`.

**Returns:** Number of days (integer, typically 29-32).

**Example:**

```c
Location loc = DEFAULT_LOCATION;
int len = solar_month_length(1, 1947, SOLAR_CAL_TAMIL, &loc);
// len = 31 → Chithirai 1947 has 31 days
```

## Implementation Details

### Algorithm

`solar_month_start()` uses the forward sankranti pipeline:

1. **Convert** regional month number to sidereal rashi (zodiac sign)
2. **Approximate** the Gregorian year/month from the era offset, correctly
   handling year wrapping for all calendars (Odia months 1-5 fall in the next
   Gregorian year relative to the year-start; Malayalam months 9-12 similarly)
3. **Find exact sankranti** via `sankranti_jd()` — 50-iteration bisection on
   sidereal solar longitude (~3 ns precision)
4. **Determine civil day** via `sankranti_to_civil_day()` — applies the
   calendar-specific critical time rule (sunset for Tamil, midnight for
   Bengali, 22:12 IST for Odia, end-of-madhyahna for Malayalam)
5. **Verify** against `gregorian_to_solar()` — the forward function validated
   to 100% match against drikpanchang.com. On rare boundary edge cases (where
   `sankranti_to_civil_day` and `bengali_rashi_correction` disagree), adjusts
   ±1 day to match the validated forward pipeline

### Year Wrapping

Each calendar has a "last month of the year" — the month after which the
regional year increments:

| Calendar | Year starts at | Last month | Year increments at |
|----------|---------------|------------|-------------------|
| Tamil | Month 1 (Mesha, ~Apr) | 12 (Panguni) | Month 12 → 1 |
| Bengali | Month 1 (Mesha, ~Apr) | 12 (Choitro) | Month 12 → 1 |
| Odia | Month 6 (Kanya, ~Sep) | 5 (Bhadrapada) | Month 5 → 6 |
| Malayalam | Month 1 (Simha, ~Aug) | 12 (Karkadakam) | Month 12 → 1 |

`solar_month_length()` uses this to determine the correct `(next_month,
next_year)` pair.

### Gregorian Year Approximation

The approximate Gregorian date is computed from the year-start position to
correctly handle all wrapping cases:

```c
int months_past = month - year_start_month;
if (months_past < 0) months_past += 12;

int gy = year + gy_offset_on;
int gm = gm_of_year_start + months_past;
// Wrap gm through December, incrementing gy as needed
```

This handles:
- **Odia** months 1-5 (Apr-Aug): fall in the next Gregorian year after the
  year-start (Kanya, ~Sep)
- **Malayalam** months 9-12 (Apr-Jul): wrap past December into the next
  Gregorian year after the year-start (Simha, ~Aug)

## Validation

Both functions were validated against **7,248 month start dates** scraped from
drikpanchang.com (1,812 months per calendar, 1900-2050):

| Calendar | Month starts | Month lengths |
|----------|-------------|---------------|
| Tamil | 1,812/1,812 (100%) | 1,811/1,811 (100%) |
| Bengali | 1,812/1,812 (100%) | 1,811/1,811 (100%) |
| Odia | 1,812/1,812 (100%) | 1,811/1,811 (100%) |
| Malayalam | 1,812/1,812 (100%) | 1,811/1,811 (100%) |

(Length validation covers n-1 months since the last month has no "next" to
measure against.)

## Test Coverage

32 assertions in `tests/test_solar.c` (`test_month_start_and_length`):
16 test cases × 2 checks (start date + length) across all 4 calendars,
including:

- Year boundary crossings (Tamil Panguni → Chithirai, Bengali Choitro → Boishakh)
- Odia year-start at Ashvina (month 6)
- Malayalam months 9-12 that wrap into the next Gregorian year
- Months of varying lengths (29-32 days)

## Important: Location Initialization

The `Location` struct has **4 fields** (latitude, longitude, altitude,
utc_offset). Always use `DEFAULT_LOCATION` or initialize all 4:

```c
/* Correct */
Location loc = DEFAULT_LOCATION;
Location loc = { 28.6139, 77.2090, 0.0, 5.5 };

/* WRONG — sets altitude=5.5, utc_offset=0.0 */
Location loc = { 28.6139, 77.2090, 5.5 };
```

## Related Functions

| Function | Purpose |
|----------|---------|
| `gregorian_to_solar()` | Convert Gregorian → solar date (forward) |
| `solar_to_gregorian()` | Convert solar date → Gregorian (inverse) |
| `solar_month_name()` | Regional month name string (1-indexed) |
| `solar_era_name()` | Regional era name string |
| `sankranti_jd()` | Find exact JD of a sankranti crossing |

## Files Modified

- `src/solar.h` — Added declarations for `solar_month_start()` and `solar_month_length()`
- `src/solar.c` — Implementation (~50 lines)
- `tests/test_solar.c` — 32 new assertions (16 cases × 2 checks)
