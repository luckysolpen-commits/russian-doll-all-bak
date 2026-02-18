#include <GL/glut.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};

int window_width = 800;
int window_height = 1200;
float background_color[4] = {0.1f, 0.1f, 0.1f, 1.0f};

// Global state for frame content (same as ASCII renderer)
char *frame_content = NULL;
off_t last_marker_size = 0;  // Track marker file size for change detection

void disableRawMode() {
  // No-op for GL renderer - GLUT handles keyboard input
}

void enableRawMode() {
  // No-op for GL renderer - GLUT handles keyboard input
}

void writeCommand(int key) {
  // Write to pieces/keyboard/history.txt as per TPM standards
  FILE *fp = fopen("pieces/keyboard/history.txt", "a");
  if (!fp) {
    system("mkdir -p pieces/keyboard");
    fp = fopen("pieces/keyboard/history.txt", "a");
  }
  if (!fp) {
    fprintf(stderr, "Could not open keyboard history file\n");
    return;
  }
  
  // Add timestamp for audit trail
  time_t rawtime;
  struct tm *timeinfo;
  char timestamp[100];
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
  
  fprintf(fp, "[%s] KEY_PRESSED: %d\n", timestamp, key);
  fclose(fp);
  
  // Also log to master ledger for complete audit trail
  FILE *ledger = fopen("pieces/master_ledger/master_ledger.txt", "a");
  if (ledger) {
      fprintf(ledger, "[%s] InputReceived: key_code=%d | Source: gl_keyboard_input\n", timestamp, key);
      fclose(ledger);
  }
}

void keyboard(unsigned char key, int x, int y) {
  int key_code = (int)key;
  if (key_code == CTRL_KEY('c')) {
    disableRawMode();
    exit(0);
  }
  writeCommand(key_code);
}

void special_keyboard(int key, int x, int y) {
  int key_code = 0;
  switch(key) {
    case GLUT_KEY_UP: key_code = ARROW_UP; break;
    case GLUT_KEY_DOWN: key_code = ARROW_DOWN; break;
    case GLUT_KEY_LEFT: key_code = ARROW_LEFT; break;
    case GLUT_KEY_RIGHT: key_code = ARROW_RIGHT; break;
    case GLUT_KEY_HOME: key_code = HOME_KEY; break;
    case GLUT_KEY_END: key_code = END_KEY; break;
    case GLUT_KEY_PAGE_UP: key_code = PAGE_UP; break;
    case GLUT_KEY_PAGE_DOWN: key_code = PAGE_DOWN; break;
    case GLUT_KEY_INSERT: key_code = DEL_KEY; break;
    default: return;
  }
  writeCommand(key_code);
}

// Same frame parser as ASCII renderer
void parse_frame_file() {
    FILE *fp = fopen("pieces/display/current_frame.txt", "r");
    if (!fp) {
        return;
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size > 0) {
        if (frame_content) free(frame_content);
        frame_content = malloc(file_size + 1);
        if (frame_content) {
            size_t bytes_read = fread(frame_content, 1, file_size, fp);
            frame_content[bytes_read] = '\0';
        }
    }
    fclose(fp);
}

// Render frame using GLUT bitmap font (same approach as ASCII, but with GL)
void render_frame() {
    glClearColor(background_color[0], background_color[1], background_color[2], background_color[3]);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, window_width, 0, window_height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (frame_content) {
        // Render text line by line using GLUT bitmap font
        float line_height = 20.0f;
        float start_x = 50.0f;
        float start_y = window_height - 50.0f;
        
        char *content_copy = strdup(frame_content);
        char *line = strtok(content_copy, "\n");
        int line_num = 0;
        
        while (line != NULL) {
            float x = start_x;
            float y = start_y - (line_num * line_height);
            
            glRasterPos2f(x, y);
            for (size_t i = 0; i < strlen(line); i++) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, line[i]);
            }
            
            line = strtok(NULL, "\n");
            line_num++;
        }
        
        free(content_copy);
    }

    glutSwapBuffers();
}

void display() {
    render_frame();
}

void reshape(int w, int h) {
    window_width = w;
    window_height = h;
    glViewport(0, 0, w, h);
    glutPostRedisplay();
}

void timer(int value) {
    // Check marker file SIZE (same as ASCII renderer)
    struct stat st;
    if (stat("pieces/master_ledger/frame_changed.txt", &st) == 0) {
        if (st.st_size != last_marker_size) {
            parse_frame_file();
            last_marker_size = st.st_size;
        }
    }
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);  // ~60fps
}

void init() {
    // Ensure directories exist
    system("mkdir -p pieces/keyboard");
    
    // Set initial marker file size
    struct stat st;
    if (stat("pieces/master_ledger/frame_changed.txt", &st) == 0) {
        last_marker_size = st.st_size;
    }
    
    // Initial parse
    parse_frame_file();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("TPM FuzzPet - GL Renderer");

    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutTimerFunc(16, timer, 0);  // Start timer immediately (was 100)
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special_keyboard);

    init();

    fprintf(stderr, "GL Renderer started. Reading from pieces/display/current_frame.txt\n");
    fprintf(stderr, "Type in the GL window to control the pet. Press Ctrl+C to quit.\n");

    glutMainLoop();

    return 0;
}
