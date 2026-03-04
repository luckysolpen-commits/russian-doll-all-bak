#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// TPM Universal Movement Engine (v3.4 - Mirror Aware)
// Usage: move.+x [PIECE_ID] [DIRECTION: w/a/s/d]

void set_state(const char* piece_id, const char* key, int val) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./pieces/master_ledger/plugins/+x/piece_manager.+x %s set-state %s %d", piece_id, key, val);
    system(cmd);
}

int get_state(const char* piece_id, const char* key) {
    char cmd[512], val[64];
    snprintf(cmd, sizeof(cmd), "./pieces/master_ledger/plugins/+x/piece_manager.+x %s get-state %s", piece_id, key);
    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;
    int result = -1;
    if (fgets(val, sizeof(val), fp)) {
        if (strcmp(val, "STATE_NOT_FOUND") != 0) result = atoi(val);
    }
    pclose(fp);
    return result;
}

int main(int argc, char *argv[]) {
    if (argc < 3) return 1;
    const char *piece_id = argv[1];
    char dir = argv[2][0];

    int x = get_state(piece_id, "pos_x");
    int y = get_state(piece_id, "pos_y");
    if (x == -1 || y == -1) return 1;

    if (dir == 'w') y--;
    else if (dir == 's') y++;
    else if (dir == 'a') x--;
    else if (dir == 'd') x++;

    // Boundaries (Standard 20x10 Map)
    if (x < 1) x = 1; if (x > 18) x = 18;
    if (y < 1) y = 1; if (y > 8) y = 8;

    set_state(piece_id, "pos_x", x);
    set_state(piece_id, "pos_y", y);

    // Advance time
    system("./pieces/apps/fuzzpet_app/clock/plugins/+x/increment_time.+x > /dev/null 2>&1");
    system("./pieces/apps/fuzzpet_app/clock/plugins/+x/increment_turn.+x > /dev/null 2>&1");

    return 0;
}
