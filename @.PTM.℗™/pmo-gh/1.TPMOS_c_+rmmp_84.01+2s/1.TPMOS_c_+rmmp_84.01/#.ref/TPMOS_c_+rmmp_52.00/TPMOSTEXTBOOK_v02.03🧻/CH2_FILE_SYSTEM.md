# 📁 Chapter 2: The Filesystem is the Database
In a traditional OS, the filesystem is just for storage. In TPMOS, the filesystem **is** the system. 🏛️

---

## 🗺️ The Project Map
Everything has a place, and there is a place for everything. The root of the world is defined in `pieces/locations/location_kvp`.

### 🔑 `location_kvp`: The Compass
This file tells the system where to find the "Ground". If you move the project, you only change this file.
*   `project_root=/home/user/TPMOS/`
*   `pieces_dir=pieces/`
*   `apps_dir=pieces/apps/`

---

## 📂 The Directory Hierarchy
Let's look at how a project is structured. It's like a nested doll. 🪆

```text
/projects/my_game/
├── project.pdl         # The "World DNA"
├── manager/            # The "World Brain" (.+x binaries)
├── maps/               # The "Geography"
└── pieces/             # The "Inhabitants"
    ├── hero/
    │   ├── piece.pdl   # Hero DNA
    │   └── state.txt   # Hero Stats
    └── slime_01/
        ├── piece.pdl   # Slime DNA
        └── state.txt   # Slime Stats
```

---

## 📬 File-Based IPC (The Mailbox) ✉️
How do two programs talk to each other if they don't share memory? They write notes!

1.  **The History Buffer:** `keyboard/history.txt`. The input driver writes `A`, the Parser reads `A`.
2.  **The Pulse:** `frame_changed.txt`. When a renderer finishes a frame, it "pulses" this file. Other programs see the pulse and wake up.
3.  **The Response:** `last_response.txt`. When an Op runs, it writes its output here so the UI can show it to the user.

---

## 🛡️ CPU Safety & The `stat()` Guard
Reading files constantly can be heavy. To stay fast, we use the **Stat-First Pattern**. 🛡️

*   **Logic:** Don't open the file unless `stat()` tells you the "Modified Time" (mtime) has changed.
*   **Result:** 0% CPU usage when nothing is happening. TPMOS is a "Green" OS! 🌿

---

## 🏛️ Scholar's Corner: The "Lost Map of Lunar Streetrace"
Legend has it that a project was once "lost" during a massive server migration. For three days, the `lsr` simulation was invisible. Then, a developer realized that only the `location_kvp` file had been corrupted. The entire world—its companies, its bots, its history—was still there in the `projects/` folder. By writing a single line back into `location_kvp`, the entire universe "woke up" as if no time had passed. This proved that in TPMOS, **"Geography is Destiny."** If the files exist, the world is never truly lost. 🗺️✨

---

## 📝 Study Questions
1.  What is the role of the `location_kvp` file?
2.  Explain File-Based IPC in your own words.
3.  How does the "Stat-First Pattern" improve CPU efficiency?
4.  **Imagine:** You want to move your TPMOS project to a different drive. Which file do you need to update to ensure everything still works?

---
[Return to Index](INDEX.md)
