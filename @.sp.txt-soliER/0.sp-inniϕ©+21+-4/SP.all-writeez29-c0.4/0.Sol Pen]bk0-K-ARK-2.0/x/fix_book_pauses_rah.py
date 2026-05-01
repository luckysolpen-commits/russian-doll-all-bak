#!/usr/bin/env python3
"""
Batch convert books to audio using edge-tts with Ava voice.
Cleans text to avoid awkward pauses and skips already converted files.
"""

import os
import asyncio
import sys
import re

# Use the requested voice
VOICE = "zh-CN-YunxiaNeural"

# Define book configurations
BOOK_CONFIGS = [
    {
        "book_dir_rel": "SP.all-writeez28-b2/0.Sol Pen]bk0-K-ARK-2.0/#.book_SP_0.full+aud-c2",
        "output_prefix": "sp",
        "chapter_finder": lambda bp, ch_dir_name, ch_num, total: (os.path.join(bp, ch_dir_name, f"chapter_{ch_num}.txt"), "single_file"),
    },
    {
        "book_dir_rel": "SP.all-writeez28-b2/1.ii.geminii]2part]a0/1.GEMANDI.part2/book_partii.gemini",
        "output_prefix": "gem2",
        "chapter_finder": lambda bp, ch_dir_name, ch_num, total: (os.path.join(bp, ch_dir_name, f"chapter_{ch_num}.txt"), "single_file"),
    },
    {
        "book_dir_rel": "SP.all-writeez28-b2/2.RedRock.AndROLL]ING]mid]a1/book_RR]BIG",
        "output_prefix": "rr",
        "chapter_finder": lambda bp, ch_dir_name, ch_num, total: (os.path.join(bp, ch_dir_name, f"chapter_{ch_num}.txt"), "single_file"),
    },
    {
        "book_dir_rel": "SP.all-writeez28-b2/3.DEATH-KRYO!]big]b2/book_template.scp",
        "output_prefix": "dk",
        "chapter_finder": lambda bp, ch_dir_name, ch_num, total: (os.path.join(bp, ch_dir_name, f"chapter_{ch_num}.txt"), "single_file"),
    },
    {
        "book_dir_rel": "SP.all-writeez28-b2/4.moonshine]big]b1/book_template.scp",
        "output_prefix": "ms",
        "chapter_finder": lambda bp, ch_dir_name, ch_num, total: (os.path.join(bp, ch_dir_name, f"chapter_{ch_num}.txt"), "single_file"),
    },
    {
        "book_dir_rel": "SP.all-writeez28-b2/5.SUPERWAR.ikrs]FULwc]bk0/book_template.scp",
        "output_prefix": "sw",
        "chapter_finder": lambda bp, ch_dir_name, ch_num, total: (os.path.join(bp, ch_dir_name, f"chapter_{ch_num}.txt"), "single_file"),
    },
    {
        "book_dir_rel": "SP.all-writeez28-b2/8.Digi_Sol.Suicider]BIG]b1/8.DSS.book_]full-gemini]a0",
        "output_prefix": "dss",
        "chapter_finder": lambda bp, ch_dir_name, ch_num, total: (os.path.join(bp, ch_dir_name), "multi_page_files"),
    },
    {
        "book_dir_rel": "SP.all-writeez28-b2/9.METAL SUN[METAL]wcsm+wtf]a1/ms_book]a0",
        "output_prefix": "msun",
        "chapter_finder": lambda bp, ch_dir_name, ch_num, total: (os.path.join(bp, ch_dir_name, f"chapter_{ch_num}.txt"), "single_file"),
    },
    {
        "book_dir_rel": "SP.all-writeez28-b2/10.xo.files]wc.smol]a0/book_template.scp",
        "output_prefix": "xo",
        "chapter_finder": lambda bp, ch_dir_name, ch_num, total: (os.path.join(bp, ch_dir_name, f"chapter_{ch_num}.txt"), "single_file"),
    },
]

OUTPUT_SUFFIX = "-rah-v1.0.mp3"
ROOT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+20"


def clean_text_for_pauses(text: str) -> str:
    """
    Removes awkward pauses by replacing newlines and multiple spaces with single spaces.
    """
    text = text.replace('\n', ' ')
    text = text.replace('\r', ' ')
    text = ' '.join(text.split())  # normalize all whitespace
    return text.strip()


async def convert_chapter(book_path, ch_dir_name, output_path, ch_num, total, book_prefix, chapter_finder_func):
    """Convert a single chapter using edge-tts with text cleaning."""
    
    try:
        txt_source, source_type = chapter_finder_func(book_path, ch_dir_name, ch_num, total)
    except Exception as e:
        print(f"[{ch_num:>2}/{total}] ERROR: Could not determine text source for chapter {ch_num}: {e}")
        return "fail"

    text = ""
    page_count = 0

    if source_type == "single_file":
        if not os.path.exists(txt_source):
            print(f"[{ch_num:>2}/{total}] SKIP (no text file): {os.path.basename(txt_source)}")
            return "skip"
        with open(txt_source, 'r', encoding='utf-8', errors='replace') as f:
            text = f.read()
        page_count = 1

    elif source_type == "multi_page_files":
        # Find all page_*.txt or chapter_N_page_N.txt files
        page_files = sorted(
            [f for f in os.listdir(txt_source)
             if f.endswith(".txt") and 
             (f.startswith("page_") or (ch_dir_name and f.startswith(f"chapter_{ch_num}_page_")))],
            key=lambda x: int(re.search(r'_(\d+)\.txt$', x).group(1)) 
                 if re.search(r'_(\d+)\.txt$', x) 
                 else float('inf')
        )
        
        if not page_files:
            print(f"[{ch_num:>2}/{total}] SKIP (no pages): chapter_{ch_num}")
            return "skip"
        
        combined_text_parts = []
        for pf in page_files:
            pf_path = os.path.join(txt_source, pf)
            with open(pf_path, 'r', encoding='utf-8', errors='replace') as f:
                combined_text_parts.append(f.read().strip())
        
        # Join pages with a blank line (or single newline - adjust if needed)
        text = "\n\n".join(combined_text_parts)
        page_count = len(page_files)

    else:
        print(f"[{ch_num:>2}/{total}] ERROR: Unknown source_type '{source_type}'")
        return "fail"

    cleaned_text = clean_text_for_pauses(text)

    if not cleaned_text:
        print(f"[{ch_num:>2}/{total}] SKIP (empty after cleaning): chapter_{ch_num}")
        return "skip"

    if os.path.exists(output_path):
        size = os.path.getsize(output_path) / (1024 * 1024)
        print(f"[{ch_num:>2}/{total}] SKIP (exists): {os.path.basename(output_path)} ({size:.1f} MB)")
        return "skip"

    print(f"[{ch_num:>2}/{total}] Converting chapter_{ch_num} ({page_count} parts, {len(cleaned_text):,} chars)...")

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
    total_converted = 0
    total_skipped = 0
    total_failed = 0
    
    for config in BOOK_CONFIGS:
        book_dir_rel = config["book_dir_rel"]
        output_prefix = config["output_prefix"]
        chapter_finder_func = config["chapter_finder"]
        
        book_path = os.path.join(ROOT_DIR, book_dir_rel)
        
        if not os.path.isdir(book_path):
            print(f"ERROR: Book directory not found: {book_path}")
            continue

        print(f"\n=== Processing Book: {os.path.basename(book_path)} ===")
        
        # Find chapter directories
        chapters_dirs_info = sorted(
            [d for d in os.listdir(book_path) 
             if d.startswith("chapter_") and os.path.isdir(os.path.join(book_path, d))],
            key=lambda x: int(x.split("_")[1])
        )
        
        if not chapters_dirs_info:
            print(f"No chapter directories found in {os.path.basename(book_path)}. Skipping.")
            continue
            
        total_chapters = len(chapters_dirs_info)
        book_done = 0
        book_skipped = 0
        book_failed = 0
        
        print(f"Found {total_chapters} chapters. Starting conversion with {VOICE}...")

        for i, ch_dir_name in enumerate(chapters_dirs_info, 1):
            try:
                ch_num = int(ch_dir_name.split("_")[1])
            except (ValueError, IndexError):
                print(f"[{i:>2}/{total_chapters}] SKIP (invalid chapter number): {ch_dir_name}")
                total_skipped += 1
                continue

            output_file_path = os.path.join(book_path, ch_dir_name, f"{output_prefix}.chapter{ch_num:02d}{OUTPUT_SUFFIX}")

            result = await convert_chapter(
                book_path, ch_dir_name, output_file_path, ch_num, 
                total_chapters, output_prefix, chapter_finder_func
            )

            if result == "done":
                book_done += 1
            elif result == "skip":
                book_skipped += 1
            else:
                book_failed += 1

            await asyncio.sleep(1)  # small delay between chapters

        print(f"\n=== {os.path.basename(book_path).upper()} DONE ===")
        print(f"Converted: {book_done} | Skipped: {book_skipped} | Failed: {book_failed}")
        
        total_converted += book_done
        total_skipped += book_skipped
        total_failed += book_failed

    print(f"\n{'='*70}")
    print("BATCH CONVERSION COMPLETE")
    print(f"Total Converted: {total_converted}")
    print(f"Total Skipped:   {total_skipped}")
    print(f"Total Failed:    {total_failed}")
    print(f"{'='*70}")


if __name__ == '__main__':
    asyncio.run(main())
