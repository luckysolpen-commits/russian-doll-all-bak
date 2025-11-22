#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdarg.h>

// Constants
#define MAX_ELEMENTS 100
#define MAX_ATTR_LEN 256
#define MAX_PATH_LEN 512
#define MAX_LABEL_LEN 256

// UI Element types
typedef struct {
    char type[MAX_ATTR_LEN];    // "text", "link", "button", "panel", "canvas"
    char label[MAX_LABEL_LEN];  // Display text
    char href[MAX_PATH_LEN];    // For links
    char onClick[MAX_ATTR_LEN]; // For buttons
    int key;                    // Numeric key (1-9)
    char id[MAX_ATTR_LEN];      // Element ID
    int width;                  // Canvas width
    int height;                 // Canvas height
    char color[MAX_ATTR_LEN];   // Canvas color
} UIElement;

// IPC State
typedef struct {
    FILE* read_pipe;    // From module
    FILE* write_pipe;   // To module
    pid_t pid;          // Module process ID
    bool active;        // Whether IPC is active
} IPCState;

// Variable storage system
#define MAX_VARS 50
#define MAX_VAR_NAME_LEN 64
#define MAX_VAR_VALUE_LEN 256

typedef struct {
    char name[MAX_VAR_NAME_LEN];
    char value[MAX_VAR_VALUE_LEN];
    bool active;
} Variable;

Variable variables[MAX_VARS];

// Global state
UIElement elements[MAX_ELEMENTS];
int element_count = 0;
char current_file[MAX_PATH_LEN];
FILE* log_file = NULL;
FILE* debug_file = NULL;  // Debug log file
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
void initialize_variables();
void set_variable(const char* name, const char* value);
char* get_variable(const char* name);
void substitute_variables_in_string(const char* src, char* dst, size_t dst_size);
void debug_log(const char* format, ...);
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
        } else if (strcmp(name_start, "width") == 0) {
            el->width = atoi(value_start);
        } else if (strcmp(name_start, "height") == 0) {
            el->height = atoi(value_start);
        } else if (strcmp(name_start, "color") == 0) {
            strncpy(el->color, value_start, MAX_ATTR_LEN - 1);
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
        else if (strcmp(tag_name, "canvas") == 0) {
            if (element_count < MAX_ELEMENTS) {
                elements[element_count].key = -1;
                strcpy(elements[element_count].type, "canvas");
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
                debug_log("Processing text element with label: %s", el->label);
                // Process variable substitution in the label
                char processed_label[MAX_LABEL_LEN * 2]; // Allow for expansion
                substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                debug_log("After variable substitution: %s", processed_label);
                printf("%s\n", processed_label);
            } else {
                // If no label, skip or print just a newline
                printf("\n");
            }
        }
        else if (strcmp(el->type, "link") == 0) {
            // Only show the link if it has a key or label
            if (el->key > 0 && el->label[0] != '\0') {
                char processed_label[MAX_LABEL_LEN * 2];
                substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                printf("%d. [%s]\n", el->key, processed_label);
            } else if (el->label[0] != '\0') {
                char processed_label[MAX_LABEL_LEN * 2];
                substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                printf("[Link: %s]\n", processed_label);
            } else if (el->key > 0) {
                printf("%d. [Link]\n", el->key);
            }
            // Skip links without either key or label
        }
        else if (strcmp(el->type, "button") == 0) {
            // Only show the button if it has a key or label
            if (el->key > 0 && el->label[0] != '\0') {
                char processed_label[MAX_LABEL_LEN * 2];
                substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                printf("%d. [%s]\n", el->key, processed_label);
            } else if (el->label[0] != '\0') {
                char processed_label[MAX_LABEL_LEN * 2];
                substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                printf("[Button: %s]\n", processed_label);
            } else if (el->key > 0) {
                printf("%d. [Button]\n", el->key);
            }
            // Skip buttons without either key or label
        }
        else if (strcmp(el->type, "panel") == 0) {
            // Panel is just a container, may add visual separation later
            // Don't print panel elements as visible items
        }

        else if (strcmp(el->type, "canvas") == 0) {
            if (el->width > 0 && el->height > 0) {
                // Print the canvas with its label (with variable substitution)
                if (el->label[0] != '\0') {
                    char processed_label[MAX_LABEL_LEN * 2];
                    substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                    printf("%s\n", processed_label);
                }
                
                // Check if color is specified and apply ANSI color code
                if (el->color[0] != '\0' && strlen(el->color) > 1 && el->color[0] == '#') {
                    // Convert hex color to ANSI if possible, or use a default color mapping
                    if (strcmp(el->color, "#CCCCCC") == 0) {
                        printf("\033[37m"); // Light gray
                    } else if (strcmp(el->color, "#FF0000") == 0) {
                        printf("\033[31m"); // Red
                    } else if (strcmp(el->color, "#00FF00") == 0) {
                        printf("\033[32m"); // Green
                    } else if (strcmp(el->color, "#0000FF") == 0) {
                        printf("\033[34m"); // Blue
                    } else if (strcmp(el->color, "#FFFF00") == 0) {
                        printf("\033[33m"); // Yellow
                    } else if (strcmp(el->color, "#00FFFF") == 0) {
                        printf("\033[36m"); // Cyan
                    } else if (strcmp(el->color, "#FF00FF") == 0) {
                        printf("\033[35m"); // Magenta
                    } else if (strcmp(el->color, "#000000") == 0) {
                        printf("\033[30m"); // Black
                    } else if (strcmp(el->color, "#FFFFFF") == 0) {
                        printf("\033[37m"); // White
                    }
                }
                
                // If IPC is active, try to get updated canvas from module
                if (ipc.active) {
                    send_ipc_command("DISPLAY");
                    
                    // Read the canvas data from the module
                    char response[1024];
                    int lines_read = 0;
                    int total_lines = 0;
                    
                    // Read responses until we have read the canvas lines
                    while (total_lines < (el->height + 5)) { // canvas height + some extra for potential position data
                        if (read_ipc_response(response, sizeof(response)) == 0) {
                            // Remove newline from response
                            response[strcspn(response, "\n")] = 0;
                            
                            // Check if this is a variable update
                            if (strncmp(response, "VAR:", 4) == 0) {
                                debug_log("Received VAR response: %s", response);
                                // Format: VAR:name=value
                                char *var_part = response + 4; // Skip "VAR:"
                                char *equals = strchr(var_part, '=');
                                if (equals) {
                                    *equals = '\0';
                                    char *name = var_part;
                                    char *value = equals + 1;
                                    set_variable(name, value);
                                } else {
                                    debug_log("ERROR: Invalid VAR format: %s", response);
                                }
                            } else if (lines_read < el->height) {
                                // Check if this looks like a canvas line (has the right format: chars with spaces)
                                // A canvas line should have the pattern "x x x x ..." where x is the character and there are spaces
                                int space_count = 0;
                                int valid_char_count = 0;  // Count characters that are likely part of a canvas ('p', '.', numbers, letters)
                                
                                for (int j = 0; response[j] != '\0'; j++) {
                                    if (response[j] == ' ') {
                                        space_count++;
                                    } else if (response[j] == '.' || response[j] == 'p' || response[j] == 'X' || response[j] == 'O' || 
                                              (response[j] >= '0' && response[j] <= '9')) {
                                        // Count characters that are typically used in canvas displays
                                        valid_char_count++;
                                    }
                                }
                                
                                // For a canvas of width N, we should have N characters and N-1 spaces between them
                                // Also, most of the content should consist of valid canvas characters, not regular text
                                if (space_count >= el->width - 1 && valid_char_count >= el->width) {
                                    // This looks like a canvas line
                                    printf("%s\n", response);
                                    lines_read++;
                                } else {
                                    debug_log("Skipping non-canvas response: %s", response);
                                }
                            } else {
                                debug_log("Received other response: %s", response);
                            }
                        } else {
                            // No more data available
                            break;
                        }
                        total_lines++;
                        
                        if (lines_read >= el->height) break;
                    }
                } else {
                    // Draw the canvas grid with placeholder
                    for (int r = 0; r < el->height; r++) {
                        for (int c = 0; c < el->width; c++) {
                            printf(". ");
                        }
                        printf("\n");
                    }
                }
                
                // Reset color
                if (el->color[0] != '\0') {
                    printf("\033[0m");
                }
            }
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
                if (ipc.active) {
                    send_ipc_command("MOVE:U");
                }
                printf("\rUp arrow pressed (player moved up)\n");
                printf("Enter key (1-9) or 'q' to quit: ");
                fflush(stdout);
                return true; // Redraw to update canvas
            case 1001: // Down arrow
                if (ipc.active) {
                    send_ipc_command("MOVE:D");
                }
                printf("\rDown arrow pressed (player moved down)\n");
                printf("Enter key (1-9) or 'q' to quit: ");
                fflush(stdout);
                return true; // Redraw to update canvas
            case 1002: // Right arrow
                if (ipc.active) {
                    send_ipc_command("MOVE:R");
                }
                printf("\rRight arrow pressed (player moved right)\n");
                printf("Enter key (1-9) or 'q' to quit: ");
                fflush(stdout);
                return true; // Redraw to update canvas
            case 1003: // Left arrow
                if (ipc.active) {
                    send_ipc_command("MOVE:L");
                }
                printf("\rLeft arrow pressed (player moved left)\n");
                printf("Enter key (1-9) or 'q' to quit: ");
                fflush(stdout);
                return true; // Redraw to update canvas
            case 'q':
            case 'Q':
                printf("\rQuitting...\n");
                exit(0);
                break;
            case 27: // ESC
                printf("\rESC pressed\n");
                printf("Enter key (1-9) or 'q' to quit: ");
                fflush(stdout);
                break;
            default:
                if (!isprint(ch)) {
                    // For non-printable characters, show info
                    printf("\rNon-printable key %d pressed\n", ch);
                    printf("Enter key (1-9) or 'q' to quit: ");
                    fflush(stdout);
                }
                break;
        }
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

    // Find module path - first try the <module>...</module> format
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
        debug_log("No module to launch for current file");
        ipc.active = false;
        return;
    }

    debug_log("Launching module: %s", module_exec_path);

    int parent_to_child[2];
    int child_to_parent[2];

    if (pipe(parent_to_child) == -1 || pipe(child_to_parent) == -1) {
        perror("pipe failed");
        debug_log("Pipe creation failed");
        return;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        debug_log("Fork failed");
        close(parent_to_child[0]);
        close(parent_to_child[1]);
        close(child_to_parent[0]);
        close(child_to_parent[1]);
        return;
    }

    if (pid == 0) { // Child process
        debug_log("In child process, executing module: %s", module_exec_path);
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
        debug_log("execl failed for: %s", module_exec_path);
        exit(1);
    }
    else { // Parent process
        debug_log("Module launched successfully, PID: %d", pid);
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
        debug_log("Failed to send IPC command (not active or no write pipe): %s", command);
        return -1;
    }
    
    debug_log("Sending IPC command: %s", command);
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

// Initialize all variables to inactive
void initialize_variables() {
    for (int i = 0; i < MAX_VARS; i++) {
        variables[i].active = false;
        variables[i].name[0] = '\0';
        variables[i].value[0] = '\0';
    }
}

// Set a variable by name and value
void set_variable(const char* name, const char* value) {
    debug_log("Setting variable: %s = %s", name, value);
    
    // Look for existing variable with this name
    for (int i = 0; i < MAX_VARS; i++) {
        if (variables[i].active && strcmp(variables[i].name, name) == 0) {
            strncpy(variables[i].value, value, MAX_VAR_VALUE_LEN - 1);
            variables[i].value[MAX_VAR_VALUE_LEN - 1] = '\0';
            debug_log("Updated existing variable %s to %s", name, value);
            return;
        }
    }
    
    // If not found, look for an inactive slot to create new variable
    for (int i = 0; i < MAX_VARS; i++) {
        if (!variables[i].active) {
            strncpy(variables[i].name, name, MAX_VAR_NAME_LEN - 1);
            variables[i].name[MAX_VAR_NAME_LEN - 1] = '\0';
            strncpy(variables[i].value, value, MAX_VAR_VALUE_LEN - 1);
            variables[i].value[MAX_VAR_VALUE_LEN - 1] = '\0';
            variables[i].active = true;
            debug_log("Created new variable %s with value %s", name, value);
            return;
        }
    }
    
    // If all slots are full, warn and return
    fprintf(stderr, "Warning: Variable storage full, cannot create variable '%s'\n", name);
    debug_log("ERROR: Variable storage full, could not create %s", name);
}

// Get a variable value by name
char* get_variable(const char* name) {
    for (int i = 0; i < MAX_VARS; i++) {
        if (variables[i].active && strcmp(variables[i].name, name) == 0) {
            return variables[i].value;
        }
    }
    return NULL; // Variable not found
}

// Substitute variables in a source string
// Variables in format ${var_name} will be replaced with their values
void substitute_variables_in_string(const char* src, char* dst, size_t dst_size) {
    const char* src_ptr = src;
    char* dst_ptr = dst;
    size_t remaining = dst_size - 1; // Leave space for null terminator
    
    while (*src_ptr && remaining > 0) {
        // Look for variable pattern ${...}
        if (*src_ptr == '$' && *(src_ptr + 1) == '{') {
            const char* var_start = src_ptr + 2; // Skip "${"
            const char* var_end = strchr(var_start, '}');
            
            if (var_end) {
                // Extract variable name
                char var_name[MAX_VAR_NAME_LEN];
                int var_name_len = var_end - var_start;
                if (var_name_len >= MAX_VAR_NAME_LEN) {
                    var_name_len = MAX_VAR_NAME_LEN - 1;
                }
                strncpy(var_name, var_start, var_name_len);
                var_name[var_name_len] = '\0';
                
                // Get variable value
                char* var_value = get_variable(var_name);
                if (var_value) {
                    size_t value_len = strlen(var_value);
                    if (value_len <= remaining) {
                        strncpy(dst_ptr, var_value, value_len);
                        dst_ptr += value_len;
                        remaining -= value_len;
                        src_ptr = var_end + 1; // Skip to after "}"
                    } else {
                        // Not enough space, copy what fits
                        strncpy(dst_ptr, var_value, remaining);
                        dst_ptr += remaining;
                        remaining = 0;
                        break;
                    }
                } else {
                    // Variable not found, keep the original pattern
                    size_t pattern_len = var_end - src_ptr + 1; // Include '}'
                    if (pattern_len <= remaining) {
                        strncpy(dst_ptr, src_ptr, pattern_len);
                        dst_ptr += pattern_len;
                        remaining -= pattern_len;
                        src_ptr = var_end + 1;
                    } else {
                        // Not enough space, copy what fits
                        strncpy(dst_ptr, src_ptr, remaining);
                        dst_ptr += remaining;
                        remaining = 0;
                        break;
                    }
                }
            } else {
                // No closing }, copy the $ character
                *dst_ptr = *src_ptr;
                dst_ptr++;
                remaining--;
                src_ptr++;
            }
        } else {
            // Regular character, copy it
            *dst_ptr = *src_ptr;
            dst_ptr++;
            remaining--;
            src_ptr++;
        }
    }
    
    *dst_ptr = '\0'; // Null terminate
}

// Debug logging function
void debug_log(const char* format, ...) {
    if (!debug_file) {
        debug_file = fopen("debug.log", "w");
        if (!debug_file) {
            perror("Error opening debug log file");
            return;
        }
    }
    
    va_list args;
    va_start(args, format);
    vfprintf(debug_file, format, args);
    va_end(args);
    fprintf(debug_file, "\n");
    fflush(debug_file);
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
    
    // Reset all variables when cleaning up
    initialize_variables();
}

// Switch to a different CHTM file
void switch_to_file(const char* filename) {
    debug_log("Switching to file: %s", filename);
    
    // Clean up existing IPC before switching
    if (ipc.active) {
        debug_log("Cleaning up existing IPC before switching");
        cleanup_ipc();
    } else {
        debug_log("No active IPC to clean up");
    }
    
    strncpy(current_file, filename, MAX_PATH_LEN - 1);
    current_file[MAX_PATH_LEN - 1] = '\0';
    
    char* content = read_file_to_string(current_file);
    if (content) {
        parse_chtm_file(content);
        free(content);
        
        // Check for module and launch if found
        parse_chtm_for_module();
        debug_log("Module path after parsing: %s", module_exec_path[0] != '\0' ? module_exec_path : "(none)");
        
        if (module_exec_path[0] != '\0') {
            launch_module();
            
            // Send canvas size information to module if canvas elements exist
            for (int i = 0; i < element_count; i++) {
                if (strcmp(elements[i].type, "canvas") == 0 && elements[i].width > 0 && elements[i].height > 0) {
                    char size_cmd[64];
                    snprintf(size_cmd, sizeof(size_cmd), "SIZE:%d,%d", elements[i].width, elements[i].height);
                    debug_log("Sending size command: %s", size_cmd);
                    if (ipc.active) {
                        send_ipc_command(size_cmd);
                    }
                    break; // Send size for first canvas found
                }
            }
        } else {
            debug_log("No module found for this file");
        }
    } else {
        printf("Error: Could not read file %s\n", current_file);
        debug_log("ERROR: Could not read file %s", current_file);
    }
}

// Main function
int main(int argc, char** argv) {
    // Initialize debug logging
    debug_file = fopen("debug.log", "w");
    if (debug_file) {
        fprintf(debug_file, "=== CHTM Renderer Started ===\n");
        fflush(debug_file);
    }
    
    // Get the CHTM file from command line argument or use default
    if (argc > 1) {
        strncpy(current_file, argv[1], MAX_PATH_LEN - 1);
        current_file[MAX_PATH_LEN - 1] = '\0';
    } else {
        strncpy(current_file, "chtm/index.chtm", MAX_PATH_LEN - 1);
        current_file[MAX_PATH_LEN - 1] = '\0';
    }
    
    debug_log("Starting with file: %s", current_file);
    
    // Initialize variables
    initialize_variables();
    
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
    
    if (debug_file) {
        fprintf(debug_file, "=== CHTM Renderer Ending ===\n");
        fclose(debug_file);
    }
    
    return 0;
}