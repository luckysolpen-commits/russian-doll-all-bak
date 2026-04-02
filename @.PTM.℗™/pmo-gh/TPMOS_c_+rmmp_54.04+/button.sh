#!/bin/bash
# button.sh - Main launcher for CHTPM buttons (Linux/macOS)
# Usage: ./button.sh <action>
# Actions: compile, run, kill, debug, watchdog, help

ACTION="${1:-help}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUTTON_DIR="$SCRIPT_DIR/pieces/buttons/linux"

case "$ACTION" in
    compile|c|build)
        bash "$BUTTON_DIR/compile.sh"
        ;;
    run|r|start)
        bash "$BUTTON_DIR/run.sh"
        ;;
    kill|k|stop)
        bash "$BUTTON_DIR/kill.sh"
        ;;
    debug|d|gl)
        bash "$BUTTON_DIR/debug_gl.sh"
        ;;
    watchdog|w)
        bash "$BUTTON_DIR/watchdog.sh"
        ;;
    help|h|-h|--help)
        echo "CHTPM Button System (Linux/macOS)"
        echo ""
        echo "Usage: ./button.sh <action>"
        echo ""
        echo "Actions:"
        echo "  compile, c, build   - Compile all CHTPM binaries"
        echo "  run, r, start       - Run CHTPM orchestrator"
        echo "  kill, k, stop       - Kill all CHTPM processes"
        echo "  debug, d, gl        - Launch GL-OS desktop"
        echo "  watchdog, w         - Start PAL watchdog"
        echo "  help, h             - Show this help"
        ;;
    *)
        echo "Unknown action: $ACTION"
        echo "Run './button.sh help' for usage."
        exit 1
        ;;
esac
