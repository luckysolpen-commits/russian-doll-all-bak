#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdbool.h>

// --- Configuration ---
#define GRID_ROWS 8
#define GRID_COLS 8
#define TILE_SIZE 70
#define MATCH_MIN 3
#define MAX_TILE_TYPES 50

// --- Data Structure ---
typedef struct {
    char emoji[8];          // e.g., "🧪"
    char compound_name[64]; // e.g., "Acetic Acid"
    char formula[32];       // e.g., "CH₃COOH"  ← KEY LEARNING ELEMENT!
    int category;
    char hint[128];         // e.g., "Carboxylic acid, pKa=4.76"
} ChemTile;

ChemTile chem_tiles[MAX_TILE_TYPES];
int num_tile_types = 0;

// --- Globals ---
FT_Library ft;
FT_Face emoji_face; // For emojis

int grid[GRID_ROWS][GRID_COLS];
bool is_animating = false;
int score = 0;

int window_width = GRID_COLS * TILE_SIZE + 40;
int window_height = GRID_ROWS * TILE_SIZE + 130;  // Extra space for full info

float info_panel_h = 70;  // Height of bottom info panel

int selected_row = -1;
int selected_col = -1;

float background_color[4] = {0.08f, 0.08f, 0.12f, 1.0f};

// Category colors
float category_colors[MAX_TILE_TYPES][3] = {
    {0.9f, 0.3f, 0.3f}, // Acids
    {0.2f, 0.6f, 1.0f}, // Alcohols/Amines
    {0.4f, 0.9f, 0.4f}, // Sugars
    {0.9f, 0.5f, 0.9f}, // Ketones
    {0.9f, 0.5f, 0.9f}, // Amines (reuse or adjust)
    {1.0f, 0.9f, 0.3f}, // Aromatics
    {0.8f, 0.8f, 0.8f}, // Salts
    {0.9f, 0.2f, 0.7f}, // Strong acids
};

float emoji_scale;
float font_color[3] = {1.0f, 1.0f, 1.0f};

char status_message[256] = "";
char selected_info[256] = "";

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
void show_match_info(int tile_index);

// --- CSV Loader (now with formula) ---
void load_chemistry_csv(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open %s\n", filename);
        exit(1);
    }

    char line[512];
    if (!fgets(line, sizeof(line), file)) {
        fprintf(stderr, "Error: Empty CSV\n");
        fclose(file);
        exit(1);
    }

    num_tile_types = 0;
    while (fgets(line, sizeof(line), file) && num_tile_types < MAX_TILE_TYPES) {
        ChemTile* tile = &chem_tiles[num_tile_types];
        char* token = strtok(line, ",");
        if (!token) continue;

        strncpy(tile->emoji, token, sizeof(tile->emoji) - 1);
        tile->emoji[sizeof(tile->emoji) - 1] = '\0';

        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(tile->compound_name, token, sizeof(tile->compound_name) - 1);
        tile->compound_name[sizeof(tile->compound_name) - 1] = '\0';

        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(tile->formula, token, sizeof(tile->formula) - 1);
        tile->formula[sizeof(tile->formula) - 1] = '\0';

        token = strtok(NULL, ",");
        if (!token) continue;
        tile->category = atoi(token);

        token = strtok(NULL, "\n");
        if (!token) continue;
        strncpy(tile->hint, token, sizeof(tile->hint) - 1);
        tile->hint[sizeof(tile->hint) - 1] = '\0';

        // Clean quotes from hint
        if (tile->hint[0] == '"') {
            memmove(tile->hint, tile->hint + 1, strlen(tile->hint));
            size_t len = strlen(tile->hint);
            if (len > 0 && tile->hint[len-1] == '"') {
                tile->hint[len-1] = '\0';
            }
        }

        num_tile_types++;
    }
    fclose(file);

    if (num_tile_types == 0) {
        fprintf(stderr, "Error: No valid entries\n");
        exit(1);
    }
    printf("Loaded %d chemistry tiles with formulas\n", num_tile_types);
}

// --- UTF8 & Font Helpers ---
int decode_utf8(const unsigned char* str, unsigned int* codepoint) {
    if (str[0] < 0x80) {
        *codepoint = str[0];
        return 1;
    }
    if ((str[0] & 0xE0) == 0xC0) {
        if ((str[1] & 0xC0) == 0x80) {
            *codepoint = ((str[0] & 0x1F) << 6) | (str[1] & 0x3F);
            return 2;
        }
    }
    if ((str[0] & 0xF0) == 0xE0) {
        if ((str[1] & 0xC0) == 0x80 && (str[2] & 0xC0) == 0x80) {
            *codepoint = ((str[0] & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
            return 3;
        }
    }
    if ((str[0] & 0xF8) == 0xF0) {
        if ((str[1] & 0xC0) == 0x80 && (str[2] & 0xC0) == 0x80 && (str[3] & 0xC0) == 0x80) {
            *codepoint = ((str[0] & 0x07) << 18) | ((str[1] & 0x3F) << 12) | ((str[2] & 0x3F) << 6) | (str[3] & 0x3F);
            return 4;
        }
    }
    *codepoint = '?';
    return 1;
}

void render_emoji(unsigned int codepoint, float x, float y) {
    FT_Error err = FT_Load_Char(emoji_face, codepoint, FT_LOAD_RENDER | FT_LOAD_COLOR);
    if (err) {
        fprintf(stderr, "Error: Could not load glyph for codepoint U+%04X, error code: %d\n", codepoint, err);
        return;
    }

    FT_GlyphSlot slot = emoji_face->glyph;
    if (!slot->bitmap.buffer) {
        fprintf(stderr, "Error: No bitmap for glyph U+%04X\n", codepoint);
        return;
    }
    if (slot->bitmap.pixel_mode != FT_PIXEL_MODE_BGRA) {
        fprintf(stderr, "Error: Incorrect pixel mode for glyph U+%04X\n", codepoint);
        return;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, slot->bitmap.width, slot->bitmap.rows, 0, GL_BGRA, GL_UNSIGNED_BYTE, slot->bitmap.buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Store current color and set to white to avoid background color bleeding
    GLfloat prev_color[4];
    glGetFloatv(GL_CURRENT_COLOR, prev_color);
    glColor3f(1.0f, 1.0f, 1.0f);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float scale_factor = emoji_scale;
    float w = slot->bitmap.width * scale_factor;
    float h = slot->bitmap.rows * scale_factor;
    // Center the bitmap at (x, y) by offsetting by half the scaled bitmap size
    float x2 = x - w / 2;
    float y2 = y - h / 2;

    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 1.0); glVertex2f(x2, y2);
    glTexCoord2f(1.0, 1.0); glVertex2f(x2 + w, y2);
    glTexCoord2f(1.0, 0.0); glVertex2f(x2 + w, y2 + h);
    glTexCoord2f(0.0, 0.0); glVertex2f(x2, y2 + h);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &texture);
    
    // Restore previous color
    glColor3f(prev_color[0], prev_color[1], prev_color[2]);
}

void initFreeType() {
    if (FT_Init_FreeType(&ft)) {
        fprintf(stderr, "Could not init FreeType Library\n");
        exit(1);
    }

    // Load emoji face
    const char *emoji_font_path = "/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf";
    FT_Error err = FT_New_Face(ft, emoji_font_path, 0, &emoji_face);
    if (err) {
        fprintf(stderr, "Error: Could not load emoji font at %s, error code: %d\n", emoji_font_path, err);
        emoji_face = NULL;
        exit(1);
    }
    if (FT_IS_SCALABLE(emoji_face)) {
        err = FT_Set_Pixel_Sizes(emoji_face, 0, TILE_SIZE - 10);  // Adjust for tile
        if (err) {
            fprintf(stderr, "Error: Could not set pixel size for emoji font, error code: %d\n", err);
            FT_Done_Face(emoji_face);
            emoji_face = NULL;
            exit(1);
        }
    } else if (emoji_face->num_fixed_sizes > 0) {
        err = FT_Select_Size(emoji_face, 0);
        if (err) {
            fprintf(stderr, "Error: Could not select size for emoji font, error code: %d\n", err);
            FT_Done_Face(emoji_face);
            emoji_face = NULL;
            exit(1);
        }
    } else {
        fprintf(stderr, "Error: No fixed sizes available in emoji font\n");
        FT_Done_Face(emoji_face);
        emoji_face = NULL;
        exit(1);
    }

    // Calculate emoji scale to fit tile
    int loaded_emoji_size = emoji_face->size->metrics.y_ppem;
    emoji_scale = (float)(TILE_SIZE * 0.8f) / (float)loaded_emoji_size;  // 80% of tile
    fprintf(stderr, "Emoji font loaded, loaded size: %d, scale: %f\n", loaded_emoji_size, emoji_scale);
}

// --- Text Rendering ---
void render_text(const char* str, float x, float y) {
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x, y);
    while (*str) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *str++);
    }
}

// --- Game Logic (unchanged core) ---
void fill_grid() {
    srand(time(NULL));
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            grid[r][c] = rand() % num_tile_types;
        }
    }
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

int last_matched_tile = -1;

bool find_matches() {
    bool has_match = false;
    bool visited[GRID_ROWS][GRID_COLS] = {false};

    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (visited[r][c]) continue;
            int type = grid[r][c];
            if (type == -1) continue;
            int count = 1;
            for (int cc = c + 1; cc < GRID_COLS && grid[r][cc] == type; cc++) count++;
            if (count >= MATCH_MIN) {
                has_match = true;
                last_matched_tile = type;
                for (int cc = c; cc < c + count; cc++) visited[r][cc] = true;
                score += count * 10;
            }
        }
    }

    for (int c = 0; c < GRID_COLS; c++) {
        for (int r = 0; r < GRID_ROWS; r++) {
            if (visited[r][c]) continue;
            int type = grid[r][c];
            if (type == -1) continue;
            int count = 1;
            for (int rr = r + 1; rr < GRID_ROWS && grid[rr][c] == type; rr++) count++;
            if (count >= MATCH_MIN) {
                has_match = true;
                last_matched_tile = type;
                for (int rr = r; rr < r + count; rr++) visited[rr][c] = true;
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
            int h_count = 1;
            for (int cc = c + 1; cc < GRID_COLS && grid[r][cc] == type; cc++) h_count++;
            if (h_count >= MATCH_MIN) {
                for (int cc = c; cc < c + h_count; cc++) grid[r][cc] = -1;
            }
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

// --- LEARNING DISPLAY: Show name + FORMULA + hint ---
void show_match_info(int tile_index) {
    if (tile_index >= 0 && tile_index < num_tile_types) {
        snprintf(status_message, sizeof(status_message),
                 "✅ Matched %s (%s)! %s",
                 chem_tiles[tile_index].compound_name,
                 chem_tiles[tile_index].formula,    // ← FORMULA INCLUDED!
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

    // Info panel at bottom
    float info_bg[3] = {0.15f, 0.15f, 0.2f};
    draw_rect(0, 0, window_width, info_panel_h, info_bg);

    // Grid (bottom-up drawing)
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            float x = 20 + c * TILE_SIZE;
            float y = info_panel_h + 20 + (GRID_ROWS - 1 - r) * TILE_SIZE;

            // Colored background by category
            int tile_idx = grid[r][c];
            float* color = (tile_idx == -1) ? 
                (float[3]){0.2f, 0.2f, 0.2f} : 
                category_colors[chem_tiles[tile_idx].category % MAX_TILE_TYPES];
            draw_rect(x, y, TILE_SIZE, TILE_SIZE, color);

            // Emoji only (no text!)
            if (tile_idx != -1) {
                const char* emoji = chem_tiles[tile_idx].emoji;
                unsigned int codepoint;
                decode_utf8((const unsigned char*)emoji, &codepoint);
                // Position emoji at the exact center of the tile
                float emoji_x = x + TILE_SIZE / 2;
                float emoji_y = y + TILE_SIZE / 2;
                
                // Ensure no background color affects emoji rendering
                glColor3f(1.0f, 1.0f, 1.0f);
                render_emoji(codepoint, emoji_x, emoji_y);
            }
        }
    }

    // Selection highlight
    if (selected_row != -1) {
        float x = 20 + selected_col * TILE_SIZE;
        float y = info_panel_h + 20 + (GRID_ROWS - 1 - selected_row) * TILE_SIZE;
        glColor3f(1.0f, 1.0f, 0.0f);
        glLineWidth(3.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x + TILE_SIZE, y);
        glVertex2f(x + TILE_SIZE, y + TILE_SIZE);
        glVertex2f(x, y + TILE_SIZE);
        glEnd();
        glColor3f(1.0f, 1.0f, 1.0f);
    }

    // Score
    char score_str[64];
    snprintf(score_str, sizeof(score_str), "Score: %d", score);
    render_text(score_str, 20, window_height - 30);

    // Status (match feedback) - moved to bottom of info panel
    render_text(status_message, 20, 10);

    // Selected tile info: NAME (FORMULA) — HINT
    if (selected_row != -1) {
        int tile_idx = grid[selected_row][selected_col];
        if (tile_idx != -1) {
            snprintf(selected_info, sizeof(selected_info),
                     "Selected: %s (%s) — %s",
                     chem_tiles[tile_idx].compound_name,
                     chem_tiles[tile_idx].formula,    // ← CRITICAL FOR LEARNING!
                     chem_tiles[tile_idx].hint);
            render_text(selected_info, 20, 30);  // Higher up in info panel
        }
    }

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
}

void set_status_message(const char* msg) {
    strncpy(status_message, msg, sizeof(status_message) - 1);
    status_message[sizeof(status_message) - 1] = '\0';
}

// --- Input ---
void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && !is_animating) {
        // Convert GLUT y (top-left origin) to OpenGL y (bottom-left origin)
        float gl_y = window_height - y;
        
        // Only allow clicking in the grid area (not in the info panel at the bottom)
        if (gl_y <= info_panel_h) {
            return;  // Click was in the info panel area, ignore
        }
        
        int col = (x - 20) / TILE_SIZE;
        // Calculate row from the top of the grid, similar to the reference implementation.
        // The top of the grid is 40px from the window top, which is (window_height - 40) in GL coords.
        int row = (int)((window_height - 40 - gl_y) / TILE_SIZE);

        if (col >= 0 && col < GRID_COLS && row >= 0 && row < GRID_ROWS) {
            if (selected_row == -1) {
                selected_row = row;
                selected_col = col;
                // Show full learning info on selection
                int tile_idx = grid[row][col];
                if (tile_idx != -1) {
                    snprintf(selected_info, sizeof(selected_info),
                             "Selected: %s (%s) — %s",
                             chem_tiles[tile_idx].compound_name,
                             chem_tiles[tile_idx].formula,
                             chem_tiles[tile_idx].hint);
                }
            } else {
                if (is_adjacent(selected_row, selected_col, row, col)) {
                    swap_candies(selected_row, selected_col, row, col);
                    bool valid = find_matches();
                    if (!valid) {
                        swap_candies(selected_row, selected_col, row, col);
                        set_status_message("❌ Invalid move! Match 3+ of the same type.");
                    } else {
                        is_animating = true;
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
            if (last_matched_tile != -1) {
                show_match_info(last_matched_tile);
                last_matched_tile = -1;
            }
        }
        glutPostRedisplay();
    }
}

void init() {
    initFreeType();
    load_chemistry_csv("chemistry_tiles.csv");
    fill_grid();
    set_status_message("🧪 Match 3+ emojis of the same type to reveal chemistry!");
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("🧪 Chemistry Match: Learn Formulas by Playing!");

    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutIdleFunc(idle);

    init();
    glutMainLoop();

    if (emoji_face) FT_Done_Face(emoji_face);
    FT_Done_FreeType(ft);

    return 0;
}
