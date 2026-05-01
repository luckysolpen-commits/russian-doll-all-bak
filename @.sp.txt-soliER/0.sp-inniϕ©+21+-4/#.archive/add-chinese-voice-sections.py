#!/usr/bin/env python3
"""
Add 中文老师 (Chinese teacher) voice sections to the memory palace document.
Finds vocabulary tables and key phrases, adds Chinese voice lines for TTS.
"""

import re

INPUT_FILE = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/footrace-chinese-memory-palace.md"
OUTPUT_FILE = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/footrace-chinese-memory-palace-tts.md"

with open(INPUT_FILE, 'r', encoding='utf-8') as f:
    content = f.read()

lines = content.split('\n')
output_lines = []
i = 0

while i < len(lines):
    line = lines[i]
    output_lines.append(line)

    # Detect vocabulary tables (lines starting with | English | 中文 |)
    if '| English | 中文 | Pinyin |' in line:
        # Collect all table rows
        table_rows = []
        j = i + 2  # Skip header separator line
        while j < len(lines) and lines[j].startswith('|'):
            row = lines[j]
            # Parse: | english | chinese | pinyin | memory |
            parts = [p.strip() for p in row.split('|') if p.strip()]
            if len(parts) >= 3:
                english = parts[0]
                chinese = parts[1]
                pinyin = parts[2]
                table_rows.append((english, chinese, pinyin))
            j += 1

        # Add Chinese voice line after the table
        if table_rows:
            # Build a readable Chinese voice line
            chinese_items = []
            for eng, chi, pin in table_rows:
                chinese_items.append(f"{chi}，{pin}")
            chinese_line = "，".join(chinese_items[:10])  # Max 10 per line
            if len(chinese_items) > 10:
                chinese_line += "，等等"
            output_lines.append("")
            output_lines.append(f"**中文老师:** 跟我读：{chinese_line}")
            output_lines.append("")

        # Skip the table rows we already processed
        i = j - 1

    # Detect key phrase blocks (> **中文** — Pinyin — "English")
    elif line.startswith('> **') and '—' in line and len(line) > 20:
        # Check if next lines contain Chinese phrase
        if i + 2 < len(lines):
            next_line = lines[i + 1]
            # Look for Chinese characters in the blockquote
            chinese_match = re.search(r'\*\*(.+?)\*\*', line)
            if chinese_match:
                chinese_text = chinese_match.group(1)
                # Check if it contains Chinese characters
                if re.search(r'[\u4e00-\u9fff]', chinese_text):
                    output_lines.append("")
                    output_lines.append(f"**中文老师:** {chinese_text}")
                    output_lines.append("")

    # Detect "Repeat after me" lines
    elif 'Repeat after me:' in line:
        # Extract the Chinese text after "Repeat after me:"
        chinese_part = line.split('Repeat after me:')[-1].strip()
        if chinese_part and re.search(r'[\u4e00-\u9fff]', chinese_part):
            output_lines.append("")
            output_lines.append(f"**中文老师:** {chinese_part}")
            output_lines.append("")

    i += 1

with open(OUTPUT_FILE, 'w', encoding='utf-8') as f:
    f.write('\n'.join(output_lines))

print(f"Done! Output saved to: {OUTPUT_FILE}")
print(f"Original: {len(lines)} lines → New: {len(output_lines)} lines")
