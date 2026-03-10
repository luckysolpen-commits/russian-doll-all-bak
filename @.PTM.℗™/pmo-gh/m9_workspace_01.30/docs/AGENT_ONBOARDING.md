================================================================================
AGENT_ONBOARDING.md - Complete System Overview (Restored)
================================================================================
Date: 2026-03-09
Status: AUTHORITATIVE ONBOARDING DOCUMENT

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

**DATA FLOW EXAMPLE:**
Scenario: User presses 'w' to move in Editor
1. keyboard_input.+x writes "119" to pieces/keyboard/history.txt
2. chtpm_parser.c reads history, sees key 119 ('w')
3. Parser is in Editor layout → injects to history.txt
4. editor_module.+x polls history, sees key 119
5. Editor calls: ./+x/place_tile.+x map_001 5 4 '#'
6. place_tile.+x reads map file, replaces char, writes back, hits frame marker.
