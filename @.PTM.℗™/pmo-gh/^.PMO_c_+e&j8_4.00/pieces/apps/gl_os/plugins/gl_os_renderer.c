#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_PATH 1024
#define MAX_WINDOWS 10
#define MAX_LINE 1024

typedef struct {
    int id;
    int x, y, w, h;
    char title[64];
    char content_source[MAX_PATH];
} GLVirtualWindow;

GLVirtualWindow windows[MAX_WINDOWS];
int window_count = 0;
int window_width = 800;
int window_height = 600;
off_t last_view_size = 0;

void parse_gl_os_view() {
    FILE *fp = fopen("pieces/apps/gl_os/view.txt", "r");
    if (!fp) return;
    
    char line[MAX_LINE];
    window_count = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "WINDOW", 6) == 0) {
            GLVirtualWindow *win = &windows[window_count++];
            sscanf(line, "WINDOW | %d | %d | %d | %d | %d | %63[^|] | %1023s", 
                   &win->id, &win->x, &win->y, &win->w, &win->h, win->title, win->content_source);
        }
    }
    fclose(fp);
}

void render_text_in_window(const char* content, int x, int y, int w, int h) {
    // Basic text-only renderer for the internal window content
    // Maps characters to screen coords
    glColor3f(1.0f, 1.0f, 1.0f);
    float start_x = (float)x + 10.0f;
    float start_y = (float)(window_height - y) - 30.0f;
    glRasterPos2f(start_x, start_y);
    
    for (const char *p = content; *p; p++) {
        if (*p == '\n') {
            start_y -= 15.0f;
            glRasterPos2f(start_x, start_y);
        } else {
            glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p);
        }
    }
}

void display() {
    glClearColor(0.0f, 0.0f, 0.2f, 1.0f); // Dark blue background
    glClear(GL_COLOR_BUFFER_BIT);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, (float)window_width, 0, (float)window_height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    for (int i = 0; i < window_count; i++) {
        GLVirtualWindow *win = &windows[i];
        
        // Window Border
        glColor3f(0.5f, 0.5f, 0.5f);
        glBegin(GL_LINE_LOOP);
            glVertex2f((float)win->x, (float)(window_height - win->y));
            glVertex2f((float)(win->x + win->w), (float)(window_height - win->y));
            glVertex2f((float)(win->x + win->w), (float)(window_height - (win->y + win->h)));
            glVertex2f((float)win->x, (float)(window_height - (win->y + win->h)));
        glEnd();
        
        // Window Title Bar
        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_QUADS);
            glVertex2f((float)win->x, (float)(window_height - win->y));
            glVertex2f((float)(win->x + win->w), (float)(window_height - win->y));
            glVertex2f((float)(win->x + win->w), (float)(window_height - (win->y + 20)));
            glVertex2f((float)win->x, (float)(window_height - (win->y + 20)));
        glEnd();
        
        // Render Title
        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2f((float)(win->x + 5), (float)(window_height - (win->y + 15)));
        for (const char *p = win->title; *p; p++) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p);
        
        // Render Content (Current Frame Buffer)
        FILE *content_f = fopen(win->content_source, "r");
        if (content_f) {
            fseek(content_f, 0, SEEK_END);
            long size = ftell(content_f);
            fseek(content_f, 0, SEEK_SET);
            char *buf = malloc(size + 1);
            if (buf) {
                fread(buf, 1, size, content_f);
                buf[size] = '\0';
                render_text_in_window(buf, win->x, win->y, win->w, win->h);
                free(buf);
            }
            fclose(content_f);
        }
    }
    
    glutSwapBuffers();
}

void timer(int value) {
    struct stat st;
    if (stat("pieces/apps/gl_os/view_changed.txt", &st) == 0) {
        if (st.st_size != last_view_size) {
            parse_gl_os_view();
            last_view_size = st.st_size;
        }
    }
    glutPostRedisplay();
    glutTimerFunc(100, timer, 0);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("PMO GL-OS Desktop");
    glutDisplayFunc(display);
    glutTimerFunc(100, timer, 0);
    
    struct stat st;
    if (stat("pieces/apps/gl_os/view_changed.txt", &st) == 0) last_view_size = st.st_size;
    parse_gl_os_view();
    
    glutMainLoop();
    return 0;
}
