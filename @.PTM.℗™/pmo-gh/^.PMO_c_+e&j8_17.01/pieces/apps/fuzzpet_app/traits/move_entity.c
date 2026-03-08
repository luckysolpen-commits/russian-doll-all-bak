#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>

// TPM Move Entity (v4.4.8 - ASPRINTF)
// Responsibility: Update piece coordinates with collision and boundary checks.

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

void resolve_paths() {
    const char* attempts[] = {"pieces/locations/location_kvp", "../pieces/locations/location_kvp", "../../pieces/locations/location_kvp", "../../../pieces/locations/location_kvp"};
    for (int i = 0; i < 4; i++) {
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
}

int get_state_int(const char* piece_id, const char* key) {
    if (!project_root) return -1;
    char *path = NULL;
    asprintf(&path, "%s/pieces/world/map_01/%s/state.txt", project_root, piece_id);
    if (access(path, F_OK) != 0) {
        free(path);
        if (strcmp(piece_id, "selector") == 0) asprintf(&path, "%s/pieces/apps/fuzzpet_app/selector/state.txt", project_root);
        else return -1;
    }
    FILE *f = fopen(path, "r");
    if (!f) { free(path); return -1; }
    char line[4096]; int val = -1;
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';
            if (strcmp(trim_str(line), key) == 0) { val = atoi(trim_str(eq + 1)); break; }
        }
    }
    fclose(f); free(path);
    return val;
}

int main(int argc, char *argv[]) {
    if (argc < 3) return 1;
    resolve_paths();
    if (!project_root) return 1;

    char *piece_id = argv[1];
    char move = argv[2][0];

    int x = get_state_int(piece_id, "pos_x");
    int y = get_state_int(piece_id, "pos_y");
    if (x == -1 || y == -1) return 1;

    int old_x = x, old_y = y;
    if (move == 'w') y--;
    else if (move == 's') y++;
    else if (move == 'a') x--;
    else if (move == 'd') x++;

    // Bounds (1-18, 1-8)
    if (x < 1) x = 1; if (x > 18) x = 18;
    if (y < 1) y = 1; if (y > 8) y = 8;

    // Collision
    int z = get_state_int(piece_id, "pos_z"); if (z == -1) z = 0;
    char *map_path = NULL;
    asprintf(&map_path, "%s/pieces/apps/fuzzpet_app/world/map_z%d.txt", project_root, z);
    if (access(map_path, F_OK) != 0) { free(map_path); asprintf(&map_path, "%s/pieces/apps/fuzzpet_app/world/map.txt", project_root); }
    
    FILE *mf = fopen(map_path, "r");
    if (mf) {
        char map_line[512];
        for (int i = 0; i <= y; i++) {
            if (!fgets(map_line, sizeof(map_line), mf)) break;
            if (i == y) {
                char tile = map_line[x];
                if (tile == '#' || tile == 'T' || tile == 'R') { x = old_x; y = old_y; }
            }
        }
        fclose(mf);
    }
    free(map_path);

    char *cmd = NULL;
    asprintf(&cmd, "'%s/pieces/master_ledger/plugins/+x/piece_manager.+x' %s set-state pos_x %d", project_root, piece_id, x);
    system(cmd); free(cmd);
    asprintf(&cmd, "'%s/pieces/master_ledger/plugins/+x/piece_manager.+x' %s set-state pos_y %d", project_root, piece_id, y);
    system(cmd); free(cmd);

    char *pulse = NULL;
    asprintf(&pulse, "%s/pieces/display/frame_changed.txt", project_root);
    FILE *pf = fopen(pulse, "a"); if (pf) { fputc('M', pf); fclose(pf); }
    free(pulse); free(project_root);

    return 0;
}
