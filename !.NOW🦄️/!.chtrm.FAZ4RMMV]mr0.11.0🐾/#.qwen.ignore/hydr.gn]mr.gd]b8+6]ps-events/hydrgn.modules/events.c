#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_event_commands(int tab_num) {
    char filename[256];
    snprintf(filename, sizeof(filename), "hydrgn.modules/events/event.commands.%d.txt", tab_num);
    
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Could not open %s\n", filename);
        // If current tab doesn't exist, show tab 1 + missing tab message
        if (tab_num > 1) {
            printf("\nTab 1 + missing tab (no commands found for tab %d)\n", tab_num);
        }
        return;
    }
    
    printf("Events for Tab %d (from %s):\n", tab_num, filename);
    printf("----------------------------------------\n");
    
    char line[512];
    int line_num = 1;
    while (fgets(line, sizeof(line), fp)) {
        // Remove newline character
        line[strcspn(line, "\n")] = 0;
        printf("%d: %s\n", line_num, line);
        line_num++;
    }
    fclose(fp);
}

int main(int argc, char *argv[]) {
    int tab = 1;  // Default to tab 1
    
    if (argc > 1) {
        tab = atoi(argv[1]);
        if (tab < 1 || tab > 3) {
            tab = 1;  // Default back to tab 1 if invalid
        }
    }
    
    print_event_commands(tab);
    
    // Show available tabs
    printf("\nAvailable tabs: ");
    for (int i = 1; i <= 3; i++) {
        if (i == tab) {
            printf("[%d] ", i);
        } else {
            printf("%d ", i);
        }
    }
    printf("\nPress any key to return to main menu...\n");
    
    // Wait for user input before continuing
    getchar();
    
    return 0;
}
