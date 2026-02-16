#!/bin/bash

# run_fuzzpet.sh - Run the FuzzPet TPM +X compliant system

echo "====================================="
echo "TPM +X FuzzPet System - Orchestrated-MVC"
echo "====================================="
echo ""
echo "Architecture: Orchestrator -> Keyboard -> Game Logic -> Renderer"
echo "Communication: File-based (pieces/keyboard/history.txt -> save_map.txt)"
echo ""
echo "Controls:"
echo "- Movement: w=up, s=down, a=left, d=right"
echo "- Actions: 1=feed, 2=play, 3=sleep, 4=status, 5=evolve"
echo "- Quit: Press Ctrl+C to exit (in keyboard input window)"
echo ""
echo "NOTE: Only renderer prints to console for display output"
echo "====================================="

# Ensure all needed directories and files exist
mkdir -p pieces/{fuzzpet,keyboard,master_ledger,display,world}

# Create initial map file if it doesn't exist
if [ ! -f "pieces/world/map.txt" ]; then
    echo "Creating initial world map..."
    cat > pieces/world/map.txt << 'EOF'
####################
#  ...............T#
#  ...............T#
#  ....R@.........T#
#  ....R..........T#
#  ....R..........T#
#  ....R..........T#
#  ................#
#                  #
####################
EOF
fi

# Create initial piece files if they don't exist
if [ ! -f "pieces/fuzzpet/piece.txt" ]; then
    echo "Creating fuzzpet piece file..."
    cat > pieces/fuzzpet/piece.txt << 'EOF'
# Piece: fuzzpet
state:
  name: Fuzzball
  hunger: 50
  happiness: 75
  energy: 100
  level: 1
  pos_x: 5
  pos_y: 5
  status: active
  alive: true
methods: feed, play, sleep, status, evolve, move_up, move_down, move_left, move_right
event_listeners: fed, played, slept, evolved, moved, command_received
responses:
  fed: Yum yum~ Feeling so happy! *tail wags wildly*
  played: Wheee~ Best playtime ever! *bounces around joyfully*
  slept: Zzz... all cozy now~ *yawns cutely* Feeling recharged!
  evolved: *glows brightly* Level up! I'm getting stronger~!
  moved: *happily moves* New spot, new adventure~
  command_received: *ears perk up* Ooh, you said something? *tilts head curiously*
  default: *confused tilt* ...huh?
EOF
fi

if [ ! -f "pieces/keyboard/piece.txt" ]; then
    echo "Creating keyboard piece file..."
    cat > pieces/keyboard/piece.txt << 'EOF'
# Piece: keyboard
state:
  session_id: 12345
  current_key_log_path: pieces/keyboard/history.txt
  logging_enabled: true
  keys_recorded: 0
  last_key_logged: none
  last_key_code: -1
  last_key_time: none
  status: active
methods: log_key, clear_log, get_last_key, reset_session
event_listeners: key_logged, input_received, session_started
responses:
  key_logged: Key pressed and logged successfully
  input_received: Input received and processed
  session_started: Keyboard session initialized
  default: Keyboard input processed
EOF
fi

if [ ! -f "pieces/master_ledger/piece.txt" ]; then
    echo "Creating master_ledger piece file..."
    cat > pieces/master_ledger/piece.txt << 'EOF'
# Piece: master_ledger
state:
  status: active
  last_entry_id: 0
  total_entries: 0
  entries_processed: 0
methods: log_event, read_log, search_log, clear_log, get_status
event_listeners: system_event, method_call, state_change, event_fired
responses:
  system_event: Processing system event in master ledger
  method_call: Logging method call to master ledger
  state_change: Recording state change in master ledger
  event_fired: Event logged to master ledger
  default: Master ledger operation completed
EOF
fi

if [ ! -f "pieces/display/piece.txt" ]; then
    echo "Creating display piece file..."
    cat > pieces/display/piece.txt << 'EOF'
# Piece: display
state:
  current_frame: Initial frame
  frame_count: 0
  last_render_time: none
  status: active
  auto_update_enabled: true
methods: render_frame, update_frame, clear_display, get_frame, enable_auto_update, disable_auto_update
event_listeners: frame_requested, display_update, render_complete
responses:
  frame_requested: Processing frame request
  display_update: Updating display
  render_complete: Frame rendered successfully
  default: Display operation completed
EOF
fi

if [ ! -f "pieces/world/piece.txt" ]; then
    echo "Creating world piece file..."
    cat > pieces/world/piece.txt << 'EOF'
# Piece: world
state:
  width: 20
  height: 10
  current_map: default
  objects_count: 0
  status: active
methods: load_map, save_map, add_object, remove_object, get_map, update_map
event_listeners: map_loaded, map_saved, object_added, object_removed
responses:
  map_loaded: Game world map loaded successfully
  map_saved: Game world map saved successfully
  object_added: Object added to the world
  object_removed: Object removed from the world
  default: World operation completed
EOF
fi

# Make sure we have the history and ledger files
touch pieces/keyboard/history.txt pieces/master_ledger/master_ledger.txt pieces/master_ledger/ledger.txt

# Create save_map file for renderer
touch save_map.txt

echo "Starting the orchestrated pipeline..."
echo "1. Orchestrator launching keyboard input, game logic, and renderer..."
echo "2. Only renderer prints to console (for display output)"
echo "3. Type in the keyboard input window to control the pet..."
echo ""
echo "Switch to the keyboard input window after starting!"

# Run the orchestrator which starts all components
exec +x/orchestrator.+x