#!/usr/bin/env python3
"""
Translate Footrace.FU chapters from English to Chinese using translate-shell (trans).
Saves chapter_N_zh.txt files alongside the originals in the same directory.
Resumable - skips already-translated files.
"""

import os
import sys
import subprocess

BOOK_DIR = "SP.all-writeez28-a1/11.FOOTRACE.FU]smol/book_template.scp"

def translate_chapter(ch_dir, ch_num, total):
    """Translate a single chapter to Chinese."""
    src_file = os.path.join(ch_dir, f"chapter_{ch_num}.txt")
    dst_file = os.path.join(ch_dir, f"chapter_{ch_num}_zh.txt")

    if not os.path.exists(src_file):
        print(f"[{ch_num:>2}/{total}] SKIP (no source): chapter_{ch_num}")
        return "skip"

    if os.path.exists(dst_file) and os.path.getsize(dst_file) > 0:
        size = os.path.getsize(dst_file) / 1024
        print(f"[{ch_num:>2}/{total}] SKIP (exists): chapter_{ch_num}_zh.txt ({size:.1f} KB)")
        return "skip"

    with open(src_file, 'r', encoding='utf-8') as f:
        text = f.read().strip()

    if not text:
        print(f"[{ch_num:>2}/{total}] SKIP (empty): chapter_{ch_num}")
        return "skip"

    print(f"[{ch_num:>2}/{total}] Translating chapter_{ch_num} ({len(text)} chars)...")

    # Use translate-shell to translate the file directly
    # Larger files need longer timeout; retry once on failure
    for attempt in range(2):
        try:
            result = subprocess.run(
                ['trans', '-brief', '-no-auto', ':zh-CN', '-o', dst_file, f'file://{src_file}'],
                capture_output=True, text=True, timeout=600
            )
            break
        except subprocess.TimeoutExpired:
            if attempt == 0:
                print(f"[{ch_num:>2}/{total}] Timeout, retrying chapter_{ch_num}...")
                # Remove partial output
                if os.path.exists(dst_file):
                    os.remove(dst_file)
            else:
                raise

    if os.path.exists(dst_file) and os.path.getsize(dst_file) > 0:
        size = os.path.getsize(dst_file) / 1024
        print(f"[{ch_num:>2}/{total}] DONE: chapter_{ch_num}_zh.txt ({size:.1f} KB)")
        return "done"
    else:
        print(f"[{ch_num:>2}/{total}] FAILED: chapter_{ch_num}")
        return "fail"

def main():
    root = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5"
    book_path = os.path.join(root, BOOK_DIR)

    if not os.path.isdir(book_path):
        print(f"ERROR: Book dir not found: {book_path}")
        sys.exit(1)

    chapters = sorted(
        [d for d in os.listdir(book_path) if d.startswith("chapter_") and os.path.isdir(os.path.join(book_path, d))],
        key=lambda x: int(x.split("_")[1])
    )

    total = len(chapters)
    done = 0
    skipped = 0
    failed = 0

    print(f"Found {total} chapters. Translating to Chinese (zh-CN)...\n")

    for ch_dir in chapters:
        ch_num = int(ch_dir.split("_")[1])
        ch_path = os.path.join(book_path, ch_dir)

        result = translate_chapter(ch_path, ch_num, total)

        if result == "done":
            done += 1
        elif result == "skip":
            skipped += 1
        else:
            failed += 1

    print(f"\n=== DONE ===")
    print(f"Translated: {done} | Skipped: {done} | Failed: {failed}")

if __name__ == '__main__':
    main()
