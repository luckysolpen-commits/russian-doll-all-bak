/*
 * fuzzpet_v2_module.c - FuzzPet V2 Simple Module (Editor-Style)
 * 
 * Pattern: Input → Module → Op → State → Render
 * No PAL, No ASM, No fork/exec complexity
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#define MAX_PATH 4096
#define MAX_LINE 256

char project_root[MAX_PATH] = ".";
char active_target_id[64] = "selector";
char last_key_str[64] = "None";
int current_z = 0;

char* trim_str(char *str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void resolve_root() {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), kvp)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(line);
                char *v = trim_str(eq + 1);
                if (strcmp(k, "project_root") == 0) snprintf(project_root, sizeof(project_root), "%s", v);
            }
        }
        fclose(kvp);
    }
}

void load_state() {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/player_app/manager/state.txt", project_root) != -1) {
        FILE *f = fopen(path, "r");
        if (f) {
            char line[MAX_LINE];
            while (fgets(line, sizeof(line), f)) {
                char *eq = strchr(line, '=');
                if (eq) {
                    *eq = '\0';
                    char *k = trim_str(line);
                    char *v = trim_str(eq + 1);
                    if (strcmp(k, "active_target_id") == 0) strncpy(active_target_id, v, sizeof(active_target_id)-1);
                    else if (strcmp(k, "current_z") == 0) current_z = atoi(v);
                }
            }
            fclose(f);
        }
        free(path);
    }
}

void save_state() {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/player_app/manager/state.txt", project_root) != -1) {
        FILE *f = fopen(path, "w");
        if (f) {
            fprintf(f, "project_id=fuzzpet_v2\n");
            fprintf(f, "active_target_id=%s\n", active_target_id);
            fprintf(f, "current_z=%d\n", current_z);
            fprintf(f, "last_key=%s\n", last_key_str);
            fclose(f);
        }
        free(path);
    }
}

void hit_frame_marker() {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/display/frame_changed.txt", project_root) != -1) {
        FILE *f = fopen(path, "a");
        if (f) { fprintf(f, "M\n"); fclose(f); }
        free(path);
    }
}

void trigger_render() {
    char *cmd = NULL;
    if (asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/render_map.+x", project_root) != -1) {
        system(cmd);
        free(cmd);
    }
}

int is_active_layout() {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/display/current_layout.txt", project_root) == -1) return 0;
    FILE *f = fopen(path, "r");
    if (!f) { free(path); return 0; }
    char line[MAX_LINE];
    if (fgets(line, sizeof(line), f)) {
        fclose(f);
        free(path);
        return (strstr(line, "fuzzpet_v2.chtpm") != NULL || strstr(line, "playrm/layouts/game.chtpm") != NULL);
    }
    fclose(f);
    free(path);
    return 0;
}

int main() {
    resolve_root();
    load_state();
    trigger_render();
    hit_frame_marker();

    // DEBUG: Log module start
    {
        FILE *dbg = fopen("debug.txt", "a");
        if (dbg) {
            time_t rawtime;
            struct tm *timeinfo;
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            fprintf(dbg, "[%02d:%02d:%02d] V2 MODULE START - project_root=%s\n",
                    timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, project_root);
            fclose(dbg);
        }
    }

    long last_pos = 0;
    struct stat st;
    char *history_path = NULL;
    /* FIX: Read from INJECTED history (where parser sends keys in INTERACT mode) */
    if (asprintf(&history_path, "%s/pieces/apps/player_app/history.txt", project_root) == -1) return 1;

    int last_key_processed = -1;

    fprintf(stderr, "[V2 MODULE] Started. project_root=%s\n", project_root);

    while (1) {
        if (!is_active_layout()) {
            usleep(16667);
            continue;
        }

        load_state();
        
        static time_t last_heartbeat = 0;
        time_t now = time(NULL);
        if (now - last_heartbeat >= 2) {
            fprintf(stderr, "[V2] heartbeat: target=%s last_key=%s\n", active_target_id, last_key_str);
            last_heartbeat = now;
        }

        if (stat(history_path, &st) == 0) {
            if (st.st_size > last_pos) {
                fprintf(stderr, "[V2] KEY DETECTED! size=%ld vs last_pos=%ld\n", st.st_size, last_pos);
                FILE *hf = fopen(history_path, "r");
                if (hf) {
                    fseek(hf, last_pos, SEEK_SET);
                    char line[256];
                    while (fgets(line, sizeof(line), hf)) {
                        // Parse "KEY_PRESSED: XXX" format
                        int key = 0;
                        char *kp = strstr(line, "KEY_PRESSED: ");
                        if (kp) {
                            key = atoi(kp + 13);
                        } else {
                            key = atoi(line);
                        }
                        if (key <= 0) continue;
                        if (key == last_key_processed) continue;
                        last_key_processed = key;

                        // Convert key to display string
                        if (key >= 32 && key <= 126) snprintf(last_key_str, sizeof(last_key_str), "%c", (char)key);
                        else if (key == 10 || key == 13) strcpy(last_key_str, "ENTER");
                        else if (key == 27) strcpy(last_key_str, "ESC");
                        else if (key == 1000) strcpy(last_key_str, "LEFT");
                        else if (key == 1001) strcpy(last_key_str, "RIGHT");
                        else if (key == 1002) strcpy(last_key_str, "UP");
                        else if (key == 1003) strcpy(last_key_str, "DOWN");
                        else snprintf(last_key_str, sizeof(last_key_str), "CODE %d", key);

                        // DEBUG: Log key processed
                        {
                            FILE *dbg = fopen("debug.txt", "a");
                            if (dbg) {
                                time_t rawtime;
                                struct tm *timeinfo;
                                time(&rawtime);
                                timeinfo = localtime(&rawtime);
                                fprintf(dbg, "[%02d:%02d:%02d] V2 KEY=%d (%s) target=%s\n",
                                        timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
                                        key, last_key_str, active_target_id);
                                fclose(dbg);
                            }
                        }

                        save_state();

                        // === SELECTOR CONTROLS ===
                        if (strcmp(active_target_id, "selector") == 0) {
                            char *cmd = NULL;
                            
                            // Movement
                            if (key == 119 || key == 1002) {  // w or UP
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/move_selector.+x up fuzzpet_v2", project_root);
                                system(cmd);
                                free(cmd);
                            }
                            else if (key == 115 || key == 1003) {  // s or DOWN
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/move_selector.+x down fuzzpet_v2", project_root);
                                system(cmd);
                                free(cmd);
                            }
                            else if (key == 97 || key == 1000) {  // a or LEFT
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/move_selector.+x left fuzzpet_v2", project_root);
                                system(cmd);
                                free(cmd);
                            }
                            else if (key == 100 || key == 1001) {  // d or RIGHT
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/move_selector.+x right fuzzpet_v2", project_root);
                                system(cmd);
                                free(cmd);
                            }
                            // Actions
                            else if (key == 50) {  // 2 - Scan
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/scan_op.+x", project_root);
                                system(cmd);
                                free(cmd);
                            }
                            else if (key == 51) {  // 3 - Collect
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/collect_op.+x", project_root);
                                system(cmd);
                                free(cmd);
                            }
                            else if (key == 52) {  // 4 - Place
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/place_op.+x", project_root);
                                system(cmd);
                                free(cmd);
                            }
                            else if (key == 53) {  // 5 - Inventory
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/inventory_op.+x", project_root);
                                system(cmd);
                                free(cmd);
                            }
                            else if (key == 10 || key == 13) {  // ENTER - Inspect
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/inspect_tile.+x", project_root);
                                system(cmd);
                                free(cmd);
                            }
                            else if (key == 57 || key == 113) {  // 9 or q - Toggle
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/toggle_selector.+x", project_root);
                                system(cmd);
                                free(cmd);
                                load_state();  // Reload after toggle
                            }
                        }
                        // === PET CONTROLS ===
                        else if (strcmp(active_target_id, "fuzzpet") == 0) {
                            char *cmd = NULL;
                            
                            // Movement
                            if (key == 119 || key == 1002) {  // w or UP
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/move_entity.+x fuzzpet up", project_root);
                                system(cmd);
                                free(cmd);
                            }
                            else if (key == 115 || key == 1003) {  // s or DOWN
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/move_entity.+x fuzzpet down", project_root);
                                system(cmd);
                                free(cmd);
                            }
                            else if (key == 97 || key == 1000) {  // a or LEFT
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/move_entity.+x fuzzpet left", project_root);
                                system(cmd);
                                free(cmd);
                            }
                            else if (key == 100 || key == 1001) {  // d or RIGHT
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/move_entity.+x fuzzpet right", project_root);
                                system(cmd);
                                free(cmd);
                            }
                            // Actions
                            else if (key == 49) {  // 1 - Feed
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/fuzzpet_action.+x fuzzpet feed", project_root);
                                system(cmd);
                                free(cmd);
                            }
                            else if (key == 50) {  // 2 - Play
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/fuzzpet_action.+x fuzzpet play", project_root);
                                system(cmd);
                                free(cmd);
                            }
                            else if (key == 51) {  // 3 - Sleep
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/fuzzpet_action.+x fuzzpet sleep", project_root);
                                system(cmd);
                                free(cmd);
                            }
                            else if (key == 52) {  // 4 - Evolve
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/fuzzpet_action.+x fuzzpet evolve", project_root);
                                system(cmd);
                                free(cmd);
                            }
                            else if (key == 53) {  // 5 - Toggle Emoji
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/fuzzpet_action.+x fuzzpet toggle_emoji", project_root);
                                system(cmd);
                                free(cmd);
                            }
                            else if (key == 57 || key == 113) {  // 9 or q - Toggle to selector
                                asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/toggle_selector.+x", project_root);
                                system(cmd);
                                free(cmd);
                                load_state();  // Reload after toggle
                            }
                        }

                        // Render and signal after every key
                        trigger_render();
                        hit_frame_marker();
                    }
                    last_pos = ftell(hf);
                    fclose(hf);
                }
            } else if (st.st_size < last_pos) {
                last_pos = 0;  // File truncated
            }
        }
        
        usleep(16667);  // 60fps cap
    }
    
    free(history_path);
    return 0;
}
