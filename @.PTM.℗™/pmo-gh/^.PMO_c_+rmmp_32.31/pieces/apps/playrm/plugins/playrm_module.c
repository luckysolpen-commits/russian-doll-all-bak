/*
 * playrm_module.c - Player Bridge Module
 * 
 * Responsibilities:
 * 1. Project Initialization & Loading
 * 2. Input Routing to Reusable Ops
 * 3. State Management (active_target_id, project_id)
 * 4. Triggering View Composition (render_map)
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
char current_project[MAX_LINE] = "template";
char active_target_id[MAX_LINE] = "hero";
char last_key_str[MAX_LINE] = "None";
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

void resolve_paths() {
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

void load_engine_state() {
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
                    if (strcmp(k, "project_id") == 0) strncpy(current_project, v, MAX_LINE-1);
                    else if (strcmp(k, "active_target_id") == 0) strncpy(active_target_id, v, MAX_LINE-1);
                    else if (strcmp(k, "current_z") == 0) current_z = atoi(v);
                }
            }
            fclose(f);
        }
        free(path);
    }
}

void save_engine_state() {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/player_app/manager/state.txt", project_root) != -1) {
        FILE *f = fopen(path, "w");
        if (f) {
            fprintf(f, "project_id=%s\n", current_project);
            fprintf(f, "active_target_id=%s\n", active_target_id);
            fprintf(f, "current_z=%d\n", current_z);
            fprintf(f, "last_key=%s\n", last_key_str);
            fclose(f);
        }
        free(path);
    }
}

void call_op(const char* op_name, const char* arg1, const char* arg2, const char* arg3) {
    char *cmd = NULL;
    if (arg3) {
        asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/%s.+x %s %s %s > /dev/null 2>&1",
                 project_root, op_name, arg1, arg2, arg3);
    } else if (arg2) {
        asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/%s.+x %s %s > /dev/null 2>&1",
                 project_root, op_name, arg1, arg2);
    } else {
        asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/%s.+x %s > /dev/null 2>&1",
                 project_root, op_name, arg1);
    }
    if (cmd) {
        system(cmd);
        free(cmd);
    }
}

void trigger_render() {
    char *cmd = NULL;
    if (asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/render_map.+x > /dev/null 2>&1", project_root) != -1) {
        system(cmd);
        free(cmd);
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

void run_project_loader() {
    char *cmd = NULL;
    if (asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/project_loader.+x", project_root) != -1) {
        system(cmd);
        free(cmd);
    }
    load_engine_state();
}

int is_active_layout() {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/display/current_layout.txt", project_root) == -1) return 0;
    FILE *f = fopen(path, "r");
    if (!f) { free(path); return 0; }
    char line[MAX_LINE];
    if (fgets(line, sizeof(line), f)) {
        fclose(f);
        int res = (strstr(line, "playrm/layouts/game.chtpm") != NULL || 
                   strstr(line, "main.chtpm") != NULL ||
                   strstr(line, "os.chtpm") != NULL); // Allow OS for background polling
        free(path);
        return res;
    }
    fclose(f);
    free(path);
    return 0;
}

void clear_response() {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/fuzzpet_app/fuzzpet/last_response.txt", project_root) != -1) {
        FILE *f = fopen(path, "w");
        if (f) fclose(f);
        free(path);
    }
}

int main() {
    resolve_paths();
    load_engine_state();

    // DEBUG: Log module start with timestamp
    {
        FILE *dbg = fopen("debug.txt", "a");
        if (dbg) {
            time_t rawtime;
            struct tm *timeinfo;
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            fprintf(dbg, "[%02d:%02d:%02d] Module START - project=%s, layout=game.chtpm\n",
                    timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, current_project);
            fclose(dbg);
        }
    }

    // Default target for fuzzpet project should be 'fuzzpet'
    if (strcmp(current_project, "fuzzpet_v2") == 0 && (strcmp(active_target_id, "hero") == 0 || strlen(active_target_id) == 0)) {
        strcpy(active_target_id, "fuzzpet");
        save_engine_state();
    }

    trigger_render();

    long last_pos = 0;
    struct stat st;
    char *history_path = NULL;
    /* TPM COMPLIANT: Poll LOCAL history.txt injected by Parser */
    if (asprintf(&history_path, "%s/pieces/apps/player_app/history.txt", project_root) == -1) return 1;

    int last_key_processed = -1;  /* For debouncing */

    while (1) {
        load_engine_state();

        if (!is_active_layout()) {
            usleep(16667);
            continue;
        }
        if (stat(history_path, &st) == 0) {
            if (st.st_size > last_pos) {
                FILE *hf = fopen(history_path, "r");
                if (hf) {
                    fseek(hf, last_pos, SEEK_SET);
                    char line[256];
                    while (fgets(line, sizeof(line), hf)) {
                        int key = -1;
                        /* Parse simple int or timestamped format */
                        if (isdigit(line[0])) key = atoi(line);
                        else if (strstr(line, "KEY_PRESSED: ")) {
                            key = atoi(strstr(line, "KEY_PRESSED: ") + 13);
                        } else {
                            // Try parsing raw number from the line (parser injects raw numbers)
                            key = atoi(line);
                        }
                        
                        if (key <= 0) continue;
                        if (key == last_key_processed) continue;
                        last_key_processed = key;

                        /* 0. Key Debug */
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
                                fprintf(dbg, "[%02d:%02d:%02d] KEY=%d (%s) project=%s target=%s\n",
                                        timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
                                        key, last_key_str, current_project, active_target_id);
                                fclose(dbg);
                            }
                        }

                        save_engine_state();

                        // HYBRID APPROACH: Movement direct, actions via PAL
                        if (strcmp(current_project, "fuzzpet_v2") == 0) {
                            // MOVEMENT: Direct op calls (works for arrows + WASD)
                            int moved = 0;
                            const char* dir = NULL;
                            if (key == 'w' || key == 'W' || key == 1002) { dir = "up"; moved = 1; }
                            else if (key == 's' || key == 'S' || key == 1003) { dir = "down"; moved = 1; }
                            else if (key == 'a' || key == 'A' || key == 1000) { dir = "left"; moved = 1; }
                            else if (key == 'd' || key == 'D' || key == 1001) { dir = "right"; moved = 1; }
                            
                            if (moved) {
                                call_op("move_entity", active_target_id, dir, NULL);
                                // Don't save here - let op handle state
                                trigger_render();
                                hit_frame_marker();
                                continue;  // Skip PAL for movement
                            }
                            
                            // ACTIONS: PAL script for complex logic
                            if (key >= '1' && key <= '7') {
                                char *prisc_cmd = NULL;
                                asprintf(&prisc_cmd, "PRISC_KEY=%d PRISC_ACTIVE_TARGET=%s PRISC_PROJECT_ROOT=%s PRISC_PROJECT_ID=%s %s/pieces/system/prisc/prisc+x %s/projects/%s/scripts/game_loop.asm '' '' %s/pieces/apps/player_app/manager/player_ops.txt > /dev/null 2>&1",
                                         key, active_target_id, project_root, current_project, project_root, project_root, current_project, project_root);
                                if (prisc_cmd) {
                                    system(prisc_cmd);
                                    free(prisc_cmd);
                                }
                                // PAL calls render + hit_frame
                            }
                            // Selector toggle (key 9)
                            else if (key == '9' || key == 27) {
                                if (strcmp(active_target_id, "selector") == 0)
                                    strcpy(active_target_id, "fuzzpet");
                                else
                                    strcpy(active_target_id, "selector");
                                save_engine_state();
                                trigger_render();
                                hit_frame_marker();
                            }
                        } else {
                            /* Default Built-in logic for other projects */
                            if (key == 'w' || key == 'W' || key == 1002) call_op("move_entity", active_target_id, "up", NULL);
                            else if (key == 's' || key == 'S' || key == 1003) call_op("move_entity", active_target_id, "down", NULL);
                            else if (key == 'a' || key == 'A' || key == 1000) call_op("move_entity", active_target_id, "left", NULL);
                            else if (key == 'd' || key == 'D' || key == 1001) call_op("move_entity", active_target_id, "right", NULL);
                            else if (key == '1') call_op("fuzzpet_action", active_target_id, "feed", NULL);
                            else if (key == '2') call_op("fuzzpet_action", active_target_id, "play", NULL);
                            else if (key == '3') call_op("fuzzpet_action", active_target_id, "sleep", NULL);
                            else if (key == '4') call_op("fuzzpet_action", active_target_id, "toggle_emoji", NULL);
                            else if (key == 10 || key == 13) call_op("interact", active_target_id, NULL, NULL);
                            else if (key == '9') {
                                if (strcmp(active_target_id, "hero") == 0 || strcmp(active_target_id, "fuzzpet") == 0)
                                    strcpy(active_target_id, "selector");
                                else {
                                    if (strcmp(current_project, "fuzzpet_v2") == 0) strcpy(active_target_id, "fuzzpet");
                                    else strcpy(active_target_id, "hero");
                                }
                                save_engine_state();
                            }
                            trigger_render();
                            hit_frame_marker();
                        }
                    }
                    last_pos = ftell(hf);
                    fclose(hf);
                }
            } else if (st.st_size < last_pos) {
                last_pos = 0;
            }
        }
        usleep(16667);
    }
    
    free(history_path);
    return 0;
}
