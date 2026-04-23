# Post-Mortem: CLI_IO Live Submission & Focus Fix (A22)

## Executive Summary
The "Live Input Submission" feature, critical for chat and CLI-style applications within TPMOS, was successfully implemented after multiple failed attempts by previous agents. The breakthrough involved moving from a "trigger-based sync" model to a "continuous live-sync" model, coupled with restoring the full module lifecycle for the Chat-Op manager.

## Why Previous Agents Failed
1.  **State Desynchronization (The "Ghost Buffer"):** Previous agents relied on the parser to capture input in an internal buffer and only "flush" it to a file when Enter was pressed. This created a race condition where the manager would receive the "Enter" signal but find an empty or stale file.
2.  **Manager "Brain" Absence:** The `chat-op_manager.c` had degraded into a code snippet. It had the logic for processing keys but lacked the "heartbeat" (main loop, history polling, signal handling) required to actually execute in the background. It was a brain without a body.
3.  **The "Exit-on-Enter" Assumption:** The core parser was hardcoded to deactivate input fields (`active_index = -1`) upon Enter. While this works for one-off form fields (username/password), it is fatal for chat UX.
4.  **Silent Crashes (Stack Overflow):** As the system grew, the large local arrays used in state-writing functions exceeded the stack limits of the orchestrator's environment. Previous agents likely saw the system "stop accepting input" and assumed logic errors, when the process was actually aborting.

## The Superior Solution (A22)
1.  **Atomic Live Sync:** Modified `chtpm_parser.c` to call `save_to_gui_state()` on **every keypress**. This ensures that `gui_state.txt` is always a 1:1 mirror of what the user sees on screen.
2.  **Full Module Restoration:** Rebuilt the manager using the `cpu_safe_module_template.c`. It now correctly polls history using `stat()`-first efficiency and manages its own state persistence.
3.  **Declarative UI Automation:** Updated the parser's `load_vars()` to automatically detect and load `manager/gui_state.txt` for any project. This removed the need for hardcoded per-project variable loading.
4.  **Heap-Safe Persistence:** Refactored the state-saving logic to use heap allocation (`malloc/free`) instead of large stack arrays, eliminating the termination/crash bugs.
5.  **Focus Retention:** Standardized the protocol where `cli_io` elements stay active after Enter if they are part of a continuous input flow.

## Lessons for Future Agents
-   **Filesystem is the Bus:** If you want two processes (Parser and Manager) to share state, sync it early and often. Don't wait for a "submit" event.
-   **Respect the Template:** Never write a manager as a fragment. Use the `cpu_safe_module_template.c` to ensure the process actually lives.
-   **Render Ownership:** Ensure the parser knows whether to show its internal buffer (Live) or the substituted variable (Static). Mixing them causes "double-display" artifacts.

---
*Documented by Superior Agent A22 - April 22, 2026*
