/*** display.c - TPM Compliant Display Module ***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE 1024
#define MAX_HISTORY_LINES 1000

// Function to read and display from history
void displayFromHistory(const char* history_file_path) {
    FILE* fp = fopen(history_file_path, "r");
    if (!fp) {
        printf("Display: ERROR - Could not open %s\n", history_file_path);
        return;
    }
    
    char history_content[MAX_HISTORY_LINES * 10]; // Buffer for history content
    strcpy(history_content, "");
    
    char line[MAX_LINE];
    int line_count = 0;
    
    while (fgets(line, MAX_LINE, fp) && line_count < MAX_HISTORY_LINES) {
        line[strcspn(line, "\n\r")] = 0; // Remove newline
        
        // Process special history entries
        if (strcmp(line, "[ENTER]") == 0) {
            strcat(history_content, "\n");
        } 
        else if (line[0] == '[' && line[strlen(line)-1] == ']') {
            // Skip special control sequences like [CTRL_X], [ARROW_LEFT], etc.
            // These don't appear in the display
        } 
        else if (strlen(line) == 1) {
            // Single character - add to display
            char temp[2] = {line[0], '\0'};
            strcat(history_content, temp);
        }
        
        line_count++;
    }
    
    fclose(fp);
    
    // Print the content in a single line for smooth display
    printf("\rDisplay: %s | History Lines: %d", history_content, line_count);
    fflush(stdout);
}

// Clear the display (by printing spaces)
void clearDisplay() {
    printf("\r%80s\r", "");  // Clear line by printing spaces and return
    fflush(stdout);
}

// Update the display if history has changed
void updateDisplayIfNeeded(const char* history_file_path, int previous_line_count) {
    FILE* fp = fopen(history_file_path, "r");
    if (!fp) return;
    
    int current_line_count = 0;
    char line[MAX_LINE];
    
    while (fgets(line, MAX_LINE, fp)) {
        current_line_count++;
    }
    
    fclose(fp);
    
    // Only update if the history has grown
    if (current_line_count > previous_line_count) {
        displayFromHistory(history_file_path);
    }
}

// Render from history in a simple way
void renderFromHistory() {
    const char* history_path = "peices/input/keyboard/history.txt";
    displayFromHistory(history_path);
}