#!/usr/bin/env python3
"""Compare Bengali solar calendar for Oct 1976: our C code vs drikpanchang.com.

Parses the drikpanchang HTML to extract every grid cell's solar day and month,
then compares day-by-day against our C code's output.
"""

import re
import subprocess
import sys
from bs4 import BeautifulSoup

HTML_PATH = "scraper/data/solar/raw/bengali/1976-10.html"
C_BINARY = "./hindu-calendar"

# Drikpanchang name -> our canonical name (Bengali)
DRIK_ALIASES = {
    "baishakh": "Boishakh", "boishakh": "Boishakh", "baisakh": "Boishakh",
    "jyaistha": "Joishtho", "joishtho": "Joishtho", "jyeshtha": "Joishtho",
    "ashadh": "Asharh", "asharh": "Asharh", "ashadha": "Asharh",
    "shravan": "Srabon", "srabon": "Srabon", "shravana": "Srabon", "shraban": "Srabon",
    "bhadra": "Bhadro", "bhadro": "Bhadro", "bhadrapada": "Bhadro",
    "ashwin": "Ashshin", "ashshin": "Ashshin", "ashvin": "Ashshin",
    "kartik": "Kartik",
    "agrahayana": "Ogrohaeon", "ogrohaeon": "Ogrohaeon", "agrahayan": "Ogrohaeon",
    "paush": "Poush", "poush": "Poush", "pausha": "Poush",
    "magh": "Magh", "magha": "Magh", "maagh": "Magh",
    "phalgun": "Falgun", "falgun": "Falgun", "phalguna": "Falgun",
    "chaitra": "Choitro", "choitro": "Choitro",
}


def normalize_month(name):
    """Normalize a drikpanchang Bengali month name to our canonical name."""
    if name is None:
        return None
    canonical = DRIK_ALIASES.get(name.lower())
    return canonical if canonical else name


def parse_header(soup):
    """Extract month names and year from page header.

    Header format: "Month1 - Month2 (AltSpelling) Year"
    Returns (month_before, month_after, year).
    """
    el = soup.find(class_="dpSmallText")
    if not el:
        return None, None, None

    text = el.get_text().strip()
    # Remove parenthetical alternate spellings
    text = re.sub(r"\s*\([^)]*\)", "", text)

    parts = text.split(" - ")
    if len(parts) != 2:
        return None, None, None

    before_tokens = parts[0].strip().split()
    after_tokens = parts[1].strip().split()

    if not before_tokens or not after_tokens:
        return None, None, None

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

    return normalize_month(month_before_raw), normalize_month(month_after_raw), year


def parse_all_cells(html_path):
    """Parse every grid cell and return list of (greg_day, solar_day, month_name).

    NOTE: The DOM order of cells may not be chronological. Drikpanchang uses a
    transposed calendar grid where the last day of the month can appear in the
    first row (sharing the same weekday slot as a pre-month inert cell). We must
    sort by Gregorian day first, then assign months based on where solar_day=1
    falls in chronological order.
    """
    with open(html_path, "r", encoding="utf-8") as f:
        html = f.read()

    soup = BeautifulSoup(html, "html.parser")

    month_before, month_after, solar_year = parse_header(soup)
    print(f"Header: {month_before} - {month_after} {solar_year}")

    cells = soup.find_all("div", class_="dpMonthGridCell")
    raw_pairs = []  # (greg_day, solar_day)

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

        raw_pairs.append((greg_day, solar_day))

    # Sort by Gregorian day (critical: DOM order may not be chronological)
    raw_pairs.sort(key=lambda x: x[0])

    # Find transition point: the Gregorian day where solar_day == 1
    transition_greg_day = None
    for greg_day, solar_day in raw_pairs:
        if solar_day == 1:
            transition_greg_day = greg_day
            break

    # Assign month names based on whether we're before or after transition
    results = []
    for greg_day, solar_day in raw_pairs:
        if transition_greg_day is not None and greg_day >= transition_greg_day:
            month_name = month_after
        else:
            month_name = month_before
        results.append((greg_day, solar_day, month_name))

    return results, solar_year


def parse_c_output():
    """Run our C code and parse its output.

    Returns list of (greg_day, solar_day, month_name).
    """
    result = subprocess.run(
        [C_BINARY, "-s", "bengali", "-y", "1976", "-m", "10"],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print(f"C binary error: {result.stderr}", file=sys.stderr)
        sys.exit(1)

    entries = []
    for line in result.stdout.strip().split("\n"):
        # Format: "1976-10-DD   Day   MonthName DD, YYYY"
        line = line.strip()
        if not line or line.startswith("Bengali") or line.startswith("Gregorian") or line.startswith("Date") or line.startswith("---"):
            continue

        parts = line.split()
        if len(parts) < 5:
            continue

        date_str = parts[0]  # "1976-10-DD"
        greg_day = int(date_str.split("-")[2])
        month_name = parts[2]  # e.g. "Ashshin" or "Kartik"
        solar_day_str = parts[3].rstrip(",")  # e.g. "15,"
        solar_day = int(solar_day_str)

        entries.append((greg_day, solar_day, month_name))

    return entries


def main():
    print("=" * 78)
    print("Bengali Solar Calendar Comparison: Oct 1976")
    print("Our C code vs drikpanchang.com")
    print("=" * 78)
    print()

    # Parse drikpanchang HTML
    print("--- Parsing drikpanchang HTML ---")
    drik_data, drik_year = parse_all_cells(HTML_PATH)
    print(f"Extracted {len(drik_data)} days")
    print()

    # Parse our C code output
    print("--- Running our C code ---")
    our_data = parse_c_output()
    print(f"Got {len(our_data)} days")
    print()

    # Build lookup dicts
    drik_by_day = {d[0]: d for d in drik_data}
    our_by_day = {d[0]: d for d in our_data}

    # Print comparison table
    all_days = sorted(set(list(drik_by_day.keys()) + list(our_by_day.keys())))

    print(f"{'Date':>12}  {'Our Code':>20}  {'Drikpanchang':>20}  {'Status'}")
    print(f"{'-'*12}  {'-'*20}  {'-'*20}  {'-'*10}")

    mismatches = 0
    for day in all_days:
        date_str = f"1976-10-{day:02d}"

        if day in our_by_day:
            o = our_by_day[day]
            our_str = f"{o[2]} {o[1]}"
        else:
            our_str = "MISSING"

        if day in drik_by_day:
            d = drik_by_day[day]
            drik_str = f"{d[2]} {d[1]}"
        else:
            drik_str = "MISSING"

        if day in our_by_day and day in drik_by_day:
            o = our_by_day[day]
            d = drik_by_day[day]
            if o[1] == d[1] and o[2] == d[2]:
                status = "OK"
            else:
                status = "MISMATCH"
                mismatches += 1
        else:
            status = "MISSING"
            mismatches += 1

        marker = " ***" if status != "OK" else ""
        print(f"{date_str:>12}  {our_str:>20}  {drik_str:>20}  {status}{marker}")

    print()
    print(f"Total days: {len(all_days)}")
    print(f"Matches: {len(all_days) - mismatches}")
    print(f"Mismatches: {mismatches}")

    if mismatches > 0:
        print()
        print("--- Mismatch Analysis ---")
        print("Checking if day numbering stays off by 1 after the boundary...")
        print()
        for day in all_days:
            if day not in our_by_day or day not in drik_by_day:
                continue
            o = our_by_day[day]
            d = drik_by_day[day]
            if o[1] != d[1] or o[2] != d[2]:
                this_date = f"1976-10-{day:02d}"
                day_diff = o[1] - d[1]
                month_diff = "same month" if o[2] == d[2] else f"ours={o[2]}, drik={d[2]}"
                print(f"  {this_date}: our day {o[1]:2d} vs drik day {d[1]:2d} (diff={day_diff:+d}), {month_diff}")


if __name__ == "__main__":
    main()
