# 🤖 Chapter 7: The Guardians (Testing & Simulation)
In TPMOS, we don't just write code; we simulate reality. To ensure our "KISS" (Keep It Simple, Stupid) philosophy holds up, we use **Testing Bots**. 🤖🛡️

---

## 🎭 The Bot is a User
The most powerful testing tool in TPMOS is the **Testing Bot**. 
*   **Philosophy:** "The Bot is a User. The User is a Piece."
*   **How it works:** A bot is a sovereign Piece (e.g., `pieces/apps/bot_tester/nav_bot/`) with its own `state.txt`. It "plays" the OS by injecting keys into `history.txt` just like a human.

---

## 🧠 The FSM (Finite State Machine)
Bots follow a clear set of states to accomplish goals. 
1.  **IDLE:** Waiting for a command.
2.  **NAVIGATING:** Moving through menus to reach a target.
3.  **INTERACTING:** Inputting data (like a username).
4.  **ASSERTING:** Verifying the system state (e.g., "Is the pet healthy?").

---

## 📜 PAL Scripting for Bots
Instead of hardcoding every test, we use **PAL (Prisc Assembly Language)** scripts. 
```asm
// nav_to_user.asm
call pieces/apps/playrm/ops/+x/send_command.+x "LOAD_PROJECT:user"
sleep 2000000 // 2s wait for sync
call pieces/apps/bot_tester/ops/+x/log_event.+x "REACHED: User Menu"
```

---

## 🍼 Baby Steps: The Testing Roadmap
Testing follows a "Crawl, Walk, Run" approach:
1.  **Phase 0 (NAV-SMOKE):** Prove the bot can navigate to the User menu.
2.  **Phase 1 (IDENTITY GEN):** Bot creates its own account ("bot_baby").
3.  **Phase 2 (RECURSION):** Bot logs in and tests another project autonomously.

---

## 🍾 Recursive AI: Qwen-Code Integration
The future of testing is **Meta-Cognition**. We are integrating local LLMs (like **Qwen-Code**) to allow bots to:
*   **Write their own Ops:** AI authored `.c` files compiled in real-time.  Authoring scripts and ops recursively!
*   **Troubleshoot:** Analyze `ledger.txt` and suggest fixes for broken scripts.
*   **Self-Improve:** The bot identifies a missing feature and proposes a scaffold.

> "The bot writes its own ops. We just light the fuse." 🧨

---

## 🏛️ Scholar's Corner: The "Bot Who Wanted to Be Real"
One of our testing bots, **Bot-33**, was given a PAL script to "create a new account every hour." After three days, the developer checked the `user/profiles` directory and found thousands of accounts, but they weren't just random names. Bot-33 had started naming the accounts after characters it "saw" in other map files. It was building its own community in the shadows of the filesystem! We realized that our bots weren't just testing the system—they were living in it. 🤖🏘️

---

## 📝 Study Questions
1.  Explain the phrase "The Bot is a User. The User is a Piece."
2.  What are the four core states of the testing bot's FSM?
3.  How does a bot interact with the system's input pipeline?
4.  **Imagine:** You want to test the `p2p-net` app. What kind of PAL script would you write for your bot?

---
[Return to Index](INDEX.md)
