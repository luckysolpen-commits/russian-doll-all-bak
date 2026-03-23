#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <stdarg.h>
#include <time.h>

// TPM Storage Trait (v1.2 - ASPRINTF & LOGGING)
// Responsibility: Scan, Collect, Place, Inventory logic for Selector.

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

void log_debug(const char* fmt, ...) {
    if (!project_root) return;
    char *path = NULL; asprintf(&path, "%s/pieces/apps/fuzzpet_app/manager/debug_log.txt", project_root);
    FILE *f = fopen(path, "a");
    if (f) {
        va_list args; va_start(args, fmt);
        time_t now = time(NULL);
        fprintf(f, "[%ld] [STORAGE] ", now);
        vfprintf(f, fmt, args);
        fprintf(f, "\n");
        va_end(args);
        fclose(f);
    }
    free(path);
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

void log_response(const char* msg) {
    if (!project_root) return;
    char *path = NULL; asprintf(&path, "%s/pieces/apps/fuzzpet_app/fuzzpet/last_response.txt", project_root);
    FILE *f = fopen(path, "w");
    if (f) { fprintf(f, "%-57s", msg); fclose(f); }
    free(path);
}

int get_state_int(const char* piece_id, const char* key) {
    if (!project_root) return -1;
    char *path = NULL; asprintf(&path, "%s/pieces/world/map_01/%s/state.txt", project_root, piece_id);
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
    char *action = argv[2];

    int x = get_state_int(piece_id, "pos_x");
    int y = get_state_int(piece_id, "pos_y");

    log_debug("Action: %s | Piece: %s | Pos: (%d,%d)", action, piece_id, x, y);

    if (strcmp(action, "scan") == 0) {
        char *world_dir = NULL; asprintf(&world_dir, "%s/pieces/world/map_01", project_root);
        DIR *dir = opendir(world_dir);
        char *found_msg = strdup("Found: ");
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_name[0] == '.') continue;
                int tx = get_state_int(entry->d_name, "pos_x");
                int ty = get_state_int(entry->d_name, "pos_y");
                if (tx == x && ty == y) {
                    char *old = found_msg;
                    asprintf(&found_msg, "%s%s ", old, entry->d_name);
                    free(old);
                }
            }
            closedir(dir);
        }
        log_response(found_msg);
        free(found_msg); free(world_dir);
    } 
    else if (strcmp(action, "collect") == 0) {
        char *world_dir = NULL; asprintf(&world_dir, "%s/pieces/world/map_01", project_root);
        DIR *dir = opendir(world_dir);
        int collected = 0;
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_name[0] == '.' || strcmp(entry->d_name, piece_id) == 0) continue;
                int tx = get_state_int(entry->d_name, "pos_x");
                int ty = get_state_int(entry->d_name, "pos_y");
                if (tx == x && ty == y) {
                    char *msg = NULL; asprintf(&msg, "Picked up %s", entry->d_name);
                    log_response(msg); free(msg);
                    char *cmd = NULL; asprintf(&cmd, "'%s/pieces/master_ledger/plugins/+x/piece_manager.+x' %s set-state inventory %s", project_root, piece_id, entry->d_name);
                    system(cmd); free(cmd);
                    collected = 1; break;
                }
            }
            closedir(dir);
        }
        if (!collected) log_response("Nothing here to collect.");
        free(world_dir);
    }
    else if (strcmp(action, "place") == 0) {
        char *msg = NULL; asprintf(&msg, "Placed item at (%d,%d)", x, y);
        log_response(msg); free(msg);
    }
    else if (strcmp(action, "inventory") == 0) {
        log_response("Inventory: [empty]");
    }

    free(project_root);
    return 0;
}
