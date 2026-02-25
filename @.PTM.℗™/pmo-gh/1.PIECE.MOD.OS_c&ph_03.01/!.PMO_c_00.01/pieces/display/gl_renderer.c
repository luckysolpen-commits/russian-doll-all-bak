#include <GL/glut.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define MAX_HISTORY 50
#define MAX_LINES_PER_FRAME 200

int window_width = 800;
int window_height = 1000;
float background_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};

typedef struct {
    char **lines;
    int num_lines;
    char timestamp[32];
} FrameEntry;

FrameEntry history[MAX_HISTORY];
int history_count = 0;
int history_head = 0;
int scroll_offset = 0;
int total_lines_in_history = 0;
off_t last_marker_size = 0;

// Mouse Interaction
int is_dragging = 0;
int drag_start_y = 0;
int scroll_start_offset = 0;

void writeCommand(int key) {
    FILE *fp = fopen("pieces/keyboard/history.txt", "a");
    if (!fp) return;
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[100];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    fprintf(fp, "KEY_PRESSED: %d\n", key);
    fclose(fp);
}

void add_to_history(const char *content) {
    if (!content || strlen(content) == 0) return;

    // Check if this is the same as the last frame to avoid spamming history
    int last_idx = (history_head + history_count - 1) % MAX_HISTORY;
    if (history_count > 0) {
        // Simple heuristic: check first line
        char *first_newline = strchr(content, '\n');
        if (first_newline) {
            int len = first_newline - content;
            if (history[last_idx].num_lines > 0 && strncmp(history[last_idx].lines[0], content, len) == 0) {
                // Potential duplicate, skip for history (but would update current if we had a separate 'current' pointer)
                // For this implementation, we append all changes to history as requested.
            }
        }
    }

    FrameEntry entry;
    entry.lines = malloc(sizeof(char*) * MAX_LINES_PER_FRAME);
    entry.num_lines = 0;
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(entry.timestamp, sizeof(entry.timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    char *copy = strdup(content);
    char *line = strtok(copy, "\n");
    while (line && entry.num_lines < MAX_LINES_PER_FRAME) {
        entry.lines[entry.num_lines++] = strdup(line);
        line = strtok(NULL, "\n");
    }
    free(copy);

    if (history_count == MAX_HISTORY) {
        total_lines_in_history -= (history[history_head].num_lines + 1);
        for (int i = 0; i < history[history_head].num_lines; i++) free(history[history_head].lines[i]);
        free(history[history_head].lines);
        history[history_head] = entry;
        history_head = (history_head + 1) % MAX_HISTORY;
    } else {
        history[history_count++] = entry;
    }

    total_lines_in_history += (entry.num_lines + 1);
    scroll_offset = 0; // Auto-scroll to bottom
}

void keyboard(unsigned char key, int x, int y) {
    if (key == CTRL_KEY('c')) exit(0);
    writeCommand((int)key);
}

void special_keyboard(int key, int x, int y) {
    int max_visible_lines = (window_height / 18) - 1;
    switch(key) {
        case GLUT_KEY_UP: scroll_offset += 2; break;
        case GLUT_KEY_DOWN: scroll_offset = (scroll_offset > 2) ? scroll_offset - 2 : 0; break;
        case GLUT_KEY_PAGE_UP: scroll_offset += 20; break;
        case GLUT_KEY_PAGE_DOWN: scroll_offset = (scroll_offset > 20) ? scroll_offset - 20 : 0; break;
    }
    if (scroll_offset > total_lines_in_history - 5) scroll_offset = total_lines_in_history - 5;
    if (scroll_offset < 0) scroll_offset = 0;
    glutPostRedisplay();
}

void parse_frame_file() {
    FILE *fp = fopen("pieces/display/current_frame.txt", "r");
    if (!fp) return;
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (file_size > 0) {
        char *buffer = malloc(file_size + 1);
        fread(buffer, 1, file_size, fp);
        buffer[file_size] = '\0';
        add_to_history(buffer);
        free(buffer);
    }
    fclose(fp);
}

void render_frame() {
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, window_width, 0, window_height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float line_height = 18.0f;
    int max_visible_lines = (window_height / line_height) - 1;
    int total_drawn = 0;
    
    // Draw frames: newest (i=0) at the bottom, history above it
    for (int i = 0; i < history_count; i++) {
        int idx = (history_head + history_count - 1 - i) % MAX_HISTORY;
        FrameEntry *fe = &history[idx];

        // Draw lines bottom-up: last line of frame first
        for (int j = fe->num_lines - 1; j >= -1; j--) {
            total_drawn++;
            if (total_drawn > scroll_offset && total_drawn <= scroll_offset + max_visible_lines) {
                float y = 20.0f + (total_drawn - scroll_offset - 1) * line_height;
                glRasterPos2f(15.0f, y);
                
                if (j == -1) { // Header (Top of its frame)
                    char header[64];
                    snprintf(header, sizeof(header), "--- FRAME UPDATE at %s ---", fe->timestamp);
                    glColor3f(0.4f, 0.4f, 0.4f);
                    for (char *p = header; *p; p++) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p);
                } else {
                    // Newest frame is bright white, history is slightly dimmer
                    if (i == 0) glColor3f(1.0f, 1.0f, 1.0f);
                    else glColor3f(0.8f, 0.8f, 0.8f);
                    for (char *p = fe->lines[j]; *p; p++) glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *p);
                }
            }
        }
        if (total_drawn > scroll_offset + max_visible_lines) break;
    }

    // Scrollbar Thumb
    float sb_w = 16.0f; // Wider for easier grabbing
    float sb_x = window_width - sb_w - 5.0f;
    float track_h = window_height - 20.0f;
    
    // Track
    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
        glVertex2f(sb_x, 10); glVertex2f(sb_x+sb_w, 10);
        glVertex2f(sb_x+sb_w, 10+track_h); glVertex2f(sb_x, 10+track_h);
    glEnd();

    if (total_lines_in_history > 0) {
        int max_visible = (window_height / 18) - 1;
        float thumb_h = (float)max_visible / (total_lines_in_history + 1) * track_h;
        if (thumb_h < 30.0f) thumb_h = 30.0f;
        if (thumb_h > track_h) thumb_h = track_h;
        
        float scroll_perc = (total_lines_in_history > max_visible) ? 
                            (float)scroll_offset / (total_lines_in_history - max_visible + 5) : 0;
        float thumb_y = 10.0f + scroll_perc * (track_h - thumb_h);

        glColor3f(0.6f, 0.6f, 0.6f); // Brighter thumb
        glBegin(GL_QUADS);
            glVertex2f(sb_x+1, thumb_y); glVertex2f(sb_x+sb_w-1, thumb_y);
            glVertex2f(sb_x+sb_w-1, thumb_y+thumb_h); glVertex2f(sb_x+1, thumb_y+thumb_h);
        glEnd();
    }

    glutSwapBuffers();
}

void mouse_func(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        float sb_w = 16.0f;
        float sb_x = window_width - sb_w - 5.0f;
        
        if (state == GLUT_DOWN) {
            if (x >= sb_x && x <= sb_x + sb_w) {
                is_dragging = 1;
                drag_start_y = y;
                scroll_start_offset = scroll_offset;
            }
        } else {
            is_dragging = 0;
        }
    }
}

void motion_func(int x, int y) {
    if (is_dragging) {
        int dy = y - drag_start_y; // Window Y: down is positive
        int max_visible = (window_height / 18) - 1;
        
        if (total_lines_in_history > max_visible) {
            // Sensitivity: Map pixels to lines
            float pixels_to_lines = (float)total_lines_in_history / (window_height - 20);
            scroll_offset = scroll_start_offset + (int)(dy * pixels_to_lines);
            
            if (scroll_offset > total_lines_in_history - 5) scroll_offset = total_lines_in_history - 5;
            if (scroll_offset < 0) scroll_offset = 0;
            glutPostRedisplay();
        }
    }
}

void timer(int value) {
    struct stat st;
    if (stat("pieces/master_ledger/frame_changed.txt", &st) == 0) {
        if (st.st_size != last_marker_size) {
            parse_frame_file();
            last_marker_size = st.st_size;
        }
    }
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(800, 1000);
    glutCreateWindow("TPM GL Terminal");
    glutDisplayFunc(render_frame);
    glutTimerFunc(16, timer, 0);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special_keyboard);
    glutMouseFunc(mouse_func);
    glutMotionFunc(motion_func);
    struct stat st;
    if (stat("pieces/master_ledger/frame_changed.txt", &st) == 0) last_marker_size = st.st_size;
    parse_frame_file();
    glutMainLoop();
    return 0;
}
