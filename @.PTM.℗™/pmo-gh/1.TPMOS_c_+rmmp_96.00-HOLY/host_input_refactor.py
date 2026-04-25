import sys

with open('pieces/apps/gl_os/plugins/gl_desktop.c', 'r') as f:
    lines = f.readlines()

# Find the keyboard function and modify the gltpm_app block
start_marker = 'void keyboard(unsigned char key, int x, int y) {'
end_marker = 'glutPostRedisplay();' # A known point after the relevant logic

start_idx = -1
end_idx = -1

for i, line in enumerate(lines):
    if 'void keyboard(unsigned char key, int x, int y)' in line:
        # Find the start of the gltpm_app handling block
        search_start = line.find('if (strcmp(win->type, "gltpm_app") == 0) {')
        if search_start != -1:
            start_idx = i
            break

if start_idx != -1:
    # Find the end of the relevant block (this is tricky, need to find matching braces or a clear end point)
    # The previous code had 'return;' after camera updates. Let's find that vicinity.
    search_end = -1
    for j in range(start_idx, len(lines)):
        if 'glutPostRedisplay();' in lines[j]: # Using this as a marker for end of local camera/mode setting
            search_end = j + 1
            break
    
    if search_end != -1:
        # Construct the new block for keyboard handling in gltpm_app
        new_gltpm_kb_logic = '''        if (strcmp(win->type, "gltpm_app") == 0) {
        DesktopWindow* win = &windows[active_idx];
        int in_map = win->is_map_control; // Use Manager's sovereign state for logic checks

        /* Host-driven Camera Fly/Pan & Mode Injection */
        if (in_map) {
            float speed = 0.2f;
            int injected_cmd = 0;
            char cmd_buffer[128];

            if (key == '1') { sprintf(cmd_buffer, "CAMERA_MODE:1"); injected_cmd = 1; }
            else if (key == '2') { sprintf(cmd_buffer, "CAMERA_MODE:2"); injected_cmd = 1; }
            else if (key == '3') { sprintf(cmd_buffer, "CAMERA_MODE:3"); injected_cmd = 1; }
            else if (key == '4') { sprintf(cmd_cmd_buffer, "CAMERA_MODE:4"); injected_cmd = 1; }
            else if (key == 'w' || key == 'W') { sprintf(cmd_buffer, "CAMERA_MOVE:0,0,%.2f", speed); injected_cmd = 1; }
            else if (key == 's' || key == 'S') { sprintf(cmd_buffer, "CAMERA_MOVE:0,0,-%.2f", speed); injected_cmd = 1; }
            else if (key == 'a' || key == 'A') { sprintf(cmd_buffer, "CAMERA_MOVE:-%.2f,0,0", speed); injected_cmd = 1; }
            else if (key == 'd' || key == 'D') { sprintf(cmd_buffer, "CAMERA_MOVE:%.2f,0,0", speed); injected_cmd = 1; }
            else if (key == 'z' || key == 'Z') { sprintf(cmd_buffer, "CAMERA_MOVE:0,-%.2f,0", speed); injected_cmd = 1; }
            else if (key == 'x' || key == 'X') { sprintf(cmd_buffer, "CAMERA_MOVE:0,%.2f,0", -speed); injected_cmd = 1; }
            else if (key == 'q') { sprintf(cmd_buffer, "CAMERA_ROTATE:0,-5.0,0"); injected_cmd = 1; }
            else if (key == 'e') { sprintf(cmd_buffer, "CAMERA_ROTATE:0,5.0,0"); injected_cmd = 1; }

            if (injected_cmd) {
                inject_command(cmd_buffer);
                glutPostRedisplay(); /* Ensure visual feedback */
                return; /* Yield control */
            }
            /* Fallback for other keys in map mode (e.g., ESC should be handled by Manager) */
        }
        /* If not in_map_control, let it fall through to general input injection (if any) */
        /* For now, assume other keys are not directly handled here for gltpm_app */

        /* Arrow keys are always injected for Xelector movement when in map mode */
        if (in_map && (key == GLUT_KEY_LEFT || key == GLUT_KEY_RIGHT || key == GLUT_KEY_UP || key == GLUT_KEY_DOWN)) {
             char history_entry[128];
             int tpm_key = 0;
             if (key == GLUT_KEY_LEFT) tpm_key = 1000; else if (key == GLUT_KEY_RIGHT) tpm_key = 1001;
             else if (key == GLUT_KEY_UP) tpm_key = 1002; else if (key == GLUT_KEY_DOWN) tpm_key = 1003;
             if (tpm_key > 0) {
                 sprintf(history_entry, "KEY_PRESSED: %d", tpm_key);
                 inject_command(history_entry);
             }
             glutPostRedisplay();
             return;
        }
    }
    /* Original keyboard handling for other types or when not in map control */
    /* ... rest of the keyboard function ... */
'''
        # Find the start and end of the block to replace
        block_start_idx = content.find('if (strcmp(win->type, "gltpm_app") == 0) {')
        if block_start_idx != -1:
            # Find the matching closing brace for this block
            brace_level = 0
            block_end_idx = -1
            for i in range(block_start_idx, len(content)):
                if content[i] == '{':
                    brace_count += 1
                elif content[i] == '}':
                    brace_count -= 1
                    if brace_count == 0:
                        block_end_idx = i + 1
                        break
            
            if block_end_idx != -1:
                # Replace the entire block
                content = content[:block_start_idx] + new_gltpm_kb_logic + content[block_end_idx:]
            else:
                print("Warning: Could not find closing brace for gltpm_app block. Manual check needed.")
        else:
            print("Warning: Could not find start of gltpm_app block.")

with open('pieces/apps/gl_os/plugins/gl_desktop.c', 'w') as f:
    f.write(content)

print("Host input refactoring applied. Attempting compilation and backup.")
