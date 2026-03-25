#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>

// TPM CHTPM Parser (v3.6 - PROJECT LOADER FIX)
// Responsibility: 100% Warning-free, app-aware routing and variables.

#define MAX_LINE 4096
#define MAX_VAR_NAME 64
#define MAX_VAR_VALUE 65536
#define MAX_VARS 300
#define MAX_ELEMENTS 400
#define MAX_BUFFER 1048576
#define MAX_PATH 1024
#define MAX_CHILDREN 100
#define MAX_LABEL_LEN 65536
#define MAX_ATTR_LEN 1024

enum editorKey {
    ARROW_LEFT = 1000, ARROW_RIGHT = 1001, ARROW_UP = 1002, ARROW_DOWN = 1003, ESC_KEY = 27,
    JOY_BUTTON_0 = 2000, JOY_BUTTON_1 = 2001, JOY_BUTTON_2 = 2002, JOY_BUTTON_3 = 2003,
    JOY_BUTTON_4 = 2004, JOY_BUTTON_5 = 2005, JOY_BUTTON_6 = 2006, JOY_BUTTON_7 = 2007,
    JOY_BUTTON_8 = 2008, JOY_LEFT = 2100, JOY_RIGHT = 2101, JOY_UP = 2102, JOY_DOWN = 2103
};

typedef struct {
    char type[32]; char label[MAX_LABEL_LEN]; char href[MAX_PATH]; char onClick[128];
    char id[MAX_ATTR_LEN]; char visibility_expr[MAX_ATTR_LEN];
    char input_buffer[256];  /* For cli_io text input */
    int parent_index; int children[MAX_CHILDREN]; int num_children;
    bool visibility; int interactive_idx;
} UIElement;

char current_layout[MAX_PATH] = "pieces/chtpm/layouts/os.chtpm";
char last_active_id[64] = "";
int focus_index = 0; int active_index = -1; 
int element_count = 0; UIElement elements[MAX_ELEMENTS];
char nav_buffer[512] = {0};
bool clear_nav_on_next = false; bool wait_for_view_change = false;
bool is_time_reactive = false;

typedef struct { char name[MAX_VAR_NAME]; char value[MAX_VAR_VALUE]; } Variable;
Variable vars[MAX_VARS]; int var_count = 0;
long last_history_position = 0; 
long last_state_file_size = 0;
long last_master_pulse_size = 0;
long last_display_pulse_size = 0;
long last_view_file_size = 0;
long last_layout_file_size = 0;
pid_t current_module_pid = -1;
char current_module_path[MAX_PATH] = "";

char* scratch_substituted = NULL;
char project_root_path[MAX_PATH] = ".";
char interact_history_path[MAX_PATH] = "";  // From <interact> tag

// Digit Accumulation State
static int digit_accum = 0;  // Accumulated digit value (0 = no accumulation)

// --- Forward Declarations ---
void parse_chtm(void);
void compose_frame(void);
void load_vars(void);
void substitute_vars(const char* src, char* dst, int max_len);
void set_var(const char* name, const char* value);
const char* get_var(const char* name);
bool is_navigable(int idx);
void initialize_focus(void);
void launch_module(const char* path);
void cleanup_module(void);
char* read_file_to_string(const char* filename);
void handle_launch_command(const char* cmd);
void send_command(const char* cmd);
bool evaluate_visibility(UIElement* el);
bool is_interactive(UIElement* el);
void parse_attributes(UIElement* el, const char* attr_str);
void render_element(int idx, char* frame, int* p_current_interactive);
char* build_path_malloc(const char* rel);
char* trim_pmo(char *str);

static bool root_has_anchors(const char* root) {
    char pieces_path[MAX_PATH];
    char projects_path[MAX_PATH];
    snprintf(pieces_path, sizeof(pieces_path), "%s/pieces", root);
    snprintf(projects_path, sizeof(projects_path), "%s/projects", root);
    return access(pieces_path, F_OK) == 0 && access(projects_path, F_OK) == 0;
}

void resolve_root() {
    if (!getcwd(project_root_path, sizeof(project_root_path))) strncpy(project_root_path, ".", sizeof(project_root_path) - 1);
    project_root_path[sizeof(project_root_path) - 1] = '\0';
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[2048];
        while (fgets(line, sizeof(line), kvp)) {
            if (strncmp(line, "project_root=", 13) == 0) {
                char *v = line + 13;
                v[strcspn(v, "\n\r")] = 0;
                if (strlen(v) > 0) {
                    char candidate[MAX_PATH];
                    strncpy(candidate, v, sizeof(candidate) - 1);
                    candidate[sizeof(candidate) - 1] = '\0';
                    if (root_has_anchors(candidate)) {
                        strncpy(project_root_path, candidate, sizeof(project_root_path) - 1);
                        project_root_path[sizeof(project_root_path) - 1] = '\0';
                    }
                }
                break;
            }
        }
        fclose(kvp);
    }
}

void build_path(char* dst, size_t sz, const char* fmt, ...) {
    va_list args; va_start(args, fmt); vsnprintf(dst, sz, fmt, args); va_end(args);
}

char* trim_pmo(char *str) {
    if (!str) return NULL;
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

char* build_path_malloc(const char* rel) {
    char* p = NULL;
    if (asprintf(&p, "%s/%s", project_root_path, rel) == -1) return NULL;
    return p;
}

void cleanup_module() { 
    if (current_module_pid > 0) { 
        kill(current_module_pid, SIGTERM); 
        waitpid(current_module_pid, NULL, WNOHANG); 
        current_module_pid = -1; 
        current_module_path[0] = '\0';
    } 
}

void handle_sigint(int sig __attribute__((unused))) { 
    cleanup_module(); if (scratch_substituted) free(scratch_substituted); exit(0); 
}

void set_var(const char* name, const char* value) {
    for (int i = 0; i < var_count; i++) { 
        if (strcmp(vars[i].name, name) == 0) { 
            strncpy(vars[i].value, value, MAX_VAR_VALUE - 1); vars[i].value[MAX_VAR_VALUE-1] = '\0';
            return; 
        } 
    }
    if (var_count < MAX_VARS) { 
        strncpy(vars[var_count].name, name, MAX_VAR_NAME - 1); vars[var_count].name[MAX_VAR_NAME-1] = '\0';
        strncpy(vars[var_count].value, value, MAX_VAR_VALUE - 1); vars[var_count].value[MAX_VAR_VALUE-1] = '\0';
        var_count++; 
    }
}

const char* get_var(const char* name) { 
    for (int i = 0; i < var_count; i++) { if (strcmp(vars[i].name, name) == 0) return vars[i].value; }
    return ""; 
}

void substitute_vars(const char* src, char* dst, int max_len) {
    const char *p_src = src; char *p_dst = dst;
    while (*p_src && (p_dst - dst) < max_len - 1) {
        if (*p_src == '$' && *(p_src+1) == '{') {
            const char *end = strchr(p_src, '}');
            if (end) {
                char var_name[64]; int len = end - (p_src + 2); if (len > 63) len = 63;
                strncpy(var_name, p_src + 2, len); var_name[len] = '\0';
                const char *val = get_var(var_name);
                while (*val && (p_dst - dst) < max_len - 1) *p_dst++ = *val++;
                p_src = end + 1; continue;
            }
        }
        *p_dst++ = *p_src++;
    }
    *p_dst = '\0';
}

char* read_file_to_string(const char* filename) {
    FILE* file = fopen(filename, "r"); if (!file) return NULL;
    fseek(file, 0, SEEK_END); long length = ftell(file); fseek(file, 0, SEEK_SET);
    char* buffer = malloc(length + 1); if (!buffer) { fclose(file); return NULL; }
    size_t n = fread(buffer, 1, length, file); buffer[n] = '\0'; fclose(file); return buffer;
}

void load_state_file(const char* rel_path, const char* prefix) {
    char *path = build_path_malloc(rel_path);
    FILE *f = fopen(path, "r");
    if (f) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), f)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *var_name = NULL;
                if (prefix) asprintf(&var_name, "%s_%s", prefix, trim_pmo(line));
                else asprintf(&var_name, "%s", trim_pmo(line));
                if (var_name) { 
                    set_var(var_name, trim_pmo(eq + 1)); 
                    free(var_name); 
                }
            }
        }
        fclose(f);
    }
    free(path);
}

static char* resolve_dynamic_pdl_path(const char* active_id) {
    char *path = NULL;
    const char *project_id = get_var("project_id");

    /* 1) Project piece.pdl (canonical) */
    asprintf(&path, "%s/projects/%s/pieces/%s/piece.pdl", project_root_path, project_id, active_id);
    if (path && access(path, F_OK) == 0) return path;
    free(path);

    /* 2) Project <piece_id>.pdl (legacy variant) */
    asprintf(&path, "%s/projects/%s/pieces/%s/%s.pdl", project_root_path, project_id, active_id, active_id);
    if (path && access(path, F_OK) == 0) return path;
    free(path);

    /* 3) App pieces (User Auth pattern) */
    asprintf(&path, "%s/pieces/apps/%s/pieces/%s/piece.pdl", project_root_path, project_id, active_id);
    if (path && access(path, F_OK) == 0) return path;
    free(path);

    asprintf(&path, "%s/pieces/apps/%s/pieces/%s/%s.pdl", project_root_path, project_id, active_id, active_id);
    if (path && access(path, F_OK) == 0) return path;
    free(path);

    /* 4) App-specific (Playrm fallback) */
    asprintf(&path, "%s/pieces/apps/playrm/%s/%s.pdl", project_root_path, active_id, active_id);
    if (path && access(path, F_OK) == 0) return path;
    free(path);

    /* 4) World/global */
    asprintf(&path, "%s/pieces/world/map_01/%s/%s.pdl", project_root_path, active_id, active_id);
    if (path && access(path, F_OK) == 0) return path;
    free(path);

    asprintf(&path, "%s/pieces/%s/%s.pdl", project_root_path, active_id, active_id);
    if (path && access(path, F_OK) == 0) return path;
    free(path);
    return NULL;
}

// Count available projects for digit accumulation bounds checking
static int count_projects() {
    char *projects_dir_path = build_path_malloc("projects");
    if (!projects_dir_path) return 0;
    DIR *dir = opendir(projects_dir_path);
    if (!dir) { free(projects_dir_path); return 0; }
    
    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        char *abs_p = NULL;
        if (asprintf(&abs_p, "%s/%s", projects_dir_path, entry->d_name) == -1) continue;
        struct stat st;
        if (stat(abs_p, &st) == 0 && S_ISDIR(st.st_mode)) {
            count++;
        }
        free(abs_p);
    }
    closedir(dir);
    free(projects_dir_path);
    return count;
}

void load_dynamic_methods(const char* active_id) {
    char *path = NULL, *methods_buf = malloc(MAX_VAR_VALUE);
    if (!methods_buf) return;
    methods_buf[0] = '\0';
    path = resolve_dynamic_pdl_path(active_id);
    
    FILE *f = fopen(path, "r");
    if (!f) { 
        free(path); set_var("piece_methods", "[No Methods]"); free(methods_buf);
        return; 
    }
    char line[MAX_LINE]; int method_idx = 2;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "METHOD", 6) == 0) {
            char *key_start = strchr(line, '|'); if (!key_start) continue;
            key_start++; char *val_start = strchr(key_start, '|'); if (!val_start) continue;
            
            // Get command value FIRST, then null val_start
            char *cmd_val = trim_pmo(val_start + 1);
            *val_start = '\0';
            char *trimmed_key = trim_pmo(key_start);
            
            if (strcmp(trimmed_key, "move") == 0 || strcmp(trimmed_key, "select") == 0 || 
                strcmp(trimmed_key, "interact") == 0 || strcmp(trimmed_key, "stat_decay") == 0 ||
                strcmp(trimmed_key, "on_turn_end") == 0 || trimmed_key[0] == '_') continue;
            
            char *btn = NULL;
            if (strcmp(cmd_val, "void") == 0) {
                asprintf(&btn, "<button label=\"%s\" onClick=\"KEY:%d\" /><br/>", trimmed_key, method_idx++);
            } else {
                asprintf(&btn, "<button label=\"%s\" onClick=\"%s\" /><br/>", trimmed_key, cmd_val);
            }

            if (btn) {
                if (strlen(methods_buf) + strlen(btn) < MAX_VAR_VALUE - 100) strcat(methods_buf, btn);
                free(btn);
            }
        }
    }
    fclose(f); free(path);
    if (strlen(methods_buf) == 0) set_var("piece_methods", "[No Methods]");
    else set_var("piece_methods", methods_buf);
    free(methods_buf);
}

void load_projects_markup() {
    char *projects_dir_path = build_path_malloc("projects");
    DIR *dir = opendir(projects_dir_path);
    if (!dir) { free(projects_dir_path); set_var("project_list", "║ [Error: No Projects Dir] ║"); return; }

    char *markup = malloc(MAX_VAR_VALUE);
    if (!markup) { closedir(dir); free(projects_dir_path); return; }
    markup[0] = '\0';
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        char *abs_p = NULL;
        if (asprintf(&abs_p, "%s/%s", projects_dir_path, entry->d_name) == -1) continue;
        struct stat st;
        if (stat(abs_p, &st) == 0 && S_ISDIR(st.st_mode)) {
            char *btn = NULL;
            if (asprintf(&btn, "<text label=\"║  \" /><button label=\"%s\" onClick=\"LOAD_PROJECT:%s\" /><text label=\"                               ║\" /><br/>", entry->d_name, entry->d_name) != -1) {
                if (strlen(markup) + strlen(btn) < MAX_VAR_VALUE - 100) strcat(markup, btn);
                free(btn);
            }
        }
        free(abs_p);
    }
    closedir(dir); free(projects_dir_path);
    if (strlen(markup) == 0) set_var("project_list", "║ [No Projects Found] ║");
    else set_var("project_list", markup);
    free(markup);
}
bool is_modern_layout(const char* layout) {
    if (!layout) return false;
    return (strstr(layout, "playrm") != NULL || strstr(layout, "player") != NULL || 
            strstr(layout, "editor") != NULL || strstr(layout, "man-add") != NULL || 
            strstr(layout, "man-ops") != NULL || strstr(layout, "man-pal") != NULL ||
            strstr(layout, "fuzz-op") != NULL || strstr(layout, "op-ed") != NULL ||
            strstr(layout, "desktop") != NULL || strstr(layout, "user") != NULL ||
            strstr(layout, "ai-labs") != NULL || strstr(layout, "p2p-net") != NULL ||
            strstr(layout, "lsr") != NULL || strstr(layout, "blank") != NULL);
}

void load_vars() {
    // PRIORITY 2: Contextual Focus Loading
    if (is_modern_layout(current_layout)) {
        load_state_file("pieces/apps/player_app/state.txt", NULL);
        load_state_file("pieces/apps/player_app/manager/state.txt", NULL);
    } else {
        load_state_file("pieces/apps/fuzzpet_app/manager/state.txt", NULL);
    }
    
    // APP LAYOUT DETECTION (Pitfall #31: prevent stale project_id from previous session)
    // MUST be AFTER load_state_file to override any stale project_id in state.txt
    if (strstr(current_layout, "pieces/apps/user/")) {
        set_var("project_id", "user");
    } else if (strstr(current_layout, "pieces/apps/op-ed/")) {
        set_var("project_id", "op-ed");
    } else if (strstr(current_layout, "pieces/apps/man-ops/")) {
        set_var("project_id", "man-ops");
    } else if (strstr(current_layout, "pieces/apps/man-pal/")) {
        set_var("project_id", "man-pal");
    } else if (strstr(current_layout, "pieces/apps/man-add/")) {
        set_var("project_id", "man-add");
    } else if (strstr(current_layout, "pieces/apps/editor/")) {
        set_var("project_id", "editor");
    }
    // Note: PROJECT layouts (projects/*/layouts/*.chtpm) keep their project_id from LOAD_PROJECT command
    
    const char* active_id = get_var("active_target_id");
    if (strlen(active_id) > 0) {
        load_dynamic_methods(active_id);
        if (strstr(active_id, "pet") != NULL) set_var("pet_active", "true"); else set_var("pet_active", "false");
        if (strcmp(active_id, "selector") == 0) set_var("selector_active", "true"); else set_var("selector_active", "false");
    }
    
    load_state_file("pieces/apps/fuzzpet_app/fuzzpet/state.txt", "fuzzpet");
    load_state_file("pieces/system/clock_daemon/state.txt", "clock");
    
    // Set dynamic module path based on project
    const char* proj_id = get_var("project_id");
    if (strcmp(proj_id, "op-ed") == 0) {
        set_var("module_path", "pieces/apps/op-ed/plugins/+x/op-ed_module.+x");
        set_var("active_layout_id", "op-ed.chtpm");
    } else if (strcmp(proj_id, "man-ops") == 0) {
        set_var("module_path", "pieces/apps/man-ops/plugins/+x/man-ops_module.+x");
        set_var("active_layout_id", "man-ops.chtpm");
    } else if (strcmp(proj_id, "man-pal") == 0) {
        set_var("module_path", "pieces/apps/playrm/plugins/+x/playrm_module.+x");
        set_var("active_layout_id", "man-pal.chtpm");
        load_state_file("projects/man-pal/pieces/hero/state.txt", "hero");
        load_state_file("projects/man-pal/pieces/clock/state.txt", "clock");
        set_var("app_title", "MAN-PAL ORCHESTRATOR");
    } else if (strcmp(proj_id, "man-add") == 0) {
        set_var("module_path", "pieces/apps/man-add/plugins/+x/man-add_module.+x");
        set_var("active_layout_id", "man-add.chtpm");
    } else if (strcmp(proj_id, "fuzz-op") == 0) {
        set_var("module_path", "projects/fuzz-op/manager/+x/fuzz-op_manager.+x");
        set_var("active_layout_id", "fuzz-op.chtpm");
        set_var("app_title", "LEGACY FUZZPET PROJECT");
        
        /* Load stats from actual active target (e.g. pet_01 or selector) */
        char *active_path = NULL;
        asprintf(&active_path, "projects/%s/pieces/%s/state.txt", proj_id, active_id);
        load_state_file(active_path, "pet");
        free(active_path);
        
        /* Fallback: Load fuzzpet piece state if active is selector */
        if (strcmp(active_id, "selector") == 0) {
            asprintf(&active_path, "projects/%s/pieces/fuzzpet/state.txt", proj_id);
            load_state_file(active_path, "pet");
            free(active_path);
        }

        set_var("pet_name", get_var("pet_name"));
        set_var("pet_status", get_var("pet_status"));
        set_var("turn_count", get_var("clock_turn"));
        set_var("game_time", get_var("clock_time"));
        set_var("hunger", get_var("pet_hunger"));
        set_var("happiness", get_var("pet_happiness"));
        set_var("energy", get_var("pet_energy"));
        set_var("level", get_var("pet_level"));
        set_var("active_target", get_var("active_target_id"));
    } else if (strcmp(proj_id, "gl-os") == 0) {
        set_var("module_path", "projects/gl-os/manager/+x/gl_os_manager.+x");
        set_var("active_layout_id", "desktop.chtpm");
    } else if (strcmp(proj_id, "user") == 0) {
        set_var("module_path", "pieces/apps/user/plugins/+x/user_module.+x");
        set_var("active_layout_id", "user.chtpm");
        /* Clear fuzzpet variables that might persist from previous session */
        set_var("pet_name", "");
        set_var("pet_status", "");
        set_var("hunger", "");
        set_var("happiness", "");
        set_var("energy", "");
        set_var("level", "");
        set_var("turn_count", "");
        set_var("game_time", "");
        set_var("game_map", "");
        
        /* Load cli_prompt from state file */
        load_state_file("pieces/apps/player_app/state.txt", NULL);
        
        /* Auto-focus CLI field for user input */
        const char* cli_prompt = get_var("cli_prompt");
        if (strcmp(cli_prompt, "username") == 0 || strcmp(cli_prompt, "password") == 0) {
            /* Find cli_io element and set active */
            for (int i = 0; i < element_count; i++) {
                if (strcmp(elements[i].type, "cli_io") == 0) {
                    active_index = i;
                    break;
                }
            }
        }
    } else if (strcmp(proj_id, "ai-labs") == 0) {
        set_var("module_path", "projects/ai-labs/manager/+x/ai_orchestrator.+x");
        set_var("active_layout_id", "ai-labs.chtpm");
    } else if (strcmp(proj_id, "p2p-net") == 0) {
        set_var("module_path", "projects/p2p-net/manager/+x/p2p_manager.+x");
        set_var("active_layout_id", "p2p-net.chtpm");
    } else if (strcmp(proj_id, "lsr") == 0) {
        set_var("module_path", "projects/lsr/manager/+x/lsr_manager.+x");
        set_var("active_layout_id", "lsr.chtpm");
    } else if (strcmp(proj_id, "blank") == 0) {
        set_var("module_path", "projects/blank/manager/+x/blank_manager.+x");
        set_var("active_layout_id", "blank.chtpm");
    } else {
        set_var("module_path", "pieces/apps/playrm/plugins/+x/playrm_module.+x");
        set_var("active_layout_id", "playrm/game.chtpm");
    }
    
    // Load last response
    char *resp_path = build_path_malloc("pieces/apps/fuzzpet_app/fuzzpet/last_response.txt");
    if (strcmp(proj_id, "fuzz-op") == 0) {
        free(resp_path);
        resp_path = build_path_malloc("projects/fuzz-op/pieces/fuzzpet/last_response.txt");
    }
    FILE *rf = fopen(resp_path, "r");
    if (rf) {
        char resp_buf[MAX_LINE];
        if (fgets(resp_buf, sizeof(resp_buf), rf)) set_var("last_response", trim_pmo(resp_buf));
        fclose(rf);
    }
    free(resp_path);
    
    load_state_file("pieces/world/map_01/selector/state.txt", "selector");
    load_state_file("pieces/world/map_01/player/state.txt", "player");

    char *view_path = NULL;
    if (is_modern_layout(current_layout))
        view_path = build_path_malloc("pieces/apps/player_app/view.txt");
    else
        view_path = build_path_malloc("pieces/apps/fuzzpet_app/fuzzpet/view.txt");

    FILE *vf = fopen(view_path, "r");
    if (vf) {
        char *map_buf = malloc(MAX_VAR_VALUE); if (!map_buf) { free(view_path); return; }
        map_buf[0] = '\0'; char line[MAX_LINE];
        while (fgets(line, sizeof(line), vf)) { if (strlen(map_buf) + strlen(line) < MAX_VAR_VALUE - 1) strcat(map_buf, line); }
        set_var("game_map", map_buf); fclose(vf); free(map_buf);
    } else {
        set_var("game_map", "[Map Loading...]");
    }
    free(view_path);
}

void inject_raw_key(int code) {
    FILE *fp;
    
    // PRIORITY 1: <interact> tag specifies path (HIGHEST PRIORITY)
    if (interact_history_path[0] != '\0') {
        char *full_path = build_path_malloc(interact_history_path);
        fp = fopen(full_path, "a");
        if (fp) { fprintf(fp, "%d\n", code); fclose(fp); }
        free(full_path);
        return;
    }
    
    const char* project_id = get_var("project_id");

    // PRIORITY 2: Project-specific history
    if (strlen(project_id) > 0 && strcmp(project_id, "template") != 0) {
        char *path = NULL;
        if (asprintf(&path, "%s/projects/%s/history.txt", project_root_path, project_id) != -1) {
            fp = fopen(path, "a");
            if (fp) { fprintf(fp, "%d\n", code); fclose(fp); }
            free(path);
            return;
        }
    }
    
    // PRIORITY 3: Layout-based fallback
    else if (is_modern_layout(current_layout)) {
        char *path = build_path_malloc("pieces/apps/player_app/history.txt");
        fp = fopen(path, "a");
        if (fp) { fprintf(fp, "%d\n", code); fclose(fp); }
        free(path);
        return;
    }
    
    // PRIORITY 4: Legacy fuzzpet fallback
    else {
        char *path = build_path_malloc("pieces/apps/fuzzpet_app/fuzzpet/history.txt");
        fp = fopen(path, "a");
        if (fp) { fprintf(fp, "%d\n", code); fclose(fp); }
        free(path);
    }
}

void launch_module(const char* launch_str) { 
    // DEBUG: Log module launch attempt
    {
        FILE *dbg = fopen("debug.txt", "a");
        if (dbg) {
            time_t t; struct tm *tm;
            time(&t); tm = localtime(&t);
            fprintf(dbg, "[%02d:%02d:%02d] PARSER: launch_module(%s) pid=%d\n",
                    tm->tm_hour, tm->tm_min, tm->tm_sec, launch_str, current_module_pid);
            fclose(dbg);
        }
    }

    if (strcmp(launch_str, current_module_path) == 0 && current_module_pid > 0) return;
    cleanup_module();
    strncpy(current_module_path, launch_str, MAX_PATH - 1);
    
    char* full_cmd = strdup(launch_str); char* args[16]; int arg_count = 0;
    char* token = strtok(full_cmd, " ");
    while (token && arg_count < 15) {
        if (arg_count == 0) args[arg_count++] = (token[0] == '/') ? strdup(token) : build_path_malloc(token);
        else args[arg_count++] = strdup(token);
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL;

    current_module_pid = fork(); 
    if (current_module_pid == 0) { 
        if (project_root_path[0] != '\0') chdir(project_root_path);
        execv(args[0], args); exit(1); 
    }
    else if (current_module_pid > 0) {
        // DEBUG: Log successful fork
        {
            FILE *dbg = fopen("debug.txt", "a");
            if (dbg) {
                time_t t; struct tm *tm;
                time(&t); tm = localtime(&t);
                fprintf(dbg, "[%02d:%02d:%02d] PARSER: forked module pid=%d\n",
                        tm->tm_hour, tm->tm_min, tm->tm_sec, current_module_pid);
                fclose(dbg);
            }
        }
        char* pm = build_path_malloc("pieces/os/plugins/+x/proc_manager.+x");
        char* module_name = NULL;
        
        if (strstr(args[0], "projects/") != NULL) {
            char project_part[64];
            const char* start = strstr(args[0], "projects/") + 9;
            const char* end = strchr(start, '/');
            if (end) {
                int len = (int)(end - start);
                if (len > 63) len = 63;
                strncpy(project_part, start, len); project_part[len] = '\0';
                const char* base = strrchr(args[0], '/');
                if (asprintf(&module_name, "%s_%s", project_part, base ? base + 1 : args[0]) == -1) { free(pm); return; }
            } else {
                const char* base = strrchr(args[0], '/');
                module_name = strdup(base ? base + 1 : args[0]);
            }
        } else {
            const char* base = strrchr(args[0], '/');
            module_name = strdup(base ? base + 1 : args[0]);
        }
        
        if (module_name) {
            char* dot = strchr(module_name, '.'); if (dot) *dot = '\0';
            
            pid_t p = fork();
            if (p == 0) {
                char *pid_str = NULL;
                if (asprintf(&pid_str, "%d", current_module_pid) != -1) {
                    execl(pm, pm, "register", module_name, pid_str, NULL);
                    free(pid_str);
                }
                exit(1);
            } else { waitpid(p, NULL, 0); }
            free(module_name);
        }
        free(pm);
    }
    for (int i = 0; i < arg_count; i++) free(args[i]);
    free(full_cmd);
}

void handle_launch_command(const char* cmd) {
    char app_name[64]; strncpy(app_name, cmd + 7, sizeof(app_name)-1); app_name[sizeof(app_name)-1] = '\0';
    char path_list_path[MAX_PATH];
    build_path(path_list_path, sizeof(path_list_path), "%s/pieces/os/app_list.txt", project_root_path);
    FILE *f = fopen(path_list_path, "r");
    if (f) { 
        char line[MAX_LINE]; 
        while (fgets(line, sizeof(line), f)) { 
            line[strcspn(line, "\n")] = 0; char *eq = strchr(line, '='); 
            if (eq) { 
                *eq = '\0'; 
                if (strcasecmp(line, app_name) == 0) { 
                    char module_path[MAX_PATH];
                    build_path(module_path, sizeof(module_path), "%s/%s", project_root_path, trim_pmo(eq + 1));
                    launch_module(module_path); break; 
                } 
            } 
        } 
        fclose(f); 
    }
}

void send_command(const char* cmd) {
    /* Handle KEY:n prefix for injecting key codes */
    if (strncmp(cmd, "KEY:", 4) == 0) {
        int key = atoi(cmd + 4);
        inject_raw_key(key);
        return;
    }
    
    if (strncmp(cmd, "LOAD_PROJECT:", 13) == 0) {
        const char *proj_name = cmd + 13;
        char *path = build_path_malloc("pieces/apps/player_app/manager/state.txt");
        FILE *f = fopen(path, "w");
        if (f) {
            fprintf(f, "project_id=%s\n", proj_name);
            // Set default active target based on project
            // Projects that use selector:
            if (strcmp(proj_name, "fuzz-op") == 0 || strcmp(proj_name, "demo_1") == 0 ||
                strcmp(proj_name, "template") == 0 || strcmp(proj_name, "man-pal") == 0 || 
                strcmp(proj_name, "man-ops") == 0 || strcmp(proj_name, "op-ed") == 0 ||
                strcmp(proj_name, "man-add") == 0 || strcmp(proj_name, "gl-os") == 0 ||
                strcmp(proj_name, "lsr") == 0 || strcmp(proj_name, "user") == 0 ||
                strcmp(proj_name, "ai-labs") == 0 || strcmp(proj_name, "p2p-net") == 0 ||
                strcmp(proj_name, "blank") == 0) {
                fprintf(f, "active_target_id=selector\n");
            } else {
                fprintf(f, "active_target_id=hero\n");
            }
            fprintf(f, "current_z=0\n");
            fprintf(f, "last_key=None\n");
            fclose(f);
        }
        free(path);
        
        // Dynamic layout lookup from project_routes.kvp
        char route_path[MAX_PATH];
        build_path(route_path, sizeof(route_path), "%s/pieces/os/project_routes.kvp", project_root_path);
        FILE *rf = fopen(route_path, "r");
        bool found = false;
        if (rf) {
            char line[MAX_LINE];
            while (fgets(line, sizeof(line), rf)) {
                if (line[0] == '#' || strlen(trim_pmo(line)) == 0) continue;
                char *eq = strchr(line, '=');
                if (eq) {
                    *eq = '\0';
                    if (strcmp(trim_pmo(line), proj_name) == 0) {
                        char *pipe1 = strchr(eq + 1, '|');
                        if (pipe1) {
                            *pipe1 = '\0';
                            strncpy(current_layout, trim_pmo(eq + 1), MAX_PATH-1);
                            found = true;
                        }
                        break;
                    }
                }
            }
            fclose(rf);
        }
        
        if (!found) {
            strncpy(current_layout, "pieces/apps/playrm/layouts/game.chtpm", MAX_PATH-1);
        }

        parse_chtm();
        initialize_focus();
        compose_frame();  // Force immediate frame update
        return;
    }
    if (strncmp(cmd, "LAUNCH:", 7) == 0) { handle_launch_command(cmd); return; }
    if (strncmp(cmd, "KEY:", 4) == 0) {
        int k = atoi(cmd + 4); if (k >= 0 && k <= 9) inject_raw_key('0' + k); else inject_raw_key(k);
        return;
    }
    if (strcmp(cmd, "BACK") == 0 || strcmp(cmd, "RELEASE") == 0) inject_raw_key('9');
}

bool evaluate_visibility(UIElement* el) {
    if (el->visibility_expr[0] == '\0') return true;
    char substituted[512]; substitute_vars(el->visibility_expr, substituted, 512);
    char *s = trim_pmo(substituted); return (strcmp(s, "true") == 0 || strcmp(s, "1") == 0);
}

bool is_interactive(UIElement* el) { return (strcmp(el->type, "button") == 0 || strcmp(el->type, "canvas") == 0 || strcmp(el->type, "cli_io") == 0 || strcmp(el->type, "scroller") == 0); }
bool is_navigable(int idx) { 
    if (idx < 0 || idx >= element_count) return false; 
    UIElement* el = &elements[idx]; return is_interactive(el) && evaluate_visibility(el); 
}

void parse_attributes(UIElement* el, const char* attr_str) {
    if (!attr_str) return; 
    char* attrs = strdup(attr_str); char* pos = attrs;
    while (*pos) {
        while (*pos && isspace((unsigned char)*pos)) { pos++; } if (!*pos) break;
        char* name_start = pos; while (*pos && *pos != '=' && !isspace((unsigned char)*pos)) { pos++; }
        char saved = *pos; *pos = '\0'; while (*(++pos) && isspace((unsigned char)*pos));
        if (*pos == '=') { pos++; while (*pos && isspace((unsigned char)*pos)) { pos++; } }
        char* val_start = pos;
        if (*pos == '"' || *pos == '\'') { char quote = *pos++; val_start = pos; while (*pos && *pos != quote) { pos++; } if (*pos) *pos++ = '\0'; }
        else { while (*pos && !isspace((unsigned char)*pos) && *pos != '/') { pos++; } if (*pos) *pos++ = '\0'; }
        if (strcmp(name_start, "label") == 0) { strncpy(el->label, val_start, MAX_LABEL_LEN - 1); }
        else if (strcmp(name_start, "href") == 0) { strncpy(el->href, val_start, MAX_PATH - 1); }
        else if (strcmp(name_start, "onClick") == 0) { strncpy(el->onClick, val_start, 127); }
        else if (strcmp(name_start, "id") == 0) { strncpy(el->id, val_start, MAX_ATTR_LEN - 1); }
        else if (strcmp(name_start, "visibility") == 0) { strncpy(el->visibility_expr, val_start, MAX_ATTR_LEN - 1); }
        else if (strcmp(name_start, "time_reactive") == 0) { is_time_reactive = (strcmp(val_start, "true") == 0); }
        *(pos - 1) = saved;
    }
    free(attrs);
}

typedef enum { TOKEN_TEXT, TOKEN_OPEN_TAG, TOKEN_CLOSE_TAG, TOKEN_SELFCLOSE_TAG } TokenType;
typedef struct { TokenType type; char content[MAX_LABEL_LEN]; char tag_name[64]; char attributes[512]; } Token;

Token* tokenize(const char* content, int* token_count) {
    Token* tokens = malloc(MAX_ELEMENTS * 4 * sizeof(Token)); *token_count = 0; const char* cursor = content;
    while (*cursor && *token_count < MAX_ELEMENTS * 4) {
        const char* tag_start = strchr(cursor, '<');
        if (!tag_start) { 
            Token* t = &tokens[(*token_count)++]; t->type = TOKEN_TEXT; strncpy(t->content, cursor, MAX_LABEL_LEN-1); t->content[MAX_LABEL_LEN-1]='\0'; break; 
        }
        if (tag_start > cursor) { 
            Token* t = &tokens[(*token_count)++]; t->type = TOKEN_TEXT; 
            int len = tag_start - cursor; if (len > MAX_LABEL_LEN-1) len = MAX_LABEL_LEN-1; 
            strncpy(t->content, cursor, len); t->content[len] = '\0'; 
        }
        const char* tag_end = strchr(tag_start, '>'); if (!tag_end) break;
        Token* t = &tokens[(*token_count)++]; char tag_body[512]; int body_len = tag_end - tag_start - 1; if (body_len > 511) body_len = 511;
        strncpy(tag_body, tag_start + 1, body_len); tag_body[body_len] = '\0';
        if (tag_body[0] == '/') { t->type = TOKEN_CLOSE_TAG; strncpy(t->tag_name, tag_body + 1, 63); }
        else {
            bool self_closing = false; if (body_len > 0 && tag_body[body_len-1] == '/') { self_closing = true; tag_body[body_len-1] = '\0'; }
            t->type = self_closing ? TOKEN_SELFCLOSE_TAG : TOKEN_OPEN_TAG;
            char* space = strchr(tag_body, ' ');
            if (space) { *space = '\0'; strncpy(t->tag_name, tag_body, 63); strncpy(t->attributes, space + 1, 511); }
            else { strncpy(t->tag_name, tag_body, 63); t->attributes[0] = '\0'; }
        }
        cursor = tag_end + 1;
    }
    return tokens;
}

void parse_chtm() {
    is_time_reactive = false; 
    
    // EXPORT CURRENT LAYOUT FOR MODULE HEARTBEAT
    char *cl_path = build_path_malloc("pieces/display/current_layout.txt");
    FILE *cl_f = fopen(cl_path, "w");
    if (cl_f) { fprintf(cl_f, "%s\n", current_layout); fclose(cl_f); }
    free(cl_path);

    load_vars(); char* content = read_file_to_string(current_layout); if (!content) return;
    char* methods_raw = (char*)get_var("piece_methods");
    char* temp_substituted = malloc(MAX_BUFFER); char* p_src = content; char* p_dst = temp_substituted;
    while(*p_src) { 
        if(strncmp(p_src, "${piece_methods}", 16) == 0) { strcpy(p_dst, methods_raw); p_dst += strlen(methods_raw); p_src += 16; }
        else { *p_dst++ = *p_src++; }
    }
    *p_dst = '\0'; free(content);
    int tc; Token* tokens = tokenize(temp_substituted, &tc); free(temp_substituted);
    element_count = 0; int stack[50], top = -1, current_interactive = 0;
    for (int i = 0; i < tc && element_count < MAX_ELEMENTS; i++) {
        Token* t = &tokens[i];
        if (t->type == TOKEN_TEXT) {
            if (i > 0 && strcmp(tokens[i-1].tag_name, "module") == 0 && tokens[i-1].type == TOKEN_OPEN_TAG) {
                char substituted[512]; substitute_vars(t->content, substituted, 512);
                char module_path[512]; strncpy(module_path, trim_pmo(substituted), 511); module_path[511] = '\0';
                launch_module(module_path); continue;
            }
            char* trim_text = strdup(t->content); char* p_trim = trim_text; while (*p_trim && isspace((unsigned char)*p_trim)) p_trim++;
            char* end_trim = p_trim + strlen(p_trim) - 1; while (end_trim > p_trim && isspace((unsigned char)*end_trim)) *end_trim-- = '\0';
            if (!*p_trim) { free(trim_text); continue; }
            UIElement* el = &elements[element_count++]; memset(el, 0, sizeof(UIElement));
            strcpy(el->type, "text"); strncpy(el->label, p_trim, MAX_LABEL_LEN - 1); 
            el->parent_index = (top >= 0) ? stack[top] : -1; free(trim_text);
        } else if (t->type == TOKEN_OPEN_TAG || t->type == TOKEN_SELFCLOSE_TAG) {
            if (strcmp(t->tag_name, "panel") == 0) { 
                UIElement dummy; memset(&dummy, 0, sizeof(UIElement)); parse_attributes(&dummy, t->attributes); 
                if (t->type == TOKEN_OPEN_TAG) stack[++top] = -1; continue; 
            }
            
            UIElement* el = &elements[element_count++]; memset(el, 0, sizeof(UIElement));
            strncpy(el->type, t->tag_name, 31);
            
            if (strcmp(t->tag_name, "br") == 0) { 
                /* No attributes for br */
            }
            else if (strcmp(t->tag_name, "module") == 0 && t->type == TOKEN_SELFCLOSE_TAG) {
                char* path = strstr(t->attributes, "src=\"");
                if (path) {
                    path += 5; char* end = strchr(path, '"');
                    if (end) { char module_path[512]; int len = end - path; strncpy(module_path, path, len); module_path[len] = '\0'; launch_module(module_path); }
                }
            }
            else if (strcmp(t->tag_name, "interact") == 0 && t->type == TOKEN_SELFCLOSE_TAG) {
                char* path_attr = strstr(t->attributes, "src=\"");
                if (path_attr) {
                    path_attr += 5; char* end = strchr(path_attr, '"');
                    if (end) { 
                        int len = end - path_attr;
                        char temp[MAX_PATH];
                        strncpy(temp, path_attr, len);
                        temp[len] = '\0';
                        substitute_vars(temp, interact_history_path, MAX_PATH);
                    }
                }
            }
            else {
                parse_attributes(el, t->attributes);
            }

            /* Initialize cli_io input_buffer from cli_buffers.txt (last value) */
            if (strcmp(t->tag_name, "cli_io") == 0) {
                FILE *bf = fopen("pieces/apps/player_app/cli_buffers.txt", "r");
                if (bf) {
                    char line[512], last_val[256] = "";
                    char prefix = strstr(el->id, "username") ? 'U' : 'P';
                    while (fgets(line, sizeof(line), bf)) {
                        if (line[0] == prefix) {
                            char* val = line + 1;
                            val[strcspn(val, "\n")] = 0;
                            strncpy(last_val, val, sizeof(last_val) - 1);
                        }
                    }
                    if (strlen(last_val) > 0) {
                        strncpy(el->input_buffer, last_val, sizeof(el->input_buffer) - 1);
                    }
                    fclose(bf);
                }
            }

            /* Initialize cli_io input_buffer from cli_input state (TPM app signal) */
            if (strcmp(t->tag_name, "cli_io") == 0) {
                const char* cli = get_var("cli_input");
                if (cli && strlen(cli) > 0) {
                    /* Extract just the buffer part after "Username: " or "Password: " */
                    const char* colon = strchr(cli, ':');
                    if (colon) {
                        colon++; /* Skip ": " */
                        while (*colon == ' ') colon++;
                        strncpy(el->input_buffer, colon, sizeof(el->input_buffer) - 1);
                    } else {
                        strncpy(el->input_buffer, cli, sizeof(el->input_buffer) - 1);
                    }
                }
            }

            el->parent_index = (top >= 0) ? stack[top] : -1; 
            if (is_interactive(el)) el->interactive_idx = ++current_interactive;
            if (el->parent_index != -1) { 
                UIElement* pa = &elements[el->parent_index]; 
                if (pa->num_children < MAX_CHILDREN) pa->children[pa->num_children++] = element_count - 1;
            }
            if (t->type == TOKEN_OPEN_TAG) stack[++top] = element_count - 1;
        } else if (t->type == TOKEN_CLOSE_TAG) { if (top >= 0) top--; }
    }
    free(tokens);
}

void export_active_index() {
    int active_gui_idx = 0;
    if (active_index != -1) active_gui_idx = elements[active_index].interactive_idx;
    else active_gui_idx = elements[focus_index].interactive_idx;
    
    char *agi_path = build_path_malloc("pieces/display/active_gui_index.txt");
    FILE *agi_f = fopen(agi_path, "w");
    if (agi_f) { fprintf(agi_f, "%d\n", active_gui_idx); fclose(agi_f); }
    free(agi_path);
}

void render_element(int idx, char* frame, int* p_current_interactive) {
    if (idx < 0 || idx >= element_count) return; 
    UIElement* el = &elements[idx]; if (!evaluate_visibility(el)) return;
    substitute_vars(el->label, scratch_substituted, MAX_LABEL_LEN);
    if (strcmp(el->type, "br") == 0) strcat(frame, "\n");
    else if (strcmp(el->type, "text") == 0) strcat(frame, scratch_substituted);
    else if (strcmp(el->type, "row") == 0) {
        char row_buf[128];
        snprintf(row_buf, sizeof(row_buf), "║ %-57s ║", scratch_substituted);
        strcat(frame, row_buf);
    }
    else if (strcmp(el->type, "scroller") == 0) {
        (*p_current_interactive)++; char *line = NULL; 
        bool is_focused = (active_index == -1 && idx == focus_index);
        bool is_active = (idx == active_index);
        char *pref = is_active ? "[^]" : (is_focused ? "[>]" : "[ ]");
        // FIX: Use scratch_substituted (which already has vars expanded)
        asprintf(&line, "║ %s %d. %-10s: %-33s ║", pref, *p_current_interactive, el->id, scratch_substituted);
        if (line) { strcat(frame, line); free(line); }
    }
    else if (strcmp(el->type, "cli_io") == 0) {
        /* CLI input field - special rendering */
        (*p_current_interactive)++; char *line = NULL;
        bool is_focused = (active_index == -1 && idx == focus_index);
        bool is_active = (idx == active_index);
        
        /* Check if password masking is needed */
        const char* cli_prompt = get_var("cli_prompt");
        int is_password = (strcmp(cli_prompt, "password") == 0);
        
        if (is_active) {
            /* Active text input mode */
            if (is_password) {
                /* Show masked */
                asprintf(&line, "║ [^] %d. [", *p_current_interactive);
                for (int i = 0; el->input_buffer[i]; i++) strcat(line, "*");
                strcat(line, "_]                                              ║");
            } else {
                /* Show plaintext */
                asprintf(&line, "║ [^] %d. [%s%s_]                                              ║", *p_current_interactive, scratch_substituted, el->input_buffer);
            }
        } else if (is_focused) {
            /* Just focused - use input_buffer if has content, else layout var */
            const char* display_val = (strlen(el->input_buffer) > 0) ? el->input_buffer : scratch_substituted;
            asprintf(&line, "║ [>] %d. [%s]                                                 ║", *p_current_interactive, display_val);
        } else {
            /* Not focused - use input_buffer if has content, else layout var */
            const char* display_val = (strlen(el->input_buffer) > 0) ? el->input_buffer : scratch_substituted;
            asprintf(&line, "║ [ ] %d. [%s]                                                 ║", *p_current_interactive, display_val);
        }
        if (line) { strcat(frame, line); free(line); }
    }
    else if (is_interactive(el)) {
        (*p_current_interactive)++; char *line = NULL; 
        bool is_focused = (active_index == -1 && idx == focus_index);
        if (idx == active_index) asprintf(&line, "[^] %d. [%s]", *p_current_interactive, scratch_substituted);
        else if (is_focused) asprintf(&line, "[>] %d. [%s]", *p_current_interactive, scratch_substituted);
        else asprintf(&line, "[ ] %d. [%s]", *p_current_interactive, scratch_substituted);
        if (line) { strcat(frame, line); free(line); }
    }
    for (int i = 0; i < el->num_children; i++) render_element(el->children[i], frame, p_current_interactive);
}

void compose_frame() {
    load_vars();
    const char* active_id = get_var("active_target_id");
    if (strcmp(active_id, last_active_id) != 0) { strncpy(last_active_id, active_id, 63); parse_chtm(); initialize_focus(); }
    
    export_active_index();

    char *frame = malloc(MAX_BUFFER); if (!frame) return; memset(frame, 0, MAX_BUFFER);
    int current_interactive = 0; for (int i = 0; i < element_count; i++) if (elements[i].parent_index == -1) render_element(i, frame, &current_interactive);
    strcat(frame, "\n╚═══════════════════════════════════════════════════════════╝\n");
    char *nav_msg = NULL;
    if (active_index == -1) asprintf(&nav_msg, "Nav > %s_", nav_buffer);
    else asprintf(&nav_msg, "Active [^]: %s (ESC to exit)", nav_buffer);
    if (nav_msg) { strcat(frame, nav_msg); strcat(frame, "\n"); free(nav_msg); }
    
    char* cur_f = build_path_malloc("pieces/display/current_frame.txt");
    FILE *out_f = fopen(cur_f, "w");
    if (out_f) {
        fprintf(out_f, "%s", frame); fclose(out_f);
        char* renderer_pulse = build_path_malloc("pieces/display/renderer_pulse.txt");
        FILE *marker = fopen(renderer_pulse, "a"); if (marker) { fprintf(marker, "P\n"); fclose(marker); }
        free(renderer_pulse);
    }
    free(cur_f); free(frame); if (clear_nav_on_next) { nav_buffer[0] = '\0'; clear_nav_on_next = false; }
}

bool do_jump(int target_num) { int cn = 0; for (int i = 0; i < element_count; i++) { if (is_navigable(i)) { cn++; if (cn == target_num) { focus_index = i; return true; } } } return false; }
void initialize_focus() { if (element_count > 0) { for (int i = 0; i < element_count; i++) { if (is_navigable(i)) { focus_index = i; return; } } } }

void process_key(int key) {
    /* Normal nav mode */
    if (active_index == -1) {
        if (key == ARROW_UP || key == 'w' || key == 'W' || key == JOY_UP) { 
            int prev = focus_index; do { focus_index--; if (focus_index < 0) focus_index = element_count-1; } while (focus_index != prev && !is_navigable(focus_index)); 
            digit_accum = 0;  // Reset accumulator on navigation
            clear_nav_on_next = true; export_active_index();
        }
        else if (key == ARROW_DOWN || key == 's' || key == 'S' || key == JOY_DOWN) { 
            int prev = focus_index; do { focus_index++; if (focus_index >= element_count) focus_index = 0; } while (focus_index != prev && !is_navigable(focus_index)); 
            digit_accum = 0;  // Reset accumulator on navigation
            clear_nav_on_next = true; export_active_index();
        }
        else if (isdigit(key)) { 
            int d = key - '0';
            int max_projects = count_projects();
            
            if (digit_accum == 0) {
                // First digit
                digit_accum = d;
                // If single digit already exceeds max or d==0, execute immediately
                if (d == 0 || d > max_projects) {
                    // Use single digit immediately (0 or invalid)
                    digit_accum = 0;
                } else {
                    // Check if d*10 could still be valid
                    if (d * 10 > max_projects) {
                        // Can't extend, execute immediately
                        if (do_jump(d)) {
                            nav_buffer[0] = (char)key;
                            nav_buffer[1] = 0;
                            clear_nav_on_next = true;
                            export_active_index();
                        }
                        digit_accum = 0;
                    }
                    // else: wait for next digit
                }
            } else {
                // Accumulating: try to append digit
                int new_val = digit_accum * 10 + d;
                if (new_val > max_projects) {
                    // Overflow: execute current accum, then use new digit
                    if (do_jump(digit_accum)) {
                        nav_buffer[0] = (char)('0' + digit_accum);
                        nav_buffer[1] = 0;
                        clear_nav_on_next = true;
                        export_active_index();
                    }
                    digit_accum = 0;
                    // Now process new digit as first digit
                    if (d > 0 && d <= max_projects) {
                        if (d * 10 > max_projects) {
                            if (do_jump(d)) {
                                nav_buffer[0] = (char)key;
                                nav_buffer[1] = 0;
                                clear_nav_on_next = true;
                                export_active_index();
                            }
                        } else {
                            digit_accum = d;
                        }
                    }
                } else {
                    // Valid accumulation
                    digit_accum = new_val;
                    // Check if we can still extend
                    if (digit_accum * 10 > max_projects) {
                        // Can't extend further, execute now
                        if (do_jump(digit_accum)) {
                            char buf[16];
                            snprintf(buf, sizeof(buf), "%d", digit_accum);
                            strncpy(nav_buffer, buf, sizeof(nav_buffer) - 1);
                            clear_nav_on_next = true;
                            export_active_index();
                        }
                        digit_accum = 0;
                    }
                    // else: wait for more digits
                }
            }
        }
        else if (key == 10 || key == 13 || key == JOY_BUTTON_0) {
            // Execute accumulated value on Enter
            if (digit_accum > 0) {
                if (do_jump(digit_accum)) {
                    char buf[16];
                    snprintf(buf, sizeof(buf), "%d", digit_accum);
                    strncpy(nav_buffer, buf, sizeof(nav_buffer) - 1);
                    clear_nav_on_next = true;
                    export_active_index();
                }
                digit_accum = 0;
            } else if (focus_index >= 0 && focus_index < element_count) {
                UIElement *el = &elements[focus_index];
                if (strcmp(el->type, "cli_io") == 0) {
                    /* CLI input field - activate on Enter */
                    active_index = focus_index;
                    clear_nav_on_next = true;
                    export_active_index();
                }
                else if (strlen(el->href) > 0) { strncpy(current_layout, el->href, MAX_PATH-1); parse_chtm(); focus_index = 0; initialize_focus(); export_active_index(); }
                else if (strcmp(el->onClick, "INTERACT") == 0) { active_index = focus_index; clear_nav_on_next = true; export_active_index(); }
                else if (strlen(el->onClick) > 0) { send_command(el->onClick); wait_for_view_change = true; clear_nav_on_next = true; }
            }
        } else if (key == 'q' || key == 'Q') exit(0);
        else {
            // Non-digit key resets accumulator
            digit_accum = 0;
        }
    } else {
        /* Active mode - copy reference pattern exactly */
        UIElement *el = &elements[active_index];
        
        /* KISS: ESC just deactivates, NEVER clears input */
        if (key == ESC_KEY || key == JOY_BUTTON_8) { 
            active_index = -1; 
            /* Do NOT clear el->input_buffer - preserve for resumption */
        }
        else if (strcmp(el->type, "cli_io") == 0) {
            /* CLI text input - use element's input_buffer like reference */
            if (key == 10 || key == 13) { 
                /* Enter - just deactivate, don't inject (keys already captured) */
                active_index = -1;
            }
            else if (key == 127 || key == 8) { 
                /* Backspace */
                int len = strlen(el->input_buffer); 
                if (len > 0) el->input_buffer[len-1] = '\0';
            }
            else if (key >= 32 && key <= 126) { 
                /* Printable char */
                int len = strlen(el->input_buffer); 
                if (len < sizeof(el->input_buffer) - 2) { 
                    el->input_buffer[len] = (char)key; 
                    el->input_buffer[len+1] = '\0';
                    /* Append to cli_buffers.txt - simple and safe */
                    FILE *bf = fopen("pieces/apps/player_app/cli_buffers.txt", "a");
                    if (bf) {
                        if (strstr(el->id, "username")) {
                            fprintf(bf, "U%s\n", el->input_buffer);
                        } else if (strstr(el->id, "password")) {
                            char masked[256] = "";
                            for (int i = 0; i < len && i < 20; i++) strcat(masked, "*");
                            fprintf(bf, "P%s\n", masked);
                        }
                        fclose(bf);
                    }
                } 
            }
        }
        else if (strcmp(el->onClick, "INTERACT") == 0) {
            int eff = key;
            if (key == ARROW_LEFT) eff = 1000; else if (key == ARROW_RIGHT) eff = 1001; else if (key == ARROW_UP) eff = 1002; else if (key == ARROW_DOWN) eff = 1003;
            else if (key >= JOY_BUTTON_0 && key <= JOY_BUTTON_8) eff = 2000 + (key - JOY_BUTTON_0);
            inject_raw_key(eff); if (key >= 32 && key <= 126) { nav_buffer[0] = (char)key; nav_buffer[1] = 0; } else if (key == 10 || key == 13) strcpy(nav_buffer, "ENTER");
            clear_nav_on_next = true;
        }
    }
}

int main(int argc, char **argv) {
    resolve_root(); scratch_substituted = malloc(MAX_LABEL_LEN); if (argc > 1) strncpy(current_layout, argv[1], MAX_PATH-1);
    signal(SIGINT, handle_sigint); parse_chtm(); initialize_focus(); compose_frame();
    struct stat st; 
    char *master_frame_ch = build_path_malloc("pieces/master_ledger/frame_changed.txt");
    char *display_frame_ch = build_path_malloc("pieces/display/frame_changed.txt");
    char *view_ch = build_path_malloc("pieces/apps/fuzzpet_app/fuzzpet/view_changed.txt");
    char *hist_p = build_path_malloc("pieces/keyboard/history.txt");
    char *layout_ch = build_path_malloc("pieces/display/layout_changed.txt");
    char *state_ch = build_path_malloc("pieces/apps/player_app/state_changed.txt");
    
    if (stat(master_frame_ch, &st) == 0) last_master_pulse_size = st.st_size;
    if (stat(display_frame_ch, &st) == 0) last_display_pulse_size = st.st_size;
    if (stat(view_ch, &st) == 0) last_view_file_size = st.st_size;
    if (stat(layout_ch, &st) == 0) last_layout_file_size = st.st_size;
    if (stat(state_ch, &st) == 0) last_state_file_size = st.st_size;

    while (1) {
        int dirty = 0; if (current_module_pid > 0) { int status; if (waitpid(current_module_pid, &status, WNOHANG) != 0) { current_module_pid = -1; dirty = 1; } }
        FILE *history = fopen(hist_p, "r");
        if (history) { fseek(history, last_history_position, SEEK_SET); char line[200]; while (fgets(line, sizeof(line), history)) { if (strstr(line, "KEY_PRESSED: ")) { int key = atoi(strstr(line, "KEY_PRESSED: ") + 13); if (key > 0) { process_key(key); dirty = 1; } } } last_history_position = ftell(history); fclose(history); }
        if (stat(master_frame_ch, &st) == 0 && st.st_size > last_master_pulse_size) { last_master_pulse_size = st.st_size; if (is_time_reactive) dirty = 1; }
        if (stat(display_frame_ch, &st) == 0 && st.st_size > last_display_pulse_size) { last_display_pulse_size = st.st_size; dirty = 1; }
        if (stat(view_ch, &st) == 0 && st.st_size > last_view_file_size) { last_view_file_size = st.st_size; dirty = 1; wait_for_view_change = false; }
        if (stat(state_ch, &st) == 0 && st.st_size > last_state_file_size) {
            last_state_file_size = st.st_size;
            var_count = 0;
            load_vars();
            const char* new_active_id = get_var("active_target_id");
            if (strcmp(new_active_id, last_active_id) != 0) {
                strncpy(last_active_id, new_active_id, 63);
                parse_chtm();
                initialize_focus();
            }
            /* Auto-activate cli_io if cli_input has content (TPM app signal) */
            const char* cli = get_var("cli_input");
            if (cli && strlen(cli) > 0 && active_index == -1) {
                for (int i = 0; i < element_count; i++) {
                    if (strcmp(elements[i].type, "cli_io") == 0) {
                        active_index = i;
                        break;
                    }
                }
            }
            dirty = 1;
        }
        if (stat(layout_ch, &st) == 0 && st.st_size > last_layout_file_size) {
            last_layout_file_size = st.st_size; FILE *lf = fopen(layout_ch, "r");
            if (lf) { 
                char line[MAX_PATH]; char last_line[MAX_PATH] = "";
                while (fgets(line, sizeof(line), lf)) {
                    if (strlen(trim_pmo(line)) > 0) strcpy(last_line, trim_pmo(line));
                }
                if (strlen(last_line) > 0) {
                    strncpy(current_layout, last_line, MAX_PATH-1);
                    parse_chtm(); initialize_focus(); dirty = 1;
                }
                fclose(lf); 
            }
        }
        if (dirty || clear_nav_on_next) { compose_frame(); dirty = 0; } usleep(16667);
    }
    return 0;
}
