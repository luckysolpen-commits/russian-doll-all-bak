#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    FILE *fp = fopen("peices/player/player.txt", "r");
    if (fp) {
        char line[200];
        int x = -1, y = -1;
        
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "x ", 2) == 0) {
                sscanf(line + 2, "%d", &x);
            } else if (strncmp(line, "y ", 2) == 0) {
                sscanf(line + 2, "%d", &y);
            }
        }
        fclose(fp);
        
        if (x != -1 && y != -1) {
            printf("%d %d\n", x, y);
            return 0;
        }
    }
    
    // Create default player file
    fp = fopen("peices/player/player.txt", "w");
    if (fp) {
        fprintf(fp, "x 15\n");
        fprintf(fp, "y 10\n");
        fprintf(fp, "symbol @\n");
        fprintf(fp, "health 100\n");
        fprintf(fp, "status active\n");
        fclose(fp);
    }
    
    printf("15 10\n");
    return 0;
}