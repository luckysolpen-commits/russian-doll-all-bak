#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// Control plugin - handles keyboard input and translates to game events
// Works with control piece to process movement and other inputs

// Function to read new inputs from keyboard history file
int getNewInputsFromFile(int *inputs, int max_inputs) {
    FILE *fp = fopen("peices/keyboard/history.txt", "r");
    if (!fp) return 0;
    
    // Seek to end to get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    if (file_size == 0) {
        fclose(fp);
        return 0;
    }
    
    // Allocate memory to read the file content
    char *buffer = malloc(file_size + 1);
    if (!buffer) {
        fclose(fp);
        return 0;
    }
    
    // Read from beginning
    fseek(fp, 0, SEEK_SET);
    fread(buffer, 1, file_size, fp);
    fclose(fp);
    buffer[file_size] = '\0';
    
    // Parse the buffer for integers (key codes)
    int count = 0;
    char *token = strtok(buffer, "\n\r ");
    while (token && count < max_inputs) {
        if (strlen(token) > 0 && token[0] != '#') { // Skip comments
            int key_code;
            if (sscanf(token, "%d", &key_code) == 1) {
                inputs[count++] = key_code;
            }
        }
        token = strtok(NULL, "\n\r ");
    }
    
    free(buffer);
    return count;
}

// Handle WASD input
int handleDirectionalInput(char key_char) {
    // Get current player position
    FILE *fp = popen("./+x/get_player_pos.+x", "r");
    int x = 0, y = 0;
    int result = 0;
    
    if (fp && fscanf(fp, "%d %d", &x, &y) == 2) {
        pclose(fp);
        
        // Calculate new position based on key
        int new_x = x, new_y = y;
        switch(key_char) {
            case 'w': case 'W': new_y--; break;
            case 's': case 'S': new_y++; break;
            case 'a': case 'A': new_x--; break;
            case 'd': case 'D': new_x++; break;
            default: return 0;
        }
        
        // Update player position
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "./+x/update_player_pos.+x %d %d", new_x, new_y);
        system(cmd);
        result = 1;
    } else if (fp) {
        pclose(fp);
    }
    
    return result;
}

// Handle arrow key input (encoded as integers from keyboard handler)
int handleArrowInput(int key_code) {
    // Get current player position
    FILE *fp = popen("./+x/get_player_pos.+x", "r");
    int x = 0, y = 0;
    int result = 0;
    
    if (fp && fscanf(fp, "%d %d", &x, &y) == 2) {
        pclose(fp);
        
        // Calculate new position based on key code
        int new_x = x, new_y = y;
        switch(key_code) {
            case 1000: new_y--; break; // ARROW_UP
            case 1001: new_y++; break; // ARROW_DOWN
            case 1002: new_x++; break; // ARROW_RIGHT
            case 1003: new_x--; break; // ARROW_LEFT
            default: return 0;
        }
        
        // Update player position
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "./+x/update_player_pos.+x %d %d", new_x, new_y);
        system(cmd);
        result = 1;
    } else if (fp) {
        pclose(fp);
    }
    
    return result;
}

// Handle number input for menu selection
int handleNumberInput(char num_char) {
    if (num_char < '0' || num_char > '9') {
        return 0;
    }
    
    int option_num = num_char - '0';
    
    // Update menu selection in piece
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "./+x/piece_manager.+x update_piece_state menu menu current_selection %d", option_num);
    system(cmd);
    
    // Execute the corresponding method using methods plugin
    snprintf(cmd, sizeof(cmd), "./+x/methods_plugin.+x execute_menu_option_%d menu", option_num);
    system(cmd);
    
    return 1;
}

int main() {
    printf("Control plugin initialized\n");
    
    // Ensure directories exist
    system("mkdir -p peices/keyboard peices/player peices/menu");
    
    // Create initial keyboard piece if it doesn't exist
    if (access("peices/keyboard/keyboard.txt", F_OK) != 0) {
        FILE *kb_fp = fopen("peices/keyboard/keyboard.txt", "w");
        if (kb_fp) {
            fprintf(kb_fp, "session_id 12345\n");
            fprintf(kb_fp, "current_key_log_path peices/keyboard/history.txt\n");
            fprintf(kb_fp, "logging_enabled true\n");
            fprintf(kb_fp, "keys_recorded 0\n");
            fprintf(kb_fp, "event_listeners key_press\n");
            fprintf(kb_fp, "last_key_logged \n");
            fprintf(kb_fp, "last_key_time \n");
            fprintf(kb_fp, "last_key_code 0\n");
            fprintf(kb_fp, "methods log_key,reset_session,get_session_log,enable_logging,disable_logging\n");
            fprintf(kb_fp, "status active\n");
            fclose(kb_fp);
        }
    }
    
    // Main processing loop
    int input_buffer[10];
    int num_inputs;
    
    // Process inputs from the keyboard history file
    num_inputs = getNewInputsFromFile(input_buffer, 10);
    
    for (int i = 0; i < num_inputs; i++) {
        int key_code = input_buffer[i];
        
        // Check if it's a number key (0-9)
        if (key_code >= '0' && key_code <= '9') {
            handleNumberInput((char)key_code);
        }
        // Check if it's a WASD key
        else if ((key_code >= 'a' && key_code <= 'z') || (key_code >= 'A' && key_code <= 'Z')) {
            char key_char = (char)key_code;
            if (key_char == 'w' || key_char == 'W' || 
                key_char == 's' || key_char == 'S' ||
                key_char == 'a' || key_char == 'A' ||
                key_char == 'd' || key_char == 'D') {
                handleDirectionalInput(key_char);
            }
        }
        // Check if it's an arrow key code (1000-1003)
        else if (key_code >= 1000 && key_code <= 1003) {
            handleArrowInput(key_code);
        }
        // Check for Ctrl+C (key code 3)
        else if (key_code == 3) {
            printf("Control+C detected, system will exit\n");
            // This will be handled by the main orchestrator
        }
    }
    
    printf("Control plugin processed %d inputs\n", num_inputs);
    return 0;
}