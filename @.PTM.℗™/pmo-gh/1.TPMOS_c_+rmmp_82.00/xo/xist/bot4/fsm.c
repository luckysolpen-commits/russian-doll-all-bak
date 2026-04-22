// fsm.c for Exo-Orchestrator Bot v4
// Simple Finite State Machine for orchestrator control.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>  // Required for sig_atomic_t

// --- Component Declarations ---
extern int initialize_tpmos_interaction(const char* target_dir);
extern void log_simulated_process(pid_t pid, const char* name);
extern void kill_all_simulated_processes();

// --- FSM Type Definition (must come before externs that use it) ---
typedef enum {
    FSM_INIT,
    FSM_LAUNCHING_CHILDREN,
    FSM_MONITORING,
    FSM_SHUTDOWN_SEQUENCE,
    FSM_DONE,
    FSM_ERROR
} FSMState;

// --- Global State ---
static FSMState current_state;
extern volatile sig_atomic_t should_exit;  // Flag to indicate shutdown

// --- FSM Function Declarations ---
int initialize_fsm();
FSMState get_current_fsm_state();
void transition_fsm_state(FSMState new_state);
void run_fsm_cycle();

// --- FSM Initialization ---
int initialize_fsm() {
    printf("Initializing Orchestrator FSM...\n");
    current_state = FSM_INIT;
    printf("FSM initialized. Current state: %d\n", current_state);
    return 0;
}

// --- Get current FSM state ---
FSMState get_current_fsm_state() {
    return current_state;
}

// --- Transition FSM state ---
void transition_fsm_state(FSMState new_state) {
    printf("FSM Transition: %d -> %d\n", current_state, new_state);
    current_state = new_state;
}

// --- Run a single FSM cycle ---
void run_fsm_cycle() {
    printf("Running Orchestrator FSM cycle...\n");
    switch (current_state) {
        case FSM_INIT:
            printf("FSM is INIT, should have transitioned.\n");
            break;
        case FSM_LAUNCHING_CHILDREN:
            printf("FSM is LAUNCHING_CHILDREN. Proceeding to MONITORING.\n");
            transition_fsm_state(FSM_MONITORING);
            break;
        case FSM_MONITORING:
            printf("FSM is MONITORING. Waiting for shutdown signal or completion.\n");
            // In a real orchestrator, this loop would check child processes.
            // For simulation, it just stays in this state until should_exit is set.
            break;
        case FSM_SHUTDOWN_SEQUENCE:
            printf("FSM is in SHUTDOWN_SEQUENCE.\n");
            // Orchestrator main loop will handle final cleanup based on this state.
            break;
        case FSM_DONE:
            printf("FSM is DONE.\n");
            break;
        case FSM_ERROR:
            printf("FSM encountered an error.\n");
            break;
        default:
            printf("FSM encountered unknown state: %d\n", current_state);
            break;
    }
}
