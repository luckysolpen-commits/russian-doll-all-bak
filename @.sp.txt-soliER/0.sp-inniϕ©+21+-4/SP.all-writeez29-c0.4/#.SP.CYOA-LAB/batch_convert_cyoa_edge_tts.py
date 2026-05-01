#!/usr/bin/env python3
"""
Batch convert CYOA (Choose Your Own Adventure) pages to audio using edge-tts.
Each page_NN.txt file becomes its own MP3 — NO aggregation.
One MP3 per page for interactive "choose audio file" playback.
Uses en-US-RogerNeural (Horli voice — male, energetic).

Usage: python3 batch_convert_cyoa_edge_tts.py <book_label> <book_dir>
  book_label: e.g. "book0", "book1", "book3", "book4"
  book_dir: relative path from project root to the CYOA book directory

Examples:
  python3 batch_convert_cyoa_edge_tts.py book0 "SP.all-writeez28-a1/#.1-13.sp.redwhiteandU]b2/00_book]kOT]a0]gmii/rb&u.CYOA_Book.0"
  python3 batch_convert_cyoa_edge_tts.py book1 "SP.all-writeez28-a1/#.1-13.sp.redwhiteandU]b2/01]ugem&]gmii/rb&u.CYOA_Book]01"
  python3 batch_convert_cyoa_edge_tts.py book3 "SP.all-writeez28-a1/#.1-13.sp.redwhiteandU]b2/3&4.cd&cclonesdrones&crx/3.rb&u.CYO_book.3]CD&C.X"
  python3 batch_convert_cyoa_edge_tts.py book4 "SP.all-writeez28-a1/#.1-13.sp.redwhiteandU]b2/3&4.cd&cclonesdrones&crx/4.rb&u.cdc.2.book.4]BEST/rb&u.cyoa_book4]cd&c2"
"""

import os
import asyncio
import sys

VOICE = "en-US-RogerNeural"

async def convert_page(page_path, output_path, page_label, total_pages):
    """Convert a single CYOA page to audio."""
    if not os.path.exists(page_path):
        return "skip"

    with open(page_path, 'r', encoding='utf-8', errors='replace') as f:
        text = f.read().strip()

    if not text:
        print(f"[{page_label:>20}] SKIP (empty)")
        return "skip"

    if os.path.exists(output_path):
        size = os.path.getsize(output_path) / (1024 * 1024)
        print(f"[{page_label:>20}] SKIP (exists) ({size:.1f} MB)")
        return "skip"

    print(f"[{page_label:>20}] Converting ({len(text)} chars)...")

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
        print(f"[{page_label:>20}] DONE ({size:.1f} MB)")
        return "done"
    else:
        print(f"[{page_label:>20}] FAILED")
        return "fail"

async def main():
    if len(sys.argv) != 3:
        print("Usage: python3 batch_convert_cyoa_edge_tts.py <book_label> <book_dir>")
        print("Examples:")
        print('  python3 batch_convert_cyoa_edge_tts.py book0 "SP.all-writeez28-a1/#.1-13.sp.redwhiteandU]b2/00_book]kOT]a0]gmii/rb&u.CYOA_Book.0"')
        sys.exit(1)

    book_label = sys.argv[1]
    book_dir = sys.argv[2]

    # MODIFIED: Set the root to the current project directory where the script is located.
    root = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+18/SP.all-writeez28-b2/#.1-13.sp.redwhiteandU]c3"
    book_path = os.path.join(root, book_dir)

    if not os.path.isdir(book_path):
        print(f"ERROR: Book dir not found: {book_path}")
        sys.exit(1)

    # Find all page files (page_NN.txt, page_NNN.txt, etc.)
    page_files = []
    for f in os.listdir(book_path):
        if f.startswith("page_") and f.endswith(".txt"):
            num_part = f.split("_")[1].split(".")[0]
            if num_part.isdigit():
                page_files.append(f)
    page_files = sorted(page_files, key=lambda x: int(x.split("_")[1].split(".")[0]))

    total = len(page_files)
    done = 0
    skipped = 0
    failed = 0

    print(f"""Found {total} pages in {book_label}. Starting CYOA conversion with edge-tts ({VOICE})...""")

    for pf in page_files:
        pf_path = os.path.join(book_path, pf)
        page_num = pf.split("_")[1].split(".")[0]
        # Output naming convention from script: rb&u.{book_label}.page{page_num}-v1.0.mp3
        output_file = os.path.join(book_path, f"rb&u.{book_label}.page{page_num}-v1.0.mp3")
        page_label = f"{book_label}.p{page_num}"

        result = await convert_page(pf_path, output_file, page_label, total)

        if result == "done":
            done += 1
        elif result == "skip":
            skipped += 1
        else:
            failed += 1

        await asyncio.sleep(1)

    print(f"""
=== {book_label.upper()} DONE ===
Converted: {done} | Skipped: {skipped} | Failed: {failed}""")

if __name__ == '__main__':
    asyncio.run(main())
