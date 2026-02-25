#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

// Maximum length for lines in the piece file
#define MAX_LINE_LENGTH 500
#define MAX_STATE_KEYS 50
#define MAX_RESPONSES 20
#define MAX_LISTENERS 10
#define MAX_METHODS 10

// Structure to hold parsed piece data
typedef struct {
    char state_keys[MAX_STATE_KEYS][50];
    char state_values[MAX_STATE_KEYS][100];
    int state_count;
    
    char responses[MAX_RESPONSES][50][100];  // [index][key][value]
    int response_count;
    
    char event_listeners[MAX_LISTENERS][50];
    int listener_count;
    
    char methods[MAX_METHODS][50];
    int method_count;
} PieceData;

// Function to trim whitespace from beginning and end of string
char* trim(char *str) {
    while(isspace(*str)) str++;
    if(*str == 0) return str;
    
    char *end = str + strlen(str) - 1;
    while(end > str && isspace(*end)) end--;
    *(end + 1) = '\0';
    return str;
}

// Function to check if line is a section marker (ends with colon)
int isSection(const char *line) {
    int len = strlen(line);
    return (len > 0 && line[len-1] == ':');
}

// Function to extract section name
char* getSectionName(const char *line) {
    static char section_name[100];
    strcpy(section_name, line);
    int len = strlen(section_name);
    if(len > 0 && section_name[len-1] == ':') {
        section_name[len-1] = '\0';
    }
    return section_name;
}

// Function to split comma-separated values
int splitCommaSeparated(const char *input, char result[][100], int max_items) {
    char temp[1000];
    strcpy(temp, input);
    
    int count = 0;
    char *token = strtok(temp, ",");
    
    while(token != NULL && count < max_items) {
        strcpy(result[count], trim(token));
        count++;
        token = strtok(NULL, ",");
    }
    
    return count;
}

// Function to parse PDLO file (Piece Definition Language)
// Format: SECTION      | KEY                | VALUE
int parsePieceFile(const char *filepath, PieceData *data) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open PDLO file %s\n", filepath);
        return 0;
    }
    
    // Initialize structure
    data->state_count = 0;
    data->response_count = 0;
    data->listener_count = 0;
    data->method_count = 0;
    
    char line[MAX_LINE_LENGTH];
    
    while (fgets(line, sizeof(line), file)) {
        // Remove trailing newline
        line[strcspn(line, "\n\r")] = 0;
        
        // Skip empty lines, comments, and header/separator lines
        if (strlen(trim(line)) == 0 || line[0] == '#' || line[0] == '-') {
            continue;
        }
        
        // Check if line is a section header (SECTION | KEY | VALUE)
        if (strncmp(line, "SECTION", 7) == 0) {
            continue;  // Skip header line
        }
        
        // Parse PDLO format: SECTION      | KEY                | VALUE
        char section[50] = "", key[100] = "", value[200] = "";
        if (sscanf(line, "%49[^|]|%99[^|]|%199[^\n]", section, key, value) == 3) {
            // Trim whitespace
            char* s = trim(section);
            char* k = trim(key);
            char* v = trim(value);
            
            if (strcmp(s, "STATE") == 0) {
                strcpy(data->state_keys[data->state_count], k);
                strcpy(data->state_values[data->state_count], v);
                data->state_count++;
            }
            else if (strcmp(s, "METHOD") == 0) {
                strcpy(data->methods[data->method_count++], k);
            }
            else if (strcmp(s, "EVENT_IN") == 0) {
                strcpy(data->event_listeners[data->listener_count++], k);
            }
            else if (strcmp(s, "RESPONSE") == 0) {
                strcpy(data->responses[data->response_count][0], k);
                strcpy(data->responses[data->response_count][1], v);
                data->response_count++;
            }
        }
    }
    
    fclose(file);
    return 1;
}

// Function to get a state value by key
char* getStateValue(PieceData *data, const char *key) {
    for (int i = 0; i < data->state_count; i++) {
        if (strcmp(data->state_keys[i], key) == 0) {
            return data->state_values[i];
        }
    }
    return NULL;
}

// Function to set a state value
int setStateValue(PieceData *data, const char *key, const char *value) {
    for (int i = 0; i < data->state_count; i++) {
        if (strcmp(data->state_keys[i], key) == 0) {
            strcpy(data->state_values[i], value);
            return 1;  // Found and updated
        }
    }
    
    // Key not found, add new one if space available
    if (data->state_count < MAX_STATE_KEYS) {
        strcpy(data->state_keys[data->state_count], key);
        strcpy(data->state_values[data->state_count], value);
        data->state_count++;
        return 1;
    }
    
    return 0;  // Failed to update
}

// Function to get a response by key
char* getResponse(PieceData *data, const char *key) {
    for (int i = 0; i < data->response_count; i++) {
        if (strcmp(data->responses[i][0], key) == 0) {
            return data->responses[i][1];
        }
    }
    return NULL;
}

// Function to check if a method exists
int hasMethod(PieceData *data, const char *method) {
    for (int i = 0; i < data->method_count; i++) {
        if (strcmp(data->methods[i], method) == 0) {
            return 1;
        }
    }
    return 0;
}

// Function to check if an event listener exists
int hasEventListener(PieceData *data, const char *event) {
    for (int i = 0; i < data->listener_count; i++) {
        if (strcmp(data->event_listeners[i], event) == 0) {
            return 1;
        }
    }
    return 0;
}

// Function to log state changes to master ledger
void logStateChange(const char *piece_id, const char *key, const char *value) {
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[100];
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    FILE *ledger = fopen("pieces/master_ledger/master_ledger.txt", "a");
    if (ledger) {
        fprintf(ledger, "[%s] StateChange: %s %s %s | Trigger: piece_manager_set_state\n", timestamp, piece_id, key, value);
        fclose(ledger);
    }
}

// Main function
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <piece_id> [action]\n", argv[0]);
        fprintf(stderr, "Actions: get-state, set-state, get-response, list-methods, list-listeners, get-name\n");
        fprintf(stderr, "Example: %s fuzzpet get-state hunger\n", argv[0]);
        return 1;
    }
    
    const char *piece_id = argv[1];
    char piece_file[500];
    snprintf(piece_file, sizeof(piece_file), "pieces/%s/%s.pdl", piece_id, piece_id);
    
    PieceData data;
    
    if (!parsePieceFile(piece_file, &data)) {
        fprintf(stderr, "Could not parse piece file: %s\n", piece_file);
        return 1;
    }
    
    if (argc < 3) {
        // Default: Print parsed data
        printf("Parsed piece data:\n");
        printf("State (%d keys):\n", data.state_count);
        for (int i = 0; i < data.state_count; i++) {
            printf("  %s: %s\n", data.state_keys[i], data.state_values[i]);
        }
        
        printf("Methods (%d):\n", data.method_count);
        for (int i = 0; i < data.method_count; i++) {
            printf("  - %s\n", data.methods[i]);
        }
        
        printf("Event Listeners (%d):\n", data.listener_count);
        for (int i = 0; i < data.listener_count; i++) {
            printf("  - %s\n", data.event_listeners[i]);
        }
        
        printf("Responses (%d):\n", data.response_count);
        for (int i = 0; i < data.response_count; i++) {
            printf("  %s: %s\n", data.responses[i][0], data.responses[i][1]);
        }
    }
    else {
        const char *action = argv[2];
        
        if (strcmp(action, "get-state") == 0 && argc >= 4) {
            const char *key = argv[3];
            char *value = getStateValue(&data, key);
            if (value) {
                printf("%s", value);
            } else {
                printf("STATE_NOT_FOUND");
            }
        }
        else if (strcmp(action, "set-state") == 0 && argc >= 5) {
            const char *key = argv[3];
            const char *value = argv[4];
            if (setStateValue(&data, key, value)) {
                // Also write changes back to PDLO file
                FILE *file = fopen(piece_file, "w");
                if (file) {
                    // Write PDLO header
                    fprintf(file, "SECTION      | KEY                | VALUE\n");
                    fprintf(file, "----------------------------------------\n");
                    
                    // Write META section
                    fprintf(file, "META         | piece_id           | %s\n", piece_id);
                    fprintf(file, "META         | version            | 1.0\n");
                    fprintf(file, "META         | determinism        | strict\n\n");
                    
                    // Write STATE section
                    for (int i = 0; i < data.state_count; i++) {
                        fprintf(file, "STATE        | %-20s | %s\n", data.state_keys[i], data.state_values[i]);
                    }
                    fprintf(file, "\n");
                    
                    // Write METHOD section
                    for (int i = 0; i < data.method_count; i++) {
                        fprintf(file, "METHOD       | %-20s | void\n", data.methods[i]);
                    }
                    fprintf(file, "\n");
                    
                    // Write EVENT_IN section
                    for (int i = 0; i < data.listener_count; i++) {
                        fprintf(file, "EVENT_IN     | %-20s | void\n", data.event_listeners[i]);
                    }
                    fprintf(file, "\n");
                    
                    // Write RESPONSE section
                    for (int i = 0; i < data.response_count; i++) {
                        fprintf(file, "RESPONSE     | %-20s | %s\n", data.responses[i][0], data.responses[i][1]);
                    }
                    
                    fclose(file);
                    
                    // Log the state change to master ledger
                    logStateChange(piece_id, key, value);
                }
            } else {
                fprintf(stderr, "Failed to set state\n");
                return 1;
            }
        }
        else if (strcmp(action, "get-response") == 0 && argc >= 4) {
            const char *key = argv[3];
            char *response = getResponse(&data, key);
            if (response) {
                printf("%s", response);
            } else {
                char *def_response = getResponse(&data, "default");
                if (def_response) {
                    printf("%s", def_response);
                } else {
                    printf("*confused* ...what?");
                }
            }
        }
        else if (strcmp(action, "has-method") == 0 && argc >= 4) {
            const char *method = argv[3];
            if (hasMethod(&data, method)) {
                printf("1");
            } else {
                printf("0");
            }
        }
        else if (strcmp(action, "has-listener") == 0 && argc >= 4) {
            const char *event = argv[3];
            if (hasEventListener(&data, event)) {
                printf("1");
            } else {
                printf("0");
            }
        }
        else if (strcmp(action, "list-methods") == 0) {
            for (int i = 0; i < data.method_count; i++) {
                printf("%s\n", data.methods[i]);
            }
        }
        else if (strcmp(action, "list-listeners") == 0) {
            for (int i = 0; i < data.listener_count; i++) {
                printf("%s\n", data.event_listeners[i]);
            }
        }
        else if (strcmp(action, "get-name") == 0) {
            char *name = getStateValue(&data, "name");
            if (name) {
                printf("%s", name);
            } else {
                printf("Pet");
            }
        }
        else {
            fprintf(stderr, "Unknown action: %s\n", action);
            return 1;
        }
    }
    
    return 0;
}