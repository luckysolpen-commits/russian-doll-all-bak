================================================================================
AGENT_ONBOARDING.md - Complete System Overview (Restored)
================================================================================
Date: 2026-03-09
Status: ONBOARDING BASELINE (Updated with Mar 18 corrections)

## 2026-03-18 CORRECTIONS (READ FIRST)

1. Canonical directory model is now world/map-first:
   - `projects/<project>/pieces/world_<id>/map_<id>/<piece_id>/`
   - no required `/pieces/` layer under map directories
2. Container model is recursive:
   - maps contain pieces
   - pieces may contain pieces (inventory/interior)
3. Current top priority is Step 1 implementation:
   - `op-ed` save-to-file
   - save/load roundtrip
   - local play bootstrap from saved artifacts
   - `pal` + op-event no-code create/bind/test flow
4. Keep fallback/root path behavior only as migration compatibility; do not treat fallback as canonical.
5. Voice accessibility, networking, gl-os, install/versioning, and kernel portability are later steps (not Step 1 scope).

### WHAT IS TPM? (True Piece Method)
TPM is a software architecture where EVERYTHING is a "Piece" - a directory containing text files that define state and behavior.

**Key Principles:**
1. Data Sovereignty - Each piece owns its state files exclusively
2. Audit Obsession - Every operation logs to master_ledger.txt
3. File-Based IPC - All communication happens through files, not memory
4. No In-Memory Shortcuts - All state persists to disk immediately

### WHAT IS PAL? (Prisc Assembly Language)
PAL is a RISC-like assembly language for scripting game events WITHOUT modifying C code.

### OPS (Reusable +x Binaries)
Ops are the bridge between TPM and PAL. They're +x binaries that:
- Work in both contexts (realtime C calls AND PAL scripts)
- Update state files
- Hit frame markers for rendering

### PIECE INTERACTION & PAL SCRIPT EDITOR (CRITICAL FEATURE)
When user presses ENTER on a placed piece in Editor (or Player), a context menu appears with options to manipulate that piece.

**PIECE CONTEXT MENU:**
- Copy / Cut / Edit / Delete
- **Edit** opens the PAL Script Editor (Visual Stack-Based Editor)

**PAL SCRIPT EDITOR:**
Allows users to stack Ops visually, configure variables, and save to piece's .asm file.
Navigation uses standard TPM nav index pattern [>].

**DATA FLOW EXAMPLE (canonicalized):**
Scenario: User moves selector in active app/project
1. keyboard_input.+x writes "1002" (UP) to pieces/keyboard/history.txt
2. chtpm_parser.c reads keyboard history
3. Layout (editor.chtpm) has `<interact src="pieces/apps/player_app/history.txt" />`
4. Parser "injects" key 1002 into pieces/apps/player_app/history.txt
5. editor_module.+x polls pieces/apps/player_app/history.txt, sees key 1002
6. Editor calls standardized move op: ./+x/move_entity.+x selector up
7. move_entity.+x updates canonical piece state under active project (world/map-first when applicable), hits frame marker.
8. Parser recomposes frame using updated map view.
