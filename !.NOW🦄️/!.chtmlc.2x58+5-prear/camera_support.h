#ifndef CAMERA_SUPPORT_H
#define CAMERA_SUPPORT_H

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

// Camera-related structures and functions based on the example camera code
static int camera_enabled = 0;
static int cam_ar_mode = 0; // 0=off, 1=webcam, 2=USB cam
static char *dev_names[] = {"/dev/video0", "/dev/video4"}; // Webcam and USB cam
static int current_dev_index = 0;
static int fd = -1;
static void *buffer_start = NULL;
static unsigned int buffer_length = 0;
static unsigned char *rgb_data = NULL;
static int WIDTH = 640;
static int HEIGHT = 480;
static int RGB_SIZE; // RGBA
static struct termios old_termios;
static unsigned int current_pixelformat = V4L2_PIX_FMT_YUYV;

// Function prototypes for camera operations
int init_camera(int ar_mode);
void cleanup_camera(void);
int read_frame(void);
void draw_camera_background(int x, int y, int width, int height);
static void init_mmap(void);
static void init_device(void);
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

#include "stb_image.h"

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
        fprintf(stderr, "Failed to set format %s at %dx%d\n", 
                pixelformat_to_string(pixelformat), width, height);
        return 0;
    }

    if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt)) {
        fprintf(stderr, "VIDIOC_G_FMT error\n");
        return 0;
    }

    // Update WIDTH, HEIGHT, and RGB_SIZE if device adjusts resolution
    if (fmt.fmt.pix.width != width || fmt.fmt.pix.height != height) {
        fprintf(stderr, "Format adjusted to %ux%u\n", 
                fmt.fmt.pix.width, fmt.fmt.pix.height);
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

    printf("Using format: %s, size: %ux%u, bytes: %u\n", 
           pixelformat_to_string(fmt.fmt.pix.pixelformat), 
           fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.sizeimage);
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

int init_camera(int ar_mode) {
    if (camera_enabled) {
        // Already initialized, just return
        return 1;
    }

    if (ar_mode != 1 && ar_mode != 2) {
        // Invalid AR mode
        return 0;
    }

    cam_ar_mode = ar_mode;
    current_dev_index = ar_mode - 1; // 1 -> 0 (webcam), 2 -> 1 (USB cam)
    
    // Initialize RGB_SIZE (RGBA)
    RGB_SIZE = WIDTH * HEIGHT * 4;

    atexit(cleanup_camera); // Ensure cleanup on exit
    signal(SIGINT, signal_handler); // Handle Ctrl+C
    
    fd = open(dev_names[current_dev_index], O_RDWR | O_NONBLOCK, 0);
    if (-1 == fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_names[current_dev_index], errno, strerror(errno));
        return 0;
    }

    init_device();
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
        yuv420_to_rgb(buffer_start, rgb_data, WIDTH, HEIGHT);
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

void draw_camera_background(int x, int y, int width, int height) {
    if (!camera_enabled) return;

    // Update camera frame if needed
    read_frame();

    // Save current matrix
    glPushMatrix();
    
    // Scale the camera texture to fit the canvas dimensions
    glScalef((float)width / WIDTH, (float)height / HEIGHT, 1.0f);
    
    // Draw the camera texture
    glRasterPos2i(x, y + height);  // Position at top-left
    glPixelZoom(1.0, -1.0);       // Flip Y-axis
    glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, rgb_data);
    glPixelZoom(1.0, 1.0);        // Restore zoom
    
    // Restore matrix
    glPopMatrix();
}

#endif // CAMERA_SUPPORT_H