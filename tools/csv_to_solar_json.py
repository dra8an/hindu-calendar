#!/usr/bin/env python3
"""Convert solar calendar CSVs into per-Gregorian-month JSON files for the validation web page."""

import argparse
import csv
import json
import os
from datetime import date, timedelta

SCRIPT_DIR = os.path.dirname(__file__)
PROJECT_ROOT = os.path.join(SCRIPT_DIR, '..')

CALENDARS = {
    'tamil': {
        'csv': 'tamil_months_1900_2050.csv',
        'era': 'Saka',
    },
    'bengali': {
        'csv': 'bengali_months_1900_2050.csv',
        'era': 'Bangabda',
    },
    'odia': {
        'csv': 'odia_months_1900_2050.csv',
        'era': 'Saka',
    },
    'malayalam': {
        'csv': 'malayalam_months_1900_2050.csv',
        'era': 'Kollam',
    },
}

MIN_YEAR = 1900
MAX_YEAR = 2050


def load_solar_months(csv_path):
    """Load solar month boundaries from CSV.

    Returns a list of dicts sorted by start date:
      { month, year, length, start_date, month_name }
    """
    months = []
    with open(csv_path, newline='') as f:
        reader = csv.DictReader(f)
        for r in reader:
            start = date(int(r['greg_year']), int(r['greg_month']), int(r['greg_day']))
            months.append({
                'month': int(r['month']),
                'year': int(r['year']),
                'length': int(r['length']),
                'start_date': start,
                'month_name': r['month_name'],
            })
    # Should already be sorted by start_date, but ensure it
    months.sort(key=lambda m: m['start_date'])
    return months


def build_day_lookup(months):
    """Build a dict mapping each Gregorian date to its solar calendar info.

    Returns { date: { solar_month, solar_month_name, solar_day, solar_year } }
    """
    lookup = {}
    for m in months:
        for day_offset in range(m['length']):
            d = m['start_date'] + timedelta(days=day_offset)
            lookup[d] = {
                'solar_month': m['month'],
                'solar_month_name': m['month_name'],
                'solar_day': day_offset + 1,
                'solar_year': m['year'],
            }
    return lookup


def generate_calendar(cal_name, cal_info, solar_dir, out_base):
    """Generate all per-Gregorian-month JSON files for one solar calendar."""
    csv_path = os.path.join(solar_dir, cal_info['csv'])
    out_dir = os.path.join(out_base, cal_name)
    os.makedirs(out_dir, exist_ok=True)

    months = load_solar_months(csv_path)
    lookup = build_day_lookup(months)

    files_written = 0
    missing_days = 0

    for year in range(MIN_YEAR, MAX_YEAR + 1):
        for month in range(1, 13):
            # Find number of days in this Gregorian month
            if month == 12:
                next_month_start = date(year + 1, 1, 1)
            else:
                next_month_start = date(year, month + 1, 1)
            month_start = date(year, month, 1)
            num_days = (next_month_start - month_start).days

            days = []
            for day in range(1, num_days + 1):
                d = date(year, month, day)
                dow = d.weekday()  # 0=Mon..6=Sun
                dow = (dow + 1) % 7  # convert to 0=Sun..6=Sat

                solar = lookup.get(d)
                if solar is None:
                    missing_days += 1
                    continue

                day_obj = {
                    'day': day,
                    'dow': dow,
                    'solar_month': solar['solar_month'],
                    'solar_month_name': solar['solar_month_name'],
                    'solar_day': solar['solar_day'],
                    'solar_year': solar['solar_year'],
                    'era_name': cal_info['era'],
                    'is_month_start': solar['solar_day'] == 1,
                }
                days.append(day_obj)

            path = os.path.join(out_dir, f'{year:04d}-{month:02d}.json')
            with open(path, 'w') as f:
                json.dump(days, f, separators=(',', ':'))
            files_written += 1

    return files_written, missing_days


def main():
    parser = argparse.ArgumentParser(description='Convert solar CSVs to per-month JSON')
    parser.add_argument('--backend', choices=['se', 'moshier'], default='se',
                        help='Backend whose CSVs to read (default: se)')
    args = parser.parse_args()

    solar_dir = os.path.join(PROJECT_ROOT, 'validation', args.backend, 'solar')
    out_base = os.path.join(PROJECT_ROOT, 'validation', 'web', 'data', args.backend)

    total_files = 0
    for cal_name, cal_info in CALENDARS.items():
        files, missing = generate_calendar(cal_name, cal_info, solar_dir, out_base)
        total_files += files
        status = f'  {cal_name}: {files} files'
        if missing > 0:
            status += f' ({missing} days without solar data)'
        print(status)

    print(f'[{args.backend}] Total: {total_files} solar JSON files written to {os.path.abspath(out_base)}')


if __name__ == '__main__':
    main()
