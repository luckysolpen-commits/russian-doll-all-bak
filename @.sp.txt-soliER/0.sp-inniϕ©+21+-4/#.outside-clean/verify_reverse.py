#!/usr/bin/env python3
"""
Verify all reversed files are in correct format.
Check for:
1. Old format patterns still present (Chinese first, then English)
2. Mangled text from regex failures
3. Count of reversed vs unreversed patterns
"""

import re
import os
import glob

REVERSE_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/reverse_learn"

def check_file(filepath):
    filename = os.path.basename(filepath)
    
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Count reversed patterns: eng English ... tx 汉字 pinyin xt
    reversed_patterns = re.findall(r'eng\s+[a-zA-Z\s\-]+?\s*\.\.\.\s*tx\s+[\u4e00-\u9fff]', content)
    
    # Count old patterns: 汉字 pinyin ... eng English ... tx (without eng...tx prefix)
    old_patterns = re.findall(r'[\u4e00-\u9fff][a-zāáǎàēéěèīíǐìōóǒòūúǔùǖǘǚǜ\s]+?\s*\.\.\.\s*eng\s+[a-zA-Z]', content)
    
    # Count xt...tx patterns that weren't reversed
    unreversed_xt = re.findall(r'xt\s+[\u4e00-\u9fff]', content)
    
    # Check for mangled patterns (eng xt, xt eng, etc.)
    mangled = re.findall(r'eng\s+xt|xt\s+eng|eng\s+eng\s+tx', content)
    
    issues = []
    if old_patterns:
        issues.append(f"OLD FORMAT: {len(old_patterns)} instances")
    if unreversed_xt:
        issues.append(f"UNREVERSED xt: {len(unreversed_xt)} instances")
    if mangled:
        issues.append(f"MANGLED: {len(mangled)} instances")
    
    status = "OK" if not issues else "ISSUES"
    
    print(f"\n{filename}: {status}")
    print(f"  Reversed: {len(reversed_patterns)}")
    if issues:
        for issue in issues:
            print(f"  ⚠️  {issue}")
    
    # Show first 3 old patterns if any
    if old_patterns:
        print(f"  Examples of old format:")
        for p in old_patterns[:3]:
            print(f"    → {p[:80]}...")
    
    # Show first 3 mangled if any
    if mangled:
        print(f"  Examples of mangled:")
        for p in mangled[:3]:
            print(f"    → {p}")
    
    return status == "OK"

# Check all files
session_files = sorted(glob.glob(os.path.join(REVERSE_DIR, "fud&d-sesh*.txt")))

print(f"Checking {len(session_files)} files...\n")

all_ok = True
for f in session_files:
    if not check_file(f):
        all_ok = False

print(f"\n{'='*60}")
if all_ok:
    print("ALL FILES OK - Ready for audio conversion!")
else:
    print("SOME FILES HAVE ISSUES - Need manual fixing before audio conversion")
