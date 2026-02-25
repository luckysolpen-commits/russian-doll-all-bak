/**
 * parser.js - The PMO OS Parser (The "Theater")
 * Strictly emulates chtpm_parser.c text-only composition.
 */

class CHTPMParser {
    constructor() {
        this.vars = {};
        this.focusIndex = 1;
        this.navBuffer = "";
    }

    setVars(newVars) {
        this.vars = { ...this.vars, ...newVars };
    }

    substitute(text) {
        if (!text) return "";
        return text.replace(/\${([^}]+)}/g, (match, varName) => {
            return this.vars[varName] !== undefined ? this.vars[varName] : match;
        });
    }

    /**
     * composeFrame() - Ported logic from chtpm_parser.c
     */
    async composeFrame(layoutContent) {
        let output = "";
        let interactiveCount = 0;
        
        // Split by tags but keep the tags in the array
        const parts = layoutContent.split(/(<[^>]+>)/g);

        for (let part of parts) {
            if (part.startsWith('<') && !part.startsWith('</')) {
                // Match the tag name
                const tagNameMatch = part.match(/<([^\s>/]+)/);
                if (!tagNameMatch) continue;
                const tagName = tagNameMatch[1];
                
                // Match the label attribute
                const attrMatch = part.match(/label="([^"]+)"/);
                const label = attrMatch ? attrMatch[1] : "";
                
                if (tagName === 'br') {
                    output += "\n";
                } else if (tagName === 'text') {
                    output += this.substitute(label);
                } else if (tagName === 'button' || tagName === 'link' || tagName === 'menu' || tagName === 'cli_io') {
                    interactiveCount++;
                    const indicator = (interactiveCount === this.focusIndex) ? "[>]" : "[ ]";
                    const btnLabel = label || (tagName === 'cli_io' ? "_" : "");
                    output += `${indicator} ${interactiveCount}. [${this.substitute(btnLabel)}] `;
                }
            } else if (part.startsWith('</')) {
                // Ignore closing tags
            } else {
                // Raw text content: Ignore pure whitespace/newlines between tags
                // In C, these are handled by the line-by-line reading.
                // Here we only keep it if it's not just "extra formatting" from the .chtpm file.
                if (part.trim() !== "" || part === "\n") {
                    // If it's the game_map variable, we keep all whitespace
                    if (part.includes("${game_map}")) {
                        output += this.substitute(part);
                    } else {
                        // For other parts, we trim leading/trailing newlines to avoid double spacing
                        const cleaned = part.replace(/^\n+|\n+$/g, '');
                        if (cleaned !== "") output += this.substitute(cleaned);
                    }
                }
            }
        }

        const footer = `\n╚═══════════════════════════════════════════════════════════╝\n` +
                      `Nav > ${this.navBuffer}_`;
        
        return output + footer;
    }
}
