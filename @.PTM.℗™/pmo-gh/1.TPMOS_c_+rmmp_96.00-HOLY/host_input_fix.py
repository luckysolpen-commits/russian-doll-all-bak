import sys

with open('pieces/apps/gl_os/plugins/gl_desktop.c', 'r') as f:
    lines = f.readlines()

# Modify keyboard() function for gltpm_app type
# Target: Find the block handling keyboard input within the gltpm_app type check.
# Strategy:
# 1. Remove local camera_mode updates ('1'-'4'). Manager will be sovereign.
# 2. Implement WASD/ZXQE direct camera fly/pan in Host when in_map is true.
# 3. Inject CAMERA_MOVE commands to history after Host local movement.
# 4. Ensure Arrow keys are still injected for Manager to process Xelector movement.

modified_lines = []
in_gltpm_app_block = False
in_keyboard_func = False
camera_mode_handled = False
wasd_handled = False
arrow_key_handled = False
map_control_check_line = -1

for i, line in enumerate(lines):
    if 'void keyboard(unsigned char key, int x, int y)' in line:
        in_keyboard_func = True
    if in_keyboard_func and 'if (strcmp(win->type, "gltpm_app") == 0)' in line:
        in_gltpm_app_block = True
    
    if in_gltpm_app_block:
        if 'if (win->is_map_control)' in line:
            map_control_check_line = i

        # Remove local camera mode setting
        if line.strip().startswith("if (key == '1'") or \
           line.strip().startswith("else if (key == '2'") or \
           line.strip().startswith("else if (key == '3'") or \
           line.strip().startswith("else if (key == '4'"):
            continue # Skip lines that set camera_mode locally

        # Insert WASD camera fly logic and CAMERA_MOVE injection
        if in_map_control_check_line != -1 and i > in_map_control_check_line:
            if 'return;' in line and not camera_mode_handled: # Found the return after camera mode setup
                # Inject WASD logic BEFORE the return/injection logic
                new_block = """
            /* Host-driven Camera Fly Controls (WASD/ZXQE) */
            float speed = 0.2f;
            int camera_moved = 0;
            if (key == 'w' || key == 'W') { win->cam_pos[2] += speed; camera_moved = 1; }
            else if (key == 's' || key == 'S') { win->cam_pos[2] -= speed; camera_moved = 1; }
            else if (key == 'a' || key == 'A') { win->cam_pos[0] -= speed; camera_moved = 1; }
            else if (key == 'd') { win->cam_pos[0] += speed; camera_moved = 1; }
            else if (key == 'z') { win->cam_pos[1] += speed; camera_moved = 1; }
            else if (key == 'x') { win->cam_pos[1] -= speed; camera_moved = 1; }
            else if (key == 'q') { win->cam_rot[1] -= 5.0f; camera_moved = 1; }
            else if (key == 'e') { win->cam_rot[1] += 5.0f; camera_moved = 1; }

            if (camera_moved) {
                char history_entry[128];
                snprintf(history_entry, sizeof(history_entry), "CAMERA_MOVE:%.2f,%.2f,%.2f:%.1f,%.1f,%.1f",
                         win->cam_pos[0], win->cam_pos[1], win->cam_pos[2],
                         win->cam_rot[0], win->cam_rot[1], win->cam_rot[2]);
                inject_command(history_entry);
                glutPostRedisplay(); /* Immediate visual feedback */
                return; /* Yield control */
            }
"""
                lines.insert(i + 1, new_block) # Insert after the 'if (in_map)' block opens
                camera_mode_handled = True # Prevent re-injection logic
            
            # Arrow keys injection should remain for Xelector
            if key == GLUT_KEY_LEFT or key == GLUT_KEY_RIGHT or key == GLUT_KEY_UP or key == GLUT_KEY_DOWN:
                # This part is already handled by injecting KEY_PRESSED, keep it.
                pass
        
        if 'strcmp(win->type, "gltpm_app") == 0' in line:
             in_gltpm_app_block = True # Ensure we are in the correct block

    # Add a check to ensure we are inside the correct gltpm_app block before modifying
    # This is tricky without full parsing. The previous replacement targeted a general area.
    # Let's assume the previous logic for injecting arrow keys is still desired.
    # The main issue was host controlling camera locally AND manager reacting to injected keys.
    # By removing local camera control and injecting commands for manager, we delegate.

    # Based on previous code, the 'in_map' block had camera mode setting AND key injection.
    # We remove local camera mode setting.
    # We keep WASD/ZXQE injection.
    # Manager will handle these injected keys.

    # The critical part is ensuring Manager handles camera_mode AND cam_pos/rot.
    # And Host only injects.

    # Need to rewrite the logic block for keyboard() within gltpm_app
    # Find the entire block from 'if (in_map)' to its closing brace.
    # This is complex. Let's try a simpler approach:
    # 1. Ensure local camera mode setting is gone.
    # 2. Ensure WASD is injected.
    # 3. Ensure ESC injection works.
    # 4. Ensure Arrow keys injection works.

    # For now, I'll assume the prior Python script correctly applied the WASD part,
    # and focus on ensuring ONLY injection happens from Host for camera mode.
    # The code was like:
    # if (in_map) {
    #    if (key == '1') win->camera_mode = 1; ...
    #    ... WASD for camera_moved ...
    #    if (camera_moved) { inject command; return; }
    #    ... rest of injections ...
    # } else { /* not in_map */ ... }

    # The previous Python script ALREADY removed local camera mode setting.
    # It also added WASD handling for camera_moved locally in Host and return.
    # This contradicts the blueprint of Host injecting WASD for Manager.

    # Let's undo the local camera control in Host and ensure WASD is injected.
    # And ensure camera mode injection happens.

    old_gltpm_kb_block = """        if (in_map) {
            /* Specifically handle camera mode shortcuts locally for visual feedback */
            if (key == '1') win->camera_mode = 1;
            else if (key == '2') win->camera_mode = 2;
            else if (key == '3') win->camera_mode = 3;
            else if (key == '4') win->camera_mode = 4;

            char *history_path = NULL;"""

    new_gltpm_kb_block = """        if (in_map) {
            /* Inject camera mode and movement commands to Manager */
            char history_entry[128];
            int injected = 0;

            if (key == '1') { sprintf(history_entry, "CAMERA_MODE:1"); injected = 1; }
            else if (key == '2') { sprintf(history_entry, "CAMERA_MODE:2"); injected = 1; }
            else if (key == '3') { sprintf(history_entry, "CAMERA_MODE:3"); injected = 1; }
            else if (key == '4') { sprintf(history_entry, "CAMERA_MODE:4"); injected = 1; }
            else {
                float speed = 0.2f;
                if (key == 'w' || key == 'W') { sprintf(history_entry, "CAMERA_MOVE:0,0,%.2f", speed); injected = 1; }
                else if (key == 's' || key == 'S') { sprintf(history_entry, "CAMERA_MOVE:0,0,-%.2f", speed); injected = 1; }
                else if (key == 'a' || key == 'A') { sprintf(history_entry, "CAMERA_MOVE:-%.2f,0,0", speed); injected = 1; }
                else if (key == 'd' || key == 'D') { sprintf(history_entry, "CAMERA_MOVE:%.2f,0,0", speed); injected = 1; }
                else if (key == 'z' || key == 'Z') { sprintf(history_entry, "CAMERA_MOVE:0,-%.2f,0", speed); injected = 1; }
                else if (key == 'x' || key == 'X') { sprintf(history_entry, "CAMERA_MOVE:0,%.2f,0", -speed); injected = 1; }
                else if (key == 'q') { sprintf(history_entry, "CAMERA_ROTATE:0,-5.0,0"); injected = 1; }
                else if (key == 'e') { sprintf(history_entry, "CAMERA_ROTATE:0,5.0,0"); injected = 1; }
            }

            if (injected) {
                inject_command(history_entry);
                glutPostRedisplay(); /* Ensure visual feedback if needed */
                return; /* Yield control */
            }
            
            /* Fallback for other keys */
            char *history_path = NULL;"""

content = content.replace(old_gltpm_kb_block, new_gltpm_kb_block)

# Ensure arrow keys are also injected and handled
# (Already handled by existing code structure injecting KEY_PRESSED)

with open('pieces/apps/gl_os/plugins/gl_desktop.c', 'w') as f:
    f.writelines(content)

print("Refactoring Host keyboard handling for gltpm_app.")
print("Attempting compilation...")
