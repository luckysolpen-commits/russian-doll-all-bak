#!/bin/bash
# pal_watchdog.sh - Monitors and kills runaway Prisc VM instances and problematic PAL modules.
# Part of the CPU Health & Throttling strategy.

MAX_RUNTIME=5 # seconds for a VM instance
CHECK_INTERVAL=1 # second
LOG_FILE="pieces/system/prisc/watchdog.log"

mkdir -p pieces/system/prisc
echo "[$(date)] PAL Watchdog started. Max runtime: ${MAX_RUNTIME}s" >> "$LOG_FILE"

while true; do
    # 1. Monitor runaway Prisc VM instances (prisc+x)
    PIDS=$(pgrep -f "prisc\+x")
    
    for PID in $PIDS; do
        ELAPSED=$(ps -p $PID -o etimes= 2>/dev/null | tr -d ' ')
        
        if [ ! -z "$ELAPSED" ]; then
            if [ "$ELAPSED" -gt "$MAX_RUNTIME" ]; then
                echo "[$(date)] [watchdog] Killing runaway Prisc process (PID: $PID, Elapsed: ${ELAPSED}s)" >> "$LOG_FILE"
                kill -9 $PID 2>/dev/null
            fi
        fi
    done
    
    # 2. Prevent Fork Bombs: Check total number of prisc+x processes
    COUNT=$(echo "$PIDS" | wc -w)
    if [ "$COUNT" -gt 15 ]; then
        echo "[$(date)] [watchdog] WARNING: High Prisc process count detected ($COUNT). Cleaning up..." >> "$LOG_FILE"
        pkill -9 -f "prisc\+x"
    fi

    # 3. Monitor specific problematic modules if they consume too much CPU
    # (Future-proofing: we could add top-based checks here)

    sleep $CHECK_INTERVAL
done
