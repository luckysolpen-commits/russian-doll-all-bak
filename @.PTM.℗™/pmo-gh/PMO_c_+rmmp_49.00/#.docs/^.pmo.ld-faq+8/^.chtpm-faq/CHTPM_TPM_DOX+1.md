[2026-03-18 ACTIVE CHTPM/TPM REFERENCE]
This file is the active concise reference for CHTPM + TPM layering.

# CHTPM + TPM Reference (Current)

## Design intent
The stack must remain modular and recursively composable so agents can modify one part while other parts keep running.
If behavior is hardcoded across layers, this design intent is broken.

## Layer contract
1. CHTPM (Theater/View)
- Layout composition
- Variable substitution
- Input routing
- No app-specific business ownership
- CHTPM support modules (player/orchestrator/render helpers) must stay infrastructure-level.
- They should not hardcode project-specific gameplay assumptions (piece IDs, app rules, world/map semantics).

2. Module (Orchestration)
- Reads routed input
- Calls ops/plugins
- Synchronizes piece state
- Triggers render/update artifacts
- Must prefer data-driven dispatch and shared contracts over hardcoded one-off logic.

3. Piece (Atomic state unit)
- File-backed state and behavior descriptors
- Container-capable (piece may contain child pieces)

## Canonical storage model
`projects/<project>/pieces/world_<id>/map_<id>/<piece_id>/`

Rules:
- world/map-first lookup
- temporary fallback compatibility for migration
- no mandatory `/pieces/` under map directories

## Runtime pipeline (simplified)
1. Input captured
2. Parser routes input to app history
3. Module handles key/event
4. Ops/plugins update piece state
5. Render op produces map/stage view artifact
6. Parser composes final frame

## Preferred UI Patterns

### Form-Based Input (RECOMMENDED for login/settings)

Instead of dynamic submenus that change screens, use **always-visible form fields**:

```xml
<!-- Layout: Always-visible fields with cli_io -->
<text label="Username: " /><cli_io id="username" label="${username_input}" /><br/>
<text label="Password: " /><cli_io id="password" label="${password_input}" /><br/>
<button label="Login" onClick="login" />
```

```c
// Module: State variables persist across renders
static char input_username[MAX_USERNAME] = "";
static char input_password[MAX_PASSWORD] = "";

// State writing (always update form fields)
fprintf(f, "username_input=%s\n", input_username);
fprintf(f, "password_input=%s\n", masked_password);

// Method handlers (no screen switching)
static void method_login(void) {
    // Process input_username and input_password
}

// ESC just deactivates - NEVER clears
if (key == 27) { return; }
```

**Benefits:**
- Fields never clear unless explicitly cleared
- ESC preserves all input
- Users see all fields at once (intuitive)
- Simple state management (no `auth_screen` switching)

### Dynamic Submenus (ONLY when truly needed)

Use `${piece_methods}` with PDL method dispatch when:
- Options change based on context (e.g., pet actions vs selector actions)
- Screen space is limited
- Different entities have different method sets

**Pattern:**
```xml
<text label="║  " />${piece_methods}<text label=" ║" /><br/>
```

```c
// Module: Dispatch by method name, not key number
static MethodMapping methods[] = {
    {2, "feed"}, {3, "play"}, {4, "sleep"}, {0, NULL}
};
dispatch_method(get_method_name(key));
```

---

## Current roadmap alignment
- Step 1 emphasis: op-ed save/load/local-play + no-code event flow.
- Later phases: AI, pet expansion, voice, simulation, network, gl-os, install/versioning, kernel.

## Implementation caution
If old docs conflict with this file, prefer this file and Mar 18 roadmap/monolith docs.
Also prefer modular composition over hardcoded shortcuts even if shortcuts appear faster in the moment.
Special caution: avoid copying legacy hardcoded path snippets from old CHTPM support code into new module logic.
Do not resolve missing metadata (`entry_layout`, `player_piece`) with static guesses when scan/registry derivation is available.
