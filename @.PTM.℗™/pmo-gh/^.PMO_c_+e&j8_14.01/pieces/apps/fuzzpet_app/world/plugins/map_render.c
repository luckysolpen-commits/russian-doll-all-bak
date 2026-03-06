#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <stdarg.h>

// TPM Map Render (v4.1 - SILENT BUILD)
// Responsibility: Draw spatial map, sync view.

#define MAP_ROWS 10
#define MAP_COLS 20
#define MAX_ENTITIES 100

typedef struct {
    char id[64];
    int x, y;
    int on_map;
    char emoji[64];
} Entity;

char map[MAP_ROWS][MAP_COLS + 1];
char project_root[1024] = ".";

char* trim_str(char *str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void build_path(char* dst, size_t sz, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(dst, sz, fmt, args);
    va_end(args);
}

void resolve_root() {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[2048];
        while (fgets(line, sizeof(line), kvp)) {
            if (strncmp(line, "project_root=", 13) == 0) {
                char *v = line + 13;
                v[strcspn(v, "\n\r")] = 0;
                // FIX: Use snprintf for guaranteed null-termination
                snprintf(project_root, sizeof(project_root), "%s", v);
                break;
            }
        }
        fclose(kvp);
    }
}

void get_state_fast(const char* piece_id, const char* key, char* out_val) {
    char path[1024], line[2048];
    build_path(path, sizeof(path), "%s/pieces/world/map_01/%s/state.txt", project_root, piece_id);
    if (access(path, F_OK) != 0) {
        if (strcmp(piece_id, "fuzzpet") == 0) build_path(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/fuzzpet/state.txt", project_root);
        else if (strcmp(piece_id, "manager") == 0) build_path(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/manager/state.txt", project_root);
        else if (strcmp(piece_id, "selector") == 0) build_path(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/selector/state.txt", project_root);
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

void load_map() {
    char path[1024];
    build_path(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/world/map.txt", project_root);
    FILE *f = fopen(path, "r");
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
    resolve_root();
    load_map();
    char active_id[64] = "unknown", last_key[64] = "None", last_resp[256] = "";
    int emoji_mode = 0;

    char path[1024];
    build_path(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/manager/state.txt", project_root);
    FILE *sf = fopen(path, "r");
    if (sf) {
        char line[128];
        while (fgets(line, sizeof(line), sf)) {
            line[strcspn(line, "\n\r")] = 0;
            if (strncmp(line, "active_target_id=", 17) == 0) strncpy(active_id, line + 17, 63);
            if (strncmp(line, "last_key=", 9) == 0) strncpy(last_key, line + 9, 63);
        }
        fclose(sf);
    }

    build_path(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/fuzzpet/state.txt", project_root);
    FILE *mir = fopen(path, "r");
    if (mir) {
        char line[128];
        while (fgets(line, sizeof(line), mir)) {
            if (strncmp(line, "emoji_mode=", 11) == 0) emoji_mode = atoi(line + 11);
        }
        fclose(mir);
    }

    build_path(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/fuzzpet/last_response.txt", project_root);
    FILE *rf = fopen(path, "r");
    if (rf) { if (fgets(last_resp, 256, rf)) { char *tr = trim_str(last_resp); strncpy(last_resp, tr, 255); } fclose(rf); }

    Entity entities[MAX_ENTITIES];
    int entity_count = 0;
    char world_path[1024];
    build_path(world_path, sizeof(world_path), "%s/pieces/world/map_01", project_root);
    DIR *dir = opendir(world_path);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && entity_count < MAX_ENTITIES) {
            char *id = entry->d_name;
            if (strcmp(id, ".") == 0 || strcmp(id, "..") == 0 || strcmp(id, "selector") == 0) continue;
            struct stat st; char entity_dir[2048];
            build_path(entity_dir, sizeof(entity_dir), "%s/%s", world_path, id);
            if (stat(entity_dir, &st) == 0 && S_ISDIR(st.st_mode)) {
                if (get_state_int_fast(id, "on_map") == 1) {
                    strncpy(entities[entity_count].id, id, 63);
                    entities[entity_count].x = get_state_int_fast(id, "pos_x");
                    entities[entity_count].y = get_state_int_fast(id, "pos_y");
                    get_state_fast(id, "emoji", entities[entity_count].emoji);
                    entity_count++;
                }
            }
        }
        closedir(dir);
    }

    char rendered_map[MAP_ROWS][1024]; 
    for (int y = 0; y < MAP_ROWS; y++) {
        char* dst = rendered_map[y];
        for (int x = 0; x < MAP_COLS; x++) {
            char cell = map[y][x];
            int ent_found = 0;
            for (int i = 0; i < entity_count; i++) {
                if (entities[i].x == x && entities[i].y == y) {
                    if (emoji_mode) {
                        if (strcmp(entities[i].emoji, "unknown") != 0 && strlen(entities[i].emoji) > 0) strcpy(dst, entities[i].emoji);
                        else {
                            if (strcmp(entities[i].id, "pet_01") == 0) strcpy(dst, "🐶");
                            else if (strcmp(entities[i].id, "pet_02") == 0) strcpy(dst, "📦");
                            else strcpy(dst, "❓");
                        }
                    } else {
                        if (strcmp(entities[i].id, "pet_01") == 0) strcpy(dst, "@");
                        else if (strcmp(entities[i].id, "pet_02") == 0) strcpy(dst, "&");
                        else strcpy(dst, "?");
                    }
                    dst += strlen(dst); ent_found = 1; break;
                }
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

    build_path(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/fuzzpet/view.txt", project_root);
    FILE *fp = fopen(path, "w");
    if (fp) {
        for (int y = 0; y < MAP_ROWS; y++) fprintf(fp, "║  %-20s                                     ║\n", rendered_map[y]);
        fprintf(fp, "╠═══════════════════════════════════════════════════════════╣\n");
        fprintf(fp, "║ [RESP]: %-49s ║\n", last_resp);
        fprintf(fp, "║ [ACTIVE]: %-10s | [KEY]: %-10s            ║\n", active_id, last_key);
        fclose(fp);
    }

    build_path(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/fuzzpet/view_changed.txt", project_root);
    FILE *marker = fopen(path, "a");
    if (marker) { fputc('X', marker); fputc('\n', marker); fclose(marker); }
    return 0;
}
