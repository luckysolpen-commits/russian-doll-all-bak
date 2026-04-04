#!/bin/bash
# debug_gl.sh - GL-OS Debug button for Linux/macOS
# Usage: ./#.buttons/linux/debug_gl.sh

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LEGACY_DIR="$SCRIPT_DIR/legacy"

echo "=== Launching GL-OS Desktop (Linux) ==="
bash "$LEGACY_DIR/run_gl_desktop.sh"
