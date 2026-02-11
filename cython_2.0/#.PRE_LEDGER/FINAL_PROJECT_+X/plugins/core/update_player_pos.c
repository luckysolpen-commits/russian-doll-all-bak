#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <new_x> <new_y>\n", argv[0]);
        return 1;
    }
    
    int new_x = atoi(argv[1]);
    int new_y = atoi(argv[2]);
    
    // Update the player position in the piece file
    FILE *fp = fopen("peices/player/player.txt", "r");
    char temp_content[1000][200];  // Temporary storage for file content
    int line_count = 0;
    char line[200];
    
    if (fp) {
        // Read all lines except x and y
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "x ", 2) != 0 && strncmp(line, "y ", 2) != 0) {
                strcpy(temp_content[line_count], line);
                line_count++;
            }
        }
        fclose(fp);
    }
    
    // Write back to file with new x and y values
    fp = fopen("peices/player/player.txt", "w");
    if (fp) {
        // Write previously stored lines
        for (int i = 0; i < line_count; i++) {
            fputs(temp_content[i], fp);
        }
        // Add the new x and y values
        fprintf(fp, "x %d\n", new_x);
        fprintf(fp, "y %d\n", new_y);
        fclose(fp);
    }
    
    printf("Player position updated to: %d %d\n", new_x, new_y);
    return 0;
}