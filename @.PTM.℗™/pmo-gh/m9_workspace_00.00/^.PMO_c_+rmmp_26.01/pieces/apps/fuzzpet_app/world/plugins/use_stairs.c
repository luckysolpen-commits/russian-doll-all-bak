#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>

// use_stairs.c - World Plugin (v1.1 - ASPRINTF REFACTOR)
// Responsibility: Warning-free vertical traversal logic.

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

int main(int argc, char* argv[]) {
    if (argc < 3) return 1;
    resolve_root();
    const char* piece_id = argv[1];
    int dz = atoi(argv[2]);

    // Simple stair logic
    char *cmd = NULL;
    asprintf(&cmd, "'./pieces/master_ledger/plugins/+x/piece_manager.+x' %s get-state pos_z", piece_id);
    FILE *fp = popen(cmd, "r");
    free(cmd);
    
    int z = 0;
    if (fp) {
        char val[128];
        if (fgets(val, sizeof(val), fp)) {
            if (strstr(val, "STATE_NOT_FOUND") == NULL) z = atoi(val);
        }
        pclose(fp);
    }

    int new_z = z + dz;
    asprintf(&cmd, "'./pieces/master_ledger/plugins/+x/piece_manager.+x' %s set-state pos_z %d > /dev/null 2>&1", piece_id, new_z);
    if (cmd) { system(cmd); free(cmd); }

    return 0;
}
