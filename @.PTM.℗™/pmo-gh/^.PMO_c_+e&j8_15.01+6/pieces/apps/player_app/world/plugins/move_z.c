#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <ctype.h>

#define MAX_PATH 16384

char* trim_str(char *str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

int get_state(const char* piece_id, const char* key) {
    char *cmd = malloc(MAX_PATH * 2);
    if (!cmd) return -1;
    snprintf(cmd, MAX_PATH * 2, "'./pieces/master_ledger/plugins/+x/piece_manager.+x' %s get-state %s", piece_id, key);
    FILE *fp = popen(cmd, "r");
    free(cmd);
    if (!fp) return -1;
    char val[128];
    int res = -1;
    if (fgets(val, sizeof(val), fp)) {
        char *t = trim_str(val);
        if (strstr(t, "STATE_NOT_FOUND") == NULL) res = atoi(t);
    }
    pclose(fp);
    return res;
}

void set_state(const char* piece_id, const char* key, int val) {
    char *cmd = malloc(MAX_PATH * 2);
    if (!cmd) return;
    snprintf(cmd, MAX_PATH * 2, "'./pieces/master_ledger/plugins/+x/piece_manager.+x' %s set-state %s %d > /dev/null 2>&1", piece_id, key, val);
    system(cmd);
    free(cmd);
}

int main(int argc, char *argv[]) {
    if (argc < 3) return 1;
    
    const char *piece_id = argv[1];
    int dz = atoi(argv[2]);

    int z = get_state(piece_id, "pos_z");
    if (z == -1) z = 0;

    int new_z = z + dz;
    
    // Pulse frame change
    system("echo 'Z' >> ./pieces/display/frame_changed.txt");

    // Update state
    set_state(piece_id, "pos_z", new_z);

    return 0;
}
