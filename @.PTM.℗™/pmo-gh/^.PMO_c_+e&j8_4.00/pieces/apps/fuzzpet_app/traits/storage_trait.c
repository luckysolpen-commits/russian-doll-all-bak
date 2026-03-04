#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// TPM Storage Trait (v4 - Response Integrated)
// Usage: storage.+x [PIECE_ID] [ACTION]

void set_state_int(const char* piece_id, const char* key, int val) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./pieces/master_ledger/plugins/+x/piece_manager.+x %s set-state %s %d > /dev/null 2>&1", piece_id, key, val);
    system(cmd);
}

void set_state_str(const char* piece_id, const char* key, const char* val) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./pieces/master_ledger/plugins/+x/piece_manager.+x %s set-state %s '%s' > /dev/null 2>&1", piece_id, key, val);
    system(cmd);
}

void get_state_str(const char* piece_id, const char* key, char* out_val) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./pieces/master_ledger/plugins/+x/piece_manager.+x %s get-state %s", piece_id, key);
    FILE *fp = popen(cmd, "r");
    if (fp) {
        if (fgets(out_val, 256, fp)) {
            out_val[strcspn(out_val, "\n\r")] = 0;
            if (strcmp(out_val, "STATE_NOT_FOUND") == 0) strcpy(out_val, "");
        } else strcpy(out_val, "");
        pclose(fp);
    } else strcpy(out_val, "");
}

int get_state_int(const char* piece_id, const char* key) {
    char val[256];
    get_state_str(piece_id, key, val);
    if (val[0] == '\0') return -1;
    return atoi(val);
}

void log_response(const char* msg) {
    FILE *f = fopen("pieces/apps/fuzzpet_app/fuzzpet/last_response.txt", "w");
    if (f) { fprintf(f, "%s", msg); fclose(f); }
}

void add_to_list(char* list, const char* id) {
    if (strlen(list) == 0 || strcmp(list, "[]") == 0) { sprintf(list, "[%s]", id); }
    else { list[strlen(list)-1] = '\0'; strcat(list, ","); strcat(list, id); strcat(list, "]"); }
}

int main(int argc, char *argv[]) {
    if (argc < 3) return 1;
    const char *my_id = argv[1];
    const char *action = argv[2];
    int my_x = get_state_int(my_id, "pos_x");
    int my_y = get_state_int(my_id, "pos_y");

    if (strcmp(action, "collect") == 0) {
        FILE *reg = fopen("pieces/apps/fuzzpet_app/world/registry.txt", "r");
        if (reg) {
            char target_id[64]; int collected = 0;
            while (fgets(target_id, sizeof(target_id), reg)) {
                target_id[strcspn(target_id, "\n\r")] = 0;
                if (strlen(target_id) == 0 || strcmp(target_id, my_id) == 0) continue;
                if (get_state_int(target_id, "on_map") == 1 && get_state_int(target_id, "pos_x") == my_x && get_state_int(target_id, "pos_y") == my_y) {
                    set_state_int(target_id, "on_map", 0); set_state_str(target_id, "owner", my_id);
                    char inv[256]; get_state_str(my_id, "inventory", inv); add_to_list(inv, target_id); set_state_str(my_id, "inventory", inv);
                    char msg[128]; snprintf(msg, sizeof(msg), "Picked up %s", target_id); log_response(msg); collected = 1; break;
                }
            }
            if (!collected) log_response("Nothing here to collect.");
            fclose(reg);
        }
    } else if (strcmp(action, "place") == 0) {
        char inv[256]; get_state_str(my_id, "inventory", inv);
        if (strlen(inv) > 2) {
            char *start = inv + 1; char *end = strpbrk(start, ",]");
            if (end) {
                char tid[64]; int len = end - start; strncpy(tid, start, len); tid[len] = '\0';
                set_state_int(tid, "on_map", 1); set_state_str(tid, "owner", "world"); set_state_int(tid, "pos_x", my_x); set_state_int(tid, "pos_y", my_y);
                char new_inv[256]; if (*end == ']') strcpy(new_inv, "[]"); else sprintf(new_inv, "[%s", end + 1);
                set_state_str(my_id, "inventory", new_inv);
                char msg[128]; snprintf(msg, sizeof(msg), "Placed %s", tid); log_response(msg);
            }
        } else log_response("Inventory empty.");
    } else if (strcmp(action, "inventory") == 0) {
        char inv[256]; get_state_str(my_id, "inventory", inv);
        char msg[256]; snprintf(msg, sizeof(msg), "Inventory: %s", inv); log_response(msg);
    } else if (strcmp(action, "scan") == 0) {
        FILE *reg = fopen("pieces/apps/fuzzpet_app/world/registry.txt", "r");
        if (reg) {
            char tid[64], res[512] = "Found: ";
            while (fgets(tid, sizeof(tid), reg)) {
                tid[strcspn(tid, "\n\r")] = 0; if (strlen(tid) == 0) continue;
                if (get_state_int(tid, "on_map") == 1) { strcat(res, tid); strcat(res, " "); }
            }
            log_response(res); fclose(reg);
        }
    }
    return 0;
}
