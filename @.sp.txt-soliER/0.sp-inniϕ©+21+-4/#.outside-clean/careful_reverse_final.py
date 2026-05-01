#!/usr/bin/env python3
"""
Carefully reverse ALL translation patterns from ORIGINAL files.
Add commas for pauses: , eng English, ... tx 汉字, pinyin, ... xt,
Fix all edge cases: unpaired markers, Chinese without English, etc.
"""

import re
import os
import glob

INPUT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/fud&d"
OUTPUT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/reverse_learn"

def reverse_and_pause(text):
    """
    Reverse translation patterns and add commas for pauses.
    Format: , eng English, ... tx 汉字, pinyin, ... xt,
    """
    
    # Pattern 1: xt 汉字 pinyin ... eng English ... tx
    def replacer1(match):
        inner = match.group(1)
        if ' ... eng ' in inner and ' ... tx' in inner:
            parts = inner.split(' ... eng ', 1)
            chinese = parts[0].strip()
            english = parts[1].replace(' ... tx', '').strip()
            if chinese and english:
                return f", eng {english}, ... tx {chinese}, ... xt,"
        return match.group(0)
    
    # Pattern 2: 汉字 pinyin ... eng English ... tx (no xt marker)
    def replacer2(match):
        full = match.group(0)
        if ' ... eng ' in full and ' ... tx' in full:
            parts = full.split(' ... eng ', 1)
            chinese = parts[0].strip()
            english = parts[1].replace(' ... tx', '').strip()
            if chinese and english:
                return f", eng {english}, ... tx {chinese}, ... xt,"
        return full
    
    # Apply pattern 1 first (xt...tx)
    text = re.sub(r'xt\s+(.+?)\s+tx', replacer1, text, flags=re.DOTALL)
    
    # Apply pattern 2 (汉字... eng English... tx)
    text = re.sub(r'([\u4e00-\u9fff][^\n]*?[a-zāáǎàēéěèīíǐìōóǒòūúǔùǖǘǚǜ]+)\s*\.\.\.\s*eng\s+([^\n]*?)\s*\.\.\.\s*tx', replacer2, text)
    
    # Fix any remaining orphan xt or tx markers
    text = text.replace(' xt ', ' , xt, ')
    text = text.replace(' tx ', ' , tx, ')
    
    # Clean up multiple commas
    text = re.sub(r',\s*,+', ',', text)
    text = re.sub(r',\s*\.\.\.', ' ...', text)
    
    return text

def validate_file(content, filename):
    """Check for common issues."""
    issues = []
    
    # Check for unpaired markers
    xt_count = len(re.findall(r'\bxt\b', content))
    tx_count = len(re.findall(r'\btx\b', content))
    if xt_count != tx_count:
        issues.append(f"Unpaired markers: xt={xt_count}, tx={tx_count}")
    
    # Check for Chinese without English
    chinese_no_eng = re.findall(r'[\u4e00-\u9fff]{2,}[^,]*?(?=\s|$)', content)
    if chinese_no_eng:
        # Filter out speaker names and normal text
        real_issues = [c for c in chinese_no_eng if len(c) > 1 and c not in ['中文老师', '师父']]
        if real_issues:
            issues.append(f"Chinese without English: {real_issues[:3]}")
    
    return issues

# Process all session files
session_files = sorted(glob.glob(os.path.join(INPUT_DIR, "fud&d-sesh*.txt")))

for input_file in session_files:
    filename = os.path.basename(input_file)
    output_file = os.path.join(OUTPUT_DIR, filename)
    
    print(f"\nProcessing: {filename}")
    
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    reversed_content = reverse_and_pause(content)
    
    # Validate
    issues = validate_file(reversed_content, filename)
    if issues:
        print(f"  ⚠️  Issues found:")
        for issue in issues:
            print(f"    - {issue}")
    else:
        print(f"  ✅ Clean")
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(reversed_content)
    
    # Count translations
    trans_count = len(re.findall(r'eng\s+[a-zA-Z]', reversed_content))
    print(f"  → Translations: {trans_count}")

print(f"\n{'='*60}")
print("Done! All files processed.")
