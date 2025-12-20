#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>

// Load stb_image implementation
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Define function pointers for modern GL functions not in standard headers
typedef void (APIENTRYP PFNGLGENTEXTURESPROC) (GLsizei n, GLuint *textures);
typedef void (APIENTRYP PFNGLBINDTEXTUREPROC) (GLenum target, GLuint texture);
typedef void (APIENTRYP PFNGLTEXIMAGE2DPROC) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);

PFNGLGENTEXTURESPROC my_glGenTextures;
PFNGLBINDTEXTUREPROC my_glBindTexture;
PFNGLTEXIMAGE2DPROC  my_glTexImage2D;

void load_pointers() {
    // Manually fetch addresses from the driver
    my_glGenTextures = (PFNGLGENTEXTURESPROC)glXGetProcAddress((const GLubyte*)"glGenTextures");
    my_glBindTexture = (PFNGLBINDTEXTUREPROC)glXGetProcAddress((const GLubyte*)"glBindTexture");
    my_glTexImage2D  = (PFNGLTEXIMAGE2DPROC)glXGetProcAddress((const GLubyte*)"glTexImage2D");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <image_path>\n", argv[0]);
        return 1;
    }

    // 1. Setup X11 and OpenGL Context
    Display *display = XOpenDisplay(NULL);
    if (!display) { printf("Cannot open display\n"); return 1; }

    XVisualInfo *vi = glXChooseVisual(display, DefaultScreen(display), (int[]){GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None});
    GLXContext context = glXCreateContext(display, vi, NULL, GL_TRUE);
    Window win = XCreateSimpleWindow(display, RootWindow(display, vi->screen), 0, 0, 800, 600, 0, 0, 0);
    glXMakeCurrent(display, win, context);

    // 2. Load Function Pointers
    load_pointers();

    // 3. Load Image File
    int width, height, channels;
    unsigned char *data = stbi_load(argv[1], &width, &height, &channels, 4); // Force RGBA
    if (!data) {
        printf("Failed to load image: %s\n", argv[1]);
        return 1;
    }
    printf("Loaded image %s (%dx%d)\n", argv[1], width, height);

    // 4. Create OpenGL Texture
    GLuint textureID;
    my_glGenTextures(1, &textureID);
    my_glBindTexture(GL_TEXTURE_2D, textureID);

    // Set filtering parameters (required to see the texture)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload raw pixel data to GPU
    my_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    printf("Texture generated successfully with ID: %u\n", textureID);

    // Enable texturing and set up the texture
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Main Loop
    XMapWindow(display, win);
    while(1) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Draw a quad with the texture mapped onto it (corrected for proper image orientation)
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f);  // Bottom-left of image -> Bottom-left of screen
            glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f, -1.0f);  // Bottom-right of image -> Bottom-right of screen
            glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f,  1.0f);  // Top-right of image -> Top-right of screen
            glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f,  1.0f);  // Top-left of image -> Top-left of screen
        glEnd();
        
        glXSwapBuffers(display, win);
    }

    return 0;
}

