#!/bin/bash

# run_chtpm.sh - Run the CHTPM v0.01 system (MODULAR REFACTOR v3.2)
# TPM-Compliant: Uses modular entity pieces and registry.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PM_BIN="./pieces/master_ledger/plugins/+x/piece_manager.+x"

# 1. Set up location_kvp
mkdir -p pieces/locations
cat > pieces/locations/location_kvp << EOF
project_root=${SCRIPT_DIR}
pieces_dir=${SCRIPT_DIR}/pieces
fuzzpet_app_dir=${SCRIPT_DIR}/pieces/apps/fuzzpet_app
fuzzpet_dir=${SCRIPT_DIR}/pieces/apps/fuzzpet_app/fuzzpet
system_dir=${SCRIPT_DIR}/pieces/system
pet_01_dir=${SCRIPT_DIR}/pieces/entities/pet_01
pet_02_dir=${SCRIPT_DIR}/pieces/entities/pet_02
selector_dir=${SCRIPT_DIR}/pieces/apps/fuzzpet_app/selector
EOF

# 2. Initialize World Registry
mkdir -p pieces/apps/fuzzpet_app/world
cat > pieces/apps/fuzzpet_app/world/registry.txt << EOF
pet_01
pet_02
selector
EOF

# 3. Initialize Pieces (DNA and Mirror Sync)
# We use piece_manager to ensure DNA (.pdl) and Mirror (state.txt) are identical
$PM_BIN pet_01 set-state pos_x 5
$PM_BIN pet_01 set-state pos_y 2
$PM_BIN pet_01 set-state on_map 1
$PM_BIN pet_01 set-state name Fuzzball

$PM_BIN pet_02 set-state pos_x 10
$PM_BIN pet_02 set-state pos_y 5
$PM_BIN pet_02 set-state on_map 1
$PM_BIN pet_02 set-state name StorageCat

$PM_BIN selector set-state pos_x 5
$PM_BIN selector set-state pos_y 5
$PM_BIN selector set-state on_map 1
$PM_BIN selector set-state name Cursor

# 4. Initialize Manager State
mkdir -p pieces/apps/fuzzpet_app/manager
echo "active_target_id=selector" > pieces/apps/fuzzpet_app/manager/state.txt
echo "last_key=None" >> pieces/apps/fuzzpet_app/manager/state.txt

# 5. Clear Session Files
> pieces/keyboard/history.txt
> pieces/apps/fuzzpet_app/fuzzpet/history.txt
> pieces/master_ledger/master_ledger.txt
> pieces/master_ledger/ledger.txt
> pieces/display/frame_changed.txt
> pieces/display/current_frame.txt
> pieces/apps/fuzzpet_app/fuzzpet/last_response.txt
rm -f pieces/chtpm/state.txt

# 6. Initialize Clock
mkdir -p pieces/apps/fuzzpet_app/clock
echo "turn 0" > pieces/apps/fuzzpet_app/clock/state.txt
echo "time 08:00:00" >> pieces/apps/fuzzpet_app/clock/state.txt

# 7. Initialize Static Map (Terrain only)
cat > pieces/apps/fuzzpet_app/world/map.txt << EOF
####################
#  ...............T#
#  ...............T#
#  ....R..........T#
#  ....R..........T#
#  ....R..........T#
#  ....R..........T#
#  ................#
#                  #
####################
EOF

echo "Starting CHTPM+OS Pipeline (TPM Standard Mode)..."

# Run orchestrator
exec ./pieces/chtpm/plugins/+x/orchestrator.+x
