================================================================================
PRISC_EXECUTION_MODEL.md - Mode 1 vs Mode 2 (Restored)
================================================================================
Date: 2026-03-09
Status: AUTHORITATIVE EXECUTION SPECIFICATION

### MODE 1: REALTIME INPUT (Direct C)
keyboard → parser → module → Op (+x binary) → state.txt
**For:** Movement (WASD), Z-level (XZ), Quick Interactions (ENTER on chest).
**Latency:** ~16ms (one frame). No Prisc involved.

### MODE 2: EVENT SCRIPTS (Prisc Scripting)
trigger → module → prisc+x → event.asm → Ops (+x) → state.txt
**For:** NPC dialogue trees, cutscenes, quest logic, triggers.
**Latency:** ~50-100ms (script execution). Prisc IS involved.

### OPS: THE BRIDGE
Ops are +x binaries that work in BOTH contexts. The Op doesn't know or care how it was called. It just reads arguments, updates state, and hits the frame marker.

### PRISC INTEGRATION POINTS
Prisc is called from playrm_module.c in these situations:
1. Entity Interaction (on_interact)
2. Zone Triggers (on_enter)
3. Background AI (on_tick)
4. Cutscenes (on_event)
