#!/bin/bash
# test_selector.sh - Test man-ops selector movement

cd /home/no/Desktop/Piecemark-IT/\[CALENDARS\]02.01/zesy/mar_11+0/mar_11+2q/\^.PMO_c_+rmmp_32.23

echo "=== SELECTOR MOVEMENT TEST ==="
echo ""

# Reset selector position
cat > projects/man-ops/pieces/selector/state.txt << 'EOF'
name=Selector
type=selector
pos_x=5
pos_y=5
pos_z=0
on_map=1
icon=X
EOF

# Set up manager state
echo "project_id=man-ops" > pieces/apps/player_app/manager/state.txt
echo "active_target_id=selector" >> pieces/apps/player_app/manager/state.txt
echo "current_z=0" >> pieces/apps/player_app/manager/state.txt
echo "last_key=None" >> pieces/apps/player_app/manager/state.txt

# Clear history
> pieces/apps/player_app/history.txt

echo "1. Initial state:"
cat projects/man-ops/pieces/selector/state.txt | grep pos
echo ""

echo "2. Pressing UP (1002)..."
echo "1002" >> pieces/apps/player_app/history.txt
sleep 0.5
echo "   State:"
cat projects/man-ops/pieces/selector/state.txt | grep pos
echo ""

echo "3. Pressing DOWN (1003)..."
echo "1003" >> pieces/apps/player_app/history.txt
sleep 0.5
echo "   State:"
cat projects/man-ops/pieces/selector/state.txt | grep pos
echo ""

echo "4. Pressing LEFT (1000)..."
echo "1000" >> pieces/apps/player_app/history.txt
sleep 0.5
echo "   State:"
cat projects/man-ops/pieces/selector/state.txt | grep pos
echo ""

echo "5. Pressing RIGHT (1001)..."
echo "1001" >> pieces/apps/player_app/history.txt
sleep 0.5
echo "   State:"
cat projects/man-ops/pieces/selector/state.txt | grep pos
echo ""

echo "6. Rendering view..."
./pieces/apps/playrm/ops/+x/render_map.+x
echo "   View (X should be at 5,5):"
cat pieces/apps/player_app/view.txt | head -12
echo ""

echo "=== TEST COMPLETE ==="
