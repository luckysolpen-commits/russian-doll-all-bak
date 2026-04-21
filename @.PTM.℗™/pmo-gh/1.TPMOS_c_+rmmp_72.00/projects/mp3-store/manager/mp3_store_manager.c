/*
 * mp3_store_manager.c - MP3 Store Player Manager
 *
 * This module handles directory navigation, file listing, and basic player state.
 * It reads base directories from pieces/user-config.txt.
 *
 * COMPILE: gcc -o +x/mp3_store_manager.+x mp3_store_manager.c -pthread
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

#define MODULE_NAME "mp3_store_manager"
#define MAX_PATH 4096
#define MAX_LINE 1024
#define MAX_VISIBLE_ITEMS 15
#define MAX_TREE_ITEMS 500

typedef struct {
    char name[MAX_LINE];
    char full_path[MAX_PATH];
    int depth;
    int is_directory;
    int is_expanded;
} TreeItem;

/* Module-specific state variables */
static char project_root[MAX_PATH] = ".";
static char current_project[MAX_LINE] = "mp3-store";
static char active_target_id[64] = "selector";
static char last_key_str[32] = "None";

/* Player state variables */
static char current_directory[MAX_PATH] = "ROOT";
static char current_file[MAX_LINE] = "None";
static char directory_listing[16384] = "";

/* Config paths */
static char config_paths[20][MAX_PATH];
static int config_path_count = 0;

/* Tree state */
static TreeItem tree_items[MAX_TREE_ITEMS];
static int tree_count = 0;
static char expanded_paths[100][MAX_PATH];
static int expanded_count = 0;
static int selected_index = 0;

/* Forward declarations */
static void update_state(void);
static void list_directory_contents(void);

/* ============================================================================
 * CPU-SAFE: Signal Handling & Process Management
 * ============================================================================ */
static volatile sig_atomic_t g_shutdown = 0;

static void handle_sigint(int sig) {
    (void)sig;
    g_shutdown = 1;
}

static int run_command(const char* cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
        execl("/bin/sh", "/bin/sh", "-c", cmd, (char *)NULL);
        _exit(127);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
    return -1;
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

static void load_config(void) {
    char config_path[MAX_PATH];
    snprintf(config_path, sizeof(config_path), "%s/projects/mp3-store/pieces/user-config.txt", project_root);
    
    FILE *f = fopen(config_path, "r");
    config_path_count = 0;
    if (f) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), f) && config_path_count < 20) {
            char *p = trim_str(line);
            if (strlen(p) == 0 || p[0] == '#') continue;
            strncpy(config_paths[config_path_count++], p, MAX_PATH - 1);
        }
        fclose(f);
    }
}

/* ============================================================================
 * TREE FUNCTIONS
 * ============================================================================ */

static int is_expanded(const char *path) {
    for (int i = 0; i < expanded_count; i++) {
        if (strcmp(expanded_paths[i], path) == 0) return 1;
    }
    return 0;
}

static void toggle_expand(const char *path) {
    for (int i = 0; i < expanded_count; i++) {
        if (strcmp(expanded_paths[i], path) == 0) {
            // Collapse: remove it and all sub-paths
            int len = strlen(path);
            for (int j = 0; j < expanded_count; ) {
                if (strncmp(expanded_paths[j], path, len) == 0 && (expanded_paths[j][len] == '\0' || expanded_paths[j][len] == '/')) {
                    for (int k = j; k < expanded_count - 1; k++) {
                        strcpy(expanded_paths[k], expanded_paths[k+1]);
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
    if (expanded_count < 100) {
        strcpy(expanded_paths[expanded_count++], path);
    }
}

static void add_to_tree(const char *name, const char *full_path, int depth, int is_dir) {
    if (tree_count >= MAX_TREE_ITEMS) return;
    
    strncpy(tree_items[tree_count].name, name, MAX_LINE - 1);
    strncpy(tree_items[tree_count].full_path, full_path, MAX_PATH - 1);
    tree_items[tree_count].depth = depth;
    tree_items[tree_count].is_directory = is_dir;
    tree_items[tree_count].is_expanded = is_expanded(full_path);
    tree_count++;
    
    if (is_dir && tree_items[tree_count-1].is_expanded) {
        DIR *dir = opendir(full_path);
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
                
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
    for (int i = 0; i < config_path_count; i++) {
        add_to_tree(config_paths[i], config_paths[i], 0, 1);
    }
}

/* ============================================================================
 * MODULE-SPECIFIC FUNCTIONS
 * ============================================================================ */

static int is_active_layout(void) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/display/current_layout.txt", project_root) == -1) return 0;
    FILE *f = fopen(path, "r");
    if (!f) { free(path); return 0; }
    char line[MAX_LINE];
    int result = 0;
    if (fgets(line, sizeof(line), f)) {
        result = (strstr(line, "mp3-store.chtpm") != NULL);
    }
    fclose(f);
    free(path);
    return result;
}

static void list_directory_contents(void) {
    directory_listing[0] = '\0';
    build_tree();

    strcat(directory_listing, "║  Music Library:
");
    int start = (selected_index >= MAX_VISIBLE_ITEMS) ? (selected_index - MAX_VISIBLE_ITEMS + 1) : 0;
    
    for (int i = start; i < tree_count && i < start + MAX_VISIBLE_ITEMS; i++) {
        char line[MAX_LINE];
        const char *marker = (i == selected_index) ? "[>]" : "[ ]";
        const char *exp_marker = "";
        
        if (tree_items[i].is_directory) {
            exp_marker = tree_items[i].is_expanded ? "[-] " : "[+] ";
        } else {
            exp_marker = "    ";
        }

        // Indent based on depth
        char indent[128] = "";
        for (int d = 0; d < tree_items[i].depth && d < 20; d++) strcat(indent, "  ");

        // Format: marker depth indicator [name]
        // Following documented pattern: ║ [marker] [index]. [indent] [expansion] [name]
        if (tree_items[i].is_directory) {
            snprintf(line, sizeof(line), "║ %s %d. %s%s[%s]
", marker, i + 1, indent, exp_marker, tree_items[i].name);
        } else {
            snprintf(line, sizeof(line), "║ %s %d. %s%s%s
", marker, i + 1, indent, exp_marker, tree_items[i].name);
        }
        strncat(directory_listing, line, sizeof(directory_listing) - strlen(directory_listing) - 1);
    }
    
    if (tree_count == 0) {
        strcat(directory_listing, "║    (No music found. Check user-config.txt)
");
    }
}

// Helper function to escape newlines for state.txt
static char* escape_newlines(const char* input) {
    if (!input) return NULL;
    
    size_t len = strlen(input);
    size_t escaped_len = len;
    for (size_t i = 0; i < len; ++i) {
        if (input[i] == '
') {
            escaped_len++; // For the backslash
        }
    }
    
    char* escaped_str = (char*)malloc(escaped_len + 1);
    if (!escaped_str) return NULL;
    
    size_t current_pos = 0;
    for (size_t i = 0; i < len; ++i) {
        if (input[i] == '
') {
            escaped_str[current_pos++] = '';
            escaped_str[current_pos++] = 'n';
        } else {
            escaped_str[current_pos++] = input[i];
        }
    }
    escaped_str[current_pos] = '\0';
    
    return escaped_str;
}

static void process_key(int key) {
    // Update last_key_str based on the key pressed
    if (key >= 32 && key <= 126) { // Printable characters
        snprintf(last_key_str, sizeof(last_key_str), "%c", (char)key);
    } else if (key == 10 || key == 13) { // ENTER
        strcpy(last_key_str, "ENTER");
    } else if (key == 27) { // ESC
        strcpy(last_key_str, "ESC");
    } else if (key == 1000 || key == 2100) { // LEFT arrow
        strcpy(last_key_str, "LEFT");
    } else if (key == 1001 || key == 2101) { // RIGHT arrow
        strcpy(last_key_str, "RIGHT");
    } else if (key == 1002 || key == 2102) { // UP arrow
        strcpy(last_key_str, "UP");
        if (selected_index > 0) selected_index--;
    } else if (key == 1003 || key == 2103) { // DOWN arrow
        strcpy(last_key_str, "DOWN");
        if (selected_index < tree_count - 1) selected_index++;
    } else {
        // For any other key, we can set a generic code or leave it as is
        // strcpy(last_key_str, "OTHER"); 
    }

    // Update directory listing and state
    list_directory_contents();
    update_state();
}

static void update_state(void) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/player_app/manager/state.txt", project_root) != -1) {
        FILE *f = fopen(path, "w");
        if (f) {
            fprintf(f, "project_id=%s
", current_project);
            fprintf(f, "active_target_id=%s
", active_target_id);
            fprintf(f, "last_key=%s
", last_key_str);
            fprintf(f, "current_z=0
");
            fprintf(f, "current_directory=%s
", current_directory);
            fprintf(f, "current_file=%s
", current_file);
            
            // Escape newlines in directory_listing for parsability
            char* escaped_listing = escape_newlines(directory_listing);
            if (escaped_listing) {
                fprintf(f, "directory_listing=%s
", escaped_listing);
                free(escaped_listing);
            } else {
                // Fallback if escaping fails
                fprintf(f, "directory_listing=%s
", directory_listing);
            }
            
            fprintf(f, "app_title=MP3 Store Player
");
            fclose(f);
        }
        free(path);
    }
}

static void trigger_render(void) {
    char *cmd = NULL;
    if (asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/render_map.+x > /dev/null 2>&1", project_root) != -1) {
        run_command(cmd);
        free(cmd);
    }
}

int main(void) {
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);
    setpgid(0, 0);
    
    resolve_paths();
    load_config();
    
    list_directory_contents();
    update_state();
    trigger_render();
    
    char *hist_path = NULL;
    asprintf(&hist_path, "%s/pieces/apps/player_app/history.txt", project_root);
    
    long last_pos = 0;
    struct stat st;
    
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
                        trigger_render();
                    }
                    last_pos = ftell(hf);
                    fclose(hf);
                }
            } else if (st.st_size < last_pos) last_pos = 0;
        }
        usleep(16667);
    }
    free(hist_path);
    return 0;
}
