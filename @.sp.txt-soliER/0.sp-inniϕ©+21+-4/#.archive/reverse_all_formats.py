#!/usr/bin/env python3
"""
Reverse the translation format in FU D&D session files.
Format 1 (Sessions 10-13): xt 汉字 pinyin ... eng English ... tx
  → eng English ... tx 汉字 pinyin xt
Format 2 (Sessions 1-9): 汉字 pinyin... English... tx
  → eng English... tx 汉字 pinyin xt
"""

import re
import os
import glob

INPUT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/fud&d"
OUTPUT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/reverse_learn"

def reverse_format1(text):
    """Reverse xt 汉字 pinyin ... eng English ... tx → eng English ... tx 汉字 pinyin xt"""
    def replacer(match):
        inner = match.group(1)
        if ' eng ' in inner:
            parts = inner.split(' eng ', 1)
            chinese_part = parts[0].strip()
            english_part = parts[1].strip()
            if english_part.endswith(' ... tx'):
                english_part = english_part[:-6].strip()
            return f"eng {english_part} ... tx {chinese_part} xt"
        return match.group(0)
    
    pattern = r'xt\s+(.+?)\s+tx'
    return re.sub(pattern, replacer, text)

def reverse_format2(text):
    """Reverse 汉字 pinyin... English... tx → eng English... tx 汉字 pinyin xt"""
    def replacer(match):
        inner = match.group(1)
        # Split by "..." to find English part
        parts = inner.split('...')
        if len(parts) >= 2:
            # First part is Chinese + pinyin, last part is English
            chinese_part = parts[0].strip()
            english_part = parts[-1].strip()
            if english_part == 'tx':
                english_part = parts[-2].strip() if len(parts) > 2 else 'unknown'
            # Reconstruct middle parts if any
            middle = '...'.join(parts[1:-1]).strip() if len(parts) > 2 else ''
            if middle and middle != 'tx':
                english_part = f"{english_part}...{middle}"
            return f"eng {english_part.strip()}... tx {chinese_part} xt"
        return match.group(0)
    
    # Match patterns like: 火车 huǒ chē... train... tx
    pattern = r'([\u4e00-\u9fff][^\n]*?[a-zāáǎàēéěèīíǐìōóǒòūúǔùǖǘǚǜ]+)\.\.\.\s*([a-zA-Z]+)\.\.\.\s*tx'
    return re.sub(pattern, replacer, text)

# Process all session files
session_files = sorted(glob.glob(os.path.join(INPUT_DIR, "fud&d-sesh*.txt")))

for input_file in session_files:
    filename = os.path.basename(input_file)
    output_file = os.path.join(OUTPUT_DIR, filename)
    
    print(f"Processing: {filename}")
    
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Try format 1 first (xt...tx), then format 2
    if 'xt ' in content:
        reversed_content = reverse_format1(content)
        print(f"  → Format 1 (xt...tx)")
    else:
        reversed_content = reverse_format2(content)
        print(f"  → Format 2 (汉字...English...tx)")
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(reversed_content)
    
    print(f"  → Saved: {filename}")

print(f"\nDone! Processed {len(session_files)} files.")
