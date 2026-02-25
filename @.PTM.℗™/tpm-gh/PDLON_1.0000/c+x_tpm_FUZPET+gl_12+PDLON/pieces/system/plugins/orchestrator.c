#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// Function prototypes
void* keyboard_thread_func(void* arg);
void* game_thread_func(void* arg);
void* render_thread_func(void* arg);
void* response_thread_func(void* arg);
void* gl_render_thread_func(void* arg);

int main() {
    pthread_t kb_thread, game_thread, render_thread, response_thread, gl_render_thread;
    
    // Create threads for each component
    if (pthread_create(&kb_thread, NULL, keyboard_thread_func, NULL) ||
        pthread_create(&game_thread, NULL, game_thread_func, NULL) ||
        pthread_create(&render_thread, NULL, render_thread_func, NULL) ||
        pthread_create(&response_thread, NULL, response_thread_func, NULL) ||
        pthread_create(&gl_render_thread, NULL, gl_render_thread_func, NULL)) {
        exit(1);
    }
    
    // Wait for all threads (though these processes typically run indefinitely)
    pthread_join(kb_thread, NULL);
    pthread_join(game_thread, NULL);
    pthread_join(render_thread, NULL);
    pthread_join(response_thread, NULL);
    pthread_join(gl_render_thread, NULL);
    
    pthread_exit(NULL);
    return 0;
}

// Keyboard thread function
void* keyboard_thread_func(void* arg) {
    system("./pieces/keyboard/plugins/+x/keyboard_input.+x");
    pthread_exit(NULL);
}

// Game logic thread function
void* game_thread_func(void* arg) {
    system("./pieces/world/plugins/+x/game_logic.+x");
    pthread_exit(NULL);
}

// Render thread function
void* render_thread_func(void* arg) {
    system("./pieces/display/plugins/+x/renderer.+x");
    pthread_exit(NULL);
}

// Response handler thread function
void* response_thread_func(void* arg) {
    system("./pieces/master_ledger/plugins/+x/response_handler.+x");
    pthread_exit(NULL);
}

// GL Renderer thread function
void* gl_render_thread_func(void* arg) {
    system("./pieces/display/plugins/+x/gl_renderer.+x");
    pthread_exit(NULL);
}
