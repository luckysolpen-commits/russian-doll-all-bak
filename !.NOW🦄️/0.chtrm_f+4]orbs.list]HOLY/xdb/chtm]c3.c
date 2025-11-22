#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <ctype.h>

// Constants
#define MAX_ELEMENTS 100
#define MAX_ATTR_LEN 256
#define MAX_PATH_LEN 512
#define MAX_LABEL_LEN 256

// UI Element types
typedef struct {
    char type[MAX_ATTR_LEN];    // "text", "link", "button", "panel"
    char label[MAX_LABEL_LEN];  // Display text
    char href[MAX_PATH_LEN];    // For links
    char onClick[MAX_ATTR_LEN]; // For buttons
    int key;                    // Numeric key (1-9)
    char id[MAX_ATTR_LEN];      // Element ID
} UIElement;

// IPC State
typedef struct {
    FILE* read_pipe;    // From module
    FILE* write_pipe;   // To module
    pid_t pid;          // Module process ID
    bool active;        // Whether IPC is active
} IPCState;

// Global state
UIElement elements[MAX_ELEMENTS];
int element_count = 0;
char current_file[MAX_PATH_LEN];
FILE* log_file = NULL;
IPCState ipc = {0};  // Initialize all fields to 0

// Function prototypes
char* read_file_to_string(const char* filename);
void parse_chtm_file(const char* content);
void parse_attributes(UIElement* el, const char* attr_str);
void render_elements();
int check_terminal_input();
bool process_input(int ch);
void switch_to_file(const char* filename);
void parse_chtm_for_module();
void launch_module();
void cleanup_ipc();
int send_ipc_command(const char* command);
int read_ipc_response(char* buffer, int buffer_size);

// Read file to string
char* read_file_to_string(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(length + 1);
    if (!buffer) {
        fprintf(stderr, "Memory allocation error\n");
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0';

    fclose(file);
    return buffer;
}

// Parse attributes from tag
void parse_attributes(UIElement* el, const char* attr_str) {
    // Make a copy of the attribute string to work with
    char* attrs = strdup(attr_str);
    char* pos = attrs;
    
    // Process each attribute
    while (*pos) {
        // Skip leading whitespace
        while (*pos && isspace(*pos)) pos++;
        if (!*pos) break;
        
        // Get attribute name start
        char* name_start = pos;
        
        // Find end of attribute name (at '=' or whitespace)
        while (*pos && *pos != '=' && !isspace(*pos)) pos++;
        if (pos == name_start) break;  // No name found
        
        // Terminate the name and save the next character
        char saved_char = *pos;
        *pos = '\0';
        
        // Skip whitespace and '=' to find value
        while (*(++pos) && isspace(*pos));  // Skip whitespace
        if (*pos == '=') {
            pos++;  // Skip '='
            // Skip whitespace after '='
            while (*pos && isspace(*pos)) pos++;
        }
        
        // Get value start
        char* value_start = pos;
        char quote_char = '\0';
        
        // Check if value is quoted
        if (*pos == '"' || *pos == '\'') {
            quote_char = *pos;
            pos++;  // Skip opening quote
            value_start = pos;
            
            // Find closing quote
            while (*pos && *pos != quote_char) pos++;
            
            if (*pos == quote_char) {
                *pos = '\0';  // Null terminate the value
                pos++;  // Move past closing quote
            }
        } else {
            // Unquoted value - stop at next whitespace or end of string
            while (*pos && !isspace(*pos) && *pos != '/') pos++;  // Stop at whitespace or end
            *pos = '\0';  // Null terminate the value
        }
        
        // Process the attribute name and value
        if (strcmp(name_start, "label") == 0) {
            strncpy(el->label, value_start, MAX_LABEL_LEN - 1);
        } else if (strcmp(name_start, "href") == 0) {
            strncpy(el->href, value_start, MAX_PATH_LEN - 1);
        } else if (strcmp(name_start, "onClick") == 0) {
            strncpy(el->onClick, value_start, MAX_ATTR_LEN - 1);
        } else if (strcmp(name_start, "key") == 0) {
            el->key = atoi(value_start);
        } else if (strcmp(name_start, "id") == 0) {
            strncpy(el->id, value_start, MAX_ATTR_LEN - 1);
        }
        
        // Restore the character after name
        *(pos - 1) = saved_char;  // Restore what was after the value
    }
    
    free(attrs);
}

// Parse CHTM content
void parse_chtm_file(const char* content) {
    element_count = 0;
    char* content_copy = strdup(content);
    char* cursor = content_copy;
    
    // Reset elements
    for (int i = 0; i < MAX_ELEMENTS; i++) {
        memset(&elements[i], 0, sizeof(UIElement));
    }
    
    while (*cursor) {
        // Find opening tag
        char* tag_start = strchr(cursor, '<');
        if (tag_start == NULL) break;
        
        // Find text content before tag if any
        if (tag_start > cursor) {
            // Process text content between tags
            char text_content[512];
            int len = tag_start - cursor;
            if (len < 512) {
                strncpy(text_content, cursor, len);
                text_content[len] = '\0';
                
                // Add as a text element if not just whitespace
                char* trimmed = text_content;
                while (*trimmed && isspace(*trimmed)) trimmed++;
                if (*trimmed) {
                    if (element_count < MAX_ELEMENTS) {
                        elements[element_count].key = -1;
                        strcpy(elements[element_count].type, "text");
                        strncpy(elements[element_count].label, trimmed, MAX_LABEL_LEN - 1);
                        element_count++;
                    }
                }
            }
        }
        
        // Find closing tag
        char* tag_end = strchr(tag_start, '>');
        if (tag_end == NULL) break;
        
        // Extract tag
        char tag[256];
        int tag_len = tag_end - (tag_start + 1);
        strncpy(tag, tag_start + 1, tag_len);
        tag[tag_len] = '\0';
        
        // Check if self-closing
        bool self_closing = false;
        if (tag[strlen(tag)-1] == '/') {
            self_closing = true;
            tag[strlen(tag)-1] = '\0';
        }
        
        // Handle closing tags
        if (tag[0] == '/') {
            cursor = tag_end + 1;
            continue;
        }
        
        // Extract tag name and attributes
        char tag_name[64];
        char* space_pos = strchr(tag, ' ');
        if (space_pos) {
            int name_len = space_pos - tag;
            if (name_len < 64) {
                strncpy(tag_name, tag, name_len);
                tag_name[name_len] = '\0';
                space_pos++; // Move past the space
            } else {
                strcpy(tag_name, "invalid"); // Tag name too long
                space_pos = "";
            }
        } else {
            strncpy(tag_name, tag, 63);
            tag_name[63] = '\0';
            space_pos = "";
        }
        
        // Process tag
        if (strcmp(tag_name, "text") == 0) {
            if (element_count < MAX_ELEMENTS) {
                elements[element_count].key = -1;
                strcpy(elements[element_count].type, "text");
                parse_attributes(&elements[element_count], space_pos);
                element_count++;
            }
        }
        else if (strcmp(tag_name, "link") == 0) {
            if (element_count < MAX_ELEMENTS) {
                elements[element_count].key = -1;
                strcpy(elements[element_count].type, "link");
                parse_attributes(&elements[element_count], space_pos);
                element_count++;
            }
        }
        else if (strcmp(tag_name, "button") == 0) {
            if (element_count < MAX_ELEMENTS) {
                elements[element_count].key = -1;
                strcpy(elements[element_count].type, "button");
                parse_attributes(&elements[element_count], space_pos);
                element_count++;
            }
        }
        else if (strcmp(tag_name, "panel") == 0) {
            // Panel is a container, just move to next tag; don't add as element
            cursor = tag_end + 1;
            continue;
        }
        
        cursor = tag_end + 1;
    }
    
    free(content_copy);
}

// Render UI elements
void render_elements() {
    printf("\033[H\033[J"); // Clear screen
    printf("Current file: %s\n\n", current_file);
    
    for (int i = 0; i < element_count; i++) {
        UIElement* el = &elements[i];
        
        if (strcmp(el->type, "text") == 0) {
            if (el->label[0] != '\0') {
                printf("%s\n", el->label);
            } else {
                // If no label, skip or print just a newline
                printf("\n");
            }
        }
        else if (strcmp(el->type, "link") == 0) {
            // Only show the link if it has a key or label
            if (el->key > 0 && el->label[0] != '\0') {
                printf("%d. [%s]\n", el->key, el->label);
            } else if (el->label[0] != '\0') {
                printf("[Link: %s]\n", el->label);
            } else if (el->key > 0) {
                printf("%d. [Link]\n", el->key);
            }
            // Skip links without either key or label
        }
        else if (strcmp(el->type, "button") == 0) {
            // Only show the button if it has a key or label
            if (el->key > 0 && el->label[0] != '\0') {
                printf("%d. [%s]\n", el->key, el->label);
            } else if (el->label[0] != '\0') {
                printf("[Button: %s]\n", el->label);
            } else if (el->key > 0) {
                printf("%d. [Button]\n", el->key);
            }
            // Skip buttons without either key or label
        }
        else if (strcmp(el->type, "panel") == 0) {
            // Panel is just a container, may add visual separation later
            // Don't print panel elements as visible items
        }
    }
    
    printf("\nEnter key (1-9) or 'q' to quit: ");
    fflush(stdout);
}

// Global input buffer to manually handle all input
char manual_input_buffer[256];
int manual_input_pos = 0;
bool manual_echo_enabled = true;

// Check terminal input with non-blocking read - no echo from terminal
int check_terminal_input() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); // Disable both canonical and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    fd_set set;
    struct timeval timeout;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    int ready = select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);
    int ch = -1;
    
    if (ready > 0) {
        ch = getchar();
        
        // Handle escape sequences for arrow keys
        if (ch == 27) {
            int ch2 = getchar();
            if (ch2 == 91) { // '[' character
                int ch3 = getchar();
                switch (ch3) {
                    case 65: ch = 1000; break; // Up arrow
                    case 66: ch = 1001; break; // Down arrow
                    case 67: ch = 1002; break; // Right arrow
                    case 68: ch = 1003; break; // Left arrow
                }
            }
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

// Log key press to commands.txt
void log_key_press(int ch) {
    if (!log_file) {
        log_file = fopen("commands.txt", "w");
        if (!log_file) {
            perror("Error opening log file");
            return;
        }
    }
    
    fprintf(log_file, "Key pressed: %d ('%c')\n", ch, isprint(ch) ? ch : '?');
    fflush(log_file);
}

// Input buffer for backspace functionality
char input_buffer[256] = {0};
int input_buffer_pos = 0;

// Add to input buffer
void add_to_input_buffer(char ch) {
    if (input_buffer_pos < 255) {  // Leave space for null terminator
        input_buffer[input_buffer_pos] = ch;
        input_buffer_pos++;
        input_buffer[input_buffer_pos] = '\0';
    }
}

// Remove from input buffer (backspace)
void backspace_input_buffer() {
    if (input_buffer_pos > 0) {
        input_buffer_pos--;
        input_buffer[input_buffer_pos] = '\0';
    }
}

// Clear input buffer
void clear_input_buffer() {
    input_buffer[0] = '\0';
    input_buffer_pos = 0;
}

// Process input
bool process_input(int ch) {
    // Check if this is a special key which should not be manually echoed
    bool is_special_key = (ch == '\r' || ch == '\n' || 
                          ch == 1000 || ch == 1001 ||  // KEY_UP, KEY_DOWN
                          ch == 1002 || ch == 1003);   // KEY_LEFT, KEY_RIGHT
    
    log_key_press(ch);
    
    // Handle backspace to visually erase the last character and remove from buffer
    if (ch == 127 || ch == 8) {
        if (input_buffer_pos > 0) {
            printf("\b \b"); // Backspace, space to clear, backspace again
            backspace_input_buffer(); // Remove from buffer
        }
        fflush(stdout);
        return false; // No redraw needed for just visual backspace
    }
    // Handle Enter key - if input buffer has content, check if it's a single digit for menu selection, otherwise evaluate
    else if (ch == '\r' || ch == '\n') {
        if (input_buffer_pos > 0) {
            // Check if the input buffer contains a single digit that matches a navigation key
            if (input_buffer_pos == 1 && isdigit(input_buffer[0])) {
                int selected_key = input_buffer[0] - '0';
                
                // Check if this digit corresponds to a menu item
                bool found = false;
                for (int i = 0; i < element_count; i++) {
                    if (elements[i].key == selected_key) {
                        if (strcmp(elements[i].type, "link") == 0) {
                            printf("\rNavigating to: %s\n", elements[i].href);
                            switch_to_file(elements[i].href);
                            char msg[600];
                            sprintf(msg, "Navigating to %s...", elements[i].href);
                            
                            if (ipc.active) { 
                                cleanup_ipc(); 
                            }
                            
                            clear_input_buffer(); // Clear the buffer after navigation
                            return true;
                        } else if (strcmp(elements[i].type, "button") == 0) {
                            char msg[300];
                            sprintf(msg, "Button clicked: %s", elements[i].onClick);
                            printf("\r%s\n", msg);
                            // In the future, a command processor would go here.
                            clear_input_buffer(); // Clear the buffer after processing
                            return true; // Update screen to show status message
                        }
                        found = true;
                        break;
                    }
                }
                
                // If the single digit doesn't correspond to a menu item, evaluate it as input
                if (!found) {
                    printf("\nProcessed input: %s\n", input_buffer); // Process the input buffer
                    clear_input_buffer();
                    return true; // Redraw needed to show status message
                }
            } else {
                // Multi-character input or non-digit single character, evaluate as input
                printf("\nProcessed input: %s\n", input_buffer); // Process the input buffer
                clear_input_buffer();
                return true; // Redraw needed to show status message
            }
        } else {
            // If no input buffer content, treat as enter on empty line
            printf("\n");
            fflush(stdout);
        }
    }
    // Manually echo printable characters (letters, numbers, symbols) except special keys
    else if (!is_special_key && ch >= 32 && ch <= 126) {
        printf("%c", ch); // Manually echo the character to terminal
        add_to_input_buffer((char)ch); // Add to buffer
        
        // If it's a digit, just let it be added to the buffer like any other character
        // The actual navigation will happen when Enter is pressed
        if (isdigit(ch)) {
            int selected_key = ch - '0';
            
            bool found = false;
            for (int i = 0; i < element_count; i++) {
                if (elements[i].key == selected_key) {
                    if (strcmp(elements[i].type, "link") == 0) {
                        char msg[300];
                        sprintf(msg, "%s selected. Press Enter to navigate.", elements[i].label);
                        printf(" %s", msg);  // Show message as additional text
                        found = true;
                        break;
                    } else if (strcmp(elements[i].type, "button") == 0) {
                        char msg[300];
                        sprintf(msg, "%s selected. Press Enter to action.", elements[i].label);
                        printf(" %s", msg);  // Show message as additional text
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                // Not a navigation key, just add to buffer
            }
        }
        
        fflush(stdout);
    }
    else {
        // Handle other keys (arrows, etc.)
        switch (ch) {
            case 1000: // Up arrow
                printf("\rUp arrow pressed\n");
                render_elements();
                break;
            case 1001: // Down arrow
                printf("\rDown arrow pressed\n");
                render_elements();
                break;
            case 1002: // Right arrow
                printf("\rRight arrow pressed\n");
                render_elements();
                break;
            case 1003: // Left arrow
                printf("\rLeft arrow pressed\n");
                render_elements();
                break;
            case 'q':
            case 'Q':
                printf("\rQuitting...\n");
                exit(0);
                break;
            case 27: // ESC
                printf("\rESC pressed\n");
                break;
            default:
                if (!isprint(ch)) {
                    // For non-printable characters, show info
                    printf("\rNon-printable key %d pressed\n", ch);
                    render_elements();
                }
                break;
        }
        fflush(stdout);
    }
    
    return false;
}

// IPC module path storage
char module_exec_path[MAX_PATH_LEN] = {0};

// Parse CHTM file for module tag
void parse_chtm_for_module() {
    char* content = read_file_to_string(current_file);
    if (!content) {
        module_exec_path[0] = '\0';
        return;
    }

    // Find module path
    char* module_start = strstr(content, "<module>");
    if (module_start) {
        char* path_start = module_start + strlen("<module>");
        char* module_end = strstr(path_start, "</module>");
        if (module_end) {
            int path_len = module_end - path_start;
            if (path_len < MAX_PATH_LEN) {
                strncpy(module_exec_path, path_start, path_len);
                module_exec_path[path_len] = '\0';
                
                // Trim whitespace
                char* end = module_exec_path + strlen(module_exec_path) - 1;
                while (end >= module_exec_path && isspace(*end)) {
                    *end-- = '\0';
                }
            }
        }
    } else {
        module_exec_path[0] = '\0';
    }
    
    free(content);
}

// Launch module process with IPC
void launch_module() {
    if (module_exec_path[0] == '\0') {
        // No module to launch
        ipc.active = false;
        return;
    }

    int parent_to_child[2];
    int child_to_parent[2];

    if (pipe(parent_to_child) == -1 || pipe(child_to_parent) == -1) {
        perror("pipe failed");
        return;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        close(parent_to_child[0]);
        close(parent_to_child[1]);
        close(child_to_parent[0]);
        close(child_to_parent[1]);
        return;
    }

    if (pid == 0) { // Child process
        close(parent_to_child[1]); // Close write end of parent->child pipe
        close(child_to_parent[0]); // Close read end of child->parent pipe

        // Redirect stdin and stdout
        dup2(parent_to_child[0], STDIN_FILENO);
        dup2(child_to_parent[1], STDOUT_FILENO);

        // Close the original pipe file descriptors
        close(parent_to_child[0]);
        close(child_to_parent[1]);

        // Execute the module
        execl(module_exec_path, module_exec_path, (char *)NULL);
        
        // If execl returns, it must have failed
        perror("execl failed");
        exit(1);
    }
    else { // Parent process
        close(parent_to_child[0]); // Close read end of parent->child pipe
        close(child_to_parent[1]); // Close write end of child->parent pipe

        // Get FILE* streams for easy reading/writing
        ipc.write_pipe = fdopen(parent_to_child[1], "w");
        ipc.read_pipe = fdopen(child_to_parent[0], "r");
        ipc.pid = pid;
        ipc.active = true;
    }
}

// Send command through IPC
int send_ipc_command(const char* command) {
    if (!ipc.active || !ipc.write_pipe) {
        return -1;
    }
    
    fprintf(ipc.write_pipe, "%s\n", command);
    fflush(ipc.write_pipe);
    return 0;
}

// Read response from IPC
int read_ipc_response(char* buffer, int buffer_size) {
    if (!ipc.active || !ipc.read_pipe) {
        return -1;
    }
    
    if (fgets(buffer, buffer_size, ipc.read_pipe) == NULL) {
        return -1;
    }
    
    return 0;
}

// Cleanup IPC resources
void cleanup_ipc() {
    if (ipc.write_pipe) {
        fclose(ipc.write_pipe);
        ipc.write_pipe = NULL;
    }
    
    if (ipc.read_pipe) {
        fclose(ipc.read_pipe);
        ipc.read_pipe = NULL;
    }
    
    if (ipc.pid > 0) {
        int status;
        waitpid(ipc.pid, &status, WNOHANG); // Don't block waiting
        ipc.pid = 0;
    }
    
    ipc.active = false;
}

// Switch to a different CHTM file
void switch_to_file(const char* filename) {
    // Clean up existing IPC before switching
    if (ipc.active) {
        cleanup_ipc();
    }
    
    strncpy(current_file, filename, MAX_PATH_LEN - 1);
    current_file[MAX_PATH_LEN - 1] = '\0';
    
    char* content = read_file_to_string(current_file);
    if (content) {
        parse_chtm_file(content);
        free(content);
        
        // Check for module and launch if found
        parse_chtm_for_module();
        if (module_exec_path[0] != '\0') {
            launch_module();
        }
    } else {
        printf("Error: Could not read file %s\n", current_file);
    }
}

// Main function
int main(int argc, char** argv) {
    // Get the CHTM file from command line argument or use default
    if (argc > 1) {
        strncpy(current_file, argv[1], MAX_PATH_LEN - 1);
        current_file[MAX_PATH_LEN - 1] = '\0';
    } else {
        strncpy(current_file, "chtm/index.chtm", MAX_PATH_LEN - 1);
        current_file[MAX_PATH_LEN - 1] = '\0';
    }
    
    // Initialize terminal - disable both ICANON and ECHO since we'll handle manually
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO); // Disable canonical and echo, we'll handle manually
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    
    // Read and parse the initial file
    char* content = read_file_to_string(current_file);
    if (content) {
        parse_chtm_file(content);
        free(content);
        
        // Check for module and launch if found
        parse_chtm_for_module();
        if (module_exec_path[0] != '\0') {
            launch_module();
        }
    } else {
        printf("Error: Could not read initial file %s\n", current_file);
        return 1;
    }
    
    // Main loop
    printf("CHTM renderer started. Press 'q' to quit.\n");
    render_elements();
    
    while (1) {
        int ch = check_terminal_input();
        if (ch != -1) {
            bool redraw = process_input(ch);
            if (redraw) {
                render_elements();
            }
        }
        usleep(50000); // 50ms delay to reduce CPU usage
    }
    
    // Cleanup
    cleanup_ipc();
    if (log_file) {
        fclose(log_file);
    }
    
    return 0;
}