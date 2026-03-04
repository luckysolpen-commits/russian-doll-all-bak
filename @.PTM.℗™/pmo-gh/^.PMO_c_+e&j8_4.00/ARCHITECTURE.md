# CHTPM+OS Architecture (PMO Standard v3.0)

**Date:** 2026-03-04  
**Status:** AUTHORITATIVE - Standard for Modular Piece-Module-OS Systems  
**Supersedes:** All prior CHTPM/CHTRM documentation.

---

## 1.0 Overview

CHTPM v3.0 is a **Modular TPM implementation** of the Piece-Module-OS (PMO) framework. It introduces high-performance state mirrors and contextual UI re-indexing to support multiple sovereign entities within a single unified view.

---

## 2.0 The Hierarchy of Sovereignty (P-M-O)

1.  **PIECE (Atomic):** Holds the "DNA" (.pdl) for auditability and the "Mirror" (state.txt) for performance.
2.  **MODULE (Brain):** The logic agent (e.g., `fuzzpet_manager`). It orchestrates movement, capabilities, and mirror synchronization.
3.  **OS / LAYOUT (Theater):** The `chtpm_parser` which provides the shell, navigation, and contextual menu re-indexing.

---

## 3.0 The Filesystem API (Communication)

### 3.1 Input Routing (Relay Mode)
*   **Parser** reads global input and translates Arrow Keys/Joystick codes into canonical characters.
*   In `INTERACT` mode, the Parser relays keys to the **Active Module's** local history.

### 3.2 The Mirror Sync Mandate
To maintain 60 FPS while preserving data sovereignty:
*   **DNA (.pdl)**: The source of truth. Highly structured, updated by `piece_manager`.
*   **Mirror (state.txt)**: A flat `key=value` file optimized for fast reads.
*   **Mandate**: Every write to the DNA MUST trigger an immediate flush to the Mirror. System components (Renderer/Manager) read from the Mirror to eliminate the overhead of sub-process execution.

---

## 4.0 Component Responsibilities

### 4.1 chtpm_parser.c (The Theater)
1.  **Contextual Re-Indexing**: Forces a full layout re-tokenization whenever `active_target_id` changes. This allows the UI to dynamically adapt buttons based on the selected piece's PDL.
2.  **Hybrid Expansion**: Expands UI variables (like `${piece_methods}`) *before* tokenization, and data variables (like `${game_map}`) *at render time* to prevent buffer truncation.
3.  **Performance Optimization**: Uses pre-allocated scratch buffers to eliminate `malloc` overhead in the render loop.

### 4.2 Module (The Stage Manager)
1.  **Registry Awareness**: Iterates the `world/registry.txt` to resolve entity positions and selection logic.
2.  **Universal Movement Engine**: Uses a shared engine (`move_entity.c`) to provide deterministic movement logic for all entities (Selector, Pets, etc.).
3.  **Shadow Selector Sync**: Automatically syncs the "invisible" Selector cursor to the Pet's position during pet control, ensuring a seamless hand-off on ESC.

---

## 5.0 High-Speed Render Pipeline

To achieve snappy performance in complex world maps:
1.  **Direct Read**: The `map_render` and `fuzzpet_manager` bypass the `piece_manager` for all read operations, accessing `state.txt` mirrors directly.
2.  **Pulse Protocol**: The `frame_changed.txt` marker file size is used to trigger instant OS recomposition.
3.  **Heap Management**: Massive 1MB buffers ensure that large game maps can be expanded without corrupting the XML tag structure of the layout.

---

## 6.0 Joystick & Universal Translation

The system implements a universal input layer:
*   **Keyboard**: Standard ASCII + Arrow codes (1000-1003).
*   **Joystick**: Normalized codes (2000-2103).
*   **Translation**: The Parser maps all directional input to a shared 'w/a/s/d' interface for internal traits, ensuring cross-device compatibility without redundant code.

---

## 7.0 Emoji Fidelity Mode

The architecture supports a global `emoji_mode` toggle:
*   **Stateful**: Mirrored in the `fuzzpet/state.txt`.
*   **Dynamic**: The `map_render` performs high-fidelity UTF-8 overlay (🐶, 📦, 🎯) when toggled, transforming the ASCII world into a rich visual experience without logic changes.

---
**Standard Maintainer:** TPM Architecture Team
**Reference Implementation:** `code-explore-pmo-st8/^.PMO_c_+e&j8_2.01`
