================================================================================
PROJECT_ROADMAP.md - Editor + Player Roadmap (Restored)
================================================================================
Date: 2026-03-09
Status: AUTHORITATIVE IMPLEMENTATION PLAN

### SEPARATE APPS, SHARED OPS
- Editor (pieces/apps/editor/)
- Player (pieces/apps/playrm/)
- Shared Ops (pieces/apps/playrm/ops/)

### IMPLEMENTATION PHASES
1. **PHASE 1: SHARED OPS LIBRARY** (Standardize Muscles)
2. **PHASE 2: PLAYER MODULE** (Project selection → Title → Game)
3. **PHASE 3: EDITOR MODULE** (4 tabs, can place tiles, edit pieces)
4. **PHASE 4: DEMO PROJECT** (demo_1 project with full Title → Game → Win loop)
5. **PHASE 5: INTEGRATION TESTING** (Edit in Editor, test in Player, iterate)

### THE MANTRA
"Editor and Player are SEPARATE. They share the same Ops. Player loads projects. The Demo proves the pipeline. Keep it simple, stupid."
