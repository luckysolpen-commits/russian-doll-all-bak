#!/usr/bin/env python3
"""
Batch convert Sol Pen Book 0 chapters to audio using edge-tts (single voice).
Skips chapters that already have output files.
Uses en-US-JennyNeural (narrator voice, warm/friendly).
"""

import os
import asyncio
import sys

BOOK_DIR = "SP.all-writeez28-a1/0.Sol Pen]bk0/#.book_SP_0.full+aud-b1"
VOICE = "en-US-JennyNeural"
OUTPUT_PREFIX = "sp.chapter"
OUTPUT_SUFFIX = "-v1.0.mp3"

async def convert_chapter(txt_path, output_path, ch_num, total):
    """Convert a single chapter using edge-tts."""
    # Read the text
    with open(txt_path, 'r', encoding='utf-8') as f:
        text = f.read().strip()

    if not text:
        print(f"[{ch_num:>2}/{total}] SKIP (empty): chapter_{ch_num}")
        return "skip"

    # Check if output already exists
    if os.path.exists(output_path):
        size = os.path.getsize(output_path) / (1024 * 1024)
        print(f"[{ch_num:>2}/{total}] SKIP (exists): {OUTPUT_PREFIX}{ch_num}{OUTPUT_SUFFIX} ({size:.1f} MB)")
        return "skip"

    print(f"[{ch_num:>2}/{total}] Converting chapter_{ch_num} ({len(text)} chars)...")

    proc = await asyncio.create_subprocess_exec(
        'edge-tts',
        '--voice', VOICE,
        '--text', text,
        '--write-media', output_path,
        stdout=asyncio.subprocess.DEVNULL,
        stderr=asyncio.subprocess.DEVNULL,
    )
    await proc.wait()

    if os.path.exists(output_path) and os.path.getsize(output_path) > 0:
        size = os.path.getsize(output_path) / (1024 * 1024)
        print(f"[{ch_num:>2}/{total}] DONE: {OUTPUT_PREFIX}{ch_num}{OUTPUT_SUFFIX} ({size:.1f} MB)")
        return "done"
    else:
        print(f"[{ch_num:>2}/{total}] FAILED: chapter_{ch_num}")
        return "fail"

async def main():
    root = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5"
    book_path = os.path.join(root, BOOK_DIR)

    if not os.path.isdir(book_path):
        print(f"ERROR: Book dir not found: {book_path}")
        sys.exit(1)

    # Find all chapter directories
    chapters = sorted(
        [d for d in os.listdir(book_path) if d.startswith("chapter_")],
        key=lambda x: int(x.split("_")[1])
    )

    total = len(chapters)
    done = 0
    skipped = 0
    failed = 0

    print(f"Found {total} chapters. Starting conversion with edge-tts ({VOICE})...\n")

    for ch_dir in chapters:
        ch_num = int(ch_dir.split("_")[1])
        ch_path = os.path.join(book_path, ch_dir)
        txt_file = os.path.join(ch_path, f"chapter_{ch_num}.txt")
        output_file = os.path.join(ch_path, f"{OUTPUT_PREFIX}{ch_num}{OUTPUT_SUFFIX}")

        if not os.path.exists(txt_file):
            print(f"[{ch_num:>2}/{total}] SKIP (no text): chapter_{ch_num}")
            skipped += 1
            continue

        result = await convert_chapter(txt_file, output_file, ch_num, total)

        if result == "done":
            done += 1
        elif result == "skip":
            skipped += 1
        else:
            failed += 1

        # Small delay to be safe
        await asyncio.sleep(1)

    print(f"\n=== DONE ===")
    print(f"Converted: {done} | Skipped: {skipped} | Failed: {failed}")

if __name__ == '__main__':
    asyncio.run(main())
