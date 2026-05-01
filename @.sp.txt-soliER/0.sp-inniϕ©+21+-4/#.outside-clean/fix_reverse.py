#!/usr/bin/env python3
"""
Fix reversal: xt 汉字 pinyin ... eng English ... tx → eng English ... tx 汉字 pinyin xt
"""

import re
import os
import glob

INPUT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/fud&d"
OUTPUT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/reverse_learn"

def reverse_xt(text):
    """Reverse xt 汉字 pinyin ... eng English ... tx → eng English ... tx 汉字 pinyin xt"""
    def replacer(match):
        full_match = match.group(0)
        inner = match.group(1)
        
        # Split by " eng " to separate Chinese and English
        if ' eng ' in inner:
            parts = inner.split(' eng ', 1)
            chinese_part = parts[0].strip()
            english_part = parts[1].strip()
            
            # Remove trailing " ... tx" from english if present
            if english_part.endswith(' ... tx'):
                english_part = english_part[:-6].strip()
            
            # New format: eng English ... tx 汉字 pinyin xt
            return f"eng {english_part} ... tx {chinese_part} xt"
        return full_match
    
    # Match xt ... tx (non-greedy, handles multiline)
    pattern = r'xt\s+(.+?)\s+tx'
    return re.sub(pattern, replacer, text, flags=re.DOTALL)

# Process all session files
session_files = sorted(glob.glob(os.path.join(INPUT_DIR, "fud&d-sesh*.txt")))

for input_file in session_files:
    filename = os.path.basename(input_file)
    output_file = os.path.join(OUTPUT_DIR, filename)
    
    print(f"Processing: {filename}")
    
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    reversed_content = reverse_xt(content)
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(reversed_content)
    
    print(f"  → Saved: {filename}")

print(f"\nDone! Processed {len(session_files)} files.")
