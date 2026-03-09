#!/bin/bash

# run_chtpm.sh - Run the CHTPM v0.01 system (NUCLEAR CLEANUP v3.3)
# Responsibility: Clean environment, set location, launch orchestrator.

# 0. NUCLEAR CLEANUP
echo "Cleaning environment..."
pkill -9 -f "\.\+x"
pkill -9 -f "orchestrator"
pkill -9 -f "chtpm_parser"
pkill -9 -f "fuzzpet_manager"
pkill -9 -f "keyboard_input"

# DYNAMIC PATH RESOLUTION
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PM_BIN="./pieces/master_ledger/plugins/+x/piece_manager.+x"

# 1. Set up location_kvp (SMF-Compliant)
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

# 2. Reset all entities to known good state
echo "Resetting entities..."
"$PM_BIN" selector set-state pos_x 5
"$PM_BIN" selector set-state pos_y 5
"$PM_BIN" selector set-state pos_z 0
"$PM_BIN" selector set-state on_map 1
"$PM_BIN" selector set-state name Selector
"$PM_BIN" selector set-state emoji 🎯

"$PM_BIN" pet_01 set-state pos_x 5
"$PM_BIN" pet_01 set-state pos_y 2
"$PM_BIN" pet_01 set-state pos_z 0
"$PM_BIN" pet_01 set-state on_map 1
"$PM_BIN" pet_01 set-state name Fuzzball
"$PM_BIN" pet_01 set-state hunger 50
"$PM_BIN" pet_01 set-state happiness 55
"$PM_BIN" pet_01 set-state energy 100
"$PM_BIN" pet_01 set-state level 1
"$PM_BIN" pet_01 set-state emoji 🐶

"$PM_BIN" pet_02 set-state pos_x 10
"$PM_BIN" pet_02 set-state pos_y 5
"$PM_BIN" pet_02 set-state pos_z 0
"$PM_BIN" pet_02 set-state on_map 1
"$PM_BIN" pet_02 set-state name StorageCat
"$PM_BIN" pet_02 set-state hunger 50
"$PM_BIN" pet_02 set-state happiness 55
"$PM_BIN" pet_02 set-state energy 100
"$PM_BIN" pet_02 set-state level 1
"$PM_BIN" pet_02 set-state emoji 🐱

"$PM_BIN" zombie_01 set-state pos_x 15
"$PM_BIN" zombie_01 set-state pos_y 2
"$PM_BIN" zombie_01 set-state pos_z 0
"$PM_BIN" zombie_01 set-state on_map 1
"$PM_BIN" zombie_01 set-state name Zombie
"$PM_BIN" zombie_01 set-state emoji 🧟

# 3. Reset Manager State
mkdir -p pieces/apps/fuzzpet_app/manager
echo "active_target_id=selector" > pieces/apps/fuzzpet_app/manager/state.txt
echo "last_key=None" >> pieces/apps/fuzzpet_app/manager/state.txt

# 4. Reset Clock Daemon
mkdir -p pieces/system/clock_daemon
echo "turn=0" > pieces/system/clock_daemon/state.txt
echo "time=08:00:00" >> pieces/system/clock_daemon/state.txt
echo "mode=auto" >> pieces/system/clock_daemon/state.txt
echo "tick_rate=1" >> pieces/system/clock_daemon/state.txt

# 5. Reset FuzzPet State (emoji mode, etc)
mkdir -p pieces/apps/fuzzpet_app/fuzzpet
echo "name=Fuzzball" > pieces/apps/fuzzpet_app/fuzzpet/state.txt
echo "hunger=50" >> pieces/apps/fuzzpet_app/fuzzpet/state.txt
echo "happiness=55" >> pieces/apps/fuzzpet_app/fuzzpet/state.txt
echo "energy=100" >> pieces/apps/fuzzpet_app/fuzzpet/state.txt
echo "level=1" >> pieces/apps/fuzzpet_app/fuzzpet/state.txt
echo "pos_x=5" >> pieces/apps/fuzzpet_app/fuzzpet/state.txt
echo "pos_y=2" >> pieces/apps/fuzzpet_app/fuzzpet/state.txt
echo "pos_z=0" >> pieces/apps/fuzzpet_app/fuzzpet/state.txt
echo "status=active" >> pieces/apps/fuzzpet_app/fuzzpet/state.txt
echo "emoji_mode=0" >> pieces/apps/fuzzpet_app/fuzzpet/state.txt

# 6. Clear Sessions and Historical Data
> pieces/keyboard/history.txt
> pieces/keyboard/ledger.txt
> pieces/apps/fuzzpet_app/fuzzpet/history.txt
> pieces/apps/fuzzpet_app/fuzzpet/ledger.txt
> pieces/apps/fuzzpet_app/fuzzpet/view_changed.txt
> pieces/apps/fuzzpet_app/fuzzpet/last_response.txt
> pieces/display/frame_changed.txt
> pieces/display/current_frame.txt
> pieces/display/ledger.txt
> pieces/chtpm/ledger.txt
> pieces/joystick/ledger.txt
> pieces/master_ledger/master_ledger.txt
> pieces/master_ledger/ledger.txt
> pieces/master_ledger/frame_changed.txt
> pieces/apps/fuzzpet_app/manager/debug_log.txt
> pieces/os/proc_list.txt
> pieces/apps/fuzzpet_app/world/map.txt
> pieces/apps/fuzzpet_app/world/ledger.txt
> pieces/apps/fuzzpet_app/clock/ledger.txt
rm -f pieces/apps/fuzzpet_app/world/map_z*.txt 2>/dev/null
rm -rf pieces/debug/frames/* 2>/dev/null
mkdir -p pieces/debug/frames

# 7. Reset World Map (copy from static map)
cat > pieces/apps/fuzzpet_app/world/map.txt << 'EOF'
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

echo "Launching TPM Pipeline (Nuclear Cleanup Complete)..."
exec ./pieces/chtpm/plugins/+x/orchestrator.+x

