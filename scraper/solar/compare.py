#!/usr/bin/env python3
"""Compare parsed drikpanchang solar months against our reference CSVs.

For each calendar, compares the Gregorian start date of each solar month
between drikpanchang (parsed) and our computed reference.

Usage:
    python3 -m scraper.solar.compare --calendar tamil
    python3 -m scraper.solar.compare --calendar all
"""

import argparse
import csv
import os

from scraper.solar.config import (
    CALENDARS,
    comparison_report,
    parsed_csv,
    ref_csv,
)


def load_ref_csv(path):
    """Load reference CSV. Returns dict of (greg_year, greg_month) -> row dict.

    We key by the Gregorian month of the solar month start, since each
    Gregorian month contains at most one solar month start.
    Actually, we need to key by solar month identity: (solar_year, solar_month_num).
    """
    data = {}
    with open(path) as f:
        for row in csv.DictReader(f):
            key = (int(row["year"]), int(row["month"]))  # solar_year, solar_month
            data[key] = {
                "greg_year": int(row["greg_year"]),
                "greg_month": int(row["greg_month"]),
                "greg_day": int(row["greg_day"]),
                "month_name": row["month_name"],
                "length": int(row["length"]),
            }
    return data


def load_parsed_csv(path):
    """Load parsed drikpanchang solar CSV. Same key structure as ref."""
    data = {}
    with open(path) as f:
        for row in csv.DictReader(f):
            year_str = row["year"]
            if not year_str:
                continue
            key = (int(year_str), int(row["month"]))
            data[key] = {
                "greg_year": int(row["greg_year"]),
                "greg_month": int(row["greg_month"]),
                "greg_day": int(row["greg_day"]),
                "month_name": row["month_name"],
                "length": int(row["length"]) if row["length"] and int(row["length"]) > 0 else None,
            }
    return data


def compare_calendar(calendar_type, report_path=None):
    """Compare parsed vs reference for one calendar."""
    parsed_path = parsed_csv(calendar_type)
    reference_path = ref_csv(calendar_type)

    if not os.path.exists(parsed_path):
        print(f"  Parsed CSV not found: {parsed_path}")
        print(f"  Run: python3 -m scraper.solar.parse --calendar {calendar_type}")
        return

    if not os.path.exists(reference_path):
        print(f"  Reference CSV not found: {reference_path}")
        return

    print(f"Calendar: {calendar_type}")
    print(f"Parsed:   {parsed_path}")
    print(f"Ref:      {reference_path}")

    parsed = load_parsed_csv(parsed_path)
    ref = load_ref_csv(reference_path)

    matched = 0
    mismatched = 0
    missing_in_ref = 0
    missing_in_parsed = 0
    mismatches = []

    for key in sorted(ref):
        solar_year, solar_month = key
        if key not in parsed:
            missing_in_parsed += 1
            continue

        p = parsed[key]
        r = ref[key]

        if (p["greg_year"] == r["greg_year"] and
            p["greg_month"] == r["greg_month"] and
            p["greg_day"] == r["greg_day"]):
            matched += 1
        else:
            mismatched += 1
            mismatches.append({
                "solar_year": solar_year,
                "solar_month": solar_month,
                "month_name": r["month_name"],
                "ref_date": f"{r['greg_year']}-{r['greg_month']:02d}-{r['greg_day']:02d}",
                "drik_date": f"{p['greg_year']}-{p['greg_month']:02d}-{p['greg_day']:02d}",
                "diff_days": _date_diff(r, p),
            })

    # Keys in parsed but not in ref
    for key in parsed:
        if key not in ref:
            missing_in_ref += 1

    total_compared = matched + mismatched
    pct = 100.0 * matched / total_compared if total_compared else 0

    lines = []
    lines.append(f"Solar month comparison: {calendar_type}")
    lines.append(f"{'=' * 60}")
    lines.append(f"Reference months:   {len(ref):>6,d}")
    lines.append(f"Parsed months:      {len(parsed):>6,d}")
    lines.append(f"Compared:           {total_compared:>6,d}")
    lines.append(f"Match:              {matched:>6,d}  ({pct:.3f}%)")
    lines.append(f"Mismatch:           {mismatched:>6,d}")
    lines.append(f"Missing in parsed:  {missing_in_parsed:>6,d}")
    lines.append(f"Missing in ref:     {missing_in_ref:>6,d}")
    lines.append("")

    if mismatches:
        lines.append("Mismatches:")
        lines.append(f"{'Year':>6s}  {'Month':>6s}  {'Name':>14s}  {'Ref Date':>12s}  {'Drik Date':>12s}  {'Diff':>6s}")
        lines.append("-" * 70)
        for m in mismatches:
            lines.append(
                f"{m['solar_year']:>6d}  {m['solar_month']:>6d}  {m['month_name']:>14s}  "
                f"{m['ref_date']:>12s}  {m['drik_date']:>12s}  {m['diff_days']:>+6d}"
            )

    report = "\n".join(lines)
    print(report)

    if report_path is None:
        report_path = comparison_report(calendar_type)
    os.makedirs(os.path.dirname(report_path), exist_ok=True)
    with open(report_path, "w") as f:
        f.write(report + "\n")
    print(f"\nReport: {report_path}\n")


def _date_diff(ref, parsed):
    """Compute difference in days between ref and parsed dates."""
    import datetime
    r = datetime.date(ref["greg_year"], ref["greg_month"], ref["greg_day"])
    p = datetime.date(parsed["greg_year"], parsed["greg_month"], parsed["greg_day"])
    return (p - r).days


def main():
    parser = argparse.ArgumentParser(description="Compare drikpanchang solar months vs reference")
    parser.add_argument("--calendar", required=True,
                        choices=CALENDARS + ["all"])
    args = parser.parse_args()

    calendars = CALENDARS if args.calendar == "all" else [args.calendar]
    for cal in calendars:
        compare_calendar(cal)


if __name__ == "__main__":
    main()
