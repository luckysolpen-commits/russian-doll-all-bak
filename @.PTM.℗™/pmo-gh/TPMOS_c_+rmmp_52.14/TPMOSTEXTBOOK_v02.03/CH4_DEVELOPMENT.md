# 💪 Chapter 4: Muscles & Brains (Ops & Modules)
In TPMOS, we separate **Thinking** from **Doing**. This is the key to a stable, CPU-safe system. 🧠💪

---

## 🧠 The Brain (The Module)
The **Module** is a long-running background process (the "Brain"). Its job is to listen for user input and make decisions. 
*   **File:** Usually named `*_module.c` or `*_manager.c`.
*   **Job:** Polls the `history.txt` buffer and updates the `state.txt` mirror.

### 🛡️ The CPU-Safe Mandate
Because Modules run in the background, they must be "polite" to the CPU. We follow the **CPU-Safe Template**:
1.  **Throttling:** If the app isn't on screen, the Module sleeps for 100ms. If it is on screen, it sleeps for 16ms (60 FPS). 😴
2.  **Stat-First:** Don't read the history file unless its size has changed (`stat()`).
3.  **Signal Handling:** Always catch `SIGINT` (Ctrl+C) so you can clean up your children. 🧹

---

## 💪 The Muscle (The Op)
The **Op** is a short-lived binary (the "Muscle"). It does one thing and then exits. 
*   **File:** Usually ends in `.+x` (e.g., `move_entity.+x`).
*   **Job:** Performs a specific action like moving a player or placing a tile.

### 🚫 Why no `system()`?
We never use the `system()` command in C. Why? Because it's hard to kill! Instead, we use `fork()` and `exec()`.
*   **Fork:** Create a child process.
*   **Exec:** Turn that child into the Muscle.
*   **Wait:** The Brain waits for the Muscle to finish.

---

## 🛠️ The Developer Workflow
1.  **Pick a Template:** Copy `#.docs/future-facing/cpu_safe_module_template.c`.
2.  **Define your Logic:** What happens when the user presses 'W'? (Call `move_entity.+x north`).
3.  **Update the Mirror:** Write the new coordinates to `state.txt`.
4.  **Pulse the Frame:** Tell the Renderer that something changed! 💓

> 💡 **Pro Tip:** Keep your Muscles "dumb" and your Brains "thin". If a Muscle can do it, don't put it in the Brain! ⚡

---

## 🏛️ Scholar's Corner: The "Thin Brain" Revolution
In the early days of TPMOS, the `fuzz_legacy_manager.c` was over 5,000 lines of code. It handled movement, hunger, the market, and rendering all in one massive loop. It was a "Thick Brain." When one part of the code crashed, the entire pet simulation died! The "Thin Brain" revolution happened when we moved the movement logic into `move_entity.+x` and the hunger logic into PAL scripts. Suddenly, the manager was under 500 lines. The system became stable, and developers could finally sleep at night. 💤🧘‍♂️

---

## 📝 Study Questions
1.  Explain the difference between a "Module" and an "Op."
2.  Why is the `fork()` / `exec()` pattern preferred over the `system()` command?
3.  What are the three core rules of the "CPU-Safe Mandate"?
4.  **True or False:** A Module should sleep longer when its app is not the "active layout."

---
[Return to Index](INDEX.md)
