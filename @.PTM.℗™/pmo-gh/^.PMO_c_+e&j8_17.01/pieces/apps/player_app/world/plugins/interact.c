#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

// interact.c - Engine Operation (v1.2 - BUFFER FIX)
// Responsibility: Find piece at current location and execute its trigger.

#define MAX_PATH 16384

char* trim_str(char *str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void get_state_str(const char* proj_id, const char* piece_id, const char* key, char* out) {
    char *path = malloc(MAX_PATH);
    if (!path) { strcpy(out, "unknown"); return; }
    snprintf(path, MAX_PATH, "./projects/%s/pieces/%s/state.txt", proj_id, piece_id);
    FILE *f = fopen(path, "r");
    free(path);
    if (!f) { strcpy(out, "unknown"); return; }
    char line[MAX_PATH];
    strcpy(out, "unknown");
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';
            char *k = trim_str(line);
            if (strcmp(k, key) == 0) {
                char *v = trim_str(eq + 1);
                strcpy(out, v);
                break;
            }
        }
    }
    fclose(f);
}

int get_state_int(const char* proj_id, const char* piece_id, const char* key) {
    char val[MAX_PATH];
    get_state_str(proj_id, piece_id, key, val);
    if (strcmp(val, "unknown") == 0) return -1;
    return atoi(val);
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    const char* active_piece = argv[1];
    
    // 1. Get Engine State (Project ID)
    char proj_id[256] = "template";
    FILE *ef = fopen("./pieces/apps/player_app/manager/state.txt", "r");
    if (ef) {
        char line[MAX_PATH];
        while (fgets(line, sizeof(line), ef)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(line);
                char *v = trim_str(eq + 1);
                if (strcmp(k, "project_id") == 0) {
                    strncpy(proj_id, v, 255);
                }
            }
        }
        fclose(ef);
    }

    // 2. Get active piece position
    int ax = get_state_int(proj_id, active_piece, "pos_x");
    int ay = get_state_int(proj_id, active_piece, "pos_y");
    int az = get_state_int(proj_id, active_piece, "pos_z");
    if (az == -1) az = 0;

    // 3. Find piece at ax, ay, az (other than active_piece)
    DIR *dir = opendir("./projects/template/pieces"); // Simplified for now
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            if (strcmp(entry->d_name, active_piece) == 0) continue;
            
            int px = get_state_int(proj_id, entry->d_name, "pos_x");
            int py = get_state_int(proj_id, entry->d_name, "pos_y");
            int pz = get_state_int(proj_id, entry->d_name, "pos_z");
            if (pz == -1) pz = 0;

            if (px == ax && py == ay && pz == az) {
                // FOUND TARGET PIECE
                char script[MAX_PATH];
                get_state_str(proj_id, entry->d_name, "on_interact", script);
                
                if (strcmp(script, "unknown") != 0) {
                    if (strstr(script, "stairs_up") != NULL) {
                        system("'./pieces/apps/player_app/world/plugins/+x/move_z.+x' selector 1");
                    } else if (strstr(script, "stairs_down") != NULL) {
                        system("'./pieces/apps/player_app/world/plugins/+x/move_z.+x' selector -1");
                    }
                }
                break;
            }
        }
        closedir(dir);
    }

    return 0;
}
