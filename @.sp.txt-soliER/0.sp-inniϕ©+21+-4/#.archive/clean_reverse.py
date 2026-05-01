#!/usr/bin/env python3
"""
Clean reversal from ORIGINAL files.
Pattern 1: xt 汉字 pinyin ... eng English ... tx → eng English ... tx 汉字 pinyin xt
Pattern 2: 汉字 pinyin ... eng English ... tx → eng English ... tx 汉字 pinyin xt
"""

import re
import os
import glob

INPUT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/fud&d"
OUTPUT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/reverse_learn"

def reverse_clean(text):
    # Pattern 1: xt 汉字 pinyin ... eng English ... tx
    def replacer1(match):
        inner = match.group(1)
        if ' ... eng ' in inner and ' ... tx' in inner:
            # Split by " ... eng "
            parts = inner.split(' ... eng ', 1)
            chinese = parts[0].strip()
            english = parts[1].replace(' ... tx', '').strip()
            if chinese and english:
                return f"eng {english} ... tx {chinese} xt"
        return match.group(0)
    
    # Pattern 2: 汉字 pinyin ... eng English ... tx (no xt marker)
    def replacer2(match):
        full = match.group(0)
        if ' ... eng ' in full and ' ... tx' in full:
            parts = full.split(' ... eng ', 1)
            chinese = parts[0].strip()
            english = parts[1].replace(' ... tx', '').strip()
            if chinese and english:
                return f"eng {english} ... tx {chinese} xt"
        return full
    
    # Apply pattern 1 first
    text = re.sub(r'xt\s+(.+?)\s+tx', replacer1, text, flags=re.DOTALL)
    
    # Apply pattern 2 - match Chinese chars + pinyin ... eng English ... tx
    text = re.sub(r'([\u4e00-\u9fff][^\n]*?[a-zāáǎàēéěèīíǐìōóǒòūúǔùǖǘǚǜ]+)\s*\.\.\.\s*eng\s+([^\n]*?)\s*\.\.\.\s*tx', replacer2, text)
    
    return text

# Process all session files
session_files = sorted(glob.glob(os.path.join(INPUT_DIR, "fud&d-sesh*.txt")))

for input_file in session_files:
    filename = os.path.basename(input_file)
    output_file = os.path.join(OUTPUT_DIR, filename)
    
    print(f"Processing: {filename}")
    
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    reversed_content = reverse_clean(content)
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(reversed_content)
    
    # Quick check
    old_count = len(re.findall(r'[\u4e00-\u9fff][^\n]*?\.\.\.\s*eng\s+[a-zA-Z]', reversed_content))
    new_count = len(re.findall(r'eng\s+[a-zA-Z].*?\.\.\.\s*tx\s+[\u4e00-\u9fff]', reversed_content))
    print(f"  → Reversed: {new_count}, Still old: {old_count}")

print(f"\nDone!")
