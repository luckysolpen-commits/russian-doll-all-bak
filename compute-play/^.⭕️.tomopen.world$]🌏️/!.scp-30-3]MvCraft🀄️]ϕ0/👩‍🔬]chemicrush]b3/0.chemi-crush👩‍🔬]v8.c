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

#define SIDEBAR_WIDTH 400
#define MAX_HISTORY 15

int window_width = GRID_COLS * TILE_SIZE + 40 + SIDEBAR_WIDTH;
int window_height = GRID_ROWS * TILE_SIZE + 130;  // Extra space for full info

float info_panel_h = 70;  // Height of bottom info panel

int selected_row = -1;
int selected_col = -1;

float background_color[4] = {0.08f, 0.08f, 0.12f, 1.0f};

// --- History ---
typedef struct {
    int swapped1;
    int swapped2;
    int matched_tile;
} MatchHistoryEntry;

MatchHistoryEntry match_history[MAX_HISTORY];
int history_count = 0;
int swapped_tile_1 = -1;
int swapped_tile_2 = -1;

// --- Difficulty Settings ---
int num_active_tiles = 6; // Default to 6 tiles for "Medium"
int* active_tile_indices = NULL;

// Category colors
float category_colors[MAX_TILE_TYPES][3] = {
    {0.9f, 0.3f, 0.3f}, // 0: Acids
    {0.2f, 0.6f, 1.0f}, // 1: Alcohols/Amines
    {0.4f, 0.9f, 0.4f}, // 2: Sugars
    {0.9f, 0.5f, 0.9f}, // 3: Ketones
    {0.9f, 0.5f, 0.9f}, // 4: Amines (reuse or adjust)
    {1.0f, 0.9f, 0.3f}, // 5: Aromatics
    {0.8f, 0.8f, 0.8f}, // 6: Salts
    {0.9f, 0.2f, 0.7f}, // 7: Strong acids
};



const char* category_names[] = {
    "Acid",
    "Alcohol/Amine",
    "Sugar",
    "Ketone",
    "Amine",
    "Aromatic",
    "Salt",
    "Strong Acid",
    "Alkane"
};
const int num_category_names = sizeof(category_names) / sizeof(char*);

float emoji_scale;
float font_color[3] = {1.0f, 1.0f, 1.0f};

// --- Menu --- 
#define MENU_BAR_HEIGHT 40
typedef struct { float x, y, w, h; } Rect;
bool is_menu_open = false;
Rect difficulty_button;
const char* difficulty_labels[] = {"Easy (5)", "Medium (6)", "Hard (8)", "Expert (All)"};
const int difficulty_values[] = {5, 6, 8, 999};
const int num_difficulty_options = 4;
Rect difficulty_options[4];


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

// Renders an emoji at a specific height and returns its rendered width
float render_emoji_scaled(unsigned int codepoint, float x, float y, float desired_h) {
    FT_Error err = FT_Load_Char(emoji_face, codepoint, FT_LOAD_RENDER | FT_LOAD_COLOR);
    if (err) { return 0; } // Silently fail in-game

    FT_GlyphSlot slot = emoji_face->glyph;
    if (!slot->bitmap.buffer || slot->bitmap.pixel_mode != FT_PIXEL_MODE_BGRA) {
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, slot->bitmap.width, slot->bitmap.rows, 0, GL_BGRA, GL_UNSIGNED_BYTE, slot->bitmap.buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLfloat prev_color[4];
    glGetFloatv(GL_CURRENT_COLOR, prev_color);
    glColor3f(1.0f, 1.0f, 1.0f);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float scale_factor = desired_h / slot->bitmap.rows;
    float w = slot->bitmap.width * scale_factor;
    float h = desired_h;
    // Draw with (x,y) as the center of the left edge of the emoji
    float y2 = y - h / 2;

    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 1.0); glVertex2f(x, y2);
    glTexCoord2f(1.0, 1.0); glVertex2f(x + w, y2);
    glTexCoord2f(1.0, 0.0); glVertex2f(x + w, y2 + h);
    glTexCoord2f(0.0, 0.0); glVertex2f(x, y2 + h);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &texture);
    
    glColor3f(prev_color[0], prev_color[1], prev_color[2]);
    return w;
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

    // 1. Create a shuffled list of all possible tile indices
    int* all_indices = malloc(num_tile_types * sizeof(int));
    for (int i = 0; i < num_tile_types; i++) {
        all_indices[i] = i;
    }
    for (int i = num_tile_types - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = all_indices[i];
        all_indices[i] = all_indices[j];
        all_indices[j] = temp;
    }

    // 2. Select the first 'num_active_tiles' for the game
    if (active_tile_indices != NULL) {
        free(active_tile_indices);
    }
    // Ensure we don't try to use more tiles than available
    if (num_active_tiles > num_tile_types) {
        num_active_tiles = num_tile_types;
    }
    active_tile_indices = malloc(num_active_tiles * sizeof(int));
    for (int i = 0; i < num_active_tiles; i++) {
        active_tile_indices[i] = all_indices[i];
    }
    free(all_indices);

    // 3. Fill the grid with the active tiles
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            grid[r][c] = active_tile_indices[rand() % num_active_tiles];
        }
    }

    // 4. Remove initial matches
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
            grid[r][c] = active_tile_indices[rand() % num_active_tiles];
        }
    }
}

// --- LEARNING DISPLAY: Show name + FORMULA + hint ---
// --- Hint Logic ---


bool check_for_match_at(int r, int c) {
    int type = grid[r][c];
    if (type == -1) return false;

    // Check horizontal (e.g., X X [T] or [T] X X or X [T] X)
    if (c > 0 && c < GRID_COLS - 1 && grid[r][c-1] == type && grid[r][c+1] == type) return true; // X [T] X
    if (c > 1 && grid[r][c-1] == type && grid[r][c-2] == type) return true; // X X [T]
    if (c < GRID_COLS - 2 && grid[r][c+1] == type && grid[r][c+2] == type) return true; // [T] X X

    // Check vertical (similar logic)
    if (r > 0 && r < GRID_ROWS - 1 && grid[r-1][c] == type && grid[r+1][c] == type) return true; // X [T] X
    if (r > 1 && grid[r-1][c] == type && grid[r-2][c] == type) return true; // X X [T]
    if (r < GRID_ROWS - 2 && grid[r+1][c] == type && grid[r+2][c] == type) return true; // [T] X X

    return false;
}

bool find_possible_move(int* r1, int* c1, int* r2, int* c2) {
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            // Check swap with right neighbor
            if (c < GRID_COLS - 1) {
                swap_candies(r, c, r, c + 1);
                if (check_for_match_at(r, c) || check_for_match_at(r, c + 1)) {
                    *r1 = r; *c1 = c; *r2 = r; *c2 = c + 1;
                    swap_candies(r, c, r, c + 1); // Swap back
                    return true;
                }
                swap_candies(r, c, r, c + 1); // Swap back
            }
            // Check swap with bottom neighbor
            if (r < GRID_ROWS - 1) {
                swap_candies(r, c, r + 1, c);
                if (check_for_match_at(r, c) || check_for_match_at(r + 1, c)) {
                    *r1 = r; *c1 = c; *r2 = r + 1; *c2 = c;
                    swap_candies(r, c, r + 1, c); // Swap back
                    return true;
                }
                swap_candies(r, c, r + 1, c); // Swap back
            }
        }
    }
    return false;
}

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

    // --- Draw Sidebar ---
    float sidebar_x = GRID_COLS * TILE_SIZE + 40;
    float sidebar_bg[3] = {0.05f, 0.05f, 0.08f};
    draw_rect(sidebar_x, 0, SIDEBAR_WIDTH, window_height, sidebar_bg);

    // Sidebar Title
    render_text("Match History", sidebar_x + 10, window_height - 30);

    // Sidebar Content
    float emoji_h = 18.0f; // Match font height
    float text_y_offset = 7.0f; // Manual offset to align text baseline with emoji center
    for (int i = 0; i < history_count; i++) {
        MatchHistoryEntry entry = match_history[i];
        int s1_idx = entry.swapped1;
        int s2_idx = entry.swapped2;
        int match_idx = entry.matched_tile;

        if (s1_idx != -1 && s2_idx != -1 && match_idx != -1) {
            float y_pos = window_height - 70 - (i * 75); // 75px per entry
            unsigned int codepoint;
            float current_x;

            // --- Line 1: Matched ---
            current_x = sidebar_x + 10;
            decode_utf8((const unsigned char*)"✅", &codepoint);
            current_x += render_emoji_scaled(codepoint, current_x, y_pos, emoji_h) + 5;
            char match_text[256];
            snprintf(match_text, sizeof(match_text), "%s (%s)",
                     chem_tiles[match_idx].compound_name,
                     chem_tiles[match_idx].formula);
            render_text(match_text, current_x, y_pos - text_y_offset);

            // --- Line 2: Why it worked (The Chemistry!) ---
            float line2_y = y_pos - 20;
            char why_text[256];
            int category_idx = chem_tiles[match_idx].category;
            if (category_idx >= 0 && category_idx < num_category_names) {
                snprintf(why_text, sizeof(why_text), "- Why: %s is classified as an %s.",
                         chem_tiles[match_idx].compound_name,
                         category_names[category_idx]);
            } else {
                snprintf(why_text, sizeof(why_text), "- Why: %s", chem_tiles[match_idx].hint); // Fallback to hint
            }
            render_text(why_text, sidebar_x + 20, line2_y - text_y_offset);

            // --- Line 3: Swapped 1 ---
            current_x = sidebar_x + 20; // Indent
            float line3_y = y_pos - 38; // 18px below line 2
            render_text("- Swapped: ", current_x, line3_y - text_y_offset);
            current_x += 90;
            decode_utf8((const unsigned char*)chem_tiles[s1_idx].emoji, &codepoint);
            current_x += render_emoji_scaled(codepoint, current_x, line3_y, emoji_h) + 5;
            render_text(chem_tiles[s1_idx].compound_name, current_x, line3_y - text_y_offset);

            // --- Line 4: Swapped 2 ---
            current_x = sidebar_x + 20; // Indent
            float line4_y = y_pos - 56; // 18px below line 3
            render_text("- With:    ", current_x, line4_y - text_y_offset);
            current_x += 90;
            decode_utf8((const unsigned char*)chem_tiles[s2_idx].emoji, &codepoint);
            current_x += render_emoji_scaled(codepoint, current_x, line4_y, emoji_h) + 5;
            render_text(chem_tiles[s2_idx].compound_name, current_x, line4_y - text_y_offset);
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

bool is_click_in_rect(int x, int y, Rect r) {
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

void menu(int value) {
    if (value == 999) { // 999 is code for "All"
        num_active_tiles = num_tile_types;
    } else {
        num_active_tiles = value;
    }
    fill_grid(); // Reset board with new setting
    glutPostRedisplay();
}

// --- Input ---
void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && !is_animating) {
        // --- Menu Hit Detection ---
        if (is_click_in_rect(x, y, difficulty_button)) {
            is_menu_open = !is_menu_open;
            glutPostRedisplay();
            return;
        }

        if (is_menu_open) {
            for (int i = 0; i < num_difficulty_options; i++) {
                if (is_click_in_rect(x, y, difficulty_options[i])) {
                    menu(difficulty_values[i]);
                    is_menu_open = false;
                    glutPostRedisplay();
                    return;
                }
            }
            // If click was outside the dropdown, close it
            is_menu_open = false;
        }

        // --- Grid Hit Detection (with y-offset for menu) ---
        float grid_top_y_window = MENU_BAR_HEIGHT + 40;
        if (y < grid_top_y_window) return; // Click was above grid

        int col = (x - 20) / TILE_SIZE;
        int row = (y - grid_top_y_window) / TILE_SIZE;

        if (col >= 0 && col < GRID_COLS && row >= 0 && row < GRID_ROWS) {
            if (selected_row == -1) {
                selected_row = row;
                selected_col = col;
            } else {
                if (is_adjacent(selected_row, selected_col, row, col)) {
                    int tile1_idx = grid[selected_row][selected_col];
                    int tile2_idx = grid[row][col];

                    swap_candies(selected_row, selected_col, row, col);
                    bool valid = find_matches();
                    if (!valid) {
                        swap_candies(selected_row, selected_col, row, col); // Swap back
                        set_status_message("❌ Invalid move! Match 3+ of the same type.");
                    } else {
                        swapped_tile_1 = tile1_idx;
                        swapped_tile_2 = tile2_idx;
                        is_animating = true;
                    }
                }
                selected_row = -1;
                selected_col = -1;
            }
        }
        glutPostRedisplay();
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

                MatchHistoryEntry new_entry = { swapped_tile_1, swapped_tile_2, last_matched_tile };

                // Add to history (newest at the top)
                if (history_count < MAX_HISTORY) {
                    // Shift existing items down
                    for (int i = history_count; i > 0; i--) {
                        match_history[i] = match_history[i-1];
                    }
                    match_history[0] = new_entry;
                    history_count++;
                } else {
                    // Shift history down, discarding the oldest
                    for (int i = MAX_HISTORY - 1; i > 0; i--) {
                        match_history[i] = match_history[i-1];
                    }
                    match_history[0] = new_entry;
                }
                last_matched_tile = -1;
                swapped_tile_1 = -1;
                swapped_tile_2 = -1;
            }
        }
        glutPostRedisplay();
    }
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 'h') {
        int r1, c1, r2, c2;
        if (find_possible_move(&r1, &c1, &r2, &c2)) {
            char hint_msg[128];
            snprintf(hint_msg, sizeof(hint_msg), "Hint: Swap tile at row %d, col %d with its neighbor.", r1, c1);
            set_status_message(hint_msg);
            // Set selected tile to highlight the hint
            selected_row = r1;
            selected_col = c1;
        } else {
            set_status_message("No possible moves found.");
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
    glutKeyboardFunc(keyboard);

    init();
    glutMainLoop();

    if (emoji_face) FT_Done_Face(emoji_face);
    FT_Done_FreeType(ft);

    return 0;
}
