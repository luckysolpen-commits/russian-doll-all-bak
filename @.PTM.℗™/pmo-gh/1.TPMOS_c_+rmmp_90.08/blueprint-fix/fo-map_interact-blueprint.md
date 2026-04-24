# Fuzz-Op Map & Interaction Blueprint (GLTPM Standard)

This document outlines the architecture for decoupled user input, state management, and rendering in a TPMOS project. It assumes a "virgin agent" is implementing a project from scratch.

## 1. Input Decoupling (The History Pipeline)

In a standard TPMOS project, input is NOT handled by direct polling of the keyboard by the manager. Instead, it follows a "History-First" pattern.

### A. The Input Files
- **Global History:** `pieces/apps/player_app/history.txt` (Legacy/System events).
- **Project History:** `projects/<project_id>/session/history.txt` (Project-specific events).

### B. Input Format
The project history file typically receives appended lines from the UI/Parser:
`[YYYY-MM-DD HH:MM:SS] KEY_PRESSED: 1002` (where 1002 is a key code)
`[YYYY-MM-DD HH:MM:SS] COMMAND: OP fuzz-op::scan`

### C. The Listener (Manager Thread)
The Manager runs a background thread that:
1.  Stores the `last_pos` (byte offset) of the history file.
2.  Polls the file every **~16ms (60 FPS)** for new data (`st_size > last_pos`).
3.  Reads only the new content, parses the key/command, and dispatches it to `route_input()`.

---

## 2. Signaling & Markers (The Pulse)

TPMOS avoids expensive file-size polling for rendering. It uses **Marker Files** (the "Pulse") to wake up the system.

### A. The Signal Files
- **Frame Marker:** `projects/<project_id>/session/frame_changed.txt`
- **State Marker:** `pieces/apps/player_app/state_changed.txt`

### B. The Protocol
When a change occurs (input processed, entity moved, state updated):
1.  **Write Logic:** Append a single character (e.g., `G\n` for GL-OS or `M\n` for Map) to the marker file.
2.  **Read Logic:** The Renderer/Parser `stat()`s the marker file. If it has grown, it performs a render cycle.

---

## 3. Method Execution (PDL-Driven Interaction)

Interactions are not hardcoded. They are discovered at runtime from the active Piece's DNA.

### A. Discovery
1.  Identify the `active_target_id` (e.g., `pet_01`) from the manager state.
2.  Call `pdl_reader.+x <piece_id> list_methods`.
3.  Map indices 2-8 to the returned list.

### B. Execution
1.  When a user presses a "Method Key" (e.g., '2'), the manager finds the 2nd method name (e.g., `feed`).
2.  Call `pdl_reader.+x <piece_id> get_method <name>` to get the handler path.
3.  Execute the handler (typically a `+x` Op) using `fork()/exec()`.

---

## 4. Map Updates & Rendering

The map is a "Theater" that reflects the current state of all Pieces in a location.

### A. The Update Trigger
After any movement or method execution:
1.  **Sync Mirror:** Update `projects/<project_id>/pieces/<piece_id>_mirror/state.txt` so the UI has a local copy.
2.  **Trigger Render Op:** Call `pieces/apps/playrm/ops/+x/render_map.+x`.

### B. The Rendering Chain
1.  `render_map.+x` reads the current `map_id` (e.g., `map_01_z0.txt`).
2.  It iterates through all Piece directories in that map/location.
3.  It reads their `pos_x`, `pos_y`, and `icon` (or `char`) from `state.txt`.
4.  It composes a text grid (the "Frame").
5.  It writes the frame to `pieces/display/current_frame.txt`.
6.  **Crucial:** The Manager then hits the `frame_changed.txt` marker to tell the UI to refresh.

---

## 5. Summary Lifecycle

1.  **USER ACTION** -> Appended to `session/history.txt`.
2.  **MANAGER** -> Detects growth, reads key, calls `route_input()`.
3.  **MANAGER** -> Calls Movement Op or PDL-Method Op.
4.  **OP** -> Modifies Piece `state.txt`.
5.  **MANAGER** -> Calls `render_map.+x`.
6.  **RENDERER** -> Reads all pieces, writes `current_frame.txt`.
7.  **MANAGER** -> Appends `G\n` to `session/frame_changed.txt`.
8.  **UI/GL-OS** -> Detects `G\n`, reads `current_frame.txt`, and draws.

---
*Language Agnostic Implementation Note:* This pattern works in C, Python, or Shell. "If it's not in a file, it's a lie. The marker is the clock."
