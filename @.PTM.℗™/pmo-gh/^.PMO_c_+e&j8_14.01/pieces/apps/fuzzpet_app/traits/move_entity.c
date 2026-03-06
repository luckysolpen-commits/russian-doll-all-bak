#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/wait.h>

#define MAX_PATH 4096

char project_root[MAX_PATH] = ".";

void resolve_root() {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[2048];
        while (fgets(line, sizeof(line), kvp)) {
            if (strncmp(line, "project_root=", 13) == 0) {
                char *v = line + 13;
                v[strcspn(v, "\n\r")] = 0;
                // FIX: Use snprintf for guaranteed null-termination
                snprintf(project_root, MAX_PATH, "%s", v);
                break;
            }
        }
        fclose(kvp);
    }
}

char* build_path_malloc(const char* rel) {
    size_t sz = strlen(project_root) + strlen(rel) + 2;
    char* p = malloc(sz);
    if (p) snprintf(p, sz, "%s/%s", project_root, rel);
    return p;
}

void set_state(const char* piece_id, const char* key, int val) {
    char* pm = build_path_malloc("pieces/master_ledger/plugins/+x/piece_manager.+x");
    char val_str[32];
    snprintf(val_str, sizeof(val_str), "%d", val);
    
    pid_t pid = fork();
    if (pid == 0) {
        freopen("/dev/null", "w", stdout);
        execl(pm, pm, piece_id, "set-state", key, val_str, NULL);
        exit(1);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    }
    free(pm);
}

int get_state(const char* piece_id, const char* key) {
    char* pm = build_path_malloc("pieces/master_ledger/plugins/+x/piece_manager.+x");
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) { free(pm); return -1; }
    
    pid_t pid = fork();
    if (pid == 0) {
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);
        execl(pm, pm, piece_id, "get-state", key, NULL);
        exit(1);
    } else if (pid > 0) {
        close(pipe_fd[1]);
        char val[256];
        ssize_t n = read(pipe_fd[0], val, sizeof(val) - 1);
        if (n >= 0) val[n] = '\0';
        close(pipe_fd[0]);
        waitpid(pid, NULL, 0);
        free(pm);
        if (n > 0 && strstr(val, "STATE_NOT_FOUND") == NULL) return atoi(val);
    }
    free(pm);
    return -1;
}

int main(int argc, char *argv[]) {
    if (argc < 3) return 1;
    resolve_root();
    const char *piece_id = argv[1];
    char dir = argv[2][0];

    int x = get_state(piece_id, "pos_x");
    int y = get_state(piece_id, "pos_y");
    if (x == -1 || y == -1) return 1;

    if (dir == 'w') y--;
    else if (dir == 's') y++;
    else if (dir == 'a') x--;
    else if (dir == 'd') x++;

    if (x < 1) x = 1; if (x > 18) x = 18;
    if (y < 1) y = 1; if (y > 8) y = 8;

    set_state(piece_id, "pos_x", x);
    set_state(piece_id, "pos_y", y);

    // Pulse frame change
    char* pulse = build_path_malloc("pieces/display/frame_changed.txt");
    FILE *f = fopen(pulse, "a");
    if (f) { fputc('M', f); fclose(f); }
    free(pulse);

    // Increment time/turn if not selector
    if (strcmp(piece_id, "selector") != 0) {
        char* inc_t = build_path_malloc("pieces/apps/fuzzpet_app/clock/plugins/+x/increment_time.+x");
        char* inc_turn = build_path_malloc("pieces/apps/fuzzpet_app/clock/plugins/+x/increment_turn.+x");
        
        pid_t p1 = fork();
        if (p1 == 0) { execl(inc_t, inc_t, NULL); exit(1); }
        else waitpid(p1, NULL, 0);
        
        pid_t p2 = fork();
        if (p2 == 0) { execl(inc_turn, inc_turn, NULL); exit(1); }
        else waitpid(p2, NULL, 0);
        
        free(inc_t); free(inc_turn);
    }

    return 0;
}
