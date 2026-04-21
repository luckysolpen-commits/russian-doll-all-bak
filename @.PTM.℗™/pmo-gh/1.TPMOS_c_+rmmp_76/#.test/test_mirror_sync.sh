#!/bin/bash
# test_mirror_sync.sh - Test that move_entity properly syncs the mirror (state.txt)
# and pulses frame_changed.txt

cd "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+16/TPMOS-TRAINING/#.reference/TPMOS_c_+rmmp_66.02"

echo "=== STEP 1: Run CHTPM (sets location_kvp dynamically) ==="
bash run_chtpm.sh &
ORCH_PID=$!
sleep 3

echo ""
echo "=== STEP 2: Verify location_kvp is correct ==="
head -1 pieces/locations/location_kvp

echo ""
echo "=== STEP 3: BEFORE - xlector state ==="
cat projects/op-ed/pieces/xlector/state.txt

echo ""
echo "=== STEP 4: Inject 'w' key (ASCII 119 = move up) ==="
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
echo "[$TIMESTAMP] KEY_PRESSED: 119" >> pieces/keyboard/history.txt
sleep 2

echo ""
echo "=== STEP 5: AFTER - xlector state ==="
cat projects/op-ed/pieces/xlector/state.txt

echo ""
echo "=== STEP 6: Frame changed? ==="
tail -3 pieces/display/frame_changed.txt 2>/dev/null

echo ""
echo "=== STEP 7: Kill orchestrator ==="
bash pieces/buttons/linux/kill.sh 2>/dev/null
kill $ORCH_PID 2>/dev/null

echo ""
echo "=== TEST COMPLETE ==="
