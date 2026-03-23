/*
 * gl_desktop.c - GL-OS Desktop Environment
 * Opens OpenGL window with desktop, windows, context menu, and toolbar
 * TPM-Compliant: Separate session from CHTPM terminal
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_LINE 1024
#define MAX_WIN_TITLE 64
#define MAX_WINDOWS 15
#define MAX_PATH 4096

/* Window structure */
typedef struct {
    int id;
    int x, y, w, h;
    char title[MAX_WIN_TITLE];
    char type[32];  /* "terminal", "cube", "folder" */
    int active;
    int minimized;
    
    /* App-specific state */
    char input_buffer[256];
    float rotation_x, rotation_y;
} DesktopWindow;

/* Global state */
int window_width = 800;
int window_height = 600;
int toolbar_height = 30;
float bg_color[3] = {0.0f, 0.0f, 0.27f};  /* #000044 */

DesktopWindow windows[MAX_WINDOWS];
int window_count = 0;
int next_window_id = 1;
int active_window_id = -1;
float global_rotation = 0.0f;

/* Mouse interaction */
int mouse_x = 0, mouse_y = 0;
int is_dragging = 0;
int is_resizing = 0;
int drag_window_id = -1;
int drag_start_x = 0, drag_start_y = 0;
int win_drag_start_x = 0, win_drag_start_y = 0;
int win_drag_start_w = 0, win_drag_start_h = 0;

/* Context menu */
int context_menu_visible = 0;
int context_menu_x = 0, context_menu_y = 0;
int context_menu_width = 150;
int context_menu_height = 100;

/* Session paths */
char project_root[MAX_PATH] = ".";
char *session_state_path = NULL;
char *session_history_path = NULL;
char *session_view_path = NULL;
char *session_view_changed_path = NULL;
char *master_ledger_path = NULL;

long last_history_pos = 0;

/* Forward declarations */
void create_terminal_window(void);
void create_cube_window(void);
void write_view(void);
void log_master(const char* event, const char* piece);
void free_paths(void);
void write_session_state(void);
void close_window(int win_id);

void resolve_root(void) {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (!kvp) return;
    
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

void build_session_path(char* dst, size_t sz, const char* rel) {
    snprintf(dst, sz, "%s/pieces/apps/gl_os/session/%s", project_root, rel);
}

void log_master(const char* event, const char* piece) {
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[100];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    FILE *ledger = fopen(master_ledger_path, "a");
    if (ledger) {
        fprintf(ledger, "[%s] GL-OS: %s on %s\n", timestamp, event, piece);
        fclose(ledger);
    }
}

void write_view(void) {
    FILE *fp = fopen(session_view_path, "w");
    if (!fp) return;
    
    fprintf(fp, "DESKTOP_VIEW\n");
    fprintf(fp, "BG: #000044\n");
    fprintf(fp, "WINDOW_COUNT: %d\n", window_count);
    
    for (int i = 0; i < window_count; i++) {
        if (!windows[i].active) continue;
        fprintf(fp, "WINDOW | %d | %d | %d | %d | %d | %s | %s | %d\n", 
                windows[i].id, windows[i].x, windows[i].y, 
                windows[i].w, windows[i].h, 
                windows[i].title, windows[i].type, windows[i].minimized);
    }
    
    fclose(fp);
    
    /* Touch view_changed marker */
    FILE *marker = fopen(session_view_changed_path, "a");
    if (marker) { 
        fprintf(marker, "G\n"); 
        fclose(marker); 
    }
}

void write_session_state(void) {
    char win_list[MAX_LINE] = "";
    char line[256];
    
    for (int i = 0; i < window_count; i++) {
        if (!windows[i].active) continue;
        snprintf(line, sizeof(line), "%d:%s(%d,%d),", 
                 windows[i].id, windows[i].title, windows[i].x, windows[i].y);
        if (strlen(win_list) + strlen(line) < MAX_LINE - 50) {
            strcat(win_list, line);
        }
    }
    
    FILE *f = fopen(session_state_path, "w");
    if (f) {
        fprintf(f, "project_id=gl-os\n");
        fprintf(f, "session_status=active\n");
        fprintf(f, "window_count=%d\n", window_count);
        fprintf(f, "windows=%s\n", win_list);
        fprintf(f, "active_window=%d\n", active_window_id);
        fclose(f);
    }
}

void create_terminal_window(void) {
    if (window_count >= MAX_WINDOWS) return;
    
    static int term_count = 0;
    term_count++;
    
    int idx = window_count++;
    windows[idx].id = next_window_id++;
    windows[idx].x = 50 + (term_count * 20);
    windows[idx].y = 50 + (term_count * 20);
    windows[idx].w = 400;
    windows[idx].h = 300;
    windows[idx].active = 1;
    windows[idx].minimized = 0;
    windows[idx].input_buffer[0] = '\0';
    snprintf(windows[idx].title, MAX_WIN_TITLE, "Terminal %d", term_count);
    strncpy(windows[idx].type, "terminal", sizeof(windows[idx].type)-1);
    
    active_window_id = windows[idx].id;
    
    log_master("WINDOW_CREATE", "terminal");
    write_view();
    write_session_state();
}

void create_cube_window(void) {
    if (window_count >= MAX_WINDOWS) return;
    
    int idx = window_count++;
    windows[idx].id = next_window_id++;
    windows[idx].x = 100 + (window_count * 20);
    windows[idx].y = 100 + (window_count * 20);
    windows[idx].w = 300;
    windows[idx].h = 300;
    windows[idx].active = 1;
    windows[idx].minimized = 0;
    windows[idx].rotation_x = 0;
    windows[idx].rotation_y = 0;
    snprintf(windows[idx].title, MAX_WIN_TITLE, "3D Cube Test");
    strncpy(windows[idx].type, "cube", sizeof(windows[idx].type)-1);
    
    active_window_id = windows[idx].id;
    log_master("WINDOW_CREATE", "cube");
    write_view();
    write_session_state();
}

void close_window(int win_id) {
    for (int i = 0; i < window_count; i++) {
        if (windows[i].id == win_id) {
            windows[i].active = 0;
            log_master("WINDOW_CLOSE", windows[i].title);
            
            if (active_window_id == win_id) {
                active_window_id = -1;
                /* Find next active window */
                for (int j = 0; j < window_count; j++) {
                    if (windows[j].active && !windows[j].minimized) {
                        active_window_id = windows[j].id;
                        break;
                    }
                }
            }
            
            write_view();
            write_session_state();
            return;
        }
    }
}

int find_window_at(int x, int y) {
    /* Search from top (last drawn) to bottom */
    for (int i = window_count - 1; i >= 0; i--) {
        if (!windows[i].active || windows[i].minimized) continue;
        /* y is GLUT (top-down), win->y is GLUT (top-down) */
        if (x >= windows[i].x && x <= windows[i].x + windows[i].w &&
            y >= windows[i].y && y <= windows[i].y + windows[i].h) {
            return i; /* Return index */
        }
    }
    return -1;
}

int is_in_titlebar(int x, int y, DesktopWindow* win) {
    int titlebar_h = 25;
    return (y >= win->y && y <= win->y + titlebar_h);
}

void draw_rect(float x, float y, float w, float h, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void draw_cube(float size) {
    float s = size / 2.0f;
    glBegin(GL_QUADS);
    /* Front */
    glColor3f(1, 0, 0); glVertex3f(-s, -s,  s); glVertex3f( s, -s,  s); glVertex3f( s,  s,  s); glVertex3f(-s,  s,  s);
    /* Back */
    glColor3f(0, 1, 0); glVertex3f(-s, -s, -s); glVertex3f(-s,  s, -s); glVertex3f( s,  s, -s); glVertex3f( s, -s, -s);
    /* Top */
    glColor3f(0, 0, 1); glVertex3f(-s,  s, -s); glVertex3f(-s,  s,  s); glVertex3f( s,  s,  s); glVertex3f( s,  s, -s);
    /* Bottom */
    glColor3f(1, 1, 0); glVertex3f(-s, -s, -s); glVertex3f( s, -s, -s); glVertex3f( s, -s,  s); glVertex3f(-s, -s,  s);
    /* Right */
    glColor3f(1, 0, 1); glVertex3f( s, -s, -s); glVertex3f( s,  s, -s); glVertex3f( s,  s,  s); glVertex3f( s, -s,  s);
    /* Left */
    glColor3f(0, 1, 1); glVertex3f(-s, -s, -s); glVertex3f(-s, -s,  s); glVertex3f(-s,  s,  s); glVertex3f(-s,  s, -s);
    glEnd();
}

void draw_window(DesktopWindow* win) {
    if (win->minimized) return;

    int x = win->x;
    int y = window_height - win->y - win->h; /* OpenGL y (bottom-up) */
    int w = win->w;
    int h = win->h;
    
    /* Window shadow */
    draw_rect(x + 3, y - 3, w, h, 0.0f, 0.0f, 0.0f);
    
    /* Window background */
    if (win->id == active_window_id) {
        draw_rect(x, y, w, h, 0.15f, 0.15f, 0.35f);
    } else {
        draw_rect(x, y, w, h, 0.1f, 0.1f, 0.25f);
    }
    
    /* Titlebar */
    if (win->id == active_window_id) {
        draw_rect(x, y + h - 25, w, 25, 0.0f, 0.4f, 0.8f);
    } else {
        draw_rect(x, y + h - 25, w, 25, 0.3f, 0.3f, 0.5f);
    }
    
    /* Title text */
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x + 10, y + h - 18);
    for (char* p = win->title; *p; p++) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *p);
    }
    
    /* Buttons: Close(X), Minimize(_) */
    /* Close */
    draw_rect(x + w - 22, y + h - 22, 18, 18, 0.8f, 0.2f, 0.2f);
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x + w - 17, y + h - 18);
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'X');
    
    /* Minimize */
    draw_rect(x + w - 44, y + h - 22, 18, 18, 0.6f, 0.6f, 0.2f);
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x + w - 39, y + h - 18);
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, '_');

    /* Resize handle (bottom-right) */
    draw_rect(x + w - 10, y, 10, 10, 0.4f, 0.4f, 0.6f);
    
    /* Window content area */
    if (strcmp(win->type, "terminal") == 0) {
        glColor3f(0.8f, 0.8f, 0.8f);
        char prompt[300];
        snprintf(prompt, sizeof(prompt), "$ %s_", win->input_buffer);
        glRasterPos2f(x + 10, y + h - 50);
        for (char* p = prompt; *p; p++) {
            glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *p);
        }
    } else if (strcmp(win->type, "cube") == 0) {
        glEnable(GL_DEPTH_TEST);
        glClear(GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluPerspective(45.0, (double)w/(double)(h-25), 0.1, 100.0);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glTranslatef(0, 0, -5);
        glRotatef(global_rotation, 1, 1, 0);
        glEnable(GL_SCISSOR_TEST);
        glScissor(x, y, w, h - 25);
        draw_cube(2.0f);
        glDisable(GL_SCISSOR_TEST);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glDisable(GL_DEPTH_TEST);
    }
}

void draw_toolbar(void) {
    draw_rect(0, 0, window_width, toolbar_height, 0.1f, 0.1f, 0.2f);
    glColor3f(0.3f, 0.3f, 0.5f);
    glBegin(GL_LINES);
    glVertex2f(0, toolbar_height);
    glVertex2f(window_width, toolbar_height);
    glEnd();
    
    char info[128];
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char timestr[16];
    strftime(timestr, sizeof(timestr), "%H:%M:%S", timeinfo);
    snprintf(info, sizeof(info), "User: guest | Time: %s", timestr);
    
    glColor3f(0.8f, 0.8f, 1.0f);
    glRasterPos2f(10, 10);
    for (char* p = info; *p; p++) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *p);
    }
    
    int x_off = 250;
    for (int i = 0; i < window_count; i++) {
        if (!windows[i].active) continue;
        if (windows[i].minimized) {
            draw_rect(x_off, 4, 120, 22, 0.2f, 0.3f, 0.5f);
        } else if (windows[i].id == active_window_id) {
            draw_rect(x_off, 4, 120, 22, 0.3f, 0.4f, 0.7f);
        } else {
            draw_rect(x_off, 4, 120, 22, 0.15f, 0.2f, 0.35f);
        }
        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2f(x_off + 5, 10);
        char short_title[15];
        strncpy(short_title, windows[i].title, 12);
        short_title[12] = '\0';
        for (char* p = short_title; *p; p++) {
            glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p);
        }
        x_off += 130;
    }
}

void draw_context_menu(void) {
    if (!context_menu_visible) return;
    int x = context_menu_x;
    int y = window_height - context_menu_y - context_menu_height;
    int w = context_menu_width;
    int h = context_menu_height;
    draw_rect(x, y, w, h, 0.2f, 0.2f, 0.3f);
    glColor3f(0.5f, 0.5f, 0.7f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y); glVertex2f(x + w, y); glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();
    char* items[] = {"[1] New Terminal", "[2] 3D Cube App", "[3] New File", "[ESC] Cancel"};
    for (int i = 0; i < 4; i++) {
        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2f(x + 10, y + h - 25 - (i * 20));
        for (char* p = items[i]; *p; p++) glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *p);
    }
}

void render_scene(void) {
    glClearColor(bg_color[0], bg_color[1], bg_color[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, window_width, 0, window_height, -10, 10);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    for (int i = 0; i < window_count; i++) {
        if (windows[i].active) draw_window(&windows[i]);
    }
    draw_toolbar();
    draw_context_menu();
    glutSwapBuffers();
}

void timer(int value) {
    (void)value;
    global_rotation += 2.0f;
    if (global_rotation > 360.0f) global_rotation -= 360.0f;
    FILE *hf = fopen(session_history_path, "r");
    if (hf) {
        struct stat st;
        if (stat(session_history_path, &st) == 0 && st.st_size > last_history_pos) {
            fseek(hf, last_history_pos, SEEK_SET);
            char line[256];
            while (fgets(line, sizeof(line), hf)) {
                if (strstr(line, "KEY_PRESSED: ")) {
                    int key = atoi(strstr(line, "KEY_PRESSED: ") + 13);
                    if (key == 't' || key == 'T') create_terminal_window();
                    else if (key == 27) context_menu_visible = 0;
                }
            }
            last_history_pos = ftell(hf);
        }
        fclose(hf);
    }
    glutPostRedisplay();
    glutTimerFunc(30, timer, 0);
}

void keyboard(unsigned char key, int x, int y) {
    (void)x; (void)y;
    if (key == 27) context_menu_visible = 0;
    else if (key == '1') { create_terminal_window(); context_menu_visible = 0; }
    else if (key == '2') { create_cube_window(); context_menu_visible = 0; }
    else if (key == 'c' || key == 'C') { if (active_window_id != -1) close_window(active_window_id); }
    else if (active_window_id != -1) {
        for (int i = 0; i < window_count; i++) {
            if (windows[i].id == active_window_id && strcmp(windows[i].type, "terminal") == 0) {
                int len = strlen(windows[i].input_buffer);
                if (key == 13) windows[i].input_buffer[0] = '\0';
                else if (key == 8 || key == 127) { if (len > 0) windows[i].input_buffer[len-1] = '\0'; }
                else if (len < 250 && key >= 32 && key <= 126) { windows[i].input_buffer[len] = key; windows[i].input_buffer[len+1] = '\0'; }
                break;
            }
        }
    }
    FILE *hf = fopen(session_history_path, "a");
    if (hf) {
        time_t rawtime; struct tm *timeinfo; char timestamp[100];
        time(&rawtime); timeinfo = localtime(&rawtime);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
        fprintf(hf, "[%s] KEY_PRESSED: %d\n", timestamp, key); fclose(hf);
    }
    write_session_state();
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    mouse_x = x; mouse_y = y;
    if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        if (y > window_height - toolbar_height) return;
        context_menu_visible = 1; context_menu_x = x; context_menu_y = y;
        if (context_menu_x + context_menu_width > window_width) context_menu_x = window_width - context_menu_width;
        if (context_menu_y + context_menu_height > window_height) context_menu_y = window_height - context_menu_height;
        log_master("CONTEXT_MENU", "desktop");
        glutPostRedisplay();
        return;
    }
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (y > window_height - toolbar_height) {
            int x_off = 250;
            for (int i = 0; i < window_count; i++) {
                if (!windows[i].active) continue;
                if (x >= x_off && x <= x_off + 120) {
                    windows[i].minimized = !windows[i].minimized;
                    if (!windows[i].minimized) {
                        active_window_id = windows[i].id;
                        DesktopWindow win = windows[i];
                        for (int j = i; j < window_count - 1; j++) windows[j] = windows[j+1];
                        windows[window_count-1] = win;
                    } else active_window_id = -1;
                    write_session_state(); glutPostRedisplay(); return;
                }
                x_off += 130;
            }
            return;
        }
        if (context_menu_visible) {
            if (x >= context_menu_x && x <= context_menu_x + context_menu_width && y >= context_menu_y && y <= context_menu_y + context_menu_height) {
                int item = (y - context_menu_y) / 25;
                if (item == 0) create_terminal_window();
                else if (item == 1) create_cube_window();
                context_menu_visible = 0; glutPostRedisplay(); return;
            }
            context_menu_visible = 0;
        }
        int idx = find_window_at(x, y);
        if (idx != -1) {
            int win_id = windows[idx].id;
            if (idx < window_count - 1) {
                DesktopWindow clicked_win = windows[idx];
                for (int i = idx; i < window_count - 1; i++) windows[i] = windows[i + 1];
                windows[window_count - 1] = clicked_win;
                idx = window_count - 1;
            }
            active_window_id = win_id;
            if (is_in_titlebar(x, y, &windows[idx])) {
                int close_x = windows[idx].x + windows[idx].w - 22;
                int min_x = windows[idx].x + windows[idx].w - 44;
                int btn_y = windows[idx].y + 4;
                if (x >= close_x && x <= close_x + 18 && y >= btn_y && y <= btn_y + 18) close_window(win_id);
                else if (x >= min_x && x <= min_x + 18 && y >= btn_y && y <= btn_y + 18) { windows[idx].minimized = 1; active_window_id = -1; }
                else { is_dragging = 1; drag_window_id = win_id; drag_start_x = x; drag_start_y = y; win_drag_start_x = windows[idx].x; win_drag_start_y = windows[idx].y; }
            } else {
                int res_x = windows[idx].x + windows[idx].w - 10;
                int res_y = windows[idx].y + windows[idx].h - 10;
                if (x >= res_x && x <= res_x + 10 && y >= res_y && y <= res_y + 10) { is_resizing = 1; drag_window_id = win_id; drag_start_x = x; drag_start_y = y; win_drag_start_w = windows[idx].w; win_drag_start_h = windows[idx].h; }
            }
            write_session_state();
        } else active_window_id = -1;
        glutPostRedisplay();
    }
    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) { is_dragging = 0; is_resizing = 0; drag_window_id = -1; }
}

void motion(int x, int y) {
    if (drag_window_id != -1) {
        int dx = x - drag_start_x; int dy = y - drag_start_y;
        if (is_dragging) {
            windows[window_count - 1].x = win_drag_start_x + dx;
            windows[window_count - 1].y = win_drag_start_y + dy;
            if (windows[window_count - 1].y < 0) windows[window_count - 1].y = 0;
            if (windows[window_count - 1].y > window_height - toolbar_height - 25) windows[window_count - 1].y = window_height - toolbar_height - 25;
        } else if (is_resizing) {
            windows[window_count - 1].w = win_drag_start_w + dx;
            windows[window_count - 1].h = win_drag_start_h + dy;
            if (windows[window_count - 1].w < 100) windows[window_count - 1].w = 100;
            if (windows[window_count - 1].h < 50) windows[window_count - 1].h = 50;
        }
        write_session_state(); glutPostRedisplay();
    }
}

void reshape(int w, int h) { window_width = w; window_height = h; glViewport(0, 0, w, h); }

void free_paths(void) {
    if (session_state_path) free(session_state_path); if (session_history_path) free(session_history_path);
    if (session_view_path) free(session_view_path); if (session_view_changed_path) free(session_view_changed_path);
    if (master_ledger_path) free(master_ledger_path);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv); glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(window_width, window_height); glutCreateWindow("GL-OS Desktop");
    resolve_root();
    asprintf(&session_state_path, "%s/pieces/apps/gl_os/session/state.txt", project_root);
    asprintf(&session_history_path, "%s/pieces/apps/gl_os/session/history.txt", project_root);
    asprintf(&session_view_path, "%s/pieces/apps/gl_os/session/view.txt", project_root);
    asprintf(&session_view_changed_path, "%s/pieces/apps/gl_os/session/view_changed.txt", project_root);
    asprintf(&master_ledger_path, "%s/pieces/master_ledger/master_ledger.txt", project_root);
    FILE *f; f = fopen(session_state_path, "a"); if (f) fclose(f);
    f = fopen(session_history_path, "a"); if (f) fclose(f);
    f = fopen(session_view_path, "a"); if (f) fclose(f);
    create_terminal_window();
    log_master("DESKTOP_START", "gl-os");
    glutDisplayFunc(render_scene); glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard); glutMouseFunc(mouse);
    glutMotionFunc(motion); glutTimerFunc(30, timer, 0);
    glutMainLoop(); free_paths(); return 0;
}
