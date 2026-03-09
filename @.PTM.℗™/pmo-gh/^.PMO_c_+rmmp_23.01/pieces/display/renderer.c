#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

// renderer.c - Terminal Renderer (v1.1 - PULSE FIX)
// Responsibility: Watch for renderer_pulse.txt to avoid spam.

off_t last_marker_size = 0;

int is_history_on() {
    FILE *f = fopen("pieces/display/state.txt", "r");
    if (!f) return 1; 
    char line[128];
    int on = 1;
    if (fgets(line, sizeof(line), f)) {
        if (strstr(line, "off")) on = 0;
    }
    fclose(f);
    return on;
}

void render_display() {
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[100];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    if (is_history_on()) {
        printf("\n\n\n\n\n");
        printf("--- FRAME UPDATE at %s ---\n", timestamp);
    } else {
        printf("\033[H\033[J");
    }
    
    FILE *frame_file = fopen("pieces/display/current_frame.txt", "r");
    if (frame_file) {
        fseek(frame_file, 0, SEEK_END);
        long file_size = ftell(frame_file);
        fseek(frame_file, 0, SEEK_SET);
        if (file_size > 0) {
            char *content = malloc(file_size + 1);
            if (content) {
                size_t n = fread(content, 1, file_size, frame_file);
                content[n] = '\0';
                printf("%s", content);
                free(content);
            }
        }
        fclose(frame_file);
    }
    fflush(stdout);
}

int main() {
    render_display();
    struct stat st;
    const char* pulse_path = "pieces/display/renderer_pulse.txt";
    if (stat(pulse_path, &st) == 0) last_marker_size = st.st_size;

    while (1) {
        if (stat(pulse_path, &st) == 0) {
            if (st.st_size != last_marker_size) {
                render_display();
                last_marker_size = st.st_size;
            }
        }
        usleep(16667); 
    }
    return 0;
}
