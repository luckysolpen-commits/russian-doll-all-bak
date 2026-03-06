#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// increment_turn - Advances turn AND time
// 
// MODE LOGIC:
// - AUTO mode: Turn increments, time also advances by 1 second
// - TURN mode: Turn increments, time also advances by 1 second
// 
// Philosophy: In turn mode, "turn" = action count. Each action takes time.
// The daemon doesn't tick in turn mode, but actions still consume time.

#define MAX_LINE 256
#define MAX_BUF 1024

char project_root[MAX_BUF] = ".";

void resolve_root() {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), kvp)) {
            if (strncmp(line, "project_root=", 13) == 0) {
                char *v = line + 13;
                v[strcspn(v, "\n\r")] = 0;
                // FIX: Use snprintf for guaranteed null-termination
                snprintf(project_root, sizeof(project_root), "%s", v);
                break;
            }
        }
        fclose(kvp);
    }
}

void increment_time_str(char *time_str) {
    int h, m, s;
    if (sscanf(time_str, "%d:%d:%d", &h, &m, &s) == 3) {
        s++;
        if (s >= 60) { s = 0; m++; }
        if (m >= 60) { m = 0; h++; }
        if (h >= 24) { h = 0; }
        snprintf(time_str, 64, "%02d:%02d:%02d", h, m, s);
    }
}

void call_piece_manager(const char *piece_id, const char *key, const char *value) {
    char *pm_path;
    asprintf(&pm_path, "%s/pieces/master_ledger/plugins/+x/piece_manager.+x", project_root);
    if (!pm_path) return;
    
    pid_t p = fork();
    if (p == 0) {
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
        execl(pm_path, pm_path, piece_id, "set-state", key, value, NULL);
        exit(1);
    } else if (p > 0) {
        waitpid(p, NULL, 0);
    }
    free(pm_path);
}

void hit_frame_marker() {
    char *path;
    asprintf(&path, "%s/pieces/master_ledger/frame_changed.txt", project_root);
    if (path) {
        FILE *f = fopen(path, "a");
        if (f) {
            fprintf(f, "X\n");
            fclose(f);
        }
        free(path);
    }
}

int main() {
    resolve_root();
    
    char *state_path;
    asprintf(&state_path, "%s/pieces/system/clock_daemon/state.txt", project_root);
    if (!state_path) return 1;
    
    // Read current state
    FILE *f = fopen(state_path, "r");
    if (!f) {
        fprintf(stderr, "Cannot open %s\n", state_path);
        free(state_path);
        return 1;
    }
    
    char turn[64] = "0";
    char time_str[64] = "08:00:00";
    char mode[64] = "auto";
    char line[MAX_LINE];
    
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n\r")] = 0;
        char *eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';
            char *k = line;
            char *v = eq + 1;
            if (strcmp(k, "turn") == 0) strncpy(turn, v, 63);
            else if (strcmp(k, "time") == 0) strncpy(time_str, v, 63);
            else if (strcmp(k, "mode") == 0) strncpy(mode, v, 63);
        }
    }
    fclose(f);
    
    // Increment turn
    int turn_val = atoi(turn);
    turn_val++;
    snprintf(turn, sizeof(turn), "%d", turn_val);
    
    // Also increment time (each action takes time)
    increment_time_str(time_str);
    
    // Write updated state
    f = fopen(state_path, "w");
    if (f) {
        fprintf(f, "turn=%s\n", turn);
        fprintf(f, "time=%s\n", time_str);
        fprintf(f, "mode=%s\n", mode);
        fclose(f);
    }
    
    // Sync via piece_manager for DNA update
    call_piece_manager("clock_daemon", "turn", turn);
    call_piece_manager("clock_daemon", "time", time_str);
    
    // Hit frame marker to trigger re-render
    hit_frame_marker();
    
    free(state_path);
    return 0;
}
