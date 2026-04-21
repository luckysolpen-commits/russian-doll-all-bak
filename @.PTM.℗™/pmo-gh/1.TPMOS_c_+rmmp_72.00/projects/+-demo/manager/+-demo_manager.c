/*
 * +-demo_manager.c - Demo Project Manager for Expand/Collapse testing
 *
 * Responsibility: Implement basic tree view logic with expand/collapse
 * for demonstration purposes.
 *
 * COMPILE: gcc -o +x/+-demo_manager.+x +-demo_manager.c -pthread
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

/* ============================================================================
 * CONFIGURATION
 * ============================================================================ */
#define MODULE_NAME "+-demo_manager"
#define MAX_PATH 4096
#define MAX_LINE 1024

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */
static char project_root[MAX_PATH] = ".";
static char current_project[MAX_LINE] = "+-demo";
static char active_target_id[64] = "selector";

static char current_directory[MAX_LINE] = "ROOT";
static char current_file[MAX_LINE] = "None";

static char directory_listing[16384] = "";
static char last_response[MAX_LINE] = "";

/* Tree state for expand/collapse */
typedef struct {
    char name[MAX_LINE];
    char full_path[MAX_PATH];
    int depth;
    int is_directory;
    int is_expanded;
} TreeItem;

#define MAX_TREE_ITEMS 500
#define MAX_EXPANDED_PATHS 100
TreeItem tree_items[MAX_TREE_ITEMS];
int tree_count = 0;
char expanded_paths[MAX_EXPANDED_PATHS][MAX_PATH];
int expanded_count = 0;
int selected_index = 0;

/* ============================================================================
 * SIGNAL HANDLING
 * ============================================================================ */
static volatile sig_atomic_t g_shutdown = 0;

static void handle_sigint(int sig) {
    (void)sig;
    g_shutdown = 1;
}

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */
static void list_directory_contents(void);
static void update_state(void);
static void trigger_render(void);
static int is_active_layout(void);
static void process_key(int key);
static int is_expanded(const char *path);
static void toggle_expand(const char *path);
static void add_to_tree(const char *name, const char *full_path, int depth, int is_dir);
static void build_tree(void);

/* ============================================================================
 * STUB FUNCTIONS
 * ============================================================================ */

static void list_directory_contents(void) {
    snprintf(directory_listing, sizeof(directory_listing),
             "ROOT\n  +-demo\n    src\n    data\n    docs\n");
}

static void update_state(void) {
    FILE *f = fopen("pieces/state/+-demo_manager.state", "w");
    if (f) {
        fprintf(f, "selected_index=%d\n", selected_index);
        fprintf(f, "tree_count=%d\n", tree_count);
        fclose(f);
    }
}

static void trigger_render(void) {
    printf("[RENDER] Tree has %d items | Selected: %d\n", tree_count, selected_index);
    fflush(stdout);
}

static int is_active_layout(void) {
    return 1;  // Always active for demo
}

static void process_key(int key) {
    printf("[KEY] Received: %d\n", key);

    if (key == ' ' || key == '\n' || key == 13) {           // Space or Enter
        if (selected_index < tree_count && tree_items[selected_index].is_directory) {
            toggle_expand(tree_items[selected_index].full_path);
            build_tree();        // Rebuild with new expansion state
            update_state();
            trigger_render();
        }
    }
    else if (key == 'j' || key == 106) {                    // j / Down
        if (selected_index < tree_count - 1) selected_index++;
    }
    else if (key == 'k' || key == 107) {                    // k / Up
        if (selected_index > 0) selected_index--;
    }
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

static char* trim_str(char *str) {
    char *end;
    if (!str) return str;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

static void resolve_paths(void) {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), kvp)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(line);
                char *v = trim_str(eq + 1);
                if (strcmp(k, "project_root") == 0) {
                    snprintf(project_root, sizeof(project_root), "%s", v);
                }
            }
        }
        fclose(kvp);
    }
}

/* ============================================================================
 * TREE VIEW FUNCTIONS
 * ============================================================================ */

static int is_expanded(const char *path) {
    for (int i = 0; i < expanded_count; i++) {
        if (strcmp(expanded_paths[i], path) == 0) return 1;
    }
    return 0;
}

static void toggle_expand(const char *path) {
    // Collapse if already expanded
    for (int i = 0; i < expanded_count; i++) {
        if (strcmp(expanded_paths[i], path) == 0) {
            int len = strlen(path);
            for (int j = 0; j < expanded_count; ) {
                if (strncmp(expanded_paths[j], path, len) == 0 &&
                    (expanded_paths[j][len] == '\0' || expanded_paths[j][len] == '/')) {
                    // Remove this entry
                    for (int k = j; k < expanded_count - 1; k++) {
                        strcpy(expanded_paths[k], expanded_paths[k + 1]);
                    }
                    expanded_count--;
                } else {
                    j++;
                }
            }
            return;
        }
    }

    // Expand
    if (expanded_count < MAX_EXPANDED_PATHS) {
        strcpy(expanded_paths[expanded_count++], path);
    }
}

static void add_to_tree(const char *name, const char *full_path, int depth, int is_dir) {
    if (tree_count >= MAX_TREE_ITEMS) return;

    strncpy(tree_items[tree_count].name, name, MAX_LINE - 1);
    tree_items[tree_count].name[MAX_LINE - 1] = '\0';
    strncpy(tree_items[tree_count].full_path, full_path, MAX_PATH - 1);
    tree_items[tree_count].full_path[MAX_PATH - 1] = '\0';

    tree_items[tree_count].depth = depth;
    tree_items[tree_count].is_directory = is_dir;
    tree_items[tree_count].is_expanded = (is_dir && is_expanded(full_path));

    tree_count++;

    // Recurse into expanded directories
    if (is_dir && tree_items[tree_count - 1].is_expanded) {
        DIR *dir = opendir(full_path);
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                    continue;

                char sub_path[MAX_PATH];
                snprintf(sub_path, sizeof(sub_path), "%s/%s", full_path, entry->d_name);

                struct stat st;
                if (stat(sub_path, &st) == 0) {
                    add_to_tree(entry->d_name, sub_path, depth + 1, S_ISDIR(st.st_mode));
                }
            }
            closedir(dir);
        }
    }
}

static void build_tree(void) {
    tree_count = 0;

    // Sample data (you can improve this later to scan real filesystem)
    const char* sample_paths[] = {
        "projects/+-demo/src",
        "projects/+-demo/src/components",
        "projects/+-demo/src/components/ui",
        "projects/+-demo/src/components/ui/buttons",
        "projects/+-demo/src/components/ui/buttons/icons",
        "projects/+-demo/src/utils",
        "projects/+-demo/data",
        "projects/+-demo/data/assets",
        "projects/+-demo/data/assets/images",
        "projects/+-demo/data/assets/sounds",
        "projects/+-demo/README.md",
        "projects/+-demo/docs",
        "projects/+-demo/docs/01-ONBOARDING",
        "projects/+-demo/docs/02-REFERENCE",
        "projects/+-demo/docs/03-HOW-TO",
        "projects/+-demo/docs/03-HOW-TO/create-project.md",
        "projects/+-demo/docs/04-INVESTIGATIONS",
        "projects/+-demo/docs/05-PITFALLS",
        "projects/+-demo/docs/05-PITFALLS/!.cpu_health+1.txt",
        "projects/+-demo/docs/06-ARCHIVED",
    };
    int num_sample_paths = sizeof(sample_paths) / sizeof(sample_paths[0]);

    // Initial expansions (only on first build)
    static int first_run = 1;
    if (first_run) {
        toggle_expand("projects/+-demo/src");
        toggle_expand("projects/+-demo/src/components");
        toggle_expand("projects/+-demo/src/components/ui");
        toggle_expand("projects/+-demo/src/components/ui/buttons");
        toggle_expand("projects/+-demo/data");
        toggle_expand("projects/+-demo/data/assets");
        toggle_expand("projects/+-demo/docs");
        toggle_expand("projects/+-demo/docs/03-HOW-TO");
        toggle_expand("projects/+-demo/docs/05-PITFALLS");
        first_run = 0;
    }

    for (int i = 0; i < num_sample_paths; ++i) {
        char name_buffer[MAX_LINE];
        const char *name = strrchr(sample_paths[i], '/');
        name = name ? name + 1 : sample_paths[i];
        strncpy(name_buffer, name, MAX_LINE - 1);
        name_buffer[MAX_LINE - 1] = '\0';

        int is_dir = (strchr(sample_paths[i], '/') != NULL &&
                      !strstr(sample_paths[i], ".md") &&
                      !strstr(sample_paths[i], ".txt"));

        int depth = 0;
        const char *p = sample_paths[i];
        while ((p = strchr(p, '/')) != NULL) {
            depth++;
            p++;
        }

        add_to_tree(name_buffer, sample_paths[i], depth, is_dir);
    }
}

/* ============================================================================
 * MAIN
 * ============================================================================ */
int main(void) {
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);
    setpgid(0, 0);

    resolve_paths();
    list_directory_contents();
    build_tree();
    update_state();
    trigger_render();

    char *hist_path = NULL;
    if (asprintf(&hist_path, "%s/projects/+-demo/history.txt", project_root) == -1) {
        perror("asprintf failed");
        return 1;
    }

    long last_pos = 0;
    struct stat st;

    printf("+-demo_manager started successfully!\n");
    printf("Controls:\n");
    printf("  j / k     - move up/down\n");
    printf("  Space / Enter - expand/collapse directory\n");
    printf("  Ctrl+C    - quit\n\n");

    while (!g_shutdown) {
        if (!is_active_layout()) {
            usleep(100000);
            continue;
        }

        if (stat(hist_path, &st) == 0) {
            if (st.st_size > last_pos) {
                FILE *hf = fopen(hist_path, "r");
                if (hf) {
                    fseek(hf, last_pos, SEEK_SET);
                    int key;
                    while (fscanf(hf, "%d", &key) == 1) {
                        process_key(key);
                        update_state();
                        trigger_render();
                    }
                    last_pos = ftell(hf);
                    fclose(hf);
                }
            } else if (st.st_size < last_pos) {
                last_pos = 0;
            }
        }

        usleep(16667);  // ~60 FPS
    }

    free(hist_path);
    printf("\n+-demo_manager shutting down.\n");
    return 0;
}
