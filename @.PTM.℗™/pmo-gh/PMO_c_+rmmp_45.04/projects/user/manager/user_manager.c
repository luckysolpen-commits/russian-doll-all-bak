#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#define MAX_LINE 1024
#define MAX_PATH 1024

char project_root[MAX_PATH] = ".";
long last_history_pos = 0;

void resolve_root() {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[2048];
        while (fgets(line, sizeof(line), kvp)) {
            if (strncmp(line, "project_root=", 13) == 0) {
                char *v = line + 13;
                v[strcspn(v, "\n\r")] = 0;
                if (strlen(v) > 0) strncpy(project_root, v, MAX_PATH-1);
                break;
            }
        }
        fclose(kvp);
    }
}

void build_p_path(char* dst, size_t sz, const char* rel) {
    snprintf(dst, sz, "%s/%s", project_root, rel);
}

void save_user_state(const char* status, const char* user) {
    char path[MAX_PATH];
    build_p_path(path, sizeof(path), "projects/user/pieces/session/state.txt");
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "session_status=%s\n", status);
        fprintf(f, "current_user=%s\n", user);
        fclose(f);
    }
}

void handle_signup() {
    // Placeholder for actual signup logic
    // In a real implementation, we'd wait for username/password input via history.txt
    save_user_state("Signup Mode Active", "guest");
}

void handle_login() {
    // Placeholder for actual login logic
    save_user_state("Login Mode Active", "guest");
}

void process_history() {
    char path[MAX_PATH];
    build_p_path(path, sizeof(path), "projects/user/history.txt");
    FILE *fp = fopen(path, "r");
    if (!fp) return;
    
    fseek(fp, last_history_pos, SEEK_SET);
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        int key = atoi(line);
        if (key == '1') handle_login();
        else if (key == '2') handle_signup();
        last_history_pos = ftell(fp);
    }
    fclose(fp);
}

int main() {
    resolve_root();
    save_user_state("Ready", "guest");
    
    while (1) {
        process_history();
        usleep(100000);
    }
    return 0;
}
