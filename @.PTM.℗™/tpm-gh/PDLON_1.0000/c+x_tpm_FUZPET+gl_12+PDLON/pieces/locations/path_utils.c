#include <stdio.h>
#include <string.h>

#define MAX_PATH 512

static char project_root[MAX_PATH] = "";
static int initialized = 0;

int init_paths() {
    if (initialized) return 1;
    
    FILE* f = fopen("pieces/locations/location_kvp", "r");
    if (!f) {
        // Try relative to executable
        f = fopen("../pieces/locations/location_kvp", "r");
        if (!f) {
            fprintf(stderr, "Error: Cannot find location_kvp\n");
            return 0;
        }
    }
    
    char line[MAX_PATH];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "project_root=", 13) == 0) {
            strncpy(project_root, line + 13, MAX_PATH - 1);
            // Remove newline
            project_root[strcspn(project_root, "\n")] = 0;
        }
    }
    fclose(f);
    
    initialized = 1;
    return 1;
}

const char* get_project_root() {
    if (!initialized && !init_paths()) {
        return NULL;
    }
    return project_root;
}

int build_path(char* output, const char* relative_path) {
    const char* root = get_project_root();
    if (!root) {
        return 0;
    }
    snprintf(output, MAX_PATH, "%s/%s", root, relative_path);
    return 1;
}
