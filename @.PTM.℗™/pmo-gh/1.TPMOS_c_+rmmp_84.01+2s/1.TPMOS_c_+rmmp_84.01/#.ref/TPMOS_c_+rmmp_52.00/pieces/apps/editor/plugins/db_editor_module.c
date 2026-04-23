/*
 * db_editor_module.c - Sub-module for Database & Global Logic
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>

#define MAX_PATH 4096
#define MAX_LINE 1024

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
}

void load_context() {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/player_app/manager/state.txt", project_root) != -1) {
        FILE *f = fopen(path, "r");
        if (f) {
            char line[MAX_LINE];
            while (fgets(line, sizeof(line), f)) {
                char *eq = strchr(line, '=');
                if (eq) {
                    *eq = '\0';
                    if (strcmp(trim_str(line), "project_id") == 0) strncpy(current_project, trim_str(eq+1), MAX_LINE-1);
                }
            }
            fclose(f);
        }
        free(path);
    }
}

void write_db_state(const char* msg) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/player_app/state.txt", project_root) != -1) {
        FILE *f = fopen(path, "w");
        if (f) {
            fprintf(f, "project_id=%s\ndb_response=[DB]: %s\n", current_project, msg);
            fclose(f);
        }
        free(path);
    }
}

int is_active_layout() {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/display/current_layout.txt", project_root) == -1) return 0;
    FILE *f = fopen(path, "r"); if (!f) return 0;
    char line[MAX_LINE];
    if (fgets(line, sizeof(line), f)) { fclose(f); int res = (strstr(line, "db_editor.chtpm") != NULL); free(path); return res; }
    fclose(f); free(path); return 0;
}

int main() {
    resolve_paths(); load_context();
    write_db_state("Ready.");
    
    long last_pos = 0; struct stat st; char *hist_path = NULL;
    asprintf(&hist_path, "%s/pieces/apps/player_app/history.txt", project_root);
    
    while (1) {
        if (!is_active_layout()) { usleep(100000); continue; }
        if (stat(hist_path, &st) == 0 && st.st_size > last_pos) {
            FILE *hf = fopen(hist_path, "r");
            if (hf) {
                fseek(hf, last_pos, SEEK_SET);
                int key;
                while (fscanf(hf, "%d", &key) == 1) {
                    if (key == '1') write_db_state("Actors Blueprint View");
                    else if (key == '2') write_db_state("Monsters Blueprint View");
                    else if (key == '3') write_db_state("Items Blueprint View");
                    else if (key == '4') {
                        /* Switch to PAL Stacker for Global Logic */
                        char *lp = NULL;
                        if (asprintf(&lp, "%s/pieces/display/layout_changed.txt", project_root) != -1) {
                            FILE *f = fopen(lp, "a"); if (f) { fprintf(f, "pieces/apps/editor/layouts/pal_editor.chtpm\n"); fclose(f); }
                            free(lp);
                        }
                    }
                }
                last_pos = ftell(hf);
                fclose(hf);
            }
        }
        usleep(16667);
    }
    return 0;
}
