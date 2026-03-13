# TPM (True Piece Method) & CHTPM OS Monolith Documentation
**Date:** 2026-03-10
**Status:** AUTHORITATIVE CANONICAL MONOLITH
**Version:** 5.0 (Omni-Sovereign Edition)

---

## 1. Core Philosophy: The PMO Hierarchy
The system follows a strict thought priority: **PIECE -> MODULE -> OS**. This ensures bottom-up stability and auditable reality.

### 1.1 PIECE (The Atomic Unit / Soul)
- **Definition:** A directory containing text files that define a "Piece" of reality.
- **Sovereignty:** A Piece owns its state exclusively. No in-memory shortcuts.
- **Components:**
    - `piece.pdl` (DNA): The ultimate source of truth. Defines state, methods, and responses.
    - `state.txt` (Mirror): A flat `key=value` file optimized for high-speed reads (Direct Mirror Access).
    - `ledger.txt`: Piece-specific audit trail.
    - `plugins/+x/`: Piece-specific behavior binaries.
- **Ontology:** Scale does not change rules. A NAND gate, a Pet, and a Galaxy are all Pieces.

### 1.2 MODULE (The Logic Agent / Brain)
- **Definition:** Orchestrates Pieces to perform complex actions.
- **Responsibility:** Interprets input, executes Ops, and performs the **Mirror Sync**.
- **Mandate:** Stateless Logic. Relies on Piece state for persistence.
- **Always Pulse:** Modules MUST poll history continuously. Never skip a pulse, even when waiting for a re-render.

### 1.3 OS / CHTPM (The Theater / View)
- **Definition:** The environment, navigation, and user interface.
- **Responsibility:** Substitutes variables (`${var}`), routes input (Nav vs. Interact), and composes the final frame.
- **Mantra:** "The Parser is the mirror's eye, not the mirror's mind."

---
## 2. The 12-Step Pipeline (Input to Render)
1. **INPUT:** `keyboard_input.+x` writes ASCII codes to `pieces/keyboard/history.txt`.
2. **ROUTING:** `chtpm_parser.c` reads the key.
3. **RELAY:** If in **INTERACT** mode, Parser "injects" the key into the buffer specified by the `<interact src="...">` tag (typically `pieces/apps/player_app/history.txt`).
4. **TICK:** The App Module (e.g., `editor_module.c` or `fuzz_legacy_manager.c`) polls this shared history buffer.
5. **TRAIT/OP:** The Module calls a standardized "Muscle" Op (e.g., `move_entity.+x`) to process logic.
6. **SOVEREIGNTY:** The Op updates the Piece DNA (`.pdl`) or Mirror (`state.txt`) within the project's `pieces/` directory.
7. **MIRROR SYNC:** The update is flushed to the Piece Mirror (`state.txt`) for high-speed read access.
8. **STAGE:** The Stage Producer (`render_map.+x`) reads all Mirrors and writes `view.txt`.
9. **SIGNAL:** Module/Op appends to `view_changed.txt` and hits `frame_changed.txt`.
10. **COMPOSITION:** Parser substitutes `${game_map}` from `view.txt` and loads system variables (Clock, Turn).
11. **RENDER:** Parser writes the final composite frame to `current_frame.txt`.
12. **DISPLAY:** The `renderer` (ASCII or GL) prints the frame to the user.

---

## 3. Advanced Capabilities

### 3.1 Prisc & PAL (Assembly Scripting)
**PAL (Prisc Assembly Language)** is a RISC-like scripting language for game events.
- **Prisc VM:** `prisc+x` executes PAL scripts (`.asm`).
- **Purpose:** Allows users to create complex dialogue trees, quests, and AI without recompiling C code.
- **Integration:** Modules call `prisc+x <script.asm>`, which then calls Ops (Muscles) to modify world state.

### 3.2 P2P & Blockchain (The Decentralized Layer)
- **Wallet System:** RSA 2048-bit keypairs. Wallet address is a hash of the public key.
- **Mining:** SHA256 Proof-of-Work (PoW) integrated into the Piece pipeline.
- **NFT Inventory:** Unique item hash generation with rarity levels (Legendary, Epic, Unique).
- **Auctions:** Item escrow (removing from inventory to auction Piece) with blockchain logging.
- **P2P Networking:** TCP socket server with Hello/Ack handshake protocol.

### 3.3 Web Stack Parity (PHP/JS)
- **Bridge.php:** Provides file-based IPC for environments where C binaries cannot run.
- **Virtual Reflection:** UI clicks are mapped to virtual key codes and injected into the history file.
- **Adaptive Polling:** Maintaining 60 FPS during interaction while dropping to 10 FPS during idle to save CPU.

### 3.4 3D & GL Rendering
- **GL Renderer:** Uses FreeType for emoji support and OpenGL for high-performance visual state representation.
- **3D Future:** Planning for Piece-based 3D coordinate systems and projection renderers.

### 3.5 Dynamic Interaction (<interact> Tag)
The `<interact>` layout tag now supports variable substitution (e.g., `src="projects/${project_id}/history.txt"`). This allows layouts to dynamically route input to project-specific or unified buffers.
- **Unified Buffer:** `pieces/apps/player_app/history.txt` is the standard for multi-project compatibility.
- **Project Buffer:** `projects/${project_id}/history.txt` is used for isolated project logic.

---

## 4. Technical Pitfalls (The "7 Deadly Pitfalls" + Extensions)

1.  **THE TRIM TRAP:** Filesystem mirrors often have trailing spaces/newlines. Always `trim()` values before comparison.
2.  **popen OVERHEAD:** Calling `popen` 100 times per frame will kill performance. Use **Direct Mirror Access** (parsing `state.txt` directly in C).
3.  **mtime RESOLUTION:** Filesystem `mtime` has 1s resolution. Use **Marker-File Size Detection** for sub-millisecond synchronization.
4.  **BUFFER TRUNCATION:** Large variables (maps) in tag attributes can cause memory corruption. Use **Hybrid Variable Expansion**.
5.  **RELATIVE PATH CONFUSION:** Never hardcode `../`. Always resolve from `location_kvp` project root.
6.  **OVERWRITING STRUCTURE:** `write_piece_state()` must be section-aware to avoid wiping out methods/responses in the `.pdl`.
7.  **REDUNDANT RESPONSES:** Background stdout is lost. Use `last_response.txt` and bake it into the frame.
8.  **THE DUAL INJECTION BUG:** Don't inject both numeric and joystick codes for a single action. Parity means equivalence, not duplication.
9.  **THE STALE ROOT DRIFTER:** Ensure `run_chtpm.sh` resolves `project_root` dynamically at runtime to prevent writing to old version folders.
10. **THE USE-AFTER-FREE PULSE:** Never `free()` a path string before the `fopen()` call is finished.

---

## 5. Architectural Nuances
- **Training Parity:** Align UI indices with INTERACT hotkeys (e.g., Button 2 is "Feed", and Key '2' in interact mode also feeds). The UI is the "Legend".
- **Silent Daemon Pattern:** Background daemons (like the Clock) must hit the global `frame_changed.txt` marker, or their work will never be rendered.
- **Shadow Selector:** In entity control mode, the selector ('X') is hidden but follows the entity's coordinates. ESC returns control to the selector at that spot.
- **Parser Sovereignty:** Exactly one writer (Parser), many readers (ASCII/GL Renderers). Never swap roles.

---

## 6. Future Plans (The Roadmap)
- **AI/LLM Integration:** Pieces as "Sovereign Minds" with PAL-driven decision trees and RL loops.
- **Master Clock:** Chaining all temporal events to a unified system Piece.
- **Cross-Node Trading:** Implementing auction/trade protocols between distributed P2P TPM nodes.
- **CRM Integration:** Pieces representing customers/leads with auditable history logs.
- **GL 3D World:** Transitioning the "Stage" from 2D ASCII to Piece-driven 3D coordinates.

---
"If it's not in a file, it's a lie."
