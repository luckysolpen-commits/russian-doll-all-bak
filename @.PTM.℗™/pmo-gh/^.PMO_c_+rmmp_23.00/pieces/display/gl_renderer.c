#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <GL/glut.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ft2build.h>
#include FT_FREETYPE_H

// TPM GL Renderer (v1.2 - PULSE FIX & ASPRINTF)
// Responsibility: High-speed output with selective redrawing.

#define CTRL_KEY(k) ((k) & 0x1f)
#define MAX_HISTORY 50
#define MAX_LINES_PER_FRAME 200

int window_width = 800;
int window_height = 1000;
float background_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};

FT_Library ft_library;
FT_Face emoji_face;
int emoji_initialized = 0;
float emoji_scale = 1.0f;

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

int is_dragging = 0;
int drag_start_y = 0;
int scroll_start_offset = 0;

void writeCommand(int key) {
    FILE *fp = fopen("pieces/keyboard/history.txt", "a");
    if (!fp) return;
    fprintf(fp, "KEY_PRESSED: %d\n", key);
    fclose(fp);
}

void add_to_history(const char *content) {
    if (!content || strlen(content) == 0) return;

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
    scroll_offset = 0;
}

void keyboard(unsigned char key, int x, int y) {
    if (key == CTRL_KEY('c')) exit(0);
    writeCommand((int)key);
}

void special_keyboard(int key, int x, int y) {
    switch(key) {
        case GLUT_KEY_UP: writeCommand(1002); break;
        case GLUT_KEY_DOWN: writeCommand(1003); break;
        case GLUT_KEY_LEFT: writeCommand(1000); break;
        case GLUT_KEY_RIGHT: writeCommand(1001); break;
    }
}

void parse_frame_file() {
    FILE *fp = fopen("pieces/display/current_frame.txt", "r");
    if (!fp) return;
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (file_size > 0) {
        char *buffer = malloc(file_size + 1);
        if (buffer) {
            fread(buffer, 1, file_size, fp);
            buffer[file_size] = '\0';
            add_to_history(buffer);
            free(buffer);
        }
    }
    fclose(fp);
}

void init_emoji_rendering() {
    if (FT_Init_FreeType(&ft_library)) return;
    const char *emoji_font_path = "/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf";
    if (FT_New_Face(ft_library, emoji_font_path, 0, &emoji_face)) {
        FT_Done_FreeType(ft_library);
        return;
    }
    FT_Set_Pixel_Sizes(emoji_face, 0, 32);
    emoji_scale = 32.0f / (float)emoji_face->size->metrics.y_ppem;
    emoji_initialized = 1;
}

int decode_utf8(const unsigned char* str, unsigned int* codepoint) {
    if (str[0] < 0x80) { *codepoint = str[0]; return 1; }
    if ((str[0] & 0xE0) == 0xC0) { if ((str[1] & 0xC0) == 0x80) { *codepoint = ((str[0] & 0x1F) << 6) | (str[1] & 0x3F); return 2; } }
    if ((str[0] & 0xF0) == 0xE0) { if ((str[1] & 0xC0) == 0x80 && (str[2] & 0xC0) == 0x80) { *codepoint = ((str[0] & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F); return 3; } }
    if ((str[0] & 0xF8) == 0xF0) { if ((str[1] & 0xC0) == 0x80 && (str[2] & 0xC0) == 0x80 && (str[3] & 0xC0) == 0x80) { *codepoint = ((str[0] & 0x07) << 18) | ((str[1] & 0x3F) << 12) | ((str[2] & 0x3F) << 6) | (str[3] & 0x3F); return 4; } }
    *codepoint = '?'; return 1;
}

int is_emoji(unsigned int cp) {
    if (cp >= 0x1F300 && cp <= 0x1F9FF) return 1;
    if (cp >= 0x2600 && cp <= 0x26FF) return 1;
    if (cp >= 0x2700 && cp <= 0x27BF) return 1;
    return 0;
}

void render_emoji_char(unsigned int codepoint, float x, float y) {
    if (!emoji_initialized) return;
    if (FT_Load_Char(emoji_face, codepoint, FT_LOAD_RENDER | FT_LOAD_COLOR)) return;
    FT_GlyphSlot slot = emoji_face->glyph;
    if (!slot->bitmap.buffer || slot->bitmap.pixel_mode != FT_PIXEL_MODE_BGRA) return;

    GLuint tex; glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, slot->bitmap.width, slot->bitmap.rows, 0, GL_BGRA, GL_UNSIGNED_BYTE, slot->bitmap.buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float w = slot->bitmap.width * emoji_scale, h = slot->bitmap.rows * emoji_scale;
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2f(x, y-4); glTexCoord2f(1, 1); glVertex2f(x+w, y-4);
    glTexCoord2f(1, 0); glVertex2f(x+w, y+h-4); glTexCoord2f(0, 0); glVertex2f(x, y+h-4);
    glEnd();
    glDisable(GL_TEXTURE_2D); glDisable(GL_BLEND); glDeleteTextures(1, &tex);
}

void render_line_mixed(const char* line, float x, float y) {
    const unsigned char* p = (const unsigned char*)line;
    float cx = x;
    while (*p) {
        unsigned int cp; int b = decode_utf8(p, &cp);
        if (is_emoji(cp)) { render_emoji_char(cp, cx, y); cx += 20.0f; }
        else { glRasterPos2f(cx, y); glutBitmapCharacter(GLUT_BITMAP_9_BY_15, (int)cp); cx += 9.0f; }
        p += b;
    }
}

void render_frame() {
    glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, window_width, 0, window_height, -1, 1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    float lh = 18.0f; int max_v = (window_height / lh) - 1, drawn = 0;
    for (int i = 0; i < history_count; i++) {
        int idx = (history_head + history_count - 1 - i) % MAX_HISTORY;
        FrameEntry *fe = &history[idx];
        for (int j = fe->num_lines - 1; j >= -1; j--) {
            drawn++;
            if (drawn > scroll_offset && drawn <= scroll_offset + max_v) {
                float y = 20.0f + (drawn - scroll_offset - 1) * lh;
                if (j == -1) {
                    char *header = NULL; asprintf(&header, "--- FRAME UPDATE at %s ---", fe->timestamp);
                    glColor3f(0.4f, 0.4f, 0.4f); glRasterPos2f(15.0f, y);
                    if (header) { for (char *p = header; *p; p++) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p); free(header); }
                } else {
                    glColor3f(1,1,1); render_line_mixed(fe->lines[j], 15.0f, y);
                }
            }
        }
        if (drawn > scroll_offset + max_v) break;
    }
    glutSwapBuffers();
}

void timer(int value) {
    struct stat st;
    // CRITICAL: Watch renderer_pulse.txt to avoid background spam
    if (stat("pieces/display/renderer_pulse.txt", &st) == 0) {
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
    init_emoji_rendering();
    struct stat st;
    if (stat("pieces/display/renderer_pulse.txt", &st) == 0) last_marker_size = st.st_size;
    parse_frame_file();
    glutMainLoop();
    return 0;
}
