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
import glob
import os
import re
import sys

from scraper.common import MAX_REQUESTS_PER_RUN, fetch_pages, install_signal_handlers
from scraper.solar.config import (
    ARITHMETICS,
    CALENDARS,
    DEFAULT_ARITHMETIC,
    DEFAULT_DELAY,
    DEFAULT_END_YEAR,
    DEFAULT_START_YEAR,
    SOLAR_URLS,
    provenance_token,
    raw_dir,
)


def verify_provenance(output_dir, calendar_type, arithmetic, sample=5):
    """Check downloaded pages really are the school we asked for.

    The page <h1> reads "... Panjika based on <School> for <City>".  If we
    asked for suryasiddhanta but the cookie failed to take, every page would
    silently be Bisuddhasiddhanta and the whole dataset would be wrong in a
    way no downstream check would catch.  Returns True if verified or if this
    calendar has no such title.
    """
    token = provenance_token(calendar_type, arithmetic)
    if token is None:
        return True

    files = sorted(glob.glob(os.path.join(output_dir, "*.html")))
    if not files:
        return True

    checked = files[:sample] + files[-sample:]
    bad = []
    for path in checked:
        with open(path, encoding="utf-8") as f:
            html = f.read()
        m = re.search(r"based on ([A-Za-z]+)", html)
        found = m.group(1) if m else "(none)"
        if found != token:
            bad.append((os.path.basename(path), found))

    if bad:
        print(f"\n  PROVENANCE FAILURE: expected {token!r}, got:", file=sys.stderr)
        for name, found in bad:
            print(f"    {name}: {found!r}", file=sys.stderr)
        return False

    print(f"  Provenance OK: {len(set(checked))} pages all 'based on {token}'")
    return True


def fetch_solar_pages(calendar_type, start_year, end_year, delay,
                      arithmetic=DEFAULT_ARITHMETIC,
                      max_requests=MAX_REQUESTS_PER_RUN):
    """Download solar calendar month pages for the given year range."""
    base_url = SOLAR_URLS[calendar_type]
    output_dir = raw_dir(calendar_type, arithmetic)

    targets = []
    for year in range(start_year, end_year + 1):
        for month in range(1, 13):
            targets.append(((year, month), f"{year:04d}-{month:02d}.html"))

    def url_fn(key):
        year, month = key
        return f"{base_url}?date=01/{month:02d}/{year:04d}"

    print(f"\n{'=' * 50}")
    print(f"Calendar:   {calendar_type}")
    print(f"Arithmetic: {arithmetic}")
    print(f"URL base:   {base_url}")
    print(f"Output:     {output_dir}")
    print(f"{'=' * 50}")
    fetch_pages(targets, output_dir, url_fn, delay,
                label=f"{calendar_type} month", arithmetic=arithmetic,
                max_requests=max_requests)
    verify_provenance(output_dir, calendar_type, arithmetic)


def main():
    install_signal_handlers()

    parser = argparse.ArgumentParser(description="Fetch drikpanchang.com solar calendar HTML")
    parser.add_argument("--calendar", required=True,
                        choices=CALENDARS + ["all"],
                        help="Solar calendar to fetch (or 'all')")
    parser.add_argument("--start-year", type=int, default=DEFAULT_START_YEAR)
    parser.add_argument("--end-year", type=int, default=DEFAULT_END_YEAR)
    parser.add_argument("--delay", type=float, default=DEFAULT_DELAY,
                        help=f"Seconds between requests (default: {DEFAULT_DELAY})")
    parser.add_argument("--arithmetic", default=DEFAULT_ARITHMETIC,
                        choices=ARITHMETICS,
                        help=f"Panchang arithmetic school (default: {DEFAULT_ARITHMETIC}). "
                             "'modern' is Bisuddhasiddhanta, 'suryasiddhanta' is the "
                             "traditional panjika. Each writes to its own directory.")
    parser.add_argument("--max-requests", type=int, default=MAX_REQUESTS_PER_RUN,
                        help=f"Stop cleanly after N requests (default: "
                             f"{MAX_REQUESTS_PER_RUN}; site blocks per-IP at 200). "
                             "0 disables the cap.")
    args = parser.parse_args()

    calendars = CALENDARS if args.calendar == "all" else [args.calendar]
    for cal in calendars:
        fetch_solar_pages(cal, args.start_year, args.end_year, args.delay,
                          args.arithmetic, args.max_requests)


if __name__ == "__main__":
    main()
