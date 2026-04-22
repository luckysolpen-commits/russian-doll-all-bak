#!/bin/bash
# run.sh - Launch script for Exo-Testing-Bot v4 (Orchestrator)

# Define bot directory and executable path
BOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
EXECUTABLE="$BOT_DIR/exo-testing-bot-v4"

# --- Argument Handling ---
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <target_tpmos_directory>"
    exit 1
fi
TARGET_TPMOS_DIR="$1"

# --- Pre-run checks ---
if [ ! -x "$EXECUTABLE" ]; then
    echo "Error: Executable '$EXECUTABLE' not found or not executable."
    echo "Please run './build_bot.sh' first."
    exit 1
fi

if [ ! -d "$TARGET_TPMOS_DIR" ]; then
    echo "Error: Target TPMOS directory '$TARGET_TPMOS_DIR' not found."
    exit 1
fi

echo "=== Starting Exo-Testing-Bot v4 (Orchestrator) ==="
echo "Target TPMOS Directory: $TARGET_TPMOS_DIR"
echo ""

# --- Execute the bot ---
# Pass the target TPMOS directory as an argument to the bot
"$EXECUTABLE" "$TARGET_TPMOS_DIR"

EXIT_CODE=$?

echo ""
echo "Bot execution finished with exit code: $EXIT_CODE"

# --- Post-run cleanup (simulated) ---
# In a real scenario, the bot's cleanup might be handled internally
# For this script, we simulate checking that logs are created
if [ -f "/tmp/bot4_proc_list.txt" ]; then
    echo "Simulated proc list found: /tmp/bot4_proc_list.txt"
fi
if [ -f "/tmp/bot4_orchestrator.log" ]; then
    echo "Simulated orchestrator log found: /tmp/bot4_orchestrator.log"
fi

exit $EXIT_CODE
