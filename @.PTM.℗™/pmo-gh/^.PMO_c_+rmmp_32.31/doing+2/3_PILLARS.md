# THE THREE PILLARS OF FUZZPET COEXISTENCE

To ensure architectural integrity and evolutionary stability, the FuzzPet project follows three mandatory pillars.

## PILLAR 1: RADICAL ENCAPSULATION (ISOLATION)
Legacy and V2 FuzzPet MUST remain strictly separated at the filesystem level.
- **Legacy FuzzPet**: Reserved as a "Pure Reference". It resides in `pieces/apps/fuzzpet_app/` and uses global system paths. It MUST NOT be modified to support V2 features.
- **V2 FuzzPet (PAL)**: Resides strictly within `projects/fuzzpet_v2/`. It owns its own `history.txt`, `manager/state.txt`, and project-specific `pieces/`.
- **The Parser Mandate**: The `chtpm_parser` must dynamically resolve paths based on the active `project_id` to ensure input injection and variable loading never cross-contaminate.

## PILLAR 2: ARCHITECTURAL PARITY (PAL + OPS)
All legacy functionality must be ported to the **Muscle/Brain (Op/PAL)** architecture.
- **The Logic (Brain)**: Moved from hardcoded C binaries into PAL Assembly (`game_loop.asm`). This allows for runtime logic updates without recompilation.
- **The Muscles (Ops)**: Individual actions (Feed, Move, Scan) are implemented as atomic, reusable binaries (`move_entity.+x`, `fuzzpet_action.+x`).
- **The Orchestrator**: The Project-Local Module (`fuzzpet_v2_module.c`) acts as a stateless pulse, relaying keys from the local history to the PAL Brain.

## PILLAR 3: PERSISTENT EVOLUTION
The goal is 100% parity with legacy features within the V2 architecture, enabling new possibilities.
- **Legacy as Spec**: Every legacy feature (Hunger decay, Emoji toggle, Selector logic) serves as the "Requirement Document" for V2 implementation.
- **V2 as Future**: By moving to PAL+Ops, we enable multi-entity control, project-specific maps, and complex scripting that legacy C-logic cannot easily handle.
- **Audit Obsession**: Both versions MUST continue to write to the shared `pieces/debug/frames/session_frame_history.txt` to ensure full system audibility during this transition phase.

---
"Respect the Past, Build the Future, Isolate the Present."
