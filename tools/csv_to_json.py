#!/usr/bin/env python3
"""Convert ref_1900_2050.csv into per-month JSON files for the validation web page."""

import csv
import json
import os
from datetime import date

CSV_PATH = os.path.join(os.path.dirname(__file__),
                        '..', 'validation', 'drikpanchang_data', 'ref_1900_2050.csv')
OUT_DIR = os.path.join(os.path.dirname(__file__), '..', 'validation', 'web', 'data')

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


def main():
    os.makedirs(OUT_DIR, exist_ok=True)

    # Read all rows
    rows = []
    with open(CSV_PATH, newline='') as f:
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
        path = os.path.join(OUT_DIR, f'{key}.json')
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

        if key != current_key:
            if current_key is not None:
                write_month(current_key, current_days)
            current_key = key
            current_days = []
        current_days.append(day_obj)

    # Write last month
    if current_key is not None:
        write_month(current_key, current_days)

    print(f'Wrote {files_written} JSON files to {os.path.abspath(OUT_DIR)}')


if __name__ == '__main__':
    main()
