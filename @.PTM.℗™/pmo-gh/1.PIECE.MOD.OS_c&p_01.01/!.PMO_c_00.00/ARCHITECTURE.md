# CHTPM+OS Architecture (PMO Standard)

**Date:** 2026-02-23  
**Status:** CANONICAL - Standard for Piece-Module-OS Systems  
**Supersedes:** All prior CHTPM/CHTRM documentation.

---

## 1.0 Overview

CHTPM is a **TPM-compliant implementation of the Piece-Module-OS (PMO) framework**. It is a terminal-based UI system where:

1. **Markup files** (`.chtpm`) define the UI layout and OS navigation.
2. **Modules** (Stateless Executables) handle application-specific logic and view composition.
3. **Filesystem API** (not pipes) provides the communication layer via `history.txt` and `state.txt`.
4. **Data Sovereignty** is maintained via the Piece directory structure and Master Ledger auditing.

---

## 2.0 The Hierarchy of Sovereignty (P-M-O)

1.  **PIECE (Atomic):** The directory holding the "DNA" (.pdl) and "Mirror" (state.txt) of an entity.
2.  **MODULE (Brain):** The logic that animates Pieces. In active mode, the Module owns the logic AND the frame composition.
3.  **OS / LAYOUT (Viewer):** The host (`chtpm_parser`) that manages navigation between modules and provides the UI shell.

---

## 3.0 The Filesystem API (Communication)

We forbid "Pipe-based IPC" (stdin/stdout) for business logic. In PMO, **The Filesystem is the Brain**.

### 3.1 Input Routing (Relay Mode)
*   **Parser** reads global `keyboard/history.txt`.
*   If an interactive element is active, Parser calls `inject_raw_key(code)` to append to the **Module's** `history.txt`.

### 3.2 State Synchronization (Mirror Sync)
*   Modules update **Master DNA** (.pdl) via `piece_manager` for the audit trail.
*   Modules immediately flush to **Fast Mirror** (state.txt) for OS/Parser variable substitution.

---

## 4.0 Component Responsibilities

### 4.1 chtpm_parser.c (The OS / Host)
1.  **Navigation:** Manages `focus_index` and `active_index` for UI elements.
2.  **Relay:** Injects raw keys into the active Module's history.
3.  **Silent Mode:** In `INTERACT` mode, the Parser **MUST NOT** write to `current_frame.txt`. It yields the screen to the Module.
4.  **Composition:** Composes UI-only frames (menus, launchers) when no module is active.

### 4.2 Module (The Animator)
1.  **Logic:** Consumes keys from its own `history.txt` and updates Piece state.
2.  **Eager Composition:** Generates the **FULL ASCII Frame** immediately after a logic tick.
3.  **Signaling:** Appends "X\n" to `frame_changed.txt` after every frame write.

---

## 5.0 Canonical Directory Structure (Macro Folder Pattern)

```
project_root/
├── pieces/
│   ├── system/                     # Global Pieces
│   │   ├── keyboard/               # history.txt (Input)
│   │   ├── display/                # current_frame.txt (Output)
│   │   └── master_ledger/          # frame_changed.txt (Signal)
│   │
│   ├── apps/                       # MACRO FOLDERS (Portability)
│   │   └── fuzzpet_app/            # Self-contained ecosystem
│   │       ├── fuzzpet/            # Pet DNA & Mirror
│   │       ├── world/              # Map data
│   │       └── plugins/            # Logic (+x binaries)
│   │
│   └── locations/                  # Path Resolution
│       └── location_kvp            # Dynamic paths
```

---

## 6.0 The KISS Pipeline Flow (User Input to Render)

1.  **User hits 'w'** -> `keyboard_input` writes to `keyboard/history.txt`.
2.  **Parser** sees 'w', detects `INTERACT` mode -> `inject_raw_key(119)` to `fuzzpet/history.txt`.
3.  **Module** reads 119 -> updates `pos_y` in `.pdl` and `state.txt`.
4.  **Module** composes ASCII Dashboard -> writes to `display/current_frame.txt`.
5.  **Module** hits marker -> `frame_changed.txt` size increases.
6.  **Renderer** sees size change -> prints frame to terminal.

---

## 7.0 Critical Nuances (The PMO Laws)

*   **Heartbeat Rule:** Every key received by a module MUST trigger a frame write, even if invalid.
*   **Transition Sovereignty:** The OS Parser writes one final frame when entering a module to show the `[^]` active indicator.
*   **Marker Protocol:** Use File Size, never `mtime`.

---
**Standard Maintainer:** TPM Architecture Team
**Reference Implementation:** `c+x_tpm_FUZPET+gl_12+PDLON/`
