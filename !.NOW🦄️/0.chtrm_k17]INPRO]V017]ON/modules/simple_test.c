#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char input[512];
    
    // Send initial ready message
    printf("Simple Test Module Ready\n");
    printf("VAR:status_message=Simple module loaded;\n");
    fflush(stdout);
    
    // Main command loop
    while (fgets(input, sizeof(input), stdin)) {
        // Remove newline
        input[strcspn(input, "\n")] = 0;
        
        if (strcmp(input, "CREATE_DYNAMIC_BUTTONS") == 0) {
            // Send INNER_HTML command
            printf("INNER_HTML:dynamic_content:<button onClick='ACTION:dynamic1'>Simple Dynamic Button</button><br/><text>Simple dynamic content!</text>\n");
            printf("VAR:status_message=Simple dynamic content created via innerHTML!;\n");
            fflush(stdout);
        }
        else if (strcmp(input, "QUIT") == 0) {
            break;
        }
        else {
            // Echo unknown commands for debugging
            printf("VAR:last_command=%s;\n", input);
            fflush(stdout);
        }
    }
    
    return 0;
}