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
    """Load reference CSV. Returns dict keyed by (greg_year, greg_month, greg_day).

    We key by Gregorian date of the solar month start to avoid era mismatches
    (e.g. Odia uses a different era than Saka on drikpanchang).
    """
    data = {}
    with open(path) as f:
        for row in csv.DictReader(f):
            gy, gm, gd = int(row["greg_year"]), int(row["greg_month"]), int(row["greg_day"])
            key = (gy, gm, gd)
            data[key] = {
                "solar_year": int(row["year"]),
                "solar_month": int(row["month"]),
                "month_name": row["month_name"],
                "length": int(row["length"]),
            }
    return data


def load_parsed_csv(path):
    """Load parsed drikpanchang solar CSV. Same key structure as ref."""
    data = {}
    with open(path) as f:
        for row in csv.DictReader(f):
            gy, gm, gd = int(row["greg_year"]), int(row["greg_month"]), int(row["greg_day"])
            key = (gy, gm, gd)
            data[key] = {
                "solar_year": int(row["year"]) if row["year"] else None,
                "solar_month": int(row["month"]),
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

    # Build parallel lists sorted by Gregorian date for alignment.
    # Match ref entries to parsed entries by finding the closest parsed date
    # for each ref solar month (same month_num).
    # Simple approach: for each ref entry, look for a parsed entry with same
    # greg_date (exact match) or ±1 day (mismatch).

    matched = 0
    mismatched = 0
    missing_in_ref = 0
    missing_in_parsed = 0
    mismatches = []

    # Index parsed by (greg_year, solar_month_num) for flexible matching
    parsed_by_gm = {}
    for key, p in parsed.items():
        gy, gm, gd = key
        idx_key = (gy, p["solar_month"])
        parsed_by_gm[idx_key] = (key, p)

    ref_matched_keys = set()
    parsed_matched_keys = set()

    for ref_key in sorted(ref):
        gy, gm, gd = ref_key
        r = ref[ref_key]

        if ref_key in parsed:
            # Exact Gregorian date match
            matched += 1
            ref_matched_keys.add(ref_key)
            parsed_matched_keys.add(ref_key)
        else:
            # Look for same solar month in same greg_year with different day
            idx_key = (gy, r["solar_month"])
            if idx_key in parsed_by_gm:
                p_key, p = parsed_by_gm[idx_key]
                mismatched += 1
                ref_matched_keys.add(ref_key)
                parsed_matched_keys.add(p_key)
                mismatches.append({
                    "solar_year": r["solar_year"],
                    "solar_month": r["solar_month"],
                    "month_name": r["month_name"],
                    "ref_date": f"{gy}-{gm:02d}-{gd:02d}",
                    "drik_date": f"{p_key[0]}-{p_key[1]:02d}-{p_key[2]:02d}",
                    "diff_days": _date_diff(
                        {"greg_year": gy, "greg_month": gm, "greg_day": gd},
                        {"greg_year": p_key[0], "greg_month": p_key[1], "greg_day": p_key[2]}),
                })
            else:
                missing_in_parsed += 1

    for key in parsed:
        if key not in parsed_matched_keys:
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
