/**
 * parser.js - The PMO OS Parser (The "Theater")
 * Strictly emulates chtpm_parser.c text-only composition.
 */

class CHTPMParser {
    constructor() {
        this.vars = {};
        this.focusIndex = 1;
        this.activeIndex = -1;
        this.navBuffer = "";
        this.elements = [];  // Store elements for doJump()
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
        this.elements = [];  // Reset elements array
        
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
                    // Store element for doJump()
                    this.elements.push({
                        type: tagName,
                        interactiveIdx: interactiveCount,
                        label: label
                    });
                    
                    let indicator = "[ ]";
                    if (interactiveCount === this.activeIndex) indicator = "[^]";
                    else if (interactiveCount === this.focusIndex) indicator = "[>]";
                    
                    const btnLabel = label || (tagName === 'cli_io' ? "_" : "");
                    output += `${indicator} ${interactiveCount}. [${this.substitute(btnLabel)}] `;
                }
            } else if (part.startsWith('</')) {
                // Ignore closing tags
            } else {
                // Raw text content
                if (part.trim() !== "" || part === "\n") {
                    if (part.includes("${game_map}")) {
                        output += this.substitute(part);
                    } else {
                        const cleaned = part.replace(/^\n+|\n+$/g, '');
                        if (cleaned !== "") output += this.substitute(cleaned);
                    }
                }
            }
        }

        let footerPrefix = (this.activeIndex === -1) ? "Nav > " : "Active [^]: ";
        let footerSuffix = (this.activeIndex === -1) ? "" : " (ESC to exit)";
        const footer = `\n╚═══════════════════════════════════════════════════════════╝\n` +
                      `${footerPrefix}${this.navBuffer}_${footerSuffix}`;
        
        return output + footer;
    }
}
