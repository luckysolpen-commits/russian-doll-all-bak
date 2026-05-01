#!/usr/bin/env python3
"""
Convert Footrace.FU English chapters to audio using edge-tts with zh-CN-YunxiaNeural voice.
Skips chapters that already have output files.
"""

import os
import asyncio
import sys
import re

# Voice for the English text, as specified by the user.
VOICE = "zh-CN-YunxiaNeural"

# Path to the specific book, corrected based on available directory structure.
BOOK_DIR_REL = "SP.all-writeez28-b2/11.FOOTRACE.FU]smol/book_template.scp"
ROOT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+20"

# Output suffix to distinguish this conversion
OUTPUT_SUFFIX = "-yunxia-v1.0.mp3"

def clean_text_for_tts(text: str) -> str:
    """Clean text to reduce unnatural pauses in edge-tts."""
    if not text:
        return ""

    # Normalize whitespace: replace newline characters with spaces, then collapse multiple spaces.
    text = text.replace(chr(10), ' ') # Replace newline with space
    text = ' '.join(text.split())    # Collapse multiple spaces

    return text.strip()

async def convert_chapter(ch_path, ch_num, total, book_prefix, text_en):
    """Convert English text to audio using the specified voice."""

    output_file_en = os.path.join(ch_path, f"{book_prefix}.chapter{ch_num:02d}{OUTPUT_SUFFIX}")

    if not text_en:
        print(f"[{ch_num:>2}/{total}] SKIP (no English text): chapter_{ch_num}")
        return "skip"

    if os.path.exists(output_file_en):
        size = os.path.getsize(output_file_en) / (1024 * 1024)
        print(f"[{ch_num:>2}/{total}] SKIP (exists, EN): {os.path.basename(output_file_en)} ({size:.1f} MB)")
        return "skip"

    print(f"[{ch_num:>2}/{total}] Converting chapter_{ch_num} (English, {len(text_en)} chars) with {VOICE}...")

    cleaned_text = clean_text_for_tts(text_en)
    if not cleaned_text:
        print(f"[{ch_num:>2}/{total}] SKIP (empty after cleaning): chapter_{ch_num}")
        return "skip"

    proc = await asyncio.create_subprocess_exec(
        'edge-tts',
        '--voice', VOICE,
        '--text', cleaned_text,
        '--write-media', output_file_en,
        stdout=asyncio.subprocess.DEVNULL,
        stderr=asyncio.subprocess.DEVNULL,
    )
    await proc.wait()

    if os.path.exists(output_file_en) and os.path.getsize(output_file_en) > 0:
        size = os.path.getsize(output_file_en) / (1024 * 1024)
        print(f"[{ch_num:>2}/{total}] DONE (EN): {os.path.basename(output_file_en)} ({size:.1f} MB)")
        return "done"
    else:
        print(f"[{ch_num:>2}/{total}] FAILED (EN): chapter_{ch_num}")
        return "fail"

async def main():
    book_path = os.path.join(ROOT_DIR, BOOK_DIR_REL)

    if not os.path.isdir(book_path):
        print(f"ERROR: Book directory not found: {book_path}")
        sys.exit(1)

    # Find all chapter directories (e.g., 'chapter_1', 'chapter_2', etc.)
    chapters_dirs = sorted(
        [d for d in os.listdir(book_path) if d.startswith("chapter_") and os.path.isdir(os.path.join(book_path, d))],
        key=lambda x: int(x.split("_")[1])
    )

    total_chapters = len(chapters_dirs)
    if not total_chapters:
        print(f"No chapter directories found in {book_path}.")
        sys.exit(1)

    print(f"Found {total_chapters} chapters. Starting conversion for FOOTRACE book (English only, Yunxia voice)...")

    overall_done = 0
    overall_skipped = 0
    overall_failed = 0

    # Extract clean book prefix from the directory name for output file names
    try:
        name_part = os.path.basename(BOOK_DIR_REL)
        if '.' in name_part:
            name_part = name_part.split('.')[0]
        name_part = name_part.split(']')[0].split('(')[0].split('[')[0]
        book_prefix_part = name_part.replace(' ', '_').lower()
        if not book_prefix_part: book_prefix_part = "footrace_en"
    except Exception:
        book_prefix_part = "footrace_en" # Generic fallback

    for i, ch_dir_name in enumerate(chapters_dirs):
        try:
            ch_num = int(ch_dir_name.split("_")[1])
        except (ValueError, IndexError):
            print(f"[{i+1:>2}/{total_chapters}] SKIP (invalid chapter number): {ch_dir_name}")
            overall_skipped += 1
            continue
        
        ch_path = os.path.join(book_path, ch_dir_name)
        
        text_en = ""
        
        # Read English text file
        txt_file_en = os.path.join(ch_path, f"chapter_{ch_num}.txt")
        if os.path.exists(txt_file_en):
            with open(txt_file_en, 'r', encoding='utf-8', errors='replace') as f:
                text_en = f.read().strip()
        else:
            print(f"[{ch_num:>2}/{total_chapters}] INFO: No English text file found: {os.path.basename(txt_file_en)}")

        if not text_en:
            print(f"[{ch_num:>2}/{total_chapters}] SKIP (no English text found for chapter {ch_num}).")
            overall_skipped += 1
            continue

        # Convert English
        result = await convert_chapter(ch_path, ch_num, total_chapters, book_prefix_part, text_en)

        if result == "done":
            overall_done += 1
        elif result == "skip":
            overall_skipped += 1
        else:
            overall_failed += 1
            
        await asyncio.sleep(0.8)  # polite delay between chapters

    print("
" + "="*60)
    print("BATCH CONVERSION COMPLETE - FOOTRACE BOOK (ENGLISH ONLY):")
    print(f"Total Chapters Processed: {total_chapters}")
    print(f"Converted : {overall_done:3d} | Skipped : {overall_skipped:3d} | Failed : {overall_failed:3d}")
    print("="*60)

if __name__ == '__main__':
    asyncio.run(main())
