#!/usr/bin/env python3
"""Extract adhika (repeated) and kshaya (skipped) tithi days from ref_1900_2050.csv.

Usage: python3 tools/extract_adhika_kshaya.py INPUT_CSV OUTPUT_CSV
"""

import csv
import sys


def main():
    if len(sys.argv) != 3:
        print(f'Usage: {sys.argv[0]} INPUT_CSV OUTPUT_CSV', file=sys.stderr)
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    # Read all rows
    rows = []
    with open(input_path, newline='') as f:
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

    # Detect adhika/kshaya and write output
    count = 0
    with open(output_path, 'w') as f:
        f.write('year,month,day,tithi,masa,adhika,saka,type\n')

        for i, row in enumerate(rows):
            if i == 0:
                continue

            prev = rows[i - 1]
            is_adhika = (row['tithi'] == prev['tithi'])
            expected = prev['tithi'] % 30 + 1
            is_kshaya = (row['tithi'] != expected and not is_adhika)

            if is_adhika or is_kshaya:
                typ = 'adhika' if is_adhika else 'kshaya'
                f.write(f"{row['year']},{row['month']},{row['day']},"
                        f"{row['tithi']},{row['masa']},{row['adhika']},{row['saka']},"
                        f"{typ}\n")
                count += 1

    print(f'Wrote {count} adhika/kshaya days to {output_path}')


if __name__ == '__main__':
    main()
