#!/bin/bash

# run_fuzzpet.sh - Run the FuzzPet TPM +X compliant system

# Set project root in location_kvp (for dynamic path resolution)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
mkdir -p pieces/locations
echo "project_root=${SCRIPT_DIR}" > pieces/locations/location_kvp
echo "pieces_dir=${SCRIPT_DIR}/pieces" >> pieces/locations/location_kvp

echo "====================================="
echo "TPM +X FuzzPet System"
echo "====================================="
echo ""
echo "Controls:"
echo "- Movement: w=up, s=down, a=left, d=right"
echo "- Actions: 1=feed, 2=play, 3=sleep, 4=status, 5=evolve, 6=end_turn"
echo "- Quit: Press Ctrl+C"
echo ""
echo "====================================="

# Ensure all needed directories exist
mkdir -p pieces/{fuzzpet,keyboard,master_ledger,display,world,clock}

# Clear history and ledger files for new session
> pieces/keyboard/history.txt
> pieces/master_ledger/master_ledger.txt
> pieces/master_ledger/ledger.txt
> pieces/display/frame_changed.txt

echo "Starting the orchestrated pipeline..."
echo ""

# Run the orchestrator from the system piece
exec ./pieces/system/plugins/+x/orchestrator.+x
