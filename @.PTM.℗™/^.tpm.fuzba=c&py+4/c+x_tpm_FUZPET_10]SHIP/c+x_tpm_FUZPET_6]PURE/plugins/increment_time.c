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
    
    // Look for time line and update it
    int time_updated = 0;
    for (int i = 0; i < line_count; i++) {
        if (strncmp(temp_lines[i], "time ", 5) == 0) {
            // Extract current time
            char current_time[20] = "08:00:00";
            // Find the time portion after "time "
            char *time_start = temp_lines[i] + 5; // Skip "time "
            while (*time_start == ' ') time_start++; // Skip spaces
            // Remove newline if present
            int len = strlen(time_start);
            if (len > 0 && time_start[len-1] == '\n') {
                time_start[len-1] = '\0';
            }
            strncpy(current_time, time_start, 19);
            current_time[19] = '\0';
            
            // Parse current time
            int hours = 8, minutes = 0, seconds = 0;
            if (sscanf(current_time, "%d:%d:%d", &hours, &minutes, &seconds) < 3) {
                // If parsing fails, use default
                hours = 8; minutes = 0; seconds = 0;
            }
            
            // Increment by 5 minutes
            minutes += 5;
            
            // Handle overflow
            if (minutes >= 60) {
                minutes -= 60;
                hours++;
                if (hours >= 24) {
                    hours = hours % 24;
                }
            }
            
            // Format the new time
            char new_time[20];
            sprintf(new_time, "%02d:%02d:%02d", hours, minutes, seconds);
            
            // Update the line
            snprintf(temp_lines[i], MAX_LINE, "time %s\n", new_time);
            time_updated = 1;
        }
    }
    
    // If no time line was found, add one
    if (!time_updated && line_count < 100) {
        strcpy(temp_lines[line_count], "time 08:05:00\n");
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