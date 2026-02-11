# Hindu Solar Calendars: Bengali, Tamil, Odia, Malayalam

## Context

We have a working Hindu lunisolar calendar (Amanta scheme) in C that matches drikpanchang.com at 100% for 186 dates. drikpanchang.com also provides **solar** calendar variants for Bengali, Tamil, Odia, and Malayalam traditions. These are fundamentally simpler than the lunisolar calendar — no moon involvement, no adhika/kshaya months or tithis — but each region uses a different rule for when a sankranti (solar sign transition) determines a new month.

The Reingold/Dershowitz book provides both Surya Siddhanta and astronomical versions of the Hindu solar calendar. Their Surya Siddhanta version uses a sidereal year that's slightly too long (365.258756 vs ~365.25636), causing growing drift. We will use the **astronomical approach** (Swiss Ephemeris + Lahiri ayanamsa), same as our lunisolar calendar, which matches drikpanchang.com's "Drik Ganita" method.

No C implementation of Hindu solar calendars exists anywhere. We would be the first.

## How Solar Calendars Work

A solar month begins when the sun enters a new sidereal zodiac sign (rashi). The moment of crossing is called **sankranti**. Each sign spans 30 degrees of sidereal longitude, so:

- Solar month = `floor(sidereal_solar_longitude / 30) + 1` (1=Mesha...12=Meena)
- Days are numbered sequentially from month start (1, 2, 3...) — no tithis
- Solar months vary from 29-32 days (sun moves faster near perihelion in winter)
- Always exactly 12 months per year, no leap months

## Regional Differences

The **only significant difference** between the four calendars is the "critical time" rule — what moment determines which civil day starts a new month when a sankranti occurs:

| Calendar | Critical Time | Era | Epoch | Year Start |
|----------|-------------|-----|-------|------------|
| **Bengali** | Midnight (start of day)* | Bangabda | Saka - 515 | Mesha (Boishakh, ~Apr 14) |
| **Tamil** | Sunset (current day) | Saka | Saka | Mesha (Chithirai, ~Apr 14) |
| **Odia** | Sunrise (next morning) | Saka | Saka | Mesha (Baisakha, ~Apr 14) |
| **Malayalam** | Apparent noon (~12:00-1:12 PM) | Kollam | Saka + 823 | Simha (Chingam, ~Aug 17) |

*Bengali has special edge-case rules when sankranti falls between 11:36 PM and 12:24 AM.

**Malayalam is unique**: its year starts at Simha (Leo, sign 5) instead of Mesha (Aries, sign 1). Month numbering rotates accordingly: Chingam=1 (Simha), Kanni=2 (Kanya), ..., Karkadakam=12 (Karka).

## What We Already Have (Reusable)

| Function | File | Reuse |
|----------|------|-------|
| `solar_longitude_sidereal(jd)` | `src/astro.c` | Core: sidereal solar longitude via Swiss Ephemeris |
| `solar_rashi(jd)` | `src/masa.c` | `ceil(sidereal_long / 30)` — gives sign 1-12 |
| `sunrise_jd(jd, loc)` | `src/astro.c` | Odia critical time |
| `sunset_jd(jd, loc)` | `src/astro.c` | Tamil critical time |
| `gregorian_to_jd(y,m,d)` | `src/date_utils.c` | JD conversion |
| `jd_to_gregorian(jd,...)` | `src/date_utils.c` | Reverse conversion |
| `hindu_year_saka(jd, masa)` | `src/masa.c` | Saka year (needs adaptation for solar) |
| Bisection infrastructure | `src/tithi.c` | Pattern for sankranti finding |
| 17-point Lagrange interpolation | `src/masa.c` | Alternative for sankranti finding |

## What Needs to Be Built

### New files

| File | Purpose |
|------|---------|
| `src/solar.h` | Public API: data types + functions for solar calendar |
| `src/solar.c` | Core: sankranti finding, solar date calculation, regional variants |
| `tests/test_solar.c` | Test suite: spot-checked dates against drikpanchang.com |

### Modified files

| File | Change |
|------|--------|
| `src/types.h` | Add `SolarDate` struct, `SolarCalendarType` enum, regional month name arrays |
| `src/main.c` | Add `-s` flag for solar calendar output |
| `Makefile` | Add `solar.o` to APP_SRCS |

---

## Implementation Plan

### Phase 1: Core Solar Module (`src/solar.h` + `src/solar.c`)

#### Data Structures (in `src/types.h`)

```c
typedef enum {
    SOLAR_CAL_TAMIL = 0,
    SOLAR_CAL_BENGALI,
    SOLAR_CAL_ODIA,
    SOLAR_CAL_MALAYALAM,
} SolarCalendarType;

typedef struct {
    int year;              /* Regional era year */
    int month;             /* 1-12 (regional month number) */
    int day;               /* Day within solar month (1-32) */
    int rashi;             /* Sidereal zodiac sign 1-12 at critical time */
    double jd_sankranti;   /* JD of the sankranti that started this month */
} SolarDate;
```

#### Public API (`src/solar.h`)

```c
/* Find exact JD when sidereal solar longitude crosses a target (e.g. 0, 30, 60...) */
double sankranti_jd(double jd_approx, double target_longitude);

/* Find the sankranti (rashi entry) on or before a given JD */
double sankranti_before(double jd_ut);

/* Convert Gregorian date to solar calendar date for a given regional variant */
SolarDate gregorian_to_solar(int year, int month, int day,
                             const Location *loc, SolarCalendarType type);

/* Convert solar calendar date back to Gregorian (inverse) */
void solar_to_gregorian(const SolarDate *sd, SolarCalendarType type,
                        const Location *loc, int *year, int *month, int *day);

/* Regional month name string */
const char *solar_month_name(int month, SolarCalendarType type);

/* Regional era name string */
const char *solar_era_name(SolarCalendarType type);
```

#### Core Algorithm: `gregorian_to_solar()`

```
1. Compute JD for the given Gregorian date
2. Compute critical time based on calendar type:
   - Tamil: sunset_jd(jd, loc)
   - Bengali: jd (midnight)
   - Odia: sunrise_jd(jd + 1, loc)  [next morning]
   - Malayalam: (sunrise_jd + sunset_jd) / 2  [approx apparent noon]
3. Get sidereal solar longitude at critical time
4. rashi = floor(longitude / 30) + 1
5. Find the sankranti that started this month:
   - target = (rashi - 1) * 30
   - Binary search backward from critical time for when longitude crossed target
6. Find the Gregorian date of that sankranti (using same critical-time rule)
7. day = gregorian_date - sankranti_greg_date + 1
8. Compute regional year from Saka year + era offset
9. Map rashi to regional month number (most: rashi directly; Malayalam: rotated)
```

#### Sankranti Finding: `sankranti_jd()`

Reuse the bisection pattern from `find_tithi_boundary()` in `tithi.c`:

```
1. Estimate: jd_approx (based on mean solar motion ~365.25636 days/year)
2. Bracket: [jd_approx - 5, jd_approx + 5]
3. Bisect: 50 iterations on (sidereal_solar_longitude(mid) - target),
   handling 360/0 wraparound
4. Return JD precise to ~0.1 seconds
```

#### Regional Configuration (static tables in `solar.c`)

Each calendar variant is defined by a config struct:

```c
typedef struct {
    SolarCalendarType type;
    int era_offset;              /* Added to Saka year to get regional year */
    int first_rashi;             /* Rashi that starts month 1 (1=Mesha, 5=Simha for Malayalam) */
    const char *month_names[12]; /* Regional month names */
    const char *era_name;        /* "Bangabda", "Saka", "Kollam" */
    /* critical_time function pointer or enum */
} SolarCalendarConfig;
```

Regional month names:

| # | Rashi | Bengali | Tamil | Odia | Malayalam |
|---|-------|---------|-------|------|-----------|
| 1 (Mesha) | Boishakh | Chithirai | Baisakha | Medam |
| 2 (Vrishabha) | Joishtho | Vaikaasi | Jyeshtha | Edavam |
| 3 (Mithuna) | Asharh | Aani | Ashadha | Mithunam |
| 4 (Karka) | Srabon | Aadi | Shravana | Karkadakam |
| 5 (Simha) | Bhadro | Aavani | Bhadrapada | **Chingam** (month 1) |
| 6 (Kanya) | Ashshin | Purattaasi | Ashvina | **Kanni** (month 2) |
| 7 (Tula) | Kartik | Aippasi | Kartika | **Thulam** (month 3) |
| 8 (Vrishchika) | Ogrohaeon | Karthikai | Margashirsha | **Vrishchikam** (month 4) |
| 9 (Dhanu) | Poush | Maargazhi | Pausha | **Dhanu** (month 5) |
| 10 (Makara) | Magh | Thai | Magha | **Makaram** (month 6) |
| 11 (Kumbha) | Falgun | Maasi | Phalguna | **Kumbham** (month 7) |
| 12 (Meena) | Choitro | Panguni | Chaitra | **Meenam** (month 8) |

Malayalam months 9-12: Medam, Edavam, Mithunam, Karkadakam (corresponding to Mesha through Karka).

Era offsets from Saka:
- Bengali (Bangabda): Saka - 515
- Tamil: Saka (no offset)
- Odia: Saka (no offset, though some use Vilayati era)
- Malayalam (Kollam): Saka + 823

### Phase 2: Tests (`tests/test_solar.c`)

Spot-check dates against drikpanchang.com for each calendar variant. Start with ~10-20 dates per calendar:

- Sankranti boundary dates (month transitions)
- Mid-month dates
- Year boundaries (Mesha sankranti for Bengali/Tamil/Odia, Simha for Malayalam)
- Dates near solstices/equinoxes (varying month lengths)

Test pattern matches existing `test_validation.c` — custom assert macros, `astro_init(NULL)`, compare our output vs known-good values.

### Phase 3: CLI Extension (`src/main.c`)

Add a `-s TYPE` flag:

```
Usage: hindu-calendar [options]
  -s TYPE    Solar calendar: tamil, bengali, odia, malayalam
             (if omitted, shows lunisolar panchang as before)
```

Output format for solar calendar month:

```
=== Bengali Solar Calendar — Boishakh 1432 (Bangabda) ===
Date       | Day | Solar Date
2025-04-14 | Mon | Boishakh 1, 1432
2025-04-15 | Tue | Boishakh 2, 1432
...
```

### Phase 4: Validation

1. Manually check ~20 dates per calendar against drikpanchang.com's regional pages
2. Generate a CSV for bulk regression (like `ref_1900_2050.csv` for lunisolar)
3. Extend the validation web page or create a separate page for solar calendars

---

## Files to Create/Modify (Summary)

| File | Action | Lines (est.) |
|------|--------|-------------|
| `src/types.h` | Modify: add SolarDate, SolarCalendarType | +20 |
| `src/solar.h` | Create: public API | ~40 |
| `src/solar.c` | Create: sankranti finding, date conversion, regional configs | ~250 |
| `src/main.c` | Modify: add `-s` flag | +30 |
| `tests/test_solar.c` | Create: validation tests | ~150 |
| `Makefile` | Modify: add solar.o | +1 line |

**Total new code: ~500 lines** (smaller than the lunisolar implementation because we reuse astro.c, date_utils.c, and don't need moon calculations at all).

## Verification

1. `make` compiles without warnings
2. `make test` — all existing tests still pass + new test_solar passes
3. `./hindu-calendar -s tamil -y 2025 -m 4` shows Chithirai 1 on April 14
4. `./hindu-calendar -s bengali -y 2025 -m 4` shows Boishakh 1 on April 14-15
5. `./hindu-calendar -s malayalam -y 2025 -m 8` shows Chingam 1 on August 17
6. Spot-check 10+ dates per calendar against drikpanchang.com regional pages
7. Verify sankranti dates match drikpanchang.com's listed sankranti times

## Implementation Status: Complete

Implemented in v0.3.0, expanded in v0.3.1. All solar tests pass: 143 unit + 327 validation + 28,976 regression = 29,446 solar assertions. Total: 51,735 assertions across 9 suites.

Malayalam apparent noon rule confirmed correct in v0.3.1 — see `Docs/MALAYALAM_NOON_FIX.md`.

## Resolved Open Questions

1. **Bengali midnight edge case**: Confirmed that drikpanchang.com uses the Reingold/Dershowitz 24-minute buffer rule. Vrishabha Sankranti 2025 at 00:11 IST (11 minutes past midnight) is treated as belonging to the same day (May 15), making Joishtho start on May 15 and giving Jun 15 = Joishtho 32. The critical time is midnight + 24 minutes: `jd_midnight + 24min/(24*60)`.

2. **Malayalam critical time**: Apparent noon (midpoint of sunrise and sunset) matches drikpanchang.com. Computed as `(sunrise_jd + sunset_jd) / 2.0`. This varies seasonally (~12:00-12:30 PM IST in New Delhi) rather than the fixed "1:12 PM" mentioned in some sources.

3. **Era details**: drikpanchang.com uses the astronomical/traditional Bengali calendar (not the reformed Bangladesh government version). Our implementation matches.

## Implementation Findings (Deviations from Plan)

### Critical Time Rules

The plan had some incorrect critical time rules. The actual rules verified against drikpanchang.com:

| Calendar | Plan | Actual | Notes |
|----------|------|--------|-------|
| **Tamil** | Sunset | **Sunset** | Correct as planned |
| **Bengali** | Midnight | **Midnight + 24min buffer** | Edge-case zone 11:36 PM - 12:24 AM from R/D book confirmed |
| **Odia** | Sunrise (next morning) | **End of civil day** | Sankranti assigned to whichever local date it falls on |
| **Malayalam** | Apparent noon | **Apparent noon** | Correct as planned; `(sunrise + sunset) / 2` |

### Era Calculations

The plan said to derive eras from Saka year with an offset. This was wrong — eras are computed directly from the Gregorian year:

| Calendar | Plan | Actual |
|----------|------|--------|
| Tamil (Saka) | Saka (no offset) | `gy - 78` on/after Mesha, `gy - 79` before |
| Bengali (Bangabda) | Saka - 515 | `gy - 593` on/after Mesha, `gy - 594` before |
| Odia (Saka) | Saka (no offset) | `gy - 78` on/after Mesha, `gy - 79` before |
| Malayalam (Kollam) | Saka + 823 | `gy - 824` on/after Simha, `gy - 825` before |

The plan's "Saka + 823" formula would give Kollam ~2769 for year 2025, when the correct value is ~1200. Kollam era is not derived from Saka at all.

### Sankranti Finding

Used bisection with a 20-day bracket (not 5-day as planned), 50 iterations. Precision is ~3 nanoseconds (`40 days / 2^50`), far exceeding the ~0.1 second estimate in the plan.

### Code Size

Actual code was ~620 lines (solar.c ~387, test_solar.c ~239), slightly larger than the ~500 line estimate due to the more complex era/year logic.
