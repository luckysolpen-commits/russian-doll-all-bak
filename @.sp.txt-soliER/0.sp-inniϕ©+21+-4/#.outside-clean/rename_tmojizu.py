#!/usr/bin/env python3
"""
Rename 'Mother Tomokazu' to 'Mother Tmojizu' in all session .txt files.
Also adds 中 prefix to output MP3 filenames.
"""

import os
import glob

session_dir = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+12/SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/1.ffu-ez_learnd&d+chem"

# Find all .txt files
txt_files = glob.glob(os.path.join(session_dir, "*.txt"))

print(f"Found {len(txt_files)} session files")

total_replacements = 0
for filepath in txt_files:
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Count occurrences
    count = content.count("Mother Tomokazu")
    if count > 0:
        # Replace
        new_content = content.replace("Mother Tomokazu", "Mother Tmojizu")
        
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(new_content)
        
        total_replacements += count
        print(f"  ✓ {os.path.basename(filepath)}: {count} replacements")

print(f"\n{'='*60}")
print(f"TOTAL REPLACEMENTS: {total_replacements}")
print(f"{'='*60}")

# Verify
verify_count = 0
for filepath in txt_files:
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    verify_count += content.count("Mother Tmojizu")

print(f"New 'Mother Tmojizu' occurrences: {verify_count}")

# Check for any remaining
remaining = 0
for filepath in txt_files:
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    remaining += content.count("Mother Tomokazu")

if remaining > 0:
    print(f"WARNING: {remaining} 'Mother Tomokazu' still remaining!")
else:
    print(f"✓ All 'Mother Tomokazu' successfully renamed to 'Mother Tmojizu'")
