#!/bin/bash

# run_chtpm.sh - Run the CHTPM v0.01 system (NUCLEAR CLEANUP v3.3)
# Responsibility: Clean environment, set location, launch orchestrator.

# 0. NUCLEAR CLEANUP
echo "Cleaning environment..."
./kill_all.sh

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
editor_dir=${SCRIPT_DIR}/pieces/apps/editor
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
# Create project-based piece structure for op-ed
mkdir -p projects/op-ed/pieces/selector

# Selector
cat > projects/op-ed/pieces/selector/state.txt << EOF
name=Selector
type=selector
pos_x=5
pos_y=5
pos_z=0
on_map=1
EOF

# 3. Reset Manager State
mkdir -p pieces/apps/fuzzpet_app/manager
echo "active_target_id=selector" > pieces/apps/fuzzpet_app/manager/state.txt
echo "last_key=None" >> pieces/apps/fuzzpet_app/manager/state.txt

mkdir -p projects/op-ed/manager
echo "project_id=op-ed" > projects/op-ed/manager/state.txt
echo "active_target_id=selector" >> projects/op-ed/manager/state.txt
echo "current_z=0" >> projects/op-ed/manager/state.txt
echo "last_key=None" >> projects/op-ed/manager/state.txt
> projects/op-ed/history.txt

# User App State
mkdir -p pieces/apps/user/pieces/session
echo "session_status=auth_required" > pieces/apps/user/pieces/session/state.txt
echo "current_user=guest" >> pieces/apps/user/pieces/session/state.txt

mkdir -p pieces/apps/player_app/manager
echo "project_id=template" > pieces/apps/player_app/manager/state.txt
echo "active_target_id=hero" >> pieces/apps/player_app/manager/state.txt
echo "current_z=0" >> pieces/apps/player_app/manager/state.txt
echo "last_key=None" >> pieces/apps/player_app/manager/state.txt

# 4.1 Reset Editor State (Isolated from FuzzPet)
mkdir -p pieces/apps/editor
echo "gui_focus=1" > pieces/apps/editor/state.txt
echo "emoji_mode=0" >> pieces/apps/editor/state.txt
echo "cursor_x=5" >> pieces/apps/editor/state.txt
echo "cursor_y=5" >> pieces/apps/editor/state.txt
echo "cursor_z=0" >> pieces/apps/editor/state.txt
echo "last_key=None" >> pieces/apps/editor/state.txt
echo "project_id=template" >> pieces/apps/editor/state.txt
echo "map_idx=0" >> pieces/apps/editor/state.txt
echo "glyph_idx=0" >> pieces/apps/editor/state.txt

mkdir -p pieces/apps/editor/manager
echo "project_id=template" > pieces/apps/editor/manager/state.txt
echo "active_target_id=selector" >> pieces/apps/editor/manager/state.txt
echo "current_z=0" >> pieces/apps/editor/manager/state.txt
echo "last_key=None" >> pieces/apps/editor/manager/state.txt

mkdir -p pieces/apps/editor/pieces/selector
echo "pos_x=5" > pieces/apps/editor/pieces/selector/state.txt
echo "pos_y=5" >> pieces/apps/editor/pieces/selector/state.txt
echo "pos_z=0" >> pieces/apps/editor/pieces/selector/state.txt
echo "active=1" >> pieces/apps/editor/pieces/selector/state.txt
echo "icon=X" >> pieces/apps/editor/pieces/selector/state.txt

# 4. Reset Clock Daemon
mkdir -p pieces/system/clock_daemon
echo "turn=0" > pieces/system/clock_daemon/state.txt
echo "time=08:00:00" >> pieces/system/clock_daemon/state.txt
echo "mode=turn" >> pieces/system/clock_daemon/state.txt
echo "tick_rate=1" >> pieces/system/clock_daemon/state.txt

# 5. Reset FuzzPet State (emoji mode, etc) - ISOLATED FROM EDITOR
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

mkdir -p pieces/world/map_01/selector
cat > pieces/world/map_01/selector/state.txt << EOF
name=Selector
type=selector
pos_x=5
pos_y=5
pos_z=0
on_map=1
emoji=🎯
EOF

# 6. Clear Sessions and Historical Data - ISOLATED PER-APP
> pieces/keyboard/history.txt
> pieces/keyboard/ledger.txt
> pieces/apps/player_app/history.txt  # Clear injected keys
> pieces/apps/player_app/state.txt    # Clear stale state

# Editor history (isolated from fuzzpet)
> pieces/apps/editor/history.txt
> pieces/apps/editor/view.txt
> pieces/apps/editor/view_changed.txt
> pieces/apps/editor/response.txt

# FuzzPet history (legacy - do not modify)
> pieces/apps/fuzzpet_app/fuzzpet/history.txt
> pieces/apps/fuzzpet_app/fuzzpet/ledger.txt
> pieces/apps/fuzzpet_app/fuzzpet/view_changed.txt
> pieces/apps/fuzzpet_app/fuzzpet/last_response.txt

# Global display
> pieces/display/frame_changed.txt
> pieces/display/current_frame.txt
> pieces/display/ledger.txt

# System ledger
> pieces/chtpm/ledger.txt
> pieces/joystick/ledger.txt
> pieces/master_ledger/master_ledger.txt
> pieces/master_ledger/ledger.txt
> pieces/master_ledger/frame_changed.txt

# Debug
> pieces/apps/fuzzpet_app/manager/debug_log.txt
> pieces/os/proc_list.txt
> debug.txt  # Module debug log (cleared each run)

# World maps (fuzzpet-specific)
> pieces/apps/fuzzpet_app/world/map.txt
> pieces/apps/fuzzpet_app/world/ledger.txt
> pieces/apps/fuzzpet_app/clock/ledger.txt
rm -f pieces/apps/fuzzpet_app/world/map_z*.txt 2>/dev/null
rm -rf pieces/debug/frames/* 2>/dev/null
mkdir -p pieces/debug/frames

# 7. Reset World Map (copy from static map) - FUZZPET WORLD ONLY
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
echo "  - Editor: Isolated (pieces/apps/editor/)"
echo "  - FuzzPet: Isolated (pieces/apps/fuzzpet_app/)"

# Launch PAL Watchdog
chmod +x ./pal_watchdog.sh
./pal_watchdog.sh &

exec ./pieces/chtpm/plugins/+x/orchestrator.+x

