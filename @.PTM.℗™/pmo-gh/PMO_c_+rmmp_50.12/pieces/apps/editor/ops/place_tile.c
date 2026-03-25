/*
 * place_tile.c - Editor Op for placing tiles on project maps
 * Usage: place_tile.+x <map_file> <x> <y> <glyph>
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#define MAX_PATH 4096
#define MAX_CMD 16384
#define MAX_LINE 1024
#define DEFAULT_HEIGHT 20
#define DEFAULT_WIDTH 40

char project_root[MAX_PATH] = ".";
char current_project[MAX_LINE] = "template";

char* trim_str(char *str) {
    char *end;
    if(!str) return str;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void resolve_paths() {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), kvp)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(line), *v = trim_str(eq + 1);
                if (strcmp(k, "project_root") == 0) snprintf(project_root, sizeof(project_root), "%s", v);
            }
        }
        fclose(kvp);
    }
    
    char *state_path = NULL;
    if (asprintf(&state_path, "%s/pieces/apps/player_app/state.txt", project_root) != -1) {
        FILE *state = fopen(state_path, "r");
        if (state) {
            char line[MAX_LINE];
            while (fgets(line, sizeof(line), state)) {
                char *eq = strchr(line, '=');
                if (eq) {
                    *eq = '\0';
                    char *k = trim_str(line), *v = trim_str(eq + 1);
                    if (strcmp(k, "project_id") == 0) snprintf(current_project, sizeof(current_project), "%s", v);
                }
            }
            fclose(state);
        }
        free(state_path);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <map_file> <x> <y> <glyph>\n", argv[0]);
        return 1;
    }
    
    resolve_paths();
    const char *map_file = argv[1];
    int tx = atoi(argv[2]);
    int ty = atoi(argv[3]);
    const char *glyph = argv[4];
    
    char map_path[MAX_CMD];
    if (map_file[0] == '/') snprintf(map_path, sizeof(map_path), "%s", map_file);
    else snprintf(map_path, sizeof(map_path), "%s/projects/%s/maps/%s", project_root, current_project, map_file);
    
    FILE *f = fopen(map_path, "r");
    char **map_data = NULL;
    int rows = 0, cols = 0;
    
    if (f) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), f)) {
            char *nl = strchr(line, '\n'); if (nl) *nl = '\0';
            nl = strchr(line, '\r'); if (nl) *nl = '\0';
            int len = strlen(line);
            if (len > cols) cols = len;
            map_data = realloc(map_data, sizeof(char*) * (rows + 1));
            map_data[rows++] = strdup(line);
        }
        fclose(f);
    } else {
        rows = DEFAULT_HEIGHT; cols = DEFAULT_WIDTH;
        map_data = malloc(sizeof(char*) * rows);
        for (int i = 0; i < rows; i++) {
            map_data[i] = malloc(cols + 1);
            memset(map_data[i], '.', cols);
            map_data[i][cols] = '\0';
        }
    }
    
    /* Ensure targeted pos is within bounds or expand */
    if (ty >= rows) {
        int new_rows = ty + 1;
        map_data = realloc(map_data, sizeof(char*) * new_rows);
        for (int i = rows; i < new_rows; i++) {
            map_data[i] = malloc(cols + 1);
            memset(map_data[i], '.', cols);
            map_data[i][cols] = '\0';
        }
        rows = new_rows;
    }
    if (tx >= cols) {
        int new_cols = tx + 1;
        for (int i = 0; i < rows; i++) {
            map_data[i] = realloc(map_data[i], new_cols + 1);
            memset(map_data[i] + cols, '.', new_cols - cols);
            map_data[i][new_cols] = '\0';
        }
        cols = new_cols;
    }
    
    if (tx >= 0 && tx < cols && ty >= 0 && ty < rows) {
        map_data[ty][tx] = glyph[0];
    }
    
    f = fopen(map_path, "w");
    if (f) {
        for (int i = 0; i < rows; i++) {
            fprintf(f, "%s\n", map_data[i]);
            free(map_data[i]);
        }
        fclose(f);
    }
    free(map_data);
    
    return 0;
}
