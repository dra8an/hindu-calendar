#!/bin/bash
# run.sh — Generate Reingold/Dershowitz Hindu calendar data and diff against ours
#
# Usage: bash validation/reingold/run.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="/tmp/calendar-code2"
CALENDAR_L="$REPO_DIR/calendar.l"
OUTPUT_CSV="$SCRIPT_DIR/reingold_1900_2050.csv"
GENERATOR="$SCRIPT_DIR/generate_reingold.lisp"
DIFFER="$SCRIPT_DIR/diff_reingold.py"

echo "=== Reingold/Dershowitz vs Our Calendar ==="
echo ""

# Step 1: Ensure calendar-code2 repo is cloned
if [ ! -f "$CALENDAR_L" ]; then
    echo "Cloning calendar-code2 repo..."
    git clone --depth 1 https://github.com/EdReingold/calendar-code2.git "$REPO_DIR"
    echo ""
else
    echo "calendar-code2 repo found at $REPO_DIR"
fi

# Step 2: Generate Reingold CSV via SBCL
if [ -f "$OUTPUT_CSV" ]; then
    ROWS=$(wc -l < "$OUTPUT_CSV" | tr -d ' ')
    if [ "$ROWS" -ge 55152 ]; then
        echo "Reingold CSV already exists with $ROWS lines, skipping generation."
        echo "(Delete $OUTPUT_CSV to regenerate)"
    else
        echo "Reingold CSV exists but only has $ROWS lines, regenerating..."
        rm -f "$OUTPUT_CSV"
    fi
fi

if [ ! -f "$OUTPUT_CSV" ]; then
    echo ""
    echo "Generating Reingold CSV (55,152 dates, ~2-3 minutes)..."
    echo "Start time: $(date)"
    CALENDAR_L_PATH="$CALENDAR_L" sbcl --noinform --non-interactive --load "$GENERATOR" > "$OUTPUT_CSV"
    echo "End time: $(date)"
    ROWS=$(wc -l < "$OUTPUT_CSV" | tr -d ' ')
    echo "Generated $ROWS lines (including header)"
    echo ""
fi

# Step 3: Spot-check Jan 1, 2025
echo ""
echo "--- Spot-check: Jan 1, 2025 ---"
grep "^2025,1,1," "$OUTPUT_CSV" || echo "WARNING: Jan 1, 2025 not found in CSV"

# Step 4: Run diff
echo ""
echo "Running diff comparison..."
echo ""
python3 "$DIFFER"

echo ""
echo "=== Done ==="
