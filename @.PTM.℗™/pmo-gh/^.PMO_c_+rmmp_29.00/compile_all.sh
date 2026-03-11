#!/bin/bash

# compile_all.sh - Compile all CHTPM v0.01 executables
# TPM-Compliant: Binaries go to their respective piece directories.
# Updated: Macro folder structure (apps/fuzzpet_app/)

echo "=== Compiling CHTPM v0.01 (File-Based Communication) ==="
echo "Using macro folder structure: pieces/apps/fuzzpet_app/"
echo ""

# --- Editor App Piece (Isolated from FuzzPet) ---
echo "Compiling editor components..."
mkdir -p pieces/apps/editor/plugins/+x
gcc -o pieces/apps/editor/plugins/+x/editor_module.+x pieces/apps/editor/plugins/editor_module.c -pthread

echo "Compiling editor/ops..."
mkdir -p pieces/apps/editor/ops/+x
gcc -o pieces/apps/editor/ops/+x/place_tile.+x pieces/apps/editor/ops/place_tile.c
gcc -o pieces/apps/editor/ops/+x/delete_piece.+x pieces/apps/editor/ops/delete_piece.c

# --- FuzzPet Module (in macro folder) ---
echo "Compiling fuzzpet_app/fuzzpet..."
mkdir -p pieces/apps/fuzzpet_app/fuzzpet/plugins/+x
gcc -o pieces/apps/fuzzpet_app/fuzzpet/plugins/+x/fuzzpet.+x pieces/apps/fuzzpet_app/fuzzpet/plugins/fuzzpet.c

echo "Compiling fuzzpet_app/fuzzpet_reset..."
mkdir -p pieces/apps/fuzzpet_app/fuzzpet/plugins/+x
gcc -o pieces/apps/fuzzpet_app/fuzzpet/plugins/+x/fuzzpet_reset.+x pieces/apps/fuzzpet_app/fuzzpet/plugins/fuzzpet_reset.c

echo "Compiling fuzzpet_app/manager..."
mkdir -p pieces/apps/fuzzpet_app/manager/plugins/+x
gcc -o pieces/apps/fuzzpet_app/manager/plugins/+x/fuzzpet_manager.+x pieces/apps/fuzzpet_app/manager/fuzzpet_manager.c -pthread

# --- Keyboard Piece (system level) ---
echo "Compiling keyboard/keyboard_input..."
mkdir -p pieces/keyboard/plugins/+x
gcc -o pieces/keyboard/plugins/+x/keyboard_input.+x pieces/keyboard/plugins/keyboard_input.c

# --- Joystick Piece (system level) ---
echo "Compiling joystick/joystick_input..."
mkdir -p pieces/joystick/plugins/+x
gcc -o pieces/joystick/plugins/+x/joystick_input.+x pieces/joystick/plugins/joystick_input.c

# --- CHTPM Piece (system level) ---
echo "Compiling chtpm/chtpm_parser..."
mkdir -p pieces/chtpm/plugins/+x
gcc -o pieces/chtpm/plugins/+x/chtpm_parser.+x pieces/chtpm/plugins/chtpm_parser.c

echo "Compiling chtpm/chtpm_player..."
gcc -o pieces/chtpm/plugins/+x/chtpm_player.+x pieces/chtpm/plugins/chtpm_player.c

echo "Compiling chtpm/orchestrator..."
mkdir -p pieces/chtpm/plugins/+x
gcc -o pieces/chtpm/plugins/+x/orchestrator.+x pieces/chtpm/plugins/orchestrator.c -pthread

# --- Display Piece (system level) ---
echo "Compiling display/renderer..."
mkdir -p pieces/display/plugins/+x
gcc -o pieces/display/plugins/+x/renderer.+x pieces/display/renderer.c

echo "Compiling display/gl_renderer (with FreeType for emoji support)..."
mkdir -p pieces/display/plugins/+x
gcc -c -I/usr/include/freetype2 pieces/display/gl_renderer.c -o /tmp/gl_renderer.o 2>&1 && \
    gcc /tmp/gl_renderer.o -o pieces/display/plugins/+x/gl_renderer.+x -lGL -lglut -lX11 -lfreetype 2>&1 && \
    rm -f /tmp/gl_renderer.o || echo "  (GL renderer skipped - FreeType may be missing)"

# --- Master Ledger Pieces (system level) ---
echo "Compiling master_ledger/piece_manager..."
mkdir -p pieces/master_ledger/plugins/+x
gcc -o pieces/master_ledger/plugins/+x/piece_manager.+x pieces/master_ledger/plugins/piece_manager.c

echo "Compiling master_ledger/response_handler..."
mkdir -p pieces/master_ledger/plugins/+x
gcc -o pieces/master_ledger/plugins/+x/response_handler.+x pieces/master_ledger/plugins/response_handler.c

# --- Clock Piece (in macro folder) ---
echo "Compiling fuzzpet_app/clock/increment_time..."
mkdir -p pieces/apps/fuzzpet_app/clock/plugins/+x
gcc -o pieces/apps/fuzzpet_app/clock/plugins/+x/increment_time.+x pieces/apps/fuzzpet_app/clock/plugins/increment_time.c

echo "Compiling fuzzpet_app/clock/increment_turn..."
mkdir -p pieces/apps/fuzzpet_app/clock/plugins/+x
gcc -o pieces/apps/fuzzpet_app/clock/plugins/+x/increment_turn.+x pieces/apps/fuzzpet_app/clock/plugins/increment_turn.c

echo "Compiling fuzzpet_app/clock/end_turn..."
mkdir -p pieces/apps/fuzzpet_app/clock/plugins/+x
gcc -o pieces/apps/fuzzpet_app/clock/plugins/+x/end_turn.+x pieces/apps/fuzzpet_app/clock/plugins/end_turn.c

# --- World Piece (in macro folder) ---
echo "Compiling fuzzpet_app/world..."
mkdir -p pieces/apps/fuzzpet_app/world/plugins/+x
gcc -o pieces/apps/fuzzpet_app/world/plugins/+x/map_render.+x pieces/apps/fuzzpet_app/world/plugins/map_render.c

echo "Compiling fuzzpet_app/traits/move_entity..."
mkdir -p pieces/apps/fuzzpet_app/traits/+x
gcc -o pieces/apps/fuzzpet_app/traits/+x/move_entity.+x pieces/apps/fuzzpet_app/traits/move_entity.c

echo "Compiling fuzzpet_app/traits/storage_trait..."
mkdir -p pieces/apps/fuzzpet_app/traits/+x
gcc -o pieces/apps/fuzzpet_app/traits/+x/storage_trait.+x pieces/apps/fuzzpet_app/traits/storage_trait.c

echo "Compiling fuzzpet_app/traits/zombie_ai..."
mkdir -p pieces/apps/fuzzpet_app/traits/+x
gcc -o pieces/apps/fuzzpet_app/traits/+x/zombie_ai.+x pieces/apps/fuzzpet_app/traits/zombie_ai.c

# --- Player/Editor App Piece ---
echo "Compiling player_app components..."
mkdir -p pieces/apps/player_app/manager/plugins/+x
mkdir -p pieces/apps/player_app/world/plugins/+x
gcc -o pieces/apps/player_app/manager/plugins/+x/player_manager.+x pieces/apps/player_app/manager/player_manager.c -pthread
gcc -o pieces/apps/player_app/world/plugins/+x/player_render.+x pieces/apps/player_app/world/plugins/player_render.c
gcc -o pieces/apps/player_app/world/plugins/+x/menu_op.+x pieces/apps/player_app/world/plugins/menu_op.c
gcc -o pieces/apps/player_app/world/plugins/+x/project_loader.+x pieces/apps/player_app/world/plugins/project_loader.c
gcc -o pieces/apps/player_app/world/plugins/+x/move_z.+x pieces/apps/player_app/world/plugins/move_z.c
gcc -o pieces/apps/player_app/world/plugins/+x/interact.+x pieces/apps/player_app/world/plugins/interact.c
gcc -o pieces/apps/player_app/world/plugins/+x/place_tile.+x pieces/apps/player_app/world/plugins/place_tile.c

# --- GL-OS Piece (New Desktop) ---
echo "Compiling apps/gl_os..."
mkdir -p pieces/apps/gl_os/plugins/+x
gcc -o pieces/apps/gl_os/plugins/+x/gl_os.+x pieces/apps/gl_os/plugins/gl_os.c
gcc -o pieces/apps/gl_os/plugins/+x/gl_os_renderer.+x pieces/apps/gl_os/plugins/gl_os_renderer.c -lGL -lglut

# --- Shared Ops Library ---
echo "Compiling Shared Ops..."
mkdir -p pieces/apps/playrm/ops/+x
gcc -o pieces/apps/playrm/ops/+x/move_player.+x pieces/apps/playrm/ops/src/move_player.c
gcc -o pieces/apps/playrm/ops/+x/move_z.+x pieces/apps/playrm/ops/src/move_z.c
gcc -o pieces/apps/playrm/ops/+x/move_selector.+x pieces/apps/playrm/ops/src/move_selector.c
gcc -o pieces/apps/playrm/ops/+x/interact.+x pieces/apps/playrm/ops/src/interact.c
gcc -o pieces/apps/playrm/ops/+x/render_map.+x pieces/apps/playrm/ops/src/render_map.c
gcc -o pieces/apps/playrm/ops/+x/menu_op.+x pieces/apps/playrm/ops/src/menu_op.c
gcc -o pieces/apps/playrm/ops/+x/project_loader.+x pieces/apps/playrm/ops/src/project_loader.c
gcc -o pieces/apps/playrm/ops/+x/console_print.+x pieces/apps/playrm/ops/src/console_print.c
gcc -o pieces/apps/playrm/ops/+x/place_tile.+x pieces/apps/playrm/ops/src/place_tile.c
gcc -o pieces/apps/playrm/ops/+x/create_piece.+x pieces/apps/playrm/ops/src/create_piece.c
gcc -o pieces/apps/playrm/ops/+x/undo_action.+x pieces/apps/playrm/ops/src/undo_action.c
gcc -o pieces/apps/playrm/ops/+x/move_entity.+x pieces/apps/playrm/ops/src/move_entity.c
gcc -o pieces/apps/playrm/ops/+x/fuzzpet_action.+x pieces/apps/playrm/ops/src/fuzzpet_action.c
gcc -o pieces/apps/playrm/ops/+x/stat_decay.+x pieces/apps/playrm/ops/src/stat_decay.c

echo "Compiling playrm_module..."
mkdir -p pieces/apps/playrm/plugins/+x
gcc -o pieces/apps/playrm/plugins/+x/playrm_module.+x pieces/apps/playrm/plugins/playrm_module.c

echo "Compiling loader_module..."
mkdir -p pieces/apps/playrm/loader/plugins/+x
gcc -o pieces/apps/playrm/loader/plugins/+x/loader_module.+x pieces/apps/playrm/loader/loader_module.c

# --- Locations Piece (system level) ---
echo "Compiling path_utils..."
mkdir -p pieces/locations/+x
gcc -o pieces/locations/+x/path_utils.+x pieces/locations/path_utils.c

# --- OS Process Monitor ---
mkdir -p pieces/os/plugins/+x
gcc -o pieces/os/plugins/+x/proc_manager.+x pieces/os/plugins/proc_manager.c

# --- System Clock Daemon ---
mkdir -p pieces/system/clock_daemon/plugins/+x
gcc -o pieces/system/clock_daemon/plugins/+x/clock_daemon.+x \
     pieces/system/clock_daemon/plugins/clock_daemon.c

echo ""
echo "=== Compilation Complete ==="
echo "Executables created:"
find pieces -name "*.+x" -type f 2>/dev/null
