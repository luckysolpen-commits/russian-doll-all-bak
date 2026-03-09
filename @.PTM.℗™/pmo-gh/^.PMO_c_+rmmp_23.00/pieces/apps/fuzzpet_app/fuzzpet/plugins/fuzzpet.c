#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>

// fuzzpet.c - FuzzPet Plugin (v1.1 - ASPRINTF REFACTOR)
// Responsibility: Warning-free pet interactions and state updates.

#define MAX_PATH 16384

char project_root[MAX_PATH] = ".";

char* trim_str(char *str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void resolve_root() {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[4096];
        while (fgets(line, sizeof(line), kvp)) {
            if (strncmp(line, "project_root=", 13) == 0) {
                char *v = line + 13;
                strncpy(project_root, trim_str(v), MAX_PATH - 1);
                break;
            }
        }
        fclose(kvp);
    }
}

int get_piece_state_int(const char* piece, const char* key) {
    char *cmd = NULL;
    asprintf(&cmd, "'./pieces/master_ledger/plugins/+x/piece_manager.+x' %s get-state %s", piece, key);
    FILE *fp = popen(cmd, "r");
    free(cmd);
    if (!fp) return -1;
    char val[128];
    int res = -1;
    if (fgets(val, sizeof(val), fp)) {
        if (strstr(val, "STATE_NOT_FOUND") == NULL) res = atoi(val);
    }
    pclose(fp);
    return res;
}

void set_piece_state_int(const char* piece, const char* key, int val) {
    char *cmd = NULL;
    asprintf(&cmd, "'./pieces/master_ledger/plugins/+x/piece_manager.+x' %s set-state %s %d > /dev/null 2>&1", piece, key, val);
    if (cmd) { system(cmd); free(cmd); }
}

int main(int argc, char* argv[]) {
    if (argc < 3) return 1;
    resolve_root();
    const char* piece_id = argv[1];
    const char* action = argv[2];

    if (strcmp(action, "feed") == 0) {
        int hunger = get_piece_state_int(piece_id, "hunger");
        set_piece_state_int(piece_id, "hunger", (hunger > 10) ? hunger - 10 : 0);
        printf("Yum yum! Hunger decreased.\n");
    } else if (strcmp(action, "play") == 0) {
        int happiness = get_piece_state_int(piece_id, "happiness");
        set_piece_state_int(piece_id, "happiness", (happiness < 90) ? happiness + 10 : 100);
        printf("Wheee! Happiness increased.\n");
    }

    // Pulse frame marker
    char *pulse = NULL;
    asprintf(&pulse, "echo 'P' >> ./pieces/display/frame_changed.txt");
    if (pulse) { system(pulse); free(pulse); }

    return 0;
}
