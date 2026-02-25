/**
 * orchestrator.js - The PMO Module Logic (The "Stage")
 * This script acts as the Fuzzpet Module.
 * It reads history, updates state.txt (Mirror), and writes view.txt (Sovereign View).
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
        this.lastHistoryPos = 0;
        this.currentLayout = 'pieces/chtpm/layouts/os.chtpm'; // FIXED: Start at OS Main Menu
    }

    async callBridge(action, path, data = null) {
        const url = `${this.bridgePath}?action=${action}&path=${path}`;
        const options = data ? { method: 'POST', body: data } : {};
        const res = await fetch(url, options);
        return await res.json();
    }

    // Initialize state from files (DNA/Mirror)
    async init() {
        const mirror = await this.callBridge('read', 'pieces/apps/fuzzpet_app/fuzzpet/state.txt');
        if (mirror.content) {
            mirror.content.split('\n').forEach(line => {
                const [k, v] = line.split('=');
                if (k) this.state[k] = isNaN(v) ? v : parseInt(v);
            });
        }
        
        // Also fetch clock state
        const clock = await this.callBridge('read', 'pieces/apps/fuzzpet_app/clock/state.txt');
        if (clock.content) {
            clock.content.split('\n').forEach(line => {
                const [k, v] = line.split(' ');
                if (k === 'turn') this.state.clock_turn = v;
                if (k === 'time') this.state.clock_time = v;
            });
        }
    }

    // The Pulse: Check for new input and update logic
    async pulse() {
        const history = await this.callBridge('read', 'pieces/apps/fuzzpet_app/fuzzpet/history.txt');
        if (history.content) {
            const lines = history.content.trim().split('\n');
            if (lines.length > this.lastHistoryPos) {
                for (let i = this.lastHistoryPos; i < lines.length; i++) {
                    await this.processKey(lines[i].replace('KEY_PRESSED: ', '').trim());
                }
                this.lastHistoryPos = lines.length;
                await this.syncState();
            }
        }
        await this.writeSovereignView();
    }

    async processKey(key) {
        this.state.last_key = `CODE ${key}`;
        if (key === '2') this.state.hunger = Math.max(0, this.state.hunger - 10); // Feed
        if (key === '3') this.state.happiness = Math.min(100, this.state.happiness + 10); // Play
        if (key === '4') this.state.energy = Math.min(100, this.state.energy + 20); // Sleep
        
        // Simple Move Logic for Map
        if (key === 'w') this.state.pos_y = Math.max(0, this.state.pos_y - 1);
        if (key === 's') this.state.pos_y = Math.min(8, this.state.pos_y + 1);
        if (key === 'a') this.state.pos_x = Math.max(0, this.state.pos_x - 1);
        if (key === 'd') this.state.pos_x = Math.min(18, this.state.pos_x + 1);
    }

    async syncState() {
        const data = Object.entries(this.state).map(([k, v]) => `${k}=${v}`).join('\n');
        await this.callBridge('write', 'pieces/apps/fuzzpet_app/fuzzpet/state.txt', data);
    }

    /**
     * writeSovereignView() - The Module generates its own stage content.
     * This matches the C version's map rendering.
     */
    async writeSovereignView() {
        let map = "  ####################\n";
        for (let y = 0; y < 9; y++) {
            let row = "  #";
            for (let x = 0; x < 18; x++) {
                if (x === this.state.pos_x && y === this.state.pos_y) row += "@";
                else if (x === 17 && y < 3) row += "T";
                else if (x === 4 && y > 2 && y < 7) row += "R";
                else row += ".";
            }
            map += row + "#\n";
        }
        map += "  ####################";

        const responseText = this.state.hunger < 20 ? "Yum yum~ Feeling so happy!" : "*moves around excitedly*";
        
        const viewContent = `╠═══════════════════════════════════════════════════════════╣\n` +
                          `║  [FUZZPET]: ${responseText.padEnd(46)} ║\n` +
                          `║  DEBUG [Last Key]: ${this.state.last_key.padEnd(39)} ║\n` +
                          `\n` +
                          map;

        await this.callBridge('write', 'pieces/apps/fuzzpet_app/fuzzpet/view.txt', viewContent);
    }
}
