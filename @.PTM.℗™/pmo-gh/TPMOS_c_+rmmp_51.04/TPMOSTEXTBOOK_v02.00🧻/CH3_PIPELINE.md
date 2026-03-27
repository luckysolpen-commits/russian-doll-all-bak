# 💓 Chapter 3: The 12-Step Pulse
How does a single keypress turn into a pixel on your screen? It's a 12-step journey through the system's "Circulatory System". 🩸

---

## 🏃‍♂️ The 12-Step Sprint

1.  **⌨️ INPUT:** `keyboard_input.+x` (the Muscle) catches your key and scribbles it into `pieces/keyboard/history.txt`.
2.  **🗺️ ROUTING:** `chtpm_parser.c` (the OS) reads the key. It uses **W-first (World-First)** resolution, checking `world_xx/map_xx/` paths before legacy fallbacks. 🗺️
3.  **📢 RELAY:** If you are "interacting" with an app, the Parser throws the key into the app's own history buffer (e.g., `player_app/history.txt`).

### 🌓 The Dual Execution Model
Once the key is relayed, the system enters one of two "modes":

*   **⚡ Mode 1: Realtime Input (Direct C)**
    *   **Latency:** ~16ms (Instant). 
    *   **Action:** Movement (WASD), Menu Navigation.
    *   **Path:** Module calls Muscle Op directly. No scripting involved. 
*   **📜 Mode 2: Event Scripts (Prisc Scripting)**
    *   **Latency:** ~50-100ms. 
    *   **Action:** NPC Dialogue, Quests, AI triggers.
    *   **Path:** Module calls `prisc+x` -> executes `event.asm` -> calls Muscle Ops.

4.  **🧠 TICK:** The App Module (the Brain) wakes up! It sees the new key in its history and decides which Mode to use.
5.  **💪 TRAIT/OP:** The Brain says, "The user pressed 'UP'. Call the `move_entity.+x` Muscle!"
6.  **🧱 SOVEREIGNTY:** The `move_entity` Muscle checks the boundaries and updates the Piece's `state.txt` (e.g., `y=y-1`).
7.  **🪞 MIRROR SYNC:** The Muscle flushes the new state to disk. Reality is now officially updated.
8.  **🎬 STAGE:** `render_map.+x` (the Stage Producer) sees the change. It reads ALL the Pieces on the map and draws a "View" (`view.txt`).
9.  **🔄 SYNC:** The Stage Producer updates the app's general state so the UI knows where the camera is.
10. **🎨 COMPOSITION:** The Parser (the Theater) takes the `view.txt` and swaps out all the variables (like `${player_health}`).
11. **🖼️ RENDER:** The Parser writes the final, beautiful ASCII frame to `current_frame.txt`.
12. **📺 DISPLAY:** The Renderer (ASCII or GL) reads `current_frame.txt` and blasts it onto your monitor!

---

## ⏱️ The Pulse Frequency
This entire 12-step loop happens in milliseconds. It is the "Heartbeat" of TPMOS. 💓

> ⚠️ **Warning:** If any step takes too long (e.g., a slow script), the pulse "stutters". This is why we keep Muscles small and fast! ⚡

---

## 🏛️ Scholar's Corner: The "Pulse That Never Ended"
During an early experiment with recursive PAL scripts (Chapter 7), a developer accidentally created a loop where a bot would "move north" every time the frame changed. Because the pulse was so fast, the bot moved through 10,000 maps in under a minute! The developer tried to kill the process, but every time a new frame rendered, a new bot was spawned. This became known as **"The Pulse That Never Ended."** It taught us the critical importance of the **Kill All** script and the value of throttling (Chapter 4). 💓♾️

---

## 📝 Study Questions
1.  Describe the journey of a keypress from Step 1 to Step 12.
2.  What is the role of the "Stage Producer" in the 12-step pipeline?
3.  Why is "Mirror Sync" (Step 7) so critical before the "Stage" (Step 8) begins?
4.  If the renderer "stutters," which step in the pipeline is most likely the culprit?

---
[Return to Index](INDEX.md)
