/*
 * Op: project_loader
 * Usage: ./+x/project_loader.+x
 * 
 * Behavior:
 *   1. Scans projects/ directory
 *   2. Generates a temporary PDL menu of projects
 *   3. Calls menu_op to let user select
 *   4. Writes selected project_id to engine state
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

#define MAX_PATH 4096
#define MAX_LINE 256

char project_root[MAX_PATH] = ".";

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
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), kvp)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(line);
                char *v = trim_str(eq + 1);
                if (strcmp(k, "project_root") == 0) snprintf(project_root, sizeof(project_root), "%s", v);
            }
        }
        fclose(kvp);
    }
}

int main() {
    resolve_paths();
    
    char projects_path[MAX_PATH];
    snprintf(projects_path, sizeof(projects_path), "%s/projects", project_root);
    
    DIR *dir = opendir(projects_path);
    if (!dir) {
        fprintf(stderr, "Error: Could not open %s\n", projects_path);
        return 1;
    }

    char tmp_pdl_path[MAX_PATH];
    snprintf(tmp_pdl_path, sizeof(tmp_pdl_path), "%s/pieces/apps/player_app/PAL/tmp_projects.pdl", project_root);
    
    /* Ensure directory exists */
    char cmd[MAX_PATH * 2];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s/pieces/apps/player_app/PAL", project_root);
    system(cmd);

    FILE *f = fopen(tmp_pdl_path, "w");
    if (!f) {
        fprintf(stderr, "Error: Could not write %s\n", tmp_pdl_path);
        closedir(dir);
        return 1;
    }

    fprintf(f, "SECTION      | KEY                | VALUE\n");
    fprintf(f, "----------------------------------------\n");
    fprintf(f, "META         | piece_id           | tmp_projects\n");
    fprintf(f, "META         | title              | Select Project\n\n");

    struct dirent *entry;
    char proj_list[16][256];
    int count = 0;
    while ((entry = readdir(dir)) != NULL && count < 16) {
        if (entry->d_name[0] == '.') continue;
        
        char path[MAX_PATH];
        snprintf(path, MAX_PATH, "%s/projects/%s", project_root, entry->d_name);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            strncpy(proj_list[count], entry->d_name, 255);
            fprintf(f, "METHOD       | %s | void\n", entry->d_name);
            count++;
        }
    }
    fclose(f);
    closedir(dir);

    if (count == 0) {
        fprintf(stderr, "No projects found.\n");
        return 0;
    }

    /* Call menu_op Op */
    snprintf(cmd, sizeof(cmd), "%s/pieces/apps/playrm/ops/+x/menu_op.+x %s", project_root, tmp_pdl_path);
    
    FILE *mf = popen(cmd, "r");
    if (mf) {
        char buf[16];
        if (fgets(buf, sizeof(buf), mf)) {
            int sel = atoi(buf);
            if (sel > 0 && sel <= count) {
                /* Success! Update engine state */
                char engine_state_path[MAX_PATH];
                snprintf(engine_state_path, sizeof(engine_state_path), "%s/pieces/apps/player_app/manager/state.txt", project_root);
                
                char lines[20][MAX_LINE];
                int line_count = 0;
                FILE *sf = fopen(engine_state_path, "r");
                if (sf) {
                    while (fgets(lines[line_count], MAX_LINE, sf) && line_count < 19) {
                        lines[line_count][strcspn(lines[line_count], "\n\r")] = 0;
                        line_count++;
                    }
                    fclose(sf);
                }
                
                /* Update or add project_id and current_z */
                int proj_found = 0, z_found = 0;
                for (int i = 0; i < line_count; i++) {
                    if (strncmp(lines[i], "project_id=", 11) == 0) {
                        snprintf(lines[i], MAX_LINE, "project_id=%s", proj_list[sel-1]);
                        proj_found = 1;
                    }
                    if (strncmp(lines[i], "current_z=", 10) == 0) {
                        snprintf(lines[i], MAX_LINE, "current_z=0");
                        z_found = 1;
                    }
                }
                if (!proj_found) snprintf(lines[line_count++], MAX_LINE, "project_id=%s", proj_list[sel-1]);
                if (!z_found) snprintf(lines[line_count++], MAX_LINE, "current_z=0");
                
                sf = fopen(engine_state_path, "w");
                if (sf) {
                    for (int i = 0; i < line_count; i++) fprintf(sf, "%s\n", lines[i]);
                    fclose(sf);
                }
                printf("%d\n", sel);
            }
        }
        pclose(mf);
    }

    return 0;
}
