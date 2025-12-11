/*
 * =====================
 * MODEL (M) - Data Storage and Management
 * =====================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <ctype.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

// Key codes from reference implementation
#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN,
    ESC_KEY                     // dedicated Escape key
};

// Constants
#define MAX_ELEMENTS 100
#define MAX_ATTR_LEN 256
#define MAX_PATH_LEN 512
#define MAX_LABEL_LEN 256

// Constants
#define MAX_ELEMENTS 100
#define MAX_ATTR_LEN 256
#define MAX_PATH_LEN 512
#define MAX_LABEL_LEN 256

// UI Element types
typedef struct {
    char type[MAX_ATTR_LEN];    // "text", "link", "button", "panel", "canvas", "cli_io"
    char label[MAX_LABEL_LEN];  // Display text
    char href[MAX_PATH_LEN];    // For links
    char onClick[MAX_ATTR_LEN]; // For buttons
    int key;                    // Numeric key (1-9) - DEPRECATED: Use array index instead
    char id[MAX_ATTR_LEN];      // Element ID
    int width;                  // Canvas width
    int height;                 // Canvas height
    char color[MAX_ATTR_LEN];   // Canvas color
    bool focused;               // Whether this element currently has focus
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

// Canvas data storage for Model
#define MAX_CANVAS_WIDTH 50
#define MAX_CANVAS_HEIGHT 50

typedef struct {
    char grid[MAX_CANVAS_HEIGHT][MAX_CANVAS_WIDTH * 2]; // *2 to account for spaces between chars
    int height;
    int width;
    bool valid; // Whether this canvas data is current/valid
    char id[MAX_ATTR_LEN]; // ID of the canvas this data belongs to
} CanvasData;

CanvasData model_canvas_data[MAX_ELEMENTS]; // Store canvas data for each canvas element
int model_canvas_count = 0;

// Global variables for terminal settings restoration
struct termios orig_termios;

// Global state
UIElement elements[MAX_ELEMENTS];
int element_count = 0;
int focused_element_index = -1;  // Index of currently focused element (-1 if none)
int active_interaction_element = -1; // Index of element in interaction mode [^] (-1 if none)
char current_file[MAX_PATH_LEN];
FILE* log_file = NULL;
FILE* debug_file = NULL;  // Debug log file
IPCState ipc = {0};  // Initialize all fields to 0

// Navigation input buffer for numeric element addressing
char nav_input_buffer[256] = {0};
int nav_input_pos = 0;
bool nav_input_active = false;  // Whether navigation input is currently active


/* 
 * =====================
 * VIEW (V) - Display Rendering
 * =====================
 */

// Function prototypes
char* read_file_to_string(const char* filename);
char* remove_comments_from_chtm(const char* content);
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
void log_frame_to_debug_file();
void add_to_render_buffer(const char* format, ...);
void capture_rendered_frame();
int find_next_interactive_element(int start_index);
int find_prev_interactive_element(int start_index);
void set_focus(int element_index);
void initialize_focus();
int send_ipc_command(const char* command);
int read_ipc_response(char* buffer, int buffer_size);
void process_module_responses();

/*
 * =====================
 * MODEL (M) - Data Management Functions
 * =====================
 */

// Model functions for canvas data management
void model_init_canvas_data(int canvas_idx, int width, int height, const char* id);
void model_update_canvas_data(int canvas_idx, int line_num, const char* line_data);
CanvasData* model_get_canvas_data(const char* canvas_id);
CanvasData* model_get_canvas_data_by_index(int canvas_idx);

// Remove HTML-style comments from CHTM content
char* remove_comments_from_chtm(const char* content) {
    if (!content) return NULL;
    
    size_t content_len = strlen(content);
    char* result = malloc(content_len + 1);  // Allocate enough space
    if (!result) return NULL;
    
    strcpy(result, content);  // Start with a copy of the original content
    
    char* search_pos = result;
    while ((search_pos = strstr(search_pos, "<!--")) != NULL) {
        char* end_comment = strstr(search_pos, "-->");
        if (end_comment != NULL) {
            // Point to the character after the closing "-->"
            end_comment += 3;
            
            // Calculate how much content remains after the comment
            size_t remaining_len = strlen(end_comment) + 1; // +1 for null terminator
            
            // Move the content after the comment to overwrite the comment
            memmove(search_pos, end_comment, remaining_len);
            // Don't advance search_pos, since we want to check for comments
            // that might have shifted into the current position
        } else {
            // No closing --> found, break to avoid infinite loop
            break;
        }
    }
    
    return result;
}

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
    // First, remove comments from the content
    char* content_without_comments = remove_comments_from_chtm(content);
    if (!content_without_comments) {
        // If memory allocation failed, use original content
        content_without_comments = strdup(content);
        if (!content_without_comments) return;  // Out of memory
    }
    
    element_count = 0;
    char* content_copy = content_without_comments;
    char* cursor = content_copy;
    
    // Reset elements
    for (int i = 0; i < MAX_ELEMENTS; i++) {
        memset(&elements[i], 0, sizeof(UIElement));
        elements[i].focused = false;  // Initialize focused to false
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
        else if (strcmp(tag_name, "br") == 0) {
            if (element_count < MAX_ELEMENTS) {
                elements[element_count].key = -1;
                strcpy(elements[element_count].type, "br");
                element_count++;
            }
        }
        else if (strcmp(tag_name, "cli_io") == 0) {
            if (element_count < MAX_ELEMENTS) {
                elements[element_count].key = -1;
                strcpy(elements[element_count].type, "cli_io");
                parse_attributes(&elements[element_count], space_pos);
                // Set default prompt if not specified
                if (elements[element_count].label[0] == '\0') {
                    strcpy(elements[element_count].label, "Command: ");
                }
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

// Check if an element is interactive (can be navigated to)
bool is_interactive_element(UIElement* el) {
    return (strcmp(el->type, "link") == 0 || 
            strcmp(el->type, "button") == 0 ||
            strcmp(el->type, "canvas") == 0 ||
            strcmp(el->type, "cli_io") == 0);
}

// Render UI elements
void render_elements() {
    printf("\033[H\033[J"); // Clear screen
    printf("Current file: %s\n\n", current_file);
    
    int interactive_index = 0; // For sequential numbering of interactive elements
    
    for (int i = 0; i < element_count; i++) {
        UIElement* el = &elements[i];
        
        if (strcmp(el->type, "br") == 0) {
            printf("\n");
        }
        else if (strcmp(el->type, "text") == 0) {
            if (el->label[0] != '\0') {
                debug_log("Processing text element with label: %s", el->label);
                // Process variable substitution in the label
                char processed_label[MAX_LABEL_LEN * 2]; // Allow for expansion
                substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                debug_log("After variable substitution: %s", processed_label);
                printf("%s", processed_label);
            } else {
                // If no label, do nothing. A <br> tag is now required for a newline.
                ;
            }
        }
        else if (strcmp(el->type, "link") == 0) {
            // Only show the link if it's interactive (has label or key that makes it interactive)
            if (el->key > 0 && el->label[0] != '\0') {
                char processed_label[MAX_LABEL_LEN * 2];
                substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                interactive_index++;
                if (i == active_interaction_element) {
                    printf("[^] %d. [%s]", interactive_index, processed_label);
                } else if (el->focused) {
                    printf("[>] %d. [%s]", interactive_index, processed_label);
                } else {
                    printf("[ ] %d. [%s]", interactive_index, processed_label);
                }
            } else if (el->label[0] != '\0') {
                char processed_label[MAX_LABEL_LEN * 2];
                substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                interactive_index++;
                if (i == active_interaction_element) {
                    printf("[^] %d. [Link: %s]", interactive_index, processed_label);
                } else if (el->focused) {
                    printf("[>] %d. [Link: %s]", interactive_index, processed_label);
                } else {
                    printf("[ ] %d. [Link: %s]", interactive_index, processed_label);
                }
            } else if (el->key > 0) {
                interactive_index++;
                if (i == active_interaction_element) {
                    printf("[^] %d. [Link]", interactive_index);
                } else if (el->focused) {
                    printf("[>] %d. [Link]", interactive_index);
                } else {
                    printf("[ ] %d. [Link]", interactive_index);
                }
            }
            // Skip links without either key or label
        }
        else if (strcmp(el->type, "button") == 0) {
            // Only show the button if it's interactive (has label or key that makes it interactive)
            if (el->key > 0 && el->label[0] != '\0') {
                char processed_label[MAX_LABEL_LEN * 2];
                substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                interactive_index++;
                if (i == active_interaction_element) {
                    printf("[^] %d. [%s]", interactive_index, processed_label);
                } else if (el->focused) {
                    printf("[>] %d. [%s]", interactive_index, processed_label);
                } else {
                    printf("[ ] %d. [%s]", interactive_index, processed_label);
                }
            } else if (el->label[0] != '\0') {
                char processed_label[MAX_LABEL_LEN * 2];
                substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                interactive_index++;
                if (i == active_interaction_element) {
                    printf("[^] %d. [Button: %s]", interactive_index, processed_label);
                } else if (el->focused) {
                    printf("[>] %d. [Button: %s]", interactive_index, processed_label);
                } else {
                    printf("[ ] %d. [Button: %s]", interactive_index, processed_label);
                }
            } else if (el->key > 0) {
                interactive_index++;
                if (i == active_interaction_element) {
                    printf("[^] %d. [Button]", interactive_index);
                } else if (el->focused) {
                    printf("[>] %d. [Button]", interactive_index);
                } else {
                    printf("[ ] %d. [Button]", interactive_index);
                }
            }
            // Skip buttons without either key or label
        }
        else if (strcmp(el->type, "panel") == 0) {
            // Panel is just a container, may add visual separation later
            // Don't print panel elements as visible items
        }

        else if (strcmp(el->type, "canvas") == 0) {
            if (el->width > 0 && el->height > 0) {
                // Print the canvas with its label (with variable substitution) and focus indicator
                interactive_index++;
                if (el->label[0] != '\0') {
                    char processed_label[MAX_LABEL_LEN * 2];
                    substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                    // Check if this element is in interaction mode
                    if (i == active_interaction_element) {
                        // Element is in interaction mode - show with "^"
                        printf("[^] %d. %s\n", interactive_index, processed_label);  // Show interaction mode indicator
                    } else if (el->focused) {
                        // Element is just selected (navigation) - show with ">"
                        printf("[>] %d. %s\n", interactive_index, processed_label);  // Show selected indicator
                    } else {
                        // Element is not focused - show with regular brackets
                        printf("[ ] %d. %s\n", interactive_index, processed_label);  // Show unselected
                    }
                } else {
                    // If canvas has no label, still show with index if it's interactive
                    if (i == active_interaction_element) {
                        printf("[^] %d. Canvas\n", interactive_index);  // Show interaction mode indicator
                    } else if (el->focused) {
                        printf("[>] %d. Canvas\n", interactive_index);  // Show selected indicator
                    } else {
                        printf("[ ] %d. Canvas\n", interactive_index);  // Show unselected
                    }
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
                    // Send unified DISPLAY command with canvas ID for all canvas types (unified approach)
                    // This allows the module to know which canvas to render while avoiding direct output conflicts
                    char display_cmd[284]; // Buffer for command with ID
                    if (strlen(el->id) > 0) {
                        snprintf(display_cmd, sizeof(display_cmd), "DISPLAY:ELEMENT_ID:%s", el->id);
                    } else {
                        strcpy(display_cmd, "DISPLAY");
                    }
                    send_ipc_command(display_cmd);
                    
                    // Read the canvas data from the module
                    char response[1024];
                    int lines_read = 0;
                    int total_lines = 0;
                    int canvas_element_idx = i; // Store the index of the canvas element we're reading for
                    
                    // Initialize canvas data storage if needed
                    if (total_lines == 0) {
                        model_init_canvas_data(canvas_element_idx, el->width, el->height, el->id);
                    }
                    
                    // Read responses until we have read the expected lines
                    // For emoji palette, expect 1 line; for game canvas, expect el->height lines
                    int expected_lines = (strcmp(el->id, "emoji_palette") == 0) ? 1 : el->height;
                    while (total_lines < (expected_lines + 5)) { // canvas height + some extra for potential position data
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
                                    
                                    // Log frame when important variables change (e.g., placement actions)
                                    if (strcmp(name, "last_action") == 0 || 
                                        strcmp(name, "last_input_was_space") == 0 ||
                                        strcmp(name, "player_x") == 0 ||
                                        strcmp(name, "player_y") == 0 ||
                                        strcmp(name, "last_placed_symbol") == 0) {
                                        log_frame_to_debug_file();
                                    }
                                } else {
                                    debug_log("ERROR: Invalid VAR format: %s", response);
                                }
                            }
                            // Check if this is a batched variable update
                            else if (strncmp(response, "VARS:", 5) == 0) {
                                debug_log("Received VARS response: %s", response);
                                // Format: VARS:name1=value1,name2=value2,name3=value3;
                                char *var_list = response + 5; // Skip "VARS:"
                                char *semi_pos = strrchr(var_list, ';'); // Find last semicolon
                                if (semi_pos) {
                                    *semi_pos = '\0'; // Replace semicolon with null terminator
                                } else {
                                    debug_log("ERROR: Invalid VARS format - missing semicolon: %s", response);
                                    continue;
                                }
                                
                                // Parse each variable in the comma-separated list
                                char *token = strtok(var_list, ",");
                                while (token != NULL) {
                                    char *equals = strchr(token, '=');
                                    if (equals) {
                                        *equals = '\0';
                                        char *name = token;
                                        char *value = equals + 1;
                                        set_variable(name, value);
                                        
                                        // Log frame when important variables change (e.g., placement actions)
                                        if (strcmp(name, "last_action") == 0 || 
                                            strcmp(name, "last_input_was_space") == 0 ||
                                            strcmp(name, "player_x") == 0 ||
                                            strcmp(name, "player_y") == 0 ||
                                            strcmp(name, "last_placed_symbol") == 0) {
                                            log_frame_to_debug_file();
                                        }
                                    } else {
                                        debug_log("ERROR: Invalid variable format in VARS: %s", token);
                                    }
                                    token = strtok(NULL, ",");
                                }
                            } 
                            // Handle emoji palette data - store in model
                            else if (strcmp(el->id, "emoji_palette") == 0 && lines_read < 1) {
                                model_update_canvas_data(canvas_element_idx, lines_read, response);
                                lines_read++;
                            } 
                            // Handle regular canvas lines
                            else if (strcmp(el->id, "emoji_palette") != 0 && lines_read < el->height) {
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
                                
                                // This looks like a canvas line
                                    model_update_canvas_data(canvas_element_idx, lines_read, response);
                                    lines_read++;
                            } else {
                                debug_log("Received other response: %s", response);
                            }
                        } else {
                            // No more data available
                            break;
                        }
                        total_lines++;
                        
                        if (lines_read >= expected_lines) break;
                    }
                    
                    // After reading, display from the stored model data to avoid duplicate output
                    if (strcmp(el->id, "emoji_palette") == 0) {
                        // Palette is special case with single line
                        if (lines_read == 0) {
                            // If no data was received, initialize with default
                            char default_line[MAX_CANVAS_WIDTH * 2];
                            for (int c = 0; c < el->width && c < MAX_CANVAS_WIDTH; c++) {
                                default_line[c * 2] = '?';      // Placeholder char
                                default_line[c * 2 + 1] = ' ';  // Space separator
                            }
                            default_line[el->width * 2] = '\0';
                            model_update_canvas_data(canvas_element_idx, 0, default_line);
                        }
                        // Display palette from model data
                        CanvasData* palette_data = model_get_canvas_data_by_index(canvas_element_idx);
                        if (palette_data && palette_data->valid) {
                            printf("%s\n", palette_data->grid[0]);
                        } else {
                            // Fallback if no data in model
                            printf("Error loading palette data\n");
                        }
                    } else {
                        // For game canvas, display from stored model data
                        CanvasData* canvas_data = model_get_canvas_data_by_index(canvas_element_idx);
                        if (canvas_data && canvas_data->valid) {
                            for (int r = 0; r < canvas_data->height && r < MAX_CANVAS_HEIGHT; r++) {
                                printf("%s\n", canvas_data->grid[r]);
                            }
                        } else {
                            // Fallback: draw the canvas grid with placeholder
                            for (int r = 0; r < el->height; r++) {
                                for (int c = 0; c < el->width; c++) {
                                    printf(". ");
                                }
                                printf("\n");
                            }
                        }
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
        else if (strcmp(el->type, "cli_io") == 0) {
            // Display the cli_io element with appropriate focus indicator
            interactive_index++;
            if (i == active_interaction_element) {
                // CLI element is in active interaction mode (receiving text input)
                char processed_label[MAX_LABEL_LEN * 2];
                substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                printf("[^] %d. %s", interactive_index, processed_label);
            } else if (el->focused) {
                // CLI element has navigation focus (waiting for activation)
                char processed_label[MAX_LABEL_LEN * 2];
                substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                printf("[>] %d. %s", interactive_index, processed_label);
            } else {
                // CLI element is not focused
                char processed_label[MAX_LABEL_LEN * 2];
                substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                printf("[ ] %d. %s", interactive_index, processed_label);
            }
        }
    }
    
    // Display navigation input prompt at the bottom of the screen, but not when an element is active
    if (active_interaction_element != -1) {
        // An element is active [^], don't show navigation prompt to avoid confusion
        printf("\n"); // Just add a newline for spacing
    } else {
        if (nav_input_active) {
            printf("\nNavigation: %s", nav_input_buffer);
        } else {
            printf("\nNavigation: Enter element index (1-%d) or 'q' to quit", interactive_index);
        }
    }
    
    fflush(stdout);
}

// Global input buffer to manually handle all input
char manual_input_buffer[256];
int manual_input_pos = 0;
bool manual_echo_enabled = true;

// Read key from log file (instead of terminal)
int read_key_from_log() {
    static FILE *log_fp = NULL;
    static long last_pos = 0;
    
    if (log_fp == NULL) {
        // Try to open log file - it may not exist yet
        log_fp = fopen("log.txt", "r");
        if (log_fp == NULL) {
            return -1;  // No log file yet
        }
        fseek(log_fp, 0, SEEK_END);  // Start at end of file
        last_pos = ftell(log_fp);
    }
    
    // Check if there's new data in the log file
    fseek(log_fp, 0, SEEK_END);
    long current_pos = ftell(log_fp);
    
    if (current_pos > last_pos) {
        // New data available, seek to the previous end position
        fseek(log_fp, last_pos, SEEK_SET);
        
        char line[256];
        if (fgets(line, sizeof(line), log_fp) != NULL) {
            // Parse the log line to extract key code
            int key_code = -1;
            char key_name[100];
            
            // Try to parse "SPECIAL: KEY_NAME (KEY_CODE)\n" format like reference implementation
            if (sscanf(line, "SPECIAL: %99[^ ] (%d)", key_name, &key_code) == 2) {
                last_pos = ftell(log_fp);
                return key_code;
            }
            // Try to parse other formats that might have key codes in parentheses
            else if (sscanf(line, "%*[^(](%d)", &key_code) == 1) {
                // Pattern to match anything followed by (number)
                last_pos = ftell(log_fp);
                return key_code;
            }
            // Try to parse simple numeric format (just the number on the line)
            else if (sscanf(line, "%d", &key_code) == 1) {
                last_pos = ftell(log_fp);
                return key_code;
            }
        }
        
        // Update position to where we are now (whether we parsed successfully or not)
        last_pos = ftell(log_fp);
    }
    
    return -1;  // No new key available
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

// Remove from navigation input buffer (backspace)
void backspace_nav_input_buffer() {
    if (nav_input_pos > 0) {
        nav_input_pos--;
        nav_input_buffer[nav_input_pos] = '\0';
    }
}

// Clear input buffer
void clear_input_buffer() {
    input_buffer[0] = '\0';
    input_buffer_pos = 0;
}

// Clear navigation input buffer
void clear_nav_input_buffer() {
    nav_input_buffer[0] = '\0';
    nav_input_pos = 0;
    nav_input_active = false;
}

// Process input
bool process_input(int ch) {
  
    // Handle Backspace (for active CLI element or navigation input)
    if (ch == 127 || ch == 8) {
        // If navigation input is active, handle backspace in nav input
        if (nav_input_active) {
            if (nav_input_pos > 0) {
                backspace_nav_input_buffer();
            } else {
                // If nav input is empty, deactivate it
                clear_nav_input_buffer();
            }
            return true; // Redraw to show updated navigation input
        }
        // Otherwise, handle backspace for active CLI element
        else if (active_interaction_element != -1 && strcmp(elements[active_interaction_element].type, "cli_io") == 0) {
            if (input_buffer_pos > 0) {
                printf("\b \b");
                backspace_input_buffer();
            }
            fflush(stdout);
            return false;
        }
    }

    // Handle Spacebar (send raw ASCII code to active elements with element ID)
    if (ch == 32 && active_interaction_element != -1 && 
        (strcmp(elements[active_interaction_element].type, "canvas") == 0 || 
         strcmp(elements[active_interaction_element].type, "cli_io") == 0)) {
        UIElement* el = &elements[active_interaction_element];
        if (strcmp(el->type, "cli_io") == 0) {
            // For CLI input, simply add space to input buffer (don't send individual characters to module)
            // The full command will be sent to the module when Enter is pressed
            printf(" "); // Echo the space character
            add_to_input_buffer(' ');
            fflush(stdout);
            return false; // Don't redraw, just continue editing (like regular characters)
        } else {
            // For canvas elements, send as key command with element ID
            char key_cmd[284]; // Increased buffer size to accommodate ID
            if (strcmp(el->type, "canvas") == 0 && strlen(el->id) > 0) {
                // Include canvas ID in the command for targeted input
                snprintf(key_cmd, sizeof(key_cmd), "KEY:%d:ELEMENT_ID:%s", ch, el->id);
            } else {
                // For non-canvas elements or canvas without ID, use basic format
                snprintf(key_cmd, sizeof(key_cmd), "KEY:%d", ch);
            }
            send_ipc_command(key_cmd);
            return true;
        }
    }

    // Handle Enter Key (Modified behavior per requirements)
    if (ch == '\r' || ch == '\n') {
        // If navigation input is active (user entered index)
        if (nav_input_active) {
            if (nav_input_pos > 0) {
                // Convert input to integer
                int target_number = atoi(nav_input_buffer);
                
                // Find the element with the specified navigation number
                int found_element_index = -1;
                int current_nav_index = 0;
                
                for (int i = 0; i < element_count; i++) {
                    if (is_interactive_element(&elements[i])) {
                        current_nav_index++;
                        if (current_nav_index == target_number) {
                            found_element_index = i;
                            break;
                        }
                    }
                }
                
                // Validate range
                if (target_number >= 1 && found_element_index != -1) {
                    // Set focus to the found element (but don't activate it)
                    set_focus(found_element_index);
                } else {
                    // Out of range - clear screen and show message
                    printf("\033[H\033[J"); // Clear screen
                    printf("Out of range. Enter a number between 1 and %d\n", current_nav_index);
                    render_elements();
                }
            } else {
                // No number entered, activate the currently focused element if one exists
                if (focused_element_index != -1) {
                    active_interaction_element = focused_element_index;
                    clear_input_buffer();
                    // Focus stays on the same element, just change to active state
                }
            }
            
            // Clear navigation input
            nav_input_buffer[0] = '\0';
            nav_input_pos = 0;
            nav_input_active = false;
            return true;
        }
        // If any element is active [^]: Enter is passed directly to the appropriate module and element stays active
        else if (active_interaction_element != -1) {
            UIElement* el = &elements[active_interaction_element];

            if (strcmp(el->type, "canvas") == 0) {
                // Pass Enter directly to canvas module as raw ASCII with element ID
                char key_cmd[284]; // Increased buffer size to accommodate ID
                if (strlen(el->id) > 0) {
                    // Include canvas ID in the command for targeted input
                    snprintf(key_cmd, sizeof(key_cmd), "KEY:%d:ELEMENT_ID:%s", 10, el->id);
                } else {
                    // For canvas without ID, use basic format
                    snprintf(key_cmd, sizeof(key_cmd), "KEY:%d", 10); // Send raw Enter key code
                }
                send_ipc_command(key_cmd);
                return true; // Keep element active, don't deactivate
            } else if (strcmp(el->type, "cli_io") == 0) {
                // For CLI, send the input but keep it active
                if (ipc.active && input_buffer_pos > 0) {
                    char cli_cmd[256];
                    snprintf(cli_cmd, sizeof(cli_cmd), "CLI_INPUT:%.*s", input_buffer_pos, input_buffer);
                    send_ipc_command(cli_cmd);
                }
                clear_input_buffer();
                return true; // Keep element active
            } else if (strcmp(el->type, "link") == 0) {
                // For links, execute the action (navigate to href) and keep element active
                switch_to_file(el->href);
                return true; // Keep element active
            } else if (strcmp(el->type, "button") == 0) {
                // For buttons, execute the action and keep element active
                if (strlen(el->onClick) > 0) {
                    // Send the onClick command to the active module
                    send_ipc_command(el->onClick);
                }
                return true; // Keep element active and trigger refresh to show variable updates
            }
        }
        // If no element is active but one is focused [>]: Enter activates it
        else if (focused_element_index != -1) {
            active_interaction_element = focused_element_index;
            clear_input_buffer();
            return true; // Redraw to show [^]
        }
        // Fallback: If nothing is focused or active, just process as a newline.
        printf("\n");
        return true;
    }

    // Handle ESC for deactivation (also clears navigation input)
    if (ch == ESC_KEY) {
        // First, clear navigation input if it's active
        if (nav_input_active) {
            clear_nav_input_buffer();
            return true;
        }
        // Otherwise, deactivate active elements
        if (active_interaction_element != -1) {
            active_interaction_element = -1;
            clear_input_buffer();
            return true;
        }
    }

    // Handle Arrow Keys (send raw ASCII codes to active modules with element ID)
    if (ch >= ARROW_LEFT && ch <= ARROW_DOWN) {
        // If navigation input is active, clear it first
        if (nav_input_active) {
            clear_nav_input_buffer();
            return true;
        }
        // If an element is active, send raw ASCII codes to the module with element ID
        if (active_interaction_element != -1) {
            UIElement* el = &elements[active_interaction_element];
            
            // Send raw ASCII codes for arrow keys to any active element that can receive input
            if (strcmp(el->type, "canvas") == 0 || strcmp(el->type, "cli_io") == 0) {
                // Send raw arrow key codes to modules with element ID
                char key_cmd[284]; // Increased buffer size to accommodate ID (key code up to 10 chars + :ELEMENT_ID: + 256 max ID chars + null)
                if (strcmp(el->type, "canvas") == 0 && strlen(el->id) > 0) {
                    // Include canvas ID in the command for targeted input
                    snprintf(key_cmd, sizeof(key_cmd), "KEY:%d:ELEMENT_ID:%s", ch, el->id);
                } else {
                    // For non-canvas elements or canvas without ID, use basic format
                    snprintf(key_cmd, sizeof(key_cmd), "KEY:%d", ch);
                }
                send_ipc_command(key_cmd);
                return true;
            }
            // For other active elements (like links, buttons), block arrow key navigation
            else {
                // Return true to prevent navigation, but don't send to module
                // This keeps the element active without changing focus
                return true;
            }
        }
        // If no element is active, navigate focus normally
        else {
            int current_focus = (focused_element_index == -1) ? 0 : focused_element_index;
            switch (ch) {
                case ARROW_UP:    set_focus(find_prev_interactive_element(current_focus)); break;
                case ARROW_DOWN:  set_focus(find_next_interactive_element(current_focus)); break;
                case ARROW_LEFT:  set_focus(find_prev_interactive_element(current_focus)); break;
                case ARROW_RIGHT: set_focus(find_next_interactive_element(current_focus)); break;
            }
        }
        return true;
    }

    // Handle Printable Characters (send raw ASCII codes to active elements with element ID)
    if (isprint(ch)) {
        // If an element is active, send all input to the active element instead of processing navigation
        if (active_interaction_element != -1) {
            UIElement* el = &elements[active_interaction_element];
            if (strcmp(el->type, "cli_io") == 0) {
                printf("%c", ch);
                add_to_input_buffer((char)ch);
                fflush(stdout);
                return false;
            } else if (strcmp(el->type, "canvas") == 0) {
                // Send printable character as raw ASCII to canvas modules with element ID
                char key_cmd[284]; // Increased buffer size to accommodate ID
                if (strlen(el->id) > 0) {
                    // Include canvas ID in the command for targeted input
                    snprintf(key_cmd, sizeof(key_cmd), "KEY:%d:ELEMENT_ID:%s", ch, el->id);
                } else {
                    // For canvas without ID, use basic format
                    snprintf(key_cmd, sizeof(key_cmd), "KEY:%d", ch);
                }
                send_ipc_command(key_cmd);
                return true;
            }
            // For other active elements (like links, buttons), they don't typically take text input,
            // so we ignore the input in those cases
        }
        // If navigation input is active, add to navigation buffer
        else if (nav_input_active) {
            if (nav_input_pos < 255 && isdigit(ch)) {  // Only allow digits for navigation
                nav_input_buffer[nav_input_pos] = ch;
                nav_input_pos++;
                nav_input_buffer[nav_input_pos] = '\0';
                return true; // Redraw to show updated navigation input
            }
        }
        // If no element is active and navigation input isn't active, check if digit starts navigation
        else {
            // Check if the character is a digit to start navigation input mode
            if (isdigit(ch)) {
                // Start navigation input mode
                nav_input_active = true;
                nav_input_buffer[0] = ch;
                nav_input_pos = 1;
                nav_input_buffer[nav_input_pos] = '\0';
                return true; // Redraw to show navigation input
            }
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

/*
 * Model functions for canvas data management
 */

// Initialize canvas data
void model_init_canvas_data(int canvas_idx, int width, int height, const char* id) {
    if (canvas_idx >= 0 && canvas_idx < MAX_ELEMENTS) {
        CanvasData* cd = &model_canvas_data[canvas_idx];
        cd->width = width;
        cd->height = height;
        cd->valid = true;
        strncpy(cd->id, id, MAX_ATTR_LEN - 1);
        cd->id[MAX_ATTR_LEN - 1] = '\0';
        
        // Initialize with placeholder
        for (int r = 0; r < height && r < MAX_CANVAS_HEIGHT; r++) {
            for (int c = 0; c < width && c < MAX_CANVAS_WIDTH; c++) {
                cd->grid[r][c * 2] = '.';      // Character
                cd->grid[r][c * 2 + 1] = ' ';  // Space separator
            }
            cd->grid[r][width * 2] = '\0'; // Null terminate
        }
        
        if (canvas_idx >= model_canvas_count) {
            model_canvas_count = canvas_idx + 1;
        }
    }
}

// Update canvas data from module response
void model_update_canvas_data(int canvas_idx, int line_num, const char* line_data) {
    if (canvas_idx >= 0 && canvas_idx < MAX_ELEMENTS && 
        line_num >= 0 && line_num < MAX_CANVAS_HEIGHT) {
        CanvasData* cd = &model_canvas_data[canvas_idx];
        if (cd->valid) {
            strncpy(cd->grid[line_num], line_data, (MAX_CANVAS_WIDTH * 2) - 1);
            cd->grid[line_num][(MAX_CANVAS_WIDTH * 2) - 1] = '\0';
        }
    }
}

// Get canvas data for a specific canvas
CanvasData* model_get_canvas_data(const char* canvas_id) {
    for (int i = 0; i < model_canvas_count; i++) {
        if (model_canvas_data[i].valid && 
            strcmp(model_canvas_data[i].id, canvas_id) == 0) {
            return &model_canvas_data[i];
        }
    }
    return NULL;
}

// Get canvas data by element index
CanvasData* model_get_canvas_data_by_index(int canvas_idx) {
    if (canvas_idx >= 0 && canvas_idx < MAX_ELEMENTS) {
        return &model_canvas_data[canvas_idx];
    }
    return NULL;
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
            const char* var_start = src_ptr + 2; // Skip "$" and "{"
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
                // No closing '}', copy the '$' character
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

// Signal handler to restore terminal settings on Ctrl+C
void signal_handler(int sig) {
    // Restore original terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    printf("\nReceived signal %d, exiting gracefully...\n", sig);
    exit(0);
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
    fprintf(debug_file, "\n");
    fflush(debug_file);
}

// Function to find next interactive element for focus navigation
int find_next_interactive_element(int start_index) {
    for (int i = start_index + 1; i < element_count; i++) {
        if (is_interactive_element(&elements[i])) {
            return i;
        }
    }
    // If we reach the end, wrap around to the beginning
    for (int i = 0; i < start_index; i++) {
        if (is_interactive_element(&elements[i])) {
            return i;
        }
    }
    // If no interactive elements found, return -1
    return -1;
}

// Function to find previous interactive element for focus navigation
int find_prev_interactive_element(int start_index) {
    for (int i = start_index - 1; i >= 0; i--) {
        if (is_interactive_element(&elements[i])) {
            return i;
        }
    }
    // If we reach the beginning, wrap around to the end
    for (int i = element_count - 1; i > start_index; i--) {
        if (is_interactive_element(&elements[i])) {
            return i;
        }
    }
    // If no interactive elements found, return -1
    return -1;
}

// Function to set focus on an element and remove focus from others
void set_focus(int element_index) {
    if (element_index < 0 || element_index >= element_count) {
        return;
    }
    
    debug_log("Setting focus to element index %d", element_index);
    
    // Remove focus from all elements
    for (int i = 0; i < element_count; i++) {
        elements[i].focused = false;
    }
    
    // Set focus on the specified element
    elements[element_index].focused = true;
    focused_element_index = element_index;
}

// Function to initialize focus (focus the first interactive element)
void initialize_focus() {
    focused_element_index = -1;
    int first_interactive = find_next_interactive_element(-1);
    if (first_interactive != -1) {
        set_focus(first_interactive);
    }
}

// Frame logging function to capture current rendered output with timestamp
void log_frame_to_debug_file() {
    // The actual rendered output will be captured by modifying the render_elements function
    // This function can be called to indicate a frame capture point
    // The real capture happens in the modified render_elements function
}

// Function to rebuild the current display state for logging
void capture_rendered_frame() {
    FILE* file = fopen("frames.txt", "a");
    if (file != NULL) {
        time_t now = time(0);
        char* time_str = ctime(&now);
        // Remove the newline at the end of ctime result
        time_str[strlen(time_str)-1] = '\0';
        
        fprintf(file, "=== FRAME CAPTURED AT: %s ===\n", time_str);
        
        // Rebuild what the display should look like based on current state
        fprintf(file, "Current file: %s\n\n", current_file);
        
        int interactive_index = 0;
        
        for (int i = 0; i < element_count; i++) {
            UIElement* el = &elements[i];
            
            if (strcmp(el->type, "text") == 0) {
                if (el->label[0] != '\0') {
                    // Process variable substitution in the label
                    char processed_label[MAX_LABEL_LEN * 2]; // Allow for expansion
                    substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                    fprintf(file, "%s\n", processed_label);
                } else {
                    // If no label, skip or print just a newline
                    fprintf(file, "\n");
                }
            }
            else if (strcmp(el->type, "link") == 0) {
                // Only show the link if it has a key or label
                if (el->key > 0 && el->label[0] != '\0') {
                    char processed_label[MAX_LABEL_LEN * 2];
                    substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                    interactive_index++;
                    if (el->focused) {
                        fprintf(file, ">[>] %d. [%s]\n", interactive_index, processed_label);
                    } else {
                        fprintf(file, "[ ] %d. [%s]\n", interactive_index, processed_label);
                    }
                } else if (el->label[0] != '\0') {
                    char processed_label[MAX_LABEL_LEN * 2];
                    substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                    interactive_index++;
                    if (i == active_interaction_element) {
                        fprintf(file, "[^] %d. [Link: %s]\n", interactive_index, processed_label);
                    } else if (el->focused) {
                        fprintf(file, ">[>] %d. [Link: %s]\n", interactive_index, processed_label);
                    } else {
                        fprintf(file, "[ ] %d. [Link: %s]\n", interactive_index, processed_label);
                    }
                } else if (el->key > 0) {
                    interactive_index++;
                    if (i == active_interaction_element) {
                        fprintf(file, "[^] %d. [Link]\n", interactive_index);
                    } else if (el->focused) {
                        fprintf(file, ">[>] %d. [Link]\n", interactive_index);
                    } else {
                        fprintf(file, "[ ] %d. [Link]\n", interactive_index);
                    }
                }
                // Skip links without either key or label
            }
            else if (strcmp(el->type, "button") == 0) {
                // Only show the button if it has a key or label
                if (el->key > 0 && el->label[0] != '\0') {
                    char processed_label[MAX_LABEL_LEN * 2];
                    substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                    interactive_index++;
                    if (i == active_interaction_element) {
                        fprintf(file, "[^] %d. [%s]\n", interactive_index, processed_label);
                    } else if (el->focused) {
                        fprintf(file, ">[>] %d. [%s]\n", interactive_index, processed_label);
                    } else {
                        fprintf(file, "[ ] %d. [%s]\n", interactive_index, processed_label);
                    }
                } else if (el->label[0] != '\0') {
                    char processed_label[MAX_LABEL_LEN * 2];
                    substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                    interactive_index++;
                    if (i == active_interaction_element) {
                        fprintf(file, "[^] %d. [Button: %s]\n", interactive_index, processed_label);
                    } else if (el->focused) {
                        fprintf(file, ">[>] %d. [Button: %s]\n", interactive_index, processed_label);
                    } else {
                        fprintf(file, "[ ] %d. [Button: %s]\n", interactive_index, processed_label);
                    }
                } else if (el->key > 0) {
                    interactive_index++;
                    if (i == active_interaction_element) {
                        fprintf(file, "[^] %d. [Button]\n", interactive_index);
                    } else if (el->focused) {
                        fprintf(file, ">[>] %d. [Button]\n", interactive_index);
                    } else {
                        fprintf(file, "[ ] %d. [Button]\n", interactive_index);
                    }
                }
                // Skip buttons without either key or label
            }
            else if (strcmp(el->type, "panel") == 0) {
                // Panel is just a container, may add visual separation later
                // Don't print panel elements as visible items
            }
            else if (strcmp(el->type, "canvas") == 0) {
                if (el->width > 0 && el->height > 0) {
                    // Print the canvas with its label (with variable substitution) and focus indicator
                    interactive_index++;
                    if (el->label[0] != '\0') {
                        char processed_label[MAX_LABEL_LEN * 2];
                        substitute_variables_in_string(el->label, processed_label, sizeof(processed_label));
                        // Check if this element is in interaction mode
                        if (i == active_interaction_element) {
                            // Element is in interaction mode - show with "^"
                            fprintf(file, "[^] %d. %s\n", interactive_index, processed_label);  // Show interaction mode indicator
                        } else if (el->focused) {
                            // Element is just selected (navigation) - show with ">"
                            fprintf(file, ">[>] %d. %s\n", interactive_index, processed_label);  // Show selected indicator
                        } else {
                            // Element is not focused - show with regular brackets
                            fprintf(file, "[ ] %d. %s\n", interactive_index, processed_label);  // Show unselected
                        }
                    }
                    
                    // Get canvas data from model and display it
                    CanvasData* canvas_data = model_get_canvas_data_by_index(i);
                    if (canvas_data && canvas_data->valid) {
                        // Print the actual canvas grid from model data
                        for (int r = 0; r < canvas_data->height && r < MAX_CANVAS_HEIGHT; r++) {
                            fprintf(file, "   %s\n", canvas_data->grid[r]);
                        }
                    } else {
                        // Fallback to placeholder if no model data is available
                        for (int r = 0; r < el->height; r++) {
                            for (int c = 0; c < el->width; c++) {
                                fprintf(file, "."); // Removed hardcoded space
                            }
                            fprintf(file, "\n");
                        }
                    }
                }
            }
        }
        
        fprintf(file, "Enter key (1-9) or 'q' to quit: \n");
        fprintf(file, "\n");
        fclose(file);
    }
}

// Function to process any pending responses from modules
void process_module_responses() {
    if (!ipc.active) {
        return; // No active module to read from
    }
    
    // Use select to check if there's data available to read without blocking
    fd_set read_fds;
    struct timeval timeout;
    int fd = fileno(ipc.read_pipe);
    
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    timeout.tv_sec = 0;  // No seconds
    timeout.tv_usec = 0; // No microseconds - makes it non-blocking
    
    // Check if data is available to read
    int activity = select(fd + 1, &read_fds, NULL, NULL, &timeout);
    
    if (activity > 0 && FD_ISSET(fd, &read_fds)) {
        // Data is available, read it
        char response[1024];
        
        if (read_ipc_response(response, sizeof(response)) == 0) {
            // Remove newline from response
            response[strcspn(response, "\n")] = 0;
            
            // Check if this is a variable update
            bool needs_refresh = false;
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
                    
                    // Check if this is a status message that should trigger a refresh
                    if (strcmp(name, "status_message") == 0) {
                        needs_refresh = true;
                    }
                    
                    // Log frame when important variables change
                    if (strcmp(name, "last_action") == 0 || 
                        strcmp(name, "last_input_was_space") == 0 ||
                        strcmp(name, "player_x") == 0 ||
                        strcmp(name, "player_y") == 0 ||
                        strcmp(name, "last_placed_symbol") == 0) {
                        log_frame_to_debug_file();
                    }
                } else {
                    debug_log("ERROR: Invalid VAR format: %s", response);
                }
            }
            // Check if this is a batched variable update
            else if (strncmp(response, "VARS:", 5) == 0) {
                debug_log("Received VARS response: %s", response);
                // Format: VARS:name1=value1,name2=value2,name3=value3;
                char *var_list = response + 5; // Skip "VARS:"
                char *semi_pos = strrchr(var_list, ';'); // Find last semicolon
                if (semi_pos) {
                    *semi_pos = '\0'; // Replace semicolon with null terminator
                } else {
                    debug_log("ERROR: Invalid VARS format - missing semicolon: %s", response);
                    return;
                }
                
                // Parse each variable in the comma-separated list
                char *token = strtok(var_list, ",");
                while (token != NULL) {
                    char *equals = strchr(token, '=');
                    if (equals) {
                        *equals = '\0';
                        char *name = token;
                        char *value = equals + 1;
                        set_variable(name, value);
                        
                        // Check if this is a status message that should trigger a refresh
                        if (strcmp(name, "status_message") == 0) {
                            needs_refresh = true;
                        }
                        
                        // Log frame when important variables change
                        if (strcmp(name, "last_action") == 0 || 
                            strcmp(name, "last_input_was_space") == 0 ||
                            strcmp(name, "player_x") == 0 ||
                            strcmp(name, "player_y") == 0 ||
                            strcmp(name, "last_placed_symbol") == 0) {
                            log_frame_to_debug_file();
                        }
                    } else {
                        debug_log("ERROR: Invalid variable format in VARS: %s", token);
                    }
                    token = strtok(NULL, ",");
                }
            }
            
            // If we received a status message, trigger a display refresh
            if (needs_refresh) {
                render_elements();
                capture_rendered_frame();
            }
            // Handle other response types as needed
            else {
                debug_log("Received other response: %s", response);
            }
        }
    }
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
    
    // Reset interaction states
    active_interaction_element = -1;
    clear_nav_input_buffer();
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
        
        // Initialize focus system after parsing elements for new page
        initialize_focus();
        // Make sure no element is in active interaction mode after file switch
        active_interaction_element = -1;
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
    
    // Save original terminal settings for restoration
    tcgetattr(STDIN_FILENO, &orig_termios);
    
    // Register signal handler for graceful exit (Ctrl+C)
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
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
        
        // Initialize focus system after parsing elements
        initialize_focus();
        // Make sure no element is in active interaction mode initially
        active_interaction_element = -1;
    } else {
        printf("Error: Could not read initial file %s\n", current_file);
        printf("Creating empty UI to allow user interaction...\n");
        
        // Initialize with an empty UI that allows navigation
        element_count = 0;
        focused_element_index = -1;
        
        // Add a default error message element
        if (element_count < MAX_ELEMENTS) {
            strcpy(elements[element_count].type, "text");
            // Create a truncated version of the filename to fit in the label buffer
            char truncated_file[MAX_LABEL_LEN - 30]; // Leave space for the rest of the message
            strncpy(truncated_file, current_file, sizeof(truncated_file) - 1);
            truncated_file[sizeof(truncated_file) - 1] = '\0'; // Ensure null termination
            snprintf(elements[element_count].label, MAX_LABEL_LEN, "Error: Could not find file '%s'", truncated_file);
            elements[element_count].key = -1;
            elements[element_count].focused = false;
            element_count++;
        }
        if (element_count < MAX_ELEMENTS) {
            strcpy(elements[element_count].type, "text");
            strcpy(elements[element_count].label, "Press 'q' to quit or enter a file path to load");
            elements[element_count].key = -1;
            elements[element_count].focused = false;
            element_count++;
        }
    }
    
    /*
 * =====================
 * CONTROLLER (C) - Input Processing and MVC Coordination
 * =====================
 */

// Main loop
    printf("CHTM renderer started. Press 'q' to quit.\n");
    render_elements();
    capture_rendered_frame(); // Capture initial render
    
    while (1) {
        int ch = read_key_from_log();
        if (ch != -1) {
            bool redraw = process_input(ch);
            if (redraw) {
                render_elements();
                capture_rendered_frame(); // Capture frame after redraw
            }
        }
        
        // Process any pending responses from modules to update variables
        process_module_responses();
        
        usleep(10000); // 10ms delay to reduce CPU usage (checking log file more frequently)
    }
    
    // Cleanup - this code will actually never be reached due to the infinite loop,
    // but is here for completeness and potential future modifications
    cleanup_ipc();
    if (log_file) {
        fclose(log_file);
    }
    
    // Restore original terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    
    if (debug_file) {
        fprintf(debug_file, "=== CHTM Renderer Ending ===\n");
        fclose(debug_file);
    }
    
    return 0;
}