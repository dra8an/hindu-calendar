# Plan: Scraper Restructure + Solar Calendar Scraping

## Context

The `scraper/` directory currently holds lunisolar-only data (1,812 drikpanchang HTML pages, parsed CSV, comparison reports) but nothing in the naming indicates it's lunisolar. We need to:
1. Reorganize the directory to clearly separate lunisolar from solar data
2. Add solar calendar scraping infrastructure (Tamil, Bengali, Odia, Malayalam)
3. Update verification tools to compare all calendars against drikpanchang

## Step 1: Directory Restructure

### Current layout
```
scraper/
  config.py, fetch.py, parse.py, compare.py, README.md, requirements.txt
  data/
    raw/month/YYYY-MM.html           ← 1,812 lunisolar HTML files (363 MB)
    raw/day/                          ← empty
    parsed/drikpanchang_lunisolar.csv
    comparison_report.txt
    comparison_report_se.txt
```

### New layout
```
scraper/
  common.py                          ← shared: session mgmt, CAPTCHA detection, fetch utils
  requirements.txt
  README.md

  lunisolar/
    config.py                        ← lunisolar-specific URLs, paths, tithi/masa mappings
    fetch.py                         ← (moved from scraper/fetch.py)
    parse.py                         ← (moved from scraper/parse.py)
    compare.py                       ← (moved from scraper/compare.py)

  solar/
    config.py                        ← solar-specific URLs, paths, month name mappings
    fetch.py                         ← new: parameterized by calendar type (--calendar tamil|bengali|odia|malayalam)
    parse.py                         ← new: extract solar day + month from HTML
    compare.py                       ← new: diff parsed solar months vs our reference CSVs

  data/
    lunisolar/
      raw/YYYY-MM.html               ← moved from data/raw/month/ (1,812 files)
      parsed/drikpanchang.csv         ← renamed from drikpanchang_lunisolar.csv
      comparison_report.txt
      comparison_report_se.txt
    solar/
      raw/tamil/YYYY-MM.html          ← new (1,812 files each, ~363 MB each)
      raw/bengali/YYYY-MM.html
      raw/odia/YYYY-MM.html
      raw/malayalam/YYYY-MM.html
      parsed/tamil.csv                ← new: parsed month boundaries
      parsed/bengali.csv
      parsed/odia.csv
      parsed/malayalam.csv
      comparison/tamil_report.txt     ← new: comparison reports
      comparison/bengali_report.txt
      comparison/odia_report.txt
      comparison/malayalam_report.txt
```

### Migration commands
```bash
# Move lunisolar data
mkdir -p scraper/data/lunisolar/raw
mv scraper/data/raw/month/*.html scraper/data/lunisolar/raw/
mv scraper/data/parsed/drikpanchang_lunisolar.csv scraper/data/lunisolar/parsed/drikpanchang.csv
mv scraper/data/comparison_report*.txt scraper/data/lunisolar/

# Move scripts into lunisolar/
mkdir -p scraper/lunisolar scraper/solar
mv scraper/fetch.py scraper/lunisolar/
mv scraper/parse.py scraper/lunisolar/
mv scraper/compare.py scraper/lunisolar/

# Create solar directories
mkdir -p scraper/data/solar/raw/{tamil,bengali,odia,malayalam}
mkdir -p scraper/data/solar/parsed
mkdir -p scraper/data/solar/comparison

# Clean up empty old dirs
rmdir scraper/data/raw/month scraper/data/raw/day scraper/data/raw scraper/data/parsed
```

### Files to modify
- `scraper/lunisolar/config.py` — update all paths to use `data/lunisolar/` instead of `data/raw/month/`, `data/parsed/`
- `scraper/lunisolar/fetch.py` — import from `..common` and `config`
- `scraper/lunisolar/parse.py` — import from `..common` and `config`
- `scraper/lunisolar/compare.py` — import from `config`
- Add `scraper/__init__.py`, `scraper/lunisolar/__init__.py`, `scraper/solar/__init__.py` for package imports

### Extract shared code → `scraper/common.py`
From existing `fetch.py`, extract into `common.py`:
- `COOKIES` dict (shared across all calendars — same location, same settings)
- `HEADERS` dict
- `MIN_VALID_SIZE` constant
- `fetch_url(url, output_path, session)` function
- `new_session()` function
- Signal handler / graceful shutdown logic

## Step 2: Solar Fetcher

### URLs (from previous exploration)
```python
SOLAR_URLS = {
    "tamil":     "https://www.drikpanchang.com/tamil/tamil-month-panchangam.html",
    "bengali":   "https://www.drikpanchang.com/bengali/bengali-month-panjika.html",
    "odia":      "https://www.drikpanchang.com/oriya/oriya-panji.html",
    "malayalam": "https://www.drikpanchang.com/malayalam/malayalam-month-calendar.html",
}
```

### Fetch strategy
- Same approach as lunisolar: fetch every Gregorian month 1900-01 through 2050-12
- 1,812 fetches per calendar × 4 calendars = 7,248 total fetches
- At 20s delay: ~10 hours per calendar, ~40 hours total
- Save to `scraper/data/solar/raw/{calendar}/YYYY-MM.html`
- Resume capability: skip existing files
- Session rotation on CAPTCHA (same as lunisolar)

### CLI
```bash
python3 -m scraper.solar.fetch --calendar tamil --delay 20
python3 -m scraper.solar.fetch --calendar bengali --start-year 2000 --end-year 2025
```

### Pre-requisite: HTML structure discovery
**Before writing the parser**, manually download 1 sample page per calendar to examine the HTML structure offline:
```bash
# User runs these manually (4 requests, won't trigger rate limit)
cd scraper/data/solar/raw
curl -b "drik-geoname-id=1261481;drik-language=en;drik-ayanamsha-type=chitra-paksha" \
  "https://www.drikpanchang.com/tamil/tamil-month-panchangam.html?date=01/01/2025" > tamil/2025-01.html
# (repeat for bengali, odia, malayalam)
```
Then examine the DOM to find CSS selectors for:
- Solar day number (equivalent of `dpBigDate` for lunisolar)
- Solar month name
- Solar year/era
- Grid cell structure

## Step 3: Solar Parser

### Expected output format
To match our reference CSV format:
```csv
month,year,length,greg_year,greg_month,greg_day,month_name
```

### Parsing approach
From each Gregorian month page, extract pairs of (gregorian_day, solar_day_number, solar_month_name).
Scan for solar day = 1 → that's the start of a new solar month.
Aggregate across all pages to build the month-start-date table.

### Comparison with reference
Our reference CSVs (`validation/moshier/solar/tamil_months_1900_2050.csv`) have 1,811 data rows each (151 years × 12 months). The parsed drikpanchang data should produce the same: for each solar month, the Gregorian date it starts on.

A mismatch means our code assigns a sankranti to a different civil day than drikpanchang — exactly the boundary cases we've been studying.

## Step 4: Solar Comparison Tool

### `scraper/solar/compare.py`
```bash
python3 -m scraper.solar.compare --calendar tamil
python3 -m scraper.solar.compare --calendar all
```

For each calendar:
1. Load parsed drikpanchang solar CSV
2. Load our reference CSV from `validation/moshier/solar/{calendar}_months_1900_2050.csv`
3. Compare month-by-month: does `(greg_year, greg_month, greg_day)` match?
4. Report: total months, matches, mismatches with details
5. Save to `scraper/data/solar/comparison/{calendar}_report.txt`

Expected results based on our edge case tests:
- Tamil: ~6 known boundary mismatches (resolved by 8.0 min buffer)
- Bengali: 1 known failure (1976-10-17)
- Odia: 0 mismatches (100/100 edge cases correct)
- Malayalam: ~15 known boundary mismatches (resolved by 9.5 min buffer)

A full scrape will tell us the true numbers across all 1,811 month boundaries per calendar.

## Step 5: Update README

Update `scraper/README.md` to document:
- New directory structure
- How to run lunisolar vs solar scraping
- Comparison workflow for all calendars

## Implementation Order

1. **Directory restructure** — move files, update imports/paths, verify lunisolar still works
2. **Extract `common.py`** — shared fetch utilities
3. **Create `scraper/solar/config.py`** — URLs, month name mappings, paths
4. **Create `scraper/solar/fetch.py`** — parameterized fetcher
5. **User manually downloads 4 sample pages** — 1 per solar calendar
6. **Examine HTML** — discover DOM selectors for solar data
7. **Create `scraper/solar/parse.py`** — build parser, test on sample pages
8. **Create `scraper/solar/compare.py`** — diff against reference CSVs
9. **User runs fetcher** — all 4 calendars (~40 hours total)
10. **Run parse + compare** — full validation

## Verification

After step 1: run `python3 -m scraper.lunisolar.compare` and confirm same 35-mismatch result.
After step 7: parse sample pages and spot-check against known dates.
After step 10: compare full results against our edge case test expectations.

## Fetch Time Estimates

| Calendar   | Pages | At 20s delay | Storage |
|-----------|-------|-------------|---------|
| Lunisolar | 1,812 | ~10h (done) | 363 MB  |
| Tamil     | 1,812 | ~10h        | ~360 MB |
| Bengali   | 1,812 | ~10h        | ~360 MB |
| Odia      | 1,812 | ~10h        | ~360 MB |
| Malayalam  | 1,812 | ~10h        | ~360 MB |
| **Total** | **9,060** | **~50h** | **~1.8 GB** |

Note: Calendars can potentially be fetched in parallel from different sessions if rate-limiting is per-session rather than per-IP. Need to test cautiously.
