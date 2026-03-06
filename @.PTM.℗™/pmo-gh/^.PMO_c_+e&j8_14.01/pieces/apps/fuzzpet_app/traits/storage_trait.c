#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdarg.h>

// TPM Storage Trait (v4.2 - SILENT BUILD)
// Usage: storage.+x [PIECE_ID] [ACTION]

char project_root[1024] = ".";

char* trim_str(char *str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void build_path(char* dst, size_t sz, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(dst, sz, fmt, args);
    va_end(args);
}

void resolve_root() {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[2048];
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

void set_state_int(const char* piece_id, const char* key, int val) {
    char cmd[2048];
    build_path(cmd, sizeof(cmd), "'%s/pieces/master_ledger/plugins/+x/piece_manager.+x' %s set-state %s %d > /dev/null 2>&1", project_root, piece_id, key, val);
    system(cmd);
}

void set_state_str(const char* piece_id, const char* key, const char* val) {
    char cmd[2048];
    build_path(cmd, sizeof(cmd), "'%s/pieces/master_ledger/plugins/+x/piece_manager.+x' %s set-state %s '%s' > /dev/null 2>&1", project_root, piece_id, key, val);
    system(cmd);
}

void get_state_str(const char* piece_id, const char* key, char* out_val) {
    char cmd[2048];
    build_path(cmd, sizeof(cmd), "'%s/pieces/master_ledger/plugins/+x/piece_manager.+x' %s get-state %s", project_root, piece_id, key);
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
    char path[1024];
    build_path(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/fuzzpet/last_response.txt", project_root);
    FILE *f = fopen(path, "w");
    if (f) { fprintf(f, "%-57s", msg); fclose(f); }
}

void add_to_list(char* list, const char* id) {
    if (strlen(list) == 0 || strcmp(list, "[]") == 0) { sprintf(list, "[%s]", id); }
    else { list[strlen(list)-1] = '\0'; strcat(list, ","); strcat(list, id); strcat(list, "]"); }
}

int main(int argc, char *argv[]) {
    resolve_root();
    if (argc < 3) return 1;
    const char *my_id = argv[1];
    const char *action = argv[2];
    int my_x = get_state_int(my_id, "pos_x");
    int my_y = get_state_int(my_id, "pos_y");

    char world_path[1024];
    build_path(world_path, sizeof(world_path), "%s/pieces/world/map_01", project_root);

    if (strcmp(action, "collect") == 0) {
        DIR *dir = opendir(world_path);
        int collected = 0;
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, my_id) == 0) continue;
                struct stat st; char entity_path[2048];
                build_path(entity_path, sizeof(entity_path), "%s/%s", world_path, entry->d_name);
                if (stat(entity_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                    if (get_state_int(entry->d_name, "on_map") == 1 && get_state_int(entry->d_name, "pos_x") == my_x && get_state_int(entry->d_name, "pos_y") == my_y) {
                        set_state_int(entry->d_name, "on_map", 0); set_state_str(entry->d_name, "owner", my_id);
                        char inv[256]; get_state_str(my_id, "inventory", inv); add_to_list(inv, entry->d_name); set_state_str(my_id, "inventory", inv);
                        char msg[128]; snprintf(msg, sizeof(msg), "Picked up %s", entry->d_name); log_response(msg); collected = 1; break;
                    }
                }
            }
            closedir(dir);
        }
        if (!collected) log_response("Nothing here to collect.");
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
        char msg[512]; snprintf(msg, sizeof(msg), "Inventory: %s", inv); log_response(msg);
    } else if (strcmp(action, "scan") == 0) {
        DIR *dir = opendir(world_path);
        char res[512] = "Found: ";
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
                struct stat st; char entity_path[2048];
                build_path(entity_path, sizeof(entity_path), "%s/%s", world_path, entry->d_name);
                if (stat(entity_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                    if (get_state_int(entry->d_name, "on_map") == 1) { strcat(res, entry->d_name); strcat(res, " "); }
                }
            }
            closedir(dir);
        }
        log_response(res);
    }
    return 0;
}
