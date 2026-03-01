#!/usr/bin/env python3
"""Fetch raw HTML from drikpanchang.com month panchang pages.

Downloads month panchang pages for every Gregorian month in the specified
year range. Saves raw HTML to scraper/data/lunisolar/raw/YYYY-MM.html
(or scraper/data/lunisolar_{location}/raw/ for non-Delhi locations).
Skips already-downloaded files (resume capability).

Usage:
    python3 -m scraper.lunisolar.fetch
    python3 -m scraper.lunisolar.fetch --location nyc --start-year 2024 --end-year 2025
    python3 -m scraper.lunisolar.fetch --fetch-days 2025-01-01 2025-01-15
"""

import argparse
import os
import time

from scraper.common import (
    fetch_pages,
    fetch_url,
    install_signal_handlers,
    is_shutdown,
    new_session,
)
from scraper.lunisolar.config import (
    BASE_URL_DAY,
    BASE_URL_MONTH,
    DEFAULT_DELAY,
    DEFAULT_END_YEAR,
    DEFAULT_START_YEAR,
    get_paths,
)


def fetch_month_pages(start_year, end_year, delay, location="delhi"):
    """Download month panchang pages for the given year range."""
    paths = get_paths(location)
    raw_dir = paths["raw_dir"]

    targets = []
    for year in range(start_year, end_year + 1):
        for month in range(1, 13):
            targets.append(((year, month), f"{year:04d}-{month:02d}.html"))

    def url_fn(key):
        year, month = key
        return f"{BASE_URL_MONTH}?date=01/{month:02d}/{year:04d}"

    fetch_pages(targets, raw_dir, url_fn, delay, label="month", location=location)


def fetch_day_pages(dates, delay, location="delhi"):
    """Download day panchang pages for specific dates.

    Args:
        dates: list of "YYYY-MM-DD" strings
    """
    paths = get_paths(location)
    raw_day_dir = os.path.join(paths["data_dir"], "raw_day")
    os.makedirs(raw_day_dir, exist_ok=True)

    session = new_session(location)
    downloaded = 0
    failed = 0

    for i, date_str in enumerate(dates):
        if is_shutdown():
            break

        year, month, day = date_str.split("-")
        output_path = os.path.join(raw_day_dir, f"{date_str}.html")
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

        if i < len(dates) - 1 and not is_shutdown():
            time.sleep(delay)

    print(f"\nDone: downloaded {downloaded}, failed {failed}")


def main():
    install_signal_handlers()

    parser = argparse.ArgumentParser(description="Fetch drikpanchang.com lunisolar HTML pages")
    parser.add_argument("--location", default="delhi", choices=["delhi", "nyc"],
                        help="Location for drikpanchang cookies (default: delhi)")
    parser.add_argument("--start-year", type=int, default=DEFAULT_START_YEAR)
    parser.add_argument("--end-year", type=int, default=DEFAULT_END_YEAR)
    parser.add_argument("--delay", type=float, default=DEFAULT_DELAY,
                        help="Seconds between requests (default: 5)")
    parser.add_argument("--fetch-days", nargs="*", metavar="YYYY-MM-DD",
                        help="Fetch specific day pages instead of month pages")
    args = parser.parse_args()

    if args.fetch_days:
        fetch_day_pages(args.fetch_days, args.delay, args.location)
    else:
        fetch_month_pages(args.start_year, args.end_year, args.delay, args.location)


if __name__ == "__main__":
    main()
