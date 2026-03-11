# fuzzpet_v2 game_loop.asm
# Responsibility: Input dispatch to ops for Fuzzpet V2

start:
    # Poll input from fuzzpet history
    # The loader/player manager handles the mapping of keys to the history file.
    read_history r0
    
    # Check for movement keys (w/a/s/d)
    li r1, 119         # 'w'
    beq r0, r1, move_up
    li r1, 115         # 's'
    beq r0, r1, move_down
    li r1, 97          # 'a'
    beq r0, r1, move_left
    li r1, 100         # 'd'
    beq r0, r1, move_right
    
    # Check for action keys (1=Feed, 2=Play, 3=Sleep, 4=Toggle Emoji)
    li r1, 49          # '1'
    beq r0, r1, feed
    li r1, 50          # '2'
    beq r0, r1, play
    li r1, 51          # '3'
    beq r0, r1, sleep
    li r1, 52          # '4'
    beq r0, r1, toggle_emoji
    
    j render

move_up:
    exec ./pieces/apps/playrm/ops/+x/move_entity.+x fuzzpet up
    exec ./pieces/apps/playrm/ops/+x/stat_decay.+x fuzzpet
    j render
move_down:
    exec ./pieces/apps/playrm/ops/+x/move_entity.+x fuzzpet down
    exec ./pieces/apps/playrm/ops/+x/stat_decay.+x fuzzpet
    j render
move_left:
    exec ./pieces/apps/playrm/ops/+x/move_entity.+x fuzzpet left
    exec ./pieces/apps/playrm/ops/+x/stat_decay.+x fuzzpet
    j render
move_right:
    exec ./pieces/apps/playrm/ops/+x/move_entity.+x fuzzpet right
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
toggle_emoji:
    # Custom toggle logic if needed, or use a shared op
    # For now, let's assume a toggle_emoji op exists or we use fuzzpet_action
    exec ./pieces/apps/playrm/ops/+x/fuzzpet_action.+x fuzzpet toggle_emoji
    j render

render:
    # Render view (shared op)
    exec ./pieces/apps/playrm/ops/+x/render_map.+x
    
    # Hit frame marker
    hit_frame
    j start
