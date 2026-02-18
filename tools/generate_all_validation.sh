#!/bin/bash
# Regenerate all validation data for both backends (Moshier and Swiss Ephemeris).
# Run from the project root directory.
set -euo pipefail

cd "$(dirname "$0")/.."

echo "=== Generating Moshier validation data ==="
make clean && make
mkdir -p validation/moshier
./build/gen_ref -o validation/moshier
./build/gen_solar_ref -o validation/moshier
python3 tools/extract_adhika_kshaya.py validation/moshier/ref_1900_2050.csv validation/moshier/adhika_kshaya_tithis.csv
python3 tools/csv_to_json.py --backend moshier
python3 tools/csv_to_solar_json.py --backend moshier

echo ""
echo "=== Generating Swiss Ephemeris validation data ==="
make clean && make USE_SWISSEPH=1
mkdir -p validation/se
./build/gen_ref -o validation/se
./build/gen_solar_ref -o validation/se
python3 tools/extract_adhika_kshaya.py validation/se/ref_1900_2050.csv validation/se/adhika_kshaya_tithis.csv
python3 tools/csv_to_json.py --backend se
python3 tools/csv_to_solar_json.py --backend se

echo ""
echo "=== Rebuilding default (Moshier) ==="
make clean && make

echo ""
echo "=== Done ==="
echo "Moshier data: validation/moshier/ + validation/web/data/moshier/"
echo "SE data:      validation/se/      + validation/web/data/se/"
