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
#include <linux/videodev2.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define WIDTH 640
#define HEIGHT 480
#define RGB_SIZE (WIDTH * HEIGHT * 3)

static char *dev_name = "/dev/video0";
static int fd = -1;
static void *buffer_start;
static unsigned int buffer_length;
unsigned char rgb_data[RGB_SIZE];

static void errno_exit(const char *s) {
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

static int xioctl(int fh, int request, void *arg) {
    int r;
    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);
    return r;
}

static void yuyv_to_rgb(unsigned char *yuyv, unsigned char *rgb, int size) {
    int i, j;
    int max_pixels = RGB_SIZE / 3;
    for (i = 0, j = 0; i < size - 3 && j < RGB_SIZE - 5; i += 4, j += 6) {
        int y0 = yuyv[i + 0];
        int u  = yuyv[i + 1] - 128;
        int y1 = yuyv[i + 2];
        int v  = yuyv[i + 3] - 128;

        int r = (298 * y0 + 409 * v + 128) >> 8;
        int g = (298 * y0 - 100 * u - 208 * v + 128) >> 8;
        int b = (298 * y0 + 516 * u + 128) >> 8;
        rgb[j + 0] = (r < 0) ? 0 : (r > 255) ? 255 : r;
        rgb[j + 1] = (g < 0) ? 0 : (g > 255) ? 255 : g;
        rgb[j + 2] = (b < 0) ? 0 : (b > 255) ? 255 : b;

        r = (298 * y1 + 409 * v + 128) >> 8;
        g = (298 * y1 - 100 * u - 208 * v + 128) >> 8;
        b = (298 * y1 + 516 * u + 128) >> 8;
        rgb[j + 3] = (r < 0) ? 0 : (r > 255) ? 255 : r;
        rgb[j + 4] = (g < 0) ? 0 : (g > 255) ? 255 : g;
        rgb[j + 5] = (b < 0) ? 0 : (b > 255) ? 255 : b;
    }
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

static void init_device(void) {
    struct v4l2_capability cap;
    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        errno_exit("VIDIOC_QUERYCAP");
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is no video capture device\n", dev_name);
        exit(EXIT_FAILURE);
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "%s does not support streaming i/o\n", dev_name);
        exit(EXIT_FAILURE);
    }

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;

    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt)) {
        errno_exit("VIDIOC_S_FMT");
    }

    if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt)) {
        errno_exit("VIDIOC_G_FMT");
    }

    printf("Using format: %u, size: %ux%u, bytes: %u\n", 
           fmt.fmt.pix.pixelformat, fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.sizeimage);
    buffer_length = fmt.fmt.pix.sizeimage;

    init_mmap();
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

    yuyv_to_rgb(buffer_start, rgb_data, buf.bytesused);

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
        errno_exit("VIDIOC_QBUF");
    }

    return 1;
}

static void save_png(void) {
    static int counter = 0;
    char filename[32];
    sprintf(filename, "capture_%03d.png", counter++);
    stbi_write_png(filename, WIDTH, HEIGHT, 3, rgb_data, WIDTH * 3);
    printf("Saved %s\n", filename);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRasterPos2i(0, HEIGHT);
    glPixelZoom(1.0, -1.0);
    glDrawPixels(WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, rgb_data);
    glPixelZoom(1.0, 1.0);
    glutSwapBuffers();
}

void idle() {
    fd_set fds;
    struct timeval tv = {0, 10000}; // 10ms timeout for smoother updates
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    FD_SET(STDIN_FILENO, &fds);

    int r = select(fd + 1, &fds, NULL, NULL, &tv);
    if (-1 == r && EINTR != errno) errno_exit("select");

    if (FD_ISSET(fd, &fds) && read_frame()) glutPostRedisplay();

    if (FD_ISSET(STDIN_FILENO, &fds)) {
        char c;
        if (read(STDIN_FILENO, &c, 1) > 0) {
            if (c == 'p' || c == 'P') save_png();
            if (c == 27 || c == 'q' || c == 'Q') exit(0); // ESC or q to exit
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
    if (key == 27) exit(0); // ESC to exit
}

void init_gl() {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    memset(rgb_data, 0, RGB_SIZE);
}

void init_terminal(void) {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO); // Non-canonical, no echo
    term.c_cc[VMIN] = 0;
    term.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

int main(int argc, char **argv) {
    fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);
    if (-1 == fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    init_device();
    start_capturing();
    init_terminal();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Webcam Renderer");

    init_gl();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(idle);
    glutMainLoop();

    close(fd);
    munmap(buffer_start, buffer_length);
    return 0;
}
