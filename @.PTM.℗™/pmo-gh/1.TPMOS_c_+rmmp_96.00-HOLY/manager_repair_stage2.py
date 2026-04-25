import sys

with open('projects/fuzz-op-gl/manager/fuzz-op-gl_manager.c', 'r') as f:
    content = f.read()

# 1. Refactor is_map_control handling in gltpm_input_thread
old_is_map_control_logic = """                            if (strcmp(action, "INTERACT") == 0) { // Host toggles map control
                                is_map_control = !is_map_control;
                                log_resp(is_map_control ? "Map Control ACTIVE" : "Menu Mode ACTIVE");
                                save_manager_state();
                                trigger_render();
                            }
                            else if (strcmp(action, "OP fuzz-op-gl::toggle_map_mode") == 0) {
                                is_map_control = !is_map_control;
                                log_resp(is_map_control ? "Map Control ACTIVE" : "Menu Mode ACTIVE");
                                save_manager_state();
                                trigger_render();
                            }"""

new_is_map_control_logic = """                            else if (strcmp(action, "INTERACT") == 0) { // Host signals absolute map control
                                is_map_control = 1;
                                log_resp("Map Control ACTIVE");
                                save_manager_state();
                                trigger_render();
                            }
                            else if (strcmp(action, "ESC") == 0) { // Host signals menu mode
                                is_map_control = 0;
                                log_resp("Menu Mode ACTIVE");
                                save_manager_state();
                                trigger_render();
                            }"""
content = content.replace(old_is_map_control_logic, new_is_map_control_logic)

# 2. Modify gltpm_input_thread to ignore local camera controls and process injected commands
old_camera_key_handling_in_gltpm_thread = """                                    /* 1. Camera Fly Controls (WASD/ZX/QE) */
                                    int moved = 0;
                                    if (key_code == '1') { camera_mode = 1; moved = 1; }
                                    else if (key_code == '2') { camera_mode = 2; moved = 1; }
                                    else if (key_code == '3') { camera_mode = 3; moved = 1; }
                                    else if (key_code == '4') { camera_mode = 4; moved = 1; }
                                    else if (key_code == 'w') { cam_pos[2] += speed; moved = 1; }
                                    else if (key_code == 's') { cam_pos[2] -= speed; moved = 1; }
                                    else if (key_code == 'a') { cam_pos[0] -= speed; moved = 1; }
                                    else if (key_code == 'd') { cam_pos[0] += speed; moved = 1; }
                                    else if (key_code == 'z') { cam_pos[1] += speed; moved = 1; }
                                    else if (key_code == 'x') { cam_pos[1] -= speed; moved = 1; }
                                    else if (key_code == 'q') { cam_rot[1] -= 5.0f; moved = 1; }
                                    else if (key_code == 'e') { cam_rot[1] += 5.0f; moved = 1; }"""

new_camera_key_handling_in_gltpm_thread = """                                    /* 1. Camera Fly/Mode Controls (Authority: Manager via injected commands) */
                                    /* Host handles local WASD fly. Manager parses injected commands for mode and state changes. */
                                    int moved = 0;
                                    if (key_code == '1') { camera_mode = 1; moved = 1; }
                                    else if (key_code == '2') { camera_mode = 2; moved = 1; }
                                    else if (key_code == '3') { camera_mode = 3; moved = 1; }
                                    else if (key_code == '4') { camera_mode = 4; moved = 1; }"""
content = content.replace(old_camera_key_handling_in_gltpm_thread, new_camera_key_handling_in_gltpm_thread)

# 3. Ensure save_manager_state includes camera state
old_save_manager_state = """    if (f) {
        fprintf(f, "project_id=%s\\nactive_target_id=%s\\nlast_key=%s\\ncurrent_z=%d\\n", current_project, active_target_id, last_key_str, current_z_val);
        fprintf(f, "current_map=map_01_z%d.txt\\n", current_z_val);
        fclose(f);
    }"""

new_save_manager_state = """    if (f) {
        fprintf(f, "project_id=%s\\nactive_target_id=%s\\nlast_key=%s\\ncurrent_z=%d\\n", current_project, active_target_id, last_key_str, current_z_val);
        fprintf(f, "current_map=map_01_z%d.txt\\n", current_z_val);
        /* Save camera state */
        fprintf(f, "camera_mode=%d\\n", camera_mode);
        fprintf(f, "cam_x=%.2f\\ncam_y=%.2f\\ncam_z=%.2f\\n", cam_pos[0], cam_pos[1], cam_pos[2]);
        fprintf(f, "cam_pitch=%.2f\\ncam_yaw=%.2f\\ncam_roll=%.2f\\n", cam_rot[0], cam_rot[1], cam_rot[2]);
        fclose(f);
    }"""
content = content.replace(old_save_manager_state, new_save_manager_state)

with open('projects/fuzz-op-gl/manager/fuzz-op-gl_manager.c', 'w') as f:
    f.write(content)

print("Manager modifications applied. Attempting compilation.")
