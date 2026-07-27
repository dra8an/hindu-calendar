#!/usr/bin/env python3
"""Fit the Bengali critical-time rule for the Suryasiddhanta panjika.

Model mirrors src/solar.c's sankranti_to_civil_day():

    D    = local civil date of the sankranti, after a per-rashi day-edge shift
    crit = D's midnight + crit_min
    if sankranti <= crit:
        month starts on D, unless the tithi rule pushes it to D+1
    else:
        month starts on D+1

The tithi rule (Sewell & Dikshit) pushes when the tithi current at sunrise of
D-1 has already ended by the time of the sankranti.  Rashi 4 never pushes and
rashi 10 always pushes, matching the Bisuddha implementation.

Everything astronomical here is Surya Siddhanta except sunrise, which uses the
project's drik sunrise (drikpanchang computes sunrise astronomically regardless
of which panjika arithmetic is selected).
"""

import csv
import datetime
import os
import subprocess
import sys

from surya_siddhanta import all_sankrantis, tithi_end_after

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))

LON, ZONE = 77.2090, 5.5
JD_RD = 1721424.5

# Committed copy of the drikpanchang scrape (see validation/drikpanchang/).
PARSED = os.path.join(REPO, "validation", "drikpanchang",
                      "bengali_suryasiddhanta.csv")
SUNRISE_TOOL = os.path.join(HERE, "sunrise_tool")


def to_ist(m):
    return m - LON / 360.0 + ZONE / 24.0


def rd_to_date(rd):
    return datetime.date(1, 1, 1) + datetime.timedelta(days=int(rd) - 1)


def load_drik():
    out = {}
    for r in csv.DictReader(open(PARSED)):
        d = datetime.date(int(r["greg_year"]), int(r["greg_month"]), int(r["greg_day"]))
        out[d] = (int(r["month"]), int(r["year"]))
    return out


def batch_sunrise(dates):
    """Return {date: jd_ut} using the project's own sunrise implementation."""
    if not os.path.exists(SUNRISE_TOOL):
        sys.exit(f"sunrise_tool not built. From the repo root run:\n"
                 f"  make && cc -O2 -std=c99 -Ilib/moshier -Isrc "
                 f"-o validation/suryasiddhanta/sunrise_tool "
                 f"validation/suryasiddhanta/sunrise_tool.c "
                 f"build/moshier/*.o build/astro.o build/date_utils.o -lm")
    inp = "\n".join(str(d) for d in sorted(dates)) + "\n"
    out = subprocess.run([SUNRISE_TOOL], input=inp,
                         capture_output=True, text=True).stdout
    sr = {}
    for line in out.strip().split("\n"):
        d, j = line.split(",")
        sr[datetime.date.fromisoformat(d)] = float(j)
    return sr


def build_cases(drik):
    first, last = min(drik), max(drik)
    cases = []
    for rashi, m in all_sankrantis(693596.0 - 40, 693596.0 + 152 * 366):
        ist = to_ist(m)
        d = rd_to_date(ist)
        if d < first or d > last:
            continue
        cases.append(dict(rashi=rashi, m=m, ist=ist, date=d))
    return cases


def predict(case, sr, crit_min, day_edge_min):
    """Predicted month-start date under the model."""
    ist = case["ist"]
    rashi = case["rashi"]

    # Day-edge shift: pull the civil-day boundary earlier so a late-evening
    # sankranti is treated as belonging to the following civil day.
    shifted = ist + day_edge_min / 1440.0
    D = rd_to_date(shifted)

    # crit is crit_min after D's midnight, expressed as an IST moment.
    d_midnight = (datetime.date(D.year, D.month, D.day) - datetime.date(1, 1, 1)).days + 1
    crit_ist = d_midnight + crit_min / 1440.0

    if ist <= crit_ist:
        if rashi == 4:
            push = False
        elif rashi == 10:
            push = True
        else:
            prev = D - datetime.timedelta(days=1)
            jd = sr.get(prev)
            if jd is None:
                push = False
            else:
                rd_local = (jd - JD_RD) + LON / 360.0
                push = tithi_end_after(rd_local) <= case["m"]
        return D + datetime.timedelta(days=1 if push else 0)
    return D + datetime.timedelta(days=1)


def evaluate(cases, drik, sr, crit_min, day_edge_min, verbose=False):
    ok = bad = 0
    misses = []
    for c in cases:
        pred = predict(c, sr, crit_min, day_edge_min)
        if pred in drik:
            ok += 1
        else:
            bad += 1
            misses.append((c, pred))
    if verbose:
        for c, pred in misses:
            s = round((c["ist"] % 1) * 86400)
            print(f"    rashi {c['rashi']:>2}  {c['date']}  "
                  f"{s//3600:02d}:{s%3600//60:02d}:{s%60:02d}  predicted {pred}")
    return ok, bad, misses


def main():
    drik = load_drik()
    cases = build_cases(drik)
    print(f"sankrantis in range: {len(cases)}   drik month starts: {len(drik)}")

    # Sunrise for every day that could be consulted.
    needed = set()
    for c in cases:
        for k in (-3, -2, -1, 0, 1):
            needed.add(c["date"] + datetime.timedelta(days=k))
    sr = batch_sunrise(needed)
    print(f"sunrises computed: {len(sr)}")
    print()

    best = None
    print("Scanning crit_min x day_edge_min:")
    print(f"{'crit':>6} {'edge':>6} {'match':>7} {'miss':>5}")
    for crit_min in range(20, 61, 4):
        for day_edge_min in (0, 5, 10, 15, 20):
            ok, bad, _ = evaluate(cases, drik, sr, crit_min, day_edge_min)
            if best is None or ok > best[0]:
                best = (ok, bad, crit_min, day_edge_min)
            if bad <= 40:
                print(f"{crit_min:>6} {day_edge_min:>6} {ok:>7} {bad:>5}")

    ok, bad, crit_min, day_edge_min = best
    print()
    print(f"BEST: crit={crit_min} min, day_edge={day_edge_min} min "
          f"-> {ok}/{len(cases)} = {100*ok/len(cases):.3f}%  ({bad} misses)")
    print()
    print("Remaining misses:")
    evaluate(cases, drik, sr, crit_min, day_edge_min, verbose=True)


if __name__ == "__main__":
    main()
