#!/usr/bin/env python3
"""Number the Footrace Chinese audio files in story order."""
import os, re

AUDIO_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5/SP.all-writeez28-a1/11.FOOTRACE.FU]smol/chinese-learning/audio"

# Map chapter names to their order number
ORDER = {
    "前言": 0,
    "第一章": 1,
    "第二章": 2,
    "第三章": 3,
    "第四章": 4,
    "第五章": 5,
    "第六章": 6,
    "第七章": 7,
    "第八章": 8,
    "第九章": 9,
    "第十章": 10,
    "第十一章": 11,
    "第十二章": 12,
    "第十三章": 13,
}

for fname in sorted(os.listdir(AUDIO_DIR)):
    if not fname.endswith('.mp3'):
        continue
    # Find which chapter this is
    num = None
    for key, val in ORDER.items():
        if key in fname:
            num = val
            break
    if num is None:
        print(f"SKIP (unknown): {fname}")
        continue

    new_name = f"{num:02d}_{fname}"
    old_path = os.path.join(AUDIO_DIR, fname)
    new_path = os.path.join(AUDIO_DIR, new_name)
    os.rename(old_path, new_path)
    print(f"  {fname} → {new_name}")

print("\nDone! Files are now numbered 00-13.")
