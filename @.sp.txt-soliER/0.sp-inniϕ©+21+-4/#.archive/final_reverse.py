#!/usr/bin/env python3
"""
Properly reverse ALL translation formats. Handles nested patterns correctly.
"""

import re
import os
import glob

INPUT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/fud&d"
OUTPUT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/reverse_learn"

def reverse_all(text):
    # Pattern 1: xt 汉字 pinyin ... eng English ... tx
    def replacer1(match):
        inner = match.group(1)
        if ' eng ' in inner:
            parts = inner.split(' eng ', 1)
            chinese = parts[0].strip()
            english = parts[1].replace(' ... tx', '').strip()
            return f"eng {english} ... tx {chinese} xt"
        return match.group(0)
    
    # Pattern 2: 汉字 pinyin ... eng English ... tx (no xt marker)
    def replacer2(match):
        full = match.group(0)
        if ' eng ' in full:
            parts = full.split(' eng ', 1)
            chinese = parts[0].strip()
            english = parts[1].replace(' ... tx', '').strip()
            return f"eng {english} ... tx {chinese} xt"
        return full
    
    # Pattern 3: 汉字 pinyin... English... tx (no eng marker)
    def replacer3(match):
        full = match.group(0)
        parts = full.split('...')
        if len(parts) >= 2:
            chinese = parts[0].strip()
            english = parts[1].strip().replace('tx', '').strip()
            if english and chinese:
                return f"eng {english}... tx {chinese} xt"
        return full
    
    # Apply pattern 1 first (xt...tx)
    text = re.sub(r'xt\s+(.+?)\s+tx', replacer1, text, flags=re.DOTALL)
    
    # Apply pattern 2 (汉字... eng English... tx) - more precise regex
    text = re.sub(r'([\u4e00-\u9fff][^\n]*?[a-zāáǎàēéěèīíǐìōóǒòūúǔùǖǘǚǜ]+)\s*\.\.\.\s*eng\s*([^\n]*?)\s*\.\.\.\s*tx', replacer2, text)
    
    # Apply pattern 3 (汉字 pinyin... English... tx)
    text = re.sub(r'([\u4e00-\u9fff][a-zāáǎàēéěèīíǐìōóǒòūúǔùǖǘǚǜ\s]+)\.\.\.\s*([a-zA-Z\s]+)\.\.\.\s*tx', replacer3, text)
    
    # Fix double dots
    text = text.replace('... ...', '...')
    
    return text

# Process all session files
session_files = sorted(glob.glob(os.path.join(INPUT_DIR, "fud&d-sesh*.txt")))

for input_file in session_files:
    filename = os.path.basename(input_file)
    output_file = os.path.join(OUTPUT_DIR, filename)
    
    print(f"Processing: {filename}")
    
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    reversed_content = reverse_all(content)
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(reversed_content)
    
    print(f"  → Saved: {filename}")

print(f"\nDone! Processed {len(session_files)} files.")
