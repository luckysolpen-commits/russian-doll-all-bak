#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h> // For directory scanning
#include <sys/stat.h> // For stat
#include <pthread.h> // For input thread
#include <ctype.h>   // For isspace

// TPM MP3 Store Manager
// Responsibility: Scan MP3s, display list, handle playback requests.

char current_project[64] = "mp3-store";
char mp3_list_str[4096] = ""; // Stores the CHTPM-formatted list of MP3 buttons
char current_mp3[256] = "None";
char player_status[64] = "Stopped";
char last_key_str[64] = "None";

char project_root[2048] = ".";
char history_path[4096] = "pieces/apps/player_app/history.txt"; // Assuming shared history
char debug_log_path[4096];

char* trim_str(char *str) {
    char *end;
    if (!str) return str;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

char* build_path_malloc(const char* rel) {
    size_t sz = strlen(project_root) + strlen(rel) + 2;
    char* p = (char*)malloc(sz);
    if (p) snprintf(p, sz, "%s/%s", project_root, rel);
    return p;
}

void resolve_paths() {
    if (!getcwd(project_root, sizeof(project_root))) strncpy(project_root, ".", sizeof(project_root) - 1);
    project_root[sizeof(project_root) - 1] = '\0';
    snprintf(debug_log_path, sizeof(debug_log_path), "%s/projects/%s/manager/debug_log.txt", project_root, current_project);
    snprintf(history_path, sizeof(history_path), "%s/pieces/apps/player_app/history.txt", project_root);
}

void scan_mp3s() {
    char mp3_dir_path[4096];
    snprintf(mp3_dir_path, sizeof(mp3_dir_path), "%s/projects/%s/mp3s", project_root, current_project);

    DIR *dir;
    struct dirent *entry;
    
    if (!(dir = opendir(mp3_dir_path))) {
        fprintf(stderr, "Could not open MP3 directory: %s\n", mp3_dir_path);
        strcpy(mp3_list_str, "<text label=\"Error loading MP3s.\" />");
        return;
    }

    mp3_list_str[0] = '\0'; // Clear previous list
    int index = 1;
    char mp3_title[256];

    while ((entry = readdir(dir)) != NULL) {
        const char *filename = entry->d_name;
        if (strstr(filename, ".mp3") != NULL) {
            // Extract title, removing extension
            strncpy(mp3_title, filename, sizeof(mp3_title) - 1);
            mp3_title[sizeof(mp3_title) - 1] = '\0';
            char *ext = strrchr(mp3_title, '.');
            if (ext) *ext = '\0';
            
            // Use asprintf → no more truncation warnings
            char *mp3_path_full = NULL;
            if (asprintf(&mp3_path_full, "%s/%s", mp3_dir_path, filename) == -1) {
                continue;
            }

            char *btn_entry = NULL;
            if (asprintf(&btn_entry, "<button label=\"%s (%d)\" onClick=\"run_command(projects/%s/ops/+x/play_mp3.+x, '%s')\" />\n",
                         mp3_title, index, current_project, mp3_path_full) == -1) {
                free(mp3_path_full);
                continue;
            }

            strncat(mp3_list_str, btn_entry, sizeof(mp3_list_str) - strlen(mp3_list_str) - 1);
            free(btn_entry);
            free(mp3_path_full);
            index++;
        }
    }
    closedir(dir);

    if (strlen(mp3_list_str) == 0) {
        strcpy(mp3_list_str, "<text label=\"No MP3s found.\" />");
    }
}

void trigger_render() {
    char* render_signal_path = build_path_malloc("pieces/display/frame_changed.txt");
    FILE *pf = fopen(render_signal_path, "a");
    if (pf) {
        fprintf(pf, "M\n"); // Generic signal
        fclose(pf);
    }
    free(render_signal_path);
}

void log_response(const char* msg) {
    char path[4096];
    snprintf(path, sizeof(path), "%s/projects/%s/pieces/mp3_player/last_response.txt", project_root, current_project);
    char dir_path[4096];
    strncpy(dir_path, path, strrchr(path, '/') - path);
    dir_path[strrchr(path, '/') - path] = '\0';
    mkdir(dir_path, 0755);

    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "%-57s", msg);
        fclose(f);
    }
    strncpy(player_status, msg, sizeof(player_status) - 1);
    player_status[sizeof(player_status) - 1] = '\0';
}

void route_input(int key) {
    if (key >= 32 && key <= 126) snprintf(last_key_str, sizeof(last_key_str), "%c", (char)key);
    else if (key == 10 || key == 13) strcpy(last_key_str, "ENTER");
    else if (key == 27) strcpy(last_key_str, "ESC");
    else { strcpy(last_key_str, "Other"); }

    if (key == '9' || key == 27) { // Exit to Menu
        log_response("Exiting MP3 Store.");
        strcpy(current_mp3, "None");
        strcpy(player_status, "Stopped");
        scan_mp3s();
        trigger_render();
        return;
    }

    char mp3_play_op_path[2048];
    snprintf(mp3_play_op_path, sizeof(mp3_play_op_path), "projects/%s/ops/+x/play_mp3.+x", current_project);
    
    char mp3_dir_path[4096];
    snprintf(mp3_dir_path, sizeof(mp3_dir_path), "%s/projects/%s/mp3s", project_root, current_project);
    
    DIR *dir;
    struct dirent *entry;
    char *mp3_path_for_play = NULL;   // dynamic allocation with asprintf
    int current_index = 1;
    int found_mp3_to_play = 0;

    if ((dir = opendir(mp3_dir_path))) {
        while ((entry = readdir(dir)) != NULL) {
            const char *filename = entry->d_name;
            if (strstr(filename, ".mp3") != NULL) {
                int selected_index = -1;
                if (key >= '1' && key <= '9') {
                    selected_index = key - '0';
                } else if (key >= 2001 && key <= 2009) {
                    selected_index = key - 2000;
                }

                if (selected_index == current_index) {
                    if (asprintf(&mp3_path_for_play, "%s/%s", mp3_dir_path, filename) == -1) {
                        found_mp3_to_play = 0;
                        break;
                    }
                    found_mp3_to_play = 1;
                    break;
                }
                current_index++;
            }
        }
        closedir(dir);
    }

    if (found_mp3_to_play) {
        char command[8192];
        snprintf(command, sizeof(command), "\"%s\" \"%s\"", mp3_play_op_path, mp3_path_for_play);
        
        int sys_ret = system(command);
        if (sys_ret == 0) {
            char mp3_filename_only[256];
            strncpy(mp3_filename_only, strrchr(mp3_path_for_play, '/') + 1, sizeof(mp3_filename_only) - 1);
            mp3_filename_only[sizeof(mp3_filename_only) - 1] = '\0';
            char *ext = strrchr(mp3_filename_only, '.');
            if (ext) *ext = '\0';

            strncpy(current_mp3, mp3_filename_only, sizeof(current_mp3) - 1);
            current_mp3[sizeof(current_mp3) - 1] = '\0';
            log_response("Playing...");
        } else {
            log_response("Error playing MP3.");
        }
    } else if ((key >= '1' && key <= '9') || (key >= 2001 && key <= 2009)) {
        log_response("Invalid MP3 selection.");
    }

    if (mp3_path_for_play) {
        free(mp3_path_for_play);
    }
    trigger_render();
}

int is_active_layout() {
    char *path = build_path_malloc("pieces/display/current_layout.txt");
    FILE *f = fopen(path, "r");
    int is_active = 0;
    if (f) {
        char line[1024];
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, "mp3-store.chtpm") != NULL) {
                is_active = 1;
                break;
            }
        }
        fclose(f);
    } else {
        is_active = 1; 
    }
    free(path);
    return is_active;
}

void* input_thread(void* arg __attribute__((unused))) {
    long last_pos = 0; struct stat st;
    if (stat(history_path, &st) == 0) last_pos = st.st_size;

    while (1) {
        if (!is_active_layout()) { usleep(100000); continue; }

        if (stat(history_path, &st) == 0) {
            if (st.st_size > last_pos) {
                FILE *hf = fopen(history_path, "r");
                if (hf) {
                    fseek(hf, last_pos, SEEK_SET);
                    int key;
                    while (fscanf(hf, "%d", &key) == 1) {
                        route_input(key);
                    }
                    last_pos = ftell(hf);
                    fclose(hf);
                }
            } else if (st.st_size < last_pos) {
                last_pos = 0;
            }
        }
        usleep(16667);
    }
    return NULL;
}

int main() {
    resolve_paths();
    scan_mp3s();
    log_response("MP3 Store Ready.");
    trigger_render();

    pthread_t t;
    pthread_create(&t, NULL, input_thread, NULL);

    while (1) {
        usleep(1000000); 
    }
    return 0;
}
