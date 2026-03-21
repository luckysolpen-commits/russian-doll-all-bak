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

// Joystick 2000-range
#define J_LF 2100
#define J_RT 2101
#define J_UP 2102
#define J_DN 2103

#define J_B0 2000
#define J_B1 2001
#define J_B2 2002
#define J_B3 2003
#define J_B4 2004
#define J_B8 2008  // Selector toggle button

// Map dimensions
#define MAP_ROWS 10
#define MAP_COLS 20

char map[MAP_ROWS][MAP_COLS + 1];
char last_key_display[64] = "None";
char current_response_key[64] = "default";

// Selector System State
int selector_active = 0;           // 0 = player mode, 1 = selector mode
int selector_x = 5;                // Selector cursor X position
int selector_y = 5;                // Selector cursor Y position
int saved_player_x = 5;            // Saved player position when selector activated
int saved_player_y = 5;            // Saved player position when selector activated
char selected_info[128] = "";      // Info about selected tile/entity

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

// Emoji mode helper - returns emoji string or NULL for ASCII
const char* get_emoji_for_char(char c, int emoji_mode) {
    if (!emoji_mode) return NULL;  // ASCII mode
    
    switch(c) {
        case '#': return "⬜";
        case '.': return "🟫";
        case '@': return "🏃";
        case 'R': return "🪨";
        case 'T': return "🧭";
        default: return NULL;
    }
}

void sync_mirror() {
    int hunger = get_piece_state_int("fuzzpet", "hunger");
    int happiness = get_piece_state_int("fuzzpet", "happiness");
    int energy = get_piece_state_int("fuzzpet", "energy");
    int level = get_piece_state_int("fuzzpet", "level");
    int pos_x = get_piece_state_int("fuzzpet", "pos_x");
    int pos_y = get_piece_state_int("fuzzpet", "pos_y");
    int emoji_mode = get_piece_state_int("fuzzpet", "emoji_mode");

    FILE *f = fopen("pieces/apps/fuzzpet_app/fuzzpet/state.txt", "w");
    if (f) {
        fprintf(f, "hunger=%d\nhappiness=%d\nenergy=%d\nlevel=%d\npos_x=%d\npos_y=%d\nstatus=active\nemoji_mode=%d\n",
                hunger, happiness, energy, level, pos_x, pos_y, emoji_mode);
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
    int emoji_mode = get_piece_state_int("fuzzpet", "emoji_mode");
    FILE *fp = fopen("pieces/apps/fuzzpet_app/fuzzpet/view.txt", "w");
    if (!fp) return;

    // Create a temporary map buffer for rendering (with selector cursor overlay)
    char render_map[MAP_ROWS][MAP_COLS + 1];
    for (int y = 0; y < MAP_ROWS; y++) {
        strncpy(render_map[y], map[y], MAP_COLS);
        render_map[y][MAP_COLS] = '\0';
    }
    
    // Overlay selector cursor if active
    if (selector_active && selector_x >= 0 && selector_x < MAP_COLS && selector_y >= 0 && selector_y < MAP_ROWS) {
        render_map[selector_y][selector_x] = 'X';  // Selector cursor marker
    }

    if (emoji_mode) {
        // Emoji mode: use compact format (emoji are wider)
        for (int y = 0; y < MAP_ROWS; y++) {
            fprintf(fp, "║");
            for (int x = 0; x < MAP_COLS; x++) {
                char c = render_map[y][x];
                const char* emoji = get_emoji_for_char(c, emoji_mode);
                if (emoji) {
                    fprintf(fp, "%s", emoji);
                } else {
                    fprintf(fp, "%c", c);
                }
            }
            fprintf(fp, "║\n");
        }
        fprintf(fp, "╠═══════════════════════════════════════════════════════════╣\n");
        fprintf(fp, "║ [FUZZPET]: %-48s ║\n", response);
        fprintf(fp, "║ [SELECTOR]: %-50s ║\n", selector_active ? selected_info : "Press 9 or J-B8 to toggle");
    } else {
        // ASCII mode: original format
        for (int y = 0; y < MAP_ROWS; y++) {
            fprintf(fp, "║  %-20s                                     ║\n", render_map[y]);
        }
        fprintf(fp, "╠═══════════════════════════════════════════════════════════╣\n");
        fprintf(fp, "║  [FUZZPET]: %-45s ║\n", response);
        fprintf(fp, "║  [SELECTOR]: %-43s ║\n", selector_active ? selected_info : "Press 9 or J-B8 to toggle");
    }
    
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
    
    // Initialize selector position to player start position
    selector_x = initial_pos_x;
    selector_y = initial_pos_y;
    saved_player_x = initial_pos_x;
    saved_player_y = initial_pos_y;
    
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

// --- Selector System Functions ---

void toggle_selector() {
    int pos_x = get_piece_state_int("fuzzpet", "pos_x");
    int pos_y = get_piece_state_int("fuzzpet", "pos_y");
    
    if (!selector_active) {
        // Activate selector mode
        selector_active = 1;
        selector_x = pos_x;
        selector_y = pos_y;
        saved_player_x = pos_x;
        saved_player_y = pos_y;
        snprintf(selected_info, sizeof(selected_info), "Selector activated at (%d,%d)", selector_x, selector_y);
        log_master("EventFire", "selector_activated", "fuzzpet", "fuzzpet_app");
    } else {
        // Deactivate selector mode
        selector_active = 0;
        snprintf(selected_info, sizeof(selected_info), "Selector deactivated");
        log_master("EventFire", "selector_deactivated", "fuzzpet", "fuzzpet_app");
    }
}

void move_selector(int dx, int dy) {
    if (!selector_active) return;
    
    int new_x = selector_x + dx;
    int new_y = selector_y + dy;
    
    // Clamp to map bounds
    if (new_x < 0) new_x = 0;
    if (new_x >= MAP_COLS) new_x = MAP_COLS - 1;
    if (new_y < 0) new_y = 0;
    if (new_y >= MAP_ROWS) new_y = MAP_ROWS - 1;
    
    selector_x = new_x;
    selector_y = new_y;
    
    // Get info about selected tile
    char tile_char = map[selector_y][selector_x];
    const char* tile_desc = "unknown";
    switch(tile_char) {
        case '#': tile_desc = "wall"; break;
        case '.': tile_desc = "floor"; break;
        case '@': tile_desc = "player"; break;
        case 'R': tile_desc = "rock"; break;
        case 'T': tile_desc = "treasure"; break;
        case 'Z': tile_desc = "zombie"; break;
        case 'S': tile_desc = "sheep"; break;
        default: tile_desc = "terrain"; break;
    }
    snprintf(selected_info, sizeof(selected_info), "Selected: %s at (%d,%d)", tile_desc, selector_x, selector_y);
}

void selector_inspect() {
    if (!selector_active) return;
    
    char tile_char = map[selector_y][selector_x];
    
    // Check if player is at this position
    int pos_x = get_piece_state_int("fuzzpet", "pos_x");
    int pos_y = get_piece_state_int("fuzzpet", "pos_y");
    
    if (selector_x == pos_x && selector_y == pos_y) {
        int hunger = get_piece_state_int("fuzzpet", "hunger");
        int happiness = get_piece_state_int("fuzzpet", "happiness");
        int energy = get_piece_state_int("fuzzpet", "energy");
        snprintf(selected_info, sizeof(selected_info), "Player at (%d,%d) - HGR:%d HAP:%d ENR:%d", 
                 selector_x, selector_y, hunger, happiness, energy);
    } else {
        const char* tile_desc = "unknown";
        switch(tile_char) {
            case '#': tile_desc = "Wall (impassable)"; break;
            case '.': tile_desc = "Floor (walkable)"; break;
            case '@': tile_desc = "Player character"; break;
            case 'R': tile_desc = "Rock (impassable)"; break;
            case 'T': tile_desc = "Treasure (collectible)"; break;
            case 'Z': tile_desc = "Zombie (enemy)"; break;
            case 'S': tile_desc = "Sheep (neutral)"; break;
            default: tile_desc = "Terrain"; break;
        }
        snprintf(selected_info, sizeof(selected_info), "%s at (%d,%d)", tile_desc, selector_x, selector_y);
    }
    
    log_master("EventFire", "selector_inspect", "fuzzpet", "fuzzpet_app");
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
    else if (key >= 2100 && key <= 2103) {
        const char* j_dirs[] = {"J-LEFT", "J-RIGHT", "J-UP", "J-DOWN"};
        strcpy(last_key_display, j_dirs[key-2100]);
    }
    else if (key >= 2000 && key < 2100) snprintf(last_key_display, sizeof(last_key_display), "J-BUTTON %d", key-2000);
    else snprintf(last_key_display, sizeof(last_key_display), "CODE %d", key);

    // Handle selector mode input
    if (selector_active) {
        if (key == '9' || key == J_B8) {
            toggle_selector();
        } else if (key == 'w' || key == 'W' || key == K_UP || key == J_UP) {
            move_selector(0, -1);
        } else if (key == 's' || key == 'S' || key == K_DOWN || key == J_DN) {
            move_selector(0, 1);
        } else if (key == 'a' || key == 'A' || key == K_LEFT || key == J_LF) {
            move_selector(-1, 0);
        } else if (key == 'd' || key == 'D' || key == K_RIGHT || key == J_RT) {
            move_selector(1, 0);
        } else if (key == 10 || key == 13 || key == J_B0) {
            selector_inspect();
        }
        // In selector mode, only selector commands are processed (input captured)
    } else {
        // Player mode - normal controls
        if (key == '9' || key == J_B8) {
            toggle_selector();
        } else if (key == 'w' || key == 'W' || key == K_UP || key == J_UP) {
            move('w');
        } else if (key == 's' || key == 'S' || key == K_DOWN || key == J_DN) {
            move('s');
        } else if (key == 'a' || key == 'A' || key == K_LEFT || key == J_LF) {
            move('a');
        } else if (key == 'd' || key == 'D' || key == K_RIGHT || key == J_RT) {
            move('d');
        } else if (key == '2' || key == J_B1) { // Feed Pet
            int h = get_piece_state_int("fuzzpet", "hunger");
            set_piece_state_int("fuzzpet", "hunger", (h > 20) ? h - 20 : 0);
            log_master("ResponseRequest", "fed", "fuzzpet", "fuzzpet_app");
            strcpy(current_response_key, "fed");
        }
        else if (key == '3' || key == J_B2) { // Play
            int h = get_piece_state_int("fuzzpet", "happiness");
            set_piece_state_int("fuzzpet", "happiness", (h < 90) ? h + 10 : 100);
            log_master("ResponseRequest", "played", "fuzzpet", "fuzzpet_app");
            strcpy(current_response_key, "played");
        }
        else if (key == '4' || key == J_B3) { // Sleep
            int e = get_piece_state_int("fuzzpet", "energy");
            set_piece_state_int("fuzzpet", "energy", (e < 70) ? e + 30 : 100);
            log_master("ResponseRequest", "slept", "fuzzpet", "fuzzpet_app");
            strcpy(current_response_key, "slept");
        }
        else if (key == '5' || key == J_B4) { 
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
        else if (key == '7') {  // Toggle Emoji Mode
            int current = get_piece_state_int("fuzzpet", "emoji_mode");
            set_piece_state_int("fuzzpet", "emoji_mode", current ? 0 : 1);
            log_master("EventFire", "emoji_toggled", "fuzzpet", "fuzzpet_app");
            strcpy(current_response_key, "default");
        }
        else {
            strcpy(current_response_key, "default");
        }
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