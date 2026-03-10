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
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/pieces/apps/player_app/manager/state.txt", project_root);
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
}

void save_engine_state() {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/pieces/apps/player_app/manager/state.txt", project_root);
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "project_id=%s\n", current_project);
        fprintf(f, "active_target_id=%s\n", active_target_id);
        fprintf(f, "current_z=%d\n", current_z);
        fprintf(f, "last_key=%s\n", last_key_str);
        fclose(f);
    }
}

void call_op(const char* op_name, const char* arg1, const char* arg2, const char* arg3) {
    char cmd[MAX_PATH * 2];
    if (arg3) {
        snprintf(cmd, sizeof(cmd), "%s/pieces/apps/playrm/ops/+x/%s.+x %s %s %s > /dev/null 2>&1",
                 project_root, op_name, arg1, arg2, arg3);
    } else if (arg2) {
        snprintf(cmd, sizeof(cmd), "%s/pieces/apps/playrm/ops/+x/%s.+x %s %s > /dev/null 2>&1",
                 project_root, op_name, arg1, arg2);
    } else {
        snprintf(cmd, sizeof(cmd), "%s/pieces/apps/playrm/ops/+x/%s.+x %s > /dev/null 2>&1",
                 project_root, op_name, arg1);
    }
    system(cmd);
}

void trigger_render() {
    char cmd[MAX_PATH];
    snprintf(cmd, sizeof(cmd), "%s/pieces/apps/playrm/ops/+x/render_map.+x > /dev/null 2>&1", project_root);
    system(cmd);
}

void run_project_loader() {
    char cmd[MAX_PATH];
    snprintf(cmd, sizeof(cmd), "%s/pieces/apps/playrm/ops/+x/project_loader.+x", project_root);
    system(cmd);
    load_engine_state();
}

int main() {
    resolve_paths();
    load_engine_state();
    
    if (strcmp(current_project, "template") == 0 || strlen(current_project) == 0) {
        run_project_loader();
    }
    
    trigger_render();
    
    long last_pos = 0;
    struct stat st;
    char history_path[MAX_PATH];
    snprintf(history_path, sizeof(history_path), "%s/pieces/apps/fuzzpet_app/fuzzpet/history.txt", project_root);
    
    while (1) {
        if (stat(history_path, &st) == 0) {
            if (st.st_size > last_pos) {
                FILE *hf = fopen(history_path, "r");
                if (hf) {
                    fseek(hf, last_pos, SEEK_SET);
                    int key;
                    while (fscanf(hf, "%d", &key) == 1) {
                        /* 0. Key Debug */
                        if (key >= 32 && key <= 126) snprintf(last_key_str, sizeof(last_key_str), "%c", (char)key);
                        else if (key == 10 || key == 13) strcpy(last_key_str, "ENTER");
                        else if (key == 27) strcpy(last_key_str, "ESC");
                        else if (key == 1000) strcpy(last_key_str, "LEFT");
                        else if (key == 1001) strcpy(last_key_str, "RIGHT");
                        else if (key == 1002) strcpy(last_key_str, "UP");
                        else if (key == 1003) strcpy(last_key_str, "DOWN");
                        else snprintf(last_key_str, sizeof(last_key_str), "CODE %d", key);
                        
                        save_engine_state();

                        /* 1. Movement */
                        if (key == 'w' || key == 'W' || key == 1002) call_op("move_player", active_target_id, "up", NULL);
                        else if (key == 's' || key == 'S' || key == 1003) call_op("move_player", active_target_id, "down", NULL);
                        else if (key == 'a' || key == 'A' || key == 1000) call_op("move_player", active_target_id, "left", NULL);
                        else if (key == 'd' || key == 'D' || key == 1001) call_op("move_player", active_target_id, "right", NULL);
                        
                        /* 2. Z-Level */
                        else if (key == 'x' || key == 'X') call_op("move_z", active_target_id, "up", NULL);
                        else if (key == 'z' || key == 'Z') call_op("move_z", active_target_id, "down", NULL);
                        
                        /* 3. Interaction */
                        else if (key == 10 || key == 13) call_op("interact", active_target_id, NULL, NULL);
                        
                        /* 4. Target Toggle (Testing) */
                        else if (key == '9') {
                            if (strcmp(active_target_id, "hero") == 0) strcpy(active_target_id, "selector");
                            else strcpy(active_target_id, "hero");
                            save_engine_state();
                            call_op("console_print", "Focus changed to", active_target_id, NULL);
                        }
                        
                        trigger_render();
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
    
    return 0;
}
