# fuzzpet_v2 game_loop.asm
# Responsibility: Main Game Loop for Fuzzpet V2 (Encapsulated)

start:
    # 1. Read Active Target from Environment via VM Op
    read_active_target r2
    
    # 2. Poll input from environment (passed by module via PRISC_KEY)
    read_env_key r0
    beq r0, x0, render # No input, just render
    
    # 3. Mode Branching
    li r1, 1
    beq r2, r1, selector_input
    j player_input
    
player_input:
    # Check for selector toggle (Key 9 or 'q')
    li r1, 57 # 9
    beq r0, r1, toggle_sel
    li r1, 113 # q
    beq r0, r1, toggle_sel
    
    # Movement keys
    li r1, 119 # w
    beq r0, r1, move_up
    li r1, 115 # s
    beq r0, r1, move_down
    li r1, 97  # a
    beq r0, r1, move_left
    li r1, 100 # d
    beq r0, r1, move_right
    
    # Arrow keys
    li r1, 1002 # UP
    beq r0, r1, move_up
    li r1, 1003 # DOWN
    beq r0, r1, move_down
    li r1, 1000 # LEFT
    beq r0, r1, move_left
    li r1, 1001 # RIGHT
    beq r0, r1, move_right
    
    # Actions - ASCII codes for number keys
    li r1, 49 # 1 - Feed
    beq r0, r1, feed
    li r1, 50 # 2 - Play
    beq r0, r1, play
    li r1, 51 # 3 - Sleep
    beq r0, r1, sleep
    li r1, 52 # 4 - Evolve
    beq r0, r1, evolve
    li r1, 53 # 5 - Toggle Emoji
    beq r0, r1, toggle_emoji
    
    j render

selector_input:
    # Key 9 or 'q' to return to player
    li r1, 57
    beq r0, r1, toggle_sel
    li r1, 113
    beq r0, r1, toggle_sel
    
    # Movement
    li r1, 119 # w
    beq r0, r1, sel_up
    li r1, 115 # s
    beq r0, r1, sel_down
    li r1, 97  # a
    beq r0, r1, sel_left
    li r1, 100 # d
    beq r0, r1, sel_right

    # Arrow keys
    li r1, 1002 # UP
    beq r0, r1, sel_up
    li r1, 1003 # DOWN
    beq r0, r1, sel_down
    li r1, 1000 # LEFT
    beq r0, r1, sel_left
    li r1, 1001 # RIGHT
    beq r0, r1, sel_right
    
    # Actions
    li r1, 50 # 2 - Scan
    beq r0, r1, scan_tile
    li r1, 51 # 3 - Collect
    beq r0, r1, collect_item
    li r1, 52 # 4 - Place
    beq r0, r1, place_item
    li r1, 53 # 5 - Inventory
    beq r0, r1, show_inventory
    
    # Enter to Inspect
    li r1, 10
    beq r0, r1, inspect
    li r1, 13
    beq r0, r1, inspect
    
    j render

# --- Player Ops ---
move_up:
    exec ./pieces/apps/playrm/ops/+x/move_entity.+x fuzzpet up
    j post_move
move_down:
    exec ./pieces/apps/playrm/ops/+x/move_entity.+x fuzzpet down
    j post_move
move_left:
    exec ./pieces/apps/playrm/ops/+x/move_entity.+x fuzzpet left
    j post_move
move_right:
    exec ./pieces/apps/playrm/ops/+x/move_entity.+x fuzzpet right
    j post_move

post_move:
    exec ./pieces/apps/playrm/ops/+x/stat_decay.+x fuzzpet
    j render

feed:
    exec ./pieces/apps/playrm/ops/+x/fuzzpet_action.+x fuzzpet feed
    j render
play:
    exec ./pieces/apps/playrm/ops/+x/fuzzpet_action.+x fuzzpet play
    j render
sleep:
    exec ./pieces/apps/playrm/ops/+x/fuzzpet_action.+x fuzzpet sleep
    j render
evolve:
    exec ./pieces/apps/playrm/ops/+x/fuzzpet_action.+x fuzzpet evolve
    j render
toggle_emoji:
    exec ./pieces/apps/playrm/ops/+x/fuzzpet_action.+x fuzzpet toggle_emoji
    j render

# --- Selector Ops ---
toggle_sel:
    exec ./pieces/apps/playrm/ops/+x/toggle_selector.+x
    j render

sel_up:
    exec ./pieces/apps/playrm/ops/+x/move_selector.+x up fuzzpet_v2
    j render
sel_down:
    exec ./pieces/apps/playrm/ops/+x/move_selector.+x down fuzzpet_v2
    j render
sel_left:
    exec ./pieces/apps/playrm/ops/+x/move_selector.+x left fuzzpet_v2
    j render
sel_right:
    exec ./pieces/apps/playrm/ops/+x/move_selector.+x right fuzzpet_v2
    j render

scan_tile:
    exec ./pieces/apps/playrm/ops/+x/scan_op.+x
    j render

collect_item:
    exec ./pieces/apps/playrm/ops/+x/collect_op.+x
    j render

place_item:
    exec ./pieces/apps/playrm/ops/+x/place_op.+x
    j render

show_inventory:
    exec ./pieces/apps/playrm/ops/+x/inventory_op.+x
    j render

inspect:
    exec ./pieces/apps/playrm/ops/+x/inspect_tile.+x
    j render

render:
    exec ./pieces/apps/playrm/ops/+x/render_map.+x
    hit_frame
    halt
