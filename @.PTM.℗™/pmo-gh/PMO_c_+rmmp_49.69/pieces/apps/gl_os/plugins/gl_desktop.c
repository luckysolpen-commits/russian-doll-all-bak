/*
 * gl_desktop.c - GL-OS Desktop Environment
 * Opens OpenGL window with desktop, windows, and context menu
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
    char type[32];  /* "terminal", "folder", "file" */
    int active;
    int is_dragging;
    int drag_offset_x, drag_offset_y;
} DesktopWindow;

/* Global state */
int window_width = 800;
int window_height = 600;
float bg_color[3] = {0.0f, 0.0f, 0.27f};  /* #000044 */

DesktopWindow windows[MAX_WINDOWS];
int window_count = 0;
int next_window_id = 1;
int active_window_id = -1;

/* Mouse interaction */
int mouse_x = 0, mouse_y = 0;
int is_dragging = 0;
int drag_window_id = -1;
int drag_start_x = 0, drag_start_y = 0;
int win_drag_start_x = 0, win_drag_start_y = 0;

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
void write_view(void);
void log_master(const char* event, const char* piece);
void free_paths(void);

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
        fprintf(fp, "WINDOW | %d | %d | %d | %d | %d | %s | %s\n", 
                windows[i].id, windows[i].x, windows[i].y, 
                windows[i].w, windows[i].h, 
                windows[i].title, windows[i].type);
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
    snprintf(windows[idx].title, MAX_WIN_TITLE, "Terminal %d", term_count);
    strncpy(windows[idx].type, "terminal", sizeof(windows[idx].type)-1);
    
    active_window_id = windows[idx].id;
    
    log_master("WINDOW_CREATE", "terminal");
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
                    if (windows[j].active) {
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
        if (!windows[i].active) continue;
        if (x >= windows[i].x && x <= windows[i].x + windows[i].w &&
            y >= (window_height - windows[i].y - windows[i].h) && 
            y <= (window_height - windows[i].y)) {
            return windows[i].id;
        }
    }
    return -1;
}

int is_in_titlebar(int x, int y, DesktopWindow* win) {
    int titlebar_h = 25;
    int win_top = window_height - win->y;
    int win_bottom = win_top - titlebar_h;
    (void)x;  /* x check done in caller */
    return (y >= win_bottom && y <= win_top);
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

void draw_window(DesktopWindow* win) {
    int x = win->x;
    int y = window_height - win->y - win->h;
    int w = win->w;
    int h = win->h;
    
    /* Window shadow */
    draw_rect(x + 3, y - 3, w, h, 0.0f, 0.0f, 0.0f);
    
    /* Window background */
    if (win->id == active_window_id) {
        draw_rect(x, y, w, h, 0.15f, 0.15f, 0.35f);  /* Active: lighter blue */
    } else {
        draw_rect(x, y, w, h, 0.1f, 0.1f, 0.25f);  /* Inactive: darker blue */
    }
    
    /* Titlebar */
    if (win->id == active_window_id) {
        draw_rect(x, y + h - 25, w, 25, 0.0f, 0.4f, 0.8f);  /* Active: bright blue */
    } else {
        draw_rect(x, y + h - 25, w, 25, 0.3f, 0.3f, 0.5f);  /* Inactive: gray-blue */
    }
    
    /* Title text */
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x + 10, y + h - 8);
    for (char* p = win->title; *p; p++) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *p);
    }
    
    /* Close button (X) */
    draw_rect(x + w - 22, y + h - 22, 18, 18, 0.8f, 0.2f, 0.2f);
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x + w - 18, y + h - 7);
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'X');
    
    /* Window content area - show terminal-like text */
    if (strcmp(win->type, "terminal") == 0) {
        glColor3f(0.8f, 0.8f, 0.8f);
        char* prompt = "$ _";
        glRasterPos2f(x + 10, y + h - 50);
        for (char* p = prompt; *p; p++) {
            glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *p);
        }
    }
}

void draw_context_menu(void) {
    if (!context_menu_visible) return;
    
    int x = context_menu_x;
    int y = context_menu_y;
    int w = context_menu_width;
    int h = context_menu_height;
    
    /* Menu background */
    draw_rect(x, y, w, h, 0.2f, 0.2f, 0.3f);
    
    /* Menu border */
    glColor3f(0.5f, 0.5f, 0.7f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
    
    /* Menu items */
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x + 10, y + h - 25);
    char* items[] = {"[1] New Terminal", "[2] New Folder", "[3] New File", "[ESC] Cancel"};
    for (int i = 0; i < 4; i++) {
        glRasterPos2f(x + 10, y + h - 25 - (i * 20));
        for (char* p = items[i]; *p; p++) {
            glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *p);
        }
    }
}

void render_scene(void) {
    glClearColor(bg_color[0], bg_color[1], bg_color[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, window_width, 0, window_height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    /* Draw all windows */
    for (int i = 0; i < window_count; i++) {
        if (windows[i].active) {
            draw_window(&windows[i]);
        }
    }
    
    /* Draw context menu */
    draw_context_menu();
    
    glutSwapBuffers();
}

void timer(int value) {
    (void)value;
    /* Check for new input in session history */
    FILE *hf = fopen(session_history_path, "r");
    if (hf) {
        struct stat st;
        if (stat(session_history_path, &st) == 0 && st.st_size > last_history_pos) {
            fseek(hf, last_history_pos, SEEK_SET);
            
            char line[256];
            while (fgets(line, sizeof(line), hf)) {
                if (strstr(line, "KEY_PRESSED: ")) {
                    int key = atoi(strstr(line, "KEY_PRESSED: ") + 13);
                    if (key == 't' || key == 'T') {
                        create_terminal_window();
                    } else if (key == 27) {  /* ESC */
                        context_menu_visible = 0;
                        write_session_state();
                    }
                }
            }
            
            last_history_pos = ftell(hf);
        }
        fclose(hf);
    }
    
    glutPostRedisplay();
    glutTimerFunc(50, timer, 0);
}

void keyboard(unsigned char key, int x, int y) {
    (void)x; (void)y;
    if (key == 27) {  /* ESC */
        context_menu_visible = 0;
        write_session_state();
    } else if (key == 't' || key == 'T') {
        create_terminal_window();
    } else if (key == 'f' || key == 'F') {
        /* New folder - TODO */
    } else if (key == 'n' || key == 'N') {
        /* New file - TODO */
    } else if (key == 'c' || key == 'C') {
        /* Close active window */
        if (active_window_id != -1) {
            close_window(active_window_id);
        }
    }
    
    /* Log key to session history */
    FILE *hf = fopen(session_history_path, "a");
    if (hf) {
        time_t rawtime;
        struct tm *timeinfo;
        char timestamp[100];
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
        fprintf(hf, "[%s] KEY_PRESSED: %d\n", timestamp, key);
        fclose(hf);
    }
}

void mouse(int button, int state, int x, int y) {
    mouse_x = x;
    mouse_y = y;  /* GLUT y is from top */
    
    if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        /* Show context menu */
        context_menu_visible = 1;
        context_menu_x = x;
        context_menu_y = window_height - y - context_menu_height;
        
        /* Keep menu on screen */
        if (context_menu_x + context_menu_width > window_width) {
            context_menu_x = window_width - context_menu_width;
        }
        if (context_menu_y < 0) {
            context_menu_y = 0;
        }
        
        log_master("CONTEXT_MENU", "desktop");
        glutPostRedisplay();
        return;
    }
    
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        context_menu_visible = 0;
        
        /* Check if clicking on a window */
        int win_id = find_window_at(x, y);
        
        if (win_id != -1) {
            /* Find the window */
            for (int i = 0; i < window_count; i++) {
                if (windows[i].id == win_id) {
                    /* Check if clicking titlebar */
                    if (is_in_titlebar(x, y, &windows[i])) {
                        is_dragging = 1;
                        drag_window_id = win_id;
                        drag_start_x = x;
                        drag_start_y = y;
                        win_drag_start_x = windows[i].x;
                        win_drag_start_y = windows[i].y;
                    }
                    
                    /* Check if clicking close button */
                    int close_x = windows[i].x + windows[i].w - 22;
                    int close_y = window_height - windows[i].y - 22;
                    if (x >= close_x && x <= close_x + 18 && y >= close_y && y <= close_y + 18) {
                        close_window(win_id);
                        is_dragging = 0;
                    }
                    
                    /* Make this window active */
                    active_window_id = win_id;
                    write_session_state();
                    break;
                }
            }
        }
        
        glutPostRedisplay();
    }
    
    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        is_dragging = 0;
        drag_window_id = -1;
    }
}

void motion(int x, int y) {
    if (is_dragging && drag_window_id != -1) {
        int dx = x - drag_start_x;
        int dy = y - drag_start_y;
        
        /* Find and move the window */
        for (int i = 0; i < window_count; i++) {
            if (windows[i].id == drag_window_id) {
                windows[i].x = win_drag_start_x + dx;
                windows[i].y = win_drag_start_y - dy;  /* Invert y for OpenGL coords */
                
                /* Keep window on screen */
                if (windows[i].x < 0) windows[i].x = 0;
                if (windows[i].y < 0) windows[i].y = 0;
                if (windows[i].x + windows[i].w > window_width) {
                    windows[i].x = window_width - windows[i].w;
                }
                if (windows[i].y + windows[i].h > window_height) {
                    windows[i].y = window_height - windows[i].h;
                }
                
                write_session_state();
                glutPostRedisplay();
                break;
            }
        }
    }
}

void reshape(int w, int h) {
    window_width = w;
    window_height = h;
    glViewport(0, 0, w, h);
}

void free_paths(void) {
    if (session_state_path) free(session_state_path);
    if (session_history_path) free(session_history_path);
    if (session_view_path) free(session_view_path);
    if (session_view_changed_path) free(session_view_changed_path);
    if (master_ledger_path) free(master_ledger_path);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("GL-OS Desktop");
    
    resolve_root();
    
    /* Allocate paths dynamically to avoid truncation warnings */
    asprintf(&session_state_path, "%s/pieces/apps/gl_os/session/state.txt", project_root);
    asprintf(&session_history_path, "%s/pieces/apps/gl_os/session/history.txt", project_root);
    asprintf(&session_view_path, "%s/pieces/apps/gl_os/session/view.txt", project_root);
    asprintf(&session_view_changed_path, "%s/pieces/apps/gl_os/session/view_changed.txt", project_root);
    asprintf(&master_ledger_path, "%s/pieces/master_ledger/master_ledger.txt", project_root);
    
    /* Ensure session files exist */
    FILE *f;
    f = fopen(session_state_path, "a"); if (f) fclose(f);
    f = fopen(session_history_path, "a"); if (f) fclose(f);
    f = fopen(session_view_path, "a"); if (f) fclose(f);
    
    /* Initialize with one terminal window */
    create_terminal_window();
    
    log_master("DESKTOP_START", "gl-os");
    
    glutDisplayFunc(render_scene);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutTimerFunc(50, timer, 0);
    
    glutMainLoop();
    
    free_paths();
    return 0;
}
