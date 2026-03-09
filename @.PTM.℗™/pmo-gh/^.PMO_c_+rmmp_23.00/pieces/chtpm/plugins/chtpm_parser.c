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
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>

// TPM CHTPM Parser (v3.1 - PREFIX & WARNING FIX)
// Responsibility: 100% Warning-free, hierarchical rendering, correct prefixing.

#define MAX_LINE 4096
#define MAX_VAR_NAME 64
#define MAX_VAR_VALUE 65536
#define MAX_VARS 250
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
long last_master_pulse_size = 0;
long last_display_pulse_size = 0;
long last_view_file_size = 0;
long last_layout_file_size = 0;
pid_t current_module_pid = -1;

char* scratch_substituted = NULL;
char project_root_path[MAX_PATH] = ".";

// Forward Decls
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
bool do_jump(int target_num);
char* build_path_malloc(const char* rel);
char* trim_pmo(char *str);

// --- Utilities ---

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
    asprintf(&p, "%s/%s", project_root_path, rel);
    return p;
}

void resolve_root() {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[4096];
        while (fgets(line, sizeof(line), kvp)) {
            if (strncmp(line, "project_root=", 13) == 0) {
                char *v = line + 13;
                strncpy(project_root_path, trim_pmo(v), MAX_PATH - 1); 
                project_root_path[MAX_PATH-1] = '\0';
                break;
            }
        }
        fclose(kvp);
    }
}

void cleanup_module() { 
    if (current_module_pid > 0) { 
        kill(current_module_pid, SIGTERM); 
        waitpid(current_module_pid, NULL, WNOHANG); 
        current_module_pid = -1; 
    } 
}

void handle_sigint(int sig __attribute__((unused))) { 
    cleanup_module(); 
    if (scratch_substituted) free(scratch_substituted); 
    exit(0); 
}

void set_var(const char* name, const char* value) {
    for (int i = 0; i < var_count; i++) { 
        if (strcmp(vars[i].name, name) == 0) { 
            strncpy(vars[i].value, value, MAX_VAR_VALUE - 1); 
            vars[i].value[MAX_VAR_VALUE-1] = '\0';
            return; 
        } 
    }
    if (var_count < MAX_VARS) { 
        strncpy(vars[var_count].name, name, MAX_VAR_NAME - 1); 
        vars[var_count].name[MAX_VAR_NAME-1] = '\0';
        strncpy(vars[var_count].value, value, MAX_VAR_VALUE - 1); 
        vars[var_count].value[MAX_VAR_VALUE-1] = '\0';
        var_count++; 
    }
}

const char* get_var(const char* name) { 
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i].name, name) == 0) return vars[i].value; 
    }
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

void load_dynamic_methods(const char* active_id) {
    char *path = NULL, *methods_buf = malloc(MAX_VAR_VALUE);
    if (!methods_buf) return;
    methods_buf[0] = '\0';
    
    asprintf(&path, "%s/pieces/world/map_01/%s/%s.pdl", project_root_path, active_id, active_id);
    if (access(path, F_OK) != 0) {
        free(path);
        asprintf(&path, "%s/pieces/%s/%s.pdl", project_root_path, active_id, active_id);
    }
    FILE *f = fopen(path, "r");
    if (!f) { 
        free(path); set_var("piece_methods", "[No Methods]"); free(methods_buf);
        return; 
    }
    char line[MAX_LINE];
    int method_idx = 2;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "METHOD", 6) == 0) {
            char *key_start = strchr(line, '|'); if (!key_start) continue;
            key_start++; char *val_start = strchr(key_start, '|'); if (!val_start) continue;
            *val_start = '\0';
            char *trimmed = trim_pmo(key_start);
            if (strcmp(trimmed, "move") == 0 || strcmp(trimmed, "select") == 0) continue;
            char *btn = NULL;
            asprintf(&btn, "<button label=\"%s\" onClick=\"KEY:%d\" /> ", trimmed, method_idx++);
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

void load_vars() {
    load_state_file("pieces/apps/fuzzpet_app/manager/state.txt", NULL);
    const char* active_id = get_var("active_target_id");
    if (strlen(active_id) > 0) {
        load_dynamic_methods(active_id);
        if (strstr(active_id, "pet") != NULL) set_var("pet_active", "true"); else set_var("pet_active", "false");
        if (strcmp(active_id, "selector") == 0) set_var("selector_active", "true"); else set_var("selector_active", "false");
    }
    // FIXED: Layout expects fuzzpet_hunger, not hunger
    load_state_file("pieces/apps/fuzzpet_app/fuzzpet/state.txt", "fuzzpet");
    load_state_file("pieces/system/clock_daemon/state.txt", "clock");
    load_state_file("pieces/apps/player_app/manager/state.txt", NULL);
    
    load_state_file("pieces/world/map_01/selector/state.txt", "selector");
    load_state_file("pieces/world/map_01/player/state.txt", "player");

    char *view_path = build_path_malloc("pieces/apps/fuzzpet_app/fuzzpet/view.txt");
    FILE *vf = fopen(view_path, "r");
    if (vf) {
        char *map_buf = malloc(MAX_VAR_VALUE); if (!map_buf) { free(view_path); return; }
        map_buf[0] = '\0'; char line[MAX_LINE];
        while (fgets(line, sizeof(line), vf)) { if (strlen(map_buf) + strlen(line) < MAX_VAR_VALUE - 1) strcat(map_buf, line); }
        set_var("game_map", map_buf); fclose(vf); free(map_buf);
    } else {
        free(view_path); view_path = build_path_malloc("pieces/apps/player_app/view.txt");
        vf = fopen(view_path, "r");
        if (vf) {
            char *map_buf = malloc(MAX_VAR_VALUE); if (!map_buf) { free(view_path); return; }
            map_buf[0] = '\0'; char line[MAX_LINE];
            while (fgets(line, sizeof(line), vf)) { if (strlen(map_buf) + strlen(line) < MAX_VAR_VALUE - 1) strcat(map_buf, line); }
            set_var("game_map", map_buf); fclose(vf); free(map_buf);
        } else {
            set_var("game_map", "[Map Loading...]");
        }
    }
    free(view_path);
}

void inject_raw_key(int code) {
    char *path = build_path_malloc("pieces/apps/fuzzpet_app/fuzzpet/history.txt");
    FILE *fp = fopen(path, "a"); if (fp) { fprintf(fp, "%d\n", code); fclose(fp); }
    free(path);
}

bool is_suspended = false;

void launch_module(const char* launch_str) { 
    cleanup_module();
    char* full_cmd = strdup(launch_str);
    char* args[16];
    int arg_count = 0;
    char* token = strtok(full_cmd, " ");
    while (token && arg_count < 15) {
        if (arg_count == 0) {
            args[arg_count++] = (token[0] == '/') ? strdup(token) : build_path_malloc(token);
        } else {
            args[arg_count++] = strdup(token);
        }
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL;

    fprintf(stderr, "OS: Launching Sovereign App: %s\n", args[0]);
    is_suspended = true; // Hand over the theater

    current_module_pid = fork(); 
    if (current_module_pid == 0) { 
        execv(args[0], args); 
        perror("execv failed");
        exit(1); 
    }
    else if (current_module_pid > 0) {
        char* pm = build_path_malloc("pieces/os/plugins/+x/proc_manager.+x");
        char module_name[64]; 
        const char* base = strrchr(args[0], '/');
        strncpy(module_name, base ? base + 1 : args[0], 63); 
        module_name[63] = '\0';
        char* dot = strchr(module_name, '.'); if (dot) *dot = '\0';
        
        pid_t p = fork();
        if (p == 0) {
            char *pid_str = NULL; asprintf(&pid_str, "%d", current_module_pid);
            if (pid_str) {
                execl(pm, pm, "register", module_name, pid_str, NULL);
                free(pid_str);
            }
            exit(1);
        } else { waitpid(p, NULL, 0); }
        free(pm);
    }
    for (int i = 0; i < arg_count; i++) free(args[i]);
    free(full_cmd);
}

void handle_launch_command(const char* cmd) {
    char app_name[64]; strncpy(app_name, cmd + 7, sizeof(app_name)-1); app_name[sizeof(app_name)-1] = '\0';
    char* path_list = build_path_malloc("pieces/os/app_list.txt");
    FILE *f = fopen(path_list, "r");
    if (f) { 
        char line[MAX_LINE]; 
        while (fgets(line, sizeof(line), f)) { 
            line[strcspn(line, "\n")] = 0; 
            char *eq = strchr(line, '='); 
            if (eq) { 
                *eq = '\0'; 
                if (strcasecmp(line, app_name) == 0) { launch_module(trim_pmo(eq + 1)); break; } 
            } 
        } 
        fclose(f); 
    }
    free(path_list);
}

void send_command(const char* cmd) {
    if (strncmp(cmd, "LAUNCH:", 7) == 0) { handle_launch_command(cmd); return; }
    if (strncmp(cmd, "KEY:", 4) == 0) {
        int k = atoi(cmd + 4);
        if (k >= 0 && k <= 9) inject_raw_key('0' + k);
        else inject_raw_key(k);
        return;
    }
    if (strcmp(cmd, "BACK") == 0 || strcmp(cmd, "RELEASE") == 0) inject_raw_key('9');
}

bool evaluate_visibility(UIElement* el) {
    if (el->visibility_expr[0] == '\0') return true;
    char substituted[512]; substitute_vars(el->visibility_expr, substituted, 512);
    char *s = trim_pmo(substituted);
    return (strcmp(s, "true") == 0 || strcmp(s, "1") == 0);
}

bool is_interactive(UIElement* el) { return (strcmp(el->type, "button") == 0 || strcmp(el->type, "canvas") == 0 || strcmp(el->type, "cli_io") == 0); }
bool is_navigable(int idx) { 
    if (idx < 0 || idx >= element_count) return false; 
    UIElement* el = &elements[idx]; 
    return is_interactive(el) && evaluate_visibility(el); 
}

void parse_attributes(UIElement* el, const char* attr_str) {
    if (!attr_str) return; 
    char* attrs = strdup(attr_str); 
    char* pos = attrs;
    while (*pos) {
        while (*pos && isspace((unsigned char)*pos)) { pos++; }
        if (!*pos) break;
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
        else if (strcmp(name_start, "time_reactive") == 0) {
            if (strcmp(val_start, "true") == 0) is_time_reactive = true;
            else if (strcmp(val_start, "false") == 0) is_time_reactive = false;
        }
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
            Token* t = &tokens[(*token_count)++]; t->type = TOKEN_TEXT; 
            strncpy(t->content, cursor, MAX_LABEL_LEN-1); t->content[MAX_LABEL_LEN-1]='\0'; 
            break; 
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
    load_vars(); char* content = read_file_to_string(current_layout); if (!content) return;
    char* methods_raw = (char*)get_var("piece_methods");
    char* temp_substituted = malloc(MAX_BUFFER); char* p_src = content; char* p_dst = temp_substituted;
    while(*p_src) { 
        if(strncmp(p_src, "${piece_methods}", 16) == 0) { 
            strcpy(p_dst, methods_raw); p_dst += strlen(methods_raw); p_src += 16; 
        } else {
            *p_dst++ = *p_src++; 
        }
    }
    *p_dst = '\0'; free(content);
    int tc; Token* tokens = tokenize(temp_substituted, &tc); free(temp_substituted);
    element_count = 0; int stack[50], top = -1, current_interactive = 0;
    for (int i = 0; i < tc && element_count < MAX_ELEMENTS; i++) {
        Token* t = &tokens[i];
        if (t->type == TOKEN_TEXT) {
            char* trim_text = strdup(t->content); char* p_trim = trim_text; 
            while (*p_trim && isspace((unsigned char)*p_trim)) { p_trim++; }
            char* end_trim = p_trim + strlen(p_trim) - 1; 
            while (end_trim > p_trim && isspace((unsigned char)*end_trim)) { *end_trim-- = '\0'; }
            if (!*p_trim) { free(trim_text); continue; }
            UIElement* el = &elements[element_count++]; memset(el, 0, sizeof(UIElement));
            strcpy(el->type, "text"); strncpy(el->label, p_trim, MAX_LABEL_LEN - 1); 
            el->parent_index = (top >= 0) ? stack[top] : -1; free(trim_text);
        } else if (t->type == TOKEN_OPEN_TAG || t->type == TOKEN_SELFCLOSE_TAG) {
            if (strcmp(t->tag_name, "panel") == 0) { 
                UIElement dummy; memset(&dummy, 0, sizeof(UIElement));
                parse_attributes(&dummy, t->attributes); 
                if (t->type == TOKEN_OPEN_TAG) { stack[++top] = -1; }
                continue; 
            }
            if (strcmp(t->tag_name, "br") == 0) { 
                UIElement* el = &elements[element_count++]; memset(el, 0, sizeof(UIElement)); strcpy(el->type, "br"); 
                el->parent_index = (top >= 0) ? stack[top] : -1; continue; 
            }
            UIElement* el = &elements[element_count++]; memset(el, 0, sizeof(UIElement));
            strncpy(el->type, t->tag_name, 31); parse_attributes(el, t->attributes);
            el->parent_index = (top >= 0) ? stack[top] : -1; if (is_interactive(el)) el->interactive_idx = ++current_interactive;
            if (el->parent_index != -1) { 
                UIElement* pa = &elements[el->parent_index]; 
                if (pa->num_children < MAX_CHILDREN) { pa->children[pa->num_children++] = element_count - 1; }
            }
            if (t->type == TOKEN_OPEN_TAG) { stack[++top] = element_count - 1; }
        } else if (t->type == TOKEN_CLOSE_TAG) { if (top >= 0) { top--; } }
    }
    free(tokens);
}

void render_element(int idx, char* frame, int* p_current_interactive) {
    if (idx < 0 || idx >= element_count) return; 
    UIElement* el = &elements[idx];
    if (!evaluate_visibility(el)) return;
    substitute_vars(el->label, scratch_substituted, MAX_LABEL_LEN);
    if (strcmp(el->type, "br") == 0) { strcat(frame, "\n"); }
    else if (strcmp(el->type, "text") == 0) { strcat(frame, scratch_substituted); }
    else if (is_interactive(el)) {
        (*p_current_interactive)++; char *line = NULL; 
        bool is_focused = (active_index == -1 && idx == focus_index);
        if (idx == active_index) asprintf(&line, "[^] %d. [%s] ", *p_current_interactive, scratch_substituted);
        else if (is_focused) asprintf(&line, "[>] %d. [%s] ", *p_current_interactive, scratch_substituted);
        else asprintf(&line, "[ ] %d. [%s] ", *p_current_interactive, scratch_substituted);
        if (line) { strcat(frame, line); free(line); }
    }
    for (int i = 0; i < el->num_children; i++) { render_element(el->children[i], frame, p_current_interactive); }
}

void compose_frame() {
    load_vars(); const char* active_id = get_var("active_target_id");
    if (strcmp(active_id, last_active_id) != 0) { strncpy(last_active_id, active_id, 63); parse_chtm(); initialize_focus(); }
    char *frame = malloc(MAX_BUFFER); if (!frame) return; memset(frame, 0, MAX_BUFFER);
    int current_interactive = 0; for (int i = 0; i < element_count; i++) { if (elements[i].parent_index == -1) render_element(i, frame, &current_interactive); }
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
    if (active_index == -1) {
        if (key == ARROW_UP || key == 'w' || key == 'W' || key == JOY_UP) { int prev = focus_index; do { focus_index--; if (focus_index < 0) focus_index = element_count-1; } while (focus_index != prev && !is_navigable(focus_index)); clear_nav_on_next = true; }
        else if (key == ARROW_DOWN || key == 's' || key == 'S' || key == JOY_DOWN) { int prev = focus_index; do { focus_index++; if (focus_index >= element_count) focus_index = 0; } while (focus_index != prev && !is_navigable(focus_index)); clear_nav_on_next = true; }
        else if (isdigit(key)) { int d = key - '0'; if (d > 0 && do_jump(d)) { nav_buffer[0] = (char)key; nav_buffer[1] = 0; clear_nav_on_next = true; } }
        else if (key == 10 || key == 13 || key == JOY_BUTTON_0) {
            if (focus_index >= 0 && focus_index < element_count) {
                UIElement *el = &elements[focus_index];
                if (strlen(el->href) > 0) { strncpy(current_layout, el->href, MAX_PATH-1); parse_chtm(); focus_index = 0; initialize_focus(); }
                else if (strcmp(el->onClick, "INTERACT") == 0) { active_index = focus_index; clear_nav_on_next = true; }
                else if (strlen(el->onClick) > 0) { send_command(el->onClick); wait_for_view_change = true; clear_nav_on_next = true; }
            }
        } else if (key == 'q' || key == 'Q') exit(0);
    } else {
        UIElement *el = &elements[active_index];
        if (key == ESC_KEY || key == JOY_BUTTON_8) { active_index = -1; }
        else if (strcmp(el->onClick, "INTERACT") == 0) {
            int eff = key;
            if (key == ARROW_LEFT) eff = 1000; else if (key == ARROW_RIGHT) eff = 1001; else if (key == ARROW_UP) eff = 1002; else if (key == ARROW_DOWN) eff = 1003;
            else if (key >= JOY_BUTTON_0 && key <= JOY_BUTTON_8) eff = 2000 + (key - JOY_BUTTON_0);
            inject_raw_key(eff);
            if (key >= 32 && key <= 126) { nav_buffer[0] = (char)key; nav_buffer[1] = 0; } else if (key == 10 || key == 13) strcpy(nav_buffer, "ENTER");
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
    
    if (stat(master_frame_ch, &st) == 0) last_master_pulse_size = st.st_size;
    if (stat(display_frame_ch, &st) == 0) last_display_pulse_size = st.st_size;
    if (stat(view_ch, &st) == 0) last_view_file_size = st.st_size;
    if (stat(layout_ch, &st) == 0) last_layout_file_size = st.st_size;

    while (1) {
        int dirty = 0; 

        // Check if sovereign app finished
        if (is_suspended && current_module_pid > 0) {
            int status;
            if (waitpid(current_module_pid, &status, WNOHANG) != 0) {
                fprintf(stderr, "OS: Sovereign App exited. Resuming OS Theater.\n");
                is_suspended = false;
                current_module_pid = -1;
                dirty = 1; // Trigger redraw of main menu
            }
        }

        FILE *history = fopen(hist_p, "r");
        if (history) { fseek(history, last_history_position, SEEK_SET); char line[200]; while (fgets(line, sizeof(line), history)) { if (strstr(line, "KEY_PRESSED: ")) { int key = atoi(strstr(line, "KEY_PRESSED: ") + 13); if (key > 0) { process_key(key); dirty = 1; } } } last_history_position = ftell(history); fclose(history); }
        
        if (!is_suspended) {
            if (stat(master_frame_ch, &st) == 0 && st.st_size > last_master_pulse_size) {
                last_master_pulse_size = st.st_size;
                if (is_time_reactive) dirty = 1; 
            }
            
            if (stat(display_frame_ch, &st) == 0 && st.st_size > last_display_pulse_size) {
                last_display_pulse_size = st.st_size;
                dirty = 1; 
            }

            if (stat(view_ch, &st) == 0 && st.st_size > last_view_file_size) {
                last_view_file_size = st.st_size;
                dirty = 1; wait_for_view_change = false; 
            }
            
            if (stat(layout_ch, &st) == 0 && st.st_size > last_layout_file_size) {
                last_layout_file_size = st.st_size; FILE *lf = fopen(layout_ch, "r");
                if (lf) { fseek(lf, -MAX_PATH, SEEK_END); char new_l[MAX_PATH]; if (fgets(new_l, sizeof(new_l), lf)) { strncpy(current_layout, trim_pmo(new_l), MAX_PATH-1); parse_chtm(); initialize_focus(); dirty = 1; } fclose(lf); }
            }
        }

        if (dirty || clear_nav_on_next) { 
            if (!is_suspended) compose_frame(); 
            dirty = 0; 
        }
        usleep(16667);
    }
    return 0;
}
