#!/usr/bin/env python3
"""
Batch convert Centroid chapters to audio using edge-tts with zh-CN-YunxiaNeural voice.
Each chapter has multiple page_N.txt files — combines them before conversion.
Skips chapters that already have output files.
"""

import os
import asyncio
import sys
import re

# Voice
VOICE = "en-US-AvaNeural"

# Paths
BOOK_DIR_REL = "SP.all-writeez28-b2/6.hellride-bk0]BIG]b1/book_template.scp"
ROOT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+20"

# Output settings
OUTPUT_PREFIX = "ava"
OUTPUT_SUFFIX = "-hel-v1.0.mp3"


def clean_text_for_tts(text: str) -> str:
    """Clean text to reduce unnatural long pauses while preserving paragraph breaks."""
    if not text:
        return ""

    # Replace multiple newlines with double newline (edge-tts treats \n\n as pause)
    text = re.sub(r'\n\s*\n+', '\n\n', text)
    
    # Normalize spaces within lines
    lines = []
    for line in text.split('\n'):
        cleaned_line = ' '.join(line.split())
        lines.append(cleaned_line)
    
    # Rejoin with double newline for natural pauses between paragraphs
    text = '\n\n'.join(lines)
    
    return text.strip()


async def convert_chapter(ch_dir: str, output_path: str, ch_num: int, total: int):
    """Combine all page_N.txt files in the chapter and convert to speech."""
    
    # Find and sort page files
    page_files = sorted(
        [f for f in os.listdir(ch_dir) if f.startswith("page_") and f.endswith(".txt")],
        key=lambda x: int(x.split("_")[1].split(".")[0])
    )

    if not page_files:
        print(f"[{ch_num:>2}/{total}] SKIP (no pages): chapter_{ch_num}")
        return "skip"

    # Combine all pages
    combined_text_parts = []
    for pf in page_files:
        pf_path = os.path.join(ch_dir, pf)
        with open(pf_path, 'r', encoding='utf-8', errors='replace') as f:
            combined_text_parts.append(f.read().strip())

    # Join with double newlines to create natural pauses between pages/paragraphs
    raw_text = '\n\n'.join(combined_text_parts)
    cleaned_text = clean_text_for_tts(raw_text)

    if not cleaned_text:
        print(f"[{ch_num:>2}/{total}] SKIP (empty after cleaning): chapter_{ch_num}")
        return "skip"

    # Skip if output file already exists
    if os.path.exists(output_path):
        size = os.path.getsize(output_path) / (1024 * 1024)
        print(f"[{ch_num:>2}/{total}] SKIP (exists): {os.path.basename(output_path)} ({size:.1f} MB)")
        return "skip"

    print(f"[{ch_num:>2}/{total}] Converting chapter_{ch_num} ({len(page_files)} pages, {len(cleaned_text)} chars)...")

    proc = await asyncio.create_subprocess_exec(
        'edge-tts',
        '--voice', VOICE,
        '--text', cleaned_text,
        '--write-media', output_path,
        stdout=asyncio.subprocess.DEVNULL,
        stderr=asyncio.subprocess.DEVNULL,
    )
    await proc.wait()

    if os.path.exists(output_path) and os.path.getsize(output_path) > 0:
        size = os.path.getsize(output_path) / (1024 * 1024)
        print(f"[{ch_num:>2}/{total}] DONE: {os.path.basename(output_path)} ({size:.1f} MB)")
        return "done"
    else:
        print(f"[{ch_num:>2}/{total}] FAILED: chapter_{ch_num}")
        return "fail"


async def main():
    book_path = os.path.join(ROOT_DIR, BOOK_DIR_REL)

    if not os.path.isdir(book_path):
        print(f"ERROR: Book directory not found: {book_path}")
        sys.exit(1)

    # Find all chapter directories
    chapters_dirs = sorted(
        [d for d in os.listdir(book_path) if d.startswith("chapter_") and os.path.isdir(os.path.join(book_path, d))],
        key=lambda x: int(x.split("_")[1])
    )

    total_chapters = len(chapters_dirs)
    if not total_chapters:
        print(f"No chapter directories found in {book_path}.")
        sys.exit(1)

    print(f"Found {total_chapters} chapters. Starting conversion with {VOICE}...")

    overall_done = 0
    overall_skipped = 0
    overall_failed = 0

    for i, ch_dir_name in enumerate(chapters_dirs):
        try:
            ch_num = int(ch_dir_name.split("_")[1])
        except (ValueError, IndexError):
            print(f"[{i+1:>2}/{total_chapters}] SKIP (invalid chapter name): {ch_dir_name}")
            overall_skipped += 1
            continue

        ch_path = os.path.join(book_path, ch_dir_name)

        # Build output filename
        output_filename = f"{OUTPUT_PREFIX}.chapter{ch_num:02d}{OUTPUT_SUFFIX}"
        output_path = os.path.join(ch_path, output_filename)

        # Perform conversion
        result = await convert_chapter(ch_path, output_path, ch_num, total_chapters)

        if result == "done":
            overall_done += 1
        elif result == "skip":
            overall_skipped += 1
        else:
            overall_failed += 1

        await asyncio.sleep(0.8)  # Polite delay between chapters

    print("\n" + "=" * 70)
    print("BATCH CONVERSION COMPLETE - hell BOOK")
    print(f"Total Chapters Processed: {total_chapters}")
    print(f"Converted : {overall_done:3d} | Skipped : {overall_skipped:3d} | Failed : {overall_failed:3d}")
    print("=" * 70)


if __name__ == '__main__':
    asyncio.run(main())
