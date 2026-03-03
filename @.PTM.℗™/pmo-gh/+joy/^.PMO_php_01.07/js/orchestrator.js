/**
 * orchestrator.js - The PMO Module Logic (The "Stage")
 * This script acts as the Fuzzpet Module.
 */

class PMOModule {
    constructor(bridgePath) {
        this.bridgePath = bridgePath;
        this.state = {
            name: "Fuzzball",
            hunger: 50,
            happiness: 75,
            energy: 100,
            level: 1,
            status: "active",
            last_key: "None",
            pos_x: 5,
            pos_y: 5,
            clock_turn: 0,
            clock_time: "08:00:00"
        };
        this.responses = {}; // Map for PDL-defined responses
        this.lastHistoryPos = 0;
        this.currentLayout = 'pieces/chtpm/layouts/os.chtpm';
        this.lastViewChangedSize = 0;  // For marker file pattern
        this.currentResponseKey = "default";  // Track which response to show
    }

    async callBridge(action, path, data = null) {
        const url = `${this.bridgePath}?action=${action}&path=${path}`;
        const options = data ? { method: 'POST', body: data } : {};
        const res = await fetch(url, options);
        return await res.json();
    }

    async init() {
        // 1. Load State
        const mirror = await this.callBridge('read', 'pieces/apps/fuzzpet_app/fuzzpet/state.txt');
        if (mirror.content) {
            mirror.content.split('\n').forEach(line => {
                const [k, v] = line.split('=');
                if (k) this.state[k] = isNaN(v) ? v : parseInt(v);
            });
        }
        
        // 2. Load Responses from PDL (Architectural Parity)
        const pdl = await this.callBridge('read', 'pieces/apps/fuzzpet_app/fuzzpet/fuzzpet.pdl');
        if (pdl.content) {
            pdl.content.split('\n').forEach(line => {
                if (line.startsWith('RESPONSE')) {
                    const parts = line.split('|').map(p => p.trim());
                    if (parts.length >= 3) {
                        this.responses[parts[1]] = parts[2];
                    }
                }
            });
        }
        
        // 3. Sync Clock
        const clock = await this.callBridge('read', 'pieces/apps/fuzzpet_app/clock/state.txt');
        if (clock.content) {
            clock.content.split('\n').forEach(line => {
                const [k, v] = line.split(' ');
                if (k === 'turn') this.state.clock_turn = v;
                if (k === 'time') this.state.clock_time = v;
            });
        }
    }

    async pulse() {
        const history = await this.callBridge('read', 'pieces/apps/fuzzpet_app/fuzzpet/history.txt');
        if (history.content) {
            const lines = history.content.trim().split('\n');
            if (lines.length > this.lastHistoryPos) {
                for (let i = this.lastHistoryPos; i < lines.length; i++) {
                    const rawKey = lines[i].replace('KEY_PRESSED: ', '').trim();
                    if (rawKey !== "") await this.processKey(rawKey);
                }
                this.lastHistoryPos = lines.length;
                await this.syncState();
            }
        }
        await this.writeSovereignView();
    }

    async processKey(rawKey) {
        // Strict Numeric Protocol (Matches fuzzpet.c)
        const code = parseInt(rawKey);

        // 1. Report Parity
        if (code >= 2100 && code <= 2103) {
            const dirs = ["J-LEFT", "J-RIGHT", "J-UP", "J-DOWN"];
            this.state.last_key = dirs[code - 2100];
        } else if (code >= 2000 && code < 2100) {
            this.state.last_key = `J-BUTTON ${code - 2000}`;
        } else if (code === 1000) this.state.last_key = "LEFT";
        else if (code === 1001) this.state.last_key = "RIGHT";
        else if (code === 1002) this.state.last_key = "UP";
        else if (code === 1003) this.state.last_key = "DOWN";
        else if (code >= 32 && code <= 126) {
            this.state.last_key = `${code} ('${String.fromCharCode(code)}')`;
        } else {
            this.state.last_key = `CODE ${code}`;
        }

        // 2. Logic Parity
        // Movement: WASD (119, 97, 115, 100) | Arrows (1000-1003) | Joy (2100-2103)
        let moved = false;
        if (code === 119 || code === 87 || code === 1002 || code === 2102) { // UP
            this.state.pos_y = Math.max(0, this.state.pos_y - 1);
            moved = true;
        } else if (code === 115 || code === 83 || code === 1003 || code === 2103) { // DOWN
            this.state.pos_y = Math.min(8, this.state.pos_y + 1);
            moved = true;
        } else if (code === 97 || code === 65 || code === 1000 || code === 2100) { // LEFT
            this.state.pos_x = Math.max(0, this.state.pos_x - 1);
            moved = true;
        } else if (code === 100 || code === 68 || code === 1001 || code === 2101) { // RIGHT
            this.state.pos_x = Math.min(18, this.state.pos_x + 1);
            moved = true;
        }

        if (moved) {
            this.currentResponseKey = "moved";
            await this.incrementClock();
        }
        // Actions: '2'-'5' (50-53) | Joy B1-B4 (2001-2004)
        else if (code === 50 || code === 2001) { // Feed
            this.state.hunger = Math.max(0, this.state.hunger - 20); // Align with C (-20)
            this.currentResponseKey = "fed";
        } else if (code === 51 || code === 2002) { // Play
            this.state.happiness = Math.min(100, this.state.happiness + 10);
            this.currentResponseKey = "played";
        } else if (code === 52 || code === 2003) { // Sleep
            this.state.energy = Math.min(100, this.state.energy + 30); // Align with C (+30)
            this.currentResponseKey = "slept";
        } else if (code === 53 || code === 2004) { // Evolve
            this.state.level++;
            this.currentResponseKey = "evolved";
        } else if (code === 54) { // End Turn
            this.currentResponseKey = "default";
        } else {
            this.currentResponseKey = "default";
        }
    }

    async incrementClock() {
        // 1. Parse current time
        let [h, m, s] = this.state.clock_time.split(':').map(Number);
        
        // 2. Increment by 5 mins (Matches C plugin)
        m += 5;
        if (m >= 60) {
            m -= 60;
            h = (h + 1) % 24;
        }
        
        // 3. Format back
        this.state.clock_time = `${String(h).padStart(2, '0')}:${String(m).padStart(2, '0')}:${String(s).padStart(2, '0')}`;
        
        // 4. Increment turn
        this.state.clock_turn = parseInt(this.state.clock_turn) + 1;
        
        await this.syncClock();
    }

    async syncClock() {
        const data = `turn ${this.state.clock_turn}\ntime ${this.state.clock_time}\n`;
        await this.callBridge('write', 'pieces/apps/fuzzpet_app/clock/state.txt', data);
    }

    async syncState() {
        const data = Object.entries(this.state).map(([k, v]) => `${k}=${v}`).join('\n');
        await this.callBridge('write', 'pieces/apps/fuzzpet_app/fuzzpet/state.txt', data);
    }

    async writeSovereignView() {
        let map = "";
        for (let y = 0; y < 10; y++) {
            let row = "";
            for (let x = 0; x < 20; x++) {
                if (x === this.state.pos_x && y === this.state.pos_y) row += "@";
                else if (x === 17 && y < 3) row += "T";
                else if (x === 4 && y > 2 && y < 7) row += "R";
                else if (x === 0 || x === 19 || y === 0 || y === 9) row += "#";
                else row += ".";
            }
            map += `║  ${row.padEnd(20)}                                     ║\n`;
        }

        const responseText = this.responses[this.currentResponseKey] || this.responses["default"] || "*confused tilt* ...huh?";
        
        const viewContent = map + 
                          `╠═══════════════════════════════════════════════════════════╣\n` +
                          `║  [FUZZPET]: ${responseText.padEnd(45)} ║\n` +
                          `║  DEBUG [Last Key]: ${this.state.last_key.padEnd(38)} ║\n`;

        await this.callBridge('write', 'pieces/apps/fuzzpet_app/fuzzpet/view.txt', viewContent);
        // Signal view change (marker file pattern - matches C version)
        await this.callBridge('append', 'pieces/apps/fuzzpet_app/fuzzpet/view_changed.txt', 'X\n');
    }
}
