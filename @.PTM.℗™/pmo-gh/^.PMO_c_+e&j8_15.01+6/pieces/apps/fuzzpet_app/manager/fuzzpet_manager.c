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

// TPM FuzzPet Manager (v4.3.8 - STABLE BUILD)
// Responsibility: Route input, sync mirror, capture responses.

#define MAX_PATH 16384

char active_target_id[64] = "selector";
char last_key_str[64] = "None";
int emoji_mode = 0;

char project_root[MAX_PATH] = ".";
char history_path[MAX_PATH] = "./pieces/apps/fuzzpet_app/fuzzpet/history.txt";

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
        char line[MAX_PATH];
        while (fgets(line, sizeof(line), kvp)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(line);
                char *v = trim_str(eq + 1);
                if (strcmp(k, "project_root") == 0) snprintf(project_root, MAX_PATH, "%s", v);
            }
        }
        fclose(kvp);
    }
}

void log_resp(const char* msg) {
    char *path = malloc(MAX_PATH);
    if (!path) return;
    snprintf(path, MAX_PATH, "./pieces/apps/fuzzpet_app/fuzzpet/last_response.txt");
    FILE *f = fopen(path, "w");
    if (f) { fprintf(f, "%-57s", msg); fclose(f); }
    free(path);
}

void get_state_fast(const char* piece_id, const char* key, char* out_val) {
    char *path = malloc(MAX_PATH);
    if (!path) { strcpy(out_val, "unknown"); return; }
    char line[MAX_PATH];
    snprintf(path, MAX_PATH, "./pieces/world/map_01/%s/state.txt", piece_id);
    if (access(path, F_OK) != 0) {
        if (strcmp(piece_id, "fuzzpet") == 0) snprintf(path, MAX_PATH, "./pieces/apps/fuzzpet_app/fuzzpet/state.txt");
        else if (strcmp(piece_id, "manager") == 0) snprintf(path, MAX_PATH, "./pieces/apps/fuzzpet_app/manager/state.txt");
        else if (strcmp(piece_id, "selector") == 0) snprintf(path, MAX_PATH, "./pieces/apps/fuzzpet_app/selector/state.txt");
    }
    FILE *f = fopen(path, "r");
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                if (strcmp(trim_str(line), key) == 0) {
                    strncpy(out_val, trim_str(eq + 1), 255);
                    fclose(f); free(path); return;
                }
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

void perform_mirror_sync() {
    char *path = malloc(MAX_PATH);
    if (!path) return;
    snprintf(path, MAX_PATH, "./pieces/apps/fuzzpet_app/fuzzpet/state.txt");
    FILE *f = fopen(path, "w");
    if (!f) { free(path); return; }
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
    free(path);
}

void save_manager_state() {
    char *path = malloc(MAX_PATH);
    if (!path) return;
    snprintf(path, MAX_PATH, "./pieces/apps/fuzzpet_app/manager/state.txt");
    FILE *f = fopen(path, "w");
    if (f) { fprintf(f, "active_target_id=%s\nlast_key=%s\n", active_target_id, last_key_str); fclose(f); }
    free(path);
}

void trigger_render() {
    system("'./pieces/apps/fuzzpet_app/world/plugins/+x/map_render.+x' > /dev/null 2>&1");
}

void route_input(int key) {
    int eff_key = key;
    if (key >= 32 && key <= 126) snprintf(last_key_str, sizeof(last_key_str), "%c", (char)key);
    else if (key == 10 || key == 13) strcpy(last_key_str, "ENTER");
    else if (key == 27) strcpy(last_key_str, "ESC");
    else if (key == 1000 || key == 2100) { strcpy(last_key_str, "LEFT"); eff_key = 'a'; }
    else if (key == 1001 || key == 2101) { strcpy(last_key_str, "RIGHT"); eff_key = 'd'; }
    else if (key == 1002 || key == 2102) { strcpy(last_key_str, "UP"); eff_key = 'w'; }
    else if (key == 1003 || key == 2103) { strcpy(last_key_str, "DOWN"); eff_key = 's'; }
    else if (key == 'z' || key == 'Z') strcpy(last_key_str, "z");
    else if (key == 'x' || key == 'X') strcpy(last_key_str, "x");
    else if (key == 2000) strcpy(last_key_str, "J-BTN 0");
    else if (key == 2008) strcpy(last_key_str, "J-BTN 8");
    else snprintf(last_key_str, sizeof(last_key_str), "CODE %d", key);
    
    if (key == '9' || key == 27 || key == 2008) { 
        strcpy(active_target_id, "selector"); 
        log_resp("Returned to Selector."); 
    }
    
    if (strcmp(active_target_id, "selector") == 0 && (key == 10 || key == 13 || key == 2000)) {
        int cx = get_state_int_fast("selector", "pos_x"); 
        int cy = get_state_int_fast("selector", "pos_y");
        int cz = get_state_int_fast("selector", "pos_z");
        if (cz == -1) cz = 0;
        
        DIR *dir = opendir("./pieces/world/map_01");
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                char *id = entry->d_name;
                if (id[0] == '.' || strcmp(id, "selector") == 0) continue;
                int tx = get_state_int_fast(id, "pos_x");
                int ty = get_state_int_fast(id, "pos_y");
                int tz = get_state_int_fast(id, "pos_z");
                if (tz == -1) tz = 0;
                if (get_state_int_fast(id, "on_map") == 1 && tx == cx && ty == cy && tz == cz) {
                    strncpy(active_target_id, id, 63);
                    char msg[128]; snprintf(msg, sizeof(msg), "Selected %s.", id); log_resp(msg);
                    break;
                }
            }
            closedir(dir);
        }
    }
    
    if (eff_key == 'w' || eff_key == 'a' || eff_key == 's' || eff_key == 'd') {
        char *cmd = malloc(MAX_PATH);
        if (cmd) {
            snprintf(cmd, MAX_PATH, "'./pieces/apps/fuzzpet_app/traits/+x/move_entity.+x' %s %c", active_target_id, (char)eff_key);
            system(cmd);
            free(cmd);
        }
    }
    
    if (key == 'z' || key == 'Z' || key == 'x' || key == 'X') {
        int dz = (key == 'x' || key == 'X') ? 1 : -1;
        int cz = get_state_int_fast(active_target_id, "pos_z");
        if (cz == -1) cz = 0;
        int nz = cz + dz;
        if (nz < -1) nz = -1; if (nz > 1) nz = 1;
        
        char *cmd = malloc(MAX_PATH);
        if (cmd) {
            snprintf(cmd, MAX_PATH, "'./pieces/master_ledger/plugins/+x/piece_manager.+x' %s set-state pos_z %d", active_target_id, nz);
            system(cmd);
            free(cmd);
        }
        system("echo 'Z' >> ./pieces/display/frame_changed.txt");
    }
    
    if (key == ' ' || key == 2000) {
        system("'./pieces/apps/fuzzpet_app/world/plugins/+x/use_stairs.+x' > /dev/null 2>&1");
    }
    
    perform_mirror_sync();
    save_manager_state();
    trigger_render();
}

void* input_thread(void* arg) {
    long last_pos = 0; struct stat st;
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
    save_manager_state(); perform_mirror_sync(); trigger_render();
    pthread_t t; pthread_create(&t, NULL, input_thread, NULL);
    while (1) { usleep(1000000); }
    return 0;
}
