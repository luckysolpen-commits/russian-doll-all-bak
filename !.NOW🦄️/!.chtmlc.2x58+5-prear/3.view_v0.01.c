
#include <GL/glut.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <dirent.h>
#include <sys/stat.h>
#include <math.h>
#include <sys/select.h>

#define MAX_ELEMENTS 50
#define MAX_MODULES 10
#define MAX_TEXT_LINES 1000
#define MAX_LINE_LENGTH 512
#define MAX_PAGE_HISTORY 20  // For back/forward navigation

#define CLIPBOARD_SIZE 10000

// Flex sizing structures
typedef enum {
    FLEX_UNIT_PIXEL,
    FLEX_UNIT_PERCENT
} FlexUnit;

typedef struct {
    float value;
    FlexUnit unit;
} FlexSize;

// UIElement structure - this must match the one in controller.c
typedef struct {
    char type[20];
    int x, y; // Original attributes from CHTML
    FlexSize width, height; // Updated to use FlexSize for percentage support
    int calculated_width, calculated_height; // Calculated values after flex resolution
    int calculated_x, calculated_y; // Calculated absolute positions after layout
    char id[50]; // Unique identifier for UI elements
    char label[50]; // Used for button label, text element value, and checkbox label
    // Multi-line text support - replace simple text_content with array of lines
    char text_content[MAX_TEXT_LINES][MAX_LINE_LENGTH];
    int num_lines; // Number of lines in the text
    int cursor_x; // X position of cursor (column)
    int cursor_y; // Y position of cursor (line number)
    int is_active; // For textfield active state
    int is_checked; // For checkbox checked state
    int slider_value; // For slider current value
    int slider_min; // For slider minimum value
    int slider_max; // For slider maximum value
    int slider_step; // For slider step increment
    float color[3];
    float alpha; // Alpha transparency
    int parent;
    // Canvas-specific properties
    int canvas_initialized; // Flag to track if canvas has been initialized
    void (*canvas_render_func)(int x, int y, int width, int height); // Render function for canvas
    char view_mode[10]; // View mode: "2d" or "3d"
    // Camera properties for 3D view
    float camera_pos[3]; // Camera position (x, y, z)
    float camera_target[3]; // Camera target (x, y, z)
    float camera_up[3]; // Camera up vector (x, y, z)
    float fov; // Field of view for 3D perspective
    // Event handling
    char onClick[50]; // Store the onClick handler function name
    // Menu-specific properties
    int menu_items_count; // Number of submenu items
    int is_open; // For menus - whether they are currently open
    // Text selection properties
    int selection_start_x; // X position of selection start
    int selection_start_y; // Y position of selection start
    int selection_end_x;   // X position of selection end
    int selection_end_y;   // Y position of selection end
    int has_selection;     // Whether there is an active selection
    // Clipboard for this element
    char clipboard[CLIPBOARD_SIZE];
    // Directory listing properties
    char dir_path[512]; // Directory path for directory listing elements
    char dir_entries[100][256]; // Store up to 100 directory entries
    int dir_entry_count; // Number of directory entries
    int dir_entry_selected; // Index of selected entry
    // Link properties
    char href[512]; // Store href attribute for links
    // Z-level for rendering order (0-99, default 0)
    int z_level;   // Z-level for depth ordering
    
    // AR camera support (0=disabled, 1=webcam, 2=USB cam)
    int ar_mode;
} UIElement;

// Comparison function for qsort to sort elements by z_level
int compare_elements_by_z_level(const void* a, const void* b) {
    UIElement* elem_a = (UIElement*)a;
    UIElement* elem_b = (UIElement*)b;
    return elem_a->z_level - elem_b->z_level;
}

// Forward declaration for canvas render function
void canvas_render_sample(int x, int y, int width, int height);

// Definition for the renderable shape object provided by the model
typedef struct {
    int id;
    char type[32];  // Increased size
    float x, y, z;
    float width, height, depth;  // Separate dimensions
    float color[3];
    float alpha;
    char label[64];  // Added label
    int active;      // Added active flag
} Shape;

// Forward declaration for the function to get shapes from the model
Shape* model_get_shapes(int* count);
char (*model_get_dir_entries(int* count))[256];

// Structure to store module paths from CHTML file
typedef struct {
    char path[512];
    int active;
} ModulePath;

// Structure to hold a page's content
typedef struct {
    UIElement elements[MAX_ELEMENTS];
    int num_elements;
    ModulePath modules[MAX_MODULES];
    int num_modules;
    char filename[512];
    int loaded;
} Page;

// ContentManager to handle multiple pages and navigation
typedef struct {
    Page pages[MAX_PAGE_HISTORY];
    int current_page_index;
    int num_pages_loaded;
    char current_file_path[512];
    int navigation_active;
} ContentManager;

// Forward declarations for functions defined later in this file
int load_page(const char* filename);
int activate_page(const char* filename);
const char* get_current_page_path();
int is_page_loaded(const char* filename);
void clear_page_cache();

// Global variables - these are defined and declared in the same place
// (external declarations for other files are in respective headers)
UIElement elements[MAX_ELEMENTS];
int num_elements = 0;

// Array to store module paths from CHTML file (separate from model's modules)
ModulePath parsed_modules[MAX_MODULES];
int num_parsed_modules = 0;

// Global content manager
ContentManager content_manager;

// Function to get the current page file path
const char* get_current_page_path() {
    return content_manager.current_file_path;
}

// Function to check if a page is already loaded in cache
int is_page_loaded(const char* filename) {
    for (int i = 0; i < content_manager.num_pages_loaded; i++) {
        if (strcmp(content_manager.pages[i].filename, filename) == 0) {
            return 1;
        }
    }
    return 0;
}



// Global variables to store window dimensions
int window_width = 800;
int window_height = 600;



// Emoji texture cache system
#define MAX_CACHED_EMOJIS 256
typedef struct {
    unsigned int codepoint;
    GLuint texture_id;
    int width;
    int height;
    int advance_x;  // For proper text flow
    int loaded;     // Whether this cache entry is populated
} EmojiCacheEntry;

EmojiCacheEntry emoji_cache[MAX_CACHED_EMOJIS];
int emoji_cache_size = 0;

// FreeType variables for emoji rendering
FT_Library ft;
FT_Face emoji_face;
float emoji_scale = 1.0f;

// Coordinate transformation helper functions
// Convert from real-world coordinates (y=0 at top) to OpenGL coordinates (y=0 at bottom)  
int convert_y_to_opengl(int y) {
    return window_height - y;
}

// Calculate absolute Y position with coordinate transformation (used in draw_element)
int calculate_absolute_y(int parent_y, int element_y, int element_height) {
    return window_height - (parent_y + element_y + element_height);
}

// Camera-related variables and functions for AR support
static int camera_enabled = 0;
static char *dev_names[] = {"/dev/video0", "/dev/video4"}; // Webcam and USB cam
static int current_dev_index = 0;
static int fd = -1;
static void *buffer_start = NULL;
static unsigned int buffer_length = 0;
static unsigned char *rgb_data = NULL;
static int cam_width = 640;
static int cam_height = 480;
static int RGB_SIZE; // RGBA
static struct termios old_termios;
static unsigned int current_pixelformat = V4L2_PIX_FMT_YUYV;

// Function prototypes for camera operations
int init_camera_for_element(UIElement* el);
static void cleanup_camera(void);
static int read_frame(void);
static void draw_camera_background_for_element(UIElement* el);
static void init_mmap(void);
static void init_device(int device_index);
static void start_capturing(void);
static void yuyv_to_rgb(unsigned char *yuyv, unsigned char *rgb, int size);
static void yuv420_to_rgb(unsigned char *yuv, unsigned char *rgb, int width, int height);
static void mjpg_to_rgb(unsigned char *mjpg, int size, unsigned char *rgb);
static int try_format(int width, int height, unsigned int pixelformat);
static int xioctl(int fh, int request, void *arg);
static void errno_exit(const char *s);
static void signal_handler(int sig);
static const char *pixelformat_to_string(unsigned int format);

// Helper function to convert pixel format to string
static const char *pixelformat_to_string(unsigned int format) {
    static char str[5];
    str[0] = format & 0xFF;
    str[1] = (format >> 8) & 0xFF;
    str[2] = (format >> 16) & 0xFF;
    str[3] = (format >> 24) & 0xFF;
    str[4] = '\0';
    return str;
}

static int xioctl(int fh, int request, void *arg) {
    int r;
    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);
    return r;
}

static void errno_exit(const char *s) {
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    cleanup_camera();
    exit(EXIT_FAILURE);
}

static void cleanup_camera(void) {
    static int cleaned = 0;
    if (cleaned) return; // Prevent multiple cleanups
    cleaned = 1;

    if (fd != -1) {
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl(fd, VIDIOC_STREAMOFF, &type);
        if (buffer_start) {
            munmap(buffer_start, buffer_length);
            buffer_start = NULL;
        }
        close(fd);
        fd = -1;
    }
    if (rgb_data) {
        free(rgb_data);
        rgb_data = NULL;
    }
    // Restore terminal settings if they were changed
}

static void signal_handler(int sig) {
    cleanup_camera();
    exit(0);
}

static void yuyv_to_rgb(unsigned char *yuyv, unsigned char *rgb, int size) {
    int i, j;
    int max_pixels = (RGB_SIZE / 4); // RGBA (4 bytes per pixel)
    for (i = 0, j = 0; i < size - 3 && j < RGB_SIZE - 7; i += 4, j += 8) {
        int y0 = yuyv[i + 0];
        int u  = yuyv[i + 1] - 128;
        int y1 = yuyv[i + 2];
        int v  = yuyv[i + 3] - 128;

        // First pixel
        int r = (298 * y0 + 409 * v + 128) >> 8;
        int g = (298 * y0 - 100 * u - 208 * v + 128) >> 8;
        int b = (298 * y0 + 516 * u + 128) >> 8;
        rgb[j + 0] = (r < 0) ? 0 : (r > 255) ? 255 : r;
        rgb[j + 1] = (g < 0) ? 0 : (g > 255) ? 255 : g;
        rgb[j + 2] = (b < 0) ? 0 : (b > 255) ? 255 : b;
        rgb[j + 3] = 255; // Alpha = 255 (opaque)

        // Second pixel
        r = (298 * y1 + 409 * v + 128) >> 8;
        g = (298 * y1 - 100 * u - 208 * v + 128) >> 8;
        b = (298 * y1 + 516 * u + 128) >> 8;
        rgb[j + 4] = (r < 0) ? 0 : (r > 255) ? 255 : r;
        rgb[j + 5] = (g < 0) ? 0 : (g > 255) ? 255 : g;
        rgb[j + 6] = (b < 0) ? 0 : (b > 255) ? 255 : b;
        rgb[j + 7] = 255; // Alpha = 255 (opaque)
    }
}

static void yuv420_to_rgb(unsigned char *yuv, unsigned char *rgb, int width, int height) {
    int y_size = width * height;
    int uv_size = y_size / 4;
    unsigned char *y = yuv;
    unsigned char *u = yuv + y_size;
    unsigned char *v = u + uv_size;

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            int y_idx = j * width + i;
            int uv_idx = (j / 2) * (width / 2) + (i / 2);
            int y_val = y[y_idx];
            int u_val = u[uv_idx] - 128;
            int v_val = v[uv_idx] - 128;

            int r = (298 * y_val + 409 * v_val + 128) >> 8;
            int g = (298 * y_val - 100 * u_val - 208 * v_val + 128) >> 8;
            int b = (298 * y_val + 516 * u_val + 128) >> 8;

            r = r < 0 ? 0 : (r > 255 ? 255 : r);
            g = g < 0 ? 0 : (g > 255 ? 255 : g);
            b = b < 0 ? 0 : (b > 255 ? 255 : b);

            int rgb_idx = (j * width + i) * 4; // RGBA
            rgb[rgb_idx + 0] = r;
            rgb[rgb_idx + 1] = g;
            rgb[rgb_idx + 2] = b;
            rgb[rgb_idx + 3] = 255; // Alpha = 255 (opaque)
        }
    }
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static void mjpg_to_rgb(unsigned char *mjpg, int size, unsigned char *rgb) {
    int width, height, comp;
    unsigned char *data = stbi_load_from_memory(mjpg, size, &width, &height, &comp, 3);
    if (!data || width != cam_width || height != cam_height || comp != 3) {
        fprintf(stderr, "MJPG decode failed or mismatch: %dx%d, %d components\n", width, height, comp);
        if (data) stbi_image_free(data);
        return;
    }
    // Convert RGB to RGBA
    for (int i = 0, j = 0; i < width * height * 3; i += 3, j += 4) {
        rgb[j + 0] = data[i + 0];
        rgb[j + 1] = data[i + 1];
        rgb[j + 2] = data[i + 2];
        rgb[j + 3] = 255; // Alpha = 255 (opaque)
    }
    stbi_image_free(data);
}

static void init_mmap(void) {
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        errno_exit("VIDIOC_REQBUFS");
    }

    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf)) {
        errno_exit("VIDIOC_QUERYBUF");
    }

    buffer_length = buf.length;
    buffer_start = mmap(NULL, buffer_length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (MAP_FAILED == buffer_start) errno_exit("mmap");
}

static int try_format(int width, int height, unsigned int pixelformat) {
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = pixelformat;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;

    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt)) {
        fprintf(stderr, "Failed to set format %s at %dx%d\n", 
                pixelformat_to_string(pixelformat), width, height);
        return 0;
    }

    if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt)) {
        fprintf(stderr, "VIDIOC_G_FMT error\n");
        return 0;
    }

    // Update cam_width, cam_height, and RGB_SIZE if device adjusts resolution
    if (fmt.fmt.pix.width != width || fmt.fmt.pix.height != height) {
        fprintf(stderr, "Format adjusted to %ux%u\n", 
                fmt.fmt.pix.width, fmt.fmt.pix.height);
        cam_width = fmt.fmt.pix.width;
        cam_height = fmt.fmt.pix.height;
        RGB_SIZE = cam_width * cam_height * 4; // RGBA
        // Reallocate rgb_data
        unsigned char *new_rgb_data = realloc(rgb_data, RGB_SIZE);
        if (!new_rgb_data) {
            fprintf(stderr, "Failed to reallocate rgb_data\n");
            return 0;
        }
        rgb_data = new_rgb_data;
        memset(rgb_data, 0, RGB_SIZE); // Clear buffer to prevent residual data
    }

    printf("Using format: %s, size: %ux%u, bytes: %u\n", 
           pixelformat_to_string(fmt.fmt.pix.pixelformat), 
           fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.sizeimage);
    buffer_length = fmt.fmt.pix.sizeimage;
    current_pixelformat = fmt.fmt.pix.pixelformat;
    return 1;
}

static void init_device(int device_index) {
    struct v4l2_capability cap;
    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        errno_exit("VIDIOC_QUERYCAP");
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "Device is no video capture device\n");
        cleanup_camera();
        exit(EXIT_FAILURE);
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "Device does not support streaming i/o\n");
        cleanup_camera();
        exit(EXIT_FAILURE);
    }

    // Try supported formats based on typical USB camera capabilities
    if (try_format(640, 480, V4L2_PIX_FMT_MJPEG)) {
        init_mmap();
        return;
    }
    if (try_format(640, 480, V4L2_PIX_FMT_YUYV)) {
        init_mmap();
        return;
    }
    if (try_format(320, 240, V4L2_PIX_FMT_YUYV)) {
        init_mmap();
        return;
    }
    if (try_format(1280, 720, V4L2_PIX_FMT_MJPEG)) {
        init_mmap();
        return;
    }
    if (try_format(320, 240, V4L2_PIX_FMT_MJPEG)) {
        init_mmap();
        return;
    }
    if (try_format(1280, 720, V4L2_PIX_FMT_YUV420)) {
        init_mmap();
        return;
    }
    if (try_format(320, 240, V4L2_PIX_FMT_YUV420)) {
        init_mmap();
        return;
    }

    fprintf(stderr, "No supported format/resolution found\n");
    cleanup_camera();
    exit(EXIT_FAILURE);
}

static void start_capturing(void) {
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
        errno_exit("VIDIOC_QBUF");
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type)) {
        errno_exit("VIDIOC_STREAMON");
    }
}

int init_camera_for_element(UIElement* el) {
    if (camera_enabled) {
        // Already initialized, just return
        return 1;
    }

    if (el->ar_mode != 1 && el->ar_mode != 2) {
        // Invalid AR mode
        return 0;
    }

    current_dev_index = el->ar_mode - 1; // 1 -> 0 (webcam), 2 -> 1 (USB cam)
    
    // Initialize RGB_SIZE (RGBA)
    RGB_SIZE = cam_width * cam_height * 4;

    atexit(cleanup_camera); // Ensure cleanup on exit
    signal(SIGINT, signal_handler); // Handle Ctrl+C
    
    fd = open(dev_names[current_dev_index], O_RDWR | O_NONBLOCK, 0);
    if (-1 == fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_names[current_dev_index], errno, strerror(errno));
        return 0;
    }

    init_device(current_dev_index);
    start_capturing();

    // Allocate RGB buffer
    rgb_data = malloc(RGB_SIZE);
    if (!rgb_data) {
        fprintf(stderr, "Failed to allocate rgb_data\n");
        cleanup_camera();
        return 0;
    }
    memset(rgb_data, 0, RGB_SIZE); // Initialize to zero

    camera_enabled = 1;
    return 1;
}

static int read_frame(void) {
    if (!camera_enabled) return 0;

    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
        if (errno == EAGAIN) return 0;
        errno_exit("VIDIOC_DQBUF");
    }

    // Clear rgb_data before processing to ensure no residual data
    memset(rgb_data, 0, RGB_SIZE);

    // Convert frame to RGBA
    if (current_pixelformat == V4L2_PIX_FMT_YUYV) {
        yuyv_to_rgb(buffer_start, rgb_data, buf.bytesused);
    } else if (current_pixelformat == V4L2_PIX_FMT_MJPEG) {
        mjpg_to_rgb(buffer_start, buf.bytesused, rgb_data);
    } else if (current_pixelformat == V4L2_PIX_FMT_YUV420) {
        yuv420_to_rgb(buffer_start, rgb_data, cam_width, cam_height);
    } else {
        fprintf(stderr, "Unsupported pixel format: %s\n", pixelformat_to_string(current_pixelformat));
        cleanup_camera();
        return 0;
    }

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
        errno_exit("VIDIOC_QBUF");
    }

    return 1;
}

static void draw_camera_background_for_element(UIElement* el) {
    if (!camera_enabled) return;

    // Update camera frame if needed
    read_frame();

    // Save current matrix
    glPushMatrix();
    
    // The camera image needs to be scaled to fit the canvas dimensions
    // We'll draw it at position 0,0 (the canvas origin) with the canvas dimensions
    float scale_x = (float)el->calculated_width / cam_width;
    float scale_y = (float)el->calculated_height / cam_height;
    
    // Scale appropriately
    glScalef(scale_x, scale_y, 1.0f);
    
    // Draw the camera texture - flip Y-axis to match OpenGL coordinates
    glRasterPos2i(0, cam_height);
    glPixelZoom(1.0, -1.0);
    glDrawPixels(cam_width, cam_height, GL_RGBA, GL_UNSIGNED_BYTE, rgb_data);
    glPixelZoom(1.0, 1.0);        // Restore zoom
    
    // Restore matrix
    glPopMatrix();
}

// FreeType initialization for emoji rendering
void init_freetype() {
    if (FT_Init_FreeType(&ft)) {
        fprintf(stderr, "Could not init FreeType Library\n");
        // Don't exit since emoji support is optional
        return;
    }

    const char *emoji_font_path = "/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf";
    FT_Error err = FT_New_Face(ft, emoji_font_path, 0, &emoji_face);
    if (err) {
        fprintf(stderr, "Warning: Could not load emoji font at %s, emoji rendering will be disabled\n", emoji_font_path);
        FT_Done_FreeType(ft);
        return;
    }
    
    if (FT_IS_SCALABLE(emoji_face)) {
        err = FT_Set_Pixel_Sizes(emoji_face, 0, 32);  // Smaller size for UI elements
        if (err) {
            fprintf(stderr, "Warning: Could not set pixel size for emoji font\n");
            FT_Done_Face(emoji_face);
            FT_Done_FreeType(ft);
            return;
        }
    } else if (emoji_face->num_fixed_sizes > 0) {
        err = FT_Select_Size(emoji_face, 0);
        if (err) {
            fprintf(stderr, "Warning: Could not select size for emoji font\n");
            FT_Done_Face(emoji_face);
            FT_Done_FreeType(ft);
            return;
        }
    } else {
        fprintf(stderr, "Warning: No fixed sizes available in emoji font\n");
        FT_Done_Face(emoji_face);
        FT_Done_FreeType(ft);
        return;
    }
    
    int loaded_emoji_size = emoji_face->size->metrics.y_ppem;
    emoji_scale = 32.0f / (float)loaded_emoji_size;
    printf("Emoji font loaded, size: %d, scale: %f\n", loaded_emoji_size, emoji_scale);
}

// Initialize the emoji cache
void init_emoji_cache() {
    for (int i = 0; i < MAX_CACHED_EMOJIS; i++) {
        emoji_cache[i].loaded = 0;
        emoji_cache[i].texture_id = 0;
        emoji_cache[i].codepoint = 0;
        emoji_cache[i].width = 0;
        emoji_cache[i].height = 0;
        emoji_cache[i].advance_x = 0;
    }
    emoji_cache_size = 0;
}

// Find an emoji in the cache
int find_emoji_in_cache(unsigned int codepoint) {
    for (int i = 0; i < emoji_cache_size && i < MAX_CACHED_EMOJIS; i++) {
        if (emoji_cache[i].codepoint == codepoint && emoji_cache[i].loaded) {
            return i;
        }
    }
    return -1; // Not found
}

// Add an emoji to the cache - only creates the texture ID, doesn't upload data yet
int add_emoji_to_cache(unsigned int codepoint) {
    // Check if emoji is already in cache
    int existing_index = find_emoji_in_cache(codepoint);
    if (existing_index != -1) {
        return existing_index;
    }
    
    // If cache is full, we could implement a replacement strategy
    // For now, just return if cache is full
    if (emoji_cache_size >= MAX_CACHED_EMOJIS) {
        // Optional: Implement LRU or other cache replacement strategy
        return -1;
    }
    
    // Generate a texture ID for this emoji
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    
    // Add to cache
    emoji_cache[emoji_cache_size].codepoint = codepoint;
    emoji_cache[emoji_cache_size].texture_id = texture_id;
    emoji_cache[emoji_cache_size].width = 0;
    emoji_cache[emoji_cache_size].height = 0;
    emoji_cache[emoji_cache_size].advance_x = 0;
    emoji_cache[emoji_cache_size].loaded = 0; // Will be set to 1 when texture data is uploaded
    
    int index = emoji_cache_size;
    emoji_cache_size++;
    
    return index;
}

// Function to decode UTF-8 character to Unicode codepoint
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

// Function to render a single emoji character at given position using cache
// Returns the advance width for proper text flow
int render_emoji(unsigned int codepoint, float x, float y) {
    if (!emoji_face) return 0; // Skip if emoji font not loaded, return 0 for advance width
    
    // Check if emoji is already in cache
    int cache_index = find_emoji_in_cache(codepoint);
    GLuint texture_id = 0;
    int width = 0, height = 0;
    int advance_x = 0;

    if (cache_index != -1) {
        // Emoji is in cache, use it
        texture_id = emoji_cache[cache_index].texture_id;
        width = emoji_cache[cache_index].width;
        height = emoji_cache[cache_index].height;
        advance_x = emoji_cache[cache_index].advance_x;
    } else {
        // Emoji not in cache, need to load and add to cache
        FT_Error err = FT_Load_Char(emoji_face, codepoint, FT_LOAD_RENDER | FT_LOAD_COLOR);
        if (err) {
            fprintf(stderr, "Warning: Could not load glyph for codepoint U+%04X\n", codepoint);
            return 0; // Return 0 for advance width on error
        }

        FT_GlyphSlot slot = emoji_face->glyph;
        if (!slot->bitmap.buffer) {
            fprintf(stderr, "Warning: No bitmap for glyph U+%04X\n", codepoint);
            return 0; // Return 0 for advance width on error
        }

        // Handle different pixel modes
        GLint format = GL_RGBA;
        if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) {
            format = GL_BGRA;
        } else if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
            format = GL_LUMINANCE_ALPHA;
        }

        // Add emoji to cache to get a texture ID
        cache_index = add_emoji_to_cache(codepoint);
        if (cache_index == -1) {
            // Cache is full, just load and render without caching
            GLuint temp_texture;
            glGenTextures(1, &temp_texture);
            glBindTexture(GL_TEXTURE_2D, temp_texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, slot->bitmap.width, slot->bitmap.rows, 0, 
                         format, GL_UNSIGNED_BYTE, slot->bitmap.buffer);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glEnable(GL_TEXTURE_2D);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            float scale_factor = emoji_scale;
            float w = slot->bitmap.width * scale_factor;
            float h = slot->bitmap.rows * scale_factor;
            
            // Calculate position so that emoji aligns with text baseline
            float x2 = x;
            float y2 = y;  // Position the emoji at the same baseline as regular text

            glBegin(GL_QUADS);
            glTexCoord2f(0.0, 1.0); glVertex2f(x2, y2);
            glTexCoord2f(1.0, 1.0); glVertex2f(x2 + w, y2);
            glTexCoord2f(1.0, 0.0); glVertex2f(x2 + w, y2 + h);
            glTexCoord2f(0.0, 0.0); glVertex2f(x2, y2 + h);
            glEnd();

            glDisable(GL_TEXTURE_2D);
            glDisable(GL_BLEND);
            glBindTexture(GL_TEXTURE_2D, 0);
            glDeleteTextures(1, &temp_texture);
            glColor3f(1.0f, 1.0f, 1.0f);
            // Calculate rough advance width for uncached emoji
            if (FT_Load_Char(emoji_face, codepoint, FT_LOAD_RENDER | FT_LOAD_COLOR) == 0) {
                return emoji_face->glyph->advance.x >> 6; // advance is in 1/64th pixels
            } else {
                return 0; // fallback if loading fails
            }
        }

        // Get the texture ID from the cache
        texture_id = emoji_cache[cache_index].texture_id;
        width = slot->bitmap.width;
        height = slot->bitmap.rows;
        advance_x = slot->advance.x >> 6; // advance is in 1/64th pixels

        // Upload texture data to the cached texture
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, 
                     format, GL_UNSIGNED_BYTE, slot->bitmap.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Update cache entry with dimensions and advance
        emoji_cache[cache_index].width = width;
        emoji_cache[cache_index].height = height;
        emoji_cache[cache_index].advance_x = slot->advance.x >> 6; // advance is in 1/64th pixels
        emoji_cache[cache_index].loaded = 1; // Mark as fully loaded
    }

    // Render using the texture
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindTexture(GL_TEXTURE_2D, texture_id);

    float scale_factor = emoji_scale;
    float w = width * scale_factor;
    float h = height * scale_factor;
    
    // Calculate position so that emoji aligns with text baseline
    float x2 = x;
    float y2 = y;  // Position the emoji at the same baseline as regular text

    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 1.0); glVertex2f(x2, y2);
    glTexCoord2f(1.0, 1.0); glVertex2f(x2 + w, y2);
    glTexCoord2f(1.0, 0.0); glVertex2f(x2 + w, y2 + h);
    glTexCoord2f(0.0, 0.0); glVertex2f(x2, y2 + h);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
    glColor3f(1.0f, 1.0f, 1.0f);
    return advance_x;
}

// Function to check if a string contains emoji characters
int contains_emoji(const char* str) {
    const unsigned char* s = (const unsigned char*)str;
    while (*s) {
        // Check for high Unicode values that are likely emojis
        unsigned int codepoint;
        int bytes = decode_utf8(s, &codepoint);
        if (codepoint >= 0x1F000) {  // Emoji range typically starts around U+1F000
            return 1;
        }
        // Also check for other emoji ranges
        if ((codepoint >= 0x2600 && codepoint <= 0x26FF) ||  // Misc symbols
            (codepoint >= 0x2700 && codepoint <= 0x27BF) ||  // Dingbats
            (codepoint >= 0x1F100 && codepoint <= 0x1F64F) ||  // Emoticons, etc.
            (codepoint >= 0x1F680 && codepoint <= 0x1F6FF) ||  // Transport & map symbols
            (codepoint >= 0x1F700 && codepoint <= 0x1F77F)) {  // Alchemical symbols
            return 1;
        }
        s += bytes;
    }
    return 0;
}

// Function to render text with emoji support
void render_text_with_emojis(const char* str, float x, float y, float color[3]) {
    glColor3fv(color);
    
    const unsigned char* s = (const unsigned char*)str;
    float current_x = x;
    float current_y = y;
    
    // Process the string character by character
    while (*s) {
        unsigned int codepoint;
        int bytes = decode_utf8(s, &codepoint);
        
        // Check if this character is an emoji
        int is_emoji = 0;
        if (codepoint >= 0x1F000 || 
            (codepoint >= 0x2600 && codepoint <= 0x26FF) ||  
            (codepoint >= 0x2700 && codepoint <= 0x27BF) ||  
            (codepoint >= 0x1F100 && codepoint <= 0x1F64F) ||  
            (codepoint >= 0x1F680 && codepoint <= 0x1F6FF) ||  
            (codepoint >= 0x1F700 && codepoint <= 0x1F77F)) {
            is_emoji = 1;
        }
        
        if (is_emoji) {
            // Render emoji using FreeType
            // Adjust the y position to align with text baseline - emojis might need vertical adjustment
            int advance_width = render_emoji(codepoint, current_x, current_y);  // Using same y as text baseline
            
            // Move position forward based on emoji advance width
            current_x += advance_width;
        } else {
            // For regular ASCII characters, we need to render with GLUT but properly spaced
            // Find the next emoji or end of string
            const unsigned char* next_ptr = s + bytes;
            while (*next_ptr) {
                unsigned int next_codepoint;
                int next_bytes = decode_utf8(next_ptr, &next_codepoint);
                
                if (next_codepoint >= 0x1F000 || 
                    (next_codepoint >= 0x2600 && next_codepoint <= 0x26FF) ||  
                    (next_codepoint >= 0x2700 && next_codepoint <= 0x27BF) ||  
                    (next_codepoint >= 0x1F100 && next_codepoint <= 0x1F64F) ||  
                    (next_codepoint >= 0x1F680 && next_codepoint <= 0x1F6FF) ||  
                    (next_codepoint >= 0x1F700 && next_codepoint <= 0x1F77F)) {
                    break; // Found next emoji
                }
                next_ptr += next_bytes;
            }
            
            // Create substring from s to next_ptr
            int segment_len = next_ptr - s;
            char* segment = malloc(segment_len + 1);
            strncpy(segment, (const char*)s, segment_len);
            segment[segment_len] = '\0';
            
            // Render this ASCII segment with GLUT at current position
            glRasterPos2f(current_x, current_y);
            for (char* c = segment; *c != '\0'; c++) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
            }
            
            // Calculate the width of the rendered segment
            float segment_width = 0;
            for (char* c = segment; *c != '\0'; c++) {
                segment_width += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, *c);
            }
            current_x += segment_width;
            
            free(segment);
            
            // Update s to continue after the ASCII segment
            s = next_ptr;
            continue; // Continue the main loop without incrementing s again
        }
        
        s += bytes;
    }
    
    glColor3f(1.0f, 1.0f, 1.0f);
}

// Simple parser for our C-HTML format
void parse_chtml(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening CHTML file");
        return;
    }

    char line[512]; // Increased buffer size for longer lines
    num_elements = 0;
    int parent_stack[MAX_ELEMENTS];
    int stack_top = -1;

    while (fgets(line, sizeof(line), file)) {
        // Trim leading/trailing whitespace
        char* trimmed_line = line;
        while (*trimmed_line == ' ' || *trimmed_line == '\t') trimmed_line++;
        size_t len = strlen(trimmed_line);
        while (len > 0 && (trimmed_line[len-1] == '\n' || trimmed_line[len-1] == '\r' || trimmed_line[len-1] == ' ' || trimmed_line[len-1] == '\t')) {
            trimmed_line[--len] = '\0';
        }

        if (len == 0) continue; // Skip empty lines

        // Check for closing tag
        if (trimmed_line[0] == '<' && trimmed_line[1] == '/') {
            if (stack_top > -1) {
                stack_top--;
            }
            continue;
        }

        // Check for opening tag
        if (trimmed_line[0] != '<') continue;

        // Extract tag name
        char* tag_start = trimmed_line + 1;
        char* tag_end = strchr(tag_start, ' ');
        if (!tag_end) tag_end = strchr(tag_start, '>');
        if (!tag_end) continue; // Malformed tag

        char tag_name[50];
        strncpy(tag_name, tag_start, tag_end - tag_start);
        tag_name[tag_end - tag_start] = '\0';
        
        // Check if this is a module tag
        if (strcmp(tag_name, "module") == 0) {
            // Parse module path from the tag content (not attributes)
            char* content_start = tag_end + 1;
            char* end_tag = strstr(content_start, "</module>");
            
            if (end_tag) {
                *end_tag = '\0'; // Temporarily terminate to get content
                // Trim whitespace from module path
                char* module_path = content_start;
                while (*module_path == ' ' || *module_path == '\t') module_path++;
                
                char* end = module_path + strlen(module_path) - 1;
                while (end > module_path && (*end == ' ' || *end == '\t')) end--;
                *(end + 1) = '\0';
                
                // Add to modules array if not full
                if (num_parsed_modules < MAX_MODULES) {
                    strncpy(parsed_modules[num_parsed_modules].path, module_path, sizeof(parsed_modules[num_parsed_modules].path) - 1);
                    parsed_modules[num_parsed_modules].path[sizeof(parsed_modules[num_parsed_modules].path) - 1] = '\0';
                    parsed_modules[num_parsed_modules].active = 1;
                    num_parsed_modules++;
                    printf("Parsed module: %s\n", parsed_modules[num_parsed_modules-1].path);
                } else {
                    fprintf(stderr, "Warning: Maximum number of modules reached!\n");
                }
                *end_tag = '<'; // Restore the original character
            } else {
                // Simple tag without matching end tag found, look for content until end of line
                // For now, just print a debug message
                printf("Debug: Found module tag but no closing tag on this line\n");
            }
            continue; // Continue to next line, don't process as UI element
        }

        if (num_elements >= MAX_ELEMENTS) {
            fprintf(stderr, "Error: Max elements reached!\n");
            break;
        }

        elements[num_elements].parent = (stack_top > -1) ? parent_stack[stack_top] : -1;
        strcpy(elements[num_elements].type, tag_name);
        elements[num_elements].id[0] = '\0'; // Initialize ID to empty string
        elements[num_elements].alpha = 1.0f; // Default alpha to opaque
        elements[num_elements].is_active = 0;
        elements[num_elements].cursor_x = 0;
        elements[num_elements].cursor_y = 0;
        elements[num_elements].num_lines = 1;
        // Initialize all lines to empty
        for (int line_idx = 0; line_idx < MAX_TEXT_LINES; line_idx++) {
            elements[num_elements].text_content[line_idx][0] = '\0';
        }
        // For textfield and textarea, set first line to the value attribute
        if (strcmp(tag_name, "textfield") == 0 || strcmp(tag_name, "textarea") == 0) {
            // Find the value attribute and set the first line
            char* attr_start = tag_end;
            while (attr_start && (attr_start = strstr(attr_start, " ")) != NULL) {
                attr_start++; // Move past space
                char* eq_sign = strchr(attr_start, '=');
                if (!eq_sign) break; // No more attributes

                char attr_name[50];
                strncpy(attr_name, attr_start, eq_sign - attr_start);
                attr_name[eq_sign - attr_start] = '\0';

                char* value_start = eq_sign + 1;
                if (*value_start != '"') break; // Value not quoted
                value_start++; // Move past opening quote

                char* value_end = strchr(value_start, '"');
                if (!value_end) break; // No closing quote

                char attr_value[256];
                strncpy(attr_value, value_start, value_end - value_start);
                attr_value[value_end - value_start] = '\0';

                if (strcmp(attr_name, "value") == 0) {
                    // Handle multi-line value by splitting on &#10; (HTML newline entity)
                    int line_idx = 0;
                    char* token = strtok(attr_value, "&#10;");
                    while (token != NULL && line_idx < MAX_TEXT_LINES) {
                        strncpy(elements[num_elements].text_content[line_idx], token, MAX_LINE_LENGTH - 1);
                        elements[num_elements].text_content[line_idx][MAX_LINE_LENGTH - 1] = '\0';
                        token = strtok(NULL, "&#10;");
                        line_idx++;
                    }
                    elements[num_elements].num_lines = line_idx > 0 ? line_idx : 1;
                    break;
                }
                attr_start = value_end + 1;
            }
        }
        elements[num_elements].selection_start_x = 0;
        elements[num_elements].selection_start_y = 0;
        elements[num_elements].selection_end_x = 0;
        elements[num_elements].selection_end_y = 0;
        elements[num_elements].has_selection = 0;
        elements[num_elements].clipboard[0] = '\0'; // Initialize clipboard to empty
        elements[num_elements].is_checked = 0;
        elements[num_elements].slider_value = 0;
        elements[num_elements].slider_min = 0;
        elements[num_elements].slider_max = 100;
        elements[num_elements].slider_step = 1;
        elements[num_elements].canvas_initialized = 0;
        elements[num_elements].canvas_render_func = NULL;
        strcpy(elements[num_elements].view_mode, "2d"); // Default to 2D view
        // Initialize camera properties for 3D
        elements[num_elements].camera_pos[0] = 0.0f;
        elements[num_elements].camera_pos[1] = 0.0f;
        elements[num_elements].camera_pos[2] = 5.0f; // Position camera in front of scene
        elements[num_elements].camera_target[0] = 0.0f;
        elements[num_elements].camera_target[1] = 0.0f;
        elements[num_elements].camera_target[2] = 0.0f; // Look at origin
        elements[num_elements].camera_up[0] = 0.0f;
        elements[num_elements].camera_up[1] = 1.0f;
        elements[num_elements].camera_up[2] = 0.0f; // Y is up
        elements[num_elements].fov = 45.0f; // Default field of view
        elements[num_elements].onClick[0] = '\0'; // Initialize onClick to empty string
        elements[num_elements].menu_items_count = 0;
        elements[num_elements].is_open = 0;
        strcpy(elements[num_elements].dir_path, ".");
        elements[num_elements].dir_entry_count = 0;
        elements[num_elements].dir_entry_selected = -1;
        elements[num_elements].href[0] = '\0'; // Initialize href to empty string
        elements[num_elements].z_level = 0;    // Initialize z-level to default (0)
        
        // Initialize flex sizing attributes
        elements[num_elements].width.value = 0;
        elements[num_elements].width.unit = FLEX_UNIT_PIXEL;
        elements[num_elements].height.value = 0;
        elements[num_elements].height.unit = FLEX_UNIT_PIXEL;
        elements[num_elements].calculated_width = 0;
        elements[num_elements].calculated_height = 0;
        elements[num_elements].calculated_x = 0;
        elements[num_elements].calculated_y = 0;

        // Parse attributes
        char* attr_start = tag_end;
        while (attr_start && (attr_start = strstr(attr_start, " ")) != NULL) {
            attr_start++; // Move past space
            char* eq_sign = strchr(attr_start, '=');
            if (!eq_sign) break; // No more attributes

            char attr_name[50];
            strncpy(attr_name, attr_start, eq_sign - attr_start);
            attr_name[eq_sign - attr_start] = '\0';

            char* value_start = eq_sign + 1;
            if (*value_start != '\"') break; // Value not quoted
            value_start++; // Move past opening quote

            char* value_end = strchr(value_start, '\"');
            if (!value_end) break; // No closing quote

            char attr_value[256];
            strncpy(attr_value, value_start, value_end - value_start);
            attr_value[value_end - value_start] = '\0';

            if (strcmp(attr_name, "x") == 0) elements[num_elements].x = atoi(attr_value);
            else if (strcmp(attr_name, "y") == 0) elements[num_elements].y = atoi(attr_value);
            else if (strcmp(attr_name, "width") == 0) {
                // Check if the value contains a percentage sign
                if (strchr(attr_value, '%') != NULL) {
                    // Parse the numeric value without the % sign
                    char* percent_str = strdup(attr_value);
                    char* percent_ptr = strchr(percent_str, '%');
                    if (percent_ptr) *percent_ptr = '\0'; // Remove the % sign
                    elements[num_elements].width.value = atof(percent_str);
                    elements[num_elements].width.unit = FLEX_UNIT_PERCENT;
                    free(percent_str);
                } else {
                    // Regular pixel value
                    elements[num_elements].width.value = atof(attr_value);
                    elements[num_elements].width.unit = FLEX_UNIT_PIXEL;
                }
            }
            else if (strcmp(attr_name, "height") == 0) {
                // Check if the value contains a percentage sign
                if (strchr(attr_value, '%') != NULL) {
                    // Parse the numeric value without the % sign
                    char* percent_str = strdup(attr_value);
                    char* percent_ptr = strchr(percent_str, '%');
                    if (percent_ptr) *percent_ptr = '\0'; // Remove the % sign
                    elements[num_elements].height.value = atof(percent_str);
                    elements[num_elements].height.unit = FLEX_UNIT_PERCENT;
                    free(percent_str);
                } else {
                    // Regular pixel value
                    elements[num_elements].height.value = atof(attr_value);
                    elements[num_elements].height.unit = FLEX_UNIT_PIXEL;
                }
            }
            else if (strcmp(attr_name, "id") == 0) strncpy(elements[num_elements].id, attr_value, 49);
            else if (strcmp(attr_name, "label") == 0) strncpy(elements[num_elements].label, attr_value, 49);
            else if (strcmp(attr_name, "value") == 0) {
                if (strcmp(elements[num_elements].type, "textfield") == 0 || strcmp(elements[num_elements].type, "textarea") == 0) {
                    // For textfield and textarea, set first line to the value attribute
                    // Handle multi-line value by splitting on &#10; (HTML newline entity)
                    int line_idx = 0;
                    char* temp_value = malloc(strlen(attr_value) + 1);
                    strcpy(temp_value, attr_value);
                    char* token = strtok(temp_value, "&#10;");
                    while (token != NULL && line_idx < MAX_TEXT_LINES) {
                        strncpy(elements[num_elements].text_content[line_idx], token, MAX_LINE_LENGTH - 1);
                        elements[num_elements].text_content[line_idx][MAX_LINE_LENGTH - 1] = '\0';
                        token = strtok(NULL, "&#10;");
                        line_idx++;
                    }
                    elements[num_elements].num_lines = line_idx > 0 ? line_idx : 1;
                    // Set cursor to end of first line
                    elements[num_elements].cursor_x = (line_idx > 0) ? strlen(elements[num_elements].text_content[0]) : 0;
                    elements[num_elements].cursor_y = 0;
                    free(temp_value);
                } else if (strcmp(elements[num_elements].type, "slider") == 0) {
                    elements[num_elements].slider_value = atoi(attr_value);
                } else if (strcmp(elements[num_elements].type, "text") == 0) {
                    // For text elements, store the value in the label field
                    strncpy(elements[num_elements].label, attr_value, 49);
                }
            }
            else if (strcmp(attr_name, "checked") == 0) {
                if (strcmp(attr_value, "true") == 0) {
                    elements[num_elements].is_checked = 1;
                }
            }
            else if (strcmp(attr_name, "min") == 0) elements[num_elements].slider_min = atoi(attr_value);
            else if (strcmp(attr_name, "max") == 0) elements[num_elements].slider_max = atoi(attr_value);
            else if (strcmp(attr_name, "step") == 0) elements[num_elements].slider_step = atoi(attr_value);
            else if (strcmp(attr_name, "color") == 0) {
                long color = strtol(attr_value + 1, NULL, 16);
                elements[num_elements].color[0] = ((color >> 16) & 0xFF) / 255.0f;
                elements[num_elements].color[1] = ((color >> 8) & 0xFF) / 255.0f;
                elements[num_elements].color[2] = (color & 0xFF) / 255.0f;
            }
            else if (strcmp(attr_name, "alpha") == 0) {
                elements[num_elements].alpha = atof(attr_value);
            }
            else if (strcmp(attr_name, "onClick") == 0) {
                strncpy(elements[num_elements].onClick, attr_value, 49);
                elements[num_elements].onClick[49] = '\0'; // Ensure null termination
            }
            else if (strcmp(attr_name, "view_mode") == 0 || strcmp(attr_name, "viewMode") == 0) {
                // Support both view_mode and viewMode for consistency
                strncpy(elements[num_elements].view_mode, attr_value, 9);
                elements[num_elements].view_mode[9] = '\0'; // Ensure null termination
            }
            else if (strcmp(attr_name, "path") == 0) {
                // For dirlist elements, set the directory path
                if (strcmp(elements[num_elements].type, "dirlist") == 0) {
                    strncpy(elements[num_elements].dir_path, attr_value, 511);
                    elements[num_elements].dir_path[511] = '\0'; // Ensure null termination
                }
            }
            else if (strcmp(attr_name, "href") == 0) {
                // For link elements, store the href value in the href field
                if (strcmp(elements[num_elements].type, "link") == 0 || 
                    strcmp(elements[num_elements].type, "a") == 0) {
                    strncpy(elements[num_elements].href, attr_value, 511); // Store href in href field
                    elements[num_elements].href[511] = '\0'; // Ensure null termination
                    if (strlen(elements[num_elements].label) == 0) {
                        // If there's no label, use the href as label (filename)
                        strncpy(elements[num_elements].label, attr_value, 49);
                        elements[num_elements].label[49] = '\0';
                    }
                }
            }
            else if (strcmp(attr_name, "class") == 0) {
                // Store class attribute value if needed
                // For now, we'll just use it to identify elements
                if (strcmp(elements[num_elements].type, "link") == 0 || 
                    strcmp(elements[num_elements].type, "a") == 0) {
                    // Store class in a temporary field, or use it for styling
                }
            }
            else if (strcmp(attr_name, "z_level") == 0 || strcmp(attr_name, "zlevel") == 0) {
                int level = atoi(attr_value);
                // Clamp the z-level to valid range 0-99
                if (level < 0) level = 0;
                if (level > 99) level = 99;
                elements[num_elements].z_level = level;
            }
            attr_start = value_end + 1;
        }

        // Ensure slider_value is within min/max bounds after parsing all attributes
        if (strcmp(elements[num_elements].type, "slider") == 0) {
            if (elements[num_elements].slider_value < elements[num_elements].slider_min) {
                elements[num_elements].slider_value = elements[num_elements].slider_min;
            }
            if (elements[num_elements].slider_value > elements[num_elements].slider_max) {
                elements[num_elements].slider_value = elements[num_elements].slider_max;
            }
        }

        if (strcmp(tag_name, "panel") == 0) {
            stack_top++;
            parent_stack[stack_top] = num_elements;
        }
        else if (strcmp(tag_name, "canvas") == 0) {
            // Assign a default render function to canvas elements
            elements[num_elements].canvas_render_func = canvas_render_sample;
            stack_top++;
            parent_stack[stack_top] = num_elements;
        }
        else if (strcmp(tag_name, "menu") == 0) {
            elements[num_elements].menu_items_count = 0; // Initialize to 0, will be updated during parsing
            stack_top++;
            parent_stack[stack_top] = num_elements;
        }
        else if (strcmp(tag_name, "menuitem") == 0) {
            // menuitem elements don't change the stack (they don't contain other elements)
            // but need to increment the parent's menu_items_count
            if (stack_top >= 0) {
                int parent_index = parent_stack[stack_top];
                if (strcmp(elements[parent_index].type, "menu") == 0) {
                    elements[parent_index].menu_items_count++;
                }
            }
        }
        else if (strcmp(tag_name, "a") == 0) {
            // Handle anchor tags with href attribute for navigation
            // This element will be treated as a special text element that can be clicked
            strcpy(elements[num_elements].type, "link"); // Use "link" type instead of "a"
        }
        else if (strcmp(tag_name, "p") == 0) {
            // Paragraph tags can contain text and links
            // For now, treat as a container that may hold other text elements
            strcpy(elements[num_elements].type, "container"); // Use "container" type for p tags
        }

        num_elements++;
    }

    fclose(file);
}

void init_view(const char* filename) {
    init_freetype();  // Initialize FreeType for emoji rendering
    init_emoji_cache();  // Initialize emoji texture cache
    
    // Initialize content manager if not already initialized
    if (content_manager.num_pages_loaded == 0) {
        content_manager.current_page_index = -1;
        content_manager.num_pages_loaded = 0;
        content_manager.navigation_active = 0;
        // Initialize all page structures
        for (int i = 0; i < MAX_PAGE_HISTORY; i++) {
            content_manager.pages[i].loaded = 0;
            content_manager.pages[i].num_elements = 0;
            content_manager.pages[i].num_modules = 0;
            content_manager.pages[i].filename[0] = '\0';
        }
        content_manager.current_file_path[0] = '\0';
    }
    
    // For initial loading, directly parse the content (first page doesn't need to go through cache system)
    // Initialize modules array
    for (int i = 0; i < MAX_MODULES; i++) {
        parsed_modules[i].active = 0;
        parsed_modules[i].path[0] = '\0';
    }
    num_parsed_modules = 0;
    
    // Parse the initial file directly
    parse_chtml(filename);
    
    // Also add to cache for future navigation
    // Store the current state to cache
    if (content_manager.num_pages_loaded < MAX_PAGE_HISTORY) {
        int page_index = content_manager.num_pages_loaded;
        Page* new_page = &content_manager.pages[page_index];
        
        strncpy(new_page->filename, filename, sizeof(new_page->filename) - 1);
        new_page->filename[sizeof(new_page->filename) - 1] = '\0';
        
        new_page->num_elements = num_elements;
        for (int i = 0; i < num_elements; i++) {
            memcpy(&new_page->elements[i], &elements[i], sizeof(UIElement));
        }
        
        new_page->num_modules = num_parsed_modules;
        for (int i = 0; i < num_parsed_modules; i++) {
            memcpy(&new_page->modules[i], &parsed_modules[i], sizeof(ModulePath));
        }
        
        new_page->loaded = 1;
        
        content_manager.current_page_index = page_index;
        content_manager.num_pages_loaded++;
        strncpy(content_manager.current_file_path, filename, sizeof(content_manager.current_file_path) - 1);
        content_manager.current_file_path[sizeof(content_manager.current_file_path) - 1] = '\0';
    }
}

// Function to load a page into the content manager
int load_page(const char* filename) {
    // Check if page is already loaded
    for (int i = 0; i < content_manager.num_pages_loaded; i++) {
        if (strcmp(content_manager.pages[i].filename, filename) == 0) {
            // Page already loaded, just make it current
            content_manager.current_page_index = i;
            strncpy(content_manager.current_file_path, filename, sizeof(content_manager.current_file_path) - 1);
            content_manager.current_file_path[sizeof(content_manager.current_file_path) - 1] = '\0';
            
            // Copy page content to active elements
            Page* page = &content_manager.pages[i];
            num_elements = page->num_elements;
            for (int j = 0; j < num_elements; j++) {
                memcpy(&elements[j], &page->elements[j], sizeof(UIElement));
            }
            num_parsed_modules = page->num_modules;
            for (int j = 0; j < num_parsed_modules; j++) {
                memcpy(&parsed_modules[j], &page->modules[j], sizeof(ModulePath));
            }
            return 1;
        }
    }
    
    // Check if we need to make space for a new page
    if (content_manager.num_pages_loaded >= MAX_PAGE_HISTORY) {
        // Remove the oldest page if necessary (simple rotation)
        // In a real system, we might implement a more sophisticated cache policy
        printf("Page cache full, replacing oldest page\n");
        // For now, just add to the end, overwriting the last slot
    }
    
    // Parse the new page content
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open CHTML file: %s\n", filename);
        return 0;
    }
    fclose(file);
    
    // Store the current state temporarily
    UIElement temp_elements[MAX_ELEMENTS];
    ModulePath temp_modules[MAX_MODULES];
    int temp_num_elements = num_elements;
    int temp_num_modules = num_parsed_modules;
    
    // Copy current state to temp
    for (int i = 0; i < num_elements; i++) {
        memcpy(&temp_elements[i], &elements[i], sizeof(UIElement));
    }
    for (int i = 0; i < num_parsed_modules; i++) {
        memcpy(&temp_modules[i], &parsed_modules[i], sizeof(ModulePath));
    }
    
    // Initialize modules array for the new page
    for (int i = 0; i < MAX_MODULES; i++) {
        parsed_modules[i].active = 0;
        parsed_modules[i].path[0] = '\0';
    }
    num_parsed_modules = 0;
    
    // Parse the new page
    parse_chtml(filename);
    
    // Verify parsing was successful by checking for elements
    if (num_elements == 0) {
        printf("Warning: Parsed file %s has no UI elements\n", filename);
        // Restore original state
        num_elements = temp_num_elements;
        num_parsed_modules = temp_num_modules;
        for (int i = 0; i < num_elements; i++) {
            elements[i] = temp_elements[i];
        }
        for (int i = 0; i < num_parsed_modules; i++) {
            parsed_modules[i] = temp_modules[i];
        }
        return 0;
    }
    
    // Store the new page in the content manager
    int page_index = content_manager.num_pages_loaded;
    if (page_index >= MAX_PAGE_HISTORY) {
        page_index = MAX_PAGE_HISTORY - 1; // Overwrite last slot
    } else {
        content_manager.num_pages_loaded++;
    }
    
    Page* new_page = &content_manager.pages[page_index];
    strncpy(new_page->filename, filename, sizeof(new_page->filename) - 1);
    new_page->filename[sizeof(new_page->filename) - 1] = '\0';
    
    new_page->num_elements = num_elements;
    for (int i = 0; i < num_elements; i++) {
        memcpy(&new_page->elements[i], &elements[i], sizeof(UIElement));
    }
    
    new_page->num_modules = num_parsed_modules;
    for (int i = 0; i < num_parsed_modules; i++) {
        memcpy(&new_page->modules[i], &parsed_modules[i], sizeof(ModulePath));
    }
    
    new_page->loaded = 1;
    
    // Update current page index
    content_manager.current_page_index = page_index;
    strncpy(content_manager.current_file_path, filename, sizeof(content_manager.current_file_path) - 1);
    content_manager.current_file_path[sizeof(content_manager.current_file_path) - 1] = '\0';
    
    // Restore original state (in case we're in transition mode)
    num_elements = temp_num_elements;
    num_parsed_modules = temp_num_modules;
    for (int i = 0; i < num_elements; i++) {
        memcpy(&elements[i], &temp_elements[i], sizeof(UIElement));
    }
    for (int i = 0; i < num_parsed_modules; i++) {
        memcpy(&parsed_modules[i], &temp_modules[i], sizeof(ModulePath));
    }
    
    printf("Successfully loaded page: %s (%d elements, %d modules)\n", filename, new_page->num_elements, new_page->num_modules);
    return 1;
}

// Function to activate a loaded page (copy its content to the active elements)
int activate_page(const char* filename) {
    printf("Activating page: %s\n", filename);
    
    for (int i = 0; i < content_manager.num_pages_loaded; i++) {
        if (strcmp(content_manager.pages[i].filename, filename) == 0) {
            // Verify the page has valid content before activating
            Page* page = &content_manager.pages[i];
            if (!page->loaded || page->num_elements <= 0) {
                printf("Error: Page %s exists in cache but has no valid content\n", filename);
                return 0;
            }
            
            // Copy page content to active elements
            num_elements = page->num_elements;
            for (int j = 0; j < num_elements; j++) {
                memcpy(&elements[j], &page->elements[j], sizeof(UIElement));
            }
            num_parsed_modules = page->num_modules;
            for (int j = 0; j < num_parsed_modules; j++) {
                memcpy(&parsed_modules[j], &page->modules[j], sizeof(ModulePath));
            }
            
            content_manager.current_page_index = i;
            strncpy(content_manager.current_file_path, filename, sizeof(content_manager.current_file_path) - 1);
            content_manager.current_file_path[sizeof(content_manager.current_file_path) - 1] = '\0';
            
            printf("Successfully activated page: %s\n", filename);
            return 1;
        }
    }
    
    printf("Page not found in cache: %s\n", filename);
    return 0;
}

// Function to resolve flex size values to actual pixel values
int resolve_flex_size(FlexSize flex_size, int window_size) {
    if (flex_size.unit == FLEX_UNIT_PERCENT) {
        return (int)((flex_size.value / 100.0f) * window_size);
    } else { // FLEX_UNIT_PIXEL
        return (int)flex_size.value;
    }
}

// Function to calculate layout for a single element
void calculate_element_layout(UIElement* element) {
    // Calculate width and height based on flex units
    element->calculated_width = resolve_flex_size(element->width, window_width);
    element->calculated_height = resolve_flex_size(element->height, window_height);
    
    // For now, use the same absolute positioning as before
    // The x, y values are still absolute positions from CHTML
    element->calculated_x = element->x;
    element->calculated_y = element->y;
}

// Function to calculate layout for all elements
void calculate_layout() {
    for (int i = 0; i < num_elements; i++) {
        calculate_element_layout(&elements[i]);
    }
}

void draw_element(UIElement* el) {
    int parent_x = 0;
    int parent_y = 0;

    if (el->parent != -1) {
        parent_x = elements[el->parent].x;
        parent_y = elements[el->parent].y;
    }

    int abs_x = parent_x + el->calculated_x;
    int abs_y = calculate_absolute_y(parent_y, el->calculated_y, el->calculated_height); // Convert from real-world to OpenGL coordinates (y=0 at top becomes y=window_height at bottom)

    if (strcmp(el->type, "canvas") == 0) {
        // Draw a border for the canvas
        glColor4f(0.5f, 0.5f, 0.5f, 1.0f); // Gray border
        glBegin(GL_LINE_LOOP);
        glVertex2i(abs_x, abs_y);
        glVertex2i(abs_x + el->calculated_width, abs_y);
        glVertex2i(abs_x + el->calculated_width, abs_y + el->calculated_height);
        glVertex2i(abs_x, abs_y + el->calculated_height);
        glEnd();
        
        // Draw canvas content without changing global OpenGL state
        if (el->canvas_render_func != NULL) {
            // Enable scissor test to clip canvas content to canvas boundaries
            glEnable(GL_SCISSOR_TEST);
            // Set scissor box to canvas boundaries in window coordinates
            // OpenGL scissor coordinates: (x, y, width, height) where (x,y) is bottom-left
            // abs_y is already the bottom Y coordinate in OpenGL's coordinate system
            glScissor(abs_x, abs_y, el->calculated_width, el->calculated_height);
            
            // Save the current modelview matrix
            glPushMatrix();
            
            // Check view mode and set up appropriate projection
            if (strcmp(el->view_mode, "3d") == 0) {
                // Switch to projection matrix to set up 3D view
                glMatrixMode(GL_PROJECTION);
                glPushMatrix(); // Save current projection matrix
                glLoadIdentity();
                
                // Set up perspective projection for 3D
                gluPerspective(el->fov, (double)el->calculated_width/(double)el->calculated_height, 0.1, 100.0);
                
                // Switch back to modelview matrix for drawing
                glMatrixMode(GL_MODELVIEW);
                
                // Set up camera view
                gluLookAt(
                    el->camera_pos[0], el->camera_pos[1], el->camera_pos[2],  // Camera position
                    el->camera_target[0], el->camera_target[1], el->camera_target[2],  // Look at point
                    el->camera_up[0], el->camera_up[1], el->camera_up[2]  // Up vector
                );
            }
            
            // Apply transformation to move to canvas position (only in 2D mode)
            if (strcmp(el->view_mode, "3d") == 0) {
                // In 3D mode, we've already set up the camera, so no translation needed
                // The canvas position will be handled in the render function if needed
            } else {
                // In 2D mode, apply translation to canvas position
                glTranslatef(abs_x, abs_y, 0.0f);
            }
            
            // Call the canvas render function, adjusting the coordinates it receives
            // The function will draw in the local coordinate system
            el->canvas_render_func(0, 0, el->calculated_width, el->calculated_height);
            
            // Restore projection matrix if we were in 3D mode
            if (strcmp(el->view_mode, "3d") == 0) {
                glMatrixMode(GL_PROJECTION);
                glPopMatrix(); // Restore previous projection matrix
                glMatrixMode(GL_MODELVIEW);
            }
            
            // Restore the modelview matrix
            glPopMatrix();
            
            // Disable scissor test after drawing canvas content
            glDisable(GL_SCISSOR_TEST);
        }
    } else if (strcmp(el->type, "menu") == 0) {
        // For context menus (like "Generic Context Menu"), only draw the menu bar when open
        // For permanent menu bars, always draw the menu bar
        int should_draw_menu_bar = 1; // By default, draw for permanent menu bars
        
        // Check if this is likely a context menu (by label or other heuristics)
        if (strstr(el->label, "context_menu") != NULL) {
            // This is a context menu - only draw if it's open
            should_draw_menu_bar = el->is_open;
        }
        
        if (should_draw_menu_bar) {
            // Draw menu bar background
            glColor4f(el->color[0], el->color[1], el->color[2], el->alpha);
            glBegin(GL_QUADS);
            glVertex2i(abs_x, abs_y);
            glVertex2i(abs_x + el->calculated_width, abs_y);
            glVertex2i(abs_x + el->calculated_width, abs_y + el->calculated_height);
            glVertex2i(abs_x, abs_y + el->calculated_height);
            glEnd();
            
            // Draw menu text - center it vertically in the menu bar
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            int text_offset_x = 5;
            // Center the text vertically: start from the bottom of the menu and add centered offset
            int text_offset_y = (el->calculated_height / 2) - 9;  // Use calculated_height
            glRasterPos2i(abs_x + text_offset_x, abs_y + text_offset_y);
            for (char* c = el->label; *c != '\0'; c++) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
            }
        }
        

    } else if (strcmp(el->type, "menuitem") == 0) {
        // Draw menu item - only if it's not part of an open menu (since those are drawn in the menu section)
        int parent_menu_index = el->parent;
        
        if (!(parent_menu_index != -1 && 
              strcmp(elements[parent_menu_index].type, "menu") == 0 && 
              elements[parent_menu_index].is_open)) {
            // Draw menu item in its original position only if its parent menu is not open
            // Draw menu item
            glColor4f(el->color[0], el->color[1], el->color[2], el->alpha);
            glBegin(GL_QUADS);
            glVertex2i(abs_x, abs_y);
            glVertex2i(abs_x + el->calculated_width, abs_y);
            glVertex2i(abs_x + el->calculated_width, abs_y + el->calculated_height);
            glVertex2i(abs_x, abs_y + el->calculated_height);
            glEnd();
            
            // Draw menu item text
            glColor4f(0.0f, 0.0f, 0.0f, 1.0f); // Black text for menu items
            glRasterPos2i(abs_x + 5, abs_y + 5); // Adjusted to prevent text from being cut off at top
            for (char* c = el->label; *c != '\0'; c++) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
            }
        }
        // If the parent menu IS open, menuitems are drawn in the menu section instead
    } else if (strcmp(el->type, "dirlist") == 0) {
        // Draw directory listing background
        glColor4f(0.2f, 0.2f, 0.2f, 1.0f); // Dark gray background
        glBegin(GL_QUADS);
        glVertex2i(abs_x, abs_y);
        glVertex2i(abs_x + el->calculated_width, abs_y);
        glVertex2i(abs_x + el->calculated_width, abs_y + el->calculated_height);
        glVertex2i(abs_x, abs_y + el->calculated_height);
        glEnd();

        // Get directory contents from the model
        int entry_count = 0;
        char (*entries)[256] = model_get_dir_entries(&entry_count);

        // Draw "dir element list:" header
        glColor4f(1.0f, 1.0f, 0.0f, 1.0f); // Yellow text for header
        glRasterPos2i(abs_x + 5, abs_y + 15);
        const char* header_text = "dir element list:";
        for (const char* c = header_text; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }

        // Draw directory entries
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // White text for entries
        int line_height = 20;
        for (int i = 0; i < entry_count; i++) {
            int text_y = abs_y + 40 + (i * line_height); // Start below header
            if (text_y + line_height < abs_y + el->calculated_height) { // Make sure we don't draw outside bounds
                // TODO: Add back selection highlighting if needed
                glRasterPos2i(abs_x + 10, text_y);
                for (const char* c = entries[i]; *c != '\0'; c++) {
                    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
                }
            }
        }
    } else if (strcmp(el->type, "textfield") == 0 || strcmp(el->type, "textarea") == 0) {
        // Background
        glColor4f(0.1f, 0.1f, 0.1f, 1.0f);
        glBegin(GL_QUADS);
        glVertex2i(abs_x, abs_y);
        glVertex2i(abs_x + el->calculated_width, abs_y);
        glVertex2i(abs_x + el->calculated_width, abs_y + el->calculated_height);
        glVertex2i(abs_x, abs_y + el->calculated_height);
        glEnd();
        // Border
        glColor4f(0.8f, 0.8f, 0.8f, 1.0f); // Light grey border
        glBegin(GL_LINE_LOOP);
        glVertex2i(abs_x, abs_y);
        glVertex2i(abs_x + el->calculated_width, abs_y);
        glVertex2i(abs_x + el->calculated_width, abs_y + el->calculated_height);
        glVertex2i(abs_x, abs_y + el->calculated_height);
        glEnd();
    } else if (strcmp(el->type, "header") == 0 || strcmp(el->type, "button") == 0 || strcmp(el->type, "panel") == 0 || strcmp(el->type, "checkbox") == 0 || strcmp(el->type, "slider") == 0) {
        if (strcmp(el->type, "checkbox") == 0) {
            glColor4f(0.8f, 0.8f, 0.8f, 1.0f); // Light background for checkbox square
        } else if (strcmp(el->type, "slider") == 0) {
            glColor4f(0.3f, 0.3f, 0.3f, 1.0f); // Dark background for slider track
        } else {
            glColor4f(el->color[0], el->color[1], el->color[2], el->alpha);
        }
        glBegin(GL_QUADS);
        glVertex2i(abs_x, abs_y);
        glVertex2i(abs_x + el->calculated_width, abs_y);
        glVertex2i(abs_x + el->calculated_width, abs_y + el->calculated_height);
        glVertex2i(abs_x, abs_y + el->calculated_height);
        glEnd();

        if (strcmp(el->type, "checkbox") == 0 && el->is_checked) {
            // Draw a simple checkmark (upright V shape, adjusted for inverted Y)
            glColor4f(0.0f, 0.0f, 0.0f, 1.0f); // Black checkmark
            glLineWidth(2.0f);
            glBegin(GL_LINES);
            // First segment: bottom-left to top-middle
            glVertex2i(abs_x + el->calculated_width * 0.2, abs_y + el->calculated_height * 0.8); // Visually lower
            glVertex2i(abs_x + el->calculated_width * 0.4, abs_y + el->calculated_height * 0.2); // Visually higher
            // Second segment: top-middle to bottom-right
            glVertex2i(abs_x + el->calculated_width * 0.4, abs_y + el->calculated_height * 0.2); // Visually higher
            glVertex2i(abs_x + el->calculated_width * 0.8, abs_y + el->calculated_height * 0.8); // Visually lower
            glEnd();
            glLineWidth(1.0f);
        } else if (strcmp(el->type, "slider") == 0) {
            // Check if this is a vertical slider by looking at the aspect ratio
            int is_vertical = (el->calculated_height > el->calculated_width);  // Use calculated dimensions
            
            if (is_vertical) {
                // Draw vertical slider track (background)
                glColor4f(0.3f, 0.3f, 0.3f, 1.0f); // Dark background for slider track
                glBegin(GL_QUADS);
                glVertex2i(abs_x + el->calculated_width/2 - 3, abs_y); // Centered track
                glVertex2i(abs_x + el->calculated_width/2 + 3, abs_y);
                glVertex2i(abs_x + el->calculated_width/2 + 3, abs_y + el->calculated_height);
                glVertex2i(abs_x + el->calculated_width/2 - 3, abs_y + el->calculated_height);
                glEnd();
            
                // Draw vertical slider thumb
                float normalized_pos = (float)(el->slider_value - el->slider_min) / (el->slider_max - el->slider_min);
                float thumb_pos_y = abs_y + normalized_pos * (el->calculated_height - 10); // Use calculated_height, position based on normalized value
                glColor4f(0.8f, 0.8f, 0.8f, 1.0f); // Light grey for thumb
                glBegin(GL_QUADS);
                glVertex2i(abs_x, thumb_pos_y); // Thumb wider than track
                glVertex2i(abs_x + el->calculated_width, thumb_pos_y);
                glVertex2i(abs_x + el->calculated_width, thumb_pos_y + 10); // Thumb height of 10
                glVertex2i(abs_x, thumb_pos_y + 10);
                glEnd();

                // Draw slider value as text
                char value_str[10];
                sprintf(value_str, "%d", el->slider_value);
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                glRasterPos2i(abs_x + el->calculated_width + 5, abs_y + el->calculated_height / 2 - 5);
                for (char* c = value_str; *c != '\0'; c++) {
                    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
                }
            } else {
                // Original horizontal slider implementation
                float thumb_pos_x = abs_x + (float)(el->slider_value - el->slider_min) / (el->slider_max - el->slider_min) * (el->calculated_width - 10); // Use calculated_width, 10 is thumb width
                glColor4f(0.8f, 0.8f, 0.8f, 1.0f); // Light grey for thumb
                glBegin(GL_QUADS);
                glVertex2i(thumb_pos_x, abs_y);
                glVertex2i(thumb_pos_x + 10, abs_y);
                glVertex2i(thumb_pos_x + 10, abs_y + el->calculated_height);
                glVertex2i(thumb_pos_x, abs_y + el->calculated_height);
                glEnd();

                // Draw slider value as text
                char value_str[10];
                sprintf(value_str, "%d", el->slider_value);
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                glRasterPos2i(abs_x + el->calculated_width + 5, abs_y + el->calculated_height / 2 - 5);
                for (char* c = value_str; *c != '\0'; c++) {
                    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
                }
            }
        }
    }

    if (strcmp(el->type, "text") == 0 || strcmp(el->type, "button") == 0 || strcmp(el->type, "checkbox") == 0 || strcmp(el->type, "link") == 0) {
        // Check if the text contains emojis
        if (contains_emoji(el->label)) {
            // Use emoji-aware text rendering
            int text_offset_x = 5;
            int text_offset_y = 15;
            if (strcmp(el->type, "checkbox") == 0) {
                text_offset_x = el->calculated_width + 5; // Label next to checkbox
                text_offset_y = el->calculated_height / 2 - 5; // Center label vertically
            }
            
            float text_color[3] = {1.0f, 1.0f, 1.0f}; // Default white
            if (strcmp(el->type, "link") == 0) {
                text_color[0] = 0.0f; // Red component
                text_color[1] = 0.0f; // Green component  
                text_color[2] = 1.0f; // Blue component (blue links)
            }
            float text_pos_x = abs_x + text_offset_x;
            float text_pos_y = abs_y + text_offset_y + 15; // Use slightly adjusted Y to align with text baseline
            
            render_text_with_emojis(el->label, text_pos_x, text_pos_y, text_color); // Use original approach
        } else {
            // Use original GLUT rendering for non-emoji text
            int text_offset_x = 5;
            int text_offset_y = 15;
            if (strcmp(el->type, "checkbox") == 0) {
                text_offset_x = el->calculated_width + 5; // Label next to checkbox
                text_offset_y = el->calculated_height / 2 - 5; // Center label vertically
            }
            
            if (strcmp(el->type, "link") == 0) {
                glColor4f(0.0f, 0.0f, 1.0f, 1.0f); // Blue color for links
            } else {
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // White for other elements
            }
            
            glRasterPos2i(abs_x + text_offset_x, abs_y + text_offset_y);
            for (char* c = el->label; *c != '\0'; c++) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
            }
            
            // Draw underline for links
            if (strcmp(el->type, "link") == 0) {
                int text_width = 0;
                for (char* c = el->label; *c != '\0'; c++) {
                    text_width += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, *c);
                }
                
                // Draw underline under the text
                glColor4f(0.0f, 0.0f, 1.0f, 1.0f); // Blue underline
                glBegin(GL_LINES);
                glVertex2i(abs_x + text_offset_x, abs_y + text_offset_y - 2);
                glVertex2i(abs_x + text_offset_x + text_width, abs_y + text_offset_y - 2);
                glEnd();
            }
        }
    } else if (strcmp(el->type, "textfield") == 0 || strcmp(el->type, "textarea") == 0) {
        // Draw multi-line text, with emoji support when needed
        int line_height = 20; // Approximate height for each line
        int start_y = abs_y + 15; // Starting y position for first line
        
        for (int line = 0; line < el->num_lines; line++) {
            if (start_y + (line * line_height) >= abs_y + el->calculated_height) break; // Don't draw outside bounds
            
            if (contains_emoji(el->text_content[line])) {
                // Use emoji-aware text rendering
                float text_color[3] = {1.0f, 1.0f, 1.0f}; // White color for textfield
                float text_pos_x = abs_x + 5;
                float text_pos_y = start_y + (line * line_height) + 15; // Use slightly adjusted Y to align with text baseline
                
                render_text_with_emojis(el->text_content[line], text_pos_x, text_pos_y, text_color); // Use original approach
            } else {
                // Use original GLUT rendering for non-emoji text
                glRasterPos2i(abs_x + 5, start_y + (line * line_height));
                for (char* c = el->text_content[line]; *c != '\0'; c++) {
                    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
                }
            }
        }

        // Draw cursor if active
        if (el->is_active) {
            // Simple blinking cursor (toggle every 500ms)
            if ((glutGet(GLUT_ELAPSED_TIME) / 500) % 2) {
                int text_width = 0;
                for (int i = 0; i < el->cursor_x && i < (int)strlen(el->text_content[el->cursor_y]); i++) {
                    text_width += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, el->text_content[el->cursor_y][i]);
                }
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                glBegin(GL_QUADS);
                glVertex2i(abs_x + 5 + text_width, start_y + (el->cursor_y * line_height) - 15);
                glVertex2i(abs_x + 5 + text_width + 2, start_y + (el->cursor_y * line_height) - 15);
                glVertex2i(abs_x + 5 + text_width + 2, start_y + (el->cursor_y * line_height) + 5);
                glVertex2i(abs_x + 5 + text_width, start_y + (el->cursor_y * line_height) + 5);
                glEnd();
            }
        }
    }
}

// Function to cleanup FreeType resources


void cleanup_freetype() {
    // Clean up cached emoji textures
    for (int i = 0; i < emoji_cache_size && i < MAX_CACHED_EMOJIS; i++) {
        if (emoji_cache[i].loaded) {
            glDeleteTextures(1, &emoji_cache[i].texture_id);
        }
    }
    
    if (emoji_face) {
        FT_Done_Face(emoji_face);
        emoji_face = NULL;
    }
    if (ft) {
        FT_Done_FreeType(ft);
        ft = 0;
    }
}

void display() {
    // Find window element and set background color
    float bg_r = 0.0f, bg_g = 0.0f, bg_b = 0.0f; // Default to black
    for (int i = 0; i < num_elements; i++) {
        if (strcmp(elements[i].type, "window") == 0) {
            bg_r = elements[i].color[0];
            bg_g = elements[i].color[1];
            bg_b = elements[i].color[2];
            break;
        }
    }
    glClearColor(bg_r, bg_g, bg_b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Calculate layout for flex sizing before drawing
    calculate_layout();

    // Sort elements by z_level to ensure proper depth ordering
    // Only sort if there are elements to avoid unnecessary work
    if (num_elements > 1) {
        qsort(elements, num_elements, sizeof(UIElement), compare_elements_by_z_level);
    }

    // Draw all regular elements first (now sorted by z_level)
    for (int i = 0; i < num_elements; i++) {
        // Draw the element but don't draw submenu items here
        // (they'll be drawn separately to ensure they appear on top)
        if (strcmp(elements[i].type, "menuitem") == 0 && 
            elements[i].parent != -1 && 
            strcmp(elements[elements[i].parent].type, "menu") == 0 &&
            elements[elements[i].parent].is_open) {
            // Skip drawing menuitems that are part of an open menu here
            // They will be drawn with the menu in the next step
        } else {
            draw_element(&elements[i]);
        }
    }

    // Draw submenu items separately to ensure they appear on top of everything else
    // Note: These are not affected by z_level sorting as they always appear on top
    for (int i = 0; i < num_elements; i++) {
        if (strcmp(elements[i].type, "menu") == 0 && elements[i].is_open) {
            // Draw this open menu's submenu items with proper positioning
            // Find and draw all menuitems that belong to this menu
            int parent_menu = i;
            int parent_menu_abs_x = 0;
            int parent_menu_abs_y = 0;
            
            // Validate parent index to prevent segmentation fault
            int parent_menu_parent = elements[parent_menu].parent;
            while (parent_menu_parent != -1 && parent_menu_parent < num_elements) {
                parent_menu_abs_x += elements[parent_menu_parent].calculated_x;
                parent_menu_abs_y += elements[parent_menu_parent].calculated_y;
                parent_menu_parent = elements[parent_menu_parent].parent;
            }
            
            int submenu_x = parent_menu_abs_x + elements[parent_menu].calculated_x;
            // Calculate submenu position using the same conversion as in draw_element but positioning below the parent menu
            int parent_menu_abs_y_coord = parent_menu_abs_y + elements[parent_menu].calculated_y;
            int parent_menu_bottom_opengl = window_height - (parent_menu_abs_y_coord + elements[parent_menu].calculated_height);
            int submenu_top_opengl = parent_menu_bottom_opengl - (elements[parent_menu].menu_items_count * 20);
            
            // Draw submenu background (only if menu_items_count is valid)
            if (elements[parent_menu].menu_items_count > 0) {
                glColor4f(elements[parent_menu].color[0], elements[parent_menu].color[1], elements[parent_menu].color[2], elements[parent_menu].alpha * 0.9f); // Slightly transparent background
                glBegin(GL_QUADS);
                glVertex2i(submenu_x, submenu_top_opengl);
                glVertex2i(submenu_x + elements[parent_menu].calculated_width, submenu_top_opengl);
                glVertex2i(submenu_x + elements[parent_menu].calculated_width, parent_menu_bottom_opengl);
                glVertex2i(submenu_x, parent_menu_bottom_opengl);
                glEnd();
                
                // Draw border around submenu
                glColor4f(0.7f, 0.7f, 0.7f, 1.0f); // Gray border
                glBegin(GL_LINE_LOOP);
                glVertex2i(submenu_x, submenu_top_opengl);
                glVertex2i(submenu_x + elements[parent_menu].calculated_width, submenu_top_opengl);
                glVertex2i(submenu_x + elements[parent_menu].calculated_width, parent_menu_bottom_opengl);
                glVertex2i(submenu_x, parent_menu_bottom_opengl);
                glEnd();
            }
            
            // Draw each submenu item
            int item_position = 0;
            for (int j = 0; j < num_elements; j++) {
                if (strcmp(elements[j].type, "menuitem") == 0 && 
                    elements[j].parent == i) {
                    // Calculate absolute position for this submenu item in OpenGL coordinates
                    int abs_x = submenu_x;
                    int abs_y = submenu_top_opengl + (item_position * 20);
                    
                    // Draw menu item background
                    glColor4f(elements[j].color[0], elements[j].color[1], elements[j].color[2], elements[j].alpha);
                    glBegin(GL_QUADS);
                    glVertex2i(abs_x, abs_y);
                    glVertex2i(abs_x + elements[j].calculated_width, abs_y);
                    glVertex2i(abs_x + elements[j].calculated_width, abs_y + 20); // Fixed height of 20 for submenu items
                    glVertex2i(abs_x, abs_y + 20);
                    glEnd();
                    
                    // Draw menu item text
                    glColor4f(0.0f, 0.0f, 0.0f, 1.0f); // Black text for menu items
                    glRasterPos2i(abs_x + 5, abs_y + 5); // Lowered text position to prevent top half from being cut off
                    for (char* c = elements[j].label; *c != '\0'; c++) {
                        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
                    }
                    
                    item_position++;
                }
            }
        }
    }

    glutSwapBuffers();
}

void canvas_render_sample(int x, int y, int width, int height) {
    // Find the canvas UIElement to check its view_mode
    UIElement* current_canvas = NULL;
    for (int i = 0; i < num_elements; i++) {
        if (strcmp(elements[i].type, "canvas") == 0 && elements[i].calculated_width == width && elements[i].calculated_height == height) {
            current_canvas = &elements[i];
            break;
        }
    }

    int is_3d = (current_canvas && strcmp(current_canvas->view_mode, "3d") == 0);

    // Get the shapes from the model
    int shape_count = 0;
    Shape* shapes = model_get_shapes(&shape_count);

    if (is_3d) {
        // --- 3D Rendering Mode ---
        glEnable(GL_DEPTH_TEST);
        // Temporarily disable lighting for consistent color appearance with 2D mode
        // glEnable(GL_LIGHTING);
        // glEnable(GL_LIGHT0);

        // Setup projection matrix for 3D
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluPerspective(current_canvas->fov, (double)width / (double)height, 0.1, 100.0);

        // Setup modelview matrix for camera
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        gluLookAt(current_canvas->camera_pos[0], current_canvas->camera_pos[1], current_canvas->camera_pos[2],
                  current_canvas->camera_target[0], current_canvas->camera_target[1], current_canvas->camera_target[2],
                  current_canvas->camera_up[0], current_canvas->camera_up[1], current_canvas->camera_up[2]);

        // Render shapes in 3D
        for (int i = 0; i < shape_count; i++) {
            Shape* s = &shapes[i];
            if (strcmp(s->type, "SQUARE") == 0 || strcmp(s->type, "RECT") == 0) {
                glPushMatrix();
                // Map the 2D coordinates from the module to the XZ plane in 3D
                float x_3d = (s->x / (float)width) * 10.0f - 5.0f;
                float z_3d = (s->y / (float)height) * 10.0f - 5.0f;
                glTranslatef(x_3d, 0.5f, z_3d);
                glColor4f(s->color[0], s->color[1], s->color[2], s->alpha);
                // Use width and height for 3D scaling
                float size_x = s->width / 10.0f;
                float size_y = (s->height) / 10.0f;  // Height for the cube
                float size_z = s->depth / 10.0f;
                
                // Draw a rectangular prism
                glBegin(GL_QUADS);
                // Front face
                glVertex3f(-size_x/2, -size_y/2, size_z/2);
                glVertex3f(size_x/2, -size_y/2, size_z/2);
                glVertex3f(size_x/2, size_y/2, size_z/2);
                glVertex3f(-size_x/2, size_y/2, size_z/2);
                
                // Back face
                glVertex3f(-size_x/2, -size_y/2, -size_z/2);
                glVertex3f(-size_x/2, size_y/2, -size_z/2);
                glVertex3f(size_x/2, size_y/2, -size_z/2);
                glVertex3f(size_x/2, -size_y/2, -size_z/2);
                
                // Top face
                glVertex3f(-size_x/2, size_y/2, -size_z/2);
                glVertex3f(-size_x/2, size_y/2, size_z/2);
                glVertex3f(size_x/2, size_y/2, size_z/2);
                glVertex3f(size_x/2, size_y/2, -size_z/2);
                
                // Bottom face
                glVertex3f(-size_x/2, -size_y/2, -size_z/2);
                glVertex3f(size_x/2, -size_y/2, -size_z/2);
                glVertex3f(size_x/2, -size_y/2, size_z/2);
                glVertex3f(-size_x/2, -size_y/2, size_z/2);
                
                // Left face
                glVertex3f(-size_x/2, -size_y/2, -size_z/2);
                glVertex3f(-size_x/2, -size_y/2, size_z/2);
                glVertex3f(-size_x/2, size_y/2, size_z/2);
                glVertex3f(-size_x/2, size_y/2, -size_z/2);
                
                // Right face
                glVertex3f(size_x/2, -size_y/2, -size_z/2);
                glVertex3f(size_x/2, size_y/2, -size_z/2);
                glVertex3f(size_x/2, size_y/2, size_z/2);
                glVertex3f(size_x/2, -size_y/2, size_z/2);
                glEnd();
                glPopMatrix();
            } else if (strcmp(s->type, "TRIANGLE") == 0) {
                glPushMatrix();
                // Map the 2D coordinates from the module to the XZ plane in 3D
                float x_3d = (s->x / (float)width) * 10.0f - 5.0f;
                float z_3d = (s->y / (float)height) * 10.0f - 5.0f;
                glTranslatef(x_3d, 0.5f, z_3d);
                glColor4f(s->color[0], s->color[1], s->color[2], s->alpha);
                // Use width and height for 3D scaling
                float avg_size = (s->width + s->height) / 2.0f;
                
                // Draw a triangle pyramid
                glBegin(GL_TRIANGLES);
                // Front face
                glVertex3f(0.0f, avg_size/10.0f, 0.0f);  // Top
                glVertex3f(-avg_size/20.0f, -avg_size/20.0f, avg_size/20.0f);  // Bottom left
                glVertex3f(avg_size/20.0f, -avg_size/20.0f, avg_size/20.0f);   // Bottom right
                
                // Right face
                glVertex3f(0.0f, avg_size/10.0f, 0.0f);  // Top
                glVertex3f(avg_size/20.0f, -avg_size/20.0f, avg_size/20.0f);   // Bottom right
                glVertex3f(0.0f, -avg_size/20.0f, -avg_size/20.0f);  // Bottom back
                
                // Left face
                glVertex3f(0.0f, avg_size/10.0f, 0.0f);  // Top
                glVertex3f(0.0f, -avg_size/20.0f, -avg_size/20.0f);  // Bottom back
                glVertex3f(-avg_size/20.0f, -avg_size/20.0f, avg_size/20.0f);  // Bottom left
                glEnd();
                glPopMatrix();
            }
        }

        // Render shapes in 3D
        for (int i = 0; i < shape_count; i++) {
            Shape* s = &shapes[i];
            if (strcmp(s->type, "SQUARE") == 0 || strcmp(s->type, "RECT") == 0) {
                glPushMatrix();
                // Map the 2D coordinates from the module to the XZ plane in 3D
                float x_3d = (s->x / (float)width) * 10.0f - 5.0f;
                float z_3d = (s->y / (float)height) * 10.0f - 5.0f;
                glTranslatef(x_3d, 0.5f, z_3d);
                glColor4f(s->color[0], s->color[1], s->color[2], s->alpha);
                // Use width and height for 3D scaling
                float size_x = s->width / 10.0f;
                float size_y = (s->height) / 10.0f;  // Height for the cube
                float size_z = s->depth / 10.0f;
                
                // Draw a rectangular prism
                glBegin(GL_QUADS);
                // Front face
                glVertex3f(-size_x/2, -size_y/2, size_z/2);
                glVertex3f(size_x/2, -size_y/2, size_z/2);
                glVertex3f(size_x/2, size_y/2, size_z/2);
                glVertex3f(-size_x/2, size_y/2, size_z/2);
                
                // Back face
                glVertex3f(-size_x/2, -size_y/2, -size_z/2);
                glVertex3f(-size_x/2, size_y/2, -size_z/2);
                glVertex3f(size_x/2, size_y/2, -size_z/2);
                glVertex3f(size_x/2, -size_y/2, -size_z/2);
                
                // Top face
                glVertex3f(-size_x/2, size_y/2, -size_z/2);
                glVertex3f(-size_x/2, size_y/2, size_z/2);
                glVertex3f(size_x/2, size_y/2, size_z/2);
                glVertex3f(size_x/2, size_y/2, -size_z/2);
                
                // Bottom face
                glVertex3f(-size_x/2, -size_y/2, -size_z/2);
                glVertex3f(size_x/2, -size_y/2, -size_z/2);
                glVertex3f(size_x/2, -size_y/2, size_z/2);
                glVertex3f(-size_x/2, -size_y/2, size_z/2);
                
                // Left face
                glVertex3f(-size_x/2, -size_y/2, -size_z/2);
                glVertex3f(-size_x/2, -size_y/2, size_z/2);
                glVertex3f(-size_x/2, size_y/2, size_z/2);
                glVertex3f(-size_x/2, size_y/2, -size_z/2);
                
                // Right face
                glVertex3f(size_x/2, -size_y/2, -size_z/2);
                glVertex3f(size_x/2, size_y/2, -size_z/2);
                glVertex3f(size_x/2, size_y/2, size_z/2);
                glVertex3f(size_x/2, -size_y/2, size_z/2);
                glEnd();
                glPopMatrix();
            } else if (strcmp(s->type, "TRIANGLE") == 0) {
                glPushMatrix();
                // Map the 2D coordinates from the module to the XZ plane in 3D
                float x_3d = (s->x / (float)width) * 10.0f - 5.0f;
                float z_3d = (s->y / (float)height) * 10.0f - 5.0f;
                glTranslatef(x_3d, 0.5f, z_3d);
                glColor4f(s->color[0], s->color[1], s->color[2], s->alpha);
                // Use width and height for 3D scaling
                float avg_size = (s->width + s->height) / 2.0f;
                
                // Draw a triangle pyramid
                glBegin(GL_TRIANGLES);
                // Front face
                glVertex3f(0.0f, avg_size/10.0f, 0.0f);  // Top
                glVertex3f(-avg_size/20.0f, -avg_size/20.0f, avg_size/20.0f);  // Bottom left
                glVertex3f(avg_size/20.0f, -avg_size/20.0f, avg_size/20.0f);   // Bottom right
                
                // Right face
                glVertex3f(0.0f, avg_size/10.0f, 0.0f);  // Top
                glVertex3f(avg_size/20.0f, -avg_size/20.0f, avg_size/20.0f);   // Bottom right
                glVertex3f(0.0f, -avg_size/20.0f, -avg_size/20.0f);  // Bottom back
                
                // Left face
                glVertex3f(0.0f, avg_size/10.0f, 0.0f);  // Top
                glVertex3f(0.0f, -avg_size/20.0f, -avg_size/20.0f);  // Bottom back
                glVertex3f(-avg_size/20.0f, -avg_size/20.0f, avg_size/20.0f);  // Bottom left
                glEnd();
                glPopMatrix();
            }
        }

        // Restore matrices and state
        glPopMatrix(); // Pop modelview
        glMatrixMode(GL_PROJECTION);
        glPopMatrix(); // Pop projection
        glMatrixMode(GL_MODELVIEW);
        glDisable(GL_DEPTH_TEST);
        // glDisable(GL_LIGHTING);  // Not needed since we didn't enable it

    } else {
        // --- 2D Rendering Mode ---
        // Draw a simple background for the canvas
        glColor4f(0.8f, 0.9f, 1.0f, current_canvas->alpha); // Light blue background
        glBegin(GL_QUADS);
        glVertex2i(0, 0);  // The drawing is translated to the canvas's origin
        glVertex2i(width, 0);
        glVertex2i(width, height);
        glVertex2i(0, height);
        glEnd();

        // Render each shape in 2D
        for (int i = 0; i < shape_count; i++) {
            Shape* s = &shapes[i];
            glColor4f(s->color[0], s->color[1], s->color[2], s->alpha);

            if (strcmp(s->type, "SQUARE") == 0 || strcmp(s->type, "RECT") == 0) {
                glBegin(GL_QUADS);
                glVertex2f(s->x - s->width / 2, s->y - s->height / 2);
                glVertex2f(s->x + s->width / 2, s->y - s->height / 2);
                glVertex2f(s->x + s->width / 2, s->y + s->height / 2);
                glVertex2f(s->x - s->width / 2, s->y + s->height / 2);
                glEnd();
            } else if (strcmp(s->type, "CIRCLE") == 0) {
                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(s->x, s->y);
                for (int j = 0; j <= 20; j++) {
                    float angle = j * (2.0f * 3.14159f / 20);
                    float dx = s->width / 2 * cos(angle);
                    float dy = s->height / 2 * sin(angle);
                    glVertex2f(s->x + dx, s->y + dy);
                }
                glEnd();
            } else if (strcmp(s->type, "TRIANGLE") == 0) {
                glBegin(GL_TRIANGLES);
                // Calculate the three vertices of the triangle
                // For an equilateral triangle centered at (x, y)
                float half_width = s->width / 2.0f;
                float half_height = s->height / 2.0f;
                
                // Vertex 1: Top vertex
                glVertex2f(s->x, s->y + half_height);
                // Vertex 2: Bottom left
                glVertex2f(s->x - half_width, s->y - half_height);
                // Vertex 3: Bottom right
                glVertex2f(s->x + half_width, s->y - half_height);
                glEnd();
            }
        }
    }
}

void reshape(int w, int h) {
    window_width = w;
    window_height = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// Getter functions for modules
ModulePath* get_modules(int* count) {
    *count = num_parsed_modules;
    return parsed_modules;
}

// Function to clean up the view state for reloading
// Function to get window properties
void get_window_properties(int* x, int* y, int* width, int* height, char* title, int title_size) {
    *x = 100;      // Default values
    *y = 100;
    *width = 800;
    *height = 600;
    strncpy(title, "C-HTML Prototype", title_size - 1);
    title[title_size - 1] = '\0';
    
    // Find window element and get its properties
    for (int i = 0; i < num_elements; i++) {
        if (strcmp(elements[i].type, "window") == 0) {
            if (elements[i].x != 0) *x = elements[i].x;
            if (elements[i].y != 0) *y = elements[i].y;
            if (elements[i].calculated_width != 0) *width = elements[i].calculated_width;
            if (elements[i].calculated_height != 0) *height = elements[i].calculated_height;
            if (strlen(elements[i].label) > 0) {
                strncpy(title, elements[i].label, title_size - 1);
                title[title_size - 1] = '\0';
            }
            break;
        }
    }
}

void cleanup_view() {
    // Reset elements array
    for (int i = 0; i < MAX_ELEMENTS; i++) {
        memset(&elements[i], 0, sizeof(UIElement));
    }
    num_elements = 0;
    
    // Reset module paths
    for (int i = 0; i < MAX_MODULES; i++) {
        memset(&parsed_modules[i], 0, sizeof(ModulePath));
    }
    num_parsed_modules = 0;
    
    // Reset other state variables as needed
    // For example, reset text content arrays, etc.
    printf("View cleaned up for reload\n");
}

// Function to clear all cached pages
void clear_page_cache() {
    for (int i = 0; i < MAX_PAGE_HISTORY; i++) {
        content_manager.pages[i].loaded = 0;
        content_manager.pages[i].filename[0] = '\0';
        content_manager.pages[i].num_elements = 0;
        content_manager.pages[i].num_modules = 0;
    }
    content_manager.current_page_index = -1;
    content_manager.num_pages_loaded = 0;
    content_manager.current_file_path[0] = '\0';
    printf("Page cache cleared\n");
}
