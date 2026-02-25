#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

void* keyboard_thread_func(void* arg) {
    system("./pieces/keyboard/plugins/+x/keyboard_input.+x > /dev/null 2>&1");
    pthread_exit(NULL);
}

void* response_thread_func(void* arg) {
    system("./pieces/master_ledger/plugins/+x/response_handler.+x > /dev/null 2>&1");
    pthread_exit(NULL);
}

void* render_thread_func(void* arg) {
    system("./pieces/display/plugins/+x/renderer.+x");
    pthread_exit(NULL);
}

void* gl_render_thread_func(void* arg) {
    system("./pieces/display/plugins/+x/gl_renderer.+x > /dev/null 2>&1");
    pthread_exit(NULL);
}

void* chtpm_thread_func(void* arg) {
    system("./pieces/chtpm/plugins/+x/chtpm_parser.+x pieces/chtpm/layouts/os.chtpm");
    pthread_exit(NULL);
}

int main() {
    pthread_t kb_t, res_t, ren_t, gl_t, parser_t;
    
    pthread_create(&kb_t, NULL, keyboard_thread_func, NULL);
    pthread_create(&res_t, NULL, response_thread_func, NULL);
    pthread_create(&parser_t, NULL, chtpm_thread_func, NULL);
    
    usleep(200000);
    
    pthread_create(&ren_t, NULL, render_thread_func, NULL);
    pthread_create(&gl_t, NULL, gl_render_thread_func, NULL);
    
    pthread_join(kb_t, NULL);
    pthread_join(res_t, NULL);
    pthread_join(parser_t, NULL);
    pthread_join(ren_t, NULL);
    pthread_join(gl_t, NULL);
    
    return 0;
}
