================================================================================
COMPILE SYSTEM DOCUMENTATION
================================================================================
Date: April 2, 2026
Version: 2.0 (Automated Compilation)

================================================================================
PROBLEM SOLVED
================================================================================

Previously, binaries had to be compiled manually, leading to:
- Missing manager binaries (fuzz-op_manager, user_manager, etc.)
- Missing op binaries (move_entity, scan_op, etc.)
- Broken functionality (selector not moving, ops not working)
- Confusion about what needs to be compiled

SOLUTION: Automated compilation + verification

================================================================================
NEW TOOLS
================================================================================

1. #.tools/compile_all.sh
   ───────────────────────
   Compiles ALL binaries automatically:
   - System components (parser, orchestrator, renderer, keyboard)
   - System apps (op-ed, man-*, player_app)
   - Shared ops (render_map, move_entity, scan_op, etc.)
   - ALL project managers
   - ALL project ops
   - Legacy apps in trunk (for reference)

   Usage:
   ./#.tools/compile_all.sh

2. #.tools/check_binaries.sh
   ──────────────────────────
   Verifies all required binaries exist and are executable.
   Reports missing binaries with clear instructions.

   Usage:
   ./#.tools/check_binaries.sh

3. button.sh (updated)
   ───────────────────
   New commands:
   - ./button.sh compile  - Compile everything + verify
   - ./button.sh check    - Verify binaries exist
   - ./button.sh run      - Start CHTPM

================================================================================
RECOMMENDED WORKFLOW
================================================================================

BEFORE RUNNING CHTPM:
─────────────────────
1. Compile all binaries:
   ./button.sh compile

2. Verify binaries exist:
   ./button.sh check

3. Run CHTPM:
   ./button.sh run

AFTER ADDING NEW CODE:
──────────────────────
1. Add new .c file (manager or op)
2. Run: ./button.sh compile
3. Verify: ./button.sh check
4. Test in CHTPM

WHEN SOMETHING DOESN'T WORK:
────────────────────────────
1. Check if binary exists:
   ./button.sh check

2. If missing, compile:
   ./button.sh compile

3. Restart CHTPM:
   ./button.sh k
   ./button.sh r

================================================================================
HOW IT WORKS
================================================================================

compile_all.sh scans:
────────────────────
1. pieces/keyboard/src/*.c → pieces/keyboard/plugins/+x/
2. pieces/chtpm/plugins/*.c → pieces/chtpm/plugins/+x/
3. pieces/display/*.c → pieces/display/plugins/+x/
4. pieces/apps/*/plugins/*.c → pieces/apps/*/plugins/+x/
5. pieces/apps/playrm/ops/src/*.c → pieces/apps/playrm/ops/+x/
6. projects/*/manager/*_manager.c → projects/*/manager/+x/
7. projects/*/ops/*.c → projects/*/ops/+x/
8. projects/trunk/legacy_archive/* → Compile for reference

check_binaries.sh verifies:
──────────────────────────
1. All system binaries exist
2. All system app binaries exist
3. All shared ops exist
4. All project managers exist (if source exists)
5. All project ops exist (if source exists)

================================================================================
EXAMPLE OUTPUT
================================================================================

$ ./button.sh compile

=== Compiling ALL binaries ===
=== CHTPM COMPILE ALL ===
=== 1. SYSTEM COMPONENTS ===
  ✓ pieces/keyboard/plugins/+x/keyboard_input.+x
  ✓ pieces/chtpm/plugins/+x/chtpm_parser.+x
  ✓ pieces/chtpm/plugins/+x/orchestrator.+x
  ✓ pieces/display/plugins/+x/renderer.+x

=== 2. SYSTEM APPS ===
  ✓ pieces/apps/op-ed/plugins/+x/op-ed_module.+x
  ✓ pieces/apps/man-ops/plugins/+x/man-ops_module.+x
  ...

=== 3. PLAYRM OPS (SHARED) ===
  ✓ pieces/apps/playrm/ops/+x/render_map.+x
  ✓ pieces/apps/playrm/ops/+x/move_entity.+x
  ...

=== 4. PROJECT MANAGERS ===
  ✓ projects/fuzz-op/manager/+x/fuzz-op_manager.+x
  ✓ projects/test_fondu/manager/+x/test_fondu_manager.+x
  ✓ projects/user/manager/+x/user_manager.+x

=== 5. PROJECT OPS ===
  ✓ projects/test_fondu/ops/+x/increment_counter.+x
  ✓ projects/test_fondu/ops/+x/decrement_counter.+x
  ✓ projects/user/ops/+x/create_profile.+x
  ...

=== COMPILE SUMMARY ===
  Compiled: 25 binaries
  Failed:   0 binaries

✓ All binaries compiled successfully!

=== Verifying binaries ===
=== BINARY VERIFICATION CHECK ===
=== SYSTEM BINARIES ===
  ✓ keyboard_input
  ✓ chtpm_parser
  ...

=== SUMMARY ===
  Present:  25 binaries
  Missing:  0 binaries

✓ All required binaries present!

================================================================================
TROUBLESHOOTING
================================================================================

Q: "Binary not found" error when running CHTPM
A: Run: ./button.sh compile

Q: Selector not moving / ops not working
A: Run: ./button.sh check
   If missing binaries: ./button.sh compile

Q: New op/manager not being compiled
A: Ensure source file is in correct location:
   - Manager: projects/<name>/manager/<name>_manager.c
   - Op: projects/<name>/ops/<op_name>.c
   Then: ./button.sh compile

Q: Compilation fails
A: Check error messages from compile_all.sh
   Common issues:
   - Missing #include statements
   - Syntax errors in .c file
   - Missing dependencies (pthread, etc.)

================================================================================
ADDING NEW PROJECTS
================================================================================

1. Create project structure:
   mkdir -p projects/<name>/{manager,ops/+x,layouts,pieces}

2. Create manager source:
   projects/<name>/manager/<name>_manager.c

3. Create ops (optional):
   projects/<name>/ops/<op_name>.c

4. Create project.pdl:
   projects/<name>/project.pdl

5. Compile:
   ./button.sh compile

6. Verify:
   ./button.sh check

7. Test in CHTPM!

================================================================================
ADDING NEW OPS
================================================================================

1. Create op source:
   projects/<name>/ops/<op_name>.c

2. Create/update ops_manifest.txt:
   projects/<name>/ops/ops_manifest.txt
   Format: op_name|description|args_format

3. Compile:
   ./button.sh compile

4. Install via Fondu (optional):
   ./pieces/system/fondu/fondu.+x --install <name>

5. Use in PAL scripts:
   OP <name>::<op_name> <args>

================================================================================
END OF COMPILE SYSTEM DOCUMENTATION
================================================================================
