# Drikpanchang.com Scraper

Scrapes drikpanchang.com to obtain independent calendar data for validation
against our computed references. Supports both lunisolar (tithi) and solar
(Tamil, Bengali, Odia, Malayalam) calendars.

## Setup

```bash
pip install -r requirements.txt
```

## Directory Structure

```
scraper/
  common.py                          # Shared: session mgmt, CAPTCHA detection, fetch utils
  requirements.txt
  README.md

  lunisolar/
    config.py                        # Lunisolar URLs, paths, tithi/masa mappings
    fetch.py                         # Download month panchang pages
    parse.py                         # Extract tithi per day from HTML
    compare.py                       # Diff parsed tithis vs reference CSV

  solar/
    config.py                        # Solar URLs, paths, month name mappings
    fetch.py                         # Download solar calendar pages (parameterized by calendar)
    parse.py                         # Extract solar month boundaries from HTML
    compare.py                       # Diff parsed solar months vs reference CSVs

  data/
    lunisolar/
      raw/YYYY-MM.html               # 1,812 lunisolar HTML files (1900-2050)
      parsed/drikpanchang.csv         # Parsed tithi per day (55,152 rows)
      comparison_report.txt           # Moshier comparison (35 mismatches, 99.937%)
      comparison_report_se.txt        # Swiss Ephemeris comparison (37 mismatches)
    solar/
      raw/tamil/YYYY-MM.html         # 1,812 HTML files per calendar
      raw/bengali/YYYY-MM.html
      raw/odia/YYYY-MM.html
      raw/malayalam/YYYY-MM.html
      parsed/tamil.csv               # Parsed month start dates
      parsed/bengali.csv
      parsed/odia.csv
      parsed/malayalam.csv
      comparison/tamil_report.txt     # Comparison reports
      comparison/bengali_report.txt
      comparison/odia_report.txt
      comparison/malayalam_report.txt
```

## Lunisolar Scraping

### 1. Fetch raw HTML

```bash
# Full range (1,812 months, ~10 hours at 20s delay)
python3 -m scraper.lunisolar.fetch

# Fetch a subset to test
python3 -m scraper.lunisolar.fetch --start-year 2024 --end-year 2025 --delay 5

# Fetch specific day pages
python3 -m scraper.lunisolar.fetch --fetch-days 2025-01-01 2025-01-15

# Resume: already-downloaded months are skipped automatically
# Ctrl+C: graceful shutdown, restart to continue
```

### 2. Parse HTML to CSV

```bash
python3 -m scraper.lunisolar.parse
```

Produces `data/lunisolar/parsed/drikpanchang.csv` with columns:
`year,month,day,tithi`

### 3. Compare against reference

```bash
# Default: compare against Moshier reference
python3 -m scraper.lunisolar.compare

# Compare against Swiss Ephemeris reference
python3 -m scraper.lunisolar.compare --ref validation/se/ref_1900_2050.csv
```

## Solar Calendar Scraping

### 1. Fetch raw HTML

```bash
# Fetch one calendar
python3 -m scraper.solar.fetch --calendar tamil
python3 -m scraper.solar.fetch --calendar bengali --start-year 2000 --end-year 2025 --delay 5

# Fetch all four calendars
python3 -m scraper.solar.fetch --calendar all
```

Each calendar: 1,812 pages, ~10 hours at 20s delay, ~360 MB storage.

### 2. Parse HTML to CSV

```bash
python3 -m scraper.solar.parse --calendar tamil
python3 -m scraper.solar.parse --calendar all
```

Produces `data/solar/parsed/{calendar}.csv` with columns matching our reference:
`month,year,length,greg_year,greg_month,greg_day,month_name`

### 3. Compare against reference

```bash
python3 -m scraper.solar.compare --calendar tamil
python3 -m scraper.solar.compare --calendar all
```

Compares Gregorian start date of each solar month between drikpanchang and
our computed reference (`validation/moshier/solar/{calendar}_months_1900_2050.csv`).

## Settings

All fetches use these cookies for New Delhi + Lahiri ayanamsa:
- `drik-school-name=amanta`
- `drik-geoname-id=1261481` (New Delhi)
- `drik-ayanamsha-type=chitra-paksha` (Lahiri)

## CAPTCHA Handling

Drikpanchang triggers CAPTCHAs after a hard limit of ~200 requests per
IP address, regardless of request delay. The CAPTCHA is detected by
response size (< 50 KB vs normal ~200 KB).

**Important findings:**
- The limit is per-IP, not per-session — session rotation alone does not help
- A 2-second delay between requests works fine (no benefit from longer delays)
- VPN/IP rotation is required after each ~200-request batch
- At 2s delay: ~400 seconds per batch, ~9 batches per calendar, ~1 hour total per calendar with VPN switching

The scrapers detect CAPTCHAs automatically and stop with a clear message.
Resume by switching IP and re-running (already-downloaded files are skipped).

## Validation Results

| Calendar | Pages | Match | Mismatch | Rate |
|----------|-------|-------|----------|------|
| Lunisolar | 1,812 | 55,117/55,152 days | 35 | 99.937% |
| Tamil | 1,812 | 1,811/1,811 months | 0 | 100.000% |
| Bengali | 1,812 | 1,811/1,811 months | 0 | 100.000% |
| Odia | 1,812 | 1,811/1,811 months | 0 | 100.000% |
| Malayalam | 1,812 | 1,811/1,811 months | 0 | 100.000% |

See `Docs/DRIKPANCHANG_VALIDATION.md` for full analysis of mismatches.

## Storage

| Calendar   | Pages | Storage |
|-----------|-------|---------|
| Lunisolar | 1,812 | 363 MB  |
| Tamil     | 1,812 | ~360 MB |
| Bengali   | 1,812 | ~360 MB |
| Odia      | 1,812 | ~360 MB |
| Malayalam  | 1,812 | ~360 MB |
| **Total** | **9,060** | **~1.8 GB** |

## Parser Notes

The lunisolar parser extracts tithi (1-30) per day from the `dpCellTithi`
element. Tithi is scheme-independent (same value regardless of Amanta/Purnimanta).

The solar parser extracts month boundaries by scanning for solar day 1 in
each Gregorian month page. It handles all four calendar-specific HTML
structures (different CSS classes, header formats, and month name mappings).
