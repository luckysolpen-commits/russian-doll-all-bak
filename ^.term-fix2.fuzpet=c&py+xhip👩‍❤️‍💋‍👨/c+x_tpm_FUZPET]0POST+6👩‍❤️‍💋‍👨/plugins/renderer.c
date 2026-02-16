#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#define MAX_LINE 1024

// Global variables for tracking changes
time_t last_current_frame_time = 0;

// Function to get file modification time
time_t get_file_mod_time(const char* filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_mtime;
    }
    return 0;
}

// Function to render the display content
void render_display() {
    // Print a separator to mark the new frame
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[100];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    printf("\n--- FRAME UPDATE at %s ---\n", timestamp);
    
    // Read and display the current frame from the display piece
    FILE *frame_file = fopen("pieces/display/current_frame.txt", "r");
    if (frame_file) {
        // Read the entire file content in one go to avoid character-by-char processing issues
        fseek(frame_file, 0, SEEK_END);
        long file_size = ftell(frame_file);
        fseek(frame_file, 0, SEEK_SET);
        
        if (file_size > 0) {
            char *content = malloc(file_size + 1);
            if (content) {
                size_t bytes_read = fread(content, 1, file_size, frame_file);
                content[bytes_read] = '\0';  // Null terminate
                printf("%s", content);
                free(content);
            } else {
                // Fallback to line-by-line if memory allocation fails
                rewind(frame_file);
                char line[200];
                while (fgets(line, sizeof(line), frame_file)) {
                    printf("%s", line);
                }
            }
        }
        fclose(frame_file);
    } else {
        // If current_frame.txt doesn't exist, create a default display
        printf("==================================\n");
        printf("       FUZZPET DASHBOARD        \n");
        printf("==================================\n");
        printf("Pet Name: Fuzzball\n");
        printf("Hunger: 50\n");
        printf("Happiness: 75\n");
        printf("Energy: 100\n");
        printf("Level: 1\n");
        printf("Position: (5, 5)\n");
        printf("\n");
        printf("GAME MAP:\n");
        printf("####################\n");
        printf("#  ...............T#\n");
        printf("#  ...............T#\n"); 
        printf("#  ....R@.........T#\n");
        printf("#  ....R..........T#\n");
        printf("#  ....R..........T#\n");
        printf("#  ....R..........T#\n");
        printf("#  ................#\n");
        printf("#                  #\n");
        printf("####################\n");
        printf("\nControls: w/s/a/d to move, 1=feed, 2=play, 3=sleep, 4=status, 5=evolve\n");
        printf("==================================\n");
    }
}

int main(int argc, char **argv) {
    printf("Renderer Service Started\n");
    printf("Monitoring pieces/display/current_frame.txt for changes...\n");
    printf("Only updates display when content changes.\n");
    printf("Preserving scroll history as frames are appended.\n");
    printf("Press Ctrl+C to stop renderer.\n\n");

    // Ensure the display directory exists
    system("mkdir -p pieces/display");
    
    // Create current_frame.txt if it doesn't exist
    FILE *test_file = fopen("pieces/display/current_frame.txt", "r");
    if (!test_file) {
        test_file = fopen("pieces/display/current_frame.txt", "w");
        if (test_file) {
            fprintf(test_file, "==================================\n");
            fprintf(test_file, "       FUZZPET DASHBOARD        \n");
            fprintf(test_file, "==================================\n");
            fprintf(test_file, "Pet Name: Fuzzball\n");
            fprintf(test_file, "Hunger: 50\n");
            fprintf(test_file, "Happiness: 75\n");
            fprintf(test_file, "Energy: 100\n");
            fprintf(test_file, "Level: 1\n");
            fprintf(test_file, "Position: (5, 5)\n");
            fprintf(test_file, "\n");
            fprintf(test_file, "GAME MAP:\n");
            fprintf(test_file, "####################\n");
            fprintf(test_file, "#  ...............T#\n");
            fprintf(test_file, "#  ...............T#\n"); 
            fprintf(test_file, "#  ....R@.........T#\n");
            fprintf(test_file, "#  ....R..........T#\n");
            fprintf(test_file, "#  ....R..........T#\n");
            fprintf(test_file, "#  ....R..........T#\n");
            fprintf(test_file, "#  ................#\n");
            fprintf(test_file, "#                  #\n");
            fprintf(test_file, "####################\n");
            fprintf(test_file, "\nControls: w/s/a/d to move, 1=feed, 2=play, 3=sleep, 4=status, 5=evolve\n");
            fprintf(test_file, "==================================\n");
            fclose(test_file);
        }
    } else {
        fclose(test_file);
    }
    
    // Do initial render
    render_display();
    last_current_frame_time = get_file_mod_time("pieces/display/current_frame.txt");
    
    // Main monitoring loop
    time_t current_time;
    while (1) {
        current_time = get_file_mod_time("pieces/display/current_frame.txt");
        if (current_time != last_current_frame_time) {
            render_display();
            last_current_frame_time = current_time;
        }
        usleep(100000); // Check every 100ms
    }
    
    return 0;
}