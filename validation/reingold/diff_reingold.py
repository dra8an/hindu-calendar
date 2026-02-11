#!/usr/bin/env python3
"""
diff_reingold.py — Compare our Hindu calendar data vs Reingold/Dershowitz

Reads:
  - ref_1900_2050.csv (our data: year,month,day,tithi,masa,adhika,saka)
  - reingold_1900_2050.csv (Reingold: year,month,day,hl_tithi,...,al_tithi,...)

Writes:
  - diffs_hindu_lunar.csv (dates where hindu-lunar-from-fixed differs)
  - diffs_astro_hindu.csv (dates where astro-hindu-lunar-from-fixed differs)

Prints summary to stdout.
"""

import csv
import sys
import os
from collections import defaultdict

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))

    our_csv = os.path.join(script_dir, "..", "drikpanchang_data", "ref_1900_2050.csv")
    reingold_csv = os.path.join(script_dir, "reingold_1900_2050.csv")

    if not os.path.exists(our_csv):
        print(f"ERROR: Our CSV not found: {our_csv}", file=sys.stderr)
        sys.exit(1)
    if not os.path.exists(reingold_csv):
        print(f"ERROR: Reingold CSV not found: {reingold_csv}", file=sys.stderr)
        sys.exit(1)

    # Load our data keyed by (year, month, day)
    our_data = {}
    with open(our_csv, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            key = (int(row["year"]), int(row["month"]), int(row["day"]))
            our_data[key] = {
                "tithi": int(row["tithi"]),
                "masa": int(row["masa"]),
                "adhika": int(row["adhika"]),
                "saka": int(row["saka"]),
            }

    # Load Reingold data
    reingold_data = {}
    with open(reingold_csv, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            key = (int(row["year"]), int(row["month"]), int(row["day"]))
            reingold_data[key] = {
                "hl_tithi": int(row["hl_tithi"]),
                "hl_masa": int(row["hl_masa"]),
                "hl_adhika": int(row["hl_adhika"]),
                "hl_vikrama": int(row["hl_vikrama"]),
                "al_tithi": int(row["al_tithi"]),
                "al_masa": int(row["al_masa"]),
                "al_adhika": int(row["al_adhika"]),
                "al_vikrama": int(row["al_vikrama"]),
            }

    print(f"Our data: {len(our_data)} dates")
    print(f"Reingold data: {len(reingold_data)} dates")

    # Check all our dates exist in Reingold data
    missing = set(our_data.keys()) - set(reingold_data.keys())
    if missing:
        print(f"WARNING: {len(missing)} dates in our data but not in Reingold data")

    extra = set(reingold_data.keys()) - set(our_data.keys())
    if extra:
        print(f"WARNING: {len(extra)} dates in Reingold data but not in our data")

    common_keys = sorted(set(our_data.keys()) & set(reingold_data.keys()))
    print(f"Common dates: {len(common_keys)}")
    print()

    # Compare hindu-lunar-from-fixed
    hl_diffs = []
    hl_stats = defaultdict(int)

    # Compare astro-hindu-lunar-from-fixed
    al_diffs = []
    al_stats = defaultdict(int)

    for key in common_keys:
        ours = our_data[key]
        theirs = reingold_data[key]
        y, m, d = key
        date_str = f"{y:04d}-{m:02d}-{d:02d}"

        # Our Saka year -> Vikrama = Saka + 135
        our_vikrama = ours["saka"] + 135

        # Compare hindu-lunar-from-fixed
        diffs_for_date = []
        if ours["tithi"] != theirs["hl_tithi"]:
            diffs_for_date.append(("tithi", ours["tithi"], theirs["hl_tithi"]))
            hl_stats["tithi"] += 1
        if ours["masa"] != theirs["hl_masa"]:
            diffs_for_date.append(("masa", ours["masa"], theirs["hl_masa"]))
            hl_stats["masa"] += 1
        if ours["adhika"] != theirs["hl_adhika"]:
            diffs_for_date.append(("adhika", ours["adhika"], theirs["hl_adhika"]))
            hl_stats["adhika"] += 1
        if our_vikrama != theirs["hl_vikrama"]:
            diffs_for_date.append(("year", our_vikrama, theirs["hl_vikrama"]))
            hl_stats["year"] += 1
        if diffs_for_date:
            hl_stats["dates_with_diffs"] += 1
            for field, our_val, their_val in diffs_for_date:
                hl_diffs.append((date_str, field, our_val, their_val))

        # Compare astro-hindu-lunar-from-fixed
        diffs_for_date = []
        if ours["tithi"] != theirs["al_tithi"]:
            diffs_for_date.append(("tithi", ours["tithi"], theirs["al_tithi"]))
            al_stats["tithi"] += 1
        if ours["masa"] != theirs["al_masa"]:
            diffs_for_date.append(("masa", ours["masa"], theirs["al_masa"]))
            al_stats["masa"] += 1
        if ours["adhika"] != theirs["al_adhika"]:
            diffs_for_date.append(("adhika", ours["adhika"], theirs["al_adhika"]))
            al_stats["adhika"] += 1
        if our_vikrama != theirs["al_vikrama"]:
            diffs_for_date.append(("year", our_vikrama, theirs["al_vikrama"]))
            al_stats["year"] += 1
        if diffs_for_date:
            al_stats["dates_with_diffs"] += 1
            for field, our_val, their_val in diffs_for_date:
                al_diffs.append((date_str, field, our_val, their_val))

    # Write diff files
    hl_diff_path = os.path.join(script_dir, "diffs_hindu_lunar.csv")
    with open(hl_diff_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["date", "field", "ours", "theirs"])
        for row in hl_diffs:
            writer.writerow(row)

    al_diff_path = os.path.join(script_dir, "diffs_astro_hindu.csv")
    with open(al_diff_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["date", "field", "ours", "theirs"])
        for row in al_diffs:
            writer.writerow(row)

    # Print summary
    print("=" * 60)
    print("HINDU-LUNAR-FROM-FIXED (Surya Siddhanta with epicycles)")
    print("=" * 60)
    print(f"  Total dates compared: {len(common_keys)}")
    print(f"  Dates with differences: {hl_stats['dates_with_diffs']}")
    print(f"  Tithi differences: {hl_stats['tithi']}")
    print(f"  Masa differences: {hl_stats['masa']}")
    print(f"  Adhika differences: {hl_stats['adhika']}")
    print(f"  Year differences: {hl_stats['year']}")
    print(f"  Diff file: {hl_diff_path}")
    print(f"  Diff rows: {len(hl_diffs)}")
    print()

    print("=" * 60)
    print("ASTRO-HINDU-LUNAR-FROM-FIXED (Astronomical ephemeris)")
    print("=" * 60)
    print(f"  Total dates compared: {len(common_keys)}")
    print(f"  Dates with differences: {al_stats['dates_with_diffs']}")
    print(f"  Tithi differences: {al_stats['tithi']}")
    print(f"  Masa differences: {al_stats['masa']}")
    print(f"  Adhika differences: {al_stats['adhika']}")
    print(f"  Year differences: {al_stats['year']}")
    print(f"  Diff file: {al_diff_path}")
    print(f"  Diff rows: {len(al_diffs)}")
    print()

    # Percentage match
    hl_match_pct = 100.0 * (len(common_keys) - hl_stats["dates_with_diffs"]) / len(common_keys) if common_keys else 0
    al_match_pct = 100.0 * (len(common_keys) - al_stats["dates_with_diffs"]) / len(common_keys) if common_keys else 0
    print(f"Hindu-lunar match rate: {hl_match_pct:.2f}%")
    print(f"Astro-hindu match rate: {al_match_pct:.2f}%")


if __name__ == "__main__":
    main()
