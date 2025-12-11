#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Test the VARS parsing functionality
void test_vars_parsing() {
    char response[] = "VARS:x=1,y=2,z=3,player_x=5,player_y=10,last_action=move;";
    
    printf("Testing VARS parsing:\n");
    printf("Input: %s\n", response);
    
    // Check if this is a batched variable update
    if (strncmp(response, "VARS:", 5) == 0) {
        printf("Detected VARS format\n");
        char *var_list = response + 5; // Skip "VARS:"
        char *semi_pos = strrchr(var_list, ';'); // Find last semicolon
        if (semi_pos) {
            *semi_pos = '\0'; // Replace semicolon with null terminator
            printf("Variable list: %s\n", var_list);
        } else {
            printf("ERROR: Invalid VARS format - missing semicolon\n");
            return;
        }
        
        // Parse each variable in the comma-separated list
        char *token = strtok(var_list, ",");
        while (token != NULL) {
            char *equals = strchr(token, '=');
            if (equals) {
                *equals = '\0';
                char *name = token;
                char *value = equals + 1;
                printf("Parsed: %s = %s\n", name, value);
            } else {
                printf("ERROR: Invalid variable format: %s\n", token);
            }
            token = strtok(NULL, ",");
        }
    }
    
    printf("Test completed.\n");
}

int main() {
    test_vars_parsing();
    return 0;
}