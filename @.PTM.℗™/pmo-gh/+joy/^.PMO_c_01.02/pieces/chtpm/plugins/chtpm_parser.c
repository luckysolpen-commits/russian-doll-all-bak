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

#define MAX_LINE 1024
#define MAX_VAR_NAME 64
#define MAX_VAR_VALUE 1024
#define MAX_VARS 100
#define MAX_ELEMENTS 100
#define MAX_BUFFER 65536
#define MAX_PATH 512
#define MAX_CHILDREN 20
#define MAX_LABEL_LEN 256
#define MAX_ATTR_LEN 256

enum editorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT = 1001,
    ARROW_UP = 1002,
    ARROW_DOWN = 1003,
    ESC_KEY = 27,
    
    // Joystick 2000-range
    JOY_BUTTON_0 = 2000,
    JOY_LEFT = 2100,
    JOY_RIGHT = 2101,
    JOY_UP = 2102,
    JOY_DOWN = 2103
};

typedef struct {
    char type[32];    
    char label[MAX_LABEL_LEN];
    char href[MAX_PATH];
    char onClick[64];
    char id[MAX_ATTR_LEN];
    int width;
    int height;
    char input_buffer[256]; 
    int parent_index;
    int children[MAX_CHILDREN];
    int num_children;
    int current_child_focus;
    bool visibility; 
    int interactive_idx;
} UIElement;

// Global state
char current_layout[MAX_PATH] = "pieces/chtpm/layouts/os.chtpm";
int focus_index = 0;
int active_index = -1; 
int element_count = 0;
UIElement elements[MAX_ELEMENTS];
char module_path[MAX_PATH] = "";
char nav_buffer[256] = {0};
bool clear_nav_on_next = false;
bool wait_for_view_change = false;  // Wait for fuzzpet view_changed before composing

typedef struct {
    char name[MAX_VAR_NAME];
    char value[MAX_VAR_VALUE];
} Variable;

Variable vars[MAX_VARS];
int var_count = 0;

long last_history_position = 0;
long last_pulse_size = 0;
long last_view_file_size = 0;
pid_t current_module_pid = -1;

// Prototypes
void launch_module(const char* path);
void send_command(const char* cmd);
void parse_chtm();
void initialize_focus();
bool is_navigable(int idx);
bool is_interactive(UIElement* el);
void compose_frame();
void set_var(const char* name, const char* value);

void cleanup_module() {
    if (current_module_pid > 0) { kill(current_module_pid, SIGTERM); waitpid(current_module_pid, NULL, WNOHANG); current_module_pid = -1; }
}

void handle_sigint(int sig) { cleanup_module(); exit(0); }

void load_state() {
    FILE *f = fopen("pieces/chtpm/state.txt", "r");
    if (f) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            if (strncmp(line, "focus_index=", 12) == 0) focus_index = atoi(line + 12);
            else if (strncmp(line, "active_index=", 13) == 0) active_index = atoi(line + 13);
            else if (strncmp(line, "current_layout=", 15) == 0) strncpy(current_layout, line + 15, MAX_PATH-1);
        }
        fclose(f);
    }
}

void save_state() {
    FILE *f = fopen("pieces/chtpm/state.txt", "w");
    if (f) { fprintf(f, "focus_index=%d\nactive_index=%d\ncurrent_layout=%s\n", focus_index, active_index, current_layout); fclose(f); }
    FILE *af = fopen("pieces/apps/fuzzpet_app/fuzzpet/active.txt", "w");
    if (af) { fprintf(af, "%d", (active_index != -1 && strcmp(elements[active_index].onClick, "INTERACT") == 0) ? 1 : 0); fclose(af); }
}

char* read_file_to_string(const char* filename) {
    FILE* file = fopen(filename, "r"); if (!file) return NULL;
    fseek(file, 0, SEEK_END); long length = ftell(file); fseek(file, 0, SEEK_SET);
    char* buffer = malloc(length + 1); if (!buffer) { fclose(file); return NULL; }
    size_t n = fread(buffer, 1, length, file); buffer[n] = '\0'; fclose(file); return buffer;
}

char* remove_comments(const char* content) {
    if (!content) return NULL; char* result = strdup(content); char* search = result;
    while ((search = strstr(search, "<!--"))) {
        char* end = strstr(search, "-->"); if (end) { end += 3; memmove(search, end, strlen(end) + 1); } else break;
    }
    return result;
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
        else if (strcmp(name_start, "width") == 0) el->width = atoi(val_start);
        else if (strcmp(name_start, "height") == 0) el->height = atoi(val_start);
        else if (strcmp(name_start, "visibility") == 0) el->visibility = (atoi(val_start) != 0);
        *(pos - 1) = saved;
    }
    free(attrs);
}

typedef enum { TOKEN_TEXT, TOKEN_OPEN_TAG, TOKEN_CLOSE_TAG, TOKEN_SELFCLOSE_TAG } TokenType;
typedef struct { TokenType type; char content[1024]; char tag_name[64]; char attributes[512]; } Token;

Token* tokenize(const char* content, int* token_count) {
    Token* tokens = malloc(MAX_ELEMENTS * 4 * sizeof(Token)); *token_count = 0; const char* cursor = content;
    while (*cursor && *token_count < MAX_ELEMENTS * 4) {
        const char* tag_start = strchr(cursor, '<');
        if (!tag_start) { Token* t = &tokens[(*token_count)++]; t->type = TOKEN_TEXT; strncpy(t->content, cursor, 1023); t->content[1023]='\0'; break; }
        if (tag_start > cursor) { Token* t = &tokens[(*token_count)++]; t->type = TOKEN_TEXT; int len = tag_start - cursor; if (len > 1023) len = 1023; strncpy(t->content, cursor, len); t->content[len] = '\0'; }
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
    char* content = read_file_to_string(current_layout); if (!content) return;
    char* clean_content = remove_comments(content); free(content);
    int tc; Token* tokens = tokenize(clean_content, &tc); free(clean_content);
    element_count = 0; module_path[0] = '\0'; int stack[50], top = -1, current_interactive = 0;
    for (int i = 0; i < tc && element_count < MAX_ELEMENTS; i++) {
        Token* t = &tokens[i];
        if (t->type == TOKEN_TEXT) {
            char* trim_text = strdup(t->content); char* p = trim_text; while (*p && isspace(*p)) p++;
            char* end = p + strlen(p) - 1; while (end > p && isspace(*end)) *end-- = '\0';
            if (!*p) { free(trim_text); continue; }
            UIElement* el = &elements[element_count++]; memset(el, 0, sizeof(UIElement));
            strcpy(el->type, "text"); strncpy(el->label, p, MAX_LABEL_LEN - 1);
            el->parent_index = (top >= 0) ? stack[top] : -1; free(trim_text);
        } else if (t->type == TOKEN_OPEN_TAG || t->type == TOKEN_SELFCLOSE_TAG) {
            if (strcmp(t->tag_name, "module") == 0) {
                while (++i < tc && tokens[i].type != TOKEN_CLOSE_TAG) {
                    if (tokens[i].type == TOKEN_TEXT) {
                        char* mp = tokens[i].content; while (*mp && isspace(*mp)) mp++;
                        char* mend = mp + strlen(mp) - 1; while (mend > mp && isspace(*mend)) *mend-- = '\0';
                        strncpy(module_path, mp, MAX_PATH-1);
                    }
                }
                continue;
            }
            if (strcmp(t->tag_name, "panel") == 0) { if (t->type == TOKEN_OPEN_TAG) stack[++top] = -1; continue; }
            UIElement* el = &elements[element_count++]; memset(el, 0, sizeof(UIElement));
            strncpy(el->type, t->tag_name, 31); parse_attributes(el, t->attributes);
            el->parent_index = (top >= 0) ? stack[top] : -1;
            if (is_interactive(el)) el->interactive_idx = ++current_interactive;
            if (el->parent_index != -1) { UIElement* p = &elements[el->parent_index]; if (p->num_children < MAX_CHILDREN) p->children[p->num_children++] = element_count - 1; }
            if (t->type == TOKEN_OPEN_TAG) stack[++top] = element_count - 1;
        } else if (t->type == TOKEN_CLOSE_TAG) { if (top >= 0) top--; }
    }
    free(tokens); if (module_path[0] && current_module_pid == -1) launch_module(module_path);
}

bool is_navigable(int idx) { if (idx < 0 || idx >= element_count) return false; return is_interactive(&elements[idx]); }

void initialize_focus() {
    if (element_count > 0 && !is_navigable(focus_index)) {
        for (int i = 0; i < element_count; i++) { if (is_navigable(i)) { focus_index = i; break; } }
    }
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

void trim_pdl_val(char* s) {
    char *p = s; while(isspace(*p)) p++;
    char *end = p + strlen(p) - 1;
    while(end > p && isspace(*end)) *end-- = '\0';
    if (p != s) memmove(s, p, strlen(p) + 1);
}

void set_var(const char* name, const char* value) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i].name, name) == 0) {
            strncpy(vars[i].value, value, MAX_VAR_VALUE - 1);
            return;
        }
    }
    if (var_count < MAX_VARS) {
        strncpy(vars[var_count].name, name, MAX_VAR_NAME - 1);
        strncpy(vars[var_count].value, value, MAX_VAR_VALUE - 1);
        var_count++;
    }
}

void load_vars_pdl(const char* filepath, const char* prefix) {
    FILE *f = fopen(filepath, "r");
    if (!f) return;
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "STATE", 5) == 0) {
            char temp_line[MAX_LINE];
            strncpy(temp_line, line, MAX_LINE - 1);
            temp_line[MAX_LINE - 1] = '\0';

            char *key_start = strchr(temp_line, '|');
            if (!key_start) continue;
            key_start++;

            char *value_start = strchr(key_start, '|');
            if (!value_start) continue;
            *value_start = '\0';
            value_start++;

            trim_pdl_val(key_start);
            trim_pdl_val(value_start);
            
            char full_name[MAX_VAR_NAME];
            snprintf(full_name, MAX_VAR_NAME, "%s_%s", prefix, key_start);
            set_var(full_name, value_start);
        }
    }
    fclose(f);
}

void load_fuzzpet_state() {
    FILE *f = fopen("pieces/apps/fuzzpet_app/fuzzpet/state.txt", "r");
    if (!f) return;
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        char *eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';
            char full_name[MAX_VAR_NAME];
            snprintf(full_name, MAX_VAR_NAME, "fuzzpet_%.50s", line);
            set_var(full_name, eq + 1);
        }
    }
    fclose(f);
}

void load_vars() {
    var_count = 0;
    // 1. Load Defaults (DNA)
    load_vars_pdl("pieces/apps/fuzzpet_app/fuzzpet/fuzzpet.pdl", "fuzzpet");
    // 2. Load Runtime (Mirror)
    load_fuzzpet_state();
    
    // 3. System Pieces
    FILE *cf = fopen("pieces/apps/fuzzpet_app/clock/state.txt", "r");
    if (cf) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), cf)) {
            line[strcspn(line, "\n")] = 0;
            if (strncmp(line, "turn ", 5) == 0) set_var("clock_turn", line + 5);
            else if (strncmp(line, "time ", 5) == 0) set_var("clock_time", line + 5);
        }
        fclose(cf);
    }
    FILE *df = fopen("pieces/display/state.txt", "r");
    if (df) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), df)) {
            line[strcspn(line, "\n")] = 0; char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0'; char full_name[MAX_VAR_NAME];
                snprintf(full_name, MAX_VAR_NAME, "display_%.50s", line);
                set_var(full_name, eq + 1);
            }
        }
        fclose(df);
    }
    // 4. SOVEREIGN COMPOSITE: Read Module's Stage (view.txt)
    FILE *mf = fopen("pieces/apps/fuzzpet_app/fuzzpet/view.txt", "r");
    if (mf) {
        char map_buf[MAX_VAR_VALUE] = "";
        char line_buffer[MAX_LINE];
        while (fgets(line_buffer, sizeof(line_buffer), mf)) { 
            if (strlen(map_buf) + strlen(line_buffer) < MAX_VAR_VALUE - 1) {
                strcat(map_buf, line_buffer);
            } else break;
        }
        set_var("game_map", map_buf);
        fclose(mf);
    }
}

const char* get_var(const char* name) { for (int i = 0; i < var_count; i++) if (strcmp(vars[i].name, name) == 0) return vars[i].value; return ""; }

void render_element(int idx, char* frame) {
    if (idx < 0 || idx >= element_count) return; UIElement* el = &elements[idx];
    if (el->parent_index != -1) { UIElement* p = &elements[el->parent_index]; if (strcmp(p->type, "menu") == 0 && !p->visibility && active_index != el->parent_index) return; }
    char substituted[MAX_LINE * 2] = ""; char *p_src = el->label, *p_dst = substituted;
    while (*p_src) {
        if (*p_src == '$' && *(p_src+1) == '{') {
            char *end = strchr(p_src, '}');
            if (end) {
                char var_name[64]; int len = end - (p_src + 2); if (len > 63) len = 63;
                strncpy(var_name, p_src + 2, len); var_name[len] = '\0';
                const char *val = get_var(var_name); strcpy(p_dst, val); p_dst += strlen(val); p_src = end + 1; continue;
            }
        }
        *p_dst++ = *p_src++;
    }
    *p_dst = '\0';
    if (strcmp(el->type, "br") == 0) strcat(frame, "\n");
    else if (strcmp(el->type, "text") == 0) strcat(frame, substituted);
    else if (is_interactive(el)) {
        char indicator[32], *line = NULL; int is_focused = (active_index == -1 && idx == focus_index);
        if (el->parent_index != -1) { UIElement* par = &elements[el->parent_index]; if (strcmp(par->type, "menu") == 0 && active_index == el->parent_index) { is_focused = 0; for (int c=0; c < par->num_children; c++) if (par->children[c] == idx && par->current_child_focus == c) is_focused = 1; } }
        if (idx == active_index) strcpy(indicator, "[^]"); else if (is_focused) strcpy(indicator, "[>]"); else strcpy(indicator, "[ ]");
        int len = 0;
        if (strcmp(el->type, "cli_io") == 0) {
            len = snprintf(NULL, 0, "%s %d. %s%s_ ", indicator, el->interactive_idx, substituted, el->input_buffer);
            line = malloc(len + 1); sprintf(line, "%s %d. %s%s_ ", indicator, el->interactive_idx, substituted, el->input_buffer);
        } else {
            len = snprintf(NULL, 0, "%s %d. [%s] ", indicator, el->interactive_idx, substituted);
            line = malloc(len + 1); sprintf(line, "%s %d. [%s] ", indicator, el->interactive_idx, substituted);
        }
        if (el->parent_index != -1) strcat(frame, "  ");
        strcat(frame, line); free(line);
    }
    if ((strcmp(el->type, "menu") == 0 || strcmp(el->type, "panel") == 0) && (el->parent_index == -1 || el->visibility || active_index == idx)) { for (int i = 0; i < el->num_children; i++) render_element(el->children[i], frame); }
}

void compose_frame() {
    load_vars();
    char *frame = calloc(1, MAX_BUFFER);
    if (!frame) return;
    
    for (int i = 0; i < element_count; i++)
        if (elements[i].parent_index == -1)
            render_element(i, frame);
    
    strcat(frame, "\n╚═══════════════════════════════════════════════════════════╝\n");
    char nav_msg[512];
    if (active_index == -1)
        snprintf(nav_msg, sizeof(nav_msg), "Nav > %s_", nav_buffer);
    else
        snprintf(nav_msg, sizeof(nav_msg), "Active [^]: %s (ESC to exit)", nav_buffer);
    strcat(frame, nav_msg);
    strcat(frame, "\n");
    
    // Count lines for audit
    int line_count = 0;
    for (char *p = frame; *p; p++)
        if (*p == '\n') line_count++;
    
    // Get timestamp
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[100];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    FILE *out_f = fopen("pieces/display/current_frame.txt", "w");
    if (out_f) {
        fprintf(out_f, "%s", frame);
        fclose(out_f);
        
        // Log to display ledger (AUDIT OBSESSION!)
        FILE *display_ledger = fopen("pieces/display/ledger.txt", "a");
        if (display_ledger) {
            fprintf(display_ledger, "[%s] FrameComposed: %d lines | Source: chtpm_parser\n", 
                    timestamp, line_count);
            fclose(display_ledger);
        }
        
        // Signal renderer (SOLE WRITER)
        FILE *marker = fopen("pieces/master_ledger/frame_changed.txt", "a");
        if (marker) {
            fprintf(marker, "X\n");
            fclose(marker);
        }
    }
    free(frame);
    if (clear_nav_on_next) {
        nav_buffer[0] = '\0';
        clear_nav_on_next = false;
    }
}

void inject_raw_key(int code) {
    // Get timestamp
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[100];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    // Write to module history
    FILE *fp = fopen("pieces/apps/fuzzpet_app/fuzzpet/history.txt", "a");
    if (fp) { fprintf(fp, "%d\n", code); fclose(fp); }
    
    // Log to master ledger (AUDIT OBSESSION!)
    FILE *master = fopen("pieces/master_ledger/master_ledger.txt", "a");
    if (master) {
        fprintf(master, "[%s] InputForwarded: key_code=%d to fuzzpet | Source: chtpm_parser\n", 
                timestamp, code);
        fclose(master);
    }
}

void send_command(const char* cmd) {
    if (strncmp(cmd, "LAUNCH:", 7) == 0) { handle_launch_command(cmd); return; }
    // VIRTUAL REFLECTION: UI click maps to Virtual Key in App History
    // Send ONLY the numeric hotkey (keyboard parity) - joystick buttons send their own codes directly
    if (strcmp(cmd, "FEED") == 0) { inject_raw_key('2'); }
    else if (strcmp(cmd, "PLAY") == 0) { inject_raw_key('3'); }
    else if (strcmp(cmd, "SLEEP") == 0) { inject_raw_key('4'); }
    else if (strcmp(cmd, "EVOLVE") == 0) { inject_raw_key('5'); }
    else if (strcmp(cmd, "END_TURN") == 0) { inject_raw_key('6'); }
    // NO delay - let view_changed.txt marker trigger the next frame
}

bool do_jump(int target_num) { for (int i = 0; i < element_count; i++) { if (elements[i].interactive_idx == target_num) { focus_index = i; return true; } } return false; }

void process_key(int key) {
    if (active_index == -1) {
        if (key == ARROW_UP || key == 'w' || key == 'W' || key == JOY_UP) {
            int prev = focus_index; do { focus_index--; } while (focus_index >= 0 && !is_navigable(focus_index));
            if (focus_index < 0) focus_index = prev; 
            if (key == JOY_UP) strcpy(nav_buffer, "J-UP"); else strcpy(nav_buffer, (key == ARROW_UP || key == 'w') ? "w" : "W"); 
            clear_nav_on_next = true;
        } else if (key == ARROW_DOWN || key == 's' || key == 'S' || key == JOY_DOWN) {
            int prev = focus_index; do { focus_index++; } while (focus_index < element_count && !is_navigable(focus_index));
            if (focus_index >= element_count) focus_index = prev; 
            if (key == JOY_DOWN) strcpy(nav_buffer, "J-DN"); else strcpy(nav_buffer, (key == ARROW_DOWN || key == 's') ? "s" : "S"); 
            clear_nav_on_next = true;
        } else if (key == ARROW_LEFT || key == 'a' || key == 'A' || key == JOY_LEFT) { 
            if (key == JOY_LEFT) strcpy(nav_buffer, "J-LF"); else strcpy(nav_buffer, (key == ARROW_LEFT || key == 'a') ? "a" : "A"); 
            clear_nav_on_next = true; 
        } else if (key == ARROW_RIGHT || key == 'd' || key == 'D' || key == JOY_RIGHT) { 
            if (key == JOY_RIGHT) strcpy(nav_buffer, "J-RT"); else strcpy(nav_buffer, (key == ARROW_RIGHT || key == 'd') ? "d" : "D"); 
            clear_nav_on_next = true; 
        } else if (isdigit(key)) {
            int len = strlen(nav_buffer); if (len >= 254 || !isdigit(nav_buffer[0])) { nav_buffer[0] = '\0'; len = 0; }
            nav_buffer[len] = (char)key; nav_buffer[len+1] = '\0';
            if (!do_jump(atoi(nav_buffer))) { char fresh[2] = {(char)key, '\0'}; if (do_jump(atoi(fresh))) strcpy(nav_buffer, fresh); else nav_buffer[0] = '\0'; }
        } else if (key == 10 || key == 13 || key == JOY_BUTTON_0) {
            // Show ENTER in nav buffer for debug - don't auto-clear
            // Nav buffer persists until next key (like number keys)
            strcpy(nav_buffer, "ENTER");
            if (element_count > 0 && focus_index < element_count) {
                UIElement *el = &elements[focus_index];
                if (strlen(el->href) > 0) { 
                    cleanup_module();
                    strncpy(current_layout, el->href, MAX_PATH-1); 
                    parse_chtm(); 
                    focus_index = 0; active_index = -1; 
                    initialize_focus(); 
                    // HREF navigation - don't wait for view change
                }
                else if (strcmp(el->type, "menu") == 0) { 
                    active_index = focus_index; 
                    if (el->num_children > 0) el->current_child_focus = 0; 
                    // Menu - don't wait for view change
                }
                else { 
                    active_index = focus_index; 
                    if (strcmp(el->type, "cli_io") != 0 && strcmp(el->onClick, "INTERACT") != 0) { 
                        if (strlen(el->onClick) > 0) {
                            send_command(el->onClick);
                            // Only wait for view change if we sent a command to fuzzpet
                            wait_for_view_change = true;
                        }
                        active_index = -1; 
                    }
                }
            }
        } else if (key == 127 || key == 8) { int len = strlen(nav_buffer); if (len > 0) nav_buffer[--len] = '\0'; }
        else if (key == 'q' || key == 'Q') exit(0);
    } else {
        UIElement *el = &elements[active_index]; if (key == ESC_KEY) { active_index = -1; }
        else if (strcmp(el->type, "cli_io") == 0) {
            if (key == 10 || key == 13) { for(int i=0; el->input_buffer[i]; i++) inject_raw_key(el->input_buffer[i]); inject_raw_key(10); el->input_buffer[0] = '\0'; active_index = -1; }
            else if (key == 127 || key == 8) { int len = strlen(el->input_buffer); if (len > 0) el->input_buffer[len-1] = '\0'; }
            else if (key >= 32 && key <= 126) { int len = strlen(el->input_buffer); if (len < 255) { el->input_buffer[len] = (char)key; el->input_buffer[len+1] = '\0'; } }
        } else if (strcmp(el->onClick, "INTERACT") == 0) {
            if (key == ESC_KEY) { active_index = -1; }
            else { 
                inject_raw_key(key); 
                if (key >= 32 && key <= 126) { nav_buffer[0] = (char)key; nav_buffer[1] = '\0'; } 
                else if (key >= 2000 && key < 2100) { snprintf(nav_buffer, sizeof(nav_buffer), "J-B%d", key-2000); }
                else if (key >= 2100 && key < 2200) {
                    const char* axis_dirs[] = {"L", "R", "U", "D"};
                    int idx = key - 2100;
                    if (idx < 4) snprintf(nav_buffer, sizeof(nav_buffer), "J-A%s", axis_dirs[idx]);
                    else snprintf(nav_buffer, sizeof(nav_buffer), "J-CODE %d", key);
                }
                else { strcpy(nav_buffer, "[Key]"); } clear_nav_on_next = true; 
            }
        }
    }
    save_state();
}

int main(int argc, char **argv) {
    load_state(); if (argc > 1) strncpy(current_layout, argv[1], MAX_PATH-1);
    signal(SIGINT, handle_sigint); parse_chtm(); initialize_focus();
    compose_frame(); struct stat st; 
    
    // Tally Initialization (Universal Pulse)
    if (stat("pieces/display/frame_changed.txt", &st) == 0) last_pulse_size = st.st_size;
    if (stat("pieces/apps/fuzzpet_app/fuzzpet/view_changed.txt", &st) == 0) last_view_file_size = st.st_size;

    while (1) {
        int dirty = 0; FILE *history = fopen("pieces/keyboard/history.txt", "r");
        if (history) {
            fseek(history, last_history_position, SEEK_SET);
            char line[200];
            while (fgets(line, sizeof(line), history)) {
                if (strstr(line, "KEY_PRESSED: ")) {
                    int key = atoi(strstr(line, "KEY_PRESSED: ") + 13);
                    if (key > 0) { process_key(key); dirty = 1; }
                }
            }
            last_history_position = ftell(history);
            fclose(history);
        }
        
        // Change Detection Node (The Heartbeat)
        if (stat("pieces/display/frame_changed.txt", &st) == 0) { 
            if (st.st_size > last_pulse_size) { last_pulse_size = st.st_size; dirty = 1; } 
        }
        if (stat("pieces/apps/fuzzpet_app/fuzzpet/view_changed.txt", &st) == 0) { 
            if (st.st_size > last_view_file_size) { 
                last_view_file_size = st.st_size; 
                dirty = 1;
                // Fuzzpet updated - safe to compose now
                wait_for_view_change = false;
            } 
        }

        // Only compose if dirty AND not waiting for view change
        if ((dirty || clear_nav_on_next) && !wait_for_view_change) { 
            compose_frame(); 
            // Update pulse size AFTER our own write to prevent self-triggering
            if (stat("pieces/display/frame_changed.txt", &st) == 0) last_pulse_size = st.st_size;
            dirty = 0; 
            if (clear_nav_on_next) { 
                nav_buffer[0] = '\0'; 
                clear_nav_on_next = false; 
                // Don't recompose immediately - let the next dirty trigger show cleared nav
                // This prevents double-composition when module also updates
            } 
        } 
        usleep(16667); 
    }
    return 0;
}