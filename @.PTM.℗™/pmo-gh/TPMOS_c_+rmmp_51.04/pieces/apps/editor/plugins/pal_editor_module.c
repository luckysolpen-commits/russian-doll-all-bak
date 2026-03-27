/*
 * pal_editor_module.c - Sub-module for Visual Op Stacking
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
#define MAX_OPS 100

char project_root[MAX_PATH] = ".";
char current_project[MAX_LINE] = "template";
char active_target_id[MAX_LINE] = "GLOBAL";

char op_palette[MAX_OPS][64];
int op_palette_count = 0;
int op_palette_idx = 0;

char script_stack[MAX_OPS][128];
int script_stack_count = 0;
int script_stack_idx = 0;

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
                    char *k = trim_str(line), *v = trim_str(eq+1);
                    if (strcmp(k, "project_id") == 0) strncpy(current_project, v, MAX_LINE-1);
                    else if (strcmp(k, "active_target_id") == 0) strncpy(active_target_id, v, MAX_LINE-1);
                }
            }
            fclose(f);
        }
        free(path);
    }
}

void scan_ops() {
    char *cmd = NULL;
    if (asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/get_ops_list.+x", project_root) != -1) {
        FILE *f = popen(cmd, "r");
        if (f) {
            char buf[4096];
            if (fgets(buf, sizeof(buf), f)) {
                char *tok = strtok(buf, ",");
                op_palette_count = 0;
                while (tok && op_palette_count < MAX_OPS) {
                    strncpy(op_palette[op_palette_count++], trim_str(tok), 63);
                    tok = strtok(NULL, ",");
                }
            }
            pclose(f);
        }
        free(cmd);
    }
}

void write_pal_editor_state() {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/player_app/state.txt", project_root) != -1) {
        FILE *f = fopen(path, "w");
        if (f) {
            fprintf(f, "project_id=%s\nactive_target_id=%s\n", current_project, active_target_id);
            char pal_str[2048] = "";
            for (int i = 0; i < op_palette_count; i++) {
                if (i == op_palette_idx) strcat(pal_str, "[");
                strcat(pal_str, op_palette[i]);
                if (i == op_palette_idx) strcat(pal_str, "]");
                strcat(pal_str, " ");
            }
            fprintf(f, "op_palette_view=%s\n", pal_str);
            char stack_str[2048] = "";
            if (script_stack_count == 0) strcpy(stack_str, "(Empty)");
            for (int i = 0; i < script_stack_count; i++) {
                if (i == script_stack_idx) strcat(stack_str, "[");
                strcat(stack_str, script_stack[i]);
                if (i == script_stack_idx) strcat(stack_str, "]");
                strcat(stack_str, " ");
            }
            fprintf(f, "script_stack_view=%s\n", stack_str);
            if (op_palette_count > 0) fprintf(f, "selected_op_name=%s\n", op_palette[op_palette_idx]);
            else fprintf(f, "selected_op_name=None\n");
            fprintf(f, "selected_op_args=...\n");
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
    if (fgets(line, sizeof(line), f)) { fclose(f); int res = (strstr(line, "pal_editor.chtpm") != NULL); free(path); return res; }
    fclose(f); free(path); return 0;
}

int main() {
    resolve_paths(); load_context(); scan_ops();
    write_pal_editor_state();
    
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
                    if (key == 1000 || key == 'a') { op_palette_idx--; if (op_palette_idx < 0) op_palette_idx = op_palette_count - 1; }
                    else if (key == 1001 || key == 'd') { op_palette_idx++; if (op_palette_idx >= op_palette_count) op_palette_idx = 0; }
                    else if (key == '1') {
                        if (script_stack_count < MAX_OPS) strncpy(script_stack[script_stack_count++], op_palette[op_palette_idx], 127);
                    }
                    else if (key == '2') {
                        if (script_stack_count > 0) script_stack_count--;
                    }
                }
                last_pos = ftell(hf);
                fclose(hf);
                write_pal_editor_state();
            }
        }
        usleep(16667);
    }
    return 0;
}
