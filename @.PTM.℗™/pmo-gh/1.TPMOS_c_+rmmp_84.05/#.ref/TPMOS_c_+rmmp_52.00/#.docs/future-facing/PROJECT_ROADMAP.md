================================================================================
PROJECT_ROADMAP.md - Editor + Player Roadmap (Restored)
================================================================================
Date: 2026-03-09
Status: HISTORICAL BASELINE (Superseded by Mar 18 roadmap)

## 2026-03-18 UPDATE (CURRENT DIRECTION)

This file is preserved for historical context. Current execution order has changed.

Authoritative near-term sequence now:
1. Step 1: `op-ed` save-to-file + save/load roundtrip + local play boot + `pal`/op-event no-code create/bind/test.
2. Step 2: AI ops layer (FSM/RL/small LLM adapters with scalable profiles).
3. Step 3: Pet expansion (chat, self-management, self-supervised training, multi-pet management, generator).
4. Step 4: Voice accessibility (voice control + voice readback; same canonical action path as joystick/keyboard).
5. Step 5: Foundation simulation game (turn-based civ/market/war/space/chem-physics).
6. Step 6+: networking, gl-os surfaces, PMOS install/versioning/remote-use, then long-horizon kernel portability.

Architecture assumptions to follow:
- Canonical containment: `projects/<project>/pieces/world_<id>/map_<id>/<piece_id>/`
- No extra `/pieces/` under `map_<id>/`
- Pieces may contain pieces (inventory/interior nesting)
- World/map-aware resolution first, fallback paths second during migration

### SEPARATE APPS, SHARED OPS
Legacy snapshot (kept for context):
- Editor (pieces/apps/editor/)
- Player (pieces/apps/playrm/)
- Shared Ops (pieces/apps/playrm/ops/)

### IMPLEMENTATION PHASES
Legacy snapshot (not current execution order):
1. **PHASE 1: SHARED OPS LIBRARY** (Standardize Muscles) - ✅ COMPLETE
2. **PHASE 2: PLAYER MODULE** (Project selection → Title → Game) - ✅ COMPLETE
3. **PHASE 3: EDITOR MODULE** (4 tabs, can place tiles, edit pieces) - 🚧 IN PROGRESS (Refactoring for dynamic interaction)
4. **PHASE 4: DEMO PROJECT** (demo_1 project with full Title → Game → Win loop) - 🚧 IN PROGRESS
5. **PHASE 5: INTEGRATION TESTING** (Edit in Editor, test in Player, iterate) - ⏳ PENDING

### THE MANTRA
"Editor and Player are SEPARATE. They share the same Ops. Player loads projects. The Demo proves the pipeline. Keep it simple, stupid."
