#!/bin/bash

echo "Emergency shutdown initiated..."

# Find and kill any running trainer processes
TRAINING_PIDS=$(ps aux | grep "trainer\|train" | grep -v grep | awk '{print $2}')

if [ -n "$TRAINING_PIDS" ]; then
    echo "Found and killing training processes: $TRAINING_PIDS"
    kill -TERM $TRAINING_PIDS 2>/dev/null
    sleep 2
    # Force kill if still running
    TRAINING_PIDS=$(ps aux | grep "trainer\|train" | grep -v grep | awk '{print $2}')
    if [ -n "$TRAINING_PIDS" ]; then
        echo "Force killing stubborn processes: $TRAINING_PIDS"
        kill -KILL $TRAINING_PIDS 2>/dev/null
    fi
else
    echo "No training processes found."
fi

# Kill any subprocesses that might be running
SUBPROCESS_PIDS=$(ps aux | grep "+x/" | grep -v grep | awk '{print $2}')
if [ -n "$SUBPROCESS_PIDS" ]; then
    echo "Killing subprocesses: $SUBPROCESS_PIDS"
    kill -TERM $SUBPROCESS_PIDS 2>/dev/null
    sleep 1
    kill -KILL $SUBPROCESS_PIDS 2>/dev/null
fi

echo "Emergency shutdown completed."