/*
 * user_module.c - User Authentication & Profile Management Module
 * App: pieces/apps/user/
 * 
 * Implements:
 * - SHA256 password hashing via OpenSSL
 * - User signup with profile creation
 * - Login authentication with session management
 * - DYNAMIC METHODS via parser's piece_methods (from user.pdl)
 * - CLI input buffer integration (<cli_io> pattern)
 * - CPU-safe patterns (signal handling, fork/exec/waitpid)
 * - Focus-aware throttling
 * - stat()-first history polling
 *
 * COMPILE: gcc -o pieces/apps/user/plugins/+x/user_module.+x pieces/apps/user/plugins/user_module.c -pthread -lssl -lcrypto
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

#define MAX_LINE 1024
#define MAX_PATH 4096
#define MAX_USERNAME 64
#define MAX_PASSWORD 128

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */
static volatile sig_atomic_t g_shutdown = 0;
static char project_root[MAX_PATH] = ".";
static char current_user[MAX_USERNAME] = "guest";
static char session_status[64] = "auth_required";
static char last_key_str[32] = "None";
static long last_history_pos = 0;

/* Screen state: which methods are shown (controls which PDL methods are active) */
static int auth_screen = 0;  /* 0=main (login/signup/exit), 1=login (username/password/confirm/cancel), 2=signup, 3=profile */
static int input_mode = 0;   /* 0=none, 1=username, 2=password */
static char input_username[MAX_USERNAME] = "";
static char input_password[MAX_PASSWORD] = "";
static char auth_response[256] = "";

/* CLI input buffer (for parser's <cli_io>) */
static char cli_buffer[MAX_PASSWORD] = "";

/* ============================================================================
 * CPU-SAFE: Signal Handling & Process Management
 * ============================================================================ */

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

static int hash_password(const char *password, char *output) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) return -1;
    
    const EVP_MD *md = EVP_sha256();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    unsigned int hash_len = 0;
    
    if (EVP_DigestInit_ex(ctx, md, NULL) != 1 ||
        EVP_DigestUpdate(ctx, password, strlen(password)) != 1 ||
        EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }
    
    EVP_MD_CTX_free(ctx);
    
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[SHA256_DIGEST_LENGTH * 2] = '\0';
    return 0;
}

static void get_timestamp(char *buffer, size_t size) {
    snprintf(buffer, size, "%ld", (long)time(NULL));
}

/* ============================================================================
 * USER PROFILE OPERATIONS
 * ============================================================================ */

static int user_exists(const char *username) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/user/pieces/profiles/%s/state.txt", 
                 project_root, username) == -1) return 0;
    struct stat st;
    int exists = (stat(path, &st) == 0);
    free(path);
    return exists;
}

static int create_user_profile(const char *username, const char *password) {
    char *path = NULL;
    char hash[65];
    char timestamp[32];
    
    if (hash_password(password, hash) != 0) return -1;
    get_timestamp(timestamp, sizeof(timestamp));
    
    if (asprintf(&path, "%s/pieces/apps/user/pieces/profiles/%s", 
                 project_root, username) == -1) return -1;
    char *mkdir_cmd = NULL;
    if (asprintf(&mkdir_cmd, "mkdir -p '%s'/projects '%s'/pieces", path, path) == -1) {
        free(path);
        return -1;
    }
    run_command(mkdir_cmd);
    free(mkdir_cmd);
    
    char *state_path = NULL;
    if (asprintf(&state_path, "%s/state.txt", path) == -1) {
        free(path);
        return -1;
    }
    FILE *f = fopen(state_path, "w");
    if (!f) {
        free(state_path);
        free(path);
        return -1;
    }
    
    fprintf(f, "name=%s\n", username);
    fprintf(f, "password_hash=%s\n", hash);
    fprintf(f, "balance=100\n");
    fprintf(f, "rank=newcomer\n");
    fprintf(f, "created_at=%s\n", timestamp);
    fprintf(f, "last_login=%s\n", timestamp);
    fprintf(f, "projects_owned=0\n");
    fprintf(f, "pieces_owned=0\n");
    fclose(f);
    
    free(state_path);
    free(path);
    return 0;
}

static int verify_password(const char *username, const char *password) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/user/pieces/profiles/%s/state.txt", 
                 project_root, username) == -1) return -1;
    
    FILE *f = fopen(path, "r");
    if (!f) {
        free(path);
        return -1;
    }
    
    char stored_hash[65] = "";
    char line[MAX_LINE];
    
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "password_hash=", 14) == 0) {
            char *h = trim_str(line + 14);
            strncpy(stored_hash, h, sizeof(stored_hash) - 1);
            break;
        }
    }
    fclose(f);
    free(path);
    
    if (strlen(stored_hash) == 0) return -1;
    
    char input_hash[65];
    if (hash_password(password, input_hash) != 0) return -1;
    
    return (strcmp(stored_hash, input_hash) == 0) ? 0 : -1;
}

static int load_user_profile(const char *username, char *balance, char *rank) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/user/pieces/profiles/%s/state.txt", 
                 project_root, username) == -1) return -1;
    
    FILE *f = fopen(path, "r");
    if (!f) {
        free(path);
        return -1;
    }
    
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';
            char *k = trim_str(line);
            char *v = trim_str(eq + 1);
            if (strcmp(k, "balance") == 0) {
                strncpy(balance, v, 31);
                balance[31] = '\0';
            } else if (strcmp(k, "rank") == 0) {
                strncpy(rank, v, 31);
                rank[31] = '\0';
            }
        }
    }
    fclose(f);
    return 0;
}

/* ============================================================================
 * SESSION OPERATIONS
 * ============================================================================ */

static void save_session_state(const char *status, const char *user) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/user/pieces/session/state.txt", 
                 project_root) == -1) return;
    
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "session_status=%s\n", status);
        fprintf(f, "current_user=%s\n", user);
        
        if (strcmp(status, "active") == 0 && strcmp(user, "guest") != 0) {
            char timestamp[32];
            char token[64];
            get_timestamp(timestamp, sizeof(timestamp));
            for (size_t i = 0; i < 32 && i < strlen("abcdef0123456789"); i++) {
                token[i] = "abcdef0123456789"[rand() % 16];
            }
            token[32] = '\0';
            fprintf(f, "login_time=%s\n", timestamp);
            fprintf(f, "session_token=%s\n", token);
        }
        fclose(f);
    }
    free(path);
}

static void start_session(const char *username) {
    strncpy(current_user, username, sizeof(current_user) - 1);
    strcpy(session_status, "active");
    save_session_state("active", username);
}

static void end_session(void) {
    strcpy(current_user, "guest");
    strcpy(session_status, "auth_required");
    auth_screen = 0;
    input_mode = 0;
    input_username[0] = '\0';
    input_password[0] = '\0';
    cli_buffer[0] = '\0';
    auth_response[0] = '\0';
}

/* ============================================================================
 * STATE MANAGEMENT
 * ============================================================================ */

static void clear_view(void) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/player_app/view.txt", project_root) == -1) return;
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "  [User Authentication]\n");
        fclose(f);
    }
    free(path);
}

static void trigger_render(void) {
    char *cmd = NULL;
    if (asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/render_map.+x > /dev/null 2>&1", project_root) == -1) return;
    run_command(cmd);
    free(cmd);
}

static void hit_frame_marker(void) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/display/frame_changed.txt", project_root) == -1) return;
    FILE *f = fopen(path, "a");
    if (f) {
        fprintf(f, "M\n");
        fclose(f);
    }
    free(path);
}

static void trigger_state_change(void) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/player_app/state_changed.txt", project_root) == -1) return;
    FILE *f = fopen(path, "a");
    if (f) {
        fprintf(f, "S\n");
        fclose(f);
    }
    free(path);
}

static void write_parser_state(void) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/player_app/state.txt", project_root) == -1) return;
    
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "project_id=user\n");
        fprintf(f, "active_target_id=user_main\n");
        
        fprintf(f, "app_title=USER AUTHENTICATION\n");
        fprintf(f, "session_status=%s\n", session_status);
        fprintf(f, "current_user=%s\n", current_user);
        fprintf(f, "last_key=%s\n", last_key_str);
        
        /* Always write username and password inputs for form fields */
        fprintf(f, "username_input=%s\n", input_username);
        /* Password masking for state */
        char pass_display[MAX_PASSWORD] = "";
        if (strlen(input_password) > 0) {
            for (size_t i = 0; i < strlen(input_password) && i < 20; i++) {
                strcat(pass_display, "*");
            }
        }
        fprintf(f, "password_input=%s\n", pass_display);
        
        /* CLI input (for backward compat - use active field) */
        if (input_mode == 1) {
            fprintf(f, "cli_input=Username: %s\n", cli_buffer);
        } else if (input_mode == 2) {
            fprintf(f, "cli_input=Password: %s\n", pass_display);
        } else {
            fprintf(f, "cli_input=\n");
        }
        
        /* Response message */
        fprintf(f, "response=%s\n", auth_response);
        
        if (strcmp(session_status, "active") == 0 && strcmp(current_user, "guest") != 0) {
            char balance[32] = "0";
            char rank[32] = "unknown";
            if (load_user_profile(current_user, balance, rank) == 0) {
                fprintf(f, "user_balance=%s\n", balance);
                fprintf(f, "user_rank=%s\n", rank);
                fprintf(f, "user_name=%s\n", current_user);
            }
        }
        
        fclose(f);
        trigger_state_change();
    }
    free(path);
}

static void write_manager_state(void) {
    char *path = NULL;
    
    if (asprintf(&path, "%s/pieces/apps/user/pieces/session/state.txt", project_root) != -1) {
        FILE *f = fopen(path, "w");
        if (f) {
            fprintf(f, "session_status=%s\n", session_status);
            fprintf(f, "current_user=%s\n", current_user);
            fprintf(f, "last_key=%s\n", last_key_str);
            fprintf(f, "auth_screen=%d\n", auth_screen);
            fprintf(f, "input_mode=%d\n", input_mode);
            fclose(f);
        }
        free(path);
    }
    
    if (asprintf(&path, "%s/pieces/apps/player_app/manager/state.txt", project_root) != -1) {
        FILE *f = fopen(path, "w");
        if (f) {
            fprintf(f, "project_id=user\n");
            fprintf(f, "session_status=%s\n", session_status);
            fprintf(f, "current_user=%s\n", current_user);
            fprintf(f, "app_title=USER AUTHENTICATION\n");
            fclose(f);
        }
        free(path);
    }

}

/* ============================================================================
 * LAYOUT RESOLUTION
 * ============================================================================ */

static char my_layout_path[MAX_PATH] = "pieces/apps/user/layouts/user.chtpm";

static void resolve_my_layout(void) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/user/user.pdl", project_root) == -1) return;
    FILE *f = fopen(path, "r");
    if (f) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, "layout_path")) {
                char *pipe = strrchr(line, '|');
                if (pipe) strncpy(my_layout_path, trim_str(pipe + 1), MAX_PATH - 1);
            }
        }
        fclose(f);
    }
    free(path);
}

static int is_active_layout(void) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/display/current_layout.txt", project_root) == -1) return 0;
    FILE *f = fopen(path, "r");
    if (!f) {
        free(path);
        return 0;
    }
    
    char line[MAX_LINE];
    int result = 0;
    if (fgets(line, sizeof(line), f)) {
        result = (strstr(line, "user.chtpm") != NULL);
    }
    
    fclose(f);
    free(path);
    return result;
}

/* ============================================================================
 * TPM-COMPLIANT: Method Dispatch Tables
 * Maps key numbers to method names for each piece type
 * This is the ONLY place where key→method mapping happens
 * ============================================================================ */

typedef struct {
    int key;
    const char* method_name;
} MethodMapping;

/* Method mappings for each user piece - matches PDL METHOD order */
static MethodMapping user_main_methods[] = {
    {2, "login"},
    {3, "signup"},
    {4, "clear"},
    {0, NULL}  // sentinel
};

/* Get method mapping for current piece - always user_main for form-based UI */
static MethodMapping* get_current_methods(void) {
    return user_main_methods;
}

/* Get method name for a key, or NULL if not found */
static const char* get_method_name(int key) {
    MethodMapping* methods = get_current_methods();
    for (int i = 0; methods[i].method_name; i++) {
        if (methods[i].key == key) {
            return methods[i].method_name;
        }
    }
    return NULL;
}

/* ============================================================================
 * METHOD HANDLERS - One function per PDL method
 * Named to match PDL METHOD names exactly
 * ============================================================================ */

static void method_login(void) {
    /* Attempt login with current username/password */
    if (strlen(input_username) > 0 && strlen(input_password) > 0) {
        if (!user_exists(input_username)) {
            strcpy(auth_response, "User not found");
        } else if (verify_password(input_username, input_password) == 0) {
            start_session(input_username);
            strcpy(auth_response, "Login successful");
        } else {
            strcpy(auth_response, "Invalid password");
        }
    } else {
        strcpy(auth_response, "Enter username and password");
    }
}

static void method_signup(void) {
    /* Attempt signup with current username/password */
    if (strlen(input_username) > 0 && strlen(input_password) > 0) {
        if (user_exists(input_username)) {
            strcpy(auth_response, "User already exists");
        } else if (create_user_profile(input_username, input_password) == 0) {
            start_session(input_username);
            strcpy(auth_response, "Account created");
        } else {
            strcpy(auth_response, "Creation failed");
        }
    } else {
        strcpy(auth_response, "Enter username and password");
    }
}

static void method_clear(void) {
    /* Clear form fields */
    input_username[0] = '\0';
    input_password[0] = '\0';
    cli_buffer[0] = '\0';
    strcpy(auth_response, "Cleared");
}

/* Dispatch method by name - TPM COMPLIANT */
static void dispatch_method(const char* method_name) {
    if (!method_name) return;
    
    if (strcmp(method_name, "login") == 0) method_login();
    else if (strcmp(method_name, "signup") == 0) method_signup();
    else if (strcmp(method_name, "clear") == 0) method_clear();
}

/* ============================================================================
 * INPUT HANDLING - Uses method dispatch, NOT hardcoded key checks
 * ============================================================================ */

static void process_key(int key) {
    /* Clear response on any key */
    auth_response[0] = '\0';
    
    if (key >= 32 && key <= 126) {
        snprintf(last_key_str, sizeof(last_key_str), "%c", (char)key);
    } else if (key == 10 || key == 13) {
        strcpy(last_key_str, "ENTER");
    } else if (key == 27) {
        strcpy(last_key_str, "ESC");
    } else {
        snprintf(last_key_str, sizeof(last_key_str), "%d", key);
    }
    
    /* ESC - just deactivate, preserve all input */
    if (key == 27) {
        /* Do nothing - preserve username/password fields */
        return;
    }
    
    /* KISS: Parser handles cli_io input - module just processes method calls */
    /* Skip key capture - parser writes directly to state */
    
    /* TPM-COMPLIANT: Dispatch by method name, not hardcoded key numbers */
    /* Get method name for this key from dispatch table */
    const char* method = get_method_name(key);
    if (method) {
        dispatch_method(method);
        return;
    }
    
    /* Fallback for unmapped keys (optional - could log warning) */
}

/* ============================================================================
 * MAIN LOOP
 * ============================================================================ */

int main(void) {
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);
    setpgid(0, 0);
    
    srand((unsigned int)time(NULL));
    resolve_paths();
    resolve_my_layout();
    
    char *mkdir_cmd = NULL;
    if (asprintf(&mkdir_cmd, "mkdir -p %s/pieces/apps/user/pieces/session %s/pieces/apps/user/pieces/profiles", project_root, project_root) != -1) {
        run_command(mkdir_cmd);
        free(mkdir_cmd);
    }
    
    save_session_state("auth_required", "guest");
    clear_view();
    write_parser_state();
    write_manager_state();
    hit_frame_marker();
    
    char *hist_path = NULL;
    if (asprintf(&hist_path, "%s/pieces/apps/player_app/history.txt", project_root) == -1) return 1;
    
    struct stat st;
    
    while (!g_shutdown) {
        if (!is_active_layout()) {
            usleep(100000);
            continue;
        }
        
        if (stat(hist_path, &st) == 0) {
            if (st.st_size > last_history_pos) {
                FILE *hf = fopen(hist_path, "r");
                if (hf) {
                    fseek(hf, last_history_pos, SEEK_SET);
                    int key;
                    while (fscanf(hf, "%d", &key) == 1) {
                        process_key(key);
                    }
                    last_history_pos = ftell(hf);
                    fclose(hf);
                    
                    /* Write state AFTER processing all keys, triggers parser reload */
                    write_parser_state();
                    write_manager_state();
                    hit_frame_marker();
                }
            } else if (st.st_size < last_history_pos) {
                last_history_pos = 0;
            }
        }
        
        usleep(16667); /* 60 FPS */
    }
    
    free(hist_path);
    return 0;
}
