#!/usr/bin/env python3
"""
Carefully reverse translation patterns without mangling.
Only handles clean patterns, leaves ambiguous ones alone.
"""

import re
import os
import glob

INPUT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/fud&d"
OUTPUT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/reverse_learn"

def reverse_carefully(text):
    # Pattern: xt 汉字 pinyin ... eng English ... tx
    # Must have clear boundaries
    def replacer(match):
        inner = match.group(1)
        # Must contain " eng " to be valid
        if ' eng ' in inner:
            parts = inner.split(' eng ', 1)
            chinese = parts[0].strip()
            english = parts[1].strip()
            # Remove trailing " ... tx"
            if english.endswith(' ... tx'):
                english = english[:-6].strip()
            # Must have actual content
            if chinese and english and len(english) > 1:
                return f"eng {english} ... tx {chinese} xt"
        return match.group(0)
    
    # Match xt ... tx with clear boundaries
    # Use word boundaries to avoid partial matches
    text = re.sub(r'\bxt\s+(.+?)\s+tx\b', replacer, text, flags=re.DOTALL)
    
    # Pattern: 汉字 pinyin ... eng English ... tx (no xt marker)
    def replacer2(match):
        full = match.group(0)
        if ' eng ' in full:
            parts = full.split(' eng ', 1)
            chinese = parts[0].strip()
            english = parts[1].strip()
            if english.endswith(' ... tx'):
                english = english[:-6].strip()
            if chinese and english and len(english) > 1:
                return f"eng {english} ... tx {chinese} xt"
        return full
    
    # Match Chinese characters followed by pinyin, then ... eng English ... tx
    text = re.sub(r'([\u4e00-\u9fff][^\n]{3,}?)\s*\.\.\.\s*eng\s+([^\n]{3,}?)\s*\.\.\.\s*tx\b', replacer2, text)
    
    return text

# Process all session files
session_files = sorted(glob.glob(os.path.join(INPUT_DIR, "fud&d-sesh*.txt")))

for input_file in session_files:
    filename = os.path.basename(input_file)
    output_file = os.path.join(OUTPUT_DIR, filename)
    
    print(f"Processing: {filename}")
    
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    reversed_content = reverse_carefully(content)
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(reversed_content)
    
    # Quick check
    old_count = len(re.findall(r'[\u4e00-\u9fff][^\n]*?\.\.\.\s*eng\s+[a-zA-Z]', reversed_content))
    new_count = len(re.findall(r'eng\s+[a-zA-Z].*?\.\.\.\s*tx\s+[\u4e00-\u9fff]', reversed_content))
    print(f"  → Reversed: {new_count}, Still old format: {old_count}")

print(f"\nDone!")
