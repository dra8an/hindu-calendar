#!/usr/bin/env python3
"""Phase 1: Fetch raw HTML from drikpanchang.com and save locally.

Downloads month panchang pages for every Gregorian month in the specified
year range. Saves raw HTML to scraper/data/raw/month/YYYY-MM.html.
Skips already-downloaded files (resume capability).

Usage:
    python3 fetch.py                          # Full range 1900-2050, 20s delay
    python3 fetch.py --start-year 2024 --end-year 2025 --delay 5
    python3 fetch.py --fetch-days 2025-01-01 2025-01-15   # Fetch specific day pages
"""

import argparse
import os
import signal
import sys
import time

import requests

from config import (
    BASE_URL_DAY,
    BASE_URL_MONTH,
    COOKIES,
    DEFAULT_DELAY,
    DEFAULT_END_YEAR,
    DEFAULT_START_YEAR,
    HEADERS,
    RAW_DAY_DIR,
    RAW_MONTH_DIR,
)

# Graceful shutdown flag
_shutdown = False


def _signal_handler(signum, frame):
    global _shutdown
    _shutdown = True
    print("\nShutdown requested, finishing current download...")


MIN_VALID_SIZE = 50000  # Normal pages are 150-250 KB; CAPTCHA pages are ~2 KB


def fetch_url(url, output_path, session):
    """Fetch a URL and save to disk. Returns 'ok', 'captcha', or 'error'."""
    try:
        resp = session.get(url, timeout=60)
        resp.raise_for_status()
        content = resp.text
        if len(content) < MIN_VALID_SIZE:
            print(f"  CAPTCHA detected ({len(content)} bytes), stopping.")
            return "captcha"
        with open(output_path, "w", encoding="utf-8") as f:
            f.write(content)
        return "ok"
    except requests.RequestException as e:
        print(f"  ERROR: {e}")
        return "error"


def fetch_month_pages(start_year, end_year, delay):
    """Download month panchang pages for the given year range."""
    os.makedirs(RAW_MONTH_DIR, exist_ok=True)

    # Build list of (year, month) pairs
    months = []
    for year in range(start_year, end_year + 1):
        for month in range(1, 13):
            months.append((year, month))

    # Count already downloaded
    existing = sum(
        1
        for y, m in months
        if os.path.exists(os.path.join(RAW_MONTH_DIR, f"{y:04d}-{m:02d}.html"))
    )
    remaining = len(months) - existing
    print(f"Total months: {len(months)}, already downloaded: {existing}, remaining: {remaining}")

    if remaining == 0:
        print("All months already downloaded.")
        return

    eta_seconds = remaining * delay
    eta_hours = eta_seconds / 3600
    print(f"Estimated time: {eta_hours:.1f} hours at {delay}s delay")
    print()

    def new_session():
        s = requests.Session()
        s.headers.update(HEADERS)
        s.cookies.update(COOKIES)
        return s

    session = new_session()
    downloaded = 0
    failed = 0

    for year, month in months:
        if _shutdown:
            print(f"\nShutdown: downloaded {downloaded}, failed {failed}")
            break

        output_path = os.path.join(RAW_MONTH_DIR, f"{year:04d}-{month:02d}.html")
        if os.path.exists(output_path):
            continue

        url = f"{BASE_URL_MONTH}?date=01/{month:02d}/{year:04d}"
        remaining -= 1
        eta_min = remaining * delay / 60

        print(f"[{downloaded + failed + existing + 1}/{len(months)}] "
              f"Fetching {year:04d}-{month:02d} ... "
              f"(remaining: {remaining}, ETA: {eta_min:.0f}m)", end="", flush=True)

        result = fetch_url(url, output_path, session)
        if result == "ok":
            size_kb = os.path.getsize(output_path) / 1024
            print(f"  OK ({size_kb:.0f} KB)")
            downloaded += 1
        elif result == "captcha":
            print(f"\nCAPTCHA hit after {downloaded} fetches. Rotating session...")
            session = new_session()
            # Retry this one with the fresh session
            result = fetch_url(url, output_path, session)
            if result == "ok":
                size_kb = os.path.getsize(output_path) / 1024
                print(f"  Retry OK ({size_kb:.0f} KB)")
                downloaded += 1
            else:
                print(f"\nStill blocked after session rotation. Stopping.")
                break
        else:
            failed += 1

        if remaining > 0 and not _shutdown:
            time.sleep(delay)

    print(f"\nDone: downloaded {downloaded}, failed {failed}, total existing {existing + downloaded}")


def fetch_day_pages(dates, delay):
    """Download day panchang pages for specific dates.

    Args:
        dates: list of "YYYY-MM-DD" strings
    """
    os.makedirs(RAW_DAY_DIR, exist_ok=True)

    session = requests.Session()
    session.headers.update(HEADERS)
    session.cookies.update(COOKIES)

    downloaded = 0
    failed = 0

    for i, date_str in enumerate(dates):
        if _shutdown:
            break

        year, month, day = date_str.split("-")
        output_path = os.path.join(RAW_DAY_DIR, f"{date_str}.html")
        if os.path.exists(output_path):
            continue

        url = f"{BASE_URL_DAY}?date={day}/{month}/{year}"
        print(f"[{i + 1}/{len(dates)}] Fetching day {date_str} ...", end="", flush=True)

        result = fetch_url(url, output_path, session)
        if result == "ok":
            size_kb = os.path.getsize(output_path) / 1024
            print(f"  OK ({size_kb:.0f} KB)")
            downloaded += 1
        elif result == "captcha":
            print(f"\nStopping due to CAPTCHA. Try again later with a longer --delay.")
            break
        else:
            failed += 1

        if i < len(dates) - 1 and not _shutdown:
            time.sleep(delay)

    print(f"\nDone: downloaded {downloaded}, failed {failed}")


def main():
    signal.signal(signal.SIGINT, _signal_handler)
    signal.signal(signal.SIGTERM, _signal_handler)

    parser = argparse.ArgumentParser(description="Fetch drikpanchang.com HTML pages")
    parser.add_argument("--start-year", type=int, default=DEFAULT_START_YEAR)
    parser.add_argument("--end-year", type=int, default=DEFAULT_END_YEAR)
    parser.add_argument("--delay", type=float, default=DEFAULT_DELAY,
                        help="Seconds between requests (default: 20)")
    parser.add_argument("--fetch-days", nargs="*", metavar="YYYY-MM-DD",
                        help="Fetch specific day pages instead of month pages")
    args = parser.parse_args()

    if args.fetch_days:
        fetch_day_pages(args.fetch_days, args.delay)
    else:
        fetch_month_pages(args.start_year, args.end_year, args.delay)


if __name__ == "__main__":
    main()
