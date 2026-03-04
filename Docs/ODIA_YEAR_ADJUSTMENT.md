# Odia Calendar Year Adjustment

## Discovery

While building the NYC validation infrastructure (Phase 19), we cross-checked
our Odia solar calendar output against the 1,812 scraped drikpanchang.com HTML
files already on disk from Phase 16. Two bugs were found in the Odia year
calculation:

1. **Wrong era**: We used Saka (epoch 78 CE) instead of Amli (epoch ~593 CE)
2. **Wrong year-start rashi**: We assumed the year increments at Mesha (~April)
   like Tamil and Bengali, but the Odia Amli year increments at Kanya (~September)

Both bugs only affected the **year number** — month names, day numbers, and
sankranti boundaries were all correct (confirmed by the 100% solar scrape match
in Phase 16, which only checks month start dates, not year numbers).

## Evidence from Scraped Data

### Era offset

For January 1989, drikpanchang.com shows:

```
Header: Pausha - Magha 1396
```

Our code produced year **1910** (Saka: 1989 − 79 = 1910). The correct value
is **1396**, giving an offset of 1989 − 1396 = 593.

### Year-start rashi

Examining all 12 months of 2025 from the scraped Odia HTML headers:

```
2025-01: Pausha - Magha 1432
2025-02: Magha - Phalgun 1432
2025-03: Phalgun - Chyatra 1432
2025-04: Chyatra - Byisakha 1432
2025-05: Byisakha - Jyosta 1432
2025-06: Jyosta - Asadha 1432
2025-07: Asadha - Srabana 1432
2025-08: Srabana - Bhadra 1432
2025-09: Bhadra 1432 - Aswin 1433      <-- year changes here
2025-10: Aswin - Kartika 1433
2025-11: Kartika - Margashira 1433
2025-12: Margashira - Pausha 1433
```

The year increments at **Aswin** (Kanya sankranti, rashi 6, ~September 17),
not at Baisakha (Mesha sankranti, rashi 1, ~April 14).

Verified across 9 different years (1950, 1970, 1989, 2000, 2010, 2020, 2030,
2040, 2050) — the pattern is 100% consistent.

## What Changed

### 1. New `year_start_rashi` field in SolarCalendarConfig

Previously, a single `first_rashi` field served both purposes:
- **Month numbering**: rashi that maps to regional month 1
- **Year start**: rashi where the year number increments

For Tamil, Bengali, and Malayalam these are the same. But for Odia they differ:
month 1 is Baisakha (Mesha, rashi 1) but the year starts at Ashvina (Kanya,
rashi 6).

**Before:**
```c
typedef struct {
    SolarCalendarType type;
    int first_rashi;       /* Rashi that starts month 1 */
    int gy_offset_on;
    int gy_offset_before;
    ...
} SolarCalendarConfig;
```

**After:**
```c
typedef struct {
    SolarCalendarType type;
    int first_rashi;       /* Rashi that starts month 1 (1=Mesha, 5=Simha) */
    int year_start_rashi;  /* Rashi where year number increments */
    int gy_offset_on;
    int gy_offset_before;
    ...
} SolarCalendarConfig;
```

### 2. Config table update

**Before:**
```c
{ SOLAR_CAL_TAMIL,     1,  78,   79,  TAMIL_MONTHS,     "Saka"     },
{ SOLAR_CAL_BENGALI,   1,  593,  594, BENGALI_MONTHS,   "Bangabda" },
{ SOLAR_CAL_ODIA,      1,  78,   79,  ODIA_MONTHS,      "Saka"     },
{ SOLAR_CAL_MALAYALAM, 5,  824,  825, MALAYALAM_MONTHS,  "Kollam"   },
```

**After:**
```c
//                   first  year_start
//                   rashi  rashi       on   before
{ SOLAR_CAL_TAMIL,     1,     1,        78,   79,  TAMIL_MONTHS,     "Saka"     },
{ SOLAR_CAL_BENGALI,   1,     1,       593,  594,  BENGALI_MONTHS,   "Bangabda" },
{ SOLAR_CAL_ODIA,      1,     6,       592,  593,  ODIA_MONTHS,      "Amli"     },
{ SOLAR_CAL_MALAYALAM, 5,     5,       824,  825,  MALAYALAM_MONTHS,  "Kollam"   },
```

Key differences for Odia:
- `first_rashi`: 1 (Mesha) — month 1 = Baisakha, unchanged
- `year_start_rashi`: 6 (Kanya) — year increments at Ashvina (~September)
- `gy_offset_on`: 592 (on/after Kanya sankranti: `gy - 592`)
- `gy_offset_before`: 593 (before Kanya sankranti: `gy - 593`)
- `era_name`: "Amli" (was "Saka")

### 3. Year calculation logic (`regional_year`)

The year calculation function now uses `year_start_rashi` instead of
`first_rashi` when finding the year-start sankranti:

```c
double target_long = (double)(cfg->year_start_rashi - 1) * 30.0;
int approx_greg_month = 3 + cfg->year_start_rashi;
```

### 4. Reverse conversion (`solar_to_gregorian`)

The reverse conversion also uses `year_start_rashi` to determine whether a
regional month falls before or after the year-start sankranti in the Gregorian
calendar:

```c
int start_greg_month = 3 + cfg->year_start_rashi;
```

For Odia, this means months 1–5 (Baisakha through Bhadrapada, Apr–Aug) are
recognized as falling *before* the year-start rashi (Kanya, Sep) and are
correctly placed in the next Gregorian year during reverse conversion.

### 5. Month numbering unchanged

`rashi_to_regional_month()` still uses `first_rashi`, so Odia month numbering
is unaffected:
- Rashi 1 (Mesha) → regional month 1 (Baisakha)
- Rashi 6 (Kanya) → regional month 6 (Ashvina)
- Rashi 12 (Meena) → regional month 12 (Chaitra)

## Offset Derivation

The Odia Amli era traces to the Amli San (ଅମଳୀ ସନ), a revenue calendar
introduced in Odisha. The epoch corresponds to ~593 CE.

From the scraped data:

| Gregorian date | Position | Odia year | Offset |
|----------------|----------|-----------|--------|
| Jan 2025 (before Kanya) | old year | 1432 | 2025 − 1432 = **593** |
| Oct 2025 (after Kanya) | new year | 1433 | 2025 − 1433 = **592** |
| Jan 1989 (before Kanya) | old year | 1396 | 1989 − 1396 = **593** |
| Sep 1989 (after Kanya) | new year | 1397 | 1989 − 1397 = **592** |
| Jan 1950 (before Kanya) | old year | 1357 | 1950 − 1357 = **593** |
| Sep 1950 (after Kanya) | new year | 1358 | 1950 − 1358 = **592** |

## Why This Wasn't Caught in Phase 16

The Phase 16 full solar scrape comparison (`scraper/solar/compare.py`) validates
**month start dates** — it checks which Gregorian date begins each solar month.
It does NOT compare year numbers. The month boundaries and month names were
always correct; only the year number was wrong.

## Files Modified

### Source code
- `src/solar.c` — added `year_start_rashi` field, changed Odia config, updated
  `regional_year()` and `solar_to_gregorian()` to use `year_start_rashi`

### Tests (year expectations updated)
- `tests/test_solar.c` — 35 Odia entries
- `tests/test_solar_edge.c` — 100 Odia entries
- `tests/test_solar_validation.c` — 24 Odia entries

### Generated data
- `validation/moshier/solar/odia_months_1900_2050.csv` — regenerated with correct years

### Documentation
- `Docs/ARCHITECTURE.md`
- `Docs/HINDU_CALENDAR_GUIDE.md` (4 tables + section heading)
- `Docs/PROJECT-STATUS.md`
- `Docs/SOLAR_PLAN.md` (3 tables)
- `Docs/TEMP_COMPREHENSIVE_GUIDE.md`
- `CHANGELOG.md`
- `Docs/memory/MEMORY.md`
- Auto-memory `MEMORY.md`

### Still pending
- `java/src/main/java/com/hindu/calendar/model/SolarCalendarType.java` — still uses Saka/78/79
- `rust/src/core/solar.rs` — still uses Saka/78/79
- Both ports need the `year_start_rashi` concept added

## Verification

After the fix, all 53,143 tests pass (0 failures). Cross-checked 6 dates
spanning 1950–2050 against scraped drikpanchang.com HTML headers — all year
numbers match exactly.
