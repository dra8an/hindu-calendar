#!/usr/bin/env python3
"""Compare drikpanchang-scraped tithis against our reference CSV.

Loads the parsed drikpanchang CSV (year,month,day,tithi) and compares
each tithi against our generated reference CSV.

Usage:
    python3 -m scraper.lunisolar.compare
    python3 -m scraper.lunisolar.compare --location nyc
    python3 -m scraper.lunisolar.compare --parsed data/lunisolar/parsed/drikpanchang.csv
"""

import argparse
import csv

from scraper.lunisolar.config import get_paths


def load_csv(path, tithi_col="tithi"):
    """Load a CSV keyed by (year, month, day) -> tithi (int)."""
    data = {}
    with open(path) as f:
        for row in csv.DictReader(f):
            key = (int(row["year"]), int(row["month"]), int(row["day"]))
            val = row[tithi_col]
            if val != "":
                data[key] = int(val)
    return data


def compare(parsed_path, ref_path, report_path):
    """Compare parsed tithis against reference and write report."""
    print(f"Parsed: {parsed_path}")
    print(f"Ref:    {ref_path}")

    parsed = load_csv(parsed_path)
    ref = load_csv(ref_path)

    matched = 0
    mismatched = 0
    missing_in_ref = 0
    missing_in_parsed = 0
    mismatches = []

    for key in sorted(parsed):
        if key not in ref:
            missing_in_ref += 1
            continue
        if parsed[key] == ref[key]:
            matched += 1
        else:
            mismatched += 1
            y, m, d = key
            mismatches.append((y, m, d, parsed[key], ref[key]))

    # Days in ref but not parsed
    for key in ref:
        if key not in parsed:
            missing_in_parsed += 1

    total_compared = matched + mismatched
    pct = 100.0 * matched / total_compared if total_compared else 0

    # Print summary
    lines = []
    lines.append(f"Tithi comparison: drikpanchang vs reference")
    lines.append(f"=" * 50)
    lines.append(f"Parsed days:       {len(parsed):>8,d}")
    lines.append(f"Reference days:    {len(ref):>8,d}")
    lines.append(f"Compared:          {total_compared:>8,d}")
    lines.append(f"Match:             {matched:>8,d}  ({pct:.3f}%)")
    lines.append(f"Mismatch:          {mismatched:>8,d}")
    lines.append(f"Missing in ref:    {missing_in_ref:>8,d}")
    lines.append(f"Missing in parsed: {missing_in_parsed:>8,d}")
    lines.append("")

    if mismatches:
        lines.append(f"Mismatches (first {min(100, len(mismatches))}):")
        lines.append(f"{'Date':>12s}  {'Drik':>6s}  {'Ref':>6s}  {'Diff':>6s}")
        lines.append("-" * 40)
        for y, m, d, dp_tithi, ref_tithi in mismatches[:100]:
            diff = dp_tithi - ref_tithi
            lines.append(f"{y:04d}-{m:02d}-{d:02d}  {dp_tithi:>6d}  {ref_tithi:>6d}  {diff:>+6d}")
        if len(mismatches) > 100:
            lines.append(f"... and {len(mismatches) - 100} more")

    report = "\n".join(lines)
    print(report)

    if report_path:
        with open(report_path, "w") as f:
            f.write(f"Tithi comparison: drikpanchang vs reference\n")
            f.write(f"{'=' * 50}\n")
            f.write(f"Parsed days:       {len(parsed):>8,d}\n")
            f.write(f"Reference days:    {len(ref):>8,d}\n")
            f.write(f"Compared:          {total_compared:>8,d}\n")
            f.write(f"Match:             {matched:>8,d}  ({pct:.3f}%)\n")
            f.write(f"Mismatch:          {mismatched:>8,d}\n")
            f.write(f"Missing in ref:    {missing_in_ref:>8,d}\n")
            f.write(f"Missing in parsed: {missing_in_parsed:>8,d}\n\n")
            if mismatches:
                f.write(f"All mismatches:\n")
                f.write(f"{'Date':>12s}  {'Drik':>6s}  {'Ref':>6s}  {'Diff':>6s}\n")
                f.write("-" * 40 + "\n")
                for y, m, d, dp_tithi, ref_tithi in mismatches:
                    diff = dp_tithi - ref_tithi
                    f.write(f"{y:04d}-{m:02d}-{d:02d}  {dp_tithi:>6d}  {ref_tithi:>6d}  {diff:>+6d}\n")
        print(f"\nFull report: {report_path}")


def main():
    parser = argparse.ArgumentParser(description="Compare drikpanchang tithis vs reference")
    parser.add_argument("--location", default="delhi", choices=["delhi", "nyc"],
                        help="Location (default: delhi)")
    parser.add_argument("--parsed", default=None,
                        help="Override parsed CSV path")
    parser.add_argument("--ref", default=None,
                        help="Override reference CSV path")
    parser.add_argument("--report", default=None,
                        help="Override report output path")
    args = parser.parse_args()

    paths = get_paths(args.location)
    parsed = args.parsed or paths["parsed_csv"]
    ref = args.ref or paths["ref_csv"]
    report = args.report or paths["comparison_report"]

    compare(parsed, ref, report)


if __name__ == "__main__":
    main()
