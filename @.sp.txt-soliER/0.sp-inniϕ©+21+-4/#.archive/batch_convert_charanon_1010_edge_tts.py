#!/usr/bin/env python3
"""
Batch convert CHAR.A.NON Book 1010% chapters to audio using edge-tts.
Each chapter has multiple page_N.txt files — combines them before conversion.
Uses en-US-AriaNeural (CHARA voice — female, confident).
"""

import os
import asyncio
import sys

BOOK_DIR = "SP.all-writeez28-a1/-1.MyChara+21]b1/CHAR.A.NON]c3]++/-1.CHAR.A.NON]1111111111111011]a0/BOOK]1010%"
VOICE = "en-US-AriaNeural"
OUTPUT_PREFIX = "ca1010.chapter"
OUTPUT_SUFFIX = "-v1.0.mp3"

async def convert_chapter(ch_dir, output_path, ch_num, total):
    page_files = sorted(
        [f for f in os.listdir(ch_dir) if f.startswith("page_") and f.endswith(".txt")],
        key=lambda x: int(x.split("_")[1].split(".")[0])
    )

    if not page_files:
        print(f"[{ch_num:>2}/{total}] SKIP (no pages): Chapter_{ch_num}")
        return "skip"

    combined_text = []
    for pf in page_files:
        pf_path = os.path.join(ch_dir, pf)
        with open(pf_path, 'r', encoding='utf-8', errors='replace') as f:
            combined_text.append(f.read().strip())

    text = "\n\n".join(combined_text)

    if not text:
        print(f"[{ch_num:>2}/{total}] SKIP (empty): Chapter_{ch_num}")
        return "skip"

    if os.path.exists(output_path):
        size = os.path.getsize(output_path) / (1024 * 1024)
        print(f"[{ch_num:>2}/{total}] SKIP (exists): {OUTPUT_PREFIX}{ch_num}{OUTPUT_SUFFIX} ({size:.1f} MB)")
        return "skip"

    print(f"[{ch_num:>2}/{total}] Converting Chapter_{ch_num} ({len(page_files)} pages, {len(text)} chars)...")

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
        print(f"[{ch_num:>2}/{total}] FAILED: Chapter_{ch_num}")
        return "fail"

async def main():
    root = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5"
    book_path = os.path.join(root, BOOK_DIR)

    if not os.path.isdir(book_path):
        print(f"ERROR: Book dir not found: {book_path}")
        sys.exit(1)

    chapters = sorted(
        [d for d in os.listdir(book_path) if d.startswith("Chapter_") and os.path.isdir(os.path.join(book_path, d))],
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
        output_file = os.path.join(ch_path, f"{OUTPUT_PREFIX}{ch_num}{OUTPUT_SUFFIX}")

        result = await convert_chapter(ch_path, output_file, ch_num, total)

        if result == "done":
            done += 1
        elif result == "skip":
            skipped += 1
        else:
            failed += 1

        await asyncio.sleep(1)

    print(f"\n=== DONE ===")
    print(f"Converted: {done} | Skipped: {skipped} | Failed: {failed}")

if __name__ == '__main__':
    asyncio.run(main())
