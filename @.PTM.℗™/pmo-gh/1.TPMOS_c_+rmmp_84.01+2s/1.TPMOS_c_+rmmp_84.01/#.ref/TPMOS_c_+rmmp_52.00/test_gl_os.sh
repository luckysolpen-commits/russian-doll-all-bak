#!/bin/bash
# test_gl_os.sh - Test GL-OS Desktop

echo "=== GL-OS Test ==="
echo ""

# Ensure session files exist
mkdir -p pieces/apps/gl_os/session
touch pieces/apps/gl_os/session/state.txt
touch pieces/apps/gl_os/session/history.txt
touch pieces/apps/gl_os/session/view.txt

echo "Starting GL-OS Desktop..."
echo ""

# Launch GL-OS desktop
./pieces/apps/gl_os/plugins/+x/gl_desktop.+x &
GL_PID=$!

echo "GL-OS desktop launched (PID: $GL_PID)"
echo ""
echo "Controls:"
echo "  [Right-Click] - Show context menu"
echo "  [t] - New Terminal window"
echo "  [c] - Close active window"
echo "  [Drag titlebar] - Move window"
echo "  [X button] - Close window"
echo "  [ESC] - Hide context menu"
echo ""
echo "Press Ctrl+C to stop GL-OS"

# Wait for the process
wait $GL_PID
