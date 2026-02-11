#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <event_name>\n", argv[0]);
        return 1;
    }
    
    const char *event_name = argv[1];
    
    // Get the response for the event
    char response_cmd[500];
    char response[500];
    FILE *fp;
    
    sprintf(response_cmd, "./pieces/plugins/+x/piece_manager.+x fuzzpet get-response %s", event_name);
    fp = popen(response_cmd, "r");
    if (fp) {
        size_t bytes_read = fread(response, 1, sizeof(response)-1, fp);
        response[bytes_read] = '\0';  // Null terminate
        pclose(fp);
        
        // Remove trailing whitespace including newlines
        for (int i = strlen(response) - 1; i >= 0 && (response[i] == '\n' || response[i] == '\r' || response[i] == ' '); i--) {
            response[i] = '\0';
        }
        
        // Print the pet's response
        printf("[Pet] %s\r", response);
    }
    
    return 0;
}