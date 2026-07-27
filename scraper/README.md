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
# Full range (1,812 months, ~2.5 hours at the 5s default delay)
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

Each calendar: 1,812 pages, ~1 hour at the 2s default delay, ~360 MB storage.

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

These are set scoped to `COOKIE_DOMAIN` / `COOKIE_PATH` so the server's own
Set-Cookie headers replace them in place. Setting them with the default empty
domain leaves two jar entries per name (ours plus the server's echo), which
makes `dict(session.cookies)` raise `CookieConflictError` and makes it
ambiguous which value is sent on later requests.

### Server-side defaults worth knowing

The response also carries drikpanchang's own defaults. Two of them
independently corroborate calculation choices we arrived at empirically:

| Cookie | Value | Corroborates |
|--------|-------|--------------|
| `drik-sunrise-type` | `edges` | Upper-limb sunrise (`Docs/DRIKPANCHANG_VALIDATION.md`) |
| `drik-geo-elevation-status` | `disabled` | No horizon dip (`Docs/ELEVATION.md`) |

## CAPTCHA Handling

Drikpanchang serves a CAPTCHA page instead of content after a few hundred
consecutive requests. It is detected by response size: real pages are
150–250 KB, CAPTCHA pages ~2 KB, so `MIN_VALID_SIZE = 50000` separates them
cleanly.

**What the code does** (`common.py`):
- Rotates to a fresh `requests.Session()` every `SESSION_ROTATE_INTERVAL = 10`
  requests, proactively, before a block is hit
- On a CAPTCHA anyway: rotates the session and retries the page once; if it is
  still blocked, stops with a clear message
- Resume is automatic — already-downloaded files are skipped, so re-running
  continues where it left off

**The limit is per-IP, not per-session.** Settled empirically during the
Bengali Suryasiddhanta scrape on 2026-07-27, over four consecutive blocks:

| Block | Fetches on that IP | Session rotation | Cleared by |
|-------|-------------------|------------------|------------|
| 1 | 200 (188 + 12 probes) | failed | VPN reset |
| 2 | 199 | failed | VPN reset |
| 3 | 200 | failed | VPN reset |
| 4 | 200 | failed | VPN reset |

Every block hit at almost exactly 200 requests. In each case the fetcher
rotated to a brand-new `requests.Session()` — acquiring a fresh
`_DRIK_SESSION_ID` from the server — and was **still** served a CAPTCHA. Only
changing the outbound IP restored access.

So a full-range fetch of 1,812 pages needs roughly 9 VPN switches. Plan for it.

Note this contradicts an earlier claim in `Docs/DRIKPANCHANG_VALIDATION.md`
that the CAPTCHA was tied to the `_DRIK_SESSION_ID` cookie. The presence of
that cookie is real but irrelevant to the rate limit; that claim has been
corrected.

**Consequence for `SESSION_ROTATE_INTERVAL`:** proactively rotating the session
every 10 requests cannot prevent a per-IP block, so it buys nothing. It is
retained only because it is harmless and the reactive rotate-and-retry on
CAPTCHA is what produces the clean stop. A worthwhile improvement would be to
stop at ~195 requests and prompt for a VPN switch, rather than spending two
requests discovering the block.

A single cold-session fetch works with no CAPTCHA, no warm-up and no VPN
(verified 2026-07-27 against `tamil-month-panchangam.html`, HTTP 200,
213,889 bytes) — the limit only bites in bulk.

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
