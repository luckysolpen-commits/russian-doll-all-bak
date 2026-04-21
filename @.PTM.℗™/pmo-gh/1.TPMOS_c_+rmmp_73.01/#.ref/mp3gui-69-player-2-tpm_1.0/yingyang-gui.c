#define _GNU_SOURCE
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <math.h>

#define MAX_PATH_LEN 1024
#define MAX_PLAYLIST_SIZE 10000
#define COMMAND_FILE "player_command.txt"
#define STATUS_FILE "player_status.txt"
#define VIZ_DATA_FILE "player_viz.dat"
#define FFT_SIZE 1024
#define POLL_INTERVAL_MS 50
#define VISIBLE_SONGS 25
#define LINE_HEIGHT 24

// GUI State
char playlist[MAX_PLAYLIST_SIZE][MAX_PATH_LEN];
int playlist_size = 0;
int current_song_index = 0;
char player_state[50] = "STOPPED";
char playback_time[50] = "0.00/0.00";
int scroll_offset = 0;
int in_search = 0;
char search_string[MAX_PATH_LEN] = "";
int highlighted_index = -1;
float fft_data[FFT_SIZE / 2];
int viz_mode = 0; // 0: Bars, 1: Waveform
int mouse_x = 0, mouse_y = 0;

void render_text(const char *text, float x, float y, float r, float g, float b) {
    glColor3f(r, g, b); glRasterPos2f(x, y);
    for (const char *c = text; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
}

void write_command(const char *cmd) {
    FILE *f = fopen(COMMAND_FILE, "w");
    if (f) { fprintf(f, "%s\n", cmd); fclose(f); }
}

void read_status() {
    FILE *f = fopen(STATUS_FILE, "r");
    if (!f) return;
    char line[MAX_PATH_LEN];
    while (fgets(line, MAX_PATH_LEN, f)) {
        if (strncmp(line, "STATE:", 6) == 0) sscanf(line, "STATE: %s", player_state);
        else if (strncmp(line, "CURRENT_SONG_INDEX:", 19) == 0) sscanf(line, "CURRENT_SONG_INDEX: %d", &current_song_index);
        else if (strncmp(line, "PLAYBACK_TIME:", 14) == 0) sscanf(line, "PLAYBACK_TIME: %s", playback_time);
    }
    fclose(f);
}

void read_viz() {
    FILE *f = fopen(VIZ_DATA_FILE, "rb");
    if (f) { 
        size_t n = fread(fft_data, sizeof(float), FFT_SIZE / 2, f); 
        (void)n;
        fclose(f); 
    }
}

void get_display_name(const char* full_path, char* display_name) {
    const char* last_slash = strrchr(full_path, '/');
    if (!last_slash) { strncpy(display_name, full_path, MAX_PATH_LEN - 1); return; }
    const char* second_last_slash = NULL;
    const char* p = last_slash - 1;
    while (p >= full_path) { if (*p == '/') { second_last_slash = p; break; } p--; }
    if (second_last_slash) strncpy(display_name, second_last_slash + 1, MAX_PATH_LEN - 1);
    else strncpy(display_name, full_path, MAX_PATH_LEN - 1);
}

void read_playlist() {
    FILE *f = fopen("playlist.txt", "r");
    if (!f) return;
    playlist_size = 0; char buf[MAX_PATH_LEN];
    while (fgets(buf, MAX_PATH_LEN, f) && playlist_size < MAX_PLAYLIST_SIZE) {
        buf[strcspn(buf, "\n")] = 0;
        get_display_name(buf, playlist[playlist_size++]);
    }
    fclose(f);
}

void draw_visualizer(int w, int h) {
    int v_x = 20, v_y = h - 220, v_w = w - 40, v_h = 200;
    glColor3f(0.2, 0.2, 0.2); glBegin(GL_LINE_LOOP);
    glVertex2f(v_x, v_y); glVertex2f(v_x + v_w, v_y);
    glVertex2f(v_x + v_w, v_y + v_h); glVertex2f(v_x, v_y + v_h);
    glEnd();

    if (viz_mode == 0) { // Bars
        float bw = (float)v_w / (FFT_SIZE / 8);
        for (int i = 0; i < FFT_SIZE / 8; i++) {
            float val = (fft_data[i] / 20.0f) * v_h; if (val > v_h) val = v_h;
            float intensity = (float)i / (FFT_SIZE / 8);
            glColor3f(intensity, intensity, intensity);
            glRectf(v_x + i * bw, v_y, v_x + (i + 1) * bw - 1, v_y + val);
        }
    } else if (viz_mode == 1) { // Waveform
        glBegin(GL_LINE_STRIP); glColor3f(1, 1, 1);
        for (int i = 0; i < FFT_SIZE / 8; i++) {
            float val = (fft_data[i] / 20.0f) * (v_h / 2);
            glVertex2f(v_x + (float)i * v_w / (FFT_SIZE / 8), v_y + v_h / 2 + (i % 2 ? val : -val));
        }
        glEnd();
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    int w = glutGet(GLUT_WINDOW_WIDTH), h = glutGet(GLUT_WINDOW_HEIGHT);
    
    char status[256]; sprintf(status, "YingYang Player | %s | %s", player_state, playback_time);
    render_text(status, 20, h - 20, 1, 1, 1);

    draw_visualizer(w, h);

    render_text("Playlist (Up/Down: Navigate, PgUp/PgDn: Scroll, Space: Pause, '/': Search):", 20, h - 250, 0.8, 0.8, 0.8);
    int max_disp = (h - 300) / LINE_HEIGHT;
    for (int i = 0; i < max_disp && (i + scroll_offset) < playlist_size; i++) {
        int idx = i + scroll_offset;
        int y_pos = h - 280 - i * LINE_HEIGHT;
        
        // Hover effect
        if (mouse_x >= 20 && mouse_x <= w - 20 && mouse_y >= y_pos - 4 && mouse_y <= y_pos + 16) {
            glColor3f(0.15, 0.15, 0.15);
            glRectf(20, y_pos - 4, w - 20, y_pos + 16);
        }

        float r = 1, g = 1, b = 1;
        if (idx == current_song_index) { r = 0; g = 1; b = 0; }
        else if (idx == highlighted_index) { r = 0; g = 1; b = 1; }
        render_text(playlist[idx], 30, y_pos, r, g, b);
    }

    if (in_search) {
        char s[MAX_PATH_LEN + 10]; sprintf(s, "Search: %s_", search_string);
        render_text(s, 20, 20, 1, 0, 1);
    }

    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
    if (in_search) {
        if (key == 13) { // Enter in search
            in_search = 0; 
            for(int i=0; i<playlist_size; i++) {
                if(strcasestr(playlist[i], search_string)) { 
                    highlighted_index = i; 
                    scroll_offset = i; 
                    current_song_index = i; // Also update selection
                    char b[64]; sprintf(b, "PLAY %d", i); write_command(b);
                    break; 
                }
            }
        } else if (key == 8 || key == 127) { int len = strlen(search_string); if(len > 0) search_string[len-1] = 0; }
        else if (key == 27) { in_search = 0; search_string[0] = 0; }
        else if (isprint(key)) { int len = strlen(search_string); if(len < MAX_PATH_LEN-1) { search_string[len] = key; search_string[len+1] = 0; } }
    } else {
        if (key == 13) { // Enter to play selection
            char b[64]; sprintf(b, "PLAY %d", current_song_index); write_command(b);
        }
        else if (key == '/') in_search = 1;
        else if (key == ' ') write_command("PAUSE");
        else if (key == 'v') viz_mode = (viz_mode + 1) % 2;
        else if (key == 27) exit(0);
    }
    glutPostRedisplay();
}

void special(int key, int x, int y) {
    if (key == GLUT_KEY_UP) {
        if(current_song_index > 0) {
            current_song_index--;
            char b[64]; sprintf(b, "PLAY %d", current_song_index); write_command(b);
            if (current_song_index < scroll_offset) scroll_offset = current_song_index;
        }
    } else if (key == GLUT_KEY_DOWN) {
        if(current_song_index < playlist_size-1) {
            current_song_index++;
            char b[64]; sprintf(b, "PLAY %d", current_song_index); write_command(b);
            int max_disp = (glutGet(GLUT_WINDOW_HEIGHT) - 300) / LINE_HEIGHT;
            if (current_song_index >= scroll_offset + max_disp) scroll_offset = current_song_index - max_disp + 1;
        }
    } else if (key == GLUT_KEY_PAGE_UP) {
        scroll_offset -= 10; if(scroll_offset < 0) scroll_offset = 0;
    } else if (key == GLUT_KEY_PAGE_DOWN) { 
        int max_disp = (glutGet(GLUT_WINDOW_HEIGHT) - 300) / LINE_HEIGHT;
        scroll_offset += 10; if(scroll_offset > playlist_size - max_disp) scroll_offset = playlist_size - max_disp;
        if(scroll_offset < 0) scroll_offset = 0;
    }
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    int gl_y = h - y;
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        int max_disp = (h - 300) / LINE_HEIGHT;
        for (int i = 0; i < max_disp && (i + scroll_offset) < playlist_size; i++) {
            int y_pos = h - 280 - i * LINE_HEIGHT;
            if (gl_y >= y_pos - 4 && gl_y <= y_pos + 16) {
                current_song_index = i + scroll_offset;
                char b[64]; sprintf(b, "PLAY %d", current_song_index); write_command(b);
                break;
            }
        }
    } else if (button == 3) { // Scroll up
        scroll_offset -= 3; if (scroll_offset < 0) scroll_offset = 0;
    } else if (button == 4) { // Scroll down
        int max_disp = (h - 300) / LINE_HEIGHT;
        scroll_offset += 3; if (scroll_offset > playlist_size - max_disp) scroll_offset = playlist_size - max_disp;
        if (scroll_offset < 0) scroll_offset = 0;
    }
    glutPostRedisplay();
}

void motion(int x, int y) {
    mouse_x = x; mouse_y = glutGet(GLUT_WINDOW_HEIGHT) - y;
    glutPostRedisplay();
}

void timer(int v) {
    read_status(); read_viz(); glutPostRedisplay();
    glutTimerFunc(POLL_INTERVAL_MS, timer, 0);
}

int main(int argc, char **argv) {
    read_playlist();
    glutInit(&argc, argv); glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 800); glutCreateWindow("YingYang Player");
    glClearColor(0.05, 0.05, 0.05, 1.0);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluOrtho2D(0, 800, 0, 800);
    glutDisplayFunc(display); glutKeyboardFunc(keyboard); glutSpecialFunc(special);
    glutMouseFunc(mouse); glutPassiveMotionFunc(motion);
    glutTimerFunc(0, timer, 0);
    printf("YingYang GUI Ready.\n"); glutMainLoop();
    return 0;
}
