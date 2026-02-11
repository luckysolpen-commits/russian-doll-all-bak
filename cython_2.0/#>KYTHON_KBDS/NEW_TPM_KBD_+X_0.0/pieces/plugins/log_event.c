#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <event_type> <description> [source_info]\n", argv[0]);
        fprintf(stderr, "Example: %s MethodCall \"feed on fuzzpet\" \"Caller: user_input\"\n", argv[0]);
        return 1;
    }
    
    const char *event_type = argv[1];
    const char *description = argv[2];
    const char *source_info = (argc > 3) ? argv[3] : "Source: unknown";
    
    // Get current timestamp
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[100];
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    // Open master ledger for appending
    FILE *ledger = fopen("pieces/master_ledger/master_ledger.txt", "a");
    if (!ledger) {
        fprintf(stderr, "Error: Could not open master ledger for writing\n");
        return 1;
    }
    
    // Write entry to the ledger
    fprintf(ledger, "[%s] %s: %s | %s\n", timestamp, event_type, description, source_info);
    fclose(ledger);
    
    return 0;
}