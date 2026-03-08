#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdarg.h>

#define MAP_ROWS 10
#define MAP_COLS 20
#define MAX_PATH 16384

char project_root[MAX_PATH] = ".";
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

void get_state(const char* piece_id, const char* key, char* out_val) {
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

int get_state_int(const char* piece_id, const char* key) {
    char val[256]; get_state(piece_id, key, val);
    if (strcmp(val, "unknown") == 0 || val[0] == '\0') return -1;
    return atoi(val);
}

void set_state_int(const char* piece_id, const char* key, int val) {
    char *cmd = malloc(MAX_PATH * 2);
    if (!cmd) return;
    snprintf(cmd, MAX_PATH * 2, "'./pieces/master_ledger/plugins/+x/piece_manager.+x' %s set-state %s %d > /dev/null 2>&1", piece_id, key, val);
    system(cmd);
    free(cmd);
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
            } else map[y][0] = '\0'; 
        }
        fclose(f);
    }
}

int main(int argc, char *argv[]) {
    char active_id[64];
    get_state("manager", "active_target_id", active_id);
    
    if (strcmp(active_id, "unknown") == 0) return 1;

    int x = get_state_int(active_id, "pos_x");
    int y = get_state_int(active_id, "pos_y");
    int z = get_state_int(active_id, "pos_z");
    if (z == -1) z = 0;

    load_map(z);
    
    if (y < 0 || y >= MAP_ROWS || x < 0 || x >= MAP_COLS) return 1;
    
    char cell = map[y][x];
    int new_z = z;
    
    if (cell == '>') {
        new_z++;
    } else if (cell == '<') {
        new_z--;
    } else {
        return 0;
    }

    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "./pieces/apps/fuzzpet_app/world/map_z%d.txt", new_z);
    if (access(path, F_OK) == 0) {
        set_state_int(active_id, "pos_z", new_z);
        system("echo 'Z' >> ./pieces/display/frame_changed.txt");
    }

    return 0;
}
