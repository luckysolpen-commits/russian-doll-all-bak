#!/usr/bin/env python3
"""
Add commas for better pauses in reversed files.
Format: eng English, ... tx 汉字, pinyin, ... xt
"""

import re
import os
import glob

REVERSE_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/reverse_learn"

def add_commas(text):
    # Pattern: eng English ... tx 汉字 pinyin xt
    # Add commas: eng English, ... tx 汉字, pinyin, ... xt
    def replacer(match):
        full = match.group(0)
        # Split by " ... tx "
        if ' ... tx ' in full:
            parts = full.split(' ... tx ', 1)
            english_part = parts[0]  # "eng English"
            chinese_part = parts[1]  # "汉字 pinyin xt"
            
            # Add comma after English word
            english_part = english_part.replace('eng ', 'eng ', 1)
            # Find the English word and add comma
            eng_match = re.match(r'eng\s+(.+)', english_part)
            if eng_match:
                english_part = f"eng {eng_match.group(1)},"
            
            # Add commas in Chinese part: 汉字, pinyin, ... xt
            # Split Chinese part by spaces
            chinese_words = chinese_part.split()
            new_chinese = []
            for i, word in enumerate(chinese_words):
                if word == 'xt':
                    new_chinese.append('... xt')
                elif i == 0:  # Chinese characters
                    new_chinese.append(f"{word},")
                else:  # pinyin
                    new_chinese.append(f"{word},")
            
            chinese_part = ' '.join(new_chinese)
            
            return f"{english_part} ... tx {chinese_part}"
        return full
    
    # Match eng ... tx ... xt patterns
    text = re.sub(r'eng\s+[a-zA-Z\s\-]+?\s*\.\.\.\s*tx\s+[\u4e00-\u9fff][^\n]*?xt', replacer, text)
    
    return text

# Process just session 10 for testing
test_file = os.path.join(REVERSE_DIR, "fud&d-sesh10-belt-gym.txt")
output_file = os.path.join(REVERSE_DIR, "fud&d-sesh10-belt-gym-commas.txt")

print(f"Processing: fud&d-sesh10-belt-gym.txt")

with open(test_file, 'r', encoding='utf-8') as f:
    content = f.read()

comma_content = add_commas(content)

with open(output_file, 'w', encoding='utf-8') as f:
    f.write(comma_content)

print(f"Saved: fud&d-sesh10-belt-gym-commas.txt")

# Show sample
lines = comma_content.split('\n')
for i, line in enumerate(lines[:30]):
    if 'eng' in line and 'tx' in line:
        print(f"Line {i+1}: {line[:120]}...")
