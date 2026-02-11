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
    
    sprintf(response_cmd, "./pieces/plugins/+x/piece_manager.+x mover get-response %s", event_name);
    fp = popen(response_cmd, "r");
    if (fp) {
        fgets(response, sizeof(response), fp);
        pclose(fp);
        
        // Remove trailing newline if present
        response[strcspn(response, "\n")] = '\0';
        
        // Print the response
        printf("[Mover] %s\n", response);
        
        // Log to master ledger
        sprintf(response_cmd, "./pieces/plugins/+x/log_event.+x EventFire \"%s on mover\" \"Trigger: event_handling\"", event_name);
        system(response_cmd);
    }
    
    return 0;
}