# 🎨 Chapter 8: GL-OS: Beyond ASCII
TPMOS was born in the world of text, but its future is in the world of light. Welcome to **GL-OS** - the OpenGL-based visualizer that turns ASCII maps into 2D and 3D realities. 🌟✨

---

## 🎭 The Two Theaters
TPMOS can render its reality in two ways simultaneously:

1.  **📟 ASCII-OS (CHTPM):** The classic, terminal-based OS.
2.  **🎮 GL-OS (OpenGL):** The desktop-based high-fidelity shell.

### "Like Linux and Windows"
Think of ASCII-OS and GL-OS as two separate operating systems running on the same machine:
*   **Same Format:** They both use the identical frame history format (the same "language"). 🤝
*   **Practical Isolation:** For speed and safety, they use separate session files (different "folders"). 📂
*   **Theoretical Compatibility:** Because they use the same format, you can copy a frame from ASCII-OS and open it in GL-OS. This is the **Frame Bridge**. 🌉

---

## 🪞 How GL-OS Works
GL-OS is **not** a separate game. It is a "Visualizer" for the ASCII world.

1.  **The Watcher:** GL-OS watches `current_frame.txt`.
2.  **The Parser:** It parses the ASCII characters and looks for specific patterns (e.g., `[P]` for Player, `#` for Wall).
3.  **The Projection:** It maps these characters to 3D models or sprites in an OpenGL window.
4.  **The Pulse:** It stays in sync with the 12-step pipeline. When the ASCII updates, the 3D world updates! 💓

---

## 💻 Code Example: GL-OS Initialization
The GL-OS renderer (`pieces/apps/gl_os/plugins/gl_os_renderer.c`) initializes the OpenGL context:

```c
// gl_os_renderer.c - OpenGL initialization
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H

GLFWwindow* window;
FT_Library ft;
FT_Face font_face;

void init_gl_os_renderer(int width, int height) {
    // 1. Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        exit(1);
    }

    // 2. Create window
    window = glfwCreateWindow(width, height, "GL-OS", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(1);
    }
    glfwMakeContextCurrent(window);

    // 3. Set up OpenGL state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);  // Dark background

    // 4. Load FreeType font for character rendering
    if (FT_Init_FreeType(&ft)) {
        fprintf(stderr, "Failed to initialize FreeType\n");
        exit(1);
    }
    if (FT_New_Face(ft, "/usr/share/fonts/truetype/arphic/uming.ttc", 0, &font_face)) {
        fprintf(stderr, "Failed to load font\n");
        exit(1);
    }
    FT_Set_Pixel_Sizes(font_face, 0, 32);  // 32px characters

    // 5. Set up projection matrix for 2D/3D
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)width/height, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}
```

**Key points:**
- GLFW handles cross-platform window creation (works on Windows/Linux/macOS)
- FreeType loads TrueType fonts for character rendering (supports Chinese characters)
- OpenGL perspective setup for 3D viewing

---

## 💻 Code Example: Frame Parser - ASCII to 3D Mapping
The frame parser reads `current_frame.txt` and maps characters to 3D models:

```c
// Frame parser - ASCII character to 3D model mapping
typedef struct {
    char ascii_char;
    GLuint model_vbo;     // Vertex buffer for 3D model
    GLuint model_texture; // Texture ID
    float scale;          // Model scale
} CharModel;

CharModel char_models[256];

void load_char_models() {
    // Map ASCII characters to 3D models
    char_models['#'] = load_model("models/wall.obj", "textures/stone.png", 1.0f);
    char_models['.'] = load_model("models/floor.obj", "textures/grass.png", 1.0f);
    char_models['@'] = load_model("models/decor.obj", "textures/gem.png", 0.5f);
    char_models['T'] = load_model("models/treasure.obj", "textures/gold.png", 0.7f);
    char_models['R'] = load_model("models/resource.obj", "textures/ore.png", 0.6f);
    char_models['X'] = load_model("models/player.obj", "textures/hero.png", 1.0f);
    char_models['Z'] = load_model("models/zombie.obj", "textures/undead.png", 1.0f);
    // ... more mappings
}

void render_frame(const char* frame_text) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Camera transform (first-person or free-cam)
    apply_camera_transform();

    // Parse frame and render models
    int x = 0, y = 0;
    for (int i = 0; frame_text[i] != '\0'; i++) {
        char c = frame_text[i];
        if (c == '\n') { y++; x = 0; continue; }

        CharModel* cm = &char_models[(unsigned char)c];
        if (cm->model_vbo != 0) {
            glPushMatrix();
            glTranslatef(x * 2.0f, -y * 2.0f, 0.0f);  // Space models
            glScalef(cm->scale, cm->scale, cm->scale);
            draw_model(cm->model_vbo, cm->model_texture);
            glPopMatrix();
        }
        x++;
    }

    glfwSwapBuffers(window);
}
```

**How it works:** Each ASCII character maps to a 3D model with a texture. The parser iterates through the frame text, placing models at grid positions. The result is a 3D scene that mirrors the ASCII map exactly.

---

## 💻 Code Example: First-Person Mode Camera
First-person mode lets you walk through the ASCII world in 3D:

```c
// First-person camera controls
float cam_x = 0.0f, cam_y = 1.7f, cam_z = 5.0f;  // Eye height = 1.7m
float yaw = -90.0f, pitch = 0.0f;

void update_camera() {
    // WASD movement
    float speed = 0.1f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        cam_x += sin(yaw * 0.01745f) * speed;
        cam_z += cos(yaw * 0.01745f) * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        cam_x -= sin(yaw * 0.01745f) * speed;
        cam_z -= cos(yaw * 0.01745f) * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        cam_x += cos(yaw * 0.01745f) * speed;
        cam_z -= sin(yaw * 0.01745f) * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        cam_x -= cos(yaw * 0.01745f) * speed;
        cam_z += sin(yaw * 0.01745f) * speed;
    }

    // Mouse look
    double mouse_x, mouse_y;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    yaw += (mouse_x - last_mouse_x) * 0.1f;
    pitch -= (mouse_y - last_mouse_y) * 0.1f;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    last_mouse_x = mouse_x;
    last_mouse_y = mouse_y;

    // Sync with ASCII player position
    int player_x = get_state_int("player", "pos_x");
    int player_y = get_state_int("player", "pos_y");
    cam_x = player_x * 2.0f;  // Scale to match model spacing
    cam_z = player_y * 2.0f + 5.0f;  // Offset behind player
}

void apply_camera_transform() {
    gluLookAt(cam_x, cam_y, cam_z,                    // Eye position
              cam_x + sin(yaw*0.01745f),              // Look target
              cam_y + sin(pitch*0.01745f),
              cam_z + cos(yaw*0.01745f),
              0.0f, 1.0f, 0.0f);                      // Up vector
}
```

---

## 💻 Code Example: Free-Cam Implementation
Free-cam lets you fly over the world from any angle:

```c
// Free-cam controls
float free_cam_x = 10.0f, free_cam_y = 20.0f, free_cam_z = 10.0f;
float free_cam_yaw = 0.0f, free_cam_pitch = -45.0f;

void update_free_cam() {
    float speed = 0.2f;

    // WASD + Q/E for up/down
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        free_cam_x += sin(free_cam_yaw * 0.01745f) * speed;
        free_cam_z += cos(free_cam_yaw * 0.01745f) * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        free_cam_x -= sin(free_cam_yaw * 0.01745f) * speed;
        free_cam_z -= cos(free_cam_yaw * 0.01745f) * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) free_cam_y += speed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) free_cam_y -= speed;

    // Camera bounds (don't go below ground or outside map)
    if (free_cam_y < 1.0f) free_cam_y = 1.0f;
    if (free_cam_y > 50.0f) free_cam_y = 50.0f;
    if (free_cam_x < -20.0f) free_cam_x = -20.0f;
    if (free_cam_x > 60.0f) free_cam_x = 60.0f;
    if (free_cam_z < -20.0f) free_cam_z = -20.0f;
    if (free_cam_z > 40.0f) free_cam_z = 40.0f;
}
```

---

## 💻 Code Example: Character Rendering with FreeType
For characters that don't have 3D models, FreeType renders them as textured quads:

```c
// FreeType character rendering
struct Character {
    GLuint texture_id;
    glm::ivec2 size;
    glm::ivec2 bearing;
    GLuint advance;
};

std::map<GLchar, Character> characters;

void load_font_characters() {
    FT_Set_Pixel_Sizes(font_face, 0, 32);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (GLubyte c = 0; c < 128; c++) {
        if (FT_Load_Char(font_face, c, FT_LOAD_RENDER)) continue;

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                     font_face->glyph->bitmap.width,
                     font_face->glyph->bitmap.rows,
                     0, GL_RED, GL_UNSIGNED_BYTE,
                     font_face->glyph->bitmap.buffer);

        characters[c] = {
            texture,
            glm::ivec2(font_face->glyph->bitmap.width, font_face->glyph->bitmap.rows),
            glm::ivec2(font_face->glyph->bitmap_left, font_face->glyph->bitmap_top),
            (GLuint)font_face->glyph->advance.x
        };
    }
}

void render_character(Character* ch, float x, float y, float scale) {
    float xpos = x + ch->bearing.x * scale;
    float ypos = y - (ch->size.y - ch->bearing.y) * scale;
    float w = ch->size.x * scale;
    float h = ch->size.y * scale;

    // Render textured quad
    glBindTexture(GL_TEXTURE_2D, ch->texture_id);
    glBegin(GL_QUADS);
        glTexCoord2f(0, 1); glVertex2f(xpos, ypos);
        glTexCoord2f(1, 1); glVertex2f(xpos + w, ypos);
        glTexCoord2f(1, 0); glVertex2f(xpos + w, ypos + h);
        glTexCoord2f(0, 0); glVertex2f(xpos, ypos + h);
    glEnd();
}
```

This renders any ASCII character (including Chinese characters from `uming.ttc`) as a textured quad in 3D space.

---

## 🖼️ 2D Rendering (Future)
GL-OS will support a 2D sprite mode:
- **Sprite-based rendering:** Characters rendered as 2D sprites instead of 3D models
- **Tile maps as 2D graphics:** Full tile maps rendered as 2D graphics with smooth scrolling
- **UI overlays:** Health bars, inventories, minimaps rendered as 2D overlays on the 3D scene

---

## 🌐 3D Rendering (Future)
Full 3D game support is on the roadmap:
- **Model loading:** OBJ/GLTF model files for characters, items, environments
- **Lighting and shadows:** Dynamic lights, shadow mapping, ambient occlusion
- **Physics engine integration:** Rigid body physics, collision detection, ragdoll
- **Particle effects:** Fire, smoke, magic spells, weather

---

## 🧘‍♂️ TPM Principles in GL-OS
Even in GL-OS, the **True Piece Method** holds true:
- **Same Piece state files drive 3D world:** Player position in `state.txt` determines 3D model position
- **Same 12-step pipeline:** Just the renderer (Step 12) is different
- **Same ops and modules work unchanged:** `move_entity.+x` updates state, GL-OS reads it
- **"Delete a character in text → 3D model disappears":** The file is the source of truth

> "The pixels are the shadow; the files are the light." 🕯️

---

## 🪟 Windows Compatibility Note
GL-OS is designed to be cross-platform:
- **GLFW:** Works on Windows, Linux, and macOS without code changes
- **OpenGL:** Drivers differ (Mesa on Linux, NVIDIA proprietary, Intel integrated). The GL-OS code uses standard OpenGL 2.1 which is widely supported.
- **FreeType:** Cross-platform font rendering library
- **Font paths:** Differ by OS:
  - Linux: `/usr/share/fonts/truetype/arphic/uming.ttc`
  - Windows: `C:\Windows\Fonts\arial.ttf`
  - macOS: `/System/Library/Fonts/Supplemental/Arial.ttf`

Reference: `pieces/display/windows_renderer.c` for Windows-specific rendering code.

---

## 🛠️ Developer Example: Adding a New 3D Model

### Step 1: Place Model File
Put your model in `pieces/display/models/`:
```
pieces/display/models/
├── dragon.obj
├── dragon.png
└── dragon.mtl
```

### Step 2: Map ASCII Character to Model
In `gl_os_renderer.c`, add to `load_char_models()`:
```c
char_models['D'] = load_model("models/dragon.obj", "textures/dragon.png", 2.0f);
```

### Step 3: Add Character to Map
Use op-ed or edit the map file directly:
```
####################
#  ........@......T#
#  ......Z........T#
#  ...D.R...@......T#  <-- Dragon placed here!
####################
```

### Step 4: Test in GL-OS
Launch GL-OS (`button.sh debug`), and you should see a 3D dragon model at that position!

---

## 🔮 Future Vision
- **GL-OS as primary game engine:** Most games will render in GL-OS by default
- **ASCII as "debug view" / "low-spec mode":** ASCII pipeline runs on anything with a terminal
- **Seamless switching:** Press a key to toggle between 2D/3D/ASCII views of the same world
- **Multi-monitor support:** ASCII on one screen (for debugging), GL on the other (for playing)
- **VR/AR potential:** TPMOS in virtual reality - walk through your ASCII worlds in full 3D

---

## ⚠️ Common Pitfalls

### Pitfall 1: Font File Not Found
**Symptom:** GL-OS crashes on startup with "Failed to load font" error.
**Cause:** Font path is hardcoded and doesn't exist on your system.
**Fix:** Use dynamic font path resolution:
```c
#ifdef _WIN32
    const char* font_path = "C:/Windows/Fonts/arial.ttf";
#else
    const char* font_path = "/usr/share/fonts/truetype/arphic/uming.ttc";
#endif
```

### Pitfall 2: ASCII/GL Desync
**Symptom:** 3D world doesn't match what's shown in ASCII terminal.
**Cause:** GL-OS reading stale `current_frame.txt` or different session files.
**Fix:** Ensure both renderers read from the same frame file, or implement proper frame bridge sync.

### Pitfall 3: Model Path Resolution
**Symptom:** Models appear as blank boxes or don't render.
**Cause:** Model file path is relative and working directory is wrong.
**Fix:** Use absolute paths resolved from `project_root`:
```c
char model_path[4096];
snprintf(model_path, sizeof(model_path), "%s/pieces/display/models/dragon.obj", project_root);
```

### Pitfall 4: OpenGL Context Not Created
**Symptom:** GL-OS window opens but is black/blank.
**Cause:** OpenGL context not created before rendering calls, or driver doesn't support required version.
**Fix:** Check `glfwCreateWindow()` return value, verify OpenGL 2.1+ support with `glxinfo`.

---

## 🏛️ Scholar's Corner: The "Ghost in the Machine"
One rainy evening, a tester running GL-OS noticed a strange flickering "ghost" figure standing in the corner of a map. Panicked, they searched the code for a secret NPC, but found nothing. They finally opened the raw `current_frame.txt` in a text editor and found a single, stray `?` character they had accidentally typed while testing. In GL-OS, that `?` was being rendered as a high-detail placeholder model - a glowing, translucent humanoid figure! This reminded us that in TPMOS, **"Every character has a weight."** Whether it's a pixel or a period, it is part of the world. 👻📟

---

## 📝 Study Questions
1.  Is GL-OS a separate operating system or a visualizer? Explain your answer.
2.  How does GL-OS know which 3D model to render for a specific ASCII character?
3.  Explain the phrase "The pixels are the shadow; the files are the light."
4.  **Imagine:** You change the color of a wall in the ASCII text file. Will GL-OS update the wall's color in the 3D window? Why or why not?
5.  **Write the code** to add a first-person "sprint" feature (hold Shift for 2x movement speed).
6.  **Scenario:** GL-OS renders correctly on Linux but shows blank models on Windows. What are three possible causes?
7.  **Critical Thinking:** If ASCII-OS and GL-OS use the same frame format, could you record a gameplay session in ASCII and replay it in GL-OS? How would you implement this?

---
[Return to Index](INDEX.md)
