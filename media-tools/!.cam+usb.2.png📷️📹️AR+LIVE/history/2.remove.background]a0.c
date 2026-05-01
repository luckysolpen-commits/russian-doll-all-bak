#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

    unsigned char target_r = 0;
    unsigned char target_g = 70;
    unsigned char target_b = 70;
    int tolerance = 80; // Tolerance for color matching (±20 per channel)


// Function to check if a pixel matches the target color within tolerance
static int color_matches(unsigned char r, unsigned char g, unsigned char b,
                        unsigned char target_r, unsigned char target_g, unsigned char target_b,
                        int tolerance) {
    return abs(r - target_r) <= tolerance &&
           abs(g - target_g) <= tolerance &&
           abs(b - target_b) <= tolerance;
}

int main(int argc, char *argv[]) {
    // Check command-line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input.png>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Load input PNG
    int width, height, channels;
    unsigned char *img = stbi_load(argv[1], &width, &height, &channels, 0);
    if (!img) {
        fprintf(stderr, "Failed to load image: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    // Ensure image has 3 or 4 channels
    if (channels != 3 && channels != 4) {
        fprintf(stderr, "Image must be RGB or RGBA, got %d channels\n", channels);
        stbi_image_free(img);
        return EXIT_FAILURE;
    }

    // Allocate output buffer for RGBA (4 channels)
    int out_channels = 4;
    size_t out_size = width * height * out_channels;
    unsigned char *out_img = malloc(out_size);
    if (!out_img) {
        fprintf(stderr, "Failed to allocate output buffer\n");
        stbi_image_free(img);
        return EXIT_FAILURE;
    }

    // Define target color (green screen: RGB 0, 255, 0) and tolerance

    // Process pixels
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * channels;
            int out_idx = (y * width + x) * out_channels;

            unsigned char r = img[idx + 0];
            unsigned char g = img[idx + 1];
            unsigned char b = img[idx + 2];

            // Check if pixel matches target color
            if (color_matches(r, g, b, target_r, target_g, target_b, tolerance)) {
                // Set to transparent (RGBA: 0, 0, 0, 0)
                out_img[out_idx + 0] = 0;
                out_img[out_idx + 1] = 0;
                out_img[out_idx + 2] = 0;
                out_img[out_idx + 3] = 0; // Alpha = 0 (transparent)
            } else {
                // Copy pixel and set opaque
                out_img[out_idx + 0] = r;
                out_img[out_idx + 1] = g;
                out_img[out_idx + 2] = b;
                out_img[out_idx + 3] = 255; // Alpha = 255 (opaque)
            }
        }
    }

    // Generate output filename (append "_transparent.png")
    char out_filename[256];
    strncpy(out_filename, argv[1], sizeof(out_filename) - 14);
    out_filename[sizeof(out_filename) - 14] = '\0'; // Ensure null termination
    char *ext = strrchr(out_filename, '.');
    if (ext) *ext = '\0'; // Remove .png extension
    strncat(out_filename, "_transparent.png", sizeof(out_filename) - strlen(out_filename) - 1);

    // Save output PNG
    if (!stbi_write_png(out_filename, width, height, out_channels, out_img, width * out_channels)) {
        fprintf(stderr, "Failed to save image: %s\n", out_filename);
        free(out_img);
        stbi_image_free(img);
        return EXIT_FAILURE;
    }

    printf("Saved transparent PNG: %s\n", out_filename);

    // Clean up
    free(out_img);
    stbi_image_free(img);
    return EXIT_SUCCESS;
}
