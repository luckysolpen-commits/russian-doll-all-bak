#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif
#include <ctype.h>
#include <dirent.h>

#define MAX_PATH 16384

/* Global path cache */
char project_root[2048] = ".";
char current_project[64] = "fuzz-op";

char* trim_str(char *str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

char* build_path_malloc(const char* rel) {
    size_t sz = strlen(project_root) + strlen(rel) + 2;
    char* p = (char*)malloc(sz);
    if (p) snprintf(p, sz, "%s/%s", project_root, rel);
    return p;
}

int root_has_anchors(const char* root) {
    char pieces_path[4096], projects_path[4096];
    snprintf(pieces_path, sizeof(pieces_path), "%s/pieces", root);
    snprintf(projects_path, sizeof(projects_path), "%s/projects", root);
    return access(pieces_path, F_OK) == 0 && access(projects_path, F_OK) == 0;
}

void resolve_paths() {
    if (!getcwd(project_root, sizeof(project_root))) strncpy(project_root, ".", sizeof(project_root) - 1);
    project_root[sizeof(project_root) - 1] = '\0';

    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[2048];
        while (fgets(line, sizeof(line), kvp)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(line);
                char *v = trim_str(eq + 1);
                if (strcmp(k, "project_root") == 0 && *v) {
                    if (root_has_anchors(v)) snprintf(project_root, sizeof(project_root), "%s", v);
                }
            }
        }
        fclose(kvp);
    }

    // Resolve project_id from manager state
    char *mgr_state = build_path_malloc("pieces/apps/player_app/manager/state.txt");
    FILE *mf = fopen(mgr_state, "r");
    if (mf) {
        char line[1024];
        while (fgets(line, sizeof(line), mf)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                if (strcmp(trim_str(line), "project_id") == 0) strncpy(current_project, trim_str(eq + 1), 63);
            }
        }
        fclose(mf);
    }
    free(mgr_state);
}

void get_state(const char* piece_id, const char* key, char* out_val, size_t out_sz) {
    char path[4096], line[4096];
    
    // Try project-specific pieces first
    snprintf(path, sizeof(path), "%s/projects/%s/pieces/%s/state.txt", project_root, current_project, piece_id);
    
    if (access(path, F_OK) != 0) {
        // Fallback to global world
        snprintf(path, sizeof(path), "%s/pieces/world/map_01/%s/state.txt", project_root, piece_id);
    }
    
    out_val[0] = '\0';
    FILE *f = fopen(path, "r");
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                if (strcmp(trim_str(line), key) == 0) {
                    strncpy(out_val, trim_str(eq + 1), out_sz - 1);
                    out_val[out_sz - 1] = '\0';
                    fclose(f);
                    return;
                }
            }
        }
        fclose(f);
    }
}

int get_state_int(const char* piece_id, const char* key) {
    char val[256];
    get_state(piece_id, key, val, sizeof(val));
    if (val[0] == '\0') return -1;
    return atoi(val);
}

void set_state(const char* piece_id, const char* key, int val) {
    char path[4096], lines[100][256];
    int lc = 0, found = 0;
    
    // Try project-specific pieces first
    snprintf(path, sizeof(path), "%s/projects/%s/pieces/%s/state.txt", project_root, current_project, piece_id);
    
    if (access(path, F_OK) != 0) {
        // Fallback to global world
        snprintf(path, sizeof(path), "%s/pieces/world/map_01/%s/state.txt", project_root, piece_id);
    }
    
    // Read existing state
    FILE *f = fopen(path, "r");
    if (f) {
        while (fgets(lines[lc], sizeof(lines[0]), f) && lc < 99) {
            if (strncmp(lines[lc], key, strlen(key)) == 0 && lines[lc][strlen(key)] == '=') {
                snprintf(lines[lc], sizeof(lines[0]), "%s=%d\n", key, val);
                found = 1;
            }
            lc++;
        }
        fclose(f);
    }
    
    // Add key if not found
    if (!found && lc < 100) {
        snprintf(lines[lc++], sizeof(lines[0]), "%s=%d\n", key, val);
    }
    
    // Write state back
    f = fopen(path, "w");
    if (f) {
        for (int i = 0; i < lc; i++) fputs(lines[i], f);
        fclose(f);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) return 1;
    
    const char *zombie_id = argv[1];
    const char *target_id = argv[2];
    
    // Resolve paths from project root
    resolve_paths();
    
    // 1. Get Zombie Position
    int zx = get_state_int(zombie_id, "pos_x");
    int zy = get_state_int(zombie_id, "pos_y");
    int zz = get_state_int(zombie_id, "pos_z");
    if (zx == -1 || zy == -1) return 0;  // Zombie doesn't exist
    if (zz == -1) zz = 0;
    
    // 2. Get Target (Player) Position
    // The manager passes active_target_id which could be "selector" or any selected entity
    // We need to chase whatever the target is
    int tx = get_state_int(target_id, "pos_x");
    int ty = get_state_int(target_id, "pos_y");
    int tz = get_state_int(target_id, "pos_z");
    if (tz == -1) tz = 0;
    
    // 3. Only chase if on same Z level
    if (zz != tz) return 0;
    
    // 4. Don't chase if already adjacent or on top of target (attack range)
    int dx = tx - zx;
    int dy = ty - zy;
    int dist = abs(dx) + abs(dy);
    if (dist <= 1) return 0;  // Already in attack range
    
    // 5. Simple Chase AI (X then Y)
    int next_x = zx;
    int next_y = zy;
    
    if (tx > zx) next_x++;
    else if (tx < zx) next_x--;
    else if (ty > zy) next_y++;
    else if (ty < zy) next_y--;
    
    // 6. Basic bounds check (keep within map bounds 0-19)
    if (next_x < 0) next_x = 0;
    if (next_x > 19) next_x = 19;
    if (next_y < 0) next_y = 0;
    if (next_y > 19) next_y = 19;
    
    // 7. Update zombie position
    set_state(zombie_id, "pos_x", next_x);
    set_state(zombie_id, "pos_y", next_y);
    
    return 0;
}
