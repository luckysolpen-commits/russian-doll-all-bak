#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <signal.h>
#include <linux/videodev2.h>
#include <math.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

 unsigned char target_r = 0;   // Green screen color
        unsigned char target_g = 70;
        unsigned char target_b = 70;
        int tolerance = 30; // Tolerance for color matching

// Function prototype
static void cleanup(void);

// Define WIDTH and HEIGHT as non-const
static int WIDTH = 640;
static int HEIGHT = 480;
static int RGB_SIZE; // Initialized in main (RGBA)
static int green_screen_enabled = 1; // Green screen toggle, enabled by default

static char *dev_names[] = {"/dev/video0", "/dev/video4"}; // Updated for USB camera
static int current_dev_index = 0; // Track current device
static int fd = -1;
static void *buffer_start = NULL;
static unsigned int buffer_length;
static unsigned char *rgb_data = NULL; // Dynamic allocation (RGBA)
static struct termios old_termios;
static unsigned int current_pixelformat = V4L2_PIX_FMT_YUYV;

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

// Function to check if a pixel matches the target color within tolerance
static int color_matches(unsigned char r, unsigned char g, unsigned char b,
                        unsigned char target_r, unsigned char target_g, unsigned char target_b,
                        int tolerance) {
    return abs(r - target_r) <= tolerance &&
           abs(g - target_g) <= tolerance &&
           abs(b - target_b) <= tolerance;
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
    cleanup();
    exit(EXIT_FAILURE);
}

static void cleanup(void) {
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
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios); // Restore terminal
}

static void signal_handler(int sig) {
    cleanup();
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

static void mjpg_to_rgb(unsigned char *mjpg, int size, unsigned char *rgb) {
    int width, height, comp;
    unsigned char *data = stbi_load_from_memory(mjpg, size, &width, &height, &comp, 3);
    if (!data || width != WIDTH || height != HEIGHT || comp != 3) {
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
        fprintf(stderr, "Failed to set format %s at %dx%d on %s\n", 
                pixelformat_to_string(pixelformat), width, height, dev_names[current_dev_index]);
        return 0;
    }

    if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt)) {
        fprintf(stderr, "VIDIOC_G_FMT error on %s\n", dev_names[current_dev_index]);
        return 0;
    }

    // Update WIDTH, HEIGHT, and RGB_SIZE if device adjusts resolution
    if (fmt.fmt.pix.width != width || fmt.fmt.pix.height != height) {
        fprintf(stderr, "Format adjusted to %ux%u on %s\n", 
                fmt.fmt.pix.width, fmt.fmt.pix.height, dev_names[current_dev_index]);
        WIDTH = fmt.fmt.pix.width;
        HEIGHT = fmt.fmt.pix.height;
        RGB_SIZE = WIDTH * HEIGHT * 4; // RGBA
        // Reallocate rgb_data
        unsigned char *new_rgb_data = realloc(rgb_data, RGB_SIZE);
        if (!new_rgb_data) {
            fprintf(stderr, "Failed to reallocate rgb_data\n");
            return 0;
        }
        rgb_data = new_rgb_data;
        memset(rgb_data, 0, RGB_SIZE); // Clear buffer to prevent residual data
    }

    printf("Using format: %s, size: %ux%u, bytes: %u on %s\n", 
           pixelformat_to_string(fmt.fmt.pix.pixelformat), 
           fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.sizeimage, dev_names[current_dev_index]);
    buffer_length = fmt.fmt.pix.sizeimage;
    current_pixelformat = fmt.fmt.pix.pixelformat;
    return 1;
}

static void init_device(void) {
    struct v4l2_capability cap;
    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        errno_exit("VIDIOC_QUERYCAP");
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is no video capture device\n", dev_names[current_dev_index]);
        cleanup();
        exit(EXIT_FAILURE);
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "%s does not support streaming i/o\n", dev_names[current_dev_index]);
        cleanup();
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

    fprintf(stderr, "No supported format/resolution found for %s\n", dev_names[current_dev_index]);
    cleanup();
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

static void switch_device(void) {
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

    current_dev_index = (current_dev_index + 1) % 2;
    printf("Switching to %s\n", dev_names[current_dev_index]);

    fd = open(dev_names[current_dev_index], O_RDWR | O_NONBLOCK, 0);
    if (-1 == fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_names[current_dev_index], errno, strerror(errno));
        cleanup();
        exit(EXIT_FAILURE);
    }

    init_device();
    start_capturing();
    // Resize window after device initialization
    glutReshapeWindow(WIDTH, HEIGHT);
}

static int read_frame(void) {
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
        yuv420_to_rgb(buffer_start, rgb_data, WIDTH, HEIGHT);
    } else {
        fprintf(stderr, "Unsupported pixel format: %s\n", pixelformat_to_string(current_pixelformat));
        cleanup();
        return 0;
    }

    // Apply green screen effect if enabled
    if (green_screen_enabled) {
       
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                int idx = (y * WIDTH + x) * 4; // RGBA
                unsigned char r = rgb_data[idx + 0];
                unsigned char g = rgb_data[idx + 1];
                unsigned char b = rgb_data[idx + 2];
                if (color_matches(r, g, b, target_r, target_g, target_b, tolerance)) {
                    rgb_data[idx + 0] = 0;
                    rgb_data[idx + 1] = 0;
                    rgb_data[idx + 2] = 0;
                    rgb_data[idx + 3] = 0; // Transparent
                }
            }
        }
    }

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
        errno_exit("VIDIOC_QBUF");
    }

    return 1;
}

static void save_png(void) {
    static int counter = 0;
    char filename[64];
    if (green_screen_enabled) {
        sprintf(filename, "capture_%03d_transparent.png", counter++);
        printf("Saving %s with resolution %dx%d, RGBA, buffer size %d bytes\n", filename, WIDTH, HEIGHT, RGB_SIZE);
        stbi_write_png(filename, WIDTH, HEIGHT, 4, rgb_data, WIDTH * 4); // Save as RGBA
    } else {
        sprintf(filename, "capture_%03d.png", counter++);
        // Extract RGB data
        unsigned char *rgb_only = malloc(WIDTH * HEIGHT * 3);
        if (!rgb_only) {
            fprintf(stderr, "Failed to allocate RGB buffer for PNG\n");
            return;
        }
        for (int i = 0, j = 0; i < RGB_SIZE; i += 4, j += 3) {
            rgb_only[j + 0] = rgb_data[i + 0];
            rgb_only[j + 1] = rgb_data[i + 1];
            rgb_only[j + 2] = rgb_data[i + 2];
        }
        printf("Saving %s with resolution %dx%d, RGB, buffer size %d bytes\n", filename, WIDTH, HEIGHT, WIDTH * HEIGHT * 3);
        stbi_write_png(filename, WIDTH, HEIGHT, 3, rgb_only, WIDTH * 3); // Save as RGB
        free(rgb_only);
    }
    printf("Saved %s\n", filename);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRasterPos2i(0, HEIGHT);
    glPixelZoom(1.0, -1.0);
    glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, rgb_data); // RGBA
    glPixelZoom(1.0, 1.0);
    glutSwapBuffers();
}

void idle() {
    fd_set fds;
    struct timeval tv = {0, 10000}; // 10ms timeout
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    FD_SET(STDIN_FILENO, &fds);

    int r = select(fd + 1, &fds, NULL, NULL, &tv);
    if (-1 == r && EINTR != errno) {
        fprintf(stderr, "select error %d, %s\n", errno, strerror(errno));
        cleanup();
    }

    if (FD_ISSET(fd, &fds) && read_frame()) glutPostRedisplay();

    if (FD_ISSET(STDIN_FILENO, &fds)) {
        char c;
        if (read(STDIN_FILENO, &c, 1) > 0) {
            if (c == 'p' || c == 'P') save_png();
            if (c == 'c' || c == 'C') switch_device();
            if (c == 'g' || c == 'G') {
                green_screen_enabled = !green_screen_enabled;
                printf("Green screen %s\n", green_screen_enabled ? "enabled" : "disabled");
            }
            if (c == 27 || c == 'q' || c == 'Q') cleanup();
        }
    }
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 'p' || key == 'P') save_png();
    if (key == 'c' || key == 'C') switch_device();
    if (key == 'g' || key == 'G') {
        green_screen_enabled = !green_screen_enabled;
        printf("Green screen %s\n", green_screen_enabled ? "enabled" : "disabled");
    }
    if (key == 27) cleanup();
}

void init_gl() {
    glClearColor(0.5, 0.5, 0.5, 1.0); // Gray background for transparency visibility
    glEnable(GL_BLEND); // Enable alpha blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    rgb_data = malloc(RGB_SIZE);
    if (!rgb_data) {
        fprintf(stderr, "Failed to allocate rgb_data\n");
        exit(EXIT_FAILURE);
    }
    memset(rgb_data, 0, RGB_SIZE); // Initialize to zero
}

void init_terminal() {
    tcgetattr(STDIN_FILENO, &old_termios);
    struct termios term = old_termios;
    term.c_lflag &= ~(ICANON | ECHO); // Non-canonical, no echo
    term.c_cc[VMIN] = 0;
    term.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

int main(int argc, char **argv) {
    // Initialize RGB_SIZE (RGBA)
    RGB_SIZE = WIDTH * HEIGHT * 4;

    // Print key instructions on startup
    printf("Webcam Renderer with Green Screen Controls:\n");
    printf("  p/P: Save a PNG capture (RGB or RGBA based on green screen toggle)\n");
    printf("  c/C: Switch between %s and %s\n", dev_names[0], dev_names[1]);
    printf("  g/G: Toggle green screen effect (green: RGB 0, 255, 0, ±20 tolerance)\n");
    printf("  q/Q or Esc: Quit\n");
    printf("  Ctrl+C: Terminate\n");
    printf("  Note: Green screen is enabled by default\n");
    printf("\n");

    atexit(cleanup); // Ensure cleanup on exit
    signal(SIGINT, signal_handler); // Handle Ctrl+C
    fd = open(dev_names[current_dev_index], O_RDWR | O_NONBLOCK, 0);
    if (-1 == fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_names[current_dev_index], errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    init_device();

    // Initialize GLUT after device setup to get correct WIDTH and HEIGHT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA); // Enable RGBA
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Webcam Renderer with Green Screen");
    glutReshapeWindow(WIDTH, HEIGHT); // Ensure window matches initial resolution

    start_capturing();
    init_terminal();
    init_gl();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(idle);
    glutMainLoop();

    return 0;
}
