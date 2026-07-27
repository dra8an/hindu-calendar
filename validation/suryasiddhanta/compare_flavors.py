#!/usr/bin/env python3
"""Characterise how Bisuddhasiddhanta and Suryasiddhanta Bengali differ.

Compares the two parsed month-start CSVs over their overlapping range and
reports: how many months differ, by how many days, and whether the difference
correlates with rashi (month number), season, or epoch.

Usage:
    python3 compare_flavors.py
"""

import collections
import csv
import datetime
import os

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))

# Committed copy of the drikpanchang scrape (see validation/drikpanchang/).
BASE = os.path.join(REPO, "validation", "drikpanchang")
BISUDDHA = os.path.join(BASE, "bengali.csv")
SURYA = os.path.join(BASE, "bengali_suryasiddhanta.csv")

BENGALI_MONTHS = {
    1: "Boishakh", 2: "Joishtho", 3: "Asharh", 4: "Srabon",
    5: "Bhadro", 6: "Ashshin", 7: "Kartik", 8: "Ogrohaeon",
    9: "Poush", 10: "Magh", 11: "Falgun", 12: "Choitro",
}


def load(path):
    """Key by (solar_year, month_num) -> start date."""
    out = {}
    with open(path) as f:
        for r in csv.DictReader(f):
            if not r["year"]:
                continue
            key = (int(r["year"]), int(r["month"]))
            out[key] = datetime.date(int(r["greg_year"]),
                                     int(r["greg_month"]),
                                     int(r["greg_day"]))
    return out


def main():
    bis = load(BISUDDHA)
    sur = load(SURYA)
    common = sorted(set(bis) & set(sur))
    if not common:
        print("No overlapping months.")
        return

    print(f"Bisuddhasiddhanta months : {len(bis):,}")
    print(f"Suryasiddhanta months    : {len(sur):,}")
    print(f"Overlapping              : {len(common):,}")
    lo = min(bis[k] for k in common)
    hi = max(bis[k] for k in common)
    print(f"Range                    : {lo} .. {hi}")
    print()

    diffs = collections.Counter()
    by_month = collections.defaultdict(collections.Counter)
    by_decade = collections.defaultdict(collections.Counter)
    examples = collections.defaultdict(list)

    for key in common:
        d = (sur[key] - bis[key]).days
        diffs[d] += 1
        by_month[key[1]][d] += 1
        by_decade[bis[key].year // 10 * 10][d] += 1
        if d != 0:
            examples[d].append((key, bis[key], sur[key]))

    same = diffs[0]
    total = len(common)
    print(f"Identical month start : {same:,}/{total:,} ({100*same/total:.2f}%)")
    print(f"Differing             : {total-same:,} ({100*(total-same)/total:.2f}%)")
    print()
    print("Offset distribution (Surya minus Bisuddha, in days):")
    for d in sorted(diffs):
        bar = "#" * max(1, round(60 * diffs[d] / total))
        print(f"  {d:+3d} day : {diffs[d]:>5,}  {bar}")
    print()

    print("By Bengali month (share differing):")
    print(f"  {'month':<12} {'n':>5} {'differ':>7} {'%':>7}   offsets")
    for m in range(1, 13):
        c = by_month[m]
        n = sum(c.values())
        if not n:
            continue
        diff = n - c[0]
        offs = ", ".join(f"{k:+d}:{v}" for k, v in sorted(c.items()) if k != 0)
        print(f"  {BENGALI_MONTHS[m]:<12} {n:>5} {diff:>7} {100*diff/n:>6.1f}%   {offs or '-'}")
    print()

    print("By decade (share differing):")
    for dec in sorted(by_decade):
        c = by_decade[dec]
        n = sum(c.values())
        diff = n - c[0]
        print(f"  {dec}s  n={n:>4}  differ={diff:>4} ({100*diff/n:>5.1f}%)")
    print()

    for d in sorted(examples):
        ex = examples[d]
        print(f"Examples of {d:+d} day ({len(ex)} total), first 5:")
        for key, b, s in ex[:5]:
            print(f"    {BENGALI_MONTHS[key[1]]:<10} {key[0]}  bisuddha {b}  surya {s}")
        print()


if __name__ == "__main__":
    main()
