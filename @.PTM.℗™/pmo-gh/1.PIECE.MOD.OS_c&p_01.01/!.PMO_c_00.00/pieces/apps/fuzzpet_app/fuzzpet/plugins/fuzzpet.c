#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_LINE 256

// Standardized Key Codes
#define K_LEFT  1000
#define K_RIGHT 1001
#define K_UP    1002
#define K_DOWN  1003

// Map dimensions
#define MAP_ROWS 10
#define MAP_COLS 20

char map[MAP_ROWS][MAP_COLS + 1];
char last_key_display[64] = "None";
char current_response_key[64] = "default";

// --- TPM Helpers ---

int get_piece_state_int(const char* piece, const char* key) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./pieces/master_ledger/plugins/+x/piece_manager.+x %s get-state %s", piece, key);
    FILE *fp = popen(cmd, "r");
    if (!fp) return 0;
    char val[64];
    int result = 0;
    if (fgets(val, sizeof(val), fp)) {
        if (strcmp(val, "STATE_NOT_FOUND") != 0) result = atoi(val);
    }
    pclose(fp);
    return result;
}

void set_piece_state_int(const char* piece, const char* key, int val) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./pieces/master_ledger/plugins/+x/piece_manager.+x %s set-state %s %d > /dev/null 2>&1", piece, key, val);
    system(cmd);
}

void log_master(const char* type, const char* event, const char* piece, const char* source) {
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[100];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    FILE *ledger = fopen("pieces/master_ledger/master_ledger.txt", "a");
    if (ledger) {
        fprintf(ledger, "[%s] %s: %s on %s | Source: %s\n", timestamp, type, event, piece, source);
        fclose(ledger);
    }
}

// --- FuzzPet Logic ---

void sync_mirror() {
    int hunger = get_piece_state_int("fuzzpet", "hunger");
    int happiness = get_piece_state_int("fuzzpet", "happiness");
    int energy = get_piece_state_int("fuzzpet", "energy");
    int level = get_piece_state_int("fuzzpet", "level");
    int pos_x = get_piece_state_int("fuzzpet", "pos_x");
    int pos_y = get_piece_state_int("fuzzpet", "pos_y");

    FILE *f = fopen("pieces/apps/fuzzpet_app/fuzzpet/state.txt", "w");
    if (f) {
        fprintf(f, "hunger=%d\nhappiness=%d\nenergy=%d\nlevel=%d\npos_x=%d\npos_y=%d\nstatus=active\n",
                hunger, happiness, energy, level, pos_x, pos_y);
        fclose(f);
    }
}

void write_sovereign_view() {
    // 1. Sync the mirror first so others see the data (Parser relies on this!)
    sync_mirror();

    // 2. Get Last Response (Audit Reflection)
    char response[256] = "";
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./pieces/master_ledger/plugins/+x/piece_manager.+x fuzzpet get-response %s", current_response_key);
    FILE *rf = popen(cmd, "r");
    if (rf) {
        fgets(response, sizeof(response), rf);
        response[strcspn(response, "\n")] = 0;
        pclose(rf);
    }

    // 3. Compose Sovereign View (The "Stage")
    FILE *fp = fopen("pieces/apps/fuzzpet_app/fuzzpet/view.txt", "w");
    if (!fp) return;

    for (int y = 0; y < MAP_ROWS; y++) {
        fprintf(fp, "║  %-20s                                     ║\n", map[y]);
    }
    fprintf(fp, "╠═══════════════════════════════════════════════════════════╣\n");
    fprintf(fp, "║  [FUZZPET]: %-45s ║\n", response);
    fprintf(fp, "║  DEBUG [Last Key]: %-38s ║\n", last_key_display);
    
    fclose(fp);
    
    // SIGNALING NODE: Append to local marker to wake up the Parser (Option Z)
    FILE *marker = fopen("pieces/apps/fuzzpet_app/fuzzpet/view_changed.txt", "a");
    if (marker) {
        fputc('X', marker);
        fputc('\n', marker);
        fclose(marker);
    }
}

void save_map() {
    FILE *f = fopen("pieces/apps/fuzzpet_app/world/map.txt", "w");
    if (f) {
        for (int y = 0; y < MAP_ROWS; y++) {
            fprintf(f, "%.*s\n", MAP_COLS, map[y]);
        }
        fclose(f);
    }
}

void load_map() {
    FILE *f = fopen("pieces/apps/fuzzpet_app/world/map.txt", "r");
    if (f) {
        for (int y = 0; y < MAP_ROWS; y++) {
            if (fgets(map[y], MAP_COLS + 2, f)) { 
                map[y][strcspn(map[y], "\n\r")] = 0; 
            } else {
                map[y][0] = '\0'; 
            }
        }
        fclose(f);
    } else {
        for (int y = 0; y < MAP_ROWS; y++) {
            for (int x = 0; x < MAP_COLS; x++) map[y][x] = '#'; 
            map[y][MAP_COLS] = 0; 
        }
        save_map();
    }

    int initial_pos_x = get_piece_state_int("fuzzpet", "pos_x");
    int initial_pos_y = get_piece_state_int("fuzzpet", "pos_y");

    for (int y = 0; y < MAP_ROWS; y++) {
        for (int x = 0; x < MAP_COLS; x++) {
            if (map[y][x] == '@') map[y][x] = '.';
        }
    }
    if (initial_pos_x >= 0 && initial_pos_x < MAP_COLS &&
        initial_pos_y >= 0 && initial_pos_y < MAP_ROWS) {
        map[initial_pos_y][initial_pos_x] = '@';
    }
    save_map(); 
}

void move(char d) {
    int pos_x = get_piece_state_int("fuzzpet", "pos_x");
    int pos_y = get_piece_state_int("fuzzpet", "pos_y");
    int nx = pos_x, ny = pos_y;
    
    if (d == 'w') ny--; else if (d == 's') ny++; else if (d == 'a') nx--; else if (d == 'd') nx++;
    
    if (nx < 0 || nx >= MAP_COLS || ny < 0 || ny >= MAP_ROWS) return;
    if (map[ny][nx] == '#' || map[ny][nx] == 'R') return;
    
    if (map[ny][nx] == 'T') {
        int hap = get_piece_state_int("fuzzpet", "happiness");
        set_piece_state_int("fuzzpet", "happiness", (hap < 90) ? hap + 10 : 100);
        log_master("EventFire", "treasure_found", "fuzzpet", "fuzzpet_app");
    }
    
    map[pos_y][pos_x] = '.';
    pos_x = nx; pos_y = ny;
    map[pos_y][pos_x] = '@';
    
    set_piece_state_int("fuzzpet", "pos_x", pos_x);
    set_piece_state_int("fuzzpet", "pos_y", pos_y);
    
    int hunger = get_piece_state_int("fuzzpet", "hunger");
    int energy = get_piece_state_int("fuzzpet", "energy");
    set_piece_state_int("fuzzpet", "hunger", (hunger < 100) ? hunger + 1 : 100);
    set_piece_state_int("fuzzpet", "energy", (energy > 0) ? energy - 1 : 0);
    
    system("./pieces/apps/fuzzpet_app/clock/plugins/+x/increment_time.+x > /dev/null 2>&1");
    system("./pieces/apps/fuzzpet_app/clock/plugins/+x/increment_turn.+x > /dev/null 2>&1");
    
    strcpy(current_response_key, "moved");
    save_map();
    log_master("EventFire", "moved", "fuzzpet", "fuzzpet_app");
}

void process_key(int key) {
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[100];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    if (key >= 32 && key <= 126) snprintf(last_key_display, sizeof(last_key_display), "%d ('%c')", key, (char)key);
    else if (key == 10 || key == 13) strcpy(last_key_display, "ENTER");
    else if (key == 27) strcpy(last_key_display, "ESC");
    else snprintf(last_key_display, sizeof(last_key_display), "CODE %d", key);

    if (key == 'w' || key == 'W' || key == K_UP) move('w');
    else if (key == 's' || key == 'S' || key == K_DOWN) move('s');
    else if (key == 'a' || key == 'A' || key == K_LEFT) move('a');
    else if (key == 'd' || key == 'D' || key == K_RIGHT) move('d');
    else if (key == '2') { // Feed Pet (Aligned to UI Index 2)
        int h = get_piece_state_int("fuzzpet", "hunger");
        set_piece_state_int("fuzzpet", "hunger", (h > 20) ? h - 20 : 0);
        log_master("ResponseRequest", "fed", "fuzzpet", "fuzzpet_app");
        strcpy(current_response_key, "fed");
    }
    else if (key == '3') { // Play (Aligned to UI Index 3)
        int h = get_piece_state_int("fuzzpet", "happiness");
        set_piece_state_int("fuzzpet", "happiness", (h < 90) ? h + 10 : 100);
        log_master("ResponseRequest", "played", "fuzzpet", "fuzzpet_app");
        strcpy(current_response_key, "played");
    }
    else if (key == '4') { // Sleep (Aligned to UI Index 4)
        int e = get_piece_state_int("fuzzpet", "energy");
        set_piece_state_int("fuzzpet", "energy", (e < 70) ? e + 30 : 100);
        log_master("ResponseRequest", "slept", "fuzzpet", "fuzzpet_app");
        strcpy(current_response_key, "slept");
    }
    else if (key == '5') { 
        int l = get_piece_state_int("fuzzpet", "level");
        set_piece_state_int("fuzzpet", "level", l + 1);
        log_master("ResponseRequest", "evolved", "fuzzpet", "fuzzpet_app");
        strcpy(current_response_key, "evolved");
    }
    else if (key == '6') { 
        system("./pieces/apps/fuzzpet_app/clock/plugins/+x/end_turn.+x > /dev/null 2>&1");
        log_master("EventFire", "turn_end", "clock", "fuzzpet_app");
        strcpy(current_response_key, "default");
    }
    else {
        strcpy(current_response_key, "default");
    }

    write_sovereign_view();
}

void* input_thread(void* arg) {
    long last_pos = 0;
    FILE *h_init = fopen("pieces/apps/fuzzpet_app/fuzzpet/history.txt", "r");
    if (h_init) {
        fseek(h_init, 0, SEEK_END);
        last_pos = ftell(h_init);
        fclose(h_init);
    }
    
    while (1) {
        FILE *hf = fopen("pieces/apps/fuzzpet_app/fuzzpet/history.txt", "r");
        if (hf) {
            struct stat st;
            if (fstat(fileno(hf), &st) == 0) {
                if (st.st_size > last_pos) {
                    fseek(hf, last_pos, SEEK_SET);
                    int key;
                    while (fscanf(hf, "%d", &key) == 1) {
                        process_key(key);
                    }
                    last_pos = ftell(hf);
                }
            }
            fclose(hf);
        }
        usleep(16667); 
    }
    return NULL;
}

int main() {
    load_map();
    write_sovereign_view(); // HEARTBEAT: Initial frame write
    pthread_t t;
    pthread_create(&t, NULL, input_thread, NULL);
    while (1) {
        usleep(16667); 
    }
    return 0;
}