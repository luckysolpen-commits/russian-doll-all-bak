#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#define MAX_PATH_LEN 512
#define MAX_BUFFER_SIZE 4096

// Buffer for batching variable updates
char var_buffer[MAX_BUFFER_SIZE];
int buf_pos = 0;

// Macro for adding variables to the buffer
#define ADD_VAR_INT(name, value) \
    buf_pos += snprintf(var_buffer + buf_pos, sizeof(var_buffer) - buf_pos, \
    "%s=%d,", #name, value)

// Specialized macro for string values
#define ADD_VAR_STR(name, value) \
    buf_pos += snprintf(var_buffer + buf_pos, sizeof(var_buffer) - buf_pos, \
    "%s=%s,", #name, value)

int main() {
    char input[512];
    
    // Debug log when module starts
    printf("innerHTML Test Module Ready\n");
    printf("VAR:status_message=innerHTML Module Loaded;\n");
    fflush(stdout);
    
    // Main loop to process commands from parent process (CHTM renderer)
    while (fgets(input, sizeof(input), stdin)) {
        // Remove newline character
        input[strcspn(input, "\n")] = 0;
        
        if (strcmp(input, "ACTION:1") == 0) {
            // Action triggered by button created via innerHTML
            printf("VAR:status_message=Button 1 from innerHTML clicked!;\n");
            fflush(stdout);
        }
        else if (strcmp(input, "ACTION:2") == 0) {
            // Action triggered by another button created via innerHTML
            printf("VAR:status_message=Button 2 from innerHTML clicked!;\n");
            fflush(stdout);
        }
        else if (strcmp(input, "ACTION:menu1") == 0) {
            // Action for menu item 1
            printf("VAR:status_message=Menu Item 1 clicked!;\n");
            fflush(stdout);
        }
        else if (strcmp(input, "ACTION:menu2") == 0) {
            // Action for menu item 2
            printf("VAR:status_message=Menu Item 2 clicked!;\n");
            fflush(stdout);
        }
        else if (strcmp(input, "CREATE_DYNAMIC_BUTTONS") == 0) {
            // Create dynamic content with buttons
            char content[1024];
            snprintf(content, sizeof(content), 
                "<button onClick=\"ACTION:dynamic1\">Dynamic Button 1</button><br/>"
                "<button onClick=\"ACTION:dynamic2\">Dynamic Button 2</button><br/>"
                "<text>This content was created dynamically!</text>");
            
            printf("INNER_HTML:dynamic_content:%s\n", content);
            printf("VAR:status_message=Dynamic content created!;\n");
            printf("INNER_HTML_DEBUG: Replacing content in element 'dynamic_content'\n");
            fflush(stdout);
        }
        else if (strcmp(input, "CREATE_COMPLEX_MENU") == 0) {
            // Create a more complex menu structure
            char content[1024];
            snprintf(content, sizeof(content),
                "<menu id=\"complex_menu\" visibility=\"1\">"
                "<text>Dynamic Menu</text><br/>"
                "<button onClick=\"ACTION:complex_menu_1\">Submenu Button 1</button><br/>"
                "<button onClick=\"ACTION:complex_menu_2\">Submenu Button 2</button><br/>"
                "</menu>");
            
            printf("INNER_HTML:dynamic_content:%s\n", content);
            printf("VAR:status_message=Complex menu created!;\n");
            fflush(stdout);
        }
        else if (strcmp(input, "QUIT") == 0) {
            printf("VAR:status_message=Module exiting...;\n");
            break;
        }
        else {
            // Unknown command - just echo it back for debugging
            printf("VAR:unknown_command=%s, status_message=Unknown command received: %s;\n", input, input);
            fflush(stdout);
        }
    }
    
    return 0;
}