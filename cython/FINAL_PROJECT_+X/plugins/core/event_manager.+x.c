#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Event Manager plugin - handles scheduled events based on clock time
// Works with events piece to execute timed methods

int main() {
    printf("Event Manager plugin initialized\n");
    
    // Check for clock events that need to be triggered
    // Get current clock time from piece manager
    FILE *fp = popen("./+x/piece_manager.+x get_piece_state clock", "r");
    
    if (fp) {
        char line[256];
        int current_time = 0;
        int last_checked_time = 0;
        
        // Read through the clock piece state
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "current_time ", 13) == 0) {
                sscanf(line + 13, "%d", &current_time);
            } else if (strncmp(line, "last_checked_time ", 18) == 0) {
                sscanf(line + 18, "%d", &last_checked_time);
            }
        }
        pclose(fp);
        
        // If this is a new turn (time has increased)
        if (current_time > last_checked_time) {
            printf("Time increased from %d to %d, checking for turn_end events\n", last_checked_time, current_time);
            
            // Trigger turn_end events by calling methods_plugin
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "./+x/methods_plugin.+x process_turn clock");
            system(cmd);
            
            // Update the last checked time in the clock piece
            snprintf(cmd, sizeof(cmd), "./+x/piece_manager.+x update_piece_state clock clock last_checked_time %d", current_time);
            system(cmd);
            
            // Check for scheduled events in clock_events piece
            FILE *events_fp = popen("./+x/piece_manager.+x get_piece_state clock_events", "r");
            if (events_fp) {
                char line[256];
                int events_triggered = 0;
                while (fgets(line, sizeof(line), events_fp)) {
                    // Look for scheduled events like event_0_time, event_1_time, etc.
                    if (strncmp(line, "event_", 6) == 0 && strstr(line, "_time ")) {
                        int event_num, event_time;
                        if (sscanf(line, "event_%d_time %d", &event_num, &event_time) == 2) {
                            if (event_time <= current_time) {
                                // Execute the event
                                char event_cmd[256];
                                snprintf(event_cmd, sizeof(event_cmd), "echo \"Executing scheduled event %d at time %d\"", event_num, event_time);
                                system(event_cmd);
                                
                                // Call the corresponding method using methods plugin
                                char method_call[256];
                                snprintf(method_call, sizeof(method_call), "./+x/methods_plugin.+x tick clock");
                                system(method_call);
                                
                                events_triggered++;
                            }
                        }
                    }
                }
                pclose(events_fp);
                
                if (events_triggered > 0) {
                    printf("Executed %d scheduled events at time %d\n", events_triggered, current_time);
                }
            }
        } else {
            // Update the last checked time but no new events to process
            if (last_checked_time > 0) {
                char cmd[256];
                snprintf(cmd, sizeof(cmd), "./+x/piece_manager.+x update_piece_state clock clock last_checked_time %d", current_time);
                system(cmd);
            }
        }
    } else {
        printf("Could not get clock state for event checking\n");
    }
    
    printf("Event Manager finished checking events\n");
    return 0;
}