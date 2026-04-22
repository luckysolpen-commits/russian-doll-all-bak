// main.c for Exo-Orchestrator Bot v4
// Orchestrates simulated child processes and manages FSM.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>

// --- Component Declarations ---
extern int initialize_tpmos_interaction(const char* target_dir);
extern void cleanup_tpmos_interaction();
extern int log_simulated_process(pid_t pid, const char* name);
extern void kill_all_simulated_processes();

// --- FSM Declarations ---
typedef enum {
    FSM_INIT,
    FSM_LAUNCHING_CHILDREN,
    FSM_MONITORING,
    FSM_SHUTDOWN_SEQUENCE,
    FSM_DONE,
    FSM_ERROR
} FSMState;

static FSMState current_state;
volatile sig_atomic_t should_exit = 0;

extern int initialize_fsm();
extern FSMState get_current_fsm_state();
extern void transition_fsm_state(FSMState new_state);
extern void run_fsm_cycle();

// --- Orchestrator Logic ---
void run_orchestrator_loop() {
    printf("Orchestrator Bot v4: Starting execution loop...\n");

    // --- Phase 1: Initialize and Launch Children ---
    printf("Orchestrator: Transitioning to LAUNCHING_CHILDREN state.\n");
    transition_fsm_state(FSM_LAUNCHING_CHILDREN);

    printf("Simulating launch of child processes...\n");
    pid_t child1_pid = 1001;
    pid_t child2_pid = 1002;
    log_simulated_process(child1_pid, "sim_child_A");
    log_simulated_process(child2_pid, "sim_child_B");
    printf("Simulated child processes launched and logged.\n");

    // --- Phase 2: Monitoring State ---
    printf("Orchestrator: Transitioning to MONITORING state.\n");
    transition_fsm_state(FSM_MONITORING);

    int cycles = 0;
    while (current_state == FSM_MONITORING && cycles < 5) {
        printf("Orchestrator: Monitoring... (cycle %d)\n", cycles + 1);
        usleep(500000);
        run_fsm_cycle();
        cycles++;
    }

    if (should_exit) {
        printf("Shutdown signal detected. Transitioning to SHUTDOWN_SEQUENCE.\n");
        transition_fsm_state(FSM_SHUTDOWN_SEQUENCE);
    } else if (get_current_fsm_state() == FSM_DONE) {
        printf("FSM reached DONE state normally.\n");
    } else {
        fprintf(stderr, "Orchestrator loop finished unexpectedly. State: %d\n", current_state);
        transition_fsm_state(FSM_ERROR);
    }
}

// --- Signal Handler ---
static void handle_orchestrator_sigint(int sig) {
    (void)sig;
    should_exit = 1;
    printf("Orchestrator received signal, initiating shutdown...\n");
    kill_all_simulated_processes();
    _exit(0);
}

#ifdef _WIN32
#include <windows.h>
BOOL WINAPI console_ctrl_handler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        handle_orchestrator_sigint(SIGINT);
        return TRUE;
    }
    return FALSE;
}
#endif

// --- Placeholder for dynamic path building ---
void build_path(char* dst, size_t sz, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(dst, sz, fmt, args);
    va_end(args);
}

// --- Placeholder for path resolution ---
char project_root[1024] = ".";
void resolve_root() {
    if (getcwd(project_root, sizeof(project_root)) == NULL) {
        perror("Error getting current working directory");
        strcpy(project_root, ".");
    }
    printf("Resolved project root to: %s\n", project_root);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <target_tpmos_directory>\n", argv[0]);
        return 1;
    }

    const char* target_dir = argv[1];

    if (initialize_tpmos_interaction(target_dir) != 0) {
        fprintf(stderr, "Failed to initialize TPMOS interaction simulation.\n");
        return 1;
    }
    printf("TPMOS Interaction simulation initialized.\n");

    if (initialize_fsm() != 0) {
        fprintf(stderr, "Failed to initialize FSM.\n");
        cleanup_tpmos_interaction();
        return 1;
    }
    printf("FSM initialized.\n");

    signal(SIGINT, handle_orchestrator_sigint);
    signal(SIGTERM, handle_orchestrator_sigint);
#ifdef _WIN32
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)console_ctrl_handler, TRUE);
#endif

    run_orchestrator_loop();

    // --- Shutdown Sequence ---
    if (current_state == FSM_MONITORING || current_state == FSM_SHUTDOWN_SEQUENCE || should_exit) {
        printf("Initiating shutdown sequence...\n");
        if (!should_exit) {
            should_exit = 1;
        }
        kill_all_simulated_processes();
        transition_fsm_state(FSM_DONE);
    }

    printf("Exiting orchestrator.\n");
    cleanup_tpmos_interaction();
    printf("Orchestrator finished.\n");

    return (get_current_fsm_state() == FSM_DONE) ? 0 : 1;
}
