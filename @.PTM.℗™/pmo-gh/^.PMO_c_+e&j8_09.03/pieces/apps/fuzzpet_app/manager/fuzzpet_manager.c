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

// TPM FuzzPet Manager (v4.3.4 - MARCH 5 STABILITY)
// Responsibility: Route input, sync mirror, capture responses.

char active_target_id[64] = "selector";
char last_key_str[64] = "None";
int emoji_mode = 0;

// Path cache for dynamic resolution - increased to 2048 for safety
char project_root[2048] = ".";
char pieces_dir[2048] = "pieces";
char history_path[4096] = "pieces/apps/fuzzpet_app/fuzzpet/history.txt";
char debug_log_path[4096] = "pieces/apps/fuzzpet_app/manager/debug_log.txt";

char* trim_str(char *str) {
    char *end;
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

// Dynamic Path Resolution
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
                if (strcmp(k, "project_root") == 0) strncpy(project_root, v, sizeof(project_root)-1);
                else if (strcmp(k, "pieces_dir") == 0) strncpy(pieces_dir, v, sizeof(pieces_dir)-1);
                else if (strcmp(k, "fuzzpet_dir") == 0) {
                    snprintf(history_path, sizeof(history_path), "%s/history.txt", v);
                }
            }
        }
        fclose(kvp);
    }
    snprintf(debug_log_path, sizeof(debug_log_path), "%s/pieces/apps/fuzzpet_app/manager/debug_log.txt", project_root);
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
    static long last_log_time = 0;
    long now = time(NULL);
    if (now == last_log_time && strstr(fmt, "idle")) return;
    
    FILE *f = fopen(debug_log_path, "a");
    if (f) {
        va_list args;
        va_start(args, fmt);
        fprintf(f, "[%ld] ", now);
        vfprintf(f, fmt, args);
        fprintf(f, "\n");
        va_end(args);
        fclose(f);
    }
    last_log_time = now;
}

void log_resp(const char* msg) {
    char path[4096];
    snprintf(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/fuzzpet/last_response.txt", project_root);
    FILE *f = fopen(path, "w");
    if (f) { fprintf(f, "%-57s", msg); fclose(f); }
}

void get_state_fast(const char* piece_id, const char* key, char* out_val) {
    char path[4096], line[4096];
    snprintf(path, sizeof(path), "%s/pieces/world/map_01/%s/state.txt", project_root, piece_id);
    
    if (access(path, F_OK) != 0) {
        if (strcmp(piece_id, "fuzzpet") == 0) 
            snprintf(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/fuzzpet/state.txt", project_root);
        else if (strcmp(piece_id, "manager") == 0) 
            snprintf(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/manager/state.txt", project_root);
        else if (strcmp(piece_id, "selector") == 0)
            snprintf(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/selector/state.txt", project_root);
    }

    FILE *f = fopen(path, "r");
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(line);
                char *v = trim_str(eq + 1);
                if (strcmp(k, key) == 0) {
                    strncpy(out_val, v, 255);
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

int has_method(const char* piece_id, const char* method) {
    char* pm = build_path_malloc("pieces/master_ledger/plugins/+x/piece_manager.+x");
    char val[64] = {0};
    read_exec_output(pm, val, sizeof(val), piece_id, "has-method", method, NULL);
    free(pm);
    return (val[0] == '1');
}

void execute_and_log_response(const char* piece_id, const char* response_key) {
    char* pm = build_path_malloc("pieces/master_ledger/plugins/+x/piece_manager.+x");
    char r[256] = {0};
    read_exec_output(pm, r, sizeof(r), piece_id, "get-response", response_key, NULL);
    free(pm);
    if (r[0] != '\0') {
        r[strcspn(r, "\n\r")] = 0;
        log_resp(trim_str(r));
    }
}

void perform_mirror_sync() {
    char path[4096];
    snprintf(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/fuzzpet/state.txt", project_root);
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
    snprintf(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/manager/state.txt", project_root);
    FILE *f = fopen(path, "w");
    if (f) { fprintf(f, "active_target_id=%s\nlast_key=%s\n", active_target_id, last_key_str); fclose(f); }
}

void trigger_render() {
    char* render = build_path_malloc("pieces/apps/fuzzpet_app/world/plugins/+x/map_render.+x");
    pid_t p = fork();
    if (p == 0) {
        freopen("/dev/null", "w", stdout); freopen("/dev/null", "w", stderr);
        execl(render, render, NULL); exit(1);
    } else if (p > 0) waitpid(p, NULL, 0);
    free(render);
}

void get_method_name(const char* piece_id, int index, char* out_method) {
    char* pm = build_path_malloc("pieces/master_ledger/plugins/+x/piece_manager.+x");
    char list[1024] = {0};
    read_exec_output(pm, list, sizeof(list), piece_id, "list-methods", NULL, NULL);
    free(pm);
    
    char *token = strtok(list, "\n\r");
    int current_idx = 2;
    while (token != NULL) {
        char *m = trim_str(token);
        if (strcmp(m, "move") != 0 && strcmp(m, "select") != 0) {
            if (current_idx == index) {
                strncpy(out_method, m, 63);
                return;
            }
            current_idx++;
        }
        token = strtok(NULL, "\n\r");
    }
    strcpy(out_method, "");
}

void route_input(int key) {
    log_debug("Input Received: %d", key);
    int eff_key = key;
    
    if (key >= 32 && key <= 126) snprintf(last_key_str, sizeof(last_key_str), "%c", (char)key);
    else if (key == 10 || key == 13) strcpy(last_key_str, "ENTER");
    else if (key == 27) strcpy(last_key_str, "ESC");
    else if (key == 1000 || key == 2100) { strcpy(last_key_str, "LEFT"); eff_key = 'a'; }
    else if (key == 1001 || key == 2101) { strcpy(last_key_str, "RIGHT"); eff_key = 'd'; }
    else if (key == 1002 || key == 2102) { strcpy(last_key_str, "UP"); eff_key = 'w'; }
    else if (key == 1003 || key == 2103) { strcpy(last_key_str, "DOWN"); eff_key = 's'; }
    else if (key == 2000) strcpy(last_key_str, "J-BTN 0");
    else if (key == 2008) strcpy(last_key_str, "J-BTN 8");
    else snprintf(last_key_str, sizeof(last_key_str), "CODE %d", key);
    
    if (key == '9' || key == 27 || key == 2008) { 
        strcpy(active_target_id, "selector"); 
        log_resp("Returned to Selector."); 
    }
    
    int method_trigger_key = -1;
    if (key >= '0' && key <= '8') method_trigger_key = key;
    else if (key >= 2000 && key <= 2008) method_trigger_key = '0' + (key - 2000);

    if (strcmp(active_target_id, "selector") == 0 && (key == 10 || key == 13 || key == 2000)) {
        int cx = get_state_int_fast("selector", "pos_x"); 
        int cy = get_state_int_fast("selector", "pos_y");
        
        char world_path[4096];
        snprintf(world_path, sizeof(world_path), "%s/pieces/world/map_01", project_root);
        DIR *dir = opendir(world_path);
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                char *id = entry->d_name;
                if (strcmp(id, ".") == 0 || strcmp(id, "..") == 0 || strcmp(id, "selector") == 0) continue;
                struct stat st;
                char entity_path[8192];
                snprintf(entity_path, sizeof(entity_path), "%s/%s", world_path, id);
                if (stat(entity_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                    int tx = get_state_int_fast(id, "pos_x");
                    int ty = get_state_int_fast(id, "pos_y");
                    int ton = get_state_int_fast(id, "on_map");
                    if (ton == 1 && tx == cx && ty == cy) {
                        strncpy(active_target_id, id, 63);
                        char msg[128]; snprintf(msg, sizeof(msg), "Selected %s.", id); log_resp(msg);
                        break;
                    }
                }
            }
            closedir(dir);
        }
    }
    
    save_manager_state();
    
    if (eff_key == 'w' || eff_key == 'a' || eff_key == 's' || eff_key == 'd') {
        char* trait = build_path_malloc("pieces/apps/fuzzpet_app/traits/+x/move_entity.+x");
        pid_t p = fork();
        if (p == 0) {
            char dir_str[2] = {(char)eff_key, '\0'};
            freopen("/dev/null", "w", stdout); freopen("/dev/null", "w", stderr);
            execl(trait, trait, active_target_id, dir_str, NULL); exit(1);
        } else waitpid(p, NULL, 0);
        free(trait);
        if (strcmp(active_target_id, "selector") != 0) {
            int px = get_state_int_fast(active_target_id, "pos_x"), py = get_state_int_fast(active_target_id, "pos_y");
            char px_s[32], py_s[32]; snprintf(px_s, sizeof(px_s), "%d", px); snprintf(py_s, sizeof(py_s), "%d", py);
            char* pm = build_path_malloc("pieces/master_ledger/plugins/+x/piece_manager.+x");
            p = fork(); if (p == 0) { execl(pm, pm, "selector", "set-state", "pos_x", px_s, NULL); exit(1); } else waitpid(p, NULL, 0);
            p = fork(); if (p == 0) { execl(pm, pm, "selector", "set-state", "pos_y", py_s, NULL); exit(1); } else waitpid(p, NULL, 0);
            free(pm);
        }
    }
    
    if (method_trigger_key >= '2' && method_trigger_key <= '8') {
        char method[64] = {0};
        get_method_name(active_target_id, method_trigger_key - '0', method);
        if (strlen(method) > 0) {
            if (strcmp(method, "scan") == 0 || strcmp(method, "collect") == 0 || 
                strcmp(method, "place") == 0 || strcmp(method, "inventory") == 0) {
                char* s = build_path_malloc("pieces/world/map_01/selector/plugins/+x/storage.+x");
                pid_t p = fork();
                if (p == 0) { execl(s, s, active_target_id, method, NULL); exit(1); }
                else waitpid(p, NULL, 0);
                free(s);
            } else if (strcmp(method, "toggle_emoji") == 0) {
                emoji_mode = !emoji_mode;
                log_resp(emoji_mode ? "Emoji Mode ON" : "Emoji Mode OFF");
            } else {
                // Log response based on method name
                char resp_key[64];
                if (strcmp(method, "feed") == 0) strcpy(resp_key, "fed");
                else if (strcmp(method, "sleep") == 0) strcpy(resp_key, "slept");
                else {
                    strcpy(resp_key, method);
                    int len = (int)strlen(resp_key);
                    if (resp_key[len-1] == 'y') { resp_key[len-1] = 'i'; strcpy(resp_key+len-1, "ied"); }
                    else if (resp_key[len-1] == 'e') strcpy(resp_key+len-1, "ed");
                    else strcpy(resp_key+len, "ed");
                }
                
                execute_and_log_response(active_target_id, resp_key);
            }
        }
    }

    perform_mirror_sync();
    save_manager_state();
    trigger_render();
}

void* input_thread(void* arg) {
    long last_pos = 0; struct stat st;
    if (stat(history_path, &st) == 0) last_pos = st.st_size;
    while (1) {
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
    log_debug("Manager Started. Root: %s", project_root);
    save_manager_state(); perform_mirror_sync(); trigger_render();
    pthread_t t; pthread_create(&t, NULL, input_thread, NULL);
    while (1) { usleep(1000000); }
    return 0;
}
