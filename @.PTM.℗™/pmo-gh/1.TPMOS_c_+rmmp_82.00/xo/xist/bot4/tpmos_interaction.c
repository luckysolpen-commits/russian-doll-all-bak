// tpmos_interaction.c for Exo-Orchestrator Bot v4
// Simulates interaction with TPMOS components, including process logging.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> // For file operations
#include <errno.h>
#include <sys/stat.h>
#include <sys/file.h> // For flock
#include <signal.h> // For SIGINT, SIGTERM
#include <stdarg.h> // For va_list, vsnprintf
#include <stdbool.h> // For bool type

#define SIMULATED_PROC_LIST_FILE "/tmp/bot4_proc_list.txt"
#define SIMULATED_LOG_FILE "/tmp/bot4_orchestrator.log"

static char g_target_tpmos_dir[256];

// --- Process Tracking ---
static void log_pid(pid_t pid, const char* name) {
    FILE* f = fopen(SIMULATED_PROC_LIST_FILE, "a"); // Append mode
    if (!f) {
        perror("TPMOS Interaction Error: Could not open proc_list.txt for logging");
        return;
    }
    flock(fileno(f), LOCK_EX); // Lock file for atomic write
    
    char timestamp[30];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm);

    fprintf(f, "[%s] Spawned PID: %d, Name: %s", timestamp, pid, name);
    fflush(f);
    fsync(fileno(f)); // Ensure data is written to disk
    flock(fileno(f), LOCK_UN);
    fclose(f);
    
    FILE* log_fp = fopen(SIMULATED_LOG_FILE, "a");
    if (log_fp) {
        fprintf(log_fp, "[%s] Logged process: PID=%d, Name=%s", timestamp, pid, name);
        fclose(log_fp);
    }

    printf("Simulated logging process: PID=%d, Name=%s", pid, name);
}

void kill_all_simulated_processes() {
    printf("Simulating kill_all_tracked_processes...");
    FILE* f = fopen(SIMULATED_PROC_LIST_FILE, "r");
    if (!f) {
        fprintf(stderr, "Simulated kill_all_tracked_processes: Cannot open %s", SIMULATED_PROC_LIST_FILE);
        return;
    }
    
    char line[256];
    // Read all lines first to ensure we have the list before clearing
    char pids_to_kill[10][256]; // Store lines to kill later
    int count = 0;
    while (fgets(line, sizeof(line), f) && count < 10) {
        strncpy(pids_to_kill[count++], line, sizeof(pids_to_kill[0]) - 1);
        pids_to_kill[count-1][sizeof(pids_to_kill[0]) - 1] = '\0';
    }
    fclose(f);
    
    // Phase 1: Simulate SIGTERM to all process groups
    for (int i = 0; i < count; ++i) {
        int pid;
        char name[128];
        if (sscanf(pids_to_kill[i], "[%*s] Spawned PID: %d, Name: %127s", &pid, name) == 2) {
            printf("Simulating SIGTERM for PID %d (%s)...", pid, name);
            // In a real scenario, this would call kill(-pid, SIGTERM) and kill(pid, SIGTERM)
        }
    }
    
    printf("Simulated graceful shutdown period (200ms)...");
    usleep(200000);
    
    // Phase 2: Simulate SIGKILL survivors
    f = fopen(SIMULATED_PROC_LIST_FILE, "r");
    if (!f) return; // File might have been cleared by cleanup or other process
    
    while (fgets(line, sizeof(line), f)) {
        int pid;
        char name[128];
        if (sscanf(line, "[%*s] Spawned PID: %d, Name: %127s", &pid, name) == 2) {
            printf("Simulating SIGKILL for PID %d (%s)...", pid, name);
            // In a real scenario, this would call kill(-pid, SIGKILL) and kill(pid, SIGKILL)
        }
    }
    fclose(f);
    
    // Clear the file after simulated kill
    f = fopen(SIMULATED_PROC_LIST_FILE, "w");
    if (f) fclose(f);
    printf("Simulated process list cleared.");
}

// Initialize TPMOS interaction simulation
int initialize_tpmos_interaction(const char* target_dir) {
    printf("Initializing TPMOS Interaction Simulation for Orchestrator...");
    if (target_dir == NULL || strlen(target_dir) == 0) {
        fprintf(stderr, "TPMOS Interaction Error: Target directory is NULL or empty.");
        return 1;
    }
    strncpy(g_target_tpmos_dir, target_dir, sizeof(g_target_tpmos_dir) - 1);
    g_target_tpmos_dir[sizeof(g_target_tpmos_dir) - 1] = '\0';
    printf("Simulated TPMOS Interaction target directory: %s", g_target_tpmos_dir);
    
    // Clear proc_list.txt for a clean run
    FILE* f_proc = fopen(SIMULATED_PROC_LIST_FILE, "w");
    if (f_proc) {
        fprintf(f_proc, "# Simulated Process List");
        fclose(f_proc);
    } else {
        perror("TPMOS Interaction Error: Could not clear simulated proc_list.txt");
        return 1;
    }

    // Clear orchestrator log
    FILE *f_log = fopen(SIMULATED_LOG_FILE, "w");
    if (f_log) {
        fprintf(f_log, "# Orchestrator Log");
        fclose(f_log);
    } else {
        perror("TPMOS Interaction Error: Could not clear simulated log file");
        return 1;
    }
    
    return 0;
}

// Cleanup TPMOS interaction resources
void cleanup_tpmos_interaction() {
    printf("Cleaning up TPMOS Interaction Simulation...");
    remove(SIMULATED_PROC_LIST_FILE);
    remove(SIMULATED_LOG_FILE);
    printf("TPMOS Interaction Simulation cleaned up.");
}
