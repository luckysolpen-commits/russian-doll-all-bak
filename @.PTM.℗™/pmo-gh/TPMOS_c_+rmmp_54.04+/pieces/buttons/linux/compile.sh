#!/bin/bash
# compile.sh - Compile button for Linux/macOS
# Usage: ./#.buttons/linux/compile.sh

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LEGACY_DIR="$SCRIPT_DIR/legacy"

echo "=== Compiling CHTPM (Linux) ==="
echo "Killing existing processes first..."
bash "$SCRIPT_DIR/kill.sh"

echo "Compiling..."
bash "$LEGACY_DIR/compile_all.sh"
