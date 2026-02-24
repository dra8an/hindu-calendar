#!/usr/bin/env python3
"""Parse saved drikpanchang.com month panchang HTML into a CSV.

Extracts tithi (1-30) for each day from month panchang HTML pages.

Usage:
    python3 -m scraper.lunisolar.parse
    python3 -m scraper.lunisolar.parse --start-year 2024 --end-year 2025
    python3 -m scraper.lunisolar.parse --output /path/to/output.csv
"""

import argparse
import calendar
import csv
import os
import sys

from bs4 import BeautifulSoup

from scraper.lunisolar.config import PARSED_CSV, RAW_DIR, tithi_to_number


def parse_month_html(html_path, year, month):
    """Parse a month panchang HTML file. Returns list of (day, tithi_num)."""
    with open(html_path, "r", encoding="utf-8") as f:
        html = f.read()

    soup = BeautifulSoup(html, "html.parser")

    days_in_month = calendar.monthrange(year, month)[1]
    day_tithis = {}  # greg_day -> tithi_num

    cells = soup.find_all("div", class_="dpMonthGridCell")
    for cell in cells:
        classes = cell.get("class", [])

        if "dpInert" in classes:
            continue

        # Gregorian day from dpBigDate (first text node: "11\n<span>Sat</span>")
        big_date_el = cell.find(class_="dpBigDate")
        if not big_date_el or not big_date_el.contents:
            continue

        greg_day_text = big_date_el.contents[0].strip()
        try:
            greg_day = int(greg_day_text)
        except ValueError:
            print(f"  WARNING: non-numeric day {greg_day_text!r} in {html_path}",
                  file=sys.stderr)
            continue

        # Tithi from dpCellTithi: "TithiName Paksha" e.g. "Dwitiya Shukla"
        tithi_el = cell.find(class_="dpCellTithi")
        if not tithi_el:
            print(f"  WARNING: no tithi for day {greg_day} in {html_path}",
                  file=sys.stderr)
            continue

        tithi_text = tithi_el.get_text().strip()
        parts = tithi_text.split()
        if len(parts) != 2:
            print(f"  WARNING: unexpected tithi format {tithi_text!r} for day {greg_day}",
                  file=sys.stderr)
            continue

        tithi_name, paksha = parts
        try:
            tithi_num = tithi_to_number(tithi_name, paksha)
        except ValueError as e:
            print(f"  WARNING: {e} for day {greg_day} in {html_path}", file=sys.stderr)
            continue

        # Adhika tithis: same greg_day appears in two cells with same tithi -- keep first
        if greg_day not in day_tithis:
            day_tithis[greg_day] = tithi_num

    # Build ordered list
    days = []
    for d in range(1, days_in_month + 1):
        if d in day_tithis:
            days.append((d, day_tithis[d]))
        else:
            print(f"  WARNING: missing day {d} in {year:04d}-{month:02d}", file=sys.stderr)
            days.append((d, None))

    return days


def build_csv(start_year, end_year, output_path):
    """Parse all available month HTML files and build the CSV."""
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    all_data = {}
    missing = 0

    for year in range(start_year, end_year + 1):
        for month in range(1, 13):
            html_path = os.path.join(RAW_DIR, f"{year:04d}-{month:02d}.html")
            if not os.path.exists(html_path):
                missing += 1
                continue

            data = parse_month_html(html_path, year, month)
            all_data[(year, month)] = data

    print(f"Parsed {len(all_data)} months, {missing} missing")

    with open(output_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["year", "month", "day", "tithi"])
        for (year, month), days in sorted(all_data.items()):
            for day, tithi in days:
                writer.writerow([year, month, day, tithi if tithi is not None else ""])

    total_days = sum(len(d) for d in all_data.values())
    total_with_tithi = sum(1 for d in all_data.values() for _, t in d if t is not None)
    print(f"Total days: {total_days}, with tithi: {total_with_tithi}")
    print(f"Output: {output_path}")


def main():
    parser = argparse.ArgumentParser(description="Parse drikpanchang lunisolar HTML -> CSV")
    parser.add_argument("--start-year", type=int, default=1900)
    parser.add_argument("--end-year", type=int, default=2050)
    parser.add_argument("--output", default=PARSED_CSV)
    args = parser.parse_args()

    build_csv(args.start_year, args.end_year, args.output)


if __name__ == "__main__":
    main()
