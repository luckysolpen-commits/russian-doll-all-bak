#!/usr/bin/env python3
"""
Reverse the xt...tx format in FU D&D session files.
From: xt 汉字 pinyin ... eng English ... tx
To:   eng English ... tx 汉字 pinyin xt
"""

import re
import os
import glob

INPUT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/fud&d"
OUTPUT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/reverse_learn"

def reverse_xt_tx(text):
    """Reverse all xt...tx patterns in text."""
    # Pattern: xt 汉字 pinyin ... eng English ... tx
    # We need to capture: the Chinese part (汉字 pinyin) and the English part (English)
    # The pattern is: xt [chinese chars] [pinyin] ... eng [english] ... tx
    
    def replacer(match):
        full = match.group(0)
        # Extract everything between xt and tx
        inner = match.group(1)
        
        # Split by " eng " to separate Chinese and English parts
        if ' eng ' in inner:
            parts = inner.split(' eng ', 1)
            chinese_part = parts[0].strip()
            english_part = parts[1].strip()
            # Remove trailing " ... tx" from english part if present
            if english_part.endswith(' ... tx'):
                english_part = english_part[:-6].strip()
            # Remove leading "..." from chinese part if present
            if chinese_part.startswith('... '):
                chinese_part = chinese_part[4:]
            # New format: eng English ... tx 汉字 pinyin xt
            return f"eng {english_part} ... tx {chinese_part} xt"
        else:
            # No "eng" found, just reverse the whole thing
            return f"eng {inner} ... tx xt"
    
    # Match xt ... tx patterns (non-greedy)
    pattern = r'xt\s+(.+?)\s+tx'
    result = re.sub(pattern, replacer, text)
    return result

# Process all session files
session_files = sorted(glob.glob(os.path.join(INPUT_DIR, "fud&d-sesh*.txt")))

for input_file in session_files:
    filename = os.path.basename(input_file)
    output_file = os.path.join(OUTPUT_DIR, filename)
    
    print(f"Processing: {filename}")
    
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    reversed_content = reverse_xt_tx(content)
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(reversed_content)
    
    print(f"  → Saved: {filename}")

print(f"\nDone! Processed {len(session_files)} files.")
print(f"Output directory: {OUTPUT_DIR}")
