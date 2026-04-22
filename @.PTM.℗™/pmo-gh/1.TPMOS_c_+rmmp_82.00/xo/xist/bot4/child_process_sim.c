// child_process_sim.c for Exo-Orchestrator Bot v4
// Simulates child processes that the orchestrator can launch.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>      // For sleep, getpid
#include <signal.h>      // For signal handling
#include <sys/stat.h>
#include <sys/file.h>    // For flock

#define SIMULATED_PROC_LIST_FILE "/tmp/bot4_proc_list.txt"
#define LOG_FILE "/tmp/child_process.log"
#define MAX_NAME_LEN 128

// Global flag for child process shutdown
volatile sig_atomic_t child_should_exit = 0;

// Simulate process activity
void child_main_loop(const char* name, pid_t pid) {
    printf("[%s - PID %d] Child process started.\n", name, pid);

    // Simulate process registration (e.g., to a shared file)
    FILE* f = fopen(SIMULATED_PROC_LIST_FILE, "a"); // Append mode
    if (f) {
        flock(fileno(f), LOCK_EX);
        char timestamp[30];
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm);
        fprintf(f, "[%s] Spawned PID: %d, Name: %s\n", timestamp, pid, name);
        fflush(f);
        fsync(fileno(f));
        flock(fileno(f), LOCK_UN);
        fclose(f);
    } else {
        perror("Child process: Could not open proc_list.txt for logging");
    }

    // Simulate work
    for (int i = 0; i < 5 && !child_should_exit; ++i) {
        printf("[%s - PID %d] Working... step %d\n", name, pid, i + 1);
        sleep(1); // Simulate work
    }
    printf("[%s - PID %d] Exiting gracefully.\n", name, pid);
    exit(0); // Normal exit
}

// Basic signal handler for graceful exit
static void handle_child_sigint(int sig) {
    (void)sig; // Unused parameter
    child_should_exit = 1;
    printf("Child process received signal, initiating shutdown...\n");
    // In a real child, this would trigger specific cleanup.
    // For simulation, we just set the flag and exit.
    exit(0);
}

// Main function for the simulated child process
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <process_name>\n", argv[0]);
        return 1;
    }

    const char *process_name = argv[1];
    pid_t pid = getpid();

    // Register signal handler for graceful shutdown
    signal(SIGINT, handle_child_sigint);
    signal(SIGTERM, handle_child_sigint);

    child_main_loop(process_name, pid);

    return 0; // Should ideally not be reached if loop exits via signal/exit
}
