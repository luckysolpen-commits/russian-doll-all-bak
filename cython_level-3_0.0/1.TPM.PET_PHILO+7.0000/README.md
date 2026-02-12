TPM FOUNDATION - KEYBOARD INPUT DEMONSTRATION
============================================

This foundation demonstrates the core TPM architecture with immediate keyboard input and master ledger integration.

ARCHITECTURE
-----------
- Plugin-based system following TPM principles
- All input goes through pieces/keyboard/history.txt
- Master ledger maintains complete audit trail
- Immediate response to WASD keys for movement
- Piece-driven state management

FILES CREATED
-------------
- engine.py - Main engine coordinating plugins
- plugin_interface.py - Base interface for all plugins
- pieces/keyboard/ - Keyboard piece with history tracking
- pieces/master_ledger/ - Central audit trail system
- pieces/mover/ - Movement handling piece
- plugins/core/ - Core functionality plugins

USAGE
-----
Run: python engine.py

Features:
- Press WASD keys for immediate movement feedback
- See position updates printed to console
- Watch pieces/master_ledger/master_ledger.txt for audit trail
- All inputs logged to pieces/keyboard/history.txt
- Piece states maintained in their respective directories

NEXT STEPS FOR IMPLEMENTATION
----------------------------
1. Expand the command processor piece to handle number commands (1-6)
2. Integrate the existing fuzzpet functionality
3. Add more complex movement logic
4. Implement additional event processing
5. Connect to the existing pet plugin methods