#!/bin/bash
# kill.sh - Kill button for Linux/macOS
# Usage: ./#.buttons/linux/kill.sh

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LEGACY_DIR="$SCRIPT_DIR/legacy"

echo "=== Killing CHTPM Processes (Linux) ==="
bash "$LEGACY_DIR/kill_all.sh"
