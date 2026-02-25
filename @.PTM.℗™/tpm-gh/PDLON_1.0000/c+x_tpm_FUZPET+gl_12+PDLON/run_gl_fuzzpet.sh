#!/bin/bash

# run_gl_fuzzpet.sh - Run the FuzzPet TPM +X system with GL renderer

echo "====================================="
echo "TPM +X FuzzPet System - GL Renderer"
echo "====================================="
echo ""
echo "Architecture: Orchestrator -> Keyboard -> Game Logic -> GL Renderer"
echo "Communication: File-based (pieces/keyboard/history.txt -> gl_display/current_frame.txt)"
echo ""
echo "Controls:"
echo "- Movement: w=up, s=down, a=left, d=right"
echo "- Actions: 1=feed, 2=play, 3=sleep, 4=status, 5=evolve, 6=end_turn"
echo "- Quit: Press Ctrl+C in terminal"
echo ""
echo "NOTE: GL window will open. Type in the keyboard input window."
echo "====================================="

# Ensure all needed directories and files exist
mkdir -p pieces/{fuzzpet,keyboard,master_ledger,display,gl_display,world}

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

if [ ! -f "pieces/gl_display/piece.txt" ]; then
    echo "Creating gl_display piece file..."
    cat > pieces/gl_display/piece.txt << 'EOF'
# Piece: gl_display
state:
  current_frame: Initial frame
  frame_count: 0
  last_render_time: none
  status: active
  auto_update_enabled: true
  render_mode: gl_emoji
methods: render_frame, update_frame, clear_display, get_frame, enable_auto_update, disable_auto_update
event_listeners: frame_requested, display_update, render_complete
responses:
  frame_requested: Processing frame request for GL rendering
  display_update: Updating GL display
  render_complete: GL frame rendered successfully
  default: GL display operation completed
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

# Create clock piece if it doesn't exist
mkdir -p pieces/clock
if [ ! -f "pieces/clock/state.txt" ]; then
    echo "Creating clock piece state..."
    cat > pieces/clock/state.txt << 'EOF'
time 08:00:00
time_increment_min 5
tick_counter 0
status active
turn 0
EOF
fi

# Clear history and ledger files to prevent processing old events/commands from previous sessions
> pieces/keyboard/history.txt
> pieces/master_ledger/master_ledger.txt
> pieces/master_ledger/ledger.txt
> pieces/gl_display/history.txt

echo "Starting the orchestrated pipeline with GL renderer..."
echo "1. GL window will open - type in GL window to control the pet"
echo "2. Terminal keyboard input ALSO works - both control the same game!"
echo "3. Orchestrator runs: keyboard_input, game_logic, renderer, response_handler, gl_renderer"
echo ""
echo "You can control the pet from EITHER the GL window OR the terminal!"
echo "Both inputs go to pieces/keyboard/history.txt and affect the same game state."

# Run the orchestrator (runs all 5 threads including gl_renderer)
exec +x/orchestrator.+x
