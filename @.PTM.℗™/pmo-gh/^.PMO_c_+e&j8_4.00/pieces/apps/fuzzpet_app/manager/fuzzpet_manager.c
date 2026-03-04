#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <ctype.h>

// TPM FuzzPet Manager (v3.6 - Robust State Access)
// Responsibility: Route input, sync mirror, capture responses.

char active_target_id[64] = "selector";
char last_key_str[64] = "None";
int emoji_mode = 0;

char* trim_str(char *str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void log_resp(const char* msg) {
    FILE *f = fopen("pieces/apps/fuzzpet_app/fuzzpet/last_response.txt", "w");
    if (f) { fprintf(f, "%-57s", msg); fclose(f); }
}

void get_state_fast(const char* piece_id, const char* key, char* out_val) {
    char path[512], line[1024];
    if (strcmp(piece_id, "selector") == 0) strcpy(path, "pieces/apps/fuzzpet_app/selector/state.txt");
    else snprintf(path, 512, "pieces/entities/%s/state.txt", piece_id);
    
    FILE *f = fopen(path, "r");
    if (f) {
        while (fgets(line, 1024, f)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(line);
                char *v = trim_str(eq + 1);
                if (strcmp(k, key) == 0) {
                    strcpy(out_val, v);
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
    char cmd[512], val[64];
    snprintf(cmd, sizeof(cmd), "./pieces/master_ledger/plugins/+x/piece_manager.+x %s has-method %s", piece_id, method);
    FILE *fp = popen(cmd, "r");
    if (fp) { if (fgets(val, sizeof(val), fp)) { pclose(fp); return (val[0] == '1'); } pclose(fp); }
    return 0;
}

void execute_and_log_response(const char* piece_id, const char* response_key) {
    char cmd[512], r[256];
    snprintf(cmd, sizeof(cmd), "./pieces/master_ledger/plugins/+x/piece_manager.+x %s get-response %s", piece_id, response_key);
    FILE *p = popen(cmd, "r");
    if (p) {
        if (fgets(r, 256, p)) { r[strcspn(r, "\n\r")] = 0; log_resp(trim_str(r)); }
        pclose(p);
    }
}

void perform_mirror_sync() {
    FILE *f = fopen("pieces/apps/fuzzpet_app/fuzzpet/state.txt", "w");
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
    FILE *f = fopen("pieces/apps/fuzzpet_app/manager/state.txt", "w");
    if (f) { fprintf(f, "active_target_id=%s\nlast_key=%s\n", active_target_id, last_key_str); fclose(f); }
}

void route_input(int key) {
    char cmd[512]; int eff_key = key;
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
    else if (strcmp(active_target_id, "selector") == 0 && (key == 10 || key == 13 || key == 2000)) {
        int cx = get_state_int_fast("selector", "pos_x"); 
        int cy = get_state_int_fast("selector", "pos_y");
        FILE *reg = fopen("pieces/apps/fuzzpet_app/world/registry.txt", "r");
        if (reg) {
            char tid[64];
            while (fgets(tid, sizeof(tid), reg)) {
                char *id = trim_str(tid);
                if (strcmp(id, "selector") == 0 || strlen(id) == 0) continue;
                
                int tx = get_state_int_fast(id, "pos_x");
                int ty = get_state_int_fast(id, "pos_y");
                int ton = get_state_int_fast(id, "on_map");
                
                if (ton == 1 && tx == cx && ty == cy) {
                    strcpy(active_target_id, id);
                    char msg[128]; snprintf(msg, sizeof(msg), "Selected %s.", id); log_resp(msg);
                    break;
                }
            }
            fclose(reg);
        }
    }
    
    save_manager_state();
    
    if (eff_key == 'w' || eff_key == 'a' || eff_key == 's' || eff_key == 'd') {
        snprintf(cmd, sizeof(cmd), "./pieces/apps/fuzzpet_app/traits/+x/move_entity.+x %s %c > /dev/null 2>&1", active_target_id, (char)eff_key);
        system(cmd);
        if (strcmp(active_target_id, "selector") != 0) {
            int px = get_state_int_fast(active_target_id, "pos_x"), py = get_state_int_fast(active_target_id, "pos_y");
            char sync[512]; 
            snprintf(sync, 512, "./pieces/master_ledger/plugins/+x/piece_manager.+x selector set-state pos_x %d > /dev/null 2>&1", px); system(sync);
            snprintf(sync, 512, "./pieces/master_ledger/plugins/+x/piece_manager.+x selector set-state pos_y %d > /dev/null 2>&1", py); system(sync);
        }
    }
    
    if (strcmp(active_target_id, "selector") == 0) {
        if (key == '2') system("./pieces/apps/fuzzpet_app/selector/plugins/+x/storage.+x selector scan");
        else if (key == '3') system("./pieces/apps/fuzzpet_app/selector/plugins/+x/storage.+x selector collect");
        else if (key == '4') system("./pieces/apps/fuzzpet_app/selector/plugins/+x/storage.+x selector place");
        else if (key == '5') system("./pieces/apps/fuzzpet_app/selector/plugins/+x/storage.+x selector inventory");
        else if (key == '6') { emoji_mode = !emoji_mode; log_resp(emoji_mode ? "Emoji Mode ON" : "Emoji Mode OFF"); }
    } else {
        if (key == '2' && has_method(active_target_id, "feed")) execute_and_log_response(active_target_id, "fed");
        else if (key == '3' && has_method(active_target_id, "play")) execute_and_log_response(active_target_id, "played");
        else if (key == '4' && has_method(active_target_id, "sleep")) execute_and_log_response(active_target_id, "slept");
        else if (key == '5' && has_method(active_target_id, "collect")) { snprintf(cmd, 512, "./pieces/apps/fuzzpet_app/selector/plugins/+x/storage.+x %s collect", active_target_id); system(cmd); }
        else if (key == '6' && has_method(active_target_id, "place")) { snprintf(cmd, 512, "./pieces/apps/fuzzpet_app/selector/plugins/+x/storage.+x %s place", active_target_id); system(cmd); }
        else if (key == '7') { emoji_mode = !emoji_mode; log_resp(emoji_mode ? "Emoji Mode ON" : "Emoji Mode OFF"); }
    }

    perform_mirror_sync();
    save_manager_state();
    system("./pieces/apps/fuzzpet_app/world/plugins/+x/map_render.+x > /dev/null 2>&1");
}

void* input_thread(void* arg) {
    long last_pos = 0; const char* history_path = "pieces/apps/fuzzpet_app/fuzzpet/history.txt";
    FILE *h_init = fopen(history_path, "r"); if (h_init) { fseek(h_init, 0, SEEK_END); last_pos = ftell(h_init); fclose(h_init); }
    while (1) {
        FILE *hf = fopen(history_path, "r");
        if (hf) { struct stat st; if (fstat(fileno(hf), &st) == 0 && st.st_size > last_pos) {
            fseek(hf, last_pos, SEEK_SET); int key; while (fscanf(hf, "%d", &key) == 1) { route_input(key); } last_pos = ftell(hf);
        } fclose(hf); }
        usleep(16667); 
    }
    return NULL;
}

int main() {
    save_manager_state(); perform_mirror_sync();
    system("./pieces/apps/fuzzpet_app/world/plugins/+x/map_render.+x > /dev/null 2>&1");
    pthread_t t; pthread_create(&t, NULL, input_thread, NULL);
    while (1) { usleep(100000); } return 0;
}
