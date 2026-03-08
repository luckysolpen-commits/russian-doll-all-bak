#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>

// TPM OS Process Manager (v1.1 - COMMAND DRIVEN)
// Responsibility: Manage PID pieces in pieces/os/procs/

char project_root[1024] = ".";
char os_procs_dir[1024] = "pieces/os/procs";

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

void resolve_paths() {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[2048];
        while (fgets(line, sizeof(line), kvp)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(line);
                char *v = trim_str(eq + 1);
                if (strcmp(k, "project_root") == 0) strncpy(project_root, v, sizeof(project_root)-1);
                else if (strcmp(k, "os_procs_dir") == 0) strncpy(os_procs_dir, v, sizeof(os_procs_dir)-1);
            }
        }
        fclose(kvp);
    }
}

void log_to_ledger(const char* fmt, ...) {
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[100];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    char path[1024];
    build_path(path, sizeof(path), "%s/pieces/master_ledger/master_ledger.txt", project_root);
    FILE *f = fopen(path, "a");
    if (f) {
        fprintf(f, "[%s] ", timestamp);
        va_list args;
        va_start(args, fmt);
        vfprintf(f, fmt, args);
        va_end(args);
        fprintf(f, "\n");
        fclose(f);
    }
}

int main(int argc, char *argv[]) {
    resolve_paths();
    if (argc < 2) {
        printf("Usage: %s [register|list|stop|cont|unregister]\n", argv[0]);
        return 1;
    }

    const char *action = argv[1];

    if (strcmp(action, "register") == 0 && argc >= 4) {
        const char *name = argv[2];
        int pid = atoi(argv[3]);
        if (kill(pid, 0) == 0) {
            char path[1024];
            build_path(path, sizeof(path), "%s/%s.pid", os_procs_dir, name);
            FILE *f = fopen(path, "w");
            if (f) {
                fprintf(f, "%d", pid);
                fclose(f);
                log_to_ledger("ProcessRegister: %s PID=%d", name, pid);
                printf("Registered %s (PID %d)\n", name, pid);
            }
        } else {
            printf("Error: PID %d is not running.\n", pid);
            return 1;
        }
    } else if (strcmp(action, "list") == 0) {
        DIR *dir = opendir(os_procs_dir);
        if (!dir) return 1;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strstr(entry->d_name, ".pid")) {
                char path[1024], name[64];
                strncpy(name, entry->d_name, sizeof(name)-1);
                char *dot = strchr(name, '.'); if (dot) *dot = '\0';
                
                build_path(path, sizeof(path), "%s/%s", os_procs_dir, entry->d_name);
                FILE *f = fopen(path, "r");
                if (f) {
                    int pid;
                    if (fscanf(f, "%d", &pid) == 1) {
                        const char *status = "DEAD";
                        if (kill(pid, 0) == 0) {
                            // Check if stopped
                            char proc_path[256];
                            snprintf(proc_path, sizeof(proc_path), "/proc/%d/status", pid);
                            FILE *pf = fopen(proc_path, "r");
                            status = "RUNNING";
                            if (pf) {
                                char line[256];
                                while (fgets(line, sizeof(line), pf)) {
                                    if (strncmp(line, "State:", 6) == 0) {
                                        if (strstr(line, "T (stopped)")) status = "STOPPED";
                                        break;
                                    }
                                }
                                fclose(pf);
                            }
                        }
                        printf("[%s] %s: %d\n", status, name, pid);
                    }
                    fclose(f);
                }
            }
        }
        closedir(dir);
    } else if (strcmp(action, "stop") == 0 && argc >= 3) {
        const char *name = argv[2];
        char path[1024];
        build_path(path, sizeof(path), "%s/%s.pid", os_procs_dir, name);
        FILE *f = fopen(path, "r");
        if (f) {
            int pid;
            if (fscanf(f, "%d", &pid) == 1) {
                if (kill(pid, SIGSTOP) == 0) {
                    log_to_ledger("ProcessStop: %s PID=%d", name, pid);
                    printf("Stopped %s (PID %d)\n", name, pid);
                }
            }
            fclose(f);
        }
    } else if (strcmp(action, "cont") == 0 && argc >= 3) {
        const char *name = argv[2];
        char path[1024];
        build_path(path, sizeof(path), "%s/%s.pid", os_procs_dir, name);
        FILE *f = fopen(path, "r");
        if (f) {
            int pid;
            if (fscanf(f, "%d", &pid) == 1) {
                if (kill(pid, SIGCONT) == 0) {
                    log_to_ledger("ProcessCont: %s PID=%d", name, pid);
                    printf("Continued %s (PID %d)\n", name, pid);
                }
            }
            fclose(f);
        }
    } else if (strcmp(action, "unregister") == 0 && argc >= 3) {
        const char *name = argv[2];
        char path[1024];
        build_path(path, sizeof(path), "%s/%s.pid", os_procs_dir, name);
        if (remove(path) == 0) {
            log_to_ledger("ProcessUnregister: %s", name);
            printf("Unregistered %s\n", name);
        }
    }

    return 0;
}
