#!/bin/sh
# clear_all_history.sh - Surgically truncate all history and audit files

echo "=== CLEARING ALL HISTORY & AUDIT LOGS ==="

# 1. Clear Master Ledger (The biggest growth point)
if [ -f "pieces/master_ledger/master_ledger.txt" ]; then
    echo " > Truncating pieces/master_ledger/master_ledger.txt"
    > pieces/master_ledger/master_ledger.txt
fi

# 2. Find and clear all history.txt files
echo " > Finding all history.txt files..."
find . -name "history.txt" -exec sh -c '> "{}"' \;

# 3. Find and clear all piece-specific ledger.txt files
echo " > Finding all ledger.txt files..."
find . -name "ledger.txt" -exec sh -c '> "{}"' \;

# 4. Clear Debug logs
echo " > Clearing debug logs..."
[ -f "debug.txt" ] && > debug.txt
[ -f "debug.log" ] && > debug.log
find . -name "debug_log.txt" -exec sh -c '> "{}"' \;

# 5. Clear Marker files (optional, but keeps them clean)
echo " > Resetting marker files..."
[ -f "pieces/display/frame_changed.txt" ] && > pieces/display/frame_changed.txt
[ -f "pieces/master_ledger/frame_changed.txt" ] && > pieces/master_ledger/frame_changed.txt

echo "=== DONE: ALL VOLATILE DATA PURGED ==="
