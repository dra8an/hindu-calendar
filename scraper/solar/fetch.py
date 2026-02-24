#!/usr/bin/env python3
"""Fetch raw HTML from drikpanchang.com solar calendar pages.

Downloads month pages for every Gregorian month in the specified year range
for a given solar calendar (Tamil, Bengali, Odia, Malayalam).

Usage:
    python3 -m scraper.solar.fetch --calendar tamil
    python3 -m scraper.solar.fetch --calendar bengali --start-year 2000 --end-year 2025 --delay 5
    python3 -m scraper.solar.fetch --calendar all
"""

import argparse

from scraper.common import fetch_pages, install_signal_handlers
from scraper.solar.config import (
    CALENDARS,
    DEFAULT_DELAY,
    DEFAULT_END_YEAR,
    DEFAULT_START_YEAR,
    SOLAR_URLS,
    raw_dir,
)


def fetch_solar_pages(calendar_type, start_year, end_year, delay):
    """Download solar calendar month pages for the given year range."""
    base_url = SOLAR_URLS[calendar_type]
    output_dir = raw_dir(calendar_type)

    targets = []
    for year in range(start_year, end_year + 1):
        for month in range(1, 13):
            targets.append(((year, month), f"{year:04d}-{month:02d}.html"))

    def url_fn(key):
        year, month = key
        return f"{base_url}?date=01/{month:02d}/{year:04d}"

    print(f"\n{'=' * 50}")
    print(f"Calendar: {calendar_type}")
    print(f"URL base: {base_url}")
    print(f"{'=' * 50}")
    fetch_pages(targets, output_dir, url_fn, delay, label=f"{calendar_type} month")


def main():
    install_signal_handlers()

    parser = argparse.ArgumentParser(description="Fetch drikpanchang.com solar calendar HTML")
    parser.add_argument("--calendar", required=True,
                        choices=CALENDARS + ["all"],
                        help="Solar calendar to fetch (or 'all')")
    parser.add_argument("--start-year", type=int, default=DEFAULT_START_YEAR)
    parser.add_argument("--end-year", type=int, default=DEFAULT_END_YEAR)
    parser.add_argument("--delay", type=float, default=DEFAULT_DELAY,
                        help="Seconds between requests (default: 20)")
    args = parser.parse_args()

    calendars = CALENDARS if args.calendar == "all" else [args.calendar]
    for cal in calendars:
        fetch_solar_pages(cal, args.start_year, args.end_year, args.delay)


if __name__ == "__main__":
    main()
