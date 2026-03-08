#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>

#define MAX_PATH 16384

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
char project_root[MAX_PATH] = ".";

// Selector System State
int selector_active = 0;           // 0 = player mode, 1 = selector mode
int selector_x = 5;                // Selector cursor X position
int selector_y = 5;                // Selector cursor Y position
int selector_z = 0;                // NEW: Z level
int saved_player_x = 5;            // Saved player position when selector activated
int saved_player_y = 5;            // Saved player position when selector activated
char selected_info[1024] = "";      // Info about selected tile/entity

// --- TPM Helpers ---

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

int get_piece_state_int(const char* piece, const char* key) {
    char *cmd = malloc(MAX_PATH * 2);
    if (!cmd) return 0;
    snprintf(cmd, MAX_PATH * 2, "'./pieces/master_ledger/plugins/+x/piece_manager.+x' %s get-state %s", piece, key);
    FILE *fp = popen(cmd, "r");
    if (!fp) { free(cmd); return 0; }
    char val[128];
    int result = 0;
    if (fgets(val, sizeof(val), fp)) {
        char *t = trim_str(val);
        if (strcmp(t, "STATE_NOT_FOUND") != 0) result = atoi(t);
    }
    pclose(fp);
    free(cmd);
    return result;
}

void set_piece_state_int(const char* piece, const char* key, int val) {
    char *cmd = malloc(MAX_PATH * 2);
    if (!cmd) return;
    snprintf(cmd, MAX_PATH * 2, "'./pieces/master_ledger/plugins/+x/piece_manager.+x' %s set-state %s %d > /dev/null 2>&1", piece, key, val);
    system(cmd);
    free(cmd);
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
        case '>': return "⬆️ ";
        case '<': return "⬇️ ";
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

    FILE *f = fopen("./pieces/apps/fuzzpet_app/fuzzpet/state.txt", "w");
    if (f) {
        fprintf(f, "hunger=%d\nhappiness=%d\nenergy=%d\nlevel=%d\npos_x=%d\npos_y=%d\nstatus=active\nemoji_mode=%d\n",
                hunger, happiness, energy, level, pos_x, pos_y, emoji_mode);
        fclose(f);
    }
}

void write_sovereign_view() {
    sync_mirror();

    char response[512] = "";
    char *cmd = malloc(MAX_PATH * 2);
    if (!cmd) return;
    snprintf(cmd, MAX_PATH * 2, "'./pieces/master_ledger/plugins/+x/piece_manager.+x' fuzzpet get-response %s", current_response_key);
    FILE *rf = popen(cmd, "r");
    if (rf) {
        if (fgets(response, sizeof(response), rf)) {
            response[strcspn(response, "\n")] = 0;
        }
        pclose(rf);
    }
    free(cmd);

    int emoji_mode = get_piece_state_int("fuzzpet", "emoji_mode");
    FILE *fp = fopen("./pieces/apps/fuzzpet_app/fuzzpet/view.txt", "w");
    if (!fp) return;

    char render_map[MAP_ROWS][MAP_COLS + 1];
    for (int y = 0; y < MAP_ROWS; y++) {
        strncpy(render_map[y], map[y], MAP_COLS);
        render_map[y][MAP_COLS] = '\0';
    }
    
    if (selector_active && selector_x >= 0 && selector_x < MAP_COLS && selector_y >= 0 && selector_y < MAP_ROWS) {
        render_map[selector_y][selector_x] = 'X';
    }

    int current_z = selector_active ? selector_z : get_piece_state_int("fuzzpet", "pos_z");
    if (current_z == -1) current_z = 0;

    if (emoji_mode) {
        for (int y = 0; y < MAP_ROWS; y++) {
            fprintf(fp, "║");
            for (int x = 0; x < MAP_COLS; x++) {
                char c = render_map[y][x];
                const char* emoji = get_emoji_for_char(c, emoji_mode);
                if (emoji) fprintf(fp, "%s", emoji);
                else fprintf(fp, "%c", c);
            }
            fprintf(fp, "║\n");
        }
        fprintf(fp, "╠═══════════════════════════════════════════════════════════╣\n");
        fprintf(fp, "║ [FUZZPET]: %-48s ║\n", response);
        fprintf(fp, "║ [Z-LEVEL]: %-3d | [ACTIVE]: %-35s ║\n", current_z, selector_active ? "selector" : "fuzzpet");
        fprintf(fp, "║ [SELECTOR]: %-50s ║\n", selector_active ? selected_info : "Press 9 or J-B8 to toggle");
    } else {
        for (int y = 0; y < MAP_ROWS; y++) {
            fprintf(fp, "║  %-20s                                     ║\n", render_map[y]);
        }
        fprintf(fp, "╠═══════════════════════════════════════════════════════════╣\n");
        fprintf(fp, "║  [FUZZPET]: %-45s ║\n", response);
        fprintf(fp, "║ [Z-LEVEL]: %-3d | [ACTIVE]: %-35s ║\n", current_z, selector_active ? "selector" : "fuzzpet");
        fprintf(fp, "║  [SELECTOR]: %-43s ║\n", selector_active ? selected_info : "Press 9 or J-B8 to toggle");
    }
    
    fclose(fp);
    
    FILE *marker = fopen("./pieces/apps/fuzzpet_app/fuzzpet/view_changed.txt", "a");
    if (marker) {
        fputc('X', marker);
        fputc('\n', marker);
        fclose(marker);
    }
}

void save_map() {
    FILE *f = fopen("./pieces/apps/fuzzpet_app/world/map.txt", "w");
    if (f) {
        for (int y = 0; y < MAP_ROWS; y++) {
            fprintf(f, "%.*s\n", MAP_COLS, map[y]);
        }
        fclose(f);
    }
}

void load_map(int z_level) {
    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "./pieces/apps/fuzzpet_app/world/map_z%d.txt", z_level);
    
    if (access(path, F_OK) != 0) {
        snprintf(path, MAX_PATH, "./pieces/apps/fuzzpet_app/world/map.txt");
    }

    FILE *f = fopen(path, "r");
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
    }

    int initial_pos_x = get_piece_state_int("fuzzpet", "pos_x");
    int initial_pos_y = get_piece_state_int("fuzzpet", "pos_y");
    int initial_pos_z = get_piece_state_int("fuzzpet", "pos_z");
    if (initial_pos_z == -1) initial_pos_z = 0;

    for (int y = 0; y < MAP_ROWS; y++) {
        for (int x = 0; x < MAP_COLS; x++) {
            if (map[y][x] == '@') map[y][x] = '.';
        }
    }
    
    if (initial_pos_z == z_level) {
        if (initial_pos_x >= 0 && initial_pos_x < MAP_COLS &&
            initial_pos_y >= 0 && initial_pos_y < MAP_ROWS) {
            map[initial_pos_y][initial_pos_x] = '@';
        }
    }
}

// --- Selector System Functions ---

void move_z(int dz) {
    const char* piece_id = selector_active ? "selector" : "fuzzpet";
    int current_z = selector_active ? selector_z : get_piece_state_int("fuzzpet", "pos_z");
    if (current_z == -1) current_z = 0;

    int new_z = current_z + dz;
    if (new_z < -1) new_z = -1;
    if (new_z > 1) new_z = 1;
    
    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "./pieces/apps/fuzzpet_app/world/map_z%d.txt", new_z);
    if (access(path, F_OK) != 0) {
        snprintf(selected_info, sizeof(selected_info), "No map at Z level %d", new_z);
        return;
    }
    
    if (selector_active) {
        selector_z = new_z;
        set_piece_state_int("selector", "pos_z", selector_z);
        load_map(selector_z);
        snprintf(selected_info, sizeof(selected_info), "Selector moved to Z level %d", selector_z);
    } else {
        set_piece_state_int("fuzzpet", "pos_z", new_z);
        load_map(new_z);
        snprintf(selected_info, sizeof(selected_info), "Player moved to Z level %d", new_z);
    }
    
    log_master("EventFire", "z_level_changed", piece_id, "fuzzpet_app");
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
    
    system("'./pieces/apps/fuzzpet_app/clock/plugins/+x/increment_time.+x' > /dev/null 2>&1");
    system("'./pieces/apps/fuzzpet_app/clock/plugins/+x/increment_turn.+x' > /dev/null 2>&1");
    
    strcpy(current_response_key, "moved");
    save_map();
    log_master("EventFire", "moved", "fuzzpet", "fuzzpet_app");
}

void toggle_selector() {
    int pos_x = get_piece_state_int("fuzzpet", "pos_x");
    int pos_y = get_piece_state_int("fuzzpet", "pos_y");
    
    if (!selector_active) {
        selector_active = 1;
        selector_x = pos_x;
        selector_y = pos_y;
        selector_z = get_piece_state_int("fuzzpet", "pos_z");
        if (selector_z == -1) selector_z = 0;
        set_piece_state_int("selector", "pos_z", selector_z);
        
        saved_player_x = pos_x;
        saved_player_y = pos_y;
        snprintf(selected_info, sizeof(selected_info), "Selector activated at (%d,%d,z:%d)", selector_x, selector_y, selector_z);
        log_master("EventFire", "selector_activated", "fuzzpet", "fuzzpet_app");
    } else {
        selector_active = 0;
        snprintf(selected_info, sizeof(selected_info), "Selector deactivated");
        log_master("EventFire", "selector_deactivated", "fuzzpet", "fuzzpet_app");
    }
}

void move_selector(int dx, int dy) {
    if (!selector_active) return;
    int new_x = selector_x + dx;
    int new_y = selector_y + dy;
    if (new_x < 0) new_x = 0;
    if (new_x >= MAP_COLS) new_x = MAP_COLS - 1;
    if (new_y < 0) new_y = 0;
    if (new_y >= MAP_ROWS) new_y = MAP_ROWS - 1;
    selector_x = new_x;
    selector_y = new_y;
    char tile_char = map[selector_y][selector_x];
    const char* tile_desc = "terrain";
    switch(tile_char) {
        case '#': tile_desc = "wall"; break;
        case '.': tile_desc = "floor"; break;
        case '@': tile_desc = "player"; break;
        case 'R': tile_desc = "rock"; break;
        case 'T': tile_desc = "treasure"; break;
        case '>': tile_desc = "stairs up"; break;
        case '<': tile_desc = "stairs down"; break;
    }
    snprintf(selected_info, sizeof(selected_info), "Selected: %s at (%d,%d)", tile_desc, selector_x, selector_y);
}

void selector_inspect() {
    if (!selector_active) return;
    char tile_char = map[selector_y][selector_x];
    int pos_x = get_piece_state_int("fuzzpet", "pos_x");
    int pos_y = get_piece_state_int("fuzzpet", "pos_y");
    int pos_z = get_piece_state_int("fuzzpet", "pos_z");
    if (pos_z == -1) pos_z = 0;

    if (selector_x == pos_x && selector_y == pos_y && selector_z == pos_z) {
        int hunger = get_piece_state_int("fuzzpet", "hunger");
        int happiness = get_piece_state_int("fuzzpet", "happiness");
        int energy = get_piece_state_int("fuzzpet", "energy");
        snprintf(selected_info, sizeof(selected_info), "Player at (%d,%d,z:%d) - HGR:%d HAP:%d ENR:%d", 
                 selector_x, selector_y, selector_z, hunger, happiness, energy);
    } else {
        const char* tile_desc = "Terrain";
        switch(tile_char) {
            case '#': tile_desc = "Wall (impassable)"; break;
            case '.': tile_desc = "Floor (walkable)"; break;
            case '@': tile_desc = "Player character"; break;
            case 'R': tile_desc = "Rock (impassable)"; break;
            case 'T': tile_desc = "Treasure (collectible)"; break;
            case '>': tile_desc = "Stairs Up"; break;
            case '<': tile_desc = "Stairs Down"; break;
        }
        snprintf(selected_info, sizeof(selected_info), "%s at (%d,%d,z:%d)", tile_desc, selector_x, selector_y, selector_z);
    }
    log_master("EventFire", "selector_inspect", "fuzzpet", "fuzzpet_app");
}

void process_key(int key) {
    if (selector_active) {
        if (key == '9' || key == J_B8) toggle_selector();
        else if (key == 'z' || key == 'Z') move_z(-1);
        else if (key == 'x' || key == 'X') move_z(1);
        else if (key == 'w' || key == 'W' || key == K_UP || key == J_UP) move_selector(0, -1);
        else if (key == 's' || key == 'S' || key == K_DOWN || key == J_DN) move_selector(0, 1);
        else if (key == 'a' || key == 'A' || key == K_LEFT || key == J_LF) move_selector(-1, 0);
        else if (key == 'd' || key == 'D' || key == K_RIGHT || key == J_RT) move_selector(1, 0);
        else if (key == 10 || key == 13 || key == J_B0 || key == ' ') {
            system("'./pieces/apps/fuzzpet_app/world/plugins/+x/use_stairs.+x' > /dev/null 2>&1");
            selector_z = get_piece_state_int("selector", "pos_z");
            if (selector_z == -1) selector_z = 0;
            load_map(selector_z);
            log_master("EventFire", "used_stairs", "fuzzpet", "fuzzpet_app");
        }
    } else {
        if (key == '9' || key == J_B8) toggle_selector();
        else if (key == 'z' || key == 'Z') move_z(-1);
        else if (key == 'x' || key == 'X') move_z(1);
        else if (key == 'w' || key == 'W' || key == K_UP || key == J_UP) move('w');
        else if (key == 's' || key == 'S' || key == K_DOWN || key == J_DN) move('s');
        else if (key == 'a' || key == 'A' || key == K_LEFT || key == J_LF) move('a');
        else if (key == 'd' || key == 'D' || key == K_RIGHT || key == J_RT) move('d');
        else if (key == ' ' || key == J_B0) {
            system("'./pieces/apps/fuzzpet_app/world/plugins/+x/use_stairs.+x' > /dev/null 2>&1");
            int current_z = get_piece_state_int("fuzzpet", "pos_z");
            if (current_z == -1) current_z = 0;
            load_map(current_z);
            log_master("EventFire", "player_used_stairs", "fuzzpet", "fuzzpet_app");
        }
        else if (key == '2' || key == J_B1) {
            int h = get_piece_state_int("fuzzpet", "hunger");
            set_piece_state_int("fuzzpet", "hunger", (h > 20) ? h - 20 : 0);
            strcpy(current_response_key, "fed");
        }
        else if (key == '3' || key == J_B2) {
            int h = get_piece_state_int("fuzzpet", "happiness");
            set_piece_state_int("fuzzpet", "happiness", (h < 90) ? h + 10 : 100);
            strcpy(current_response_key, "played");
        }
        else if (key == '4' || key == J_B3) {
            int e = get_piece_state_int("fuzzpet", "energy");
            set_piece_state_int("fuzzpet", "energy", (e < 70) ? e + 30 : 100);
            strcpy(current_response_key, "slept");
        }
        else if (key == '7') {
            int current = get_piece_state_int("fuzzpet", "emoji_mode");
            set_piece_state_int("fuzzpet", "emoji_mode", current ? 0 : 1);
        }
    }
    write_sovereign_view();
}

void* input_thread(void* arg) {
    long last_pos = 0;
    while (1) {
        FILE *hf = fopen("./pieces/apps/fuzzpet_app/fuzzpet/history.txt", "r");
        if (hf) {
            struct stat st;
            if (fstat(fileno(hf), &st) == 0) {
                if (st.st_size > last_pos) {
                    fseek(hf, last_pos, SEEK_SET);
                    int key;
                    while (fscanf(hf, "%d", &key) == 1) process_key(key);
                    last_pos = ftell(hf);
                } else if (st.st_size < last_pos) last_pos = 0;
            }
            fclose(hf);
        }
        usleep(16667); 
    }
    return NULL;
}

int main() {
    resolve_root();
    selector_z = get_piece_state_int("selector", "pos_z");
    if (selector_z == -1) selector_z = 0;
    load_map(selector_z);
    write_sovereign_view();
    pthread_t t;
    pthread_create(&t, NULL, input_thread, NULL);
    while (1) usleep(1000000);
    return 0;
}
