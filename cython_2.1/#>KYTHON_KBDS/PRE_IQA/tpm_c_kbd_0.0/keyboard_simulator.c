/*** keyboard_simulator.c - TPM Compliant Keyboard Simulator ***/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define HISTORY_FILE "peices/input/keyboard/history.txt"

// Function to append a key to the history file
void appendToHistory(const char* key_str) {
    FILE* fp = fopen(HISTORY_FILE, "a");
    if (fp) {
        fprintf(fp, "%s\n", key_str);
        fclose(fp);
    }
}

int main() {
    printf("Starting TPM Keyboard Simulator...\n");
    printf("Simulating keyboard input by writing to history.txt\n");
    printf("Press Ctrl+C to quit\n\n");
    
    // Clear history file on startup
    FILE* fp = fopen(HISTORY_FILE, "w");
    if (fp) {
        fclose(fp);
    }
    
    // Simulate typing "hello" followed by enter
    char* simulated_input[] = {"h", "e", "l", "l", "o", "[ENTER]", "w", "o", "r", "l", "d", "[ENTER]"};
    int num_inputs = sizeof(simulated_input) / sizeof(simulated_input[0]);
    
    int i = 0;
    while (1) {
        if (i < num_inputs) {
            appendToHistory(simulated_input[i]);
            printf("Added: %s\n", simulated_input[i]);
            i++;
            sleep(1); // Wait 1 second between inputs
        } else {
            // Cycle back to beginning after completing the sequence
            sleep(2);
            i = 0;  // Reset to replay the sequence
        }
    }
    
    return 0;
}