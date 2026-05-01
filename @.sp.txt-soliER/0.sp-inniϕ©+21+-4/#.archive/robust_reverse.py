#!/usr/bin/env python3
"""
Robust reversal handling all format types:
1. xt 汉字 pinyin ... eng English ... tx → , eng English, ... tx 汉字, pinyin, ... xt,
2. 汉字 pinyin... English... tx → , eng English, ... tx 汉字, pinyin, ... xt,
3. 汉字 pinyin ... eng English ... tx → , eng English, ... tx 汉字, pinyin, ... xt,
Leaves normal Chinese text (speaker names, etc.) alone.
"""

import re
import os
import glob

INPUT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/fud&d"
OUTPUT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/reverse_learn"

def reverse_robust(text):
    # Pattern 1: xt 汉字 pinyin ... eng English ... tx
    def replacer1(match):
        inner = match.group(1)
        if ' ... eng ' in inner and ' ... tx' in inner:
            parts = inner.split(' ... eng ', 1)
            chinese = parts[0].strip()
            english = parts[1].replace(' ... tx', '').strip()
            if chinese and english and len(english) > 1:
                return f", eng {english}, ... tx {chinese}, ... xt,"
        return match.group(0)
    
    # Pattern 2: 汉字 pinyin... English... tx (no eng marker)
    def replacer2(match):
        full = match.group(0)
        parts = full.split('...')
        if len(parts) >= 3:
            chinese_pinyin = parts[0].strip()
            english = parts[1].strip()
            # Remove 'tx' from english
            english = english.replace('tx', '').strip()
            if chinese_pinyin and english and len(english) > 1:
                # Split chinese and pinyin
                cp_match = re.match(r'([\u4e00-\u9fff]+)\s*([a-zāáǎàēéěèīíǐìōóǒòūúǔùǖǘǚǜ\s]+)', chinese_pinyin)
                if cp_match:
                    chinese = cp_match.group(1)
                    pinyin = cp_match.group(2).strip()
                    return f", eng {english}, ... tx {chinese}, {pinyin}, ... xt,"
        return full
    
    # Pattern 3: 汉字 pinyin ... eng English ... tx (no xt marker)
    def replacer3(match):
        full = match.group(0)
        if ' ... eng ' in full and ' ... tx' in full:
            parts = full.split(' ... eng ', 1)
            chinese = parts[0].strip()
            english = parts[1].replace(' ... tx', '').strip()
            if chinese and english and len(english) > 1:
                return f", eng {english}, ... tx {chinese}, ... xt,"
        return full
    
    # Apply pattern 1 first (xt...tx)
    text = re.sub(r'xt\s+(.+?)\s+tx', replacer1, text, flags=re.DOTALL)
    
    # Apply pattern 3 (汉字... eng English... tx)
    text = re.sub(r'([\u4e00-\u9fff][^\n]*?[a-zāáǎàēéěèīíǐìōóǒòūúǔùǖǘǚǜ]+)\s*\.\.\.\s*eng\s+([^\n]*?)\s*\.\.\.\s*tx', replacer3, text)
    
    # Apply pattern 2 (汉字 pinyin... English... tx)
    text = re.sub(r'([\u4e00-\u9fff]+[a-zāáǎàēéěèīíǐìōóǒòūúǔùǖǘǚǜ\s]*)\.\.\.\s*([a-zA-Z\s]+)\.\.\.\s*tx', replacer2, text)
    
    # Clean up multiple commas and spaces
    text = re.sub(r',\s*,+', ',', text)
    text = re.sub(r',\s*\.\.\.', ' ...', text)
    text = re.sub(r'\s+,', ',', text)
    
    return text

def validate_file(content, filename):
    """Check for common issues."""
    issues = []
    
    # Check for unpaired markers
    xt_count = len(re.findall(r'\bxt\b', content))
    tx_count = len(re.findall(r'\btx\b', content))
    if xt_count != tx_count:
        issues.append(f"Unpaired: xt={xt_count}, tx={tx_count}")
    
    return issues

# Process all session files
session_files = sorted(glob.glob(os.path.join(INPUT_DIR, "fud&d-sesh*.txt")))

for input_file in session_files:
    filename = os.path.basename(input_file)
    output_file = os.path.join(OUTPUT_DIR, filename)
    
    print(f"\nProcessing: {filename}")
    
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    reversed_content = reverse_robust(content)
    
    # Validate
    issues = validate_file(reversed_content, filename)
    if issues:
        print(f"  ⚠️  {', '.join(issues)}")
    else:
        print(f"  ✅ Clean")
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(reversed_content)
    
    # Count translations
    trans_count = len(re.findall(r',\s*eng\s+[a-zA-Z]', reversed_content))
    print(f"  → Translations: {trans_count}")

print(f"\n{'='*60}")
print("Done!")
