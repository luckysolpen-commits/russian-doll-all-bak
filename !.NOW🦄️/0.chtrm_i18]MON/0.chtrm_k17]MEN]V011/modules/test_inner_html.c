#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

// Global flag to handle graceful shutdown
volatile sig_atomic_t g_sig_received = 0;

// Signal handler for graceful shutdown
void signal_handler(int sig) {
    g_sig_received = sig;
}

int main() {
    // Register signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    char input[1024];
    
    // Debug: Log module startup with timestamp
    FILE *debug_file = fopen("module_debug.log", "a");
    if (debug_file) {
        time_t now = time(0);
        char* time_str = ctime(&now);
        fprintf(debug_file, "MODULE STARTUP: innerhtml test module started at %s", time_str);
        fclose(debug_file);
    }
    
    // Send initial status message to indicate readiness
    printf("VAR:last_action=InnerHTML Test Module Ready\n");
    fflush(stdout);
    
    printf("VAR:status_message=InnerHTML Test Module Active\n");
    fflush(stdout);
    
    while (1) {
        // Check if a signal was received that requires termination
        if (g_sig_received) {
            debug_file = fopen("module_debug.log", "a");
            if (debug_file) {
                time_t now = time(0);
                char* time_str = ctime(&now);
                fprintf(debug_file, "MODULE SHUTDOWN: shutting down after signal %d at %s", g_sig_received, time_str);
                fclose(debug_file);
            }
            
            printf("VAR:status_message=Shutting down after signal %d\n", g_sig_received);
            fflush(stdout);
            break;
        }
        
        if (fgets(input, sizeof(input), stdin) != NULL) {
            // Remove newline if present
            input[strcspn(input, "\n")] = 0;
            
            // Log what we received with timestamp
            debug_file = fopen("module_debug.log", "a");
            if (debug_file) {
                time_t now = time(0);
                char* time_str = ctime(&now);
                fprintf(debug_file, "RECEIVED: %s at %s", input, time_str);
                fclose(debug_file);
            }
            
            if (strncmp(input, "TEST_INNER_HTML", 15) == 0) {
                debug_file = fopen("module_debug.log", "a");
                if (debug_file) {
                    time_t now = time(0);
                    char* time_str = ctime(&now);
                    fprintf(debug_file, "PROCESSING: TEST_INNER_HTML command at %s", time_str);
                    fclose(debug_file);
                }
                
                // Debug: log sending innerHTML command
                debug_file = fopen("module_debug.log", "a");
                if (debug_file) {
                    time_t now = time(0);
                    char* time_str = ctime(&now);
                    fprintf(debug_file, "SENDING: INNER_HTML:test_container:... at %s", time_str);
                    fclose(debug_file);
                }
                
                // Send innerHTML command to populate content
                printf("INNER_HTML:test_container:<h3>Dynamic Projects List:</h3><button onClick=\"PROJECT_SELECTED:Project1\">Project 1</button><br/><button onClick=\"PROJECT_SELECTED:Project2\">Project 2</button><br/><button onClick=\"PROJECT_SELECTED:Project3\">Project 3</button><br/><text label=\"This content was dynamically added!\"/><br/>\n");
                fflush(stdout);
                
                // Send status update
                printf("VAR:status_message=InnerHTML command processed\n");
                fflush(stdout);
                
                // Debug: log that commands were sent
                debug_file = fopen("module_debug.log", "a");
                if (debug_file) {
                    time_t now = time(0);
                    char* time_str = ctime(&now);
                    fprintf(debug_file, "SENT: Commands sent to framework at %s", time_str);
                    fclose(debug_file);
                }
            }
            else if (strcmp(input, "LOAD_MORE_CONTENT") == 0) {
                // Send different content to test replacement
                printf("INNER_HTML:test_container:<h3>Updated Content:</h3><menu label=\"Dynamic Menu:\"><button onClick=\"MENU_ITEM:Option1\">Option 1</button><button onClick=\"MENU_ITEM:Option2\">Option 2</button></menu><br/><button onClick=\"ANOTHER_ACTION\">Additional Button</button><br/><text label=\"Content updated dynamically!\"/><br/>\n");
                fflush(stdout);
            }
            else if (strcmp(input, "CLEAR_CONTENT") == 0) {
                // Clear the content
                printf("INNER_HTML:test_container:<p>Container is now empty. Click buttons above to load content.</p>\n");
                fflush(stdout);
            }
            else if (strncmp(input, "PROJECT_SELECTED:", 17) == 0) {
                // Handle project selection
                char* project_name = input + 17; // Skip "PROJECT_SELECTED:"
                printf("VAR:status_message=Selected project: %s\n", project_name);
                printf("INNER_HTML:test_container:<h3>Selected: %s</h3><p>You selected the project \"%s\" successfully!</p><button onClick=\"TEST_INNER_HTML\">Back to Projects List</button><br/>\n", project_name, project_name);
                fflush(stdout);
            }
            else if (strncmp(input, "MENU_ITEM:", 10) == 0) {
                // Handle menu item selection
                char* item_name = input + 10; // Skip "MENU_ITEM:"
                printf("VAR:status_message=Selected menu item: %s\n", item_name);
                printf("INNER_HTML:test_container:<h3>Menu Item Selected: %s</h3><p>You clicked on \"%s\" from the dynamic menu!</p><button onClick=\"LOAD_MORE_CONTENT\">Back to Menu List</button><br/>\n", item_name, item_name);
                fflush(stdout);
            }
            else if (strcmp(input, "QUIT") == 0 || strcmp(input, "q") == 0) {
                break;
            }
        }
        
        // Small delay to prevent excessive CPU usage
        usleep(10000); // 10ms
    }
    
    return 0;
}