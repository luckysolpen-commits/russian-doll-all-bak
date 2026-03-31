#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../../../../system/win_compat.h"

// TPM Movement Trait
// Usage: move.+x [PIECE_ID] [DIRECTION: w/a/s/d]

void set_state(const char* piece_id, const char* key, int val) {
    char cmd[512];
#ifndef _WIN32
    snprintf(cmd, sizeof(cmd), "./pieces/master_ledger/plugins/+x/piece_manager.+x %s set-state %s %d > /dev/null 2>&1", piece_id, key, val);
#else
    snprintf(cmd, sizeof(cmd), "powershell -NoProfile -ExecutionPolicy Bypass -Command \"& { .\\pieces\\master_ledger\\plugins\\+x\\piece_manager.+x %s set-state %s %d } > $null 2>&1\"", piece_id, key, val);
#endif
    system(cmd);
}

int get_state(const char* piece_id, const char* key) {
    char cmd[512], val[64];
#ifndef _WIN32
    snprintf(cmd, sizeof(cmd), "./pieces/master_ledger/plugins/+x/piece_manager.+x %s get-state %s", piece_id, key);
#else
    snprintf(cmd, sizeof(cmd), "powershell -NoProfile -ExecutionPolicy Bypass -Command \"& { .\\pieces\\master_ledger\\plugins\\+x\\piece_manager.+x %s get-state %s }\"", piece_id, key);
#endif
    FILE *fp = popen(cmd, "r");
    if (!fp) return 0;
    int result = 0;
    if (fgets(val, sizeof(val), fp)) {
        if (strcmp(val, "STATE_NOT_FOUND") != 0) result = atoi(val);
    }
    pclose(fp);
    return result;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <piece_id> <direction>\n", argv[0]);
        return 1;
    }

    const char *piece_id = argv[1];
    char direction = argv[2][0];

    int pos_x = get_state(piece_id, "pos_x");
    int pos_y = get_state(piece_id, "pos_y");
    int nx = pos_x, ny = pos_y;

    if (direction == 'w') ny--;
    else if (direction == 's') ny++;
    else if (direction == 'a') nx--;
    else if (direction == 'd') nx++;

    // Basic map bounds (could be fetched from world piece in future)
    if (nx < 0 || nx >= 20 || ny < 0 || ny >= 10) return 0;

    // Update position
    set_state(piece_id, "pos_x", nx);
    set_state(piece_id, "pos_y", ny);

    // Costs of movement
    int hunger = get_state(piece_id, "hunger");
    int energy = get_state(piece_id, "energy");
    if (hunger != -1) set_state(piece_id, "hunger", (hunger < 100) ? hunger + 1 : 100);
    if (energy != -1) set_state(piece_id, "energy", (energy > 0) ? energy - 1 : 0);

    // Advance world time (system pieces)
#ifndef _WIN32
    system("./pieces/apps/fuzzpet_app/clock/plugins/+x/increment_time.+x > /dev/null 2>&1");
    system("./pieces/apps/fuzzpet_app/clock/plugins/+x/increment_turn.+x > /dev/null 2>&1");
#else
    system("powershell -NoProfile -ExecutionPolicy Bypass -Command \"& { .\\pieces\\apps\\fuzzpet_app\\clock\\plugins\\+x\\increment_time.+x } > $null 2>&1\"");
    system("powershell -NoProfile -ExecutionPolicy Bypass -Command \"& { .\\pieces\\apps\\fuzzpet_app\\clock\\plugins\\+x\\increment_turn.+x } > $null 2>&1\"");
#endif

    return 0;
}
