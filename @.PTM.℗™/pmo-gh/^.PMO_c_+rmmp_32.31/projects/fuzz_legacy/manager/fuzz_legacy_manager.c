#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <dirent.h>

// TPM FuzzLegacy Manager (v1.0 - PROJECT AWARE)
// Responsibility: Route input for legacy projects, sync project mirrors.

char current_project[64] = "fuzz_legacy";
char active_target_id[64] = "selector";
char last_key_str[64] = "None";
int emoji_mode = 0;

// Path cache
char project_root[2048] = ".";
char history_path[4096] = "pieces/apps/player_app/history.txt";
char debug_log_path[4096] = "projects/fuzz_legacy/manager/debug_log.txt";

char* trim_str(char *str) {
    char *end;
    if(!str) return str;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

char* build_path_malloc(const char* rel) {
    size_t sz = strlen(project_root) + strlen(rel) + 2;
    char* p = (char*)malloc(sz);
    if (p) snprintf(p, sz, "%s/%s", project_root, rel);
    return p;
}

void resolve_paths() {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[2048];
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
    
    // Resolve project_id from manager state
    char *mgr_state = build_path_malloc("pieces/apps/player_app/manager/state.txt");
    FILE *mf = fopen(mgr_state, "r");
    if (mf) {
        char line[1024];
        while (fgets(line, sizeof(line), mf)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                if (strcmp(trim_str(line), "project_id") == 0) strncpy(current_project, trim_str(eq + 1), 63);
            }
        }
        fclose(mf);
    }
    free(mgr_state);

    snprintf(debug_log_path, sizeof(debug_log_path), "%s/projects/%s/manager/debug_log.txt", project_root, current_project);
    snprintf(history_path, sizeof(history_path), "%s/pieces/apps/player_app/history.txt", project_root);
}

int read_exec_output(const char* path, char* out_buf, size_t out_sz, const char* a1, const char* a2, const char* a3, const char* a4) {
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) return -1;
    pid_t pid = fork();
    if (pid == 0) {
        close(pipe_fd[0]); dup2(pipe_fd[1], STDOUT_FILENO); close(pipe_fd[1]);
        execl(path, path, a1, a2, a3, a4, NULL); exit(1);
    } else if (pid > 0) {
        close(pipe_fd[1]);
        ssize_t n = read(pipe_fd[0], out_buf, out_sz - 1);
        if (n >= 0) out_buf[n] = '\0';
        close(pipe_fd[0]); waitpid(pid, NULL, 0); return 0;
    }
    return -1;
}

void log_debug(const char* fmt, ...) {
    FILE *f = fopen(debug_log_path, "a");
    if (f) {
        va_list args; va_start(args, fmt);
        fprintf(f, "[%ld] ", time(NULL)); vfprintf(f, fmt, args); fprintf(f, "\n");
        va_end(args); fclose(f);
    }
}

void log_resp(const char* msg) {
    // Write to project piece's last_response.txt
    char path[4096];
    snprintf(path, sizeof(path), "%s/projects/%s/pieces/fuzzpet/last_response.txt", project_root, current_project);
    FILE *f = fopen(path, "w");
    if (f) { fprintf(f, "%-57s", msg); fclose(f); }
}

void get_state_fast(const char* piece_id, const char* key, char* out_val) {
    char path[4096], line[4096];
    // Prioritize project-specific pieces
    snprintf(path, sizeof(path), "%s/projects/%s/pieces/%s/state.txt", project_root, current_project, piece_id);
    
    if (access(path, F_OK) != 0) {
        // Fallback to global world
        snprintf(path, sizeof(path), "%s/pieces/world/map_01/%s/state.txt", project_root, piece_id);
    }

    FILE *f = fopen(path, "r");
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                if (strcmp(trim_str(line), key) == 0) {
                    strncpy(out_val, trim_str(eq + 1), 255);
                    fclose(f); return;
                }
            }
        }
        fclose(f);
    }
    strcpy(out_val, "unknown");
}

int get_state_int_fast(const char* piece_id, const char* key) {
    char val[256]; get_state_fast(piece_id, key, val);
    if (strcmp(val, "unknown") == 0 || val[0] == '\0') return -1;
    return atoi(val);
}

void perform_mirror_sync() {
    char path[4096];
    // Sync project piece mirror
    snprintf(path, sizeof(path), "%s/projects/%s/pieces/fuzzpet/state.txt", project_root, current_project);
    FILE *f = fopen(path, "w");
    if (!f) return;
    
    if (strcmp(active_target_id, "selector") == 0) {
        fprintf(f, "name=SELECTOR\nhunger=0\nhappiness=0\nenergy=0\nlevel=0\nstatus=active\nemoji_mode=%d\n", emoji_mode);
    } else {
        char name[64], hunger[64], happiness[64], energy[64], level[64];
        get_state_fast(active_target_id, "name", name);
        get_state_fast(active_target_id, "hunger", hunger);
        get_state_fast(active_target_id, "happiness", happiness);
        get_state_fast(active_target_id, "energy", energy);
        get_state_fast(active_target_id, "level", level);
        fprintf(f, "name=%s\nhunger=%s\nhappiness=%s\nenergy=%s\nlevel=%s\nstatus=active\nemoji_mode=%d\n", 
                name, hunger, happiness, energy, level, emoji_mode);
    }
    fclose(f);
}

void save_manager_state() {
    char path[4096];
    // Update player_app manager state
    snprintf(path, sizeof(path), "%s/pieces/apps/player_app/manager/state.txt", project_root);
    FILE *f = fopen(path, "w");
    if (f) { 
        fprintf(f, "project_id=%s\nactive_target_id=%s\nlast_key=%s\ncurrent_z=0\n", current_project, active_target_id, last_key_str); 
        fclose(f); 
    }
}

void trigger_render() {
    char* render = build_path_malloc("pieces/apps/playrm/ops/+x/render_map.+x");
    pid_t p = fork();
    if (p == 0) {
        freopen("/dev/null", "w", stdout); freopen("/dev/null", "w", stderr);
        execl(render, render, NULL); exit(1);
    } else if (p > 0) waitpid(p, NULL, 0);
    free(render);
}

void route_input(int key) {
    if (key >= 32 && key <= 126) snprintf(last_key_str, sizeof(last_key_str), "%c", (char)key);
    else if (key == 10 || key == 13) strcpy(last_key_str, "ENTER");
    else if (key == 27) strcpy(last_key_str, "ESC");
    else if (key == 1000 || key == 2100) strcpy(last_key_str, "LEFT");
    else if (key == 1001 || key == 2101) strcpy(last_key_str, "RIGHT");
    else if (key == 1002 || key == 2102) strcpy(last_key_str, "UP");
    else if (key == 1003 || key == 2103) strcpy(last_key_str, "DOWN");
    
    if (key == '9' || key == 27 || key == 2008) { 
        strcpy(active_target_id, "selector"); 
        log_resp("Returned to Selector."); 
    }
    
    if (strcmp(active_target_id, "selector") == 0 && (key == 10 || key == 13 || key == 2000)) {
        int cx = get_state_int_fast("selector", "pos_x"); 
        int cy = get_state_int_fast("selector", "pos_y");
        
        char pieces_dir[4096];
        snprintf(pieces_dir, sizeof(pieces_dir), "%s/projects/%s/pieces", project_root, current_project);
        DIR *dir = opendir(pieces_dir);
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_name[0] == '.' || strcmp(entry->d_name, "selector") == 0) continue;
                int tx = get_state_int_fast(entry->d_name, "pos_x");
                int ty = get_state_int_fast(entry->d_name, "pos_y");
                if (tx == cx && ty == cy) {
                    strncpy(active_target_id, entry->d_name, 63);
                    char msg[128]; snprintf(msg, sizeof(msg), "Selected %s.", entry->d_name); log_resp(msg);
                    break;
                }
            }
            closedir(dir);
        }
    }
    
    if (key == 'w' || key == 'a' || key == 's' || key == 'd' || (key >= 1000 && key <= 1003)) {
        char* trait = build_path_malloc("pieces/apps/playrm/ops/+x/move_entity.+x");
        char dir_str[16];
        if (key == 'w' || key == 1002) strcpy(dir_str, "up");
        else if (key == 's' || key == 1003) strcpy(dir_str, "down");
        else if (key == 'a' || key == 1000) strcpy(dir_str, "left");
        else if (key == 'd' || key == 1001) strcpy(dir_str, "right");
        
        pid_t p = fork();
        if (p == 0) {
            freopen("/dev/null", "w", stdout); freopen("/dev/null", "w", stderr);
            execl(trait, trait, active_target_id, dir_str, NULL); exit(1);
        } else waitpid(p, NULL, 0);
        free(trait);

        // SHADOW SELECTOR SYNC: If not selector, sync selector to active entity
        if (strcmp(active_target_id, "selector") != 0) {
            int px = get_state_int_fast(active_target_id, "pos_x");
            int py = get_state_int_fast(active_target_id, "pos_y");
            if (px != -1 && py != -1) {
                char px_s[32], py_s[32]; snprintf(px_s, sizeof(px_s), "%d", px); snprintf(py_s, sizeof(py_s), "%d", py);
                char* pm = build_path_malloc("pieces/master_ledger/plugins/+x/piece_manager.+x");
                p = fork(); if (p == 0) { execl(pm, pm, "selector", "set-state", "pos_x", px_s, NULL); exit(1); } else waitpid(p, NULL, 0);
                p = fork(); if (p == 0) { execl(pm, pm, "selector", "set-state", "pos_y", py_s, NULL); exit(1); } else waitpid(p, NULL, 0);
                free(pm);
            }
        }

        /* Call zombie AI to chase player */
        char* zombie_cmd = build_path_malloc("pieces/apps/fuzzpet_app/traits/+x/zombie_ai.+x");
        p = fork();
        if (p == 0) {
            freopen("/dev/null", "w", stdout); freopen("/dev/null", "w", stderr);
            execl(zombie_cmd, zombie_cmd, "zombie_01", active_target_id, NULL); exit(1);
        } else waitpid(p, NULL, 0);
        free(zombie_cmd);
    }
    
    perform_mirror_sync();
    save_manager_state();
    trigger_render();
}

int is_active_layout() {
    char *path = build_path_malloc("pieces/display/current_layout.txt");
    FILE *f = fopen(path, "r");
    if (!f) { free(path); return 0; }
    char line[1024];
    if (fgets(line, sizeof(line), f)) {
        fclose(f);
        int res = (strstr(line, "game.chtpm") != NULL);
        free(path);
        return res;
    }
    fclose(f); free(path);
    return 0;
}

void* input_thread(void* arg __attribute__((unused))) {
    long last_pos = 0; struct stat st;
    while (1) {
        if (!is_active_layout()) { usleep(100000); continue; }
        if (stat(history_path, &st) == 0) {
            if (st.st_size > last_pos) {
                FILE *hf = fopen(history_path, "r");
                if (hf) { fseek(hf, last_pos, SEEK_SET); int key; while (fscanf(hf, "%d", &key) == 1) route_input(key); last_pos = ftell(hf); fclose(hf); }
            } else if (st.st_size < last_pos) last_pos = 0;
        }
        usleep(16667);
    }
    return NULL;
}

int main() {
    resolve_paths();
    save_manager_state(); perform_mirror_sync(); trigger_render();
    pthread_t t; pthread_create(&t, NULL, input_thread, NULL);
    while (1) { usleep(1000000); }
    return 0;
}
