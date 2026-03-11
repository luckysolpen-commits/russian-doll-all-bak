#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

// TPM Orchestrator (v2.3 - FUZZPET MANAGER INTEGRATED)
// Responsibility: Launch core components AND application managers.

char project_root[1024] = ".";

void build_path(char* dst, size_t sz, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(dst, sz, fmt, args);
    va_end(args);
}

void resolve_root() {
    getcwd(project_root, sizeof(project_root));
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[2048];
        while (fgets(line, sizeof(line), kvp)) {
            if (strncmp(line, "project_root=", 13) == 0) {
                char *v = line + 13;
                v[strcspn(v, "\n\r")] = 0;
                if (strlen(v) > 0) snprintf(project_root, sizeof(project_root), "%s", v);
                break;
            }
        }
        fclose(kvp);
    }
}

void proc_mgr_call(const char* action, const char* name, int pid) {
    char pm_path[2048];
    snprintf(pm_path, sizeof(pm_path), "%s/pieces/os/plugins/+x/proc_manager.+x", project_root);
    pid_t p = fork();
    if (p == 0) {
        if (pid >= 0) {
            char pid_str[32]; snprintf(pid_str, sizeof(pid_str), "%d", pid);
            execl(pm_path, pm_path, action, name, pid_str, NULL);
        } else execl(pm_path, pm_path, action, name, NULL);
        exit(1);
    } else if (p > 0) waitpid(p, NULL, 0);
}

void launch_and_register(const char* name, const char* path, const char* args, bool quiet) {
    char abs_path[1024];
    if (path[0] == '/') strcpy(abs_path, path);
    else build_path(abs_path, sizeof(abs_path), "%s/%s", project_root, path);

    pid_t pid = fork();
    if (pid == 0) {
        if (project_root[0] != '\0') chdir(project_root);
        if (quiet) { freopen("/dev/null", "w", stdout); freopen("/dev/null", "w", stderr); }
        if (args && strlen(args) > 0) execl(abs_path, abs_path, args, NULL);
        else execl(abs_path, abs_path, NULL);
        exit(1);
    } else if (pid > 0) {
        proc_mgr_call("register", name, pid);
        int status; waitpid(pid, &status, 0);
        proc_mgr_call("unregister", name, -1);
    }
}

void* keyboard_thread_func(void* arg) { launch_and_register("keyboard_input", "pieces/keyboard/plugins/+x/keyboard_input.+x", NULL, true); return NULL; }
void* response_thread_func(void* arg) { launch_and_register("response_handler", "pieces/master_ledger/plugins/+x/response_handler.+x", NULL, true); return NULL; }
void* render_thread_func(void* arg) { launch_and_register("renderer", "pieces/display/plugins/+x/renderer.+x", NULL, false); return NULL; }
void* gl_render_thread_func(void* arg) { launch_and_register("gl_renderer", "pieces/display/plugins/+x/gl_renderer.+x", NULL, true); return NULL; }
void* clock_daemon_thread_func(void* arg) { launch_and_register("clock_daemon", "pieces/system/clock_daemon/plugins/+x/clock_daemon.+x", NULL, true); return NULL; }
void* chtpm_thread_func(void* arg) { launch_and_register("chtpm_parser", "pieces/chtpm/plugins/+x/chtpm_parser.+x", "pieces/chtpm/layouts/os.chtpm", true); return NULL; }

// Apps launched via <module> tag in CHTPM layouts (like HTML <script> tag)

int main() {
    resolve_root();
    proc_mgr_call("register", "orchestrator", getpid());

    pthread_t kb_t, res_t, ren_t, gl_t, clock_t, parser_t;
    
    pthread_create(&kb_t, NULL, keyboard_thread_func, NULL);
    pthread_create(&res_t, NULL, response_thread_func, NULL);
    pthread_create(&parser_t, NULL, chtpm_thread_func, NULL);
    
    usleep(200000);
    
    pthread_create(&ren_t, NULL, render_thread_func, NULL);
    pthread_create(&gl_t, NULL, gl_render_thread_func, NULL);
    pthread_create(&clock_t, NULL, clock_daemon_thread_func, NULL);
    
    pthread_join(kb_t, NULL);
    pthread_join(res_t, NULL);
    pthread_join(parser_t, NULL);
    pthread_join(ren_t, NULL);
    pthread_join(gl_t, NULL);
    pthread_join(clock_t, NULL);
    
    return 0;
}
