# 🏭 Chapter 5: The App Factory
How do you build a new tool for TPMOS? It's easier than you think! We call this the **App Factory** pattern. 🛠️🏗️

---

## 🧬 What is an App?
In TPMOS, an "App" is just a directory with a specific DNA file. 🧬

### 📂 The App Anatomy
```text
pieces/apps/my_new_app/
├── my_new_app.pdl      # The App DNA
├── layouts/            # The UI screens (.chtpm files)
├── manager/            # The Brain (+x/module.+x)
└── pieces/             # Local pieces used by the app
```

---

## 📝 The App DNA (`.pdl`)
Every app needs a descriptor so the OS knows how to launch it.

```pdl
<app_id>op-ed</app_id>
<version>6.0</version>
<layout_path>pieces/apps/op-ed/layouts/main.chtpm</layout_path>
<module_path>pieces/apps/op-ed/manager/+x/op-ed_module.+x</module_path>
```

---

## 🛠️ Featured Apps
TPMOS comes with several powerful system tools:

### 🖋️ `op-ed` (The Piece Editor)
The most important tool! It allows you to create, delete, and modify Pieces in real-time. 
*   **Feature:** Roundtrip Parity. You can edit a file, save it, and the game updates instantly.

### 🐾 `fuzz-ops` (The Pet Manager)
A complex simulation app that manages "Fuzzpets". 
*   **Feature:** It uses **PAL (Prisc Assembly Language)** to give pets AI behaviors like "Hunger" and "Play".

### 👤 `user` (The Identity Manager)
Handles logins, profiles, and preferences. 
*   **Feature:** Persistent user state across different projects.

---

## 🛰️ The Horizon: Upcoming Apps
Several high-impact apps are currently in the incubation phase:

*   🤖 **AI-Labs**: A laboratory for local LLM (like Qwen/Gemma) integration. Used for tech R&D and autonomous codegen.
*   🌕 **LSR (Lunar Streetrace Raider)**: A "Civ-Lite" simulation where AI agents build lunar civilizations.
*   ⛓️ **P2P-NET**: A decentralized networking layer with blockchain and NFT trading.

---

## 🚀 Launching your App
To see your app in the OS, you just need to add a button to the main `os.chtpm` layout:

```xml
<button label="🚀 My New App" onClick="KEY:m" href="pieces/apps/my_new_app/my_new_app.pdl" />
```

> 💡 **Pro Tip:** Start by duplicating an existing app (like `playrm`) and changing the names. It's the fastest way to learn! 🏃‍♂️💨

---

## 🏛️ Scholar's Corner: The "Day the Editor Became the Game"
One of the most profound moments in TPMOS history was when a developer accidentally launched the **`op-ed`** (the editor) inside the `fuzz-op` (the pet game). Instead of crashing, the system simply rendered the editor's selector on top of the pet's map. The developer realized they could "play" the game by editing the world in real-time—placing walls to trap pets or changing their health with a few keystrokes. This blurred the line between **playing** and **building**, leading to our philosophy that "The Editor is the Game." 🖋️🎮

---

## 📝 Study Questions
1.  What is the minimum requirement for a directory to be considered an "App" in TPMOS?
2.  Which app is responsible for managing user profiles?
3.  How do you add a new app to the main OS menu?
4.  **True or False:** Every app must have its own `.pdl` file.

---
[Return to Index](INDEX.md)
