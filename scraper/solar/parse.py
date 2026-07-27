#!/usr/bin/env python3
"""Parse saved drikpanchang.com solar calendar HTML into CSVs.

Extracts solar month boundaries from monthly HTML pages. Each page has:
  - Header: "Month1 - Month2 (AltSpelling) Year" in dpSmallText
  - Grid cells: dpBigDate (Gregorian day) + dpSmallDate (solar day 1-31)

When dpSmallDate == 1, that Gregorian day starts the new solar month (Month2
from the header). Aggregates across all pages to build the month-start table.

Output CSV format (matching our reference):
    month,year,length,greg_year,greg_month,greg_day,month_name

Usage:
    python3 -m scraper.solar.parse --calendar tamil
    python3 -m scraper.solar.parse --calendar all
    python3 -m scraper.solar.parse --calendar tamil --start-year 2024 --end-year 2025
"""

import argparse
import csv
import datetime
import os
import re
import sys

from bs4 import BeautifulSoup

from scraper.solar.config import (
    ARITHMETICS,
    CALENDARS,
    DEFAULT_ARITHMETIC,
    MONTH_NAME_TO_NUM,
    normalize_month_name,
    parsed_csv,
    provenance_token,
    raw_dir,
)


def check_provenance(html, expected_token):
    """Return the page's school token, or None if the page has no title.

    Every page states its own school in the <h1>: "... based on <School> for
    <City>".  Verifying it per page means a mixed directory is caught at parse
    time instead of silently producing a blended CSV.
    """
    m = re.search(r"based on ([A-Za-z]+)", html)
    if not m:
        return None
    return m.group(1)


def _parse_header(soup, calendar_type):
    """Extract month names and year from page header.

    Header format: "Month1 - Month2 (AltSpelling) Year"
    e.g. "Poush - Magh (Maagh) 1431"
         "Maargazhi - Thai 1946"

    Returns (month_before, month_after, year) with normalized names,
    or (None, None, None) if parsing fails.
    """
    el = soup.find(class_="dpSmallText")
    if not el:
        return None, None, None

    text = el.get_text().strip()

    # Remove parenthetical alternate spellings: "Magh (Maagh)" -> "Magh"
    text = re.sub(r"\s*\([^)]*\)", "", text)

    # Split on " - "
    parts = text.split(" - ")
    if len(parts) != 2:
        return None, None, None

    # Each side may have "MonthName Year" — strip trailing year from both
    before_tokens = parts[0].strip().split()
    after_tokens = parts[1].strip().split()

    if not before_tokens or not after_tokens:
        return None, None, None

    # Year is the last token if numeric
    if before_tokens[-1].isdigit():
        month_before_raw = " ".join(before_tokens[:-1])
    else:
        month_before_raw = " ".join(before_tokens)

    if after_tokens[-1].isdigit():
        month_after_raw = " ".join(after_tokens[:-1])
        year = int(after_tokens[-1])
    else:
        month_after_raw = " ".join(after_tokens)
        year = None

    month_before = normalize_month_name(month_before_raw, calendar_type)
    month_after = normalize_month_name(month_after_raw, calendar_type)

    if month_before is None:
        print(f"  WARNING: unrecognized month_before {month_before_raw!r} in header {text!r}",
              file=sys.stderr)
    if month_after is None:
        print(f"  WARNING: unrecognized month_after {month_after_raw!r} in header {text!r}",
              file=sys.stderr)

    return month_before, month_after, year


def parse_month_html(html_path, greg_year, greg_month, calendar_type,
                     expected_token=None):
    """Parse a solar calendar month page.

    Returns a list of month-start records:
        [(month_name, solar_year, greg_year, greg_month, greg_day), ...]

    Typically 0 or 1 records per page (one month transition per Gregorian month).
    Rarely 2 if two solar months start within the same Gregorian month.

    If expected_token is given, the page's declared school must match it or
    ValueError is raised, so a directory holding a mix of Bisuddhasiddhanta and
    Suryasiddhanta pages fails loudly instead of producing a blended CSV.
    """
    with open(html_path, "r", encoding="utf-8") as f:
        html = f.read()

    if expected_token is not None:
        found = check_provenance(html, expected_token)
        if found is not None and found != expected_token:
            raise ValueError(
                f"{os.path.basename(html_path)}: expected school "
                f"{expected_token!r}, page says {found!r}")

    soup = BeautifulSoup(html, "html.parser")

    # Get month names and year from header
    month_before, month_after, solar_year = _parse_header(soup, calendar_type)

    results = []

    # Scan grid cells for solar day = 1 (month transition)
    cells = soup.find_all("div", class_="dpMonthGridCell")
    found_transition = False

    for cell in cells:
        classes = cell.get("class", [])
        if "dpInert" in classes:
            continue

        # Gregorian day from dpBigDate
        big_date_el = cell.find(class_="dpBigDate")
        if not big_date_el or not big_date_el.contents:
            continue

        greg_day_text = big_date_el.contents[0].strip()
        try:
            greg_day = int(greg_day_text)
        except ValueError:
            continue

        # Solar day from dpSmallDate
        small_date_el = cell.find(class_="dpSmallDate")
        if not small_date_el:
            continue

        solar_day_text = small_date_el.get_text().strip()
        try:
            solar_day = int(solar_day_text)
        except ValueError:
            continue

        if solar_day == 1:
            if not found_transition:
                # First transition: this is month_after
                month_name = month_after
                found_transition = True
            else:
                # Second transition in same page (rare: two month starts in one Gregorian month)
                # This would mean month_after already consumed, and a third month starts.
                # The header only shows two months, so we log a warning.
                print(f"  WARNING: second month transition at {greg_year}-{greg_month:02d}-{greg_day:02d} "
                      f"in {html_path}", file=sys.stderr)
                month_name = None

            if month_name:
                results.append((month_name, solar_year, greg_year, greg_month, greg_day))

    return results


def find_month_starts(calendar_type, start_year, end_year,
                      arithmetic=DEFAULT_ARITHMETIC):
    """Scan all HTML pages and find solar month start dates.

    Returns list of (solar_month_num, solar_year, greg_year, greg_month, greg_day, month_name)
    sorted by Gregorian date.

    Raises ValueError if any page declares a school other than the one
    requested (see parse_month_html).
    """
    html_dir = raw_dir(calendar_type, arithmetic)
    name_to_num = MONTH_NAME_TO_NUM[calendar_type]
    expected_token = provenance_token(calendar_type, arithmetic)

    month_starts = []
    pages_parsed = 0
    pages_missing = 0
    provenance_failures = []

    for year in range(start_year, end_year + 1):
        for month in range(1, 13):
            html_path = os.path.join(html_dir, f"{year:04d}-{month:02d}.html")
            if not os.path.exists(html_path):
                pages_missing += 1
                continue

            pages_parsed += 1
            try:
                results = parse_month_html(html_path, year, month, calendar_type,
                                           expected_token)
            except ValueError as e:
                provenance_failures.append(str(e))
                continue
            for month_name, solar_year, gy, gm, gd in results:
                month_num = name_to_num.get(month_name)
                if month_num is None:
                    print(f"  WARNING: unknown month {month_name!r} at {gy}-{gm:02d}-{gd:02d}",
                          file=sys.stderr)
                    continue
                month_starts.append((month_num, solar_year, gy, gm, gd, month_name))

    # Sort by Gregorian date
    month_starts.sort(key=lambda e: (e[2], e[3], e[4]))

    # Deduplicate (same month start seen from overlapping pages)
    seen = set()
    unique = []
    for entry in month_starts:
        key = (entry[2], entry[3], entry[4])  # greg date
        if key not in seen:
            seen.add(key)
            unique.append(entry)

    if provenance_failures:
        print(f"  PROVENANCE: {len(provenance_failures)} page(s) declared the "
              f"wrong school:", file=sys.stderr)
        for msg in provenance_failures[:10]:
            print(f"    {msg}", file=sys.stderr)
        if len(provenance_failures) > 10:
            print(f"    ... and {len(provenance_failures) - 10} more", file=sys.stderr)
        raise ValueError(
            f"{len(provenance_failures)} page(s) in {html_dir} are not "
            f"{expected_token!r} — refusing to build a mixed CSV")

    label = f" [{expected_token}]" if expected_token else ""
    print(f"  Pages parsed: {pages_parsed}, missing: {pages_missing}{label}")
    return unique


def compute_lengths(month_starts):
    """Compute month lengths from consecutive start dates.

    Returns list of (month_num, solar_year, length, greg_year, greg_month, greg_day, month_name).
    """
    result = []
    for i in range(len(month_starts)):
        month_num, solar_year, gy, gm, gd, month_name = month_starts[i]

        if i + 1 < len(month_starts):
            _, _, ny, nm, nd, _ = month_starts[i + 1]
            start = datetime.date(gy, gm, gd)
            end = datetime.date(ny, nm, nd)
            length = (end - start).days
        else:
            length = 0  # Unknown for last entry

        result.append((month_num, solar_year, length, gy, gm, gd, month_name))

    return result


def build_csv(calendar_type, start_year, end_year, arithmetic=DEFAULT_ARITHMETIC):
    """Build parsed solar CSV for a calendar + arithmetic."""
    output_path = parsed_csv(calendar_type, arithmetic)
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    print(f"Parsing {calendar_type} [{arithmetic}] solar pages "
          f"({start_year}-{end_year})...")
    month_starts = find_month_starts(calendar_type, start_year, end_year, arithmetic)
    print(f"  Found {len(month_starts)} month starts")

    if not month_starts:
        print(f"  WARNING: no month starts found for {calendar_type}!", file=sys.stderr)
        print(f"  This likely means the HTML structure doesn't match the parser.")
        print(f"  Download sample pages and examine the DOM structure.")
        return

    entries = compute_lengths(month_starts)

    with open(output_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["month", "year", "length", "greg_year", "greg_month", "greg_day", "month_name"])
        for month_num, solar_year, length, gy, gm, gd, month_name in entries:
            writer.writerow([month_num, solar_year if solar_year else "",
                             length, gy, gm, gd, month_name])

    print(f"  Output: {output_path} ({len(entries)} rows)")


def main():
    parser = argparse.ArgumentParser(description="Parse drikpanchang solar HTML -> CSV")
    parser.add_argument("--calendar", required=True,
                        choices=CALENDARS + ["all"])
    parser.add_argument("--start-year", type=int, default=1900)
    parser.add_argument("--end-year", type=int, default=2050)
    parser.add_argument("--arithmetic", default=DEFAULT_ARITHMETIC,
                        choices=ARITHMETICS,
                        help=f"Panchang arithmetic school (default: {DEFAULT_ARITHMETIC})")
    args = parser.parse_args()

    calendars = CALENDARS if args.calendar == "all" else [args.calendar]
    for cal in calendars:
        build_csv(cal, args.start_year, args.end_year, args.arithmetic)


if __name__ == "__main__":
    main()
