#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <stdarg.h>

// TPM Map Render (v5.3 - SELECTOR HIDE & ALIGN)
// Responsibility: Ensure selector is hidden by active pet and perfect alignment.

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

void resolve_root() {
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
    char cwd[4096]; getcwd(cwd, sizeof(cwd)); project_root = strdup(cwd);
}

void get_state_fast(const char* piece_id, const char* key, char* out_val) {
    if (!project_root) { strcpy(out_val, "unknown"); return; }
    char *path = NULL;
    asprintf(&path, "%s/pieces/world/map_01/%s/state.txt", project_root, piece_id);
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

void load_map(int z_level) {
    if (!project_root) return;
    char *path = NULL;
    asprintf(&path, "%s/pieces/apps/fuzzpet_app/world/map_z%d.txt", project_root, z_level);
    if (access(path, F_OK) != 0) {
        free(path);
        asprintf(&path, "%s/pieces/apps/fuzzpet_app/world/map.txt", project_root);
    }
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
    free(path);
}

int main() {
    resolve_root();
    int sx = get_state_int_fast("selector", "pos_x");
    int sy = get_state_int_fast("selector", "pos_y");
    int sz = get_state_int_fast("selector", "pos_z");
    if (sz == -1) sz = 0;
    
    load_map(sz);
    char active_id[64] = "selector", last_key[64] = "None", last_resp[512] = "";
    int emoji_mode = 0;

    char *path = NULL;
    asprintf(&path, "%s/pieces/apps/fuzzpet_app/manager/state.txt", project_root);
    FILE *sf = fopen(path, "r");
    if (sf) {
        char line[4096];
        while (fgets(line, sizeof(line), sf)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                if (strcmp(trim_str(line), "active_target_id") == 0) strncpy(active_id, trim_str(eq + 1), 63);
                else if (strcmp(trim_str(line), "last_key") == 0) strncpy(last_key, trim_str(eq + 1), 63);
            }
        }
        fclose(sf);
    }
    free(path);

    asprintf(&path, "%s/pieces/apps/fuzzpet_app/fuzzpet/state.txt", project_root);
    FILE *mir = fopen(path, "r");
    if (mir) {
        char line[128];
        while (fgets(line, sizeof(line), mir)) {
            if (strncmp(line, "emoji_mode=", 11) == 0) emoji_mode = atoi(line + 11);
        }
        fclose(mir);
    }
    free(path);

    asprintf(&path, "%s/pieces/apps/fuzzpet_app/fuzzpet/last_response.txt", project_root);
    FILE *rf = fopen(path, "r");
    if (rf) { if (fgets(last_resp, 512, rf)) { char *tr = trim_str(last_resp); strncpy(last_resp, tr, 511); } fclose(rf); }
    free(path);

    Entity entities[MAX_ENTITIES];
    int entity_count = 0;
    asprintf(&path, "%s/pieces/world/map_01", project_root);
    DIR *dir = opendir(path);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && entity_count < MAX_ENTITIES) {
            char *id = entry->d_name; if (id[0] == '.' || strcmp(id, "selector") == 0) continue;
            if (get_state_int_fast(id, "on_map") == 1) {
                int ez = get_state_int_fast(id, "pos_z"); if (ez == -1) ez = 0;
                if (ez != sz) continue;
                strncpy(entities[entity_count].id, id, 63);
                entities[entity_count].x = get_state_int_fast(id, "pos_x");
                entities[entity_count].y = get_state_int_fast(id, "pos_y");
                get_state_fast(id, "emoji", entities[entity_count].emoji);
                entity_count++;
            }
        }
        closedir(dir);
    }
    free(path);

    asprintf(&path, "%s/pieces/apps/fuzzpet_app/fuzzpet/view.txt", project_root);
    FILE *fp = fopen(path, "w");
    if (!fp) { free(path); return 1; }

    for (int y = 0; y < MAP_ROWS; y++) {
        fprintf(fp, "║  ");
        int visual_width = 0;
        for (int x = 0; x < MAP_COLS; x++) {
            int ent_found = 0;
            char *found_id = NULL;
            char *found_emoji = NULL;

            // PRIORITY 1: Entities (including Pet)
            for (int i = 0; i < entity_count; i++) {
                if (entities[i].x == x && entities[i].y == y) {
                    ent_found = 1;
                    found_id = entities[i].id;
                    found_emoji = entities[i].emoji;
                    break;
                }
            }

            if (ent_found) {
                if (emoji_mode) {
                    if (strcmp(found_emoji, "unknown") != 0 && strlen(found_emoji) > 0) fprintf(fp, "%s", found_emoji);
                    else if (strcmp(found_id, "pet_01") == 0) fprintf(fp, "🐶");
                    else if (strstr(found_id, "zombie") != NULL) fprintf(fp, "🧟");
                    else fprintf(fp, "❓");
                } else {
                    if (strcmp(found_id, "pet_01") == 0) fprintf(fp, "@");
                    else if (strstr(found_id, "zombie") != NULL) fprintf(fp, "Z");
                    else fprintf(fp, "?");
                }
            } 
            // PRIORITY 2: Selector (ONLY if no entity is here OR if selector is active target)
            else if (x == sx && y == sy) {
                if (emoji_mode) fprintf(fp, "🎯"); else fprintf(fp, "X");
            }
            // PRIORITY 3: Tiles
            else {
                char cell = map[y][x];
                if (emoji_mode) {
                    if (cell == '#') fprintf(fp, "🧱"); else if (cell == '.') fprintf(fp, "⬛"); else fprintf(fp, "%c", cell);
                } else fprintf(fp, "%c", cell);
            }
            visual_width++;
        }
        for (int p = visual_width; p < 25; p++) fprintf(fp, "  ");
        fprintf(fp, "   ║\n");
    }

    fprintf(fp, "╠═══════════════════════════════════════════════════════════╣\n");
    fprintf(fp, "║ [RESP]: %-49s ║\n", last_resp);
    fprintf(fp, "║ [Z-LEVEL]: %-3d | [ACTIVE]: %-10s | [KEY]: %-10s ║\n", sz, active_id, last_key);
    fclose(fp);

    asprintf(&path, "%s/pieces/apps/fuzzpet_app/fuzzpet/view_changed.txt", project_root);
    FILE *marker = fopen(path, "a"); if (marker) { fputc('X', marker); fputc('\n', marker); fclose(marker); }
    free(path); free(project_root);
    return 0;
}
