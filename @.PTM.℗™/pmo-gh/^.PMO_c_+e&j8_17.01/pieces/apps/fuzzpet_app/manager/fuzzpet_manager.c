#define _GNU_SOURCE
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

// TPM FuzzPet Manager (v4.8.0 - DYNAMIC DISPATCH)
// Responsibility: Route input via PDL DNA and maintain sovereign state.

char active_target_id[64] = "selector";
char last_key_str[64] = "None";
int emoji_mode = 0;
char *project_root = NULL;

char* trim_str(char *str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void log_debug(const char* fmt, ...) {
    if (!project_root) return;
    char *path = NULL; asprintf(&path, "%s/pieces/apps/fuzzpet_app/manager/debug_log.txt", project_root);
    FILE *f = fopen(path, "a");
    if (f) {
        va_list args; va_start(args, fmt);
        time_t now = time(NULL);
        fprintf(f, "[%ld] ", now);
        vfprintf(f, fmt, args);
        fprintf(f, "\n");
        va_end(args);
        fclose(f);
    }
    free(path);
}

void log_resp(const char* msg) {
    if (!project_root) return;
    char *path = NULL; asprintf(&path, "%s/pieces/apps/fuzzpet_app/fuzzpet/last_response.txt", project_root);
    FILE *f = fopen(path, "w");
    if (f) { fprintf(f, "%-57s", msg); fclose(f); }
    free(path);
}

void resolve_paths() {
    const char* attempts[] = {"pieces/locations/location_kvp", "../pieces/locations/location_kvp", "../../pieces/locations/location_kvp"};
    for (int i = 0; i < 3; i++) {
        FILE *kvp = fopen(attempts[i], "r");
        if (kvp) {
            char line[4096];
            while (fgets(line, sizeof(line), kvp)) {
                if (strncmp(line, "project_root=", 13) == 0) {
                    project_root = strdup(trim_str(line + 13));
                    fclose(kvp); return;
                }
            }
            fclose(kvp);
        }
    }
}

void load_manager_state() {
    if (!project_root) return;
    char *path = NULL; asprintf(&path, "%s/pieces/apps/fuzzpet_app/manager/state.txt", project_root);
    FILE *f = fopen(path, "r");
    if (f) {
        char line[4096];
        while (fgets(line, sizeof(line), f)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(line);
                char *v = trim_str(eq + 1);
                if (strcmp(k, "active_target_id") == 0) strncpy(active_target_id, v, 63);
            }
        }
        fclose(f);
    }
    free(path);
}

void save_manager_state() {
    if (!project_root) return;
    char *path = NULL; asprintf(&path, "%s/pieces/apps/fuzzpet_app/manager/state.txt", project_root);
    FILE *f = fopen(path, "w");
    if (f) { fprintf(f, "active_target_id=%s\nlast_key=%s\n", active_target_id, last_key_str); fclose(f); }
    free(path);
}

void get_state_fast(const char* piece_id, const char* key, char* out_val) {
    if (!project_root) { strcpy(out_val, "unknown"); return; }
    char *path = NULL; asprintf(&path, "%s/pieces/world/map_01/%s/state.txt", project_root, piece_id);
    if (access(path, F_OK) != 0) {
        free(path);
        if (strcmp(piece_id, "fuzzpet") == 0) asprintf(&path, "%s/pieces/apps/fuzzpet_app/fuzzpet/state.txt", project_root);
        else if (strcmp(piece_id, "manager") == 0) asprintf(&path, "%s/pieces/apps/fuzzpet_app/manager/state.txt", project_root);
        else if (strcmp(piece_id, "selector") == 0) asprintf(&path, "%s/pieces/apps/fuzzpet_app/selector/state.txt", project_root);
        else asprintf(&path, "state.txt");
    }
    FILE *f = fopen(path, "r");
    if (f) {
        char line[4096];
        while (fgets(line, sizeof(line), f)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                if (strcmp(trim_str(line), key) == 0) { strncpy(out_val, trim_str(eq + 1), 255); fclose(f); free(path); return; }
            }
        }
        fclose(f);
    }
    free(path);
    strcpy(out_val, "unknown");
}

int get_state_int_fast(const char* piece_id, const char* key) {
    char val[256]; get_state_fast(piece_id, key, val);
    if (strcmp(val, "unknown") == 0 || val[0] == '\0') return -1;
    return atoi(val);
}

void set_piece_state_int(const char* piece_id, const char* key, int val) {
    if (!project_root) return;
    char *path = NULL; asprintf(&path, "%s/pieces/world/map_01/%s/state.txt", project_root, piece_id);
    if (access(path, F_OK) != 0) {
        free(path);
        if (strcmp(piece_id, "fuzzpet") == 0) asprintf(&path, "%s/pieces/apps/fuzzpet_app/fuzzpet/state.txt", project_root);
        else if (strcmp(piece_id, "manager") == 0) asprintf(&path, "%s/pieces/apps/fuzzpet_app/manager/state.txt", project_root);
        else if (strcmp(piece_id, "selector") == 0) asprintf(&path, "%s/pieces/apps/fuzzpet_app/selector/state.txt", project_root);
        else asprintf(&path, "state.txt");
    }
    
    // Read all lines, update the key, write back
    FILE *f = fopen(path, "r");
    char lines[50][256];
    int count = 0, found = 0;
    if (f) {
        while (fgets(lines[count], sizeof(lines[count]), f) && count < 50) {
            lines[count][strcspn(lines[count], "\n\r")] = 0;
            if (strncmp(lines[count], key, strlen(key)) == 0 && lines[count][strlen(key)] == '=') {
                snprintf(lines[count], sizeof(lines[count]), "%s=%d", key, val);
                found = 1;
            }
            count++;
        }
        fclose(f);
    }
    if (!found) snprintf(lines[count++], sizeof(lines[count]), "%s=%d", key, val);
    
    f = fopen(path, "w");
    if (f) {
        for (int i = 0; i < count; i++) fprintf(f, "%s\n", lines[i]);
        fclose(f);
    }
    free(path);
}

void perform_mirror_sync() {
    if (!project_root) return;
    char *path = NULL; asprintf(&path, "%s/pieces/apps/fuzzpet_app/fuzzpet/state.txt", project_root);
    FILE *f = fopen(path, "w");
    if (!f) { free(path); return; }
    if (strcmp(active_target_id, "selector") == 0) {
        // Selector has no hunger/happiness/energy - use hardcoded values
        fprintf(f, "name=SELECTOR\nhunger=0\nhappiness=0\nenergy=0\nlevel=0\nstatus=active\nemoji_mode=%d\n", emoji_mode);
    } else {
        char name[256], hunger[64], happiness[64], energy[64], level[64];
        get_state_fast(active_target_id, "name", name);
        get_state_fast(active_target_id, "hunger", hunger);
        get_state_fast(active_target_id, "happiness", happiness);
        get_state_fast(active_target_id, "energy", energy);
        get_state_fast(active_target_id, "level", level);
        fprintf(f, "name=%s\nhunger=%s\nhappiness=%s\nenergy=%s\nlevel=%s\nstatus=active\nemoji_mode=%d\n", 
                name, hunger, happiness, energy, level, emoji_mode);
    }
    fclose(f); free(path);
}

void trigger_render() {
    if (!project_root) return;
    char *cmd = NULL; asprintf(&cmd, "'%s/pieces/apps/fuzzpet_app/world/plugins/+x/map_render.+x' > /dev/null 2>&1", project_root);
    system(cmd); free(cmd);
}

char* get_method_from_pdl(const char* piece_id, int index) {
    if (!project_root) return NULL;
    char *path = NULL; asprintf(&path, "%s/pieces/world/map_01/%s/%s.pdl", project_root, piece_id, piece_id);
    if (access(path, F_OK) != 0) {
        free(path);
        if (strcmp(piece_id, "selector") == 0) asprintf(&path, "%s/pieces/apps/fuzzpet_app/selector/selector.pdl", project_root);
        else if (strcmp(piece_id, "pet_01") == 0 || strcmp(piece_id, "pet_02") == 0) asprintf(&path, "%s/pieces/apps/fuzzpet_app/fuzzpet/fuzzpet.pdl", project_root);
        else return NULL;
    }
    FILE *f = fopen(path, "r");
    if (!f) { free(path); return NULL; }
    char line[1024]; int count = 2;  // Start at 2 to skip move/select (like old get_method_name)
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "METHOD", 6) == 0) {
            char *bar = strchr(line, '|');
            if (bar) {
                char *next_bar = strchr(bar + 1, '|');
                if (next_bar) {
                    *next_bar = '\0';
                    char *method_name = trim_str(bar + 1);
                    // Skip move and select methods (they're not hotkey-triggered)
                    if (strcmp(method_name, "move") == 0 || strcmp(method_name, "select") == 0) {
                        continue;  // Don't increment count, just skip
                    }
                    if (count == index) {
                        free(path); fclose(f); return strdup(method_name);
                    }
                    count++;
                }
            }
        }
    }
    free(path); fclose(f); return NULL;
}

void route_input(int key) {
    load_manager_state();
    char eff_move = 0, eff_z = 0;

    // Normalize
    if (key >= 32 && key <= 126) snprintf(last_key_str, sizeof(last_key_str), "%c", (char)key);
    else if (key == 10 || key == 13) strcpy(last_key_str, "ENTER");
    else if (key == 27) strcpy(last_key_str, "ESC");
    else snprintf(last_key_str, sizeof(last_key_str), "CODE %d", key);

    // 1. Critical release logic
    if (key == '9' || key == 27 || key == 2008) {
        strcpy(active_target_id, "selector");
        save_manager_state(); perform_mirror_sync(); trigger_render(); return;
    }

    // 2. Mapping WASD/Arrows
    if (key == 'w' || key == 'W' || key == 1002 || key == 2102) eff_move = 'w';
    else if (key == 's' || key == 'S' || key == 1003 || key == 2103) eff_move = 's';
    else if (key == 'a' || key == 'A' || key == 1000 || key == 2100) eff_move = 'a';
    else if (key == 'd' || key == 'D' || key == 1001 || key == 2101) eff_move = 'd';
    else if (key == 'z' || key == 'Z' || key == 122) eff_z = 'z';
    else if (key == 'x' || key == 'X' || key == 120) eff_z = 'x';

    // 3. Dynamic Hotkeys (2-8 -> Method indices 2-8, skipping move/select)
    // Works for selector AND pets
    if (key >= '2' && key <= '8') {
        char *method = get_method_from_pdl(active_target_id, key - '0');
        if (method) {
            // Handle different method types appropriately
            if (strcmp(method, "scan") == 0 || strcmp(method, "collect") == 0 || 
                strcmp(method, "place") == 0 || strcmp(method, "inventory") == 0) {
                char *cmd = NULL;
                asprintf(&cmd, "'%s/pieces/apps/fuzzpet_app/traits/+x/storage_trait.+x' %s %s", project_root, active_target_id, method);
                system(cmd); free(cmd);
            } else if (strcmp(method, "toggle_emoji") == 0) {
                emoji_mode = !emoji_mode;
                log_resp(emoji_mode ? "Emoji Mode ON" : "Emoji Mode OFF");
            } else if (strcmp(method, "toggle_clock") == 0) {
                // Toggle clock mode between auto and turn
                char *cmd = NULL;
                asprintf(&cmd, "'%s/pieces/master_ledger/plugins/+x/piece_manager.+x' clock_daemon get-state mode", project_root);
                FILE *fp = popen(cmd, "r");
                char current_mode[64] = "auto";
                if (fp) { fgets(current_mode, sizeof(current_mode), fp); pclose(fp); }
                current_mode[strcspn(current_mode, "\n\r")] = 0;
                free(cmd);
                
                const char *new_mode = (strcmp(current_mode, "auto") == 0) ? "turn" : "auto";
                asprintf(&cmd, "'%s/pieces/master_ledger/plugins/+x/piece_manager.+x' clock_daemon set-state mode %s", project_root, new_mode);
                system(cmd); free(cmd);
                log_resp(new_mode == "auto" ? "Clock: Auto Mode" : "Clock: Turn Mode");
            } else if (strcmp(method, "feed") == 0) {
                int h = get_state_int_fast(active_target_id, "hunger");
                set_piece_state_int(active_target_id, "hunger", (h > 20) ? h - 20 : 0);
                log_resp("Yum yum!");
            } else if (strcmp(method, "play") == 0) {
                int h = get_state_int_fast(active_target_id, "happiness");
                set_piece_state_int(active_target_id, "happiness", (h < 90) ? h + 10 : 100);
                log_resp("Wheee~ Best playtime ever!");
            } else if (strcmp(method, "sleep") == 0) {
                int e = get_state_int_fast(active_target_id, "energy");
                set_piece_state_int(active_target_id, "energy", (e < 70) ? e + 30 : 100);
                log_resp("Zzz...");
            } else {
                // Other methods - use storage_trait as fallback
                char *cmd = NULL;
                asprintf(&cmd, "'%s/pieces/apps/fuzzpet_app/traits/+x/storage_trait.+x' %s %s", project_root, active_target_id, method);
                system(cmd); free(cmd);
            }
            free(method);
        }
    }

    // 4. Selection Logic (only when on same z-level)
    if (strcmp(active_target_id, "selector") == 0 && (key == 10 || key == 13 || key == 2000)) {
        int cx = get_state_int_fast("selector", "pos_x");
        int cy = get_state_int_fast("selector", "pos_y");
        int cz = get_state_int_fast("selector", "pos_z"); if (cz == -1) cz = 0;
        char *world_path = NULL; asprintf(&world_path, "%s/pieces/world/map_01", project_root);
        DIR *dir = opendir(world_path);
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                char *id = entry->d_name;
                // Skip selector, zombie, and hidden entities
                if (id[0] == '.' || strcmp(id, "selector") == 0 || strstr(id, "zombie") != NULL) continue;
                // Only select entities on same z-level and on_map
                int ez = get_state_int_fast(id, "pos_z"); if (ez == -1) ez = 0;
                int on_map = get_state_int_fast(id, "on_map");
                if (ez == cz && on_map == 1 && get_state_int_fast(id, "pos_x") == cx && get_state_int_fast(id, "pos_y") == cy) {
                    strncpy(active_target_id, id, 63); save_manager_state(); break;
                }
            }
            closedir(dir);
        }
        free(world_path);
    }
    
    // 5. Execution Logic
    if (eff_move) {
        char *cmd = NULL; asprintf(&cmd, "'%s/pieces/apps/fuzzpet_app/traits/+x/move_entity.+x' %s %c", project_root, active_target_id, eff_move);
        system(cmd); free(cmd);
        if (strcmp(active_target_id, "selector") != 0) {
            int nx = get_state_int_fast(active_target_id, "pos_x"), ny = get_state_int_fast(active_target_id, "pos_y");
            asprintf(&cmd, "'%s/pieces/master_ledger/plugins/+x/piece_manager.+x' selector set-state pos_x %d", project_root, nx); system(cmd); free(cmd);
            asprintf(&cmd, "'%s/pieces/master_ledger/plugins/+x/piece_manager.+x' selector set-state pos_y %d", project_root, ny); system(cmd); free(cmd);
        }
        // Run AI chase
        asprintf(&cmd, "'%s/pieces/apps/fuzzpet_app/traits/+x/zombie_ai.+x' zombie_01 %s", project_root, active_target_id); system(cmd); free(cmd);
    }

    if (eff_z) {
        int dz = (eff_z == 'x') ? 1 : -1;
        int cz = get_state_int_fast(active_target_id, "pos_z"); if (cz == -1) cz = 0;
        int nz = cz + dz; if (nz < -1) nz = -1; if (nz > 1) nz = 1;
        char *cmd = NULL; asprintf(&cmd, "'%s/pieces/master_ledger/plugins/+x/piece_manager.+x' %s set-state pos_z %d", project_root, active_target_id, nz);
        system(cmd); free(cmd);
        if (strcmp(active_target_id, "selector") != 0) {
            asprintf(&cmd, "'%s/pieces/master_ledger/plugins/+x/piece_manager.+x' selector set-state pos_z %d", project_root, nz);
            system(cmd); free(cmd);
        }
        char *p_path = NULL; asprintf(&p_path, "%s/pieces/display/frame_changed.txt", project_root);
        FILE *pf = fopen(p_path, "a"); if (pf) { fprintf(pf, "Z\n"); fclose(pf); } free(p_path);
    }
    
    perform_mirror_sync(); save_manager_state(); trigger_render();
}

void* input_thread(void* arg) {
    long last_pos = 0; struct stat st;
    char *h_path = NULL; asprintf(&h_path, "%s/pieces/apps/fuzzpet_app/fuzzpet/history.txt", project_root);
    while (1) {
        if (stat(h_path, &st) == 0) {
            if (st.st_size > last_pos) {
                FILE *hf = fopen(h_path, "r");
                if (hf) { fseek(hf, last_pos, SEEK_SET); int key; while (fscanf(hf, "%d", &key) == 1) route_input(key); last_pos = ftell(hf); fclose(hf); }
            } else if (st.st_size < last_pos) last_pos = 0;
        }
        usleep(16667);
    }
    free(h_path); return NULL;
}

int main() {
    resolve_paths(); load_manager_state(); perform_mirror_sync(); trigger_render();
    pthread_t t; pthread_create(&t, NULL, input_thread, NULL);
    while (1) usleep(1000000);
    return 0;
}
