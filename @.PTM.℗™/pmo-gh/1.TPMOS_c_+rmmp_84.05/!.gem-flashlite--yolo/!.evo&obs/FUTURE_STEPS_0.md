# TPMOS Future Development Roadmap (FUTURE_STEPS_0)
**Date:** 2026-04-22
**Status:** STRATEGIC VISION & IMPLEMENTATION STEPS
**Version:** 2.0 (Consolidated Edition)

---

## 1. P2P Networking: Ring-Broadcast Hybrid Chat
### Current State Assessment
- `chat-op` provides the functional UI testbed (input field + scrollable message log).
- `p2p-net` has basic broadcast and subnet config logic.
- Peer discovery is established via UDP broadcast in the 8000-8010 port range.

### Technical Details of State
- **Identity**: Unique `node_hash` based on PID/Time.
- **Registry**: File-backed nodes in `pieces/network/registry/`.
- **UI Driver**: `projects/chat-op/layouts/chat-op.chtpm` using `<cli_io>` and `${messages}`.

### Desired Improvements
- **Hybrid Topology**: Combine broadcast discovery with a deterministic **Ring Path** for message propagation.
- **Single Room POC**: Unified global chatroom where every terminal acts as a relay node.

### Steps & KPIs
1.  **Ring-Forwarding Op**: Implement `ring_relay.+x` to identify logical neighbors and pass messages.
2.  **Message Dedup**: Implement hash-checking to prevent infinite loops in the ring.
3.  **Chat-Op Integration**: Bridge the manager to the P2P relay logic.
- **KPI**: A message sent from one terminal successfully traverses a multi-node ring and appears on all instances.

---

## 2. XO (Exo-Operating) Bot: Cognitive Evolution
### Current State Assessment
- `bot5` is a pure orchestrator living in `xo/bot5/`.
- Infrastructure for recording frame/input sequences (Memories) is functional and Exo-Sovereign.

### Technical Details of State
- **Brain**: Code in `xo/bot5/cognition/`.
- **Memory**: Sovereign data piece in `xo/bot5/pieces/memories/`.
- **Format**: `F:XXXX | [TIMESTAMP] KEY_PRESSED: <code>`.

### Desired Improvements
- **Supervised Imitation**: Agent-labeled memories (`solve_quiz`, `find_zombie`) used as training sets.
- **RL Weight Training**: implementing probability matrices in `weights/` to map Frame-States to Input-Actions.
- **Cognitive Layers**: 
    - **Attention**: Text-parsing for CYOA/Chat sentiment.
    - **Knowledge Distillation**: Extracting heuristics from human runs into bot PDL.

### Steps & KPIs
1.  **Replay Engine**: Implement `replay_memory.+x` to verify capture fidelity.
2.  **Feature Extractor**: Develop `parse_frame_features.+x` to identify UI context.
3.  **Weight Trainer**: Implement `train_rl.+x` to adjust probability weights based on success pulses.
- **KPI**: Bot completes a 10-step navigation task autonomously with >90% accuracy using RL weights.

---

## 3. GL-OS: High-Fidelity File-State Mirroring
### Current State Assessment
- `gl_desktop.c` provides a windowed OpenGL environment but is currently a "projector" (generating its own views) rather than a "TV".

### Technical Infrastructure Clarification
- **The "TV" Principle**: GL-OS terminals must mirror **file states** (`current_frame.txt`), not the ASCII OS memory.
- **Process Isolation**: GL-OS and ASCII-OS are separate process trees.
- **Universal Infrastructure**: GL apps use the same Piece/PDL/State-File DNA as ASCII apps.

### Desired Improvements
- **Mirror Terminal**: Select and launch GL-native projects from the ASCII Project Loader.
- **Virtual Windows**: Windows that render the output of other projects (e.g., Fuzz-Op, Op-Ed) by reading their frame files.
- **Tile-Based Rendering (8x8x8 RGB)**: Transition from ASCII symbols to custom tiles defined in `.txt` files containing RGB256 codes. Each tile represents an 8x8x8 3D cube.
    - **2D Mode**: These cubes are viewed top-down as flat tiles.
    - **3D Mode**: These cubes are rendered in 3D space with height determined by Z-level data.
    - **Switchable Camera (3D Only)**: Support for **Free Camera**, **1st Person**, and **3rd Person** perspective modes.

### Steps & KPIs
1.  **Virtual Mirror Window**: Implement `GLWindow` type that polls `pieces/display/current_frame.txt`.
2.  **Input Bridge**: Delegate keyboard/joystick input from the focused GL window to the ASCII project's history file.
3.  **Tile Format Spec**: Define the `.txt` RGB format for 8x8x8 cubes.
4.  **Camera Controller**: Implement the 3-mode camera system (Free/1st/3rd person).
- **KPI**: Opening "Fuzz-Op-GL" or "Op-Ed-GL" in the desktop shows a live mirror using custom RGB tiles; user can toggle between 2D and 3D views with a dedicated camera controller.

---

## 4. Op-Ed: Advanced Mapping & 3D ASCII POC
### Current State Assessment
- `op-ed` is functional for 2D mapping but lacks true Z-level depth and event binding.

### Desired Improvements
- **RPG Maker MV Parity**: Full event binding via PDL and true multi-floor Z-level switching.
- **3D ASCII View (Explorative POC)**: A "3D Mode" using skewed ASCII projection (isometric) to visualize height differences in the terminal.
- **GL Tile-Version**: A 3D version of Op-Ed in GL-OS with tiles extruded to heights based on Z-level data.

### Steps & KPIs
1.  **Z-Filtering**: Refactor `render_map.c` to only draw pieces/tiles on the `current_z` floor.
2.  **3D ASCII Projection Op**: Create `project_isometric.+x` to generate a 3D-feeling view in ASCII.
3.  **GL Tile Extrusion**: Map `pos_z` to physical Y-axis height in the GL renderer.
- **KPI**: User toggles "3D View" and sees a clear visual distinction between Floor 0 and Floor 1.

---

## 5. Complexity & Implementation Report

### Relative Difficulty Assessment
| Project | Difficulty | Primary Challenge |
| :--- | :--- | :--- |
| **XO Bot (Imitation)** | **Easiest** | Orchestrating file-reads into folders. Uses existing FSM patterns. |
| **Op-Ed (3D ASCII)** | **Moderate** | Math for isometric projection and Z-level filtering logic. |
| **Hybrid Chat** | **Moderate/Hard** | Networking edge cases and process synchronization in the ring. |
| **GL-OS (3D Theater)** | **Hardest** | Managing GL contexts and syncing input focus between windows/files. |

### Recommended Implementation Order
1.  **XO Bot (Imitation)**: (PHASE 1 COMPLETE) Immediate value for testing.
2.  **Op-Ed (Z-Level & 3D ASCII)**: High visual impact for minimal code deviation.
3.  **Hybrid Chat**: builds on existing `chat-op` UI to prove P2P stability.
4.  **GL-OS (Virtual Window)**: Long-term goal requiring system-wide file-state stability.

---
*"Build the easiest muscle first to lift the heaviest weight later. The file is the truth."*
