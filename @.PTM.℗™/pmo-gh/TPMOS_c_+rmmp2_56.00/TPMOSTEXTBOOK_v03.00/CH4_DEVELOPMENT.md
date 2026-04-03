# 💪 Chapter 4: Muscles & Brains (Ops & Modules)
In TPMOS, we separate **Thinking** from **Doing**. This is the key to a stable, CPU-safe system. 🧠💪

---

## 🧠 The Brain (The Module)
The **Module** is a long-running background process (the "Brain"). Its job is to listen for user input and make decisions. 
*   **File:** Usually named `*_module.c` or `*_manager.c`.
*   **Job:** Polls the `history.txt` buffer and updates the `state.txt` mirror.

### 🛡️ The CPU-Safe Mandate
Because Modules run in the background, they must be "polite" to the CPU. We follow the **CPU-Safe Template**:
1.  **Throttling:** If the app isn't on screen, the Module sleeps for 100ms. If it is on screen, it sleeps for 16ms (60 FPS). 😴
2.  **Stat-First:** Don't read the history file unless its size has changed (`stat()`).
3.  **Signal Handling:** Always catch `SIGINT` (Ctrl+C) so you can clean up your children. 🧹

### 🛡️ The Network & Path Safety Mandates (New April 2026)
As TPMOS evolves, we've added two critical mandates for system stability:
1.  **Dynamic Path Mandate:** NEVER hardcode project paths. Always resolve the `project_root` from `pieces/locations/location_kvp`. This ensures your code works even if the folder is moved or versioned. 🗺️
2.  **Network Safety Mandate:** Ghost listeners cause crashes! All network-aware modules must ensure their ports (default 8000-8010) are released on exit. The `kill_all.sh` script is the surgical tool for this. 释放端口! 🔌

---

## 💪 The Muscle (The Op)
The **Op** is a short-lived binary (the "Muscle"). It does one thing and then exits.
*   **File:** Usually ends in `.+x` (e.g., `move_entity.+x`).
*   **Job:** Performs a specific action like moving a player or placing a tile.

### 🚫 Why no `system()`?
We never use the `system()` command in C. Why? Because it's hard to kill! Instead, we use `fork()` and `exec()`.
*   **Fork:** Create a child process.
*   **Exec:** Turn that child into the Muscle.
*   **Wait:** The Brain waits for the Muscle to finish.

---

## 📦 Organizing Ops: The Centralized Directory Pattern

### ✓ DO: Centralized Ops Directory
All ops for a project should live in ONE central location:

```
projects/<project_id>/ops/
├── +x/                    # Compiled binaries
│   ├── create_profile.+x
│   ├── auth_user.+x
│   └── get_session.+x
├── src/                   # Source code (optional)
│   ├── create_profile.c
│   ├── auth_user.c
│   └── get_session.c
└── ops_manifest.txt       # Registry of all ops (REQUIRED for Fondu)
```

### ops_manifest.txt Format
```
# Comment lines start with #
# Format: op_name|description|args_format

create_profile|Creates a new user profile|project_path:string,username:string
auth_user|Authenticates a user session|project_path:string,username:string
get_session|Get current session data|project_path:string,username:string
```

### project.pdl Registration
In your `project.pdl`, declare that your project exposes ops:

```
[META]
project_id = user
exposes_ops = true
entry_layout = projects/user/layouts/user.chtpm

[OPS]
ops_dir = projects/user/ops/
manifest = projects/user/ops/ops_manifest.txt
```

### ✗ DON'T: Scatter Ops
```
✗ projects/<project>/traits/move_entity.c     (scattered in traits/)
✗ projects/<project>/plugins/move_entity.c    (scattered in plugins/)
✗ projects/<project>/manager/move_entity.c    (scattered in manager/)
```

---

## 🔧 Fondu Installation of Ops

When you run `./fondu --install <project>`, Fondu will:

1. Read `project.pdl` → finds `exposes_ops = true`
2. Read `ops/ops_manifest.txt` → gets list of ops
3. Copy `ops/+x/*.+x` to `pieces/apps/installed/<project>/ops/+x/`
4. Register ops in `pieces/os/ops_registry/<project>.txt`
5. Update `pieces/os/ops_catalog.txt`

Now other projects can call your ops via PAL scripts:
```
OP user::create_profile "player1"
OP user::auth_user "player1"
```

---

## 📜 External Ops KVP (Advanced)

For advanced setups, you can maintain an external KVP file:

**File:** `projects/<project_id>/ops/ops_registry.kvp`

**Format:**
```
op_create_profile=projects/user/ops/create_profile.c|username:string
op_auth_user=projects/user/ops/auth_user.c|username:string
op_get_session=projects/user/ops/get_session.c|username:string
```

**Use Cases:**
- Dynamic op discovery
- Ops that span multiple directories
- External/trunk ops that aren't in `projects/`

---

## 🎯 Why Centralize Ops?

| Benefit | Explanation |
|---------|-------------|
| ✓ **Fondu Bundling** | Fondu can install all ops as a single bundle |
| ✓ **Discovery** | Other projects discover ops via `--list-ops` |
| ✓ **Separation** | Clean separation of "muscle" (ops) from "brain" (manager) |
| ✓ **Testing** | Easy to test ops standalone (no manager needed) |
| ✓ **Reusability** | Reusable across projects (PAL scripts call by `project::op_name`) |
| ✓ **Single Truth** | Single source of truth for what ops exist |

---

## 🛠️ The Developer Workflow
1.  **Pick a Template:** Copy `#.docs/future-facing/cpu_safe_module_template.c`.
2.  **Define your Logic:** What happens when the user presses 'W'? (Call `move_entity.+x north`).
3.  **Update the Mirror:** Write the new coordinates to `state.txt`.
4.  **Pulse the Frame:** Tell the Renderer that something changed! 💓

> 💡 **Pro Tip:** Keep your Muscles "dumb" and your Brains "thin". If a Muscle can do it, don't put it in the Brain! ⚡

---

## 🏛️ Scholar's Corner: The "Thin Brain" Revolution
In the early days of TPMOS, the `fuzz_legacy_manager.c` was over 5,000 lines of code. It handled movement, hunger, the market, and rendering all in one massive loop. It was a "Thick Brain." When one part of the code crashed, the entire pet simulation died! The "Thin Brain" revolution happened when we moved the movement logic into `move_entity.+x` and the hunger logic into PAL scripts. Suddenly, the manager was under 500 lines. The system became stable, and developers could finally sleep at night. 💤🧘‍♂️

### Modern Example: The User Project (April 2026)
The `projects/user/` project is the canonical example of centralized ops:

```
projects/user/
├── project.pdl (exposes_ops = true)
├── ops/
│   ├── +x/create_profile.+x
│   ├── +x/auth_user.+x
│   ├── +x/get_session.+x
│   └── ops_manifest.txt
└── manager/+x/user_manager.+x
```

When installed via Fondu (`./fondu --install user`), all three ops become available to every other project via PAL scripts. This is the "Ops as a Service" pattern that powers TPMOS. ⚡

---

## 📝 Study Questions
1.  Explain the difference between a "Module" and an "Op."
2.  Why is the `fork()` / `exec()` pattern preferred over the `system()` command?
3.  What are the three core rules of the "CPU-Safe Mandate"?
4.  **True or False:** A Module should sleep longer when its app is not the "active layout."
5.  **Short Answer:** What is the correct directory structure for organizing ops in a project?
6.  **Scenario:** You want to create a new op called `teleport_entity`. Where do you put it, and what file do you update to make it discoverable by Fondu?
7.  **Critical Thinking:** Why is centralizing ops better than scattering them in `traits/`, `plugins/`, etc.?

---
[Return to Index](INDEX.md)
