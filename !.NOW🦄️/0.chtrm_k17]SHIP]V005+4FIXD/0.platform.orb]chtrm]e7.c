#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define MAX_THREADS 10

struct termios original_term; // Store original terminal settings

void save_term() {
    tcgetattr(0, &original_term); // Save terminal settings
}

void restore_term() {
    tcsetattr(0, TCSANOW, &original_term); // Restore terminal settings
}

// Thread function that executes the system command
void* thread_func(void* data) {
    char* command = (char*)data;
    save_term(); // Save terminal settings before running command
    system(command);
    restore_term(); // Restore terminal settings after command
    free(command);  // Free the duplicated string
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    // Check if we have enough arguments
    if (argc < 2) {
        printf("Usage: %s <command1> [command2] [command3] ... [chtm_file_path]\n", argv[0]);
        printf("Up to %d commands can be specified\n", MAX_THREADS);
        return 1;
    }

    // Truncate or create the output file
    FILE* fp = fopen("gl_cli_out.txt", "w");
    if (fp) {
        fclose(fp);
    } else {
        perror("Failed to open gl_cli_out.txt");
        exit(1);
    }

    // Use the last argument as the file path only when there are more than 2 commands (program + 2 commands)
    char file_path[512] = "chtm/index.chtm"; // Default path
    int has_file_arg = 0; // Flag to indicate if there's an extra file argument
    
    // If we have more than program name + 2 commands, treat the last argument as a file path
    if (argc > 3) {  // More than program + 2 commands
        strncpy(file_path, argv[argc - 1], sizeof(file_path) - 1);
        file_path[sizeof(file_path) - 1] = '\0';
        has_file_arg = 1;
    }

    // Calculate number of threads (limited to MAX_THREADS)
    // Exclude the last argument if it's a file path
    int num_threads = has_file_arg ? argc - 2 : argc - 1;  // Excluding program name and possibly file arg
    
    if (num_threads <= 0) {
        printf("Error: No valid commands provided.\n");
        return 1;
    }
    if (num_threads > MAX_THREADS) {
        num_threads = MAX_THREADS;
        printf("Warning: Limited to %d threads. Additional arguments ignored.\n", MAX_THREADS);
    }

    // Array to store thread IDs and command strings
    pthread_t threads[MAX_THREADS];
    char* thread_commands[MAX_THREADS];

    // Create threads
    for (int i = 0; i < num_threads; i++) {
        // Create command string with redirection using tee -a
        char* original_cmd = argv[i + 1];
        
        // Always treat as a renderer command and append the file path if we have one
        char* modified_cmd = NULL;
        if (has_file_arg) {
            const char* redirect = " 2>&1 | tee -a gl_cli_out.txt";
            size_t len = strlen(original_cmd) + strlen(file_path) + 2 + strlen(redirect) + 1; // +2 for space and null
            modified_cmd = malloc(len);
            if (!modified_cmd) {
                perror("Failed to allocate memory for modified command string");
                exit(1);
            }
            snprintf(modified_cmd, len, "%s %s%s", original_cmd, file_path, redirect);
        } else {
            // No file argument, just add the redirect
            const char* redirect = " 2>&1 | tee -a gl_cli_out.txt";
            size_t len = strlen(original_cmd) + strlen(redirect) + 1;
            modified_cmd = malloc(len);
            if (!modified_cmd) {
                perror("Failed to allocate memory for command string");
                exit(1);
            }
            snprintf(modified_cmd, len, "%s%s", original_cmd, redirect);
        }
        
        thread_commands[i] = modified_cmd;

        // Create thread
        if (pthread_create(&threads[i], NULL, thread_func, thread_commands[i])) {
            perror("Failed to create thread");
            free(thread_commands[i]);
            exit(1);
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Print the file path being used
    printf("Using file: %s\n", file_path);

    return 0;
}
