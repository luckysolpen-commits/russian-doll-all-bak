#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

// Function to get file modification time
time_t get_file_mod_time(const char* filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_mtime;
    }
    return 0;
}

int main() {
    printf("Response Handler Service Started\n");
    printf("Monitoring pieces/master_ledger/master_ledger.txt for response requests...\n");
    printf("Will trigger responses when ResponseRequest events are detected.\n");
    printf("Press Ctrl+C to stop response handler.\n\n");

    // Ensure the master ledger directory exists
    system("mkdir -p pieces/master_ledger");
    
    // Create master ledger file if it doesn't exist
    FILE *test_file = fopen("pieces/master_ledger/master_ledger.txt", "a");
    if (test_file) {
        fclose(test_file);
    }
    
    // Track the last position in the ledger file
    long last_position = 0;
    
    printf("Response handler is running and monitoring for events...\n");
    
    while (1) {
        FILE *fp = fopen("pieces/master_ledger/master_ledger.txt", "r");
        if (fp) {
            // Seek to last known position
            fseek(fp, last_position, SEEK_SET);
            
            char line[500];
            
            // Read new lines
            while (fgets(line, sizeof(line), fp)) {
                // Update last position
                last_position = ftell(fp);
                
                // Check if this is a response request
                if (strstr(line, "ResponseRequest: ")) {
                    // Extract the event info more carefully from the line
                    // Line format: [timestamp] ResponseRequest: event_type on piece_name | Source: source_info
                    char *original_request_part = strstr(line, "ResponseRequest: ");
                    if (original_request_part) {
                        char *request_start = original_request_part + 16;  // Skip "ResponseRequest: " (16 chars: ResponseRequest: + space)
                        
                        // Skip leading whitespace
                        while (*request_start == ' ') request_start++;
                        
                        // Find the event type (before " on ")
                        char *on_pos = strstr(request_start, " on ");
                        if (on_pos) {
                            // Calculate the length of event_type
                            int event_len = on_pos - request_start;
                            char *event_type = malloc(event_len + 1);
                            if (event_type) {
                                strncpy(event_type, request_start, event_len);
                                event_type[event_len] = '\0';
                                
                                // Extract piece name (after " on " until " |")
                                char *pipe_pos = strstr(on_pos + 4, " |");
                                if (pipe_pos) {
                                    int piece_len = pipe_pos - (on_pos + 4);
                                    char *piece_name = malloc(piece_len + 1);
                                    if (piece_name) {
                                        strncpy(piece_name, on_pos + 4, piece_len);
                                        piece_name[piece_len] = '\0';
                                        
                                        // Trim leading whitespace from piece_name (though shouldn't be needed now)
                                        char *start_piece = piece_name;
                                        while (*start_piece == ' ') start_piece++;
                                        
                                        // Call piece manager to get the appropriate response
                                        char response_cmd[500];
                                        char response[500];
                                        FILE *rsp_fp;
                                        
                                        sprintf(response_cmd, "./+x/piece_manager.+x %s get-response %s", start_piece, event_type);
                                        
                                        rsp_fp = popen(response_cmd, "r");
                                        if (rsp_fp) {
                                            if (fgets(response, sizeof(response), rsp_fp)) {
                                                response[strcspn(response, "\n")] = 0; // Remove newline
                                                
                                                // Print the response to the console for user feedback
                                                printf("\n[FUZZPET RESPONSE] %s\n", response);
                                                
                                                // Log the response to the master ledger
                                                time_t rawtime;
                                                struct tm *timeinfo;
                                                char timestamp[100];
                                                time(&rawtime);
                                                timeinfo = localtime(&rawtime);
                                                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
                                                
                                                FILE *log_fp = fopen("pieces/master_ledger/master_ledger.txt", "a");
                                                if (log_fp) {
                                                    fprintf(log_fp, "[%s] ResponseTriggered: %s on %s | Message: %s | Source: response_handler\n", 
                                                           timestamp, event_type, start_piece, response);
                                                    fclose(log_fp);
                                                }
                                            }
                                            pclose(rsp_fp);
                                        }
                                        
                                        free(piece_name);
                                    }
                                }
                                free(event_type);
                            }
                        }
                    }
                }
            }
            
            fclose(fp);
        }
        
        usleep(100000); // 100ms delay - check every 100ms
    }
    
    return 0;
}