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
#define MAX_TERM_OPTIONS 10
#define MAX_OPTION_LEN 80

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
    char output_history[12][80];
    int history_count;
    float rotation_x, rotation_y;

    /* Terminal menu navigation state (joystick-first) */
    int selected_index;        /* Currently highlighted option (0-based) */
    int show_menu;             /* 1 = showing menu options, 0 = text input mode */
    int option_count;          /* Number of menu options */
    char menu_options[MAX_TERM_OPTIONS][MAX_OPTION_LEN];  /* Option labels */
    
    /* Thumb scroll state (Linux terminal behavior) */
    int scroll_offset;         /* Lines scrolled UP from bottom (0 = at bottom) */
    int max_scroll;            /* Maximum scroll offset (history_count - visible_lines) */
} DesktopWindow;

/* Global state */
int window_width = 800;
int window_height = 600;
int toolbar_height = 30;
float bg_color[3] = {0.0f, 0.0f, 0.27f};  /* #000044 */
int cursor_blink = 0;

DesktopWindow windows[MAX_WINDOWS];
int window_count = 0;
int next_window_id = 1;
int active_window_id = -1;
float global_rotation = 0.0f;

/* Joystick state (global for edge-triggering) */
int last_joy_x = 0;
int last_joy_y = 0;
unsigned int last_joy_buttons = 0;

/* Mouse interaction */
int mouse_x = 0, mouse_y = 0;
int is_dragging = 0;
int is_resizing = 0;
int drag_window_id = -1;
int drag_start_x = 0, drag_start_y = 0;
int win_drag_start_x = 0, win_drag_start_y = 0;
int win_drag_start_w = 0, win_drag_start_h = 0;

/* Scroll thumb dragging (Linux terminal behavior) */
int is_scroll_thumb_drag = 0;
int scroll_window_id = -1;
int scroll_track_y = 0;
int scroll_track_h = 0;

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
char *input_focus_lock_path = NULL;  /* Input focus lock (TPM isolation) */

long last_history_pos = 0;

/* Forward declarations */
void create_terminal_window(void);
void create_cube_window(void);
void write_view(void);
void log_master(const char* event, const char* piece);
void free_paths(void);
void write_session_state(void);
void close_window(int win_id);
void init_terminal_menu(DesktopWindow* win);
void execute_option(DesktopWindow* win, int index);
void move_selection(DesktopWindow* win, int direction);
void draw_text_clipped(float x, float y, float max_width, const char* text);
void draw_scroll_thumb(float x, float y, float h, int scroll_offset, int max_scroll, int visible_lines);

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

void add_history(DesktopWindow* win, const char* text) {
    if (win->history_count < 12) {
        strncpy(win->output_history[win->history_count++], text, 79);
    } else {
        for (int i = 0; i < 11; i++) strcpy(win->output_history[i], win->output_history[i+1]);
        strncpy(win->output_history[11], text, 79);
    }
}

void init_terminal_menu(DesktopWindow* win) {
    /* Initialize default menu options (joystick-first UX) */
    win->option_count = 0;
    win->selected_index = 0;
    win->show_menu = 1;
    
    /* Initialize scroll state (Linux terminal: thumb at bottom = newest) */
    win->scroll_offset = 0;      /* Start at bottom (newest content) */
    win->max_scroll = 0;         /* Will be calculated during render */
    
    /* Default menu options - auto-show on terminal open */
    snprintf(win->menu_options[win->option_count++], MAX_OPTION_LEN, 
             "Desktop Apps - Apps inside GL-OS");
    snprintf(win->menu_options[win->option_count++], MAX_OPTION_LEN, 
             "Terminal Apps - CHTPM CLI Apps");
    snprintf(win->menu_options[win->option_count++], MAX_OPTION_LEN, 
             "Cube Demo - Launch 3D Demo");
}

void execute_option(DesktopWindow* win, int index) {
    if (index < 0 || index >= win->option_count) return;
    
    /* Add selection to history */
    char selection[200];
    snprintf(selection, sizeof(selection), "[EXECUTE] %s", win->menu_options[index]);
    add_history(win, selection);
    
    /* Execute based on selected option */
    if (index == 0) {
        /* Desktop Apps */
        add_history(win, "Desktop Apps:");
        add_history(win, "  - cube (Launch 3D Demo)");
        add_history(win, "  - folder (WIP)");
    } else if (index == 1) {
        /* Terminal Apps */
        add_history(win, "Terminal Apps:");
        add_history(win, "  - fuzzpet");
        add_history(win, "  - projects");
        add_history(win, "  - proc (Monitor)");
    } else if (index == 2) {
        /* Cube Demo */
        add_history(win, "Launching 3D Cube App...");
        create_cube_window();
    }
    
    /* Clear input buffer after execution */
    win->input_buffer[0] = '\0';
}

void move_selection(DesktopWindow* win, int direction) {
    if (!win->show_menu) return;
    
    win->selected_index += direction;
    if (win->selected_index < 0) win->selected_index = 0;
    if (win->selected_index >= win->option_count) win->selected_index = win->option_count - 1;
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
    windows[idx].history_count = 0;
    snprintf(windows[idx].title, MAX_WIN_TITLE, "Terminal %d", term_count);
    strncpy(windows[idx].type, "terminal", sizeof(windows[idx].type)-1);

    /* Initialize menu navigation state (joystick-first UX) */
    init_terminal_menu(&windows[idx]);

    add_history(&windows[idx], "GL-OS Virtual Terminal v0.02");
    add_history(&windows[idx], "Navigate with joystick or number keys, ENTER to select.");

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

void process_terminal_command(DesktopWindow* win) {
    char cmd[256];
    strncpy(cmd, win->input_buffer, 255);
    win->input_buffer[0] = '\0';
    
    char prompt[300];
    snprintf(prompt, sizeof(prompt), "$ %s", cmd);
    add_history(win, prompt);
    
    if (strcmp(cmd, "help") == 0) {
        add_history(win, "Available Categories:");
        add_history(win, "  []1.desktop  - Apps inside GL-OS");
        add_history(win, "  []2.terminal - CHTPM CLI Apps");
    } else if (strcmp(cmd, "1") == 0 || strcmp(cmd, "desktop") == 0) {
        add_history(win, "Desktop Apps:");
        add_history(win, "  - cube (Launch 3D Demo)");
        add_history(win, "  - folder (WIP)");
    } else if (strcmp(cmd, "2") == 0 || strcmp(cmd, "terminal") == 0) {
        add_history(win, "Terminal Apps:");
        add_history(win, "  - fuzzpet");
        add_history(win, "  - projects");
        add_history(win, "  - proc (Monitor)");
    } else if (strcmp(cmd, "cube") == 0) {
        add_history(win, "Launching 3D Cube App...");
        create_cube_window();
    } else if (strcmp(cmd, "clear") == 0) {
        win->history_count = 0;
    } else if (strlen(cmd) > 0) {
        add_history(win, "Unknown command. Type 'help' for options.");
    }
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

/* Draw text clipped to max_width - adds "..." if too long */
void draw_text_clipped(float x, float y, float max_width, const char* text) {
    /* Measure text width (approximate: 9 pixels per char for 9x15 font) */
    int text_width = strlen(text) * 9;
    
    if (text_width <= max_width) {
        /* Text fits - draw normally */
        glRasterPos2f(x, y);
        for (const char* p = text; *p; p++)
            glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *p);
    } else {
        /* Text too long - clip and add "..." */
        int max_chars = max_width / 9;
        if (max_chars > 3) {
            char clipped[256];
            strncpy(clipped, text, max_chars - 3);
            clipped[max_chars - 3] = '\0';
            strcat(clipped, "...");
            
            glRasterPos2f(x, y);
            for (const char* p = clipped; *p; p++)
                glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *p);
        }
    }
}

/* Draw scroll thumb on right edge (Linux terminal behavior)
 * thumb at BOTTOM = viewing newest content (scroll_offset = 0)
 * thumb at TOP = viewing oldest content (scroll_offset = max_scroll)
 * y is the BOTTOM of the scroll track (OpenGL coords, y increases upward)
 * Stores track position for mouse click detection
 */
void draw_scroll_thumb(float x, float y, float h, int scroll_offset, int max_scroll, int visible_lines) {
    /* Store track position for mouse interaction */
    scroll_track_y = (int)y;
    scroll_track_h = (int)h;
    
    if (max_scroll <= 0) return;  /* No scroll needed */
    
    /* Thumb height proportional to visible/total (min 10%) */
    int total_lines = max_scroll + visible_lines;
    float thumb_h_ratio = (float)visible_lines / (float)total_lines;
    if (thumb_h_ratio < 0.1f) thumb_h_ratio = 0.1f;
    
    int thumb_h = (int)(h * thumb_h_ratio);
    if (thumb_h < 10) thumb_h = 10;
    
    /* Thumb position: 0.0 = bottom (newest), 1.0 = top (oldest) */
    float thumb_pos = (max_scroll > 0) ? ((float)scroll_offset / (float)max_scroll) : 0.0f;
    
    /* Y position: y is BOTTOM, so thumb at bottom when thumb_pos = 0 */
    int track_h = (int)h - thumb_h;
    int thumb_y = (int)(y + track_h * thumb_pos);  /* y + 0 = bottom, y + track_h = top */
    
    /* Draw thumb track (background) */
    glColor3f(0.3f, 0.3f, 0.3f);  /* Dark grey track */
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + 8, y);
    glVertex2f(x + 8, y + h);
    glVertex2f(x, y + h);
    glEnd();
    
    /* Draw thumb */
    if (scroll_offset == 0) {
        glColor3f(0.5f, 0.5f, 0.5f);  /* Grey - at bottom */
    } else {
        glColor3f(0.7f, 0.7f, 0.7f);  /* Lighter grey - scrolled up */
    }
    glBegin(GL_QUADS);
    glVertex2f(x + 1, thumb_y);
    glVertex2f(x + 7, thumb_y);
    glVertex2f(x + 7, thumb_y + thumb_h);
    glVertex2f(x + 1, thumb_y + thumb_h);
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

    /* Title text (clipped to titlebar width) */
    glColor3f(1.0f, 1.0f, 1.0f);
    draw_text_clipped(x + 10, y + h - 18, w - 100, win->title);

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

    /* Resize handle (bottom-right) - Linux style */
    draw_rect(x + w - 15, y, 15, 15, 0.6f, 0.6f, 0.8f);  /* Lighter blue - more visible */
    /* Draw diagonal lines on resize handle */
    glColor3f(0.8f, 0.8f, 1.0f);
    glBegin(GL_LINES);
    glVertex2f(x + w - 13, y + 3); glVertex2f(x + w - 3, y + 3);
    glVertex2f(x + w - 13, y + 3); glVertex2f(x + w - 13, y + 13);
    glVertex2f(x + w - 10, y + 6); glVertex2f(x + w - 3, y + 6);
    glVertex2f(x + w - 10, y + 6); glVertex2f(x + w - 10, y + 13);
    glEnd();
    
    /* Window content area */
    if (strcmp(win->type, "terminal") == 0) {
        /* Layout (top to bottom visually):
         * [Title bar]
         * [Menu header - blue]
         * [Menu options - yellow selected, grey unselected]
         * [Prompt - $ _]
         * [History - grey, scrolling from bottom]
         */
        
        int content_top = y + h - 30;  /* Below titlebar */
        int content_bottom = y + 10;   /* Above window floor */
        int line_height = 18;
        int x_start = x + 10;
        
        /* Calculate layout positions */
        int menu_y = content_top;
        int option_count = win->show_menu ? win->option_count : 0;
        int menu_height = (option_count > 0) ? (30 + option_count * line_height) : 0;
        int prompt_y = menu_y - menu_height - 10;
        int history_top_y = prompt_y - 25;
        
        /* Calculate scroll state */
        int history_area_h = history_top_y - content_bottom;
        int visible_lines = history_area_h / line_height;
        win->max_scroll = (win->history_count > visible_lines) ? 
                          (win->history_count - visible_lines) : 0;
        
        /* Clamp scroll offset */
        if (win->scroll_offset > win->max_scroll) win->scroll_offset = win->max_scroll;
        if (win->scroll_offset < 0) win->scroll_offset = 0;
        
        /* Draw menu (if enabled) */
        if (option_count > 0) {
            /* Menu header */
            glColor3f(0.5f, 0.8f, 1.0f);  /* Blue */
            const char header[] = "=== GL-OS Menu ===";
            draw_text_clipped(x_start, menu_y, w - 40, header);
            
            /* Menu options */
            int opt_y = menu_y - 25;
            for (int i = 0; i < option_count; i++) {
                if (i == win->selected_index) {
                    glColor3f(1.0f, 1.0f, 0.5f);  /* Yellow - selected */
                    char line[MAX_OPTION_LEN + 20];
                    snprintf(line, sizeof(line), "[>] %d. %s", i+1, win->menu_options[i]);
                    draw_text_clipped(x_start, opt_y - (i * line_height), w - 40, line);
                } else {
                    glColor3f(0.7f, 0.7f, 0.7f);  /* Grey - unselected */
                    char line[MAX_OPTION_LEN + 20];
                    snprintf(line, sizeof(line), "[ ] %d. %s", i+1, win->menu_options[i]);
                    draw_text_clipped(x_start, opt_y - (i * line_height), w - 40, line);
                }
            }
        }
        
        /* Draw prompt */
        glColor3f(0.8f, 0.8f, 0.8f);  /* Light grey */
        char prompt[300];
        if (cursor_blink < 10)
            snprintf(prompt, sizeof(prompt), "$ %s_", win->input_buffer);
        else
            snprintf(prompt, sizeof(prompt), "$ %s ", win->input_buffer);
        
        draw_text_clipped(x_start, prompt_y, w - 40, prompt);
        
        /* Draw history (from bottom up, with scroll offset)
         * scroll_offset = 0 → show NEWEST lines (at bottom, near prompt)
         * scroll_offset = max → show OLDEST lines (at top)
         * history[0] = oldest, history[history_count-1] = newest
         */
        int hist_y = content_bottom;
        /* Start index: when scroll_offset=0, start from (history_count - visible_lines) */
        int start_idx = win->history_count - visible_lines - win->scroll_offset;
        if (start_idx < 0) start_idx = 0;  /* Don't go negative */
        
        int lines_drawn = 0;
        for (int i = start_idx; i < win->history_count && lines_drawn < visible_lines; i++) {
            if (hist_y > history_top_y) break;  /* Don't draw into menu area */
            glColor3f(0.6f, 0.6f, 0.6f);  /* Dim grey */
            draw_text_clipped(x_start, hist_y, w - 40, win->output_history[i]);
            hist_y += line_height;
            lines_drawn++;
        }
        
        /* Draw scroll thumb on right edge */
        draw_scroll_thumb(x + w - 12, content_bottom, history_top_y - content_bottom,
                         win->scroll_offset, win->max_scroll, visible_lines);
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

    cursor_blink++;
    if (cursor_blink >= 20) cursor_blink = 0;

    /* Force GLUT to poll joystick (required for continuous polling) */
    glutForceJoystickFunc();

    /* Removed: CHTPM history polling - GL-OS now uses direct GLUT input */
    /* Joystick: handled by joystick() callback (direct GLUT) */
    /* Keyboard: handled by keyboard() callback (direct GLUT) */
    /* GL-OS is completely isolated from CHTPM input systems */

    glutPostRedisplay();
    glutTimerFunc(30, timer, 0);
}

void joystick(unsigned int buttonMask, int x, int y, int z) {
    (void)y; (void)z;

    /* Joystick navigation for terminal menu (joystick-first UX) */
    if (active_window_id != -1) {
        for (int i = 0; i < window_count; i++) {
            if (windows[i].id == active_window_id && strcmp(windows[i].type, "terminal") == 0) {
                /* X axis: -1000 to 1000 (left to right) - edge-triggered */
                if (x < -500 && last_joy_x >= -500) {
                    move_selection(&windows[i], -1);  /* Left: up */
                } else if (x > 500 && last_joy_x <= 500) {
                    move_selection(&windows[i], 1);   /* Right: down */
                }
                last_joy_x = x;

                /* Y axis: -1000 to 1000 (up/down) - edge-triggered */
                /* If menu is showing, navigate menu. Otherwise, scroll history */
                if (windows[i].show_menu && windows[i].option_count > 0) {
                    /* Menu navigation mode */
                    if (y < -500 && last_joy_y >= -500) {
                        move_selection(&windows[i], -1);  /* Up: up */
                    } else if (y > 500 && last_joy_y <= 500) {
                        move_selection(&windows[i], 1);   /* Down: down */
                    }

                    /* Button A: execute - edge-triggered */
                    if ((buttonMask & GLUT_JOYSTICK_BUTTON_A) &&
                        !(last_joy_buttons & GLUT_JOYSTICK_BUTTON_A)) {
                        execute_option(&windows[i], windows[i].selected_index);
                    }
                } else {
                    /* Scroll mode - navigate history with Y axis */
                    /* Joystick UP → scroll up → see OLDER content */
                    if (y < -500 && last_joy_y >= -500) {
                        windows[i].scroll_offset++;
                        if (windows[i].scroll_offset > windows[i].max_scroll)
                            windows[i].scroll_offset = windows[i].max_scroll;
                    }
                    /* Joystick DOWN → scroll down → see NEWER content (return to bottom) */
                    else if (y > 500 && last_joy_y <= 500) {
                        windows[i].scroll_offset--;
                        if (windows[i].scroll_offset < 0)
                            windows[i].scroll_offset = 0;
                    }
                }
                last_joy_y = y;
                last_joy_buttons = buttonMask;
                break;
            }
        }
    }

    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    (void)x; (void)y;

    /* Check if a terminal window is currently focused */
    int terminal_focused = 0;
    if (active_window_id != -1) {
        for (int i = 0; i < window_count; i++) {
            if (windows[i].id == active_window_id && strcmp(windows[i].type, "terminal") == 0) {
                terminal_focused = 1;
                break;
            }
        }
    }

    /* ESC always closes context menu */
    if (key == 27) {
        context_menu_visible = 0;
    }
    /* Hotkeys only work when NO terminal is focused */
    else if (!terminal_focused) {
        if (key == '1') { create_terminal_window(); context_menu_visible = 0; }
        else if (key == '2') { create_cube_window(); context_menu_visible = 0; }
        else if (key == 'c' || key == 'C') { if (active_window_id != -1) close_window(active_window_id); }
    }

    /* Terminal input - only process if terminal is focused */
    if (terminal_focused) {
        for (int i = 0; i < window_count; i++) {
            if (windows[i].id == active_window_id && strcmp(windows[i].type, "terminal") == 0) {
                int len = strlen(windows[i].input_buffer);
                
                /* Menu navigation: number keys select option (don't execute) */
                if (windows[i].show_menu && key >= '1' && key <= '9') {
                    int idx = key - '1';
                    if (idx >= 0 && idx < windows[i].option_count) {
                        windows[i].selected_index = idx;
                        glutPostRedisplay();
                        break;
                    }
                }
                
                /* ENTER: execute selected option (if menu) or command (if text) */
                if (key == 13) {
                    if (windows[i].show_menu && windows[i].option_count > 0) {
                        /* Execute selected menu option */
                        execute_option(&windows[i], windows[i].selected_index);
                    } else {
                        /* Execute text command */
                        process_terminal_command(&windows[i]);
                    }
                } else if (key == 8 || key == 127) {
                    if (len > 0) windows[i].input_buffer[len-1] = '\0';
                } else if (len < 250 && key >= 32 && key <= 126) {
                    windows[i].input_buffer[len] = key;
                    windows[i].input_buffer[len+1] = '\0';
                }
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
            
            /* Check if clicking on scroll thumb (right edge of terminal window) */
            if (strcmp(windows[idx].type, "terminal") == 0) {
                int thumb_x = windows[idx].x + windows[idx].w - 12;
                int thumb_right = windows[idx].x + windows[idx].w - 4;
                /* Check if click is in thumb track area */
                if (x >= thumb_x && x <= thumb_right && y >= scroll_track_y && y <= scroll_track_y + scroll_track_h) {
                    is_scroll_thumb_drag = 1;
                    scroll_window_id = win_id;
                    glutPostRedisplay();
                    return;
                }
            }
            
            if (is_in_titlebar(x, y, &windows[idx])) {
                int close_x = windows[idx].x + windows[idx].w - 22;
                int min_x = windows[idx].x + windows[idx].w - 44;
                int btn_y = windows[idx].y + 4;
                if (x >= close_x && x <= close_x + 18 && y >= btn_y && y <= btn_y + 18) close_window(win_id);
                else if (x >= min_x && x <= min_x + 18 && y >= btn_y && y <= btn_y + 18) { windows[idx].minimized = 1; active_window_id = -1; }
                else { is_dragging = 1; drag_window_id = win_id; drag_start_x = x; drag_start_y = y; win_drag_start_x = windows[idx].x; win_drag_start_y = windows[idx].y; }
            } else {
                int res_x = windows[idx].x + windows[idx].w - 15;  /* Match resize handle size */
                int res_y = windows[idx].y + windows[idx].h - 15;
                if (x >= res_x && x <= res_x + 15 && y >= res_y && y <= res_y + 15) { is_resizing = 1; drag_window_id = win_id; drag_start_x = x; drag_start_y = y; win_drag_start_w = windows[idx].w; win_drag_start_h = windows[idx].h; }
            }
            write_session_state();
        } else active_window_id = -1;
        glutPostRedisplay();
    }
    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        is_dragging = 0; is_resizing = 0; drag_window_id = -1;
        is_scroll_thumb_drag = 0; scroll_window_id = -1;
    }
}

void motion(int x, int y) {
    /* Handle scroll thumb dragging */
    if (is_scroll_thumb_drag && scroll_window_id != -1) {
        for (int i = 0; i < window_count; i++) {
            if (windows[i].id == scroll_window_id && strcmp(windows[i].type, "terminal") == 0) {
                /* Calculate scroll based on mouse Y position in thumb track
                 * Linux terminal behavior: drag thumb UP → see OLDER content (scroll_offset increases)
                 * drag thumb DOWN → see NEWER content (scroll_offset decreases to 0)
                 * GLUT y is top-down, so smaller y = higher on screen
                 */
                int track_y = scroll_track_y;
                int track_h = scroll_track_h;
                
                /* Thumb height (same as in draw_scroll_thumb) */
                int visible_lines = (track_h - 20) / 18;
                int total_lines = windows[i].max_scroll + visible_lines;
                float thumb_h_ratio = (float)visible_lines / (float)total_lines;
                if (thumb_h_ratio < 0.1f) thumb_h_ratio = 0.1f;
                int thumb_h = (int)(track_h * thumb_h_ratio);
                if (thumb_h < 10) thumb_h = 10;
                
                /* Calculate new scroll position from mouse Y
                 * Mouse y is GLUT (top-down), track_y is bottom of track
                 * Drag UP (smaller y) → increase scroll_offset (see older)
                 * Drag DOWN (larger y) → decrease scroll_offset (see newer)
                 */
                int track_h_adj = track_h - thumb_h;
                /* Invert: mouse at top (small y) = max_scroll, mouse at bottom (large y) = 0 */
                float mouse_ratio = 1.0f - ((float)(y - track_y - thumb_h/2) / (float)track_h_adj);
                if (mouse_ratio < 0.0f) mouse_ratio = 0.0f;
                if (mouse_ratio > 1.0f) mouse_ratio = 1.0f;
                
                windows[i].scroll_offset = (int)(mouse_ratio * windows[i].max_scroll);
                glutPostRedisplay();
                return;
            }
        }
    }
    
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
    if (input_focus_lock_path) free(input_focus_lock_path);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv); glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(window_width, window_height); glutCreateWindow("GL-OS Desktop");
    resolve_root();
    asprintf(&session_state_path, "%s/pieces/apps/gl_os/session/state.txt", project_root);
    asprintf(&session_history_path, "%s/pieces/apps/gl_os/session/history.txt", project_root);
    asprintf(&session_view_path, "%s/pieces/apps/gl_os/session/view.txt", project_root);
    asprintf(&session_view_changed_path, "%s/pieces/apps/gl_os/session/view_changed.txt", project_root);
    asprintf(&master_ledger_path, "%s/pieces/apps/gl_os/ledger/master_ledger.txt", project_root);
    asprintf(&input_focus_lock_path, "%s/pieces/apps/gl_os/session/input_focus.lock", project_root);
    
    /* Create input focus lock - tells CHTPM daemons to stop writing input */
    FILE *lock = fopen(input_focus_lock_path, "w");
    if (lock) {
        fprintf(lock, "# GL-OS Input Focus Lock\n");
        fprintf(lock, "# CHTPM daemons: DO NOT write to input files while this file exists\n");
        fprintf(lock, "owner=gl_desktop\n");
        fprintf(lock, "session=gl-os\n");
        fclose(lock);
    }
    
    /* Initialize session files (clear history on each launch) */
    FILE *f; f = fopen(session_state_path, "w"); if (f) fclose(f);  /* Clear state */
    f = fopen(session_history_path, "w"); if (f) fclose(f);        /* Clear history */
    f = fopen(session_view_path, "w"); if (f) fclose(f);           /* Clear view */
    f = fopen(session_view_changed_path, "w"); if (f) fclose(f);   /* Clear marker */
    f = fopen(master_ledger_path, "a"); if (f) fclose(f);          /* Keep ledger (append) */
    
    create_terminal_window();
    log_master("DESKTOP_START", "gl-os");
    glutDisplayFunc(render_scene); glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard); glutMouseFunc(mouse);
    glutMotionFunc(motion); glutTimerFunc(30, timer, 0);
    glutJoystickFunc(joystick, 50);  /* Joystick callback (50ms poll rate) */
    glutMainLoop();
    
    /* Remove input focus lock on exit - CHTPM can resume input */
    if (input_focus_lock_path) {
        remove(input_focus_lock_path);
    }
    
    free_paths(); return 0;
}
