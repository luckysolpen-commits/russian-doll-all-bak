#include <GL/glut.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

// --- Globals ---

#define GRID_ROWS 8
#define GRID_COLS 8
#define TILE_SIZE 64
#define MATCH_MIN 3

// Max number of tile types (will be set by CSV)
#define MAX_TILE_TYPES 50

typedef struct {
    char display_text[64];
    char compound_name[64];
    int category;
    char hint[128];
} ChemTile;

ChemTile chem_tiles[MAX_TILE_TYPES];
int num_tile_types = 0;

int grid[GRID_ROWS][GRID_COLS];
bool is_animating = false;
int score = 0;

int window_width = GRID_COLS * TILE_SIZE + 20;
int window_height = GRID_ROWS * TILE_SIZE + 60;

int selected_row = -1;
int selected_col = -1;

float background_color[4] = {0.1f, 0.1f, 0.1f, 1.0f};
float font_color[3] = {1.0f, 1.0f, 1.0f};

// Color palette for categories (extend as needed)
float category_colors[MAX_TILE_TYPES][3] = {
    {1.0f, 0.3f, 0.3f}, // Red: Acids
    {0.3f, 0.3f, 1.0f}, // Blue: Alcohols
    {1.0f, 0.7f, 0.0f}, // Orange: Aldehydes
    {0.8f, 0.4f, 1.0f}, // Purple: Ketones
    {0.3f, 1.0f, 0.3f}, // Green: Amines/Amides
    {1.0f, 1.0f, 0.3f}, // Yellow: Aromatics
    // Add more if needed
};

char status_message[256] = "";

// --- Function Prototypes ---
void load_chemistry_csv(const char* filename);
void display();
void reshape(int w, int h);
void mouse(int button, int state, int x, int y);
void init();
void idle();
void draw_rect(float x, float y, float w, float h, float color[3]);
void fill_grid();
void swap_candies(int r1, int c1, int r2, int c2);
bool find_matches();
void remove_matches();
void drop_candies();
bool is_adjacent(int r1, int c1, int r2, int c2);
void set_status_message(const char* msg);
void render_text(const char* str, float x, float y);
void show_chemistry_tip(int category);

// --- CSV Loader ---
void load_chemistry_csv(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open %s\n", filename);
        exit(1);
    }

    char line[256];
    // Skip header
    if (!fgets(line, sizeof(line), file)) {
        fprintf(stderr, "Error: Empty CSV file\n");
        fclose(file);
        exit(1);
    }

    num_tile_types = 0;
    while (fgets(line, sizeof(line), file) && num_tile_types < MAX_TILE_TYPES) {
        ChemTile* tile = &chem_tiles[num_tile_types];
        char* token = strtok(line, ",");
        if (!token) continue;

        // display_text
        strncpy(tile->display_text, token, sizeof(tile->display_text) - 1);
        tile->display_text[sizeof(tile->display_text) - 1] = '\0';

        token = strtok(NULL, ",");
        if (!token) continue;
        // compound_name
        strncpy(tile->compound_name, token, sizeof(tile->compound_name) - 1);
        tile->compound_name[sizeof(tile->compound_name) - 1] = '\0';

        token = strtok(NULL, ",");
        if (!token) continue;
        // category
        tile->category = atoi(token);

        token = strtok(NULL, "\n");
        if (!token) continue;
        // hint (may contain commas, so take rest of line)
        strncpy(tile->hint, token, sizeof(tile->hint) - 1);
        tile->hint[sizeof(tile->hint) - 1] = '\0';

        // Remove quotes if present (simple)
        if (tile->hint[0] == '"') {
            memmove(tile->hint, tile->hint + 1, strlen(tile->hint));
            if (strlen(tile->hint) > 0 && tile->hint[strlen(tile->hint)-1] == '"') {
                tile->hint[strlen(tile->hint)-1] = '\0';
            }
        }

        num_tile_types++;
    }
    fclose(file);

    if (num_tile_types == 0) {
        fprintf(stderr, "Error: No valid entries in CSV\n");
        exit(1);
    }

    printf("Loaded %d chemistry tiles from %s\n", num_tile_types, filename);
}

// --- Text Rendering (GLUT Bitmap) ---
void render_text(const char* str, float x, float y) {
    glColor3fv(font_color);
    glRasterPos2f(x, y);
    while (*str) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *str++);
    }
    glColor3f(1.0f, 1.0f, 1.0f);
}

// --- Game Logic ---
void fill_grid() {
    srand(time(NULL));
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            grid[r][c] = rand() % num_tile_types;
        }
    }
    // Remove initial matches
    while (find_matches()) {
        remove_matches();
        drop_candies();
    }
}

bool is_adjacent(int r1, int c1, int r2, int c2) {
    return (abs(r1 - r2) + abs(c1 - c2) == 1);
}

void swap_candies(int r1, int c1, int r2, int c2) {
    int temp = grid[r1][c1];
    grid[r1][c1] = grid[r2][c2];
    grid[r2][c2] = temp;
}

// Track matched categories for educational tip
int last_matched_category = -1;

bool find_matches() {
    bool has_match = false;
    bool visited[GRID_ROWS][GRID_COLS] = {{false}};

    // Horizontal
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (visited[r][c]) continue;
            int type = grid[r][c];
            if (type == -1) continue;
            int count = 1;
            for (int cc = c + 1; cc < GRID_COLS && grid[r][cc] == type; cc++) {
                count++;
            }
            if (count >= MATCH_MIN) {
                has_match = true;
                last_matched_category = chem_tiles[type].category;
                for (int cc = c; cc < c + count; cc++) {
                    visited[r][cc] = true;
                }
                score += count * 10;
            }
        }
    }

    // Vertical
    for (int c = 0; c < GRID_COLS; c++) {
        for (int r = 0; r < GRID_ROWS; r++) {
            if (visited[r][c]) continue;
            int type = grid[r][c];
            if (type == -1) continue;
            int count = 1;
            for (int rr = r + 1; rr < GRID_ROWS && grid[rr][c] == type; rr++) {
                count++;
            }
            if (count >= MATCH_MIN) {
                has_match = true;
                last_matched_category = chem_tiles[type].category;
                for (int rr = r; rr < r + count; rr++) {
                    visited[rr][c] = true;
                }
                score += count * 10;
            }
        }
    }

    return has_match;
}

void remove_matches() {
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (grid[r][c] == -1) continue;
            int type = grid[r][c];
            // Horizontal
            int h_count = 1;
            for (int cc = c + 1; cc < GRID_COLS && grid[r][cc] == type; cc++) h_count++;
            if (h_count >= MATCH_MIN) {
                for (int cc = c; cc < c + h_count; cc++) grid[r][cc] = -1;
            }
            // Vertical
            int v_count = 1;
            for (int rr = r + 1; rr < GRID_ROWS && grid[rr][c] == type; rr++) v_count++;
            if (v_count >= MATCH_MIN) {
                for (int rr = r; rr < r + v_count; rr++) grid[rr][c] = -1;
            }
        }
    }
}

void drop_candies() {
    for (int c = 0; c < GRID_COLS; c++) {
        int write_r = GRID_ROWS - 1;
        for (int r = GRID_ROWS - 1; r >= 0; r--) {
            if (grid[r][c] != -1) {
                grid[write_r][c] = grid[r][c];
                if (write_r != r) grid[r][c] = -1;
                write_r--;
            }
        }
        for (int r = write_r; r >= 0; r--) {
            grid[r][c] = rand() % num_tile_types;
        }
    }
}

void show_chemistry_tip(int tile_index) {
    if (tile_index >= 0 && tile_index < num_tile_types) {
        snprintf(status_message, sizeof(status_message),
                 "Matched %s! %s",
                 chem_tiles[tile_index].compound_name,
                 chem_tiles[tile_index].hint);
    }
}

// --- Display ---
void display() {
    glClearColor(background_color[0], background_color[1], background_color[2], background_color[3]);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, window_width, 0, window_height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw grid background
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            float x = 10 + c * TILE_SIZE;
            float y = window_height - 50 - (r + 1) * TILE_SIZE;
            // Use category color for tile background
            int tile_idx = grid[r][c];
            float* color = (tile_idx == -1) ? 
                (float[3]){0.2f, 0.2f, 0.2f} : 
                category_colors[chem_tiles[tile_idx].category % MAX_TILE_TYPES];
            draw_rect(x, y, TILE_SIZE, TILE_SIZE, color);
        }
    }

    // Render chemistry text
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (grid[r][c] == -1) continue;
            float x = 10 + c * TILE_SIZE + 5;
            float y = window_height - 50 - (r + 1) * TILE_SIZE + TILE_SIZE/2 + 8;
            render_text(chem_tiles[grid[r][c]].display_text, x, y);
        }
    }

    // Draw selection highlight
    if (selected_row != -1) {
        float sel_color[3] = {1.0f, 1.0f, 0.0f};
        float x = 10 + selected_col * TILE_SIZE;
        float y = window_height - 50 - (selected_row + 1) * TILE_SIZE;
        glLineWidth(3.0f);
        glColor3fv(sel_color);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x + TILE_SIZE, y);
        glVertex2f(x + TILE_SIZE, y + TILE_SIZE);
        glVertex2f(x, y + TILE_SIZE);
        glEnd();
        glColor3f(1.0f, 1.0f, 1.0f);
    }

    // Score and status
    char score_str[50];
    snprintf(score_str, 50, "Score: %d", score);
    render_text(score_str, 10, window_height - 30);
    render_text(status_message, 10, 10);

    glutSwapBuffers();
}

void draw_rect(float x, float y, float w, float h, float color[3]) {
    glColor3fv(color);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
}

void set_status_message(const char* msg) {
    strncpy(status_message, msg, sizeof(status_message) - 1);
    status_message[sizeof(status_message) - 1] = '\0';
}

// --- GLUT Callbacks ---
void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && !is_animating) {
        int col = (x - 10) / TILE_SIZE;
        float gl_y = window_height - y;
        int row = (int)((window_height - 50 - gl_y) / TILE_SIZE);
        if (col >= 0 && col < GRID_COLS && row >= 0 && row < GRID_ROWS) {
            if (selected_row == -1) {
                selected_row = row;
                selected_col = col;
            } else {
                if (is_adjacent(selected_row, selected_col, row, col)) {
                    swap_candies(selected_row, selected_col, row, col);
                    bool valid = find_matches();
                    if (!valid) {
                        swap_candies(selected_row, selected_col, row, col);
                        set_status_message("Invalid move! Match 3+ of the same compound type.");
                    } else {
                        is_animating = true;
                        // Show tip after match processing
                    }
                }
                selected_row = -1;
                selected_col = -1;
            }
            glutPostRedisplay();
        }
    }
}

void reshape(int w, int h) {
    window_width = w;
    window_height = h;
    glViewport(0, 0, w, h);
    glutPostRedisplay();
}

void idle() {
    if (is_animating) {
        remove_matches();
        drop_candies();
        if (!find_matches()) {
            is_animating = false;
            // Show educational tip
            if (last_matched_category != -1) {
                // Find a tile with this category to show tip
                for (int i = 0; i < num_tile_types; i++) {
                    if (chem_tiles[i].category == last_matched_category) {
                        show_chemistry_tip(i);
                        break;
                    }
                }
                last_matched_category = -1;
            }
        }
        glutPostRedisplay();
    }
}

void init() {
    load_chemistry_csv("chemistry_tiles.csv");
    fill_grid();
    set_status_message("Match 3+ of the same functional group!");
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("Chemistry Match: ACS/MCAT Prep");

    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutIdleFunc(idle);

    init();
    glutMainLoop();

    return 0;
}
