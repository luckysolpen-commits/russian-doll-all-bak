/*** history_monitor.c - TPM Compliant History Monitor ***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

// Include the modules directly since we're not using headers
#include "piece_manager.c"
#include "event_manager.c"
#include "display.c"

#define HISTORY_FILE "peices/input/keyboard/history.txt"
#define MAX_PATH 256

// Function to get the last modification time of a file
time_t getFileModificationTime(const char* filename) {
    struct stat attrib;
    if (stat(filename, &attrib) == 0) {
        return attrib.st_mtime;
    }
    return 0;
}

int main() {
    printf("Starting TPM History Monitor...\n");
    printf("Using True Piece Method Architecture\n");
    printf("- Watching history.txt for changes\n");
    printf("- Display reads from history to simulate typing\n");
    printf("- Press Ctrl+C to quit\n\n");
    
    // Initialize TPM components
    initPieceManager();
    initEventManager();
    
    // Ensure directories exist
    system("mkdir -p peices/input/keyboard");
    
    // Create/clear history file on startup
    FILE* fp = fopen(HISTORY_FILE, "w");
    if (fp) {
        fclose(fp);
    }
    
    time_t last_modified = getFileModificationTime(HISTORY_FILE);
    int iteration_count = 0;
    
    printf("Display:  | History Lines: 0\r");
    fflush(stdout);
    
    while (iteration_count < 10000) { // Run for a while then exit for demonstration
        time_t current_modified = getFileModificationTime(HISTORY_FILE);
        
        // Check if history file has changed
        if (current_modified != last_modified) {
            // Update the display
            renderFromHistory();
            last_modified = current_modified;
        }
        
        iteration_count++;
        usleep(100000); // 100ms delay
    }
    
    printf("\nHistory Monitor stopped.\n");
    return 0;
}