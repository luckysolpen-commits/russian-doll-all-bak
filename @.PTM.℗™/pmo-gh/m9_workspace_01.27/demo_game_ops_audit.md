# TPM Demo Game: Ops & Pieces Audit Guide (2026-03-09)

This document explains the standardized Ops and Pieces currently implemented in the `demo_1` project and the `playrm_module` engine.

## 1. The "Muscles" (Standardized Ops)
All Ops live in `pieces/apps/playrm/ops/+x/` and follow the TPM binary standard.

### `move_player.+x <piece_id> <direction>`
*   **Logic:** Reads the target piece's `pos_x`, `pos_y`, and `pos_z`. Calculates the new position based on direction (up/down/left/right). Checks collision against the project's map file (`maps/map_0001_zN.txt`).
*   **Audit:** Logs `MoveEntity` or `MoveBlocked` to `master_ledger.txt`.
*   **Sovereignty:** Updates both the piece's `state.txt` and DNA.

### `move_z.+x <piece_id> <direction>`
*   **Logic:** Increments or decrements the `pos_z` value of the piece.
*   **Audit:** Logs `MoveZ` to `master_ledger.txt`.

### `interact.+x <piece_id>`
*   **Logic:** Gets the position of the interactor. Scans the `projects/{project}/pieces/` directory for any other entity at that exact coordinate.
*   **Prisc Bridge:** If a match is found, it reads the target's `on_interact` script path and executes it using the Prisc VM (`prisc+x`).
*   **Audit:** Logs `Interact` to `master_ledger.txt`.

### `render_map.+x`
*   **Logic:** The "Stage Producer." Loads the project terrain and scans all pieces. Filters entities by the current engine Z-level. 
*   **Hierarchy:** Renders icons in priority order: Hero (`@`) > NPCs (`&`) > Selector (`X`) > Terrain (`#`, `.`, etc.).
*   **Output:** Writes to `pieces/apps/player_app/view.txt`.

### `project_loader.+x`
*   **Logic:** Scans the `projects/` directory. Generates a temporary PDL menu.
*   **State:** Writes the selected `project_id` to `pieces/apps/player_app/manager/state.txt`.

---

## 2. The "Brains" (Bridge Module)
### `playrm_module.c`
*   **Responsibility:** The persistent background process launched by the CHTPM Parser.
*   **Loop:** Polls `pieces/apps/fuzzpet_app/fuzzpet/history.txt` for raw key codes.
*   **Routing:** 
    *   `WASD` -> `move_player`
    *   `XZ` -> `move_z`
    *   `ENTER` -> `interact`
    *   `9` -> Toggles `active_target_id` between "hero" and "selector" (for testing).
*   **Debug:** Updates `last_key` in engine state for UI visibility.

---

## 3. The "World" (Project demo_1)
### Pieces
*   **hero:** `type=player`. The main avatar.
*   **selector:** `type=selector`. A secondary cursor piece.
*   **npc_01:** `type=npc`. Has `on_interact=scripts/npc_greeting.asm`.
*   **chest_01:** `type=chest`. Has `on_interact=scripts/chest_open.asm`.

### Scripts (PAL/ASM)
*   **npc_greeting.asm:** Uses `console_print` to greet the player.
*   **chest_open.asm:** Triggers a "Sanity check PASSED" message.

---

## 4. The "Theater" (CHTPM Layout)
### `game.chtpm`
*   **Variables:** Displays `${project_id}`, `${active_target_id}`, `${last_key}`, and `${game_map}`.
*   **Handover:** The "Control Map" button uses `onClick="INTERACT"` to lock the UI and pipe keys directly to the module.
