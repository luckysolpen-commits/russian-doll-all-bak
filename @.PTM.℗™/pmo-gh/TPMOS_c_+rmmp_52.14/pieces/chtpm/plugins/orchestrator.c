#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifndef _WIN32
    #include <sys/wait.h>
    #include <signal.h>
    #include <termios.h>
#else
    #include <conio.h>
    #include <windows.h>
    #include <direct.h>
#endif
#include "../../system/win_compat.h"
#include "../../system/win_spawn.h"
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>

// TPM Orchestrator (v2.5 - INTEGRATED KEYBOARD INPUT)
// Responsibility: Launch core components AND handle keyboard input directly.
// Keyboard input is now handled in the main process to maintain terminal access.

char project_root[1024] = ".";

/* Keyboard input handling */
#define HISTORY_PATH "pieces/keyboard/history.txt"
#define LEDGER_PATH "pieces/keyboard/ledger.txt"

enum editorKey {
    ARROW_LEFT = 1000, ARROW_RIGHT = 1001, ARROW_UP = 1002, ARROW_DOWN = 1003, ESC_KEY = 27
};

#ifndef _WIN32
struct termios orig_termios;
#else
DWORD orig_console_mode = 0;
HANDLE h_console_input = INVALID_HANDLE_VALUE;
#endif
int tty_fd = -1;
volatile sig_atomic_t should_exit = 0;  /* Flag for Ctrl+C handling */

void disableRawMode() {
#ifndef _WIN32
    if (tty_fd >= 0) {
        tcsetattr(tty_fd, TCSAFLUSH, &orig_termios);
    }
#else
    // Restore console mode on Windows
    if (h_console_input != INVALID_HANDLE_VALUE) {
        SetConsoleMode(h_console_input, orig_console_mode);
        h_console_input = INVALID_HANDLE_VALUE;
    }
#endif
}

void handle_sigint(int sig) {
    should_exit = 1;
    disableRawMode();
    fprintf(stderr, "\n[orchestrator] Received signal %d, shutting down TPM components...\n", sig);

#ifndef _WIN32
    /* Kill all processes in the process group */
    kill(0, SIGTERM);

    /* Specifically kill GL renderer if running */
    system("pkill -9 -f 'gl_renderer' 2>/dev/null");

    /* Run the surgical cleanup script as a final sweep */
    system("./kill_all.sh > /dev/null 2>&1");
#else
    system("powershell.exe -File ./kill_all.ps1 > $null 2>&1");
#endif

    exit(0);
}

void enableRawMode() {
#ifndef _WIN32
    tty_fd = STDIN_FILENO;  /* Use stdin directly */
    tcgetattr(tty_fd, &orig_termios);
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;  /* 0.1 second timeout */
    tcsetattr(tty_fd, TCSAFLUSH, &raw);
#else
    /* Windows: Get console handle and store original mode */
    h_console_input = GetStdHandle(STD_INPUT_HANDLE);
    if (h_console_input != INVALID_HANDLE_VALUE) {
        GetConsoleMode(h_console_input, &orig_console_mode);
        /* Disable ENABLE_LINE_INPUT and ENABLE_ECHO_INPUT for raw-like behavior */
        DWORD mode = orig_console_mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
        SetConsoleMode(h_console_input, mode);
    }
#endif
}

int readKey() {
    int nread;
    char c;
#ifndef _WIN32
    while ((nread = read(tty_fd, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) return -1;
        usleep(10000);
    }
#else
    /* Windows: Use _kbhit() with a small delay to prevent CPU spinning */
    if (_kbhit()) {
        c = (char)_getch();
    } else {
        /* Small sleep to prevent CPU spinning */
        Sleep(100);  /* 100ms */
        return -1;
    }
#endif
    if (c == '\x1b') {
#ifndef _WIN32
        char seq[3];
        if (read(tty_fd, &seq[0], 1) != 1) return '\x1b';
        if (read(tty_fd, &seq[1], 1) != 1) return '\x1b';
        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
            }
        }
#else
        // In conio.h, arrows are often prefixed with 224 or 0
        c = _getch();
        switch (c) {
            case 72: return ARROW_UP;
            case 80: return ARROW_DOWN;
            case 77: return ARROW_RIGHT;
            case 75: return ARROW_LEFT;
        }
#endif
        return '\x1b';
    }
    return c;
}

void writeCommand(int key) {
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[100];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    FILE *kb_ledger = fopen(LEDGER_PATH, "a");
    if (kb_ledger) {
        char key_char = (key >= 32 && key <= 126) ? key : '?';
        fprintf(kb_ledger, "[%s] KeyCaptured: %d ('%c') | Source: orchestrator\n", timestamp, key, key_char);
        fclose(kb_ledger);
    }

    FILE *fp = fopen(HISTORY_PATH, "a");
    if (!fp) {
#ifdef _WIN32
        _mkdir("pieces\\keyboard");
#else
        system("mkdir -p pieces/keyboard");
#endif
        fp = fopen(HISTORY_PATH, "a");
    }
    if (fp) {
        fprintf(fp, "[%s] KEY_PRESSED: %d\n", timestamp, key);
        fclose(fp);
    }

    FILE *master = fopen("pieces/master_ledger/master_ledger.txt", "a");
    if (master) {
        fprintf(master, "[%s] InputReceived: key_code=%d | Source: orchestrator\n", timestamp, key);
        fclose(master);
    }
}

void* keyboard_thread_func(void* arg) {
    enableRawMode();
    while (!should_exit) {
        int c = readKey();
        if (c == -1) continue;
        if (c == 3) {  /* Ctrl+C - set exit flag */
            should_exit = 1;
            break;
        }
        writeCommand(c);
        usleep(10000);
    }
    return NULL;
}

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
    
#ifndef _WIN32
    pid_t p = fork();
    if (p == 0) {
        if (pid >= 0) {
            char pid_str[32]; snprintf(pid_str, sizeof(pid_str), "%d", pid);
            execl(pm_path, pm_path, action, name, pid_str, NULL);
        } else execl(pm_path, pm_path, action, name, NULL);
        exit(1);
    } else if (p > 0) waitpid(p, NULL, 0);
#else
    /* Windows: use spawn */
    if (pid >= 0) {
        char pid_str[32]; snprintf(pid_str, sizeof(pid_str), "%d", pid);
        _spawnl(_P_DETACH, pm_path, pm_path, action, name, pid_str, NULL);
    } else {
        _spawnl(_P_DETACH, pm_path, pm_path, action, name, NULL);
    }
#endif
}

void launch_and_register(const char* name, const char* path, const char* args, bool quiet) {
    char abs_path[1024];
    if (path[0] == '/') strcpy(abs_path, path);
    else build_path(abs_path, sizeof(abs_path), "%s/%s", project_root, path);

#ifndef _WIN32
    pid_t pid = fork();
    if (pid == 0) {
        if (project_root[0] != '\0') chdir(project_root);
        if (quiet) {
            freopen("/dev/null", "w", stdout);
            freopen("/dev/null", "w", stderr);
        }
        if (args && strlen(args) > 0) execl(abs_path, abs_path, args, NULL);
        else execl(abs_path, abs_path, NULL);
        exit(1);
    } else if (pid > 0) {
        proc_mgr_call("register", name, pid);
        int status; waitpid(pid, &status, 0);
        proc_mgr_call("unregister", name, -1);
    }
#else
    /* Windows: use spawn with working directory change */
    (void)quiet;
    int pid;
    /* Save current directory and change to project root */
    char old_cwd[MAX_PATH];
    _getcwd(old_cwd, sizeof(old_cwd));
    if (project_root[0] != '\0') _chdir(project_root);
    
    if (args && strlen(args) > 0) {
        pid = _spawnl(_P_DETACH, abs_path, abs_path, args, NULL);
    } else {
        pid = _spawnl(_P_DETACH, abs_path, abs_path, NULL);
    }
    
    /* Restore original directory */
    _chdir(old_cwd);
    
    if (pid > 0) {
        proc_mgr_call("register", name, pid);
        proc_mgr_call("unregister", name, -1);
    }
#endif
}

void* joystick_thread_func(void* arg) {
    /* Joystick runs forever - spawn detached */
#ifndef _WIN32
    pid_t pid = fork();
    if (pid == 0) {
        execl("./pieces/joystick/plugins/+x/joystick_input.+x", "joystick_input.+x", ".", NULL);
        exit(1);
    }
#else
    /* Windows: use spawn detached */
    _spawnl(_P_DETACH, "./pieces/joystick/plugins/+x/joystick_input.+x", "joystick_input.+x", ".", NULL);
#endif
    /* Parent: just return, don't wait */
    (void)arg;
    return NULL;
}
void* response_thread_func(void* arg) { launch_and_register("response_handler", "pieces/master_ledger/plugins/+x/response_handler.+x", NULL, true); return NULL; }
void* render_thread_func(void* arg) { launch_and_register("renderer", "pieces/display/plugins/+x/renderer.+x", NULL, false); return NULL; }
void* gl_render_thread_func(void* arg) { launch_and_register("gl_renderer", "pieces/display/plugins/+x/gl_renderer.+x", NULL, true); return NULL; }
void* clock_daemon_thread_func(void* arg) { launch_and_register("clock_daemon", "pieces/system/clock_daemon/plugins/+x/clock_daemon.+x", NULL, true); return NULL; }
void* chtpm_thread_func(void* arg) { launch_and_register("chtpm_parser", "pieces/chtpm/plugins/+x/chtpm_parser.+x", "pieces/chtpm/layouts/os.chtpm", true); return NULL; }

// Apps launched via <module> tag in CHTPM layouts (like HTML <script> tag)

int main() {
    resolve_root();
    proc_mgr_call("register", "orchestrator", getpid());
    
    /* Register signal handlers */
    signal(SIGINT, handle_sigint);
    signal(SIGHUP, handle_sigint);
    signal(SIGTERM, handle_sigint);

    /* Create keyboard history directory */
#ifdef _WIN32
    _mkdir("pieces\\keyboard");
    _mkdir("pieces\\joystick");
#else
    system("mkdir -p pieces/keyboard");
    system("mkdir -p pieces/joystick");
#endif

    pthread_t kb_t, joy_t, res_t, ren_t, gl_t, clock_t, parser_t;

    /* Start keyboard input in a thread (runs in main process, has terminal access) */
    pthread_create(&kb_t, NULL, keyboard_thread_func, NULL);
    
    /* Start joystick input thread */
    pthread_create(&joy_t, NULL, joystick_thread_func, NULL);

    pthread_create(&res_t, NULL, response_thread_func, NULL);
    pthread_create(&parser_t, NULL, chtpm_thread_func, NULL);

    usleep(50000);

    pthread_create(&ren_t, NULL, render_thread_func, NULL);
    pthread_create(&gl_t, NULL, gl_render_thread_func, NULL);
    pthread_create(&clock_t, NULL, clock_daemon_thread_func, NULL);

    /* Wait for keyboard thread - when it exits (Ctrl+C), we exit */
    pthread_join(kb_t, NULL);
    
    /* Cleanup and exit via signal handler to ensure all components die */
    handle_sigint(0);
}
