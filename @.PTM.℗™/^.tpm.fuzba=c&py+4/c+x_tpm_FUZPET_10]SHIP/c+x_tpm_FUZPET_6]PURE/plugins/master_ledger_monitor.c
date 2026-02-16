#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Master ledger monitor continuously processes events from master_ledger.txt
int main() {
    printf("Master Ledger Monitor Started\n");
    printf("Continuously monitoring pieces/master_ledger/master_ledger.txt for new entries...\n");
    printf("Press Ctrl+C to stop monitor.\n\n");
    
    // Ensure master ledger directory and file exist
    system("mkdir -p pieces/master_ledger");
    
    FILE *fp = fopen("pieces/master_ledger/master_ledger.txt", "a+");
    if (fp) {
        fclose(fp);
    }
    
    // Track the last position in the ledger file
    long last_position = 0;
    
    while (1) {
        FILE *fp = fopen("pieces/master_ledger/master_ledger.txt", "r");
        if (fp) {
            // Seek to last known position
            fseek(fp, last_position, SEEK_SET);
            
            char line[500];
            int new_entries = 0;
            
            // Read new lines
            while (fgets(line, sizeof(line), fp)) {
                // Update last position
                last_position = ftell(fp);
                
                // Process the ledger entry
                if (strstr(line, "MethodCall:")) {
                    printf(">> Processing MethodCall: %s", line);
                } else if (strstr(line, "StateChange:")) {
                    printf(">> Processing StateChange: %s", line);
                } else if (strstr(line, "EventFire:") || strstr(line, "event_fired")) {
                    printf(">> Processing Event: %s", line);
                } else if (strstr(line, "SystemEvent:")) {
                    printf(">> Processing SystemEvent: %s", line);
                } else if (strstr(line, "InputReceived:")) {
                    printf(">> Processing Input: %s", line);
                } else {
                    printf(">> Log Entry: %s", line);
                }
                new_entries++;
            }
            
            fclose(fp);
            
            if (new_entries > 0) {
                printf("--- End of new entries ---\n\n");
            }
        }
        
        // Small delay to avoid busy-waiting and reduce CPU usage
        usleep(100000); // 100ms delay
    }
    
    return 0;
}