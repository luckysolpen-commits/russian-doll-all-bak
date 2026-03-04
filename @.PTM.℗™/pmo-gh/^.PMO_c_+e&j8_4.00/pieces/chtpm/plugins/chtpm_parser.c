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

#define MAX_LINE 4096
#define MAX_VAR_NAME 64
#define MAX_VAR_VALUE 32768
#define MAX_VARS 100
#define MAX_ELEMENTS 200
#define MAX_BUFFER 1048576
#define MAX_PATH 512
#define MAX_CHILDREN 50
#define MAX_LABEL_LEN 32768
#define MAX_ATTR_LEN 512

enum editorKey {
    ARROW_LEFT = 1000, ARROW_RIGHT = 1001, ARROW_UP = 1002, ARROW_DOWN = 1003, ESC_KEY = 27,
    JOY_BUTTON_0 = 2000, JOY_LEFT = 2100, JOY_RIGHT = 2101, JOY_UP = 2102, JOY_DOWN = 2103
};

typedef struct {
    char type[32]; char label[MAX_LABEL_LEN]; char href[MAX_PATH]; char onClick[64];
    char id[MAX_ATTR_LEN]; char visibility_expr[MAX_ATTR_LEN];
    int width; int height; char input_buffer[256]; 
    int parent_index; int children[MAX_CHILDREN]; int num_children; int current_child_focus;
    bool visibility; int interactive_idx;
} UIElement;

char current_layout[MAX_PATH] = "pieces/chtpm/layouts/os.chtpm";
char last_active_id[64] = "";
int focus_index = 0; int active_index = -1; 
int element_count = 0; UIElement elements[MAX_ELEMENTS];
char module_path[MAX_PATH] = ""; char nav_buffer[256] = {0};
bool clear_nav_on_next = false; bool wait_for_view_change = false;

typedef struct { char name[MAX_VAR_NAME]; char value[MAX_VAR_VALUE]; } Variable;
Variable vars[MAX_VARS]; int var_count = 0;
long last_history_position = 0; long last_pulse_size = 0; long last_view_file_size = 0;
pid_t current_module_pid = -1;

char* scratch_substituted = NULL;

// Forward Decls
void parse_chtm();
void compose_frame();
void load_vars();
void substitute_vars(const char* src, char* dst, int max_len);
void trim_pdl_val(char* s);
void set_var(const char* name, const char* value);
const char* get_var(const char* name);
bool is_navigable(int idx);
void launch_module(const char* path);
void cleanup_module();
void handle_launch_command(const char* cmd);
void initialize_focus();

void cleanup_module() { if (current_module_pid > 0) { kill(current_module_pid, SIGTERM); waitpid(current_module_pid, NULL, WNOHANG); current_module_pid = -1; } }
void handle_sigint(int sig) { cleanup_module(); if (scratch_substituted) free(scratch_substituted); exit(0); }

void set_var(const char* name, const char* value) {
    for (int i = 0; i < var_count; i++) { if (strcmp(vars[i].name, name) == 0) { strncpy(vars[i].value, value, MAX_VAR_VALUE - 1); return; } }
    if (var_count < MAX_VARS) { strncpy(vars[var_count].name, name, MAX_VAR_NAME - 1); strncpy(vars[var_count].value, value, MAX_VAR_VALUE - 1); var_count++; }
}
const char* get_var(const char* name) { for (int i = 0; i < var_count; i++) if (strcmp(vars[i].name, name) == 0) return vars[i].value; return ""; }

void substitute_vars(const char* src, char* dst, int max_len) {
    if (!strchr(src, '$')) { strncpy(dst, src, max_len-1); dst[max_len-1] = '\0'; return; }
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

void load_dynamic_methods(const char* active_id) {
    char path[MAX_PATH], methods_buf[MAX_VAR_VALUE] = "";
    snprintf(path, sizeof(path), "pieces/entities/%s/%s.pdl", active_id, active_id);
    if (strcmp(active_id, "selector") == 0) strcpy(path, "pieces/apps/fuzzpet_app/selector/selector.pdl");
    
    FILE *f = fopen(path, "r");
    if (!f) { set_var("piece_methods", "[No Methods]"); return; }
    
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "METHOD", 6) == 0) {
            char *key_start = strchr(line, '|'); if (!key_start) continue;
            key_start++; char *val_start = strchr(key_start, '|'); if (!val_start) continue;
            *val_start = '\0'; trim_pdl_val(key_start);
            if (strcmp(key_start, "move") == 0 || strcmp(key_start, "select") == 0) continue;
            
            char btn[512], label[64]; strcpy(label, key_start); label[0] = toupper(label[0]);
            char upper_onClick[64]; int j; for(j=0; key_start[j]; j++) upper_onClick[j] = toupper(key_start[j]); upper_onClick[j] = '\0';
            snprintf(btn, sizeof(btn), "<button label=\"%s\" onClick=\"%s\" /> ", label, upper_onClick);
            if (strlen(methods_buf) + strlen(btn) < MAX_VAR_VALUE - 100) strcat(methods_buf, btn);
        }
    }
    fclose(f);
    set_var("piece_methods", methods_buf);
}

void load_vars() {
    char active_id[64] = "selector";
    FILE *msf = fopen("pieces/apps/fuzzpet_app/manager/state.txt", "r");
    if (msf) { char line[128]; while (fgets(line, sizeof(line), msf)) { if (strncmp(line, "active_target_id=", 17) == 0) sscanf(line + 17, "%63s", active_id); } fclose(msf); }
    
    if (strcmp(active_id, last_active_id) != 0) {
        load_dynamic_methods(active_id);
    }
    set_var("active_target_id", active_id);
    
    FILE *sf = fopen("pieces/apps/fuzzpet_app/fuzzpet/state.txt", "r");
    if (sf) { 
        char line[MAX_LINE]; 
        while (fgets(line, sizeof(line), sf)) { 
            line[strcspn(line, "\n")] = 0; char *eq = strchr(line, '='); 
            if (eq) { 
                *eq = '\0'; 
                size_t nlen = strlen(line) + 16; char *fname = malloc(nlen);
                if (fname) { snprintf(fname, nlen, "fuzzpet_%s", line); set_var(fname, eq + 1); free(fname); }
            } 
        } 
        fclose(sf); 
    }
    
    FILE *rf = fopen("pieces/apps/fuzzpet_app/fuzzpet/last_response.txt", "r");
    if (rf) { char line[256]; if (fgets(line, sizeof(line), rf)) { line[strcspn(line, "\n\r")] = 0; set_var("fuzzpet_response", line); } fclose(rf); }
    else { set_var("fuzzpet_response", ""); }

    FILE *mf = fopen("pieces/apps/fuzzpet_app/fuzzpet/view.txt", "r");
    if (mf) {
        char *map_buf = malloc(MAX_VAR_VALUE); map_buf[0] = '\0'; char line_buffer[MAX_LINE];
        while (fgets(line_buffer, sizeof(line_buffer), mf)) { if (strlen(map_buf) + strlen(line_buffer) < MAX_VAR_VALUE - 1) strcat(map_buf, line_buffer); else break; }
        set_var("game_map", map_buf); free(map_buf); fclose(mf);
    }
}

void trim_pdl_val(char* s) { char *p = s; while(isspace(*p)) p++; char *end = p + strlen(p) - 1; while(end > p && isspace(*end)) *end-- = '\0'; if (p != s) memmove(s, p, strlen(p) + 1); }

char* read_file_to_string(const char* filename) {
    FILE* file = fopen(filename, "r"); if (!file) return NULL;
    fseek(file, 0, SEEK_END); long length = ftell(file); fseek(file, 0, SEEK_SET);
    char* buffer = malloc(length + 1); if (!buffer) { fclose(file); return NULL; }
    size_t n = fread(buffer, 1, length, file); buffer[n] = '\0'; fclose(file); return buffer;
}

void parse_attributes(UIElement* el, const char* attr_str) {
    if (!attr_str) return; char* attrs = strdup(attr_str); char* pos = attrs;
    while (*pos) {
        while (*pos && isspace(*pos)) pos++; if (!*pos) break;
        char* name_start = pos; while (*pos && *pos != '=' && !isspace(*pos)) pos++;
        char saved = *pos; *pos = '\0'; while (*(++pos) && isspace(*pos));
        if (*pos == '=') { pos++; while (*pos && isspace(*pos)) pos++; }
        char* val_start = pos;
        if (*pos == '"' || *pos == '\'') { char quote = *pos++; val_start = pos; while (*pos && *pos != quote) pos++; if (*pos) *pos++ = '\0'; }
        else { while (*pos && !isspace(*pos) && *pos != '/') pos++; if (*pos) *pos++ = '\0'; }
        if (strcmp(name_start, "label") == 0) strncpy(el->label, val_start, MAX_LABEL_LEN - 1);
        else if (strcmp(name_start, "href") == 0) strncpy(el->href, val_start, MAX_PATH - 1);
        else if (strcmp(name_start, "onClick") == 0) strncpy(el->onClick, val_start, 63);
        else if (strcmp(name_start, "id") == 0) strncpy(el->id, val_start, MAX_ATTR_LEN - 1);
        else if (strcmp(name_start, "visibility") == 0) { strncpy(el->visibility_expr, val_start, MAX_ATTR_LEN - 1); el->visibility = (atoi(val_start) != 0 || val_start[0] == '$'); }
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
        if (!tag_start) { Token* t = &tokens[(*token_count)++]; t->type = TOKEN_TEXT; strncpy(t->content, cursor, MAX_LABEL_LEN-1); t->content[MAX_LABEL_LEN-1]='\0'; break; }
        if (tag_start > cursor) { Token* t = &tokens[(*token_count)++]; t->type = TOKEN_TEXT; int len = tag_start - cursor; if (len > MAX_LABEL_LEN-1) len = MAX_LABEL_LEN-1; strncpy(t->content, cursor, len); t->content[len] = '\0'; }
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

bool is_interactive(UIElement* el) { return (strcmp(el->type, "button") == 0 || strcmp(el->type, "link") == 0 || strcmp(el->type, "cli_io") == 0 || strcmp(el->type, "menu") == 0); }

void parse_chtm() {
    load_vars();
    char* content = read_file_to_string(current_layout); if (!content) return;
    
    char* methods_raw = (char*)get_var("piece_methods");
    char* temp_substituted = malloc(MAX_BUFFER);
    char* p_src = content; char* p_dst = temp_substituted;
    while(*p_src) {
        if(strncmp(p_src, "${piece_methods}", 16) == 0) {
            strcpy(p_dst, methods_raw); p_dst += strlen(methods_raw); p_src += 16;
        } else *p_dst++ = *p_src++;
    }
    *p_dst = '\0'; free(content);

    int tc; Token* tokens = tokenize(temp_substituted, &tc); free(temp_substituted);
    element_count = 0; module_path[0] = '\0'; int stack[50], top = -1, current_interactive = 0;
    for (int i = 0; i < tc && element_count < MAX_ELEMENTS; i++) {
        Token* t = &tokens[i];
        if (t->type == TOKEN_TEXT) {
            char* trim_text = strdup(t->content); char* p = trim_text; while (*p && isspace(*p)) p++; char* end = p + strlen(p) - 1; while (end > p && isspace(*end)) *end-- = '\0';
            if (!*p) { free(trim_text); continue; }
            UIElement* el = &elements[element_count++]; memset(el, 0, sizeof(UIElement));
            strcpy(el->type, "text"); strncpy(el->label, p, MAX_LABEL_LEN - 1); el->parent_index = (top >= 0) ? stack[top] : -1; free(trim_text);
        } else if (t->type == TOKEN_OPEN_TAG || t->type == TOKEN_SELFCLOSE_TAG) {
            if (strcmp(t->tag_name, "module") == 0) {
                while (++i < tc && tokens[i].type != TOKEN_CLOSE_TAG) { if (tokens[i].type == TOKEN_TEXT) {
                    char* mp = tokens[i].content; while (*mp && isspace(*mp)) mp++; char* mend = mp + strlen(mp) - 1; while (mend > mp && isspace(*mend)) *mend-- = '\0';
                    strncpy(module_path, mp, MAX_PATH-1); } } continue;
            }
            if (strcmp(t->tag_name, "panel") == 0) { if (t->type == TOKEN_OPEN_TAG) stack[++top] = -1; continue; }
            if (strcmp(t->tag_name, "br") == 0) { UIElement* el = &elements[element_count++]; memset(el, 0, sizeof(UIElement)); strcpy(el->type, "br"); el->parent_index = (top >= 0) ? stack[top] : -1; continue; }
            UIElement* el = &elements[element_count++]; memset(el, 0, sizeof(UIElement));
            strncpy(el->type, t->tag_name, 31); parse_attributes(el, t->attributes);
            el->parent_index = (top >= 0) ? stack[top] : -1; if (is_interactive(el)) el->interactive_idx = ++current_interactive;
            if (el->parent_index != -1) { UIElement* p = &elements[el->parent_index]; if (p->num_children < MAX_CHILDREN) p->children[p->num_children++] = element_count - 1; }
            if (t->type == TOKEN_OPEN_TAG) stack[++top] = element_count - 1;
        } else if (t->type == TOKEN_CLOSE_TAG) { if (top >= 0) top--; }
    }
    free(tokens); if (module_path[0] && current_module_pid == -1) {
        const char* active_id = get_var("active_target_id");
        strcpy(last_active_id, active_id);
        launch_module(module_path);
    }
}

bool evaluate_visibility(UIElement* el) {
    if (el->visibility_expr[0] == '\0') return true;
    char substituted[512]; substitute_vars(el->visibility_expr, substituted, 512);
    return (atoi(substituted) != 0);
}

bool is_navigable(int idx) { 
    if (idx < 0 || idx >= element_count) return false; UIElement* el = &elements[idx];
    if (!is_interactive(el) || !evaluate_visibility(el)) return false;
    return true;
}

void launch_module(const char* path) { cleanup_module(); current_module_pid = fork(); if (current_module_pid == 0) { execl(path, path, NULL); exit(1); } }

void handle_launch_command(const char* cmd) {
    char app_name[64]; strncpy(app_name, cmd + 7, sizeof(app_name)-1); app_name[sizeof(app_name)-1] = '\0';
    FILE *f = fopen("pieces/os/app_list.txt", "r");
    if (f) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0; char *eq = strchr(line, '=');
            if (eq) { *eq = '\0'; if (strcasecmp(line, app_name) == 0) { launch_module(eq + 1); break; } }
        }
        fclose(f);
    }
}

void render_element(int idx, char* frame, int* p_current_interactive) {
    if (idx < 0 || idx >= element_count) return; UIElement* el = &elements[idx];
    if (!evaluate_visibility(el)) return;
    
    substitute_vars(el->label, scratch_substituted, MAX_LABEL_LEN);
    if (strcmp(el->type, "br") == 0) strcat(frame, "\n");
    else if (strcmp(el->type, "text") == 0) strcat(frame, scratch_substituted);
    else if (is_interactive(el)) {
        (*p_current_interactive)++; char indicator[32], line[MAX_LINE]; 
        bool is_focused = (active_index == -1 && idx == focus_index);
        if (idx == active_index) strcpy(indicator, "[^]"); else if (is_focused) strcpy(indicator, "[>]"); else strcpy(indicator, "[ ]");
        snprintf(line, sizeof(line), "%s %d. [%s] ", indicator, *p_current_interactive, scratch_substituted);
        strcat(frame, line);
    }
    for (int i = 0; i < el->num_children; i++) render_element(el->children[i], frame, p_current_interactive);
}

void compose_frame() {
    load_vars();
    const char* active_id = get_var("active_target_id");
    if (strcmp(active_id, last_active_id) != 0) { 
        strcpy(last_active_id, active_id); 
        parse_chtm(); initialize_focus(); 
    }
    
    char *frame = malloc(MAX_BUFFER); memset(frame, 0, MAX_BUFFER);
    int current_interactive = 0; for (int i = 0; i < element_count; i++) if (elements[i].parent_index == -1) render_element(i, frame, &current_interactive);
    strcat(frame, "\n╚═══════════════════════════════════════════════════════════╝\n");
    char nav_msg[512]; if (active_index == -1) snprintf(nav_msg, sizeof(nav_msg), "Nav > %s_", nav_buffer); else snprintf(nav_msg, sizeof(nav_msg), "Active [^]: %s (ESC to exit)", nav_buffer);
    strcat(frame, nav_msg); strcat(frame, "\n");
    FILE *out_f = fopen("pieces/display/current_frame.txt", "w"); if (out_f) { fprintf(out_f, "%s", frame); fclose(out_f); FILE *marker = fopen("pieces/master_ledger/frame_changed.txt", "a"); if (marker) { fprintf(marker, "X\n"); fclose(marker); } }
    free(frame); if (clear_nav_on_next) { nav_buffer[0] = '\0'; clear_nav_on_next = false; }
}

void inject_raw_key(int code) {
    FILE *fp = fopen("pieces/apps/fuzzpet_app/fuzzpet/history.txt", "a"); if (fp) { fprintf(fp, "%d\n", code); fclose(fp); }
}

void send_command(const char* cmd) {
    if (strncmp(cmd, "LAUNCH:", 7) == 0) { handle_launch_command(cmd); return; }
    if (strcmp(cmd, "SCAN") == 0) { inject_raw_key('2'); }
    else if (strcmp(cmd, "COLLECT") == 0) { inject_raw_key('3'); }
    else if (strcmp(cmd, "PLACE") == 0) { inject_raw_key('4'); }
    else if (strcmp(cmd, "INVENTORY") == 0) { inject_raw_key('5'); }
    else if (strcmp(cmd, "TOGGLE_EMOJI") == 0) { inject_raw_key('6'); }
    else if (strcmp(cmd, "FEED") == 0) { inject_raw_key('2'); }
    else if (strcmp(cmd, "PLAY") == 0) { inject_raw_key('3'); }
    else if (strcmp(cmd, "SLEEP") == 0) { inject_raw_key('4'); }
    else if (strcmp(cmd, "BACK") == 0) { inject_raw_key('9'); }
}

bool do_jump(int target_num) { if (target_num <= 0) return false; int cn = 0; for (int i = 0; i < element_count; i++) { if (is_navigable(i)) { cn++; if (cn == target_num) { focus_index = i; return true; } } } return false; }

void initialize_focus() {
    if (element_count > 0) {
        for (int i = 0; i < element_count; i++) { if (is_navigable(i)) { focus_index = i; return; } }
    }
}

void process_key(int key) {
    if (active_index == -1) {
        if (key == ARROW_UP || key == 'w' || key == 'W' || key == JOY_UP) { int prev = focus_index; do { focus_index--; if (focus_index < 0) focus_index = element_count-1; } while (focus_index != prev && !is_navigable(focus_index)); clear_nav_on_next = true; }
        else if (key == ARROW_DOWN || key == 's' || key == 'S' || key == JOY_DOWN) { int prev = focus_index; do { focus_index++; if (focus_index >= element_count) focus_index = 0; } while (focus_index != prev && !is_navigable(focus_index)); clear_nav_on_next = true; }
        else if (isdigit(key)) { char digit_str[2] = {(char)key, '\0'}; if (do_jump(atoi(digit_str))) { nav_buffer[0] = (char)key; nav_buffer[1] = '\0'; clear_nav_on_next = true; } }
        else if (key == 10 || key == 13 || key == JOY_BUTTON_0) {
            if (element_count > 0 && focus_index < element_count) {
                UIElement *el = &elements[focus_index];
                if (strlen(el->href) > 0) { strncpy(current_layout, el->href, MAX_PATH-1); parse_chtm(); focus_index = 0; initialize_focus(); }
                else if (strcmp(el->onClick, "INTERACT") == 0) { active_index = focus_index; clear_nav_on_next = true; }
                else if (strlen(el->onClick) > 0) { send_command(el->onClick); wait_for_view_change = true; clear_nav_on_next = true; }
            }
        } else if (key == 'q' || key == 'Q') exit(0);
    } else { 
        UIElement *el = &elements[active_index]; 
        if (key == ESC_KEY) { active_index = -1; } 
        else if (strcmp(el->onClick, "INTERACT") == 0) { 
            int eff_key = key;
            if (key == ARROW_LEFT) eff_key = 1000;
            else if (key == ARROW_RIGHT) eff_key = 1001;
            else if (key == ARROW_UP) eff_key = 1002;
            else if (key == ARROW_DOWN) eff_key = 1003;
            inject_raw_key(eff_key); clear_nav_on_next = true; 
        } 
    }
}

int main(int argc, char **argv) {
    scratch_substituted = malloc(MAX_LABEL_LEN);
    if (argc > 1) strncpy(current_layout, argv[1], MAX_PATH-1);
    signal(SIGINT, handle_sigint); parse_chtm(); initialize_focus(); compose_frame(); struct stat st; 
    if (stat("pieces/display/frame_changed.txt", &st) == 0) last_pulse_size = st.st_size;
    if (stat("pieces/apps/fuzzpet_app/fuzzpet/view_changed.txt", &st) == 0) last_view_file_size = st.st_size;
    while (1) {
        int dirty = 0; FILE *history = fopen("pieces/keyboard/history.txt", "r");
        if (history) { fseek(history, last_history_position, SEEK_SET); char line[200]; while (fgets(line, sizeof(line), history)) { if (strstr(line, "KEY_PRESSED: ")) { int key = atoi(strstr(line, "KEY_PRESSED: ") + 13); if (key > 0) { process_key(key); dirty = 1; } } } last_history_position = ftell(history); fclose(history); }
        if (stat("pieces/display/frame_changed.txt", &st) == 0 && st.st_size > last_pulse_size) { last_pulse_size = st.st_size; dirty = 1; }
        if (stat("pieces/apps/fuzzpet_app/fuzzpet/view_changed.txt", &st) == 0 && st.st_size > last_view_file_size) { last_view_file_size = st.st_size; dirty = 1; wait_for_view_change = false; }
        if ((dirty || clear_nav_on_next) && !wait_for_view_change) { compose_frame(); if (stat("pieces/display/frame_changed.txt", &st) == 0) last_pulse_size = st.st_size; dirty = 0; } 
        usleep(16667); 
    }
    return 0;
}
