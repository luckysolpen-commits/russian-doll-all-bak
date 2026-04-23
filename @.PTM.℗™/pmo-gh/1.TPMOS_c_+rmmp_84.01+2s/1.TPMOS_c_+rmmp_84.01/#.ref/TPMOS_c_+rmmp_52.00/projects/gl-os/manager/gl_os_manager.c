#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <stdarg.h>

#define MAX_LINE 1024
#define MAX_WINDOWS 15
#define MAX_PATH 1024

typedef struct {
    int id;
    int x, y, w, h;
    char title[64];
    char content_source[MAX_PATH];
    bool active;
    char type[32]; // "terminal", "menu", "folder"
} VirtualWindow;

VirtualWindow windows[MAX_WINDOWS];
int window_count = 0;
int active_window_id = -1;
long last_history_pos = 0;
char project_root[MAX_PATH] = ".";

// Drag State
bool is_dragging = false;
int drag_win_id = -1;
int last_mouse_x = -1;
int last_mouse_y = -1;

void resolve_root() {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (!kvp) {
        // Fallback for when we're in the manager's directory
        kvp = fopen("../../../pieces/locations/location_kvp", "r");
    }
    
    if (kvp) {
        char line[2048];
        while (fgets(line, sizeof(line), kvp)) {
            if (strncmp(line, "project_root=", 13) == 0) {
                char *v = line + 13;
                v[strcspn(v, "\n\r")] = 0;
                if (strlen(v) > 0) strncpy(project_root, v, MAX_PATH-1);
                break;
            }
        }
        fclose(kvp);
    }
}

void build_p_path(char* dst, size_t sz, const char* rel) {
    snprintf(dst, sz, "%s/%s", project_root, rel);
}

void log_master(const char* type, const char* event, const char* piece, const char* source) {
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[100];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    char path[MAX_PATH];
    build_p_path(path, sizeof(path), "pieces/master_ledger/master_ledger.txt");
    FILE *ledger = fopen(path, "a");
    if (ledger) {
        fprintf(ledger, "[%s] %s: %s on %s | Source: %s\n", timestamp, type, event, piece, source);
        fclose(ledger);
    }
}

void write_sovereign_view() {
    char path[MAX_PATH];
    build_p_path(path, sizeof(path), "pieces/apps/gl_os/view.txt");
    FILE *fp = fopen(path, "w");
    if (!fp) return;
    
    fprintf(fp, "DESKTOP_VIEW\n");
    fprintf(fp, "BG: #000044\n");
    for (int i = 0; i < window_count; i++) {
        if (!windows[i].active) continue;
        fprintf(fp, "WINDOW | %d | %d | %d | %d | %d | %s | %s\n", 
                windows[i].id, windows[i].x, windows[i].y, windows[i].w, windows[i].h, 
                windows[i].title, windows[i].content_source);
    }
    fclose(fp);
    
    char m_path[MAX_PATH];
    build_p_path(m_path, sizeof(m_path), "pieces/apps/gl_os/view_changed.txt");
    FILE *marker = fopen(m_path, "a");
    if (marker) { fprintf(marker, "X\n"); fclose(marker); }
}

void open_window(const char* title, const char* source, int x, int y, int w, int h, const char* type) {
    if (window_count >= MAX_WINDOWS) return;
    int id = window_count++;
    windows[id].id = id;
    windows[id].x = x;
    windows[id].y = y;
    windows[id].w = w;
    windows[id].h = h;
    strncpy(windows[id].title, title, 63);
    
    // Convert source to absolute if it's relative
    if (source[0] != '/') {
        build_p_path(windows[id].content_source, MAX_PATH, source);
    } else {
        strncpy(windows[id].content_source, source, MAX_PATH-1);
    }
    
    strncpy(windows[id].type, type, 31);
    windows[id].active = true;
    active_window_id = id;
    write_sovereign_view();
}

void handle_right_click(int x, int y) {
    char menu_content_rel[MAX_PATH];
    snprintf(menu_content_rel, sizeof(menu_content_rel), "pieces/apps/gl_os/menu_content.txt");
    
    char menu_content_abs[MAX_PATH];
    build_p_path(menu_content_abs, sizeof(menu_content_abs), menu_content_rel);

    // Open a "Context Menu" window at the click location
    open_window("Context Menu", menu_content_abs, x, y, 150, 100, "menu");
    
    // Create temporary menu content
    FILE *f = fopen(menu_content_abs, "w");
    if (f) {
        fprintf(f, "1. New Terminal\n");
        fprintf(f, "2. Close Menu\n");
        fprintf(f, "3. Shutdown OS\n");
        fclose(f);
    }
    log_master("EventFire", "context_menu_opened", "gl_os", "gl_os_manager");
}

void debug_log(const char* fmt, ...) {
    FILE *f = fopen("projects/gl-os/manager/debug.log", "a");
    if (f) {
        va_list args;
        va_start(args, fmt);
        vfprintf(f, fmt, args);
        va_end(args);
        fflush(f);
        fclose(f);
    }
}

void process_history() {
    char path[MAX_PATH];
    build_p_path(path, sizeof(path), "projects/gl-os/history.txt");
    FILE *fp = fopen(path, "r");
    if (!fp) return;
    
    fseek(fp, last_history_pos, SEEK_SET);
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "MOUSE_CLICK:")) {
            int btn, state, x, y;
            if (sscanf(line, "MOUSE_CLICK: btn=%d, state=%d, x=%d, y=%d", &btn, &state, &x, &y) == 4) {
                if (btn == 0) { // Left Click
                    if (state == 0) { // DOWN
                        is_dragging = false;
                        for (int i = window_count - 1; i >= 0; i--) {
                            if (!windows[i].active) continue;
                            debug_log("[DEBUG] Click at %d,%d | Win %d at %d,%d w=%d h=%d\n", x, y, i, windows[i].x, windows[i].y, windows[i].w, windows[i].h);
                            if (x >= windows[i].x && x <= windows[i].x + windows[i].w &&
                                y >= windows[i].y && y <= windows[i].y + 25) {
                                is_dragging = true;
                                drag_win_id = i;
                                last_mouse_x = x;
                                last_mouse_y = y;
                                debug_log("[DEBUG] Drag START on window %d\n", i);
                                break;
                            }
                        }
                    } else if (state == 1) { // UP
                        if (is_dragging) debug_log("[DEBUG] Drag END\n");
                        is_dragging = false;
                        drag_win_id = -1;
                    }
                } else if (btn == 2 && state == 0) {
                    handle_right_click(x, y);
                }
            }
        } else if (strstr(line, "MOUSE_MOTION:")) {
            int x, y;
            if (sscanf(line, "MOUSE_MOTION: x=%d, y=%d", &x, &y) == 2) {
                if (is_dragging && drag_win_id != -1) {
                    int dx = x - last_mouse_x;
                    int dy = y - last_mouse_y;
                    windows[drag_win_id].x += dx;
                    windows[drag_win_id].y += dy;
                    last_mouse_x = x;
                    last_mouse_y = y;
                    write_sovereign_view();
                }
            }
        }
        last_history_pos = ftell(fp);
    }
    fclose(fp);
}

int main() {
    resolve_root();
    debug_log("[DEBUG] Manager starting with root: %s\n", project_root);

    // TPM: Initial Desktop Setup
    mkdir("pieces/apps/gl_os", 0777);
    
    // Launch dedicated renderer as a child process using absolute path
    char renderer_path[MAX_PATH];
    build_p_path(renderer_path, sizeof(renderer_path), "pieces/apps/gl_os/plugins/+x/gl_os_renderer.+x");
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "%s &", renderer_path);
    system(cmd);
    
    // Start with a terminal
    open_window("Terminal", "pieces/display/current_frame.txt", 50, 50, 500, 400, "terminal");
    
    while (1) {
        process_history();
        usleep(50000); // 20Hz polling, matching Python timer logic
    }
    return 0;
}
