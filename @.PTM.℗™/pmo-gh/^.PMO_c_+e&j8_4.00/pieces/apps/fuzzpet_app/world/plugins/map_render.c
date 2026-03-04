#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>

#define MAP_ROWS 10
#define MAP_COLS 20

char map[MAP_ROWS][MAP_COLS + 1];

char* trim_str(char *str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
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
                if (strcmp(k, key) == 0) { strcpy(out_val, v); fclose(f); return; }
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

void load_map() {
    FILE *f = fopen("pieces/apps/fuzzpet_app/world/map.txt", "r");
    if (f) {
        for (int y = 0; y < MAP_ROWS; y++) {
            if (fgets(map[y], MAP_COLS + 2, f)) { 
                map[y][strcspn(map[y], "\n\r")] = 0; 
                for (int x = 0; x < MAP_COLS; x++) if (map[y][x] == '@') map[y][x] = '.';
            } else map[y][0] = '\0'; 
        }
        fclose(f);
    }
}

int main() {
    load_map();
    char active_id[64] = "unknown", last_key[64] = "None", last_resp[256] = "";
    int emoji_mode = 0;

    FILE *sf = fopen("pieces/apps/fuzzpet_app/manager/state.txt", "r");
    if (sf) {
        char line[128];
        while (fgets(line, sizeof(line), sf)) {
            line[strcspn(line, "\n\r")] = 0;
            if (strncmp(line, "active_target_id=", 17) == 0) strcpy(active_id, line + 17);
            if (strncmp(line, "last_key=", 9) == 0) strcpy(last_key, line + 9);
        }
        fclose(sf);
    }

    FILE *mir = fopen("pieces/apps/fuzzpet_app/fuzzpet/state.txt", "r");
    if (mir) {
        char line[128];
        while (fgets(line, sizeof(line), mir)) {
            if (strncmp(line, "emoji_mode=", 11) == 0) emoji_mode = atoi(line + 11);
        }
        fclose(mir);
    }

    FILE *rf = fopen("pieces/apps/fuzzpet_app/fuzzpet/last_response.txt", "r");
    if (rf) { if (fgets(last_resp, 256, rf)) { char *tr = trim_str(last_resp); strcpy(last_resp, tr); } fclose(rf); }

    char rendered_map[MAP_ROWS][256]; 
    for (int y = 0; y < MAP_ROWS; y++) {
        char* dst = rendered_map[y];
        for (int x = 0; x < MAP_COLS; x++) {
            char cell = map[y][x];
            int ent_found = 0;
            FILE *reg = fopen("pieces/apps/fuzzpet_app/world/registry.txt", "r");
            if (reg) {
                char pid[64];
                while (fgets(pid, sizeof(pid), reg)) {
                    char *id = trim_str(pid);
                    if (strlen(id) == 0 || strcmp(id, "selector") == 0) continue;
                    if (get_state_int_fast(id, "on_map") == 1 && get_state_int_fast(id, "pos_x") == x && get_state_int_fast(id, "pos_y") == y) {
                        if (emoji_mode) {
                            if (strcmp(id, "pet_01") == 0) strcpy(dst, "🐶");
                            else if (strcmp(id, "pet_02") == 0) strcpy(dst, "📦");
                            else strcpy(dst, "❓");
                        } else {
                            if (strcmp(id, "pet_01") == 0) strcpy(dst, "@");
                            else if (strcmp(id, "pet_02") == 0) strcpy(dst, "&");
                            else strcpy(dst, "?");
                        }
                        dst += strlen(dst); ent_found = 1; break;
                    }
                }
                fclose(reg);
            }

            if (!ent_found) {
                if (strcmp(active_id, "selector") == 0 && get_state_int_fast("selector", "pos_x") == x && get_state_int_fast("selector", "pos_y") == y) {
                    if (emoji_mode) strcpy(dst, "🎯"); else strcpy(dst, "X");
                } else {
                    if (emoji_mode) {
                        if (cell == '#') strcpy(dst, "🧱");
                        else if (cell == '.') strcpy(dst, "⬛");
                        else if (cell == 'T') strcpy(dst, "🌲");
                        else if (cell == 'R') strcpy(dst, "⛰️ ");
                        else { *dst = cell; *(dst+1) = '\0'; }
                    } else { *dst = cell; *(dst+1) = '\0'; }
                }
                dst += strlen(dst);
            }
        }
        *dst = '\0';
    }

    FILE *fp = fopen("pieces/apps/fuzzpet_app/fuzzpet/view.txt", "w");
    if (fp) {
        for (int y = 0; y < MAP_ROWS; y++) fprintf(fp, "║  %-20s                                     ║\n", rendered_map[y]);
        fprintf(fp, "╠═══════════════════════════════════════════════════════════╣\n");
        fprintf(fp, "║ [RESP]: %-49s ║\n", last_resp);
        fprintf(fp, "║ [ACTIVE]: %-10s | [KEY]: %-10s            ║\n", active_id, last_key);
        fclose(fp);
    }

    FILE *marker = fopen("pieces/apps/fuzzpet_app/fuzzpet/view_changed.txt", "a");
    if (marker) { fputc('X', marker); fputc('\n', marker); fclose(marker); }
    return 0;
}
