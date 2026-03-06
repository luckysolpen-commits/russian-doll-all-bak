#!/bin/bash

# run_chtpm.sh - Run the CHTPM v0.01 system (MODULAR REFACTOR v3.2)
# TPM-Compliant: Uses modular entity pieces and registry.

# DYNAMIC PATH RESOLUTION (Standard SMF/TPM)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PM_BIN="./pieces/master_ledger/plugins/+x/piece_manager.+x"

# 0. Clean up orphaned processes
# If kill_all.sh is missing, we create a simple one
if [ ! -f ./kill_all.sh ]; then
    cat > ./kill_all.sh << 'EOF'
#!/bin/bash
pkill -f "orchestrator.+x"
pkill -f "chtpm_parser.+x"
pkill -f "renderer.+x"
pkill -f "gl_renderer.+x"
pkill -f "clock_daemon.+x"
pkill -f "response_handler.+x"
pkill -f "keyboard_input.+x"
pkill -f "joystick_input.+x"
pkill -f "fuzzpet_manager.+x"
EOF
    chmod +x ./kill_all.sh
fi
bash ./kill_all.sh

# 1. Set up location_kvp (SMF-Compliant) - DYNAMIC
mkdir -p pieces/locations
cat > pieces/locations/location_kvp << EOF
project_root=${SCRIPT_DIR}
pieces_dir=${SCRIPT_DIR}/pieces
fuzzpet_app_dir=${SCRIPT_DIR}/pieces/apps/fuzzpet_app
fuzzpet_dir=${SCRIPT_DIR}/pieces/apps/fuzzpet_app/fuzzpet
system_dir=${SCRIPT_DIR}/pieces/system
pet_01_dir=${SCRIPT_DIR}/pieces/world/map_01/pet_01
pet_02_dir=${SCRIPT_DIR}/pieces/world/map_01/pet_02
selector_dir=${SCRIPT_DIR}/pieces/world/map_01/selector
os_procs_dir=${SCRIPT_DIR}/pieces/os/procs
clock_daemon_dir=${SCRIPT_DIR}/pieces/system/clock_daemon
manager_dir=${SCRIPT_DIR}/pieces/apps/fuzzpet_app/manager
EOF

# 2. [DEPRECATED] Initialize World Registry
rm -f pieces/apps/fuzzpet_app/world/registry.txt

# 3. Initialize Pieces (DNA and Mirror Sync)
# The piece_manager resolves paths via location_kvp
"$PM_BIN" pet_01 set-state pos_x 5
"$PM_BIN" pet_01 set-state pos_y 2
"$PM_BIN" pet_01 set-state on_map 1
"$PM_BIN" pet_01 set-state name Fuzzball

"$PM_BIN" pet_02 set-state pos_x 10
"$PM_BIN" pet_02 set-state pos_y 5
"$PM_BIN" pet_02 set-state on_map 1
"$PM_BIN" pet_02 set-state name StorageCat

"$PM_BIN" selector set-state pos_x 5
"$PM_BIN" selector set-state pos_y 5
"$PM_BIN" selector set-state on_map 1
"$PM_BIN" selector set-state name Cursor

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

# 6. Initialize Clock Daemon (System Level) - TPM 13.00 Standard
mkdir -p pieces/system/clock_daemon
echo "turn=0" > pieces/system/clock_daemon/state.txt
echo "time=08:00:00" >> pieces/system/clock_daemon/state.txt
echo "mode=auto" >> pieces/system/clock_daemon/state.txt
echo "tick_rate=1" >> pieces/system/clock_daemon/state.txt

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
