#!/usr/bin/env python3
"""
Add commas ONLY within eng...tx...xt patterns for pauses.
Does NOT touch surrounding English text.
"""

import re
import os

REVERSE_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/reverse_learn"

def add_commas_precise(text):
    # Only match: eng English ... tx 汉字 pinyin xt
    # Add commas: eng English, ... tx 汉字, pinyin, ... xt
    def replacer(match):
        full = match.group(0)
        
        # Split by " ... tx "
        if ' ... tx ' in full:
            parts = full.split(' ... tx ', 1)
            english_part = parts[0]  # "eng English"
            chinese_part = parts[1]  # "汉字 pinyin xt"
            
            # Add comma after English word
            eng_match = re.match(r'eng\s+(.+)', english_part)
            if eng_match:
                english_part = f"eng {eng_match.group(1)},"
            
            # Add commas in Chinese part: 汉字, pinyin, ... xt
            # Only split Chinese chars and pinyin, NOT the whole string
            chinese_match = re.match(r'([\u4e00-\u9fff]+)\s+([a-zāáǎàēéěèīíǐìōóǒòūúǔùǖǘǚǜ]+)\s+xt', chinese_part)
            if chinese_match:
                chars = chinese_match.group(1)
                pinyin = chinese_match.group(2)
                chinese_part = f"{chars}, {pinyin}, ... xt"
            else:
                # Multi-word pinyin like "shi jian shui jing"
                chinese_match2 = re.match(r'([\u4e00-\u9fff]+)\s+(.+?)\s+xt', chinese_part)
                if chinese_match2:
                    chars = chinese_match2.group(1)
                    pinyin = chinese_match2.group(2)
                    chinese_part = f"{chars}, {pinyin}, ... xt"
            
            return f"{english_part} ... tx {chinese_part}"
        return full
    
    # Match eng ... tx ... xt patterns precisely
    text = re.sub(r'eng\s+[a-zA-Z\s\-]+?\s*\.\.\.\s*tx\s+[\u4e00-\u9fff][^\n]*?xt', replacer, text)
    
    return text

# Process session 10
test_file = os.path.join(REVERSE_DIR, "fud&d-sesh10-belt-gym.txt")
output_file = os.path.join(REVERSE_DIR, "fud&d-sesh10-belt-gym-commas.txt")

print(f"Processing: fud&d-sesh10-belt-gym.txt")

with open(test_file, 'r', encoding='utf-8') as f:
    content = f.read()

comma_content = add_commas_precise(content)

with open(output_file, 'w', encoding='utf-8') as f:
    f.write(comma_content)

print(f"Saved: fud&d-sesh10-belt-gym-commas.txt")

# Show sample lines with translations
lines = comma_content.split('\n')
for i, line in enumerate(lines[:15]):
    if 'eng' in line and 'tx' in line and 'xt' in line:
        print(f"\nLine {i+1}:")
        print(f"  {line[:150]}...")
