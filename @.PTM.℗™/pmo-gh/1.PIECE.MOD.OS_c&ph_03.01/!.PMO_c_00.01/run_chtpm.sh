#!/bin/bash

# run_chtpm.sh - Run the CHTPM v0.01 system
# TPM-Compliant: Uses macro folder structure (apps/fuzzpet_app/)

# Use a more portable way to get the script directory
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Set up location_kvp for dynamic path resolution
mkdir -p pieces/locations
cat > pieces/locations/location_kvp << EOF
project_root=${SCRIPT_DIR}
pieces_dir=${SCRIPT_DIR}/pieces
fuzzpet_app_dir=${SCRIPT_DIR}/pieces/apps/fuzzpet_app
fuzzpet_dir=${SCRIPT_DIR}/pieces/apps/fuzzpet_app/fuzzpet
system_dir=${SCRIPT_DIR}/pieces/system
EOF

# Ensure all needed directories exist (using macro folders)
mkdir -p pieces/apps/fuzzpet_app/{fuzzpet,keyboard,master_ledger,display,world,clock}
mkdir -p pieces/{system,chtpm,master_ledger,ui_components,ui_instances,locations}

# Clear history and ledger files for new session
> pieces/keyboard/history.txt
> pieces/apps/fuzzpet_app/fuzzpet/history.txt
> pieces/master_ledger/master_ledger.txt
> pieces/master_ledger/ledger.txt
> pieces/display/frame_changed.txt
> pieces/display/current_frame.txt

# Clear piece ledgers for new session
> pieces/keyboard/ledger.txt
> pieces/apps/fuzzpet_app/fuzzpet/ledger.txt
> pieces/apps/fuzzpet_app/world/ledger.txt
> pieces/apps/fuzzpet_app/clock/ledger.txt
> pieces/display/ledger.txt
> pieces/chtpm/ledger.txt

rm -f pieces/chtpm/state.txt

# Reset fuzzpet.pdl to ensure clean start (using macro folder path)
cat > pieces/apps/fuzzpet_app/fuzzpet/fuzzpet.pdl << EOF
SECTION      | KEY                | VALUE
----------------------------------------
META         | piece_id           | fuzzpet
META         | version            | 1.0
META         | determinism        | strict

STATE        | name               | Fuzzball
STATE        | hunger             | 50
STATE        | happiness          | 75
STATE        | energy             | 100
STATE        | level              | 1
STATE        | pos_x              | 5
STATE        | pos_y              | 5
STATE        | status             | active
STATE        | alive              | true

METHOD       | feed               | void
METHOD       | play               | void
METHOD       | sleep              | void
METHOD       | status             | void
METHOD       | evolve             | void
METHOD       | move_up            | void
METHOD       | move_down          | void
METHOD       | move_left          | void
METHOD       | move_right         | void

EVENT_IN     | fed                | void
EVENT_IN     | played             | void
EVENT_IN     | slept              | void
EVENT_IN     | evolved            | void
EVENT_IN     | moved              | void
EVENT_IN     | command_received   | void

RESPONSE     | fed                | Yum yum~ Feeling so happy! *tail wags wildly*
RESPONSE     | played             | Wheee~ Best playtime ever!
RESPONSE     | slept              | Zzz... feeling recharged!
RESPONSE     | evolved            | Wow! I evolved and got stronger!
RESPONSE     | moved              | *moves around excitedly*
RESPONSE     | command_received   | *looks at you curiously*
RESPONSE     | default            | *confused tilt* ...huh?
EOF

# Reset clock/state.txt (using macro folder path)
cat > pieces/apps/fuzzpet_app/clock/state.txt << EOF
turn 0
time 08:00:00
EOF

# Reset world/map.txt (using macro folder path)
cat > pieces/apps/fuzzpet_app/world/map.txt << EOF
####################
#  ...............T#
#  ...............T#
#  ....R...@......T#
#  ....R..........T#
#  ....R..........T#
#  ....R..........T#
#  ................#
#                  #
####################
EOF

echo "Starting CHTPM+OS Pipeline..."

# Run the orchestrator from the system piece
exec ./pieces/chtpm/plugins/+x/orchestrator.+x
