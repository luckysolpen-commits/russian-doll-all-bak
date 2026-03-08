#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <stdarg.h>

// TPM Player Renderer (v1.2 - BUFFER FIX & ALIGNMENT)
// Responsibility: Draw spatial map based on active project state.

#define MAP_ROWS 10
#define MAP_COLS 20
#define MAX_ENTITIES 100
#define MAX_PATH 8192

typedef struct {
    char id[64];
    int x, y, z;
    char emoji[64];
} Entity;

char map_data[MAP_ROWS][MAP_COLS + 1];

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

// --- State Helpers ---

void get_engine_state(char* proj_id, char* map_id, int* z, char* active_piece) {
    const char* path = "./pieces/apps/player_app/manager/state.txt";
    FILE *f = fopen(path, "r");
    if (f) {
        char line[MAX_PATH];
        while (fgets(line, sizeof(line), f)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(line);
                char *v = trim_str(eq + 1);
                if (strcmp(k, "project_id") == 0) strcpy(proj_id, v);
                else if (strcmp(k, "current_map") == 0) strcpy(map_id, v);
                else if (strcmp(k, "current_z") == 0) *z = atoi(v);
                else if (strcmp(k, "active_piece") == 0) strcpy(active_piece, v);
            }
        }
        fclose(f);
    }
}

void get_piece_state(const char* proj_id, const char* piece_id, const char* key, char* out_val) {
    char *path = malloc(MAX_PATH);
    if (!path) { strcpy(out_val, "unknown"); return; }
    snprintf(path, MAX_PATH, "./projects/%s/pieces/%s/state.txt", proj_id, piece_id);
    FILE *f = fopen(path, "r");
    if (f) {
        char line[MAX_PATH];
        while (fgets(line, sizeof(line), f)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                if (strcmp(trim_str(line), key) == 0) {
                    strncpy(out_val, trim_str(eq + 1), 63);
                    fclose(f); free(path); return;
                }
            }
        }
        fclose(f);
    }
    free(path);
    strcpy(out_val, "unknown");
}

int get_piece_int(const char* proj_id, const char* piece_id, const char* key) {
    char val[64];
    get_piece_state(proj_id, piece_id, key, val);
    if (strcmp(val, "unknown") == 0) return -1;
    return atoi(val);
}

// --- Rendering Logic ---

void load_map(const char* proj_id, const char* map_id, int z) {
    char *path = malloc(MAX_PATH);
    if (!path) return;
    snprintf(path, MAX_PATH, "./projects/%s/maps/%s_z%d.txt", proj_id, map_id, z);
    FILE *f = fopen(path, "r");
    if (f) {
        for (int y = 0; y < MAP_ROWS; y++) {
            if (fgets(map_data[y], MAP_COLS + 2, f)) {
                map_data[y][strcspn(map_data[y], "\n\r")] = 0;
            } else map_data[y][0] = '\0';
        }
        fclose(f);
    } else {
        for (int y = 0; y < MAP_ROWS; y++) {
            memset(map_data[y], '.', MAP_COLS);
            map_data[y][MAP_COLS] = '\0';
        }
    }
    free(path);
}

int main() {
    char proj_id[256] = "template", map_id[256] = "map_0001", active_piece[64] = "selector";
    int current_z = 0;
    get_engine_state(proj_id, map_id, &current_z, active_piece);
    
    load_map(proj_id, map_id, current_z);

    // Fetch Entities
    Entity entities[MAX_ENTITIES];
    int entity_count = 0;
    char *pieces_path = malloc(MAX_PATH);
    if (!pieces_path) return 1;
    snprintf(pieces_path, MAX_PATH, "./projects/%s/pieces", proj_id);
    DIR *dir = opendir(pieces_path);
    free(pieces_path);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && entity_count < MAX_ENTITIES) {
            if (entry->d_name[0] == '.') continue;
            int pz = get_piece_int(proj_id, entry->d_name, "pos_z");
            if (pz == current_z) {
                strncpy(entities[entity_count].id, entry->d_name, 63);
                entities[entity_count].x = get_piece_int(proj_id, entry->d_name, "pos_x");
                entities[entity_count].y = get_piece_int(proj_id, entry->d_name, "pos_y");
                get_piece_state(proj_id, entry->d_name, "emoji", entities[entity_count].emoji);
                entity_count++;
            }
        }
        closedir(dir);
    }

    // Compose View
    FILE *fp = fopen("./pieces/apps/player_app/view.txt", "w");
    if (fp) {
        fprintf(fp, "╔═══════════════════════════════════════════════════════════╗\n");
        fprintf(fp, "║ PROJECT: %-10s | FLOOR: %-3d | FOCUS: %-10s ║\n", proj_id, current_z, active_piece);
        fprintf(fp, "╠═══════════════════════════════════════════════════════════╣\n");
        
        for (int y = 0; y < MAP_ROWS; y++) {
            fprintf(fp, "║  ");
            int chars_printed = 0;
            for (int x = 0; x < MAP_COLS; x++) {
                int ent_idx = -1;
                for(int i=0; i<entity_count; i++) {
                    if (entities[i].x == x && entities[i].y == y) {
                        ent_idx = i; break;
                    }
                }
                
                if (ent_idx != -1) {
                    if (strcmp(entities[ent_idx].id, "selector") == 0) fprintf(fp, "X");
                    else if (strcmp(entities[ent_idx].id, "player") == 0) fprintf(fp, "@");
                    else if (strstr(entities[ent_idx].id, "zombie") != NULL) fprintf(fp, "Z");
                    else if (strstr(entities[ent_idx].id, "stairs_up") != NULL) fprintf(fp, ">");
                    else if (strstr(entities[ent_idx].id, "stairs_down") != NULL) fprintf(fp, "<");
                    else fprintf(fp, "?");
                } else {
                    fprintf(fp, "%c", map_data[y][x]);
                }
                chars_printed++;
            }
            for (int p = chars_printed; p < 25; p++) fprintf(fp, "  ");
            fprintf(fp, "   ║\n");
        }
        fprintf(fp, "╚═══════════════════════════════════════════════════════════╝\n");
        fclose(fp);
    }

    return 0;
}
