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
#include <stdbool.h>

// TPM Player/Editor Engine (v1.2 - BUFFER FIX)
// Responsibility: Unified Engine for Project Playback and Editing.

#define MAX_PATH 16384
#define MAP_ROWS 10
#define MAP_COLS 20

typedef struct {
    char project_id[256];
    char current_map[256];
    int current_z;
    bool selector_mode;
    char active_piece[64];
} EngineState;

EngineState g_state = { "template", "map_0001", 0, true, "selector" };
char project_root[MAX_PATH] = ".";

// Forward declaration
void log_master(const char* type, const char* event, const char* piece, const char* source);

// --- Utility Functions ---

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
        char line[MAX_PATH];
        while (fgets(line, sizeof(line), kvp)) {
            if (strncmp(line, "project_root=", 13) == 0) {
                char *v = line + 13;
                v[strcspn(v, "\n\r")] = 0;
                snprintf(project_root, MAX_PATH, "%s", v);
                break;
            }
        }
        fclose(kvp);
    }
}

// --- TPM Piece Interaction ---

int get_piece_int(const char* piece_id, const char* key) {
    char *cmd = malloc(MAX_PATH * 2);
    if (!cmd) return -1;
    snprintf(cmd, MAX_PATH * 2, "'./pieces/master_ledger/plugins/+x/piece_manager.+x' %s get-state %s", piece_id, key);
    FILE *fp = popen(cmd, "r");
    free(cmd);
    if (!fp) return -1;
    char val[128];
    int res = -1;
    if (fgets(val, sizeof(val), fp)) {
        if (strstr(val, "STATE_NOT_FOUND") == NULL) res = atoi(val);
    }
    pclose(fp);
    return res;
}

void set_piece_int(const char* piece_id, const char* key, int val) {
    char *cmd = malloc(MAX_PATH * 2);
    if (!cmd) return;
    snprintf(cmd, MAX_PATH * 2, "'./pieces/master_ledger/plugins/+x/piece_manager.+x' %s set-state %s %d > /dev/null 2>&1", piece_id, key, val);
    system(cmd);
    free(cmd);
}

// --- Engine Core ---

void sync_state() {
    char *path = malloc(MAX_PATH);
    if (!path) return;
    snprintf(path, MAX_PATH, "./pieces/apps/player_app/manager/state.txt");
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "project_id=%s\n", g_state.project_id);
        fprintf(f, "current_map=%s\n", g_state.current_map);
        fprintf(f, "current_z=%d\n", g_state.current_z);
        fprintf(f, "selector_mode=%d\n", g_state.selector_mode ? 1 : 0);
        fprintf(f, "active_piece=%s\n", g_state.active_piece);
        fclose(f);
    }
    free(path);
}

void trigger_render() {
    sync_state();
    system("echo 'R' >> ./pieces/display/frame_changed.txt");
}

void run_op(const char* op_name, const char* arg1, const char* arg2) {
    char *cmd = malloc(MAX_PATH * 2);
    if (!cmd) return;
    snprintf(cmd, MAX_PATH * 2, "'./pieces/apps/player_app/world/plugins/+x/%s.+x' %s %s > /dev/null 2>&1", 
             op_name, arg1 ? arg1 : "", arg2 ? arg2 : "");
    system(cmd);
    free(cmd);
}

void process_input(int key) {
    if (key == '9' || key == 2008) {
        g_state.selector_mode = !g_state.selector_mode;
        strncpy(g_state.active_piece, g_state.selector_mode ? "selector" : "player", 63);
        log_master("EventFire", "toggle_selector", g_state.active_piece, "player_app");
    }

    // Standard Movement Ops
    else if (key == 'w' || key == 1002 || key == 2102) run_op("move_entity", g_state.active_piece, "w");
    else if (key == 's' || key == 1003 || key == 2103) run_op("move_entity", g_state.active_piece, "s");
    else if (key == 'a' || key == 1000 || key == 2100) run_op("move_entity", g_state.active_piece, "a");
    else if (key == 'd' || key == 1001 || key == 2101) run_op("move_entity", g_state.active_piece, "d");
    
    // Z-Level Ops (Standardized)
    else if (key == 'z' || key == 'Z') run_op("move_z", g_state.active_piece, "-1");
    else if (key == 'x' || key == 'X') run_op("move_z", g_state.active_piece, "1");

    // Interaction Op (Check for piece scripts)
    else if (key == ' ' || key == 10 || key == 13 || key == 2000) {
        run_op("interact", g_state.active_piece, "");
    } else {
        return; // Don't increment turn for non-actions
    }

    // Increment Turn after any action
    system("'./pieces/apps/fuzzpet_app/clock/plugins/+x/increment_turn.+x' > /dev/null 2>&1");

    trigger_render();
}

void log_master(const char* type, const char* event, const char* piece, const char* source) {
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[100];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    FILE *ledger = fopen("./pieces/master_ledger/master_ledger.txt", "a");
    if (ledger) {
        fprintf(ledger, "[%s] %s: %s on %s | Source: %s\n", timestamp, type, event, piece, source);
        fclose(ledger);
    }
}

void* input_loop(void* arg) {
    const char* history_path = "./pieces/keyboard/history.txt";
    long last_pos = 0;
    
    while(1) {
        FILE *f = fopen(history_path, "r");
        if (f) {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            if (size > last_pos) {
                fseek(f, last_pos, SEEK_SET);
                int key;
                while (fscanf(f, "%d", &key) == 1) {
                    process_input(key);
                }
                last_pos = ftell(f);
            }
            fclose(f);
        }
        usleep(16667);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    printf("TPM Player Engine Starting...\n");

    pthread_t thread;
    pthread_create(&thread, NULL, input_loop, NULL);
    
    while(1) {
        usleep(1000000);
    }
    
    return 0;
}
