#!/usr/bin/env python3
"""Convert ref_1900_2050.csv into per-month JSON files for the validation web page."""

import argparse
import csv
import json
import os
from datetime import date

SCRIPT_DIR = os.path.dirname(__file__)
PROJECT_ROOT = os.path.join(SCRIPT_DIR, '..')

TITHI_NAMES = [
    '',            # 0 - unused
    'Pratipada',   # 1
    'Dwitiya',     # 2
    'Tritiya',     # 3
    'Chaturthi',   # 4
    'Panchami',    # 5
    'Shashthi',    # 6
    'Saptami',     # 7
    'Ashtami',     # 8
    'Navami',      # 9
    'Dashami',     # 10
    'Ekadashi',    # 11
    'Dwadashi',    # 12
    'Trayodashi',  # 13
    'Chaturdashi', # 14
    'Purnima',     # 15
]

MASA_NAMES = [
    '',             # 0 - unused
    'Chaitra',      # 1
    'Vaishakha',    # 2
    'Jyeshtha',     # 3
    'Ashadha',      # 4
    'Shravana',     # 5
    'Bhadrapada',   # 6
    'Ashvina',      # 7
    'Kartika',      # 8
    'Margashirsha', # 9
    'Pausha',       # 10
    'Magha',        # 11
    'Phalguna',     # 12
]


def tithi_name(t):
    """Return human-readable tithi name for tithi number 1-30."""
    if t == 30:
        return 'Amavasya'
    if 1 <= t <= 15:
        return TITHI_NAMES[t]
    # Krishna paksha: 16-29 → map to names 1-14
    return TITHI_NAMES[t - 15]


def paksha_str(t):
    """Return paksha string for tithi number 1-30."""
    if 1 <= t <= 15:
        return 'Shukla'
    return 'Krishna'


def load_reingold(reingold_path):
    """Load Reingold CSV keyed by (year, month, day). Returns empty dict if file missing."""
    if not os.path.exists(reingold_path):
        print(f'Warning: Reingold CSV not found at {reingold_path}, skipping diffs')
        return {}
    data = {}
    with open(reingold_path, newline='') as f:
        reader = csv.DictReader(f)
        for r in reader:
            key = (int(r['year']), int(r['month']), int(r['day']))
            data[key] = {
                'hl_tithi': int(r['hl_tithi']),
                'hl_masa': int(r['hl_masa']),
                'hl_adhika': int(r['hl_adhika']),
            }
    print(f'Loaded {len(data)} Reingold entries')
    return data


def main():
    parser = argparse.ArgumentParser(description='Convert ref CSV to per-month JSON')
    parser.add_argument('--backend', choices=['se', 'moshier'], default='se',
                        help='Backend whose CSV to read (default: se)')
    args = parser.parse_args()

    csv_path = os.path.join(PROJECT_ROOT, 'validation', args.backend, 'ref_1900_2050.csv')
    reingold_path = os.path.join(PROJECT_ROOT, 'validation', 'reingold', 'reingold_1900_2050.csv')
    out_dir = os.path.join(PROJECT_ROOT, 'validation', 'web', 'data', args.backend)

    os.makedirs(out_dir, exist_ok=True)

    reingold = load_reingold(reingold_path)

    # Read all rows
    rows = []
    with open(csv_path, newline='') as f:
        reader = csv.DictReader(f)
        for r in reader:
            rows.append({
                'year': int(r['year']),
                'month': int(r['month']),
                'day': int(r['day']),
                'tithi': int(r['tithi']),
                'masa': int(r['masa']),
                'adhika': int(r['adhika']),
                'saka': int(r['saka']),
            })

    # Compute adhika_tithi and kshaya_tithi by comparing consecutive days
    for i, row in enumerate(rows):
        if i == 0:
            row['adhika_tithi'] = False
            row['kshaya_tithi'] = False
        else:
            prev = rows[i - 1]
            # Adhika tithi: same tithi as yesterday
            row['adhika_tithi'] = (row['tithi'] == prev['tithi'])
            # Kshaya tithi: a tithi was skipped
            # Expected next tithi: prev + 1, wrapping 30 → 1
            expected = prev['tithi'] % 30 + 1
            row['kshaya_tithi'] = (row['tithi'] != expected and not row['adhika_tithi'])

    # Group by year-month and write JSON files
    current_key = None
    current_days = []
    files_written = 0

    def write_month(key, days):
        nonlocal files_written
        path = os.path.join(out_dir, f'{key}.json')
        with open(path, 'w') as f:
            json.dump(days, f, separators=(',', ':'))
        files_written += 1

    for row in rows:
        key = f"{row['year']:04d}-{row['month']:02d}"
        d = date(row['year'], row['month'], row['day'])
        dow = d.weekday()  # 0=Mon..6=Sun
        dow = (dow + 1) % 7  # convert to 0=Sun..6=Sat

        day_obj = {
            'day': row['day'],
            'dow': dow,
            'tithi': row['tithi'],
            'tithi_name': tithi_name(row['tithi']),
            'paksha': paksha_str(row['tithi']),
            'masa': row['masa'],
            'masa_name': MASA_NAMES[row['masa']],
            'adhika': row['adhika'],
            'saka': row['saka'],
            'adhika_tithi': row['adhika_tithi'],
            'kshaya_tithi': row['kshaya_tithi'],
        }

        # Add Reingold diffs (only fields that differ, to keep JSON small)
        rkey = (row['year'], row['month'], row['day'])
        if rkey in reingold:
            rl = reingold[rkey]
            differs = False
            if rl['hl_tithi'] != row['tithi']:
                day_obj['hl_tithi'] = rl['hl_tithi']
                differs = True
            if rl['hl_masa'] != row['masa']:
                day_obj['hl_masa'] = rl['hl_masa']
                differs = True
            if rl['hl_adhika'] != row['adhika']:
                day_obj['hl_adhika'] = rl['hl_adhika']
                differs = True
            if differs:
                day_obj['hl_diff'] = True

        if key != current_key:
            if current_key is not None:
                write_month(current_key, current_days)
            current_key = key
            current_days = []
        current_days.append(day_obj)

    # Write last month
    if current_key is not None:
        write_month(current_key, current_days)

    print(f'[{args.backend}] Wrote {files_written} JSON files to {os.path.abspath(out_dir)}')


if __name__ == '__main__':
    main()
