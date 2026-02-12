#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// Function prototypes
void* keyboard_thread_func(void* arg);
void* game_thread_func(void* arg);
void* render_thread_func(void* arg);

int main() {
    pthread_t kb_thread, game_thread, render_thread;
 /*   
    printf("=========================================\n");
    printf("Pure Platform Orchestrator Starting...\n");
    printf("This orchestrator follows TPM +X standards:\n");
    printf("- Orchestrator only manages threads\n");
    printf("- No business logic (TPM +X compliance)\n");
    printf("=========================================\n\n");
    */
    // Create threads for each component
    if (pthread_create(&kb_thread, NULL, keyboard_thread_func, NULL) ||
        pthread_create(&game_thread, NULL, game_thread_func, NULL) ||
        pthread_create(&render_thread, NULL, render_thread_func, NULL)) {
      //  printf("Error creating threads\n");
        exit(1);
    }
    
    /*
    printf("All threads created successfully\n");
    printf("Components are now running: keyboard input, game logic, display rendering\n\n");
   */ 
    // Wait for all threads (though these processes typically run indefinitely)
    pthread_join(kb_thread, NULL);
    pthread_join(game_thread, NULL);
    pthread_join(render_thread, NULL);
    
    pthread_exit(NULL);
    return 0;
}

// Keyboard thread function
void* keyboard_thread_func(void* arg) {
    system("./+x/keyboard_input.+x");
    pthread_exit(NULL);
}

// Game logic thread function
void* game_thread_func(void* arg) {
    system("./+x/game_logic.+x");
    pthread_exit(NULL);
}

// Render thread function
void* render_thread_func(void* arg) {
    system("./+x/renderer.+x");
    pthread_exit(NULL);
}
