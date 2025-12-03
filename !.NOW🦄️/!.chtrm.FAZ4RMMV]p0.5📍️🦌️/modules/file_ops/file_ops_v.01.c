#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

#define MAX_PATH_LEN 512
#define MAX_FILENAME_LEN 256
#define MAX_PROJECTS 100

// Global variables for signal handling
int g_sig_received = 0;
int g_sig_num = 0;

// Buffer for batching variable updates
char var_buffer[4096];
int buf_pos = 0;

// Macro for adding variables to the buffer
#define ADD_VAR_INT(name, value) \
    buf_pos += snprintf(var_buffer + buf_pos, sizeof(var_buffer) - buf_pos, \
    "%s=%d,", #name, value)

// Specialized macro for character values
#define ADD_VAR_CHAR(name, value) \
    buf_pos += snprintf(var_buffer + buf_pos, sizeof(var_buffer) - buf_pos, \
    "%s=%c,", #name, value)

// Specialized macro for string values
#define ADD_VAR_STR(name, value) \
    buf_pos += snprintf(var_buffer + buf_pos, sizeof(var_buffer) - buf_pos, \
    "%s=%s,", #name, value)

// Generic macro that defaults to integer
#define ADD_VAR(name, value) ADD_VAR_INT(name, value)

// Function to ensure directories exist
void ensure_dirs_exist() {
    // Create data directory if it doesn't exist
    system("mkdir -p data");
    // Create projects directory if it doesn't exist
    system("mkdir -p projects");
    // Create debug directory if it doesn't exist
    system("mkdir -p '#.debug'");
}

// Function to copy a file from source to destination
bool copy_file(const char* src, const char* dest) {
    FILE* src_file = fopen(src, "r");
    if (!src_file) {
        return false;
    }
    
    FILE* dest_file = fopen(dest, "w");
    if (!dest_file) {
        fclose(src_file);
        return false;
    }
    
    char buffer[4096];
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dest_file) != bytes_read) {
            fclose(src_file);
            fclose(dest_file);
            return false;
        }
    }
    
    fclose(src_file);
    fclose(dest_file);
    return true;
}

// Function to save canvas state to projects directory
void save_project(const char* project_name) {
    char dest_path[MAX_PATH_LEN];
    
    // If project_name is empty or default, use "default" directory
    if (project_name == NULL || strlen(project_name) == 0 || strcmp(project_name, "default") == 0) {
        snprintf(dest_path, sizeof(dest_path), "projects/default");
    } else {
        snprintf(dest_path, sizeof(dest_path), "projects/%s", project_name);
    }
    
    // Create project directory
    char cmd[MAX_PATH_LEN + 50];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", dest_path);
    system(cmd);
    
    // Copy canvas state to project directory
    char src_path[] = "data/canvas_state.txt";
    char dest_file_path[MAX_PATH_LEN * 2]; // Make buffer larger to avoid overflow
    int dest_len = snprintf(dest_file_path, sizeof(dest_file_path), "%s/canvas_state.txt", dest_path);
    if (dest_len >= sizeof(dest_file_path)) {
        // Path would be truncated, handle error
        buf_pos = 0;
        ADD_VAR_STR(save_status, "path_too_long");
        ADD_VAR_STR(saved_project, project_name);
        printf("VARS:%s;\n", var_buffer);
        fflush(stdout);
        return;
    }
    
    if (copy_file(src_path, dest_file_path)) {
        // Log success
        FILE* debug_file = fopen("#.debug/quit.txt", "a");
        if (debug_file != NULL) {
            time_t current_time = time(NULL);
            fprintf(debug_file, "[%s] Project '%s' saved successfully\n", ctime(&current_time), project_name);
            fclose(debug_file);
        }
        
        // Output success variables and status message
        buf_pos = 0;
        ADD_VAR_STR(save_status, "success");
        ADD_VAR_STR(saved_project, project_name);
        ADD_VAR_STR(status_message, "Project saved successfully!");
        
        printf("VARS:%s;\n", var_buffer);
        printf("Project '%s' saved successfully!\n", project_name);
    } else {
        // Log failure
        FILE* debug_file = fopen("#.debug/quit.txt", "a");
        if (debug_file != NULL) {
            time_t current_time = time(NULL);
            fprintf(debug_file, "[%s] Failed to save project '%s'\n", ctime(&current_time), project_name);
            fclose(debug_file);
        }
        
        // Output failure variables and status message
        buf_pos = 0;
        ADD_VAR_STR(save_status, "failed");
        ADD_VAR_STR(saved_project, project_name);
        ADD_VAR_STR(status_message, "Failed to save project!");
        
        printf("VARS:%s;\n", var_buffer);
        printf("Error: Failed to save project '%s'!\n", project_name);
    }
    
    fflush(stdout);
}

// Function to save canvas state with a new name
void saveas_project(const char* project_name) {
    save_project(project_name);
}

// Function to load a project from projects directory
void load_project(const char* project_name) {
    char src_path[MAX_PATH_LEN];
    
    // If project_name is empty or default, use "default" directory
    if (project_name == NULL || strlen(project_name) == 0 || strcmp(project_name, "default") == 0) {
        snprintf(src_path, sizeof(src_path), "projects/default/canvas_state.txt");
    } else {
        snprintf(src_path, sizeof(src_path), "projects/%s/canvas_state.txt", project_name);
    }
    
    // Check if the project file exists
    if (access(src_path, F_OK) == 0) {
        // Copy project file to current canvas state
        if (copy_file(src_path, "data/canvas_state.txt")) {
            // Log success
            FILE* debug_file = fopen("#.debug/quit.txt", "a");
            if (debug_file != NULL) {
                time_t current_time = time(NULL);
                fprintf(debug_file, "[%s] Project '%s' loaded successfully\n", ctime(&current_time), project_name);
                fclose(debug_file);
            }
            
            // Output success variables
            buf_pos = 0;
            ADD_VAR_STR(load_status, "success");
            ADD_VAR_STR(loaded_project, project_name);
            
            printf("VARS:%s;\n", var_buffer);
        } else {
            // Log failure
            FILE* debug_file = fopen("#.debug/quit.txt", "a");
            if (debug_file != NULL) {
                time_t current_time = time(NULL);
                fprintf(debug_file, "[%s] Failed to load project '%s'\n", ctime(&current_time), project_name);
                fclose(debug_file);
            }
            
            // Output failure variables
            buf_pos = 0;
            ADD_VAR_STR(load_status, "failed");
            ADD_VAR_STR(loaded_project, project_name);
            
            printf("VARS:%s;\n", var_buffer);
        }
    } else {
        // File doesn't exist
        FILE* debug_file = fopen("#.debug/quit.txt", "a");
        if (debug_file != NULL) {
            time_t current_time = time(NULL);
            fprintf(debug_file, "[%s] Project '%s' does not exist\n", ctime(&current_time), project_name);
            fclose(debug_file);
        }
        
        // Output failure variables
        buf_pos = 0;
        ADD_VAR_STR(load_status, "not_found");
        ADD_VAR_STR(loaded_project, project_name);
        
        printf("VARS:%s;\n", var_buffer);
    }
    
    fflush(stdout);
}

// Function to list projects in the projects directory
void list_projects() {
    DIR* dir;
    struct dirent* entry;
    
    char project_names[MAX_PROJECTS][MAX_FILENAME_LEN];
    int project_count = 0;
    
    dir = opendir("projects");
    if (dir != NULL) {
        while ((entry = readdir(dir)) != NULL && project_count < MAX_PROJECTS) {
            // Skip . and .. entries
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                // Check if it's a directory (project folder)
                char full_path[MAX_PATH_LEN];
                snprintf(full_path, sizeof(full_path), "projects/%s", entry->d_name);
                
                struct stat path_stat;
                if (stat(full_path, &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
                    strncpy(project_names[project_count], entry->d_name, MAX_FILENAME_LEN - 1);
                    project_names[project_count][MAX_FILENAME_LEN - 1] = '\0';
                    project_count++;
                }
            }
        }
        closedir(dir);
    }
    
    // Output project list as variables
    buf_pos = 0;
    ADD_VAR_INT(project_count, project_count);
    ADD_VAR_STR(status_message, "Projects listed successfully!");
    
    // Output each project name as a variable
    for (int i = 0; i < project_count && i < 10; i++) {  // Limit to first 10 projects
        char var_name[50];
        snprintf(var_name, sizeof(var_name), "project_%d", i);
        ADD_VAR_STR(var_name, project_names[i]);
    }
    
    printf("VARS:%s;\n", var_buffer);
    printf("Found %d projects\n", project_count);
    
    // Also output the project names as lines for direct display
    for (int i = 0; i < project_count; i++) {
        printf("%s\n", project_names[i]);
    }
    
    fflush(stdout);
}

// Signal handler to clean up on termination
void signal_handler(int sig) {
    // Only set flags and use async-signal-safe functions
    g_sig_received = 1;
    g_sig_num = sig;
    
    // Write directly to file descriptor (async-signal-safe)
    char msg[256];
    int fd = open("#.debug/quit.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    
    if (fd != -1) {
        int len = snprintf(msg, sizeof(msg), "[Signal handler] Signal %d received\n", sig);
        write(fd, msg, len);
        close(fd);
    }
    
    // Don't do complex operations in signal handler - just set flag
    // Actual processing will happen in main loop
}

int main() {
    char input[512];
    
    // Debug log when module starts
    FILE* debug_file = fopen("#.debug/quit.txt", "a");
    if (debug_file != NULL) {
        time_t current_time = time(NULL);
        fprintf(debug_file, "[%s] File Operations Module Started\n", ctime(&current_time));
        fclose(debug_file);
    }
    
    // Ensure required directories exist
    ensure_dirs_exist();
    
    // Register signal handlers to terminate gracefully
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);
    
    // Send initial status message
    buf_pos = 0;
    ADD_VAR_STR(status_message, "File Operations Ready");
    printf("VARS:%s;\n", var_buffer);
    printf("File Operations Module Ready\n");
    fflush(stdout);
    
    // Main loop to process commands from parent process (CHTM renderer)
    while (fgets(input, sizeof(input), stdin) && !g_sig_received) {
        // Remove newline character
        input[strcspn(input, "\n")] = 0;
        
        // Check if a signal was received that requires termination
        if (g_sig_received) {
            break;
        }
        
        if (strcmp(input, "SAVE_DEFAULT") == 0 || strcmp(input, "SAVE") == 0) {
            // Save to default project
            save_project("default");  // Use "default" as project name
        }
        else if (strncmp(input, "SAVE:", 5) == 0) {
            // Parse save command: SAVE:project_name
            char *project_name = input + 5; // Skip "SAVE:"
            save_project(project_name);
        }
        else if (strncmp(input, "SAVEAS:", 7) == 0) {
            // Parse saveas command: SAVEAS:project_name
            char *project_name = input + 7; // Skip "SAVEAS:"
            saveas_project(project_name);
        }
        else if (strcmp(input, "LOAD_DEFAULT") == 0 || strcmp(input, "LOAD") == 0) {
            // Load from default project
            load_project("default");  // Use "default" as project name
        }
        else if (strncmp(input, "LOAD:", 5) == 0) {
            // Parse load command: LOAD:project_name
            char *project_name = input + 5; // Skip "LOAD:"
            load_project(project_name);
        }
        else if (strcmp(input, "LIST_PROJECTS") == 0) {
            // List all available projects
            list_projects();
        }
        else if (strcmp(input, "LOAD_PROJECTS_INNER") == 0) {
            // Test INNER_HTML functionality - load projects and update container
            DIR* dir;
            struct dirent* entry;
            char content_buffer[2048] = {0};
            
            strcat(content_buffer, "<text>Available Projects:</text><br/>\n");
            
            dir = opendir("projects");
            if (dir != NULL) {
                while ((entry = readdir(dir)) != NULL) {
                    // Skip . and .. entries
                    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                        // Check if it's a directory
                        char full_path[512];
                        snprintf(full_path, sizeof(full_path), "projects/%s", entry->d_name);
                        
                        struct stat path_stat;
                        if (stat(full_path, &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
                            char item[512];  // Increased size to prevent overflow
                            // Limit the displayed name to prevent buffer overflow
                            char safe_name[200];
                            strncpy(safe_name, entry->d_name, sizeof(safe_name) - 1);
                            safe_name[sizeof(safe_name) - 1] = '\0';
                            
                            snprintf(item, sizeof(item), 
                                "<button onClick=\"LOAD:%s\">%s</button><br/>\n", 
                                safe_name, safe_name);
                            strcat(content_buffer, item);
                        }
                    }
                }
                closedir(dir);
            } else {
                strcat(content_buffer, "<text>No projects directory found</text><br/>\n");
            }
            
            // Send INNER_HTML command to update the projects container
            printf("INNER_HTML:projects_container:%s\n", content_buffer);
            printf("VARS:load_status=success;\n");
            fflush(stdout);
        }
        else if (strcmp(input, "QUIT") == 0) {
            // Debug logging to file
            FILE* debug_file = fopen("#.debug/quit.txt", "a");
            if (debug_file != NULL) {
                time_t current_time = time(NULL);
                fprintf(debug_file, "[%s] Received QUIT command, exiting...\n", ctime(&current_time));
                fclose(debug_file);
            }
            
            printf("Received QUIT command, exiting...\n");
            break;
        }
        else {
            // Unknown command
            buf_pos = 0;
            ADD_VAR_STR(error, "unknown_command");
            ADD_VAR_STR(received_command, input);
            
            printf("VARS:%s;\n", var_buffer);
            fflush(stdout);
        }
    }
    
    // Final debug logging
    debug_file = fopen("#.debug/quit.txt", "a");
    if (debug_file != NULL) {
        time_t current_time = time(NULL);
        fprintf(debug_file, "[%s] File Operations Module Ending\n", ctime(&current_time));
        fclose(debug_file);
    }
    
    return 0;
}