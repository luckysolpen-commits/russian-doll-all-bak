#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define MAX_LINE 1024

// Global variables for tracking changes
off_t last_marker_size = 0;

// Function to check if frame history is enabled
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

// Function to render the display content
void render_display() {
    // Get timestamp
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[100];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    if (is_history_on()) {
        // Neat History: Pad with 5-10 lines for clean separation
        printf("\n\n\n\n\n");  // 5 blank lines
        printf("--- FRAME UPDATE at %s ---\n", timestamp);
    } else {
        // Static UI: Traditional clear
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
                size_t bytes_read = fread(content, 1, file_size, frame_file);
                content[bytes_read] = '\0';
                printf("%s", content);
                free(content);
            }
        }
        fclose(frame_file);
    }
    fflush(stdout);
    
    // Log to display ledger (AUDIT OBSESSION!)
    FILE *display_ledger = fopen("pieces/display/ledger.txt", "a");
    if (display_ledger) {
        fprintf(display_ledger, "[%s] FrameRendered: from current_frame.txt | Source: renderer\n", timestamp);
        fclose(display_ledger);
    }
    
    // Log to master ledger (AUDIT OBSESSION!)
    FILE *master = fopen("pieces/master_ledger/master_ledger.txt", "a");
    if (master) {
        fprintf(master, "[%s] FrameRendered: at %s | Source: renderer\n", timestamp, timestamp);
        fclose(master);
    }
    
    // Log full frame to session history (for debugging and audit)
    FILE *history = fopen("pieces/debug/frames/session_frame_history.txt", "a");
    if (history) {
        fprintf(history, "\n--- FRAME UPDATE at %s ---\n", timestamp);
        // Re-read and write frame content to history
        FILE *frame_file = fopen("pieces/display/current_frame.txt", "r");
        if (frame_file) {
            fseek(frame_file, 0, SEEK_END);
            long file_size = ftell(frame_file);
            fseek(frame_file, 0, SEEK_SET);
            if (file_size > 0) {
                char *content = malloc(file_size + 1);
                if (content) {
                    size_t bytes_read = fread(content, 1, file_size, frame_file);
                    content[bytes_read] = '\0';
                    fprintf(history, "%s\n", content);
                    free(content);
                }
            }
            fclose(frame_file);
        }
        fclose(history);
    }
}

int main(int argc, char **argv) {
    // Initial render
    render_display();

    struct stat st;
    if (stat("pieces/master_ledger/frame_changed.txt", &st) == 0) {
        last_marker_size = st.st_size;
    }

    while (1) {
        if (stat("pieces/master_ledger/frame_changed.txt", &st) == 0) {
            if (st.st_size != last_marker_size) {
                render_display();
                last_marker_size = st.st_size;
            }
        }
        usleep(16667); 
    }
    return 0;
}
