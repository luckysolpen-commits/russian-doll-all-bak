#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 256

int main() {
    // Read the current state file into a temporary buffer
    FILE *infile = fopen("pieces/clock/state.txt", "r");
    if (!infile) {
        // Create the directory if it doesn't exist
        system("mkdir -p pieces/clock");
        FILE *newfile = fopen("pieces/clock/state.txt", "w");
        if (newfile) {
            fprintf(newfile, "turn 0\ntime 08:00:00\n");
            fclose(newfile);
        }
        infile = fopen("pieces/clock/state.txt", "r");
        if (!infile) {
            return 1;
        }
    }
    
    char temp_lines[100][MAX_LINE];
    int line_count = 0;
    
    // Read all lines
    while (line_count < 100 && fgets(temp_lines[line_count], MAX_LINE, infile)) {
        line_count++;
    }
    fclose(infile);
    
    // Look for turn line and update it
    int turn_updated = 0;
    for (int i = 0; i < line_count; i++) {
        if (strncmp(temp_lines[i], "turn ", 5) == 0) {
            // Extract current turn
            int current_turn = 0;
            // Find the turn portion after "turn "
            char *turn_start = temp_lines[i] + 5; // Skip "turn "
            while (*turn_start == ' ') turn_start++; // Skip spaces
            // Parse the number
            current_turn = atoi(turn_start);
            
            // Increment turn
            current_turn++;
            
            // Update the line
            snprintf(temp_lines[i], MAX_LINE, "turn %d\n", current_turn);
            turn_updated = 1;
        }
    }
    
    // If no turn line was found, add one
    if (!turn_updated && line_count < 100) {
        strcpy(temp_lines[line_count], "turn 1\n");
        line_count++;
    }
    
    // Write the updated content back to the file
    FILE *outfile = fopen("pieces/clock/state.txt", "w");
    if (!outfile) {
        return 1;
    }
    
    for (int i = 0; i < line_count; i++) {
        fputs(temp_lines[i], outfile);
    }
    
    fclose(outfile);
    return 0;
}