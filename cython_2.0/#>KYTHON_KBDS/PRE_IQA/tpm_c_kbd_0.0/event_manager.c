/*** event_manager.c - TPM Compliant Event Manager ***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_EVENTS 100
#define MAX_LISTENERS 50
#define MAX_EVENT_NAME 256

typedef struct {
    char event_name[MAX_EVENT_NAME];
    char piece_category[256];
    char piece_name[256];
    char method_name[256];
    int timestamp;
} Event;

Event event_queue[MAX_EVENTS];
int event_count = 0;

// Initialize the event manager
void initEventManager() {
    event_count = 0;
}

// Add an event to the queue
void triggerEvent(const char* event_name, const char* category, const char* piece_name, const char* method_name) {
    if (event_count >= MAX_EVENTS) return;
    
    Event* evt = &event_queue[event_count];
    strcpy(evt->event_name, event_name);
    strcpy(evt->piece_category, category);
    strcpy(evt->piece_name, piece_name);
    strcpy(evt->method_name, method_name);
    evt->timestamp = (int)time(NULL);
    
    event_count++;
}

// Process events - dummy implementation for now
void processEvents() {
    // In a real implementation, this would call the appropriate methods on pieces
    // For now, we'll just clear the queue after processing
    event_count = 0;
}

// Check if an event was triggered recently
int wasEventTriggered(const char* event_name) {
    for (int i = 0; i < event_count; i++) {
        if (strcmp(event_queue[i].event_name, event_name) == 0) {
            return 1;
        }
    }
    return 0;
}

// Process piece changes and trigger appropriate events
void monitorPieceChanges(const char* category, const char* piece_name) {
    // In a real implementation, we would compare current state to previous state
    // and trigger events based on differences
    // For example: trigger "keyboard_input_received" when input buffer changes
    // trigger "history_updated" when history file changes, etc.
    
    // For now we'll just trigger a generic event to indicate the piece changed
    triggerEvent("piece_changed", category, piece_name, "update_display");
}