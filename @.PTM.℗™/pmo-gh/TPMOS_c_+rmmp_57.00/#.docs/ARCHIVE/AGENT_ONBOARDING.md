================================================================================
AGENT_ONBOARDING.md - Complete System Overview
================================================================================
Date: 2026-03-09
Status: ONBOARDING BASELINE (Updated with Apr 1 Fondu clarifications)

## 2026-04-01 CLARIFICATIONS (CRITICAL - READ FIRST)

1. **EVERYTHING IS A PROJECT** - There is no "APP vs. PROJECT" distinction. "Apps" are just projects launched from different entry points (OS menu vs. loader menu).

2. **OPS ARE THE PRODUCT** - Projects expose reusable ops that other projects can call via PAL scripts. This is the core reusability model.

3. **FONDU = LIFECYCLE + OPS REGISTRY** - Fondu moves projects between states (active/archived/installed) and registers their ops for discovery.

4. **USER PROJECT IS OP-BASED** - `projects/user/` exposes ops like `create_profile`, `auth_user`, `get_session` that game projects script against.

5. **FONDU IS C FROM START** - Dogfooding the architecture: Fondu itself is a modular C utility with swappable ops.

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
- **Are discoverable via Fondu registry** (`pieces/os/ops_registry/`)

### THE OPS ECOSYSTEM (NEW - Apr 1)

**Projects expose ops → Other projects call them:**
```
projects/user/ops/+x/         projects/fuzz-op/
├── create_profile.+x  ──────>│ manager/
├── auth_user.+x              │   └── fuzz-op_manager.c
└── get_session.+x            │       # PAL script:
                              │       OP user::create_profile "player1"
pieces/os/ops_registry/       │       OP user::auth_user "player1"
└── user.txt                  │
    (registered ops)          │
```

**Fondu makes ops discoverable:**
```bash
$ fondu --list-ops
=== user ===
  create_profile    - Creates a new user profile
  auth_user         - Authenticates a user session
  get_session       - Gets active session data

=== fuzz-op ===
  move_entity       - Moves an entity on the map
  place_tile        - Places a tile on the map
```

### PROJECT LIFECYCLE (NEW - Apr 1)

**Three states, managed by Fondu:**
```
ACTIVE (projects/)     ←→ ARCHIVED (trunk/) ←→ INSTALLED (pieces/apps/installed/)
- Editable source         - Source only         - Compiled binaries
- In compile cycle        - Excluded from build - Read-only (chmod 555)
- Dev-facing              - Backup/archive      - User-facing
```

**Fondu commands:**
- `fondu --install <project>` - Compile + deploy + register ops
- `fondu --uninstall <app>` - Remove + unregister ops
- `fondu --archive <project>` - Move to trunk (source only)
- `fondu --restore <project>` - Restore from trunk + recompile
- `fondu --list` - Show all projects and states
- `fondu --list-ops` - Show all available ops

### DATA FLOW EXAMPLE (canonicalized, Apr 1):
Scenario: User moves selector in active app/project
1. keyboard_input.+x writes "1002" (UP) to pieces/keyboard/history.txt
2. chtpm_parser.c reads keyboard history
3. Layout (editor.chtpm) has `<interact src="pieces/apps/player_app/history.txt" />`
4. Parser "injects" key 1002 into pieces/apps/player_app/history.txt
5. editor_module.+x polls pieces/apps/player_app/history.txt, sees key 1002
6. Editor calls standardized move op: ./+x/move_entity.+x selector up
7. move_entity.+x updates canonical piece state under active project (world/map-first when applicable), hits frame marker.
8. Parser recomposes frame using updated map view.

**Alternative: PAL script calling another project's ops:**
```
# In fuzz-op PAL script:
OP user::create_profile "player1"    # Calls user project's op
OP user::auth_user "player1"         # Reuses auth logic
# ... game logic continues
```
