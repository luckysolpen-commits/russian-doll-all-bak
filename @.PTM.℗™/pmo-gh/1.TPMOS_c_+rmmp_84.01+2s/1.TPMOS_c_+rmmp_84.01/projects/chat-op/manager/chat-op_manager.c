#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>

#define MODULE_NAME "chat-op"
#define MAX_PATH 4096
#define MAX_LINE 1024
#define MAX_MSG_LEN 256
#define MAX_HISTORY_LEN 4096

static char project_root[MAX_PATH] = ".";
static char current_project[MAX_LINE] = "chat-op";
static char active_target_id[64] = "selector";
static char last_key_str[32] = "None";
static char current_input_buffer[MAX_MSG_LEN] = "";
static char gui_state_path[MAX_PATH] = "projects/chat-op/manager/gui_state.txt";

static volatile sig_atomic_t g_shutdown = 0;

static void handle_sigint(int sig) {
    (void)sig;
    g_shutdown = 1;
}

static int run_command(const char* cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
        execl("/bin/sh", "/bin/sh", "-c", cmd, (char *)NULL);
        _exit(127);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
    return -1;
}

static char* trim_str(char *str) {
    char *end;
    if (!str) return str;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

static void resolve_paths(void) {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), kvp)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(line);
                char *v = trim_str(eq + 1);
                if (strcmp(k, "project_root") == 0) {
                    snprintf(project_root, sizeof(project_root), "%s", v);
                }
            }
        }
        fclose(kvp);
    }
    snprintf(gui_state_path, sizeof(gui_state_path), "%s/projects/chat-op/manager/gui_state.txt", project_root);
}

static int is_active_layout(void) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/display/current_layout.txt", project_root) == -1) return 0;
    FILE *f = fopen(path, "r");
    if (!f) { free(path); return 0; }
    char line[MAX_LINE];
    int result = 0;
    if (fgets(line, sizeof(line), f)) {
        result = (strstr(line, "chat-op.chtpm") != NULL);
    }
    fclose(f);
    free(path);
    return result;
}

static void trigger_render(void) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/display/frame_changed.txt", project_root) != -1) {
        FILE *f = fopen(path, "a");
        if (f) { fprintf(f, "M\n"); fclose(f); }
        free(path);
    }
}

static void append_to_chat_log(const char* msg) {
    char *path = NULL;
    if (asprintf(&path, "%s/projects/chat-op/chat_log.txt", project_root) != -1) {
        FILE *f = fopen(path, "a");
        if (f) {
            time_t now = time(NULL);
            char *ts = ctime(&now);
            ts[strlen(ts) - 1] = '\0';
            fprintf(f, "[%s] %s\n", ts, msg);
            fclose(f);
        }
        free(path);
    }
}

static void update_gui_state(const char* messages, const char* input) {
    FILE *f = fopen(gui_state_path, "w");
    if (f) {
        if (messages) fprintf(f, "messages=%s\n", messages);
        if (input) fprintf(f, "input_text=%s\n", input);
        fclose(f);
    }
}

static void save_manager_state(void) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/player_app/manager/state.txt", project_root) != -1) {
        FILE *f = fopen(path, "w");
        if (f) {
            fprintf(f, "project_id=%s\nactive_target_id=%s\nlast_key=%s\n", 
                    current_project, active_target_id, last_key_str);
            fclose(f);
        }
        free(path);
    }
}

static void process_key(int key) {
    if (key == 10 || key == 13) { /* Enter key */
        char input_text_from_gui[MAX_MSG_LEN] = "";
        char line[MAX_LINE];
        char temp_messages[MAX_HISTORY_LEN] = "";

        // Read current state from gui_state.txt to get actual message content
        FILE *f = fopen(gui_state_path, "r");
        if (f) {
            while (fgets(line, sizeof(line), f)) {
                char *eq = strchr(line, '=');
                if (eq) {
                    *eq = '\0';
                    char *k = trim_str(line);
                    char *v = trim_str(eq + 1);
                    if (strcmp(k, "messages") == 0) {
                        strncpy(temp_messages, v, sizeof(temp_messages) - 1);
                        temp_messages[sizeof(temp_messages) - 1] = '\0';
                    } else if (strcmp(k, "input_text") == 0) {
                        strncpy(input_text_from_gui, v, sizeof(input_text_from_gui) - 1);
                        input_text_from_gui[sizeof(input_text_from_gui) - 1] = '\0';
                    }
                }
            }
            fclose(f);
        }

        if (strlen(input_text_from_gui) > 0) {
            // Append the message read from gui_state.txt to chat log
            append_to_chat_log(input_text_from_gui);

            // Build new_messages: existing + new message
            char new_messages[MAX_HISTORY_LEN];
            if (strlen(temp_messages) > 0) {
                snprintf(new_messages, sizeof(new_messages), "%s\\n%s", temp_messages, input_text_from_gui);
            } else {
                snprintf(new_messages, sizeof(new_messages), "%s", input_text_from_gui);
            }
            new_messages[sizeof(new_messages) - 1] = '\0';

            // Write updated state back to gui_state.txt
            update_gui_state(new_messages, "");

            // Clear manager's local buffer for consistency
            current_input_buffer[0] = '\0';

            save_manager_state();
            trigger_render();
        }
        // stay active! removing active_index = -1;
    } else if (key >= 32 && key <= 126) { /* Printable ASCII */
        size_t len = strlen(current_input_buffer);
        if (len < MAX_MSG_LEN - 1) {
            current_input_buffer[len] = (char)key;
            current_input_buffer[len + 1] = '\0';
            // In live mode, the parser might be writing gui_state.txt
            // but we sync here just in case.
            // update_gui_state(NULL, current_input_buffer);
            // trigger_render();
        }
    } else if (key == 8 || key == 127) { /* Backspace */
        size_t len = strlen(current_input_buffer);
        if (len > 0) {
            current_input_buffer[len - 1] = '\0';
            // update_gui_state(NULL, current_input_buffer);
            // trigger_render();
        }
    }
    snprintf(last_key_str, sizeof(last_key_str), "KEY:%d", key);
}

int main(void) {
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);
    setpgid(0, 0);
    resolve_paths();

    char *hist_path = NULL;
    if (asprintf(&hist_path, "%s/pieces/apps/player_app/history.txt", project_root) == -1) return 1;

    long last_pos = 0;
    struct stat st;

    while (!g_shutdown) {
        if (!is_active_layout()) {
            usleep(100000);
            continue;
        }

        if (stat(hist_path, &st) == 0) {
            if (st.st_size > last_pos) {
                FILE *hf = fopen(hist_path, "r");
                if (hf) {
                    fseek(hf, last_pos, SEEK_SET);
                    int key;
                    while (fscanf(hf, "%d", &key) == 1) {
                        process_key(key);
                        save_manager_state();
                    }
                    last_pos = ftell(hf);
                    fclose(hf);
                }
            } else if (st.st_size < last_pos) last_pos = 0;
        }
        usleep(16667);
    }
    free(hist_path);
    return 0;
}
