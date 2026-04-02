# PIECEMARK-IT / CHTPM+OS Monolith
**Version:** 6.0 (Standardized Ops Edition)
**Date:** March 25, 2026
**Philosophy:** PIECE -> MODULE -> OS (PMO)

This is the central documentation for the Piecemark-IT ecosystem, a file-backed, auditable, and modular OS environment built on the **True Piece Method (TPM)**.

---

## 📑 Table of Contents
1. [Core Philosophy (PMO/TPM)](#1-core-philosophy-pmotpm)
2. [PJARGO Dictionary (Internal Dialect)](#2-pjargo-dictionary-internal-dialect)
3. [System Architecture](#3-system-architecture)
4. [App vs. Project (Critical Distinction)](#4-app-vs-project-critical-distinction)
5. [The Runtime Pipeline (12 Steps)](#5-the-runtime-pipeline-12-steps)
6. [Development Standards](#6-development-standards)
7. [How to Add to the Codebase](#7-how-to-add-to-the-codebase)
8. [Future Roadmap](#8-future-roadmap)
9. [Tools & Maintenance](#9-tools--maintenance)

---

## 🧬 1. Core Philosophy (PMO/TPM)
The system follows a strict thought priority: **PIECE -> MODULE -> OS**.

*   **PIECE (The Atomic Unit):** A directory containing text files defining state (`state.txt`) and DNA (`piece.pdl`). A piece owns its state exclusively. "If it's not in a file, it's a lie."
*   **MODULE (The Brain):** Orchestrates pieces, interprets input, and executes **Ops**. It should delegate "Muscle" logic to standalone binaries.
*   **OS / CHTPM (The Theater):** The environment and UI. It substitutes variables (`${var}`), routes input, and composes the final frame. "The Parser is the mirror's eye, not the mirror's mind."

### TPM Principles:
*   **Data Sovereignty:** Each piece owns its files.
*   **Auditability:** All major events log to `master_ledger.txt`.
*   **File-Based IPC:** Communication happens via files, not memory.
*   **Recursive Containment:** Maps contain pieces; pieces can contain pieces (inventory/interiors).

---

## 🗣️ 2. PJARGO Dictionary (Internal Dialect)
*High-leverage vocabulary for developers and agents.*

*   **W-first:** World-first path resolution (`world_<id>/map_<id>/...`).
*   **RT Parity:** Roundtrip parity (Save -> Load yields identical state).
*   **Slice-Safe:** Isolate changes so other subsystems keep running.
*   **Ghost Hardcode:** Hidden project-specific assumptions in generic code (AVOID).
*   **Mirror:** The `state.txt` runtime representation.
*   **DNA:** The `piece.pdl` definition layer.
*   **Sea Shell:** The core (Parser, Nav) is strong; growth (Apps) is at the edges.

---

## 🏗️ 3. System Architecture
The filesystem is the database.

```
/
├── pieces/
│   ├── apps/          # System tools (op-ed, gl-os, user)
│   ├── chtpm/         # Parser, orchestrator, layouts
│   ├── display/       # current_frame.txt, current_layout.txt
│   ├── system/        # pdl_reader, kvp_db, prisc (VM)
│   └── world/         # Global world instances
├── projects/          # User-created content (fuzz-op, lsr, demo_1)
├── #.docs/            # Canonical documentation
├── #.plans/           # Future roadmap
└── #.tools/           # Maintenance scripts
```

### World/Map/Piece Nesting Model:
`projects/<project>/pieces/world_<id>/map_<id>/<piece_id>/`

---

## 🚀 4. App vs. Project (Critical Distinction)
| Feature | APP (System Tool) | PROJECT (User Content) |
| :--- | :--- | :--- |
| **Location** | `pieces/apps/<app_id>/` | `projects/<project_id>/` |
| **Launch** | via `<button href="...">` | via Loader Menu selection |
| **Descriptor** | `<app_id>.pdl` (layout_path) | `project.pdl` (entry_layout) |
| **Example** | `op-ed`, `gl_os`, `user` | `fuzz-op`, `lsr`, `demo_1` |

---

## ⚙️ 5. The Runtime Pipeline (12 Steps)
1.  **INPUT:** `keyboard_input.+x` writes to `keyboard/history.txt`.
2.  **ROUTING:** `chtpm_parser.c` reads the key.
3.  **RELAY:** Parser injects key into active app history (e.g., `player_app/history.txt`).
4.  **TICK:** App Module polls history buffer.
5.  **TRAIT/OP:** Module calls a "Muscle" Op (e.g., `move_entity.+x`).
6.  **SOVEREIGNTY:** Op updates Piece DNA (`.pdl`) or Mirror (`state.txt`).
7.  **MIRROR SYNC:** Update flushed to `state.txt` for fast reading.
8.  **STAGE:** `render_map.+x` reads mirrors and writes `view.txt`.
9.  **SYNC:** `render_map.+x` appends to `frame_changed.txt` (pulse).
10. **COMPOSITION:** Parser substitutes variables and loads system state.
11. **RENDER:** Parser writes composite frame to `current_frame.txt`.
12. **DISPLAY:** Renderer (ASCII or GL) prints to user.

---

## 🛠️ 6. Development Standards
### CPU Safety (Mandatory)
Modules must follow the `fuzz_legacy_manager.c` pattern:
*   **Signal Handling:** Explicit `SIGINT` handlers to prevent orphaned children.
*   **Process Groups:** `setpgid(0, 0)` and group-killing on exit.
*   **Fork/Exec:** Prefer `fork()/exec()/waitpid()` over `system()`.
*   **Throttling:** Use `is_active_layout()` to sleep 100ms when not in focus.
*   **Stat-First:** Only open files if `stat()` confirms they changed.

### UI/Layout Patterns
*   **Buttons:** NEVER manually write `[>]`. Use `<button label="X" onClick="KEY:n" />`.
*   **CLI Input:** Use `<cli_io id="x" label="${var}" />` for text input. The parser handles buffering and masking.

---

## ➕ 7. How to Add to the Codebase
### Creating a New App
1.  Setup: `mkdir -p pieces/apps/<name>/{layouts,plugins/+x,pieces}`.
2.  DNA: Create `<name>.pdl` with `app_id` and `layout_path`.
3.  Logic: Implement `_module.c` using the **CPU-Safe Template**.
4.  Wiring: Add a `<button href="...">` to `os.chtpm` or the loader.

### Creating a New Project
1.  Setup: `mkdir -p projects/<name>/{layouts,manager/+x,maps,pieces}`.
2.  DNA: Create `project.pdl` with `project_id` and `entry_layout`.
3.  Logic: Implement `_manager.c` to handle game state.

---

## 📅 Near-Term Strategy (Mar 2026)
**"Stabilize the Core, Ignite the AI."**

The immediate priority is to finish the "Hard Stuff" in ASCII-OS before layering on GL-OS complications. Focus on the core loop: **Save/Load -> PAL Scripting -> AI Recursion.**

### 1. ASCII-OS Stabilization (The "Hard Stuff")
*   **op-ed Save/Load:** Finalize deterministic roundtrip parity for project files.
*   **PAL Mastery:** Complete the scripting engine to allow deep, no-code interaction between pieces.

### 2. AI-Recurse & Auto-Battle (AI-Studio)
*   **Playable Bots:** Implement "Chimochio/AOW" bots that use PAL scripting to auto-battle and navigate.
*   **AI-Recurse Studio:** A real-time environment where players use scripts to program bot behaviors (mining, crafting, working jobs).
*   **Master Scripts:** Seasonal and context-aware script switching (e.g., changing behavior based on in-game "seasons" or logic gates).

### 3. The P2P Market & Trunking
*   **Market Launch:** P2P auctions and trading for items/skills/formulas, populated by active bots.
*   **Entity Trunking:** Efficiently manage high-turnover NPCs (soldiers, "trees") that die frequently, reusing levels and states.

### 4. Advanced Synthesis (Chemistry & Contracts)
*   **Chemistry/Evolution:** Blending elemental chemistry for crafting and piece evolution.
*   **Scripted Contracts:** "Cops/Courts/Gov" pieces that enforce or vote on scripts (legal/social logic).
*   **AI Distillation:** Use local LLMs (Gemma/Qwen) via `ai-labs` to research compounds, laws, and physics, automatically generating `.pdl` and `.asm` artifacts.

---

## 🔭 8. Future Roadmap
*   **AI-LABS:** Knowledge distillation, local LLM training, and chat instance management.
*   **LSR (Lunar Streetrace Raider):** A "Civ-Lite" simulation with AI-driven R&D and a lunar economy.
*   **P2P-NET:** TPM-compliant blockchain, NFT items, and decentralized trading.
*   **GL-OS (The Visual Shell):** High-fidelity OpenGL rendering shell mirroring text frames.
    *   **Perspective Views:** Real-time 1st person, 3rd person, and free-cam 3D renders of underlying 2D ASCII-OS projects.
    *   **Near-Term Targets:** Rapid implementation of 2D/3D visualizations for `op-ed` and `fuzz-ops`.
    *   **Concept:** GL-OS acts as a visual "theater" that bridges ASCII logic into a modern 3D viewport.
*   **Kernel:** Long-horizon path to RISC-V/x86 kernel using Piece architecture.

---

## 🔧 9. Tools & Maintenance
*   `./compile_all.sh`: Rebuilds all system and project modules.
*   `./run_chtpm.sh`: Starts the environment pipeline.
*   `./kill_all.sh`: Forcefully terminates all PMO processes.
*   `#.tools/pmo_cpu_health.sh`: Monitors for runaway processes.
*   `#.tools/fix_old_pieces.sh`: Migrates legacy pieces to `map_id` standards.

---
"Softness wins. The empty center of the flexbox holds ten thousand things."
