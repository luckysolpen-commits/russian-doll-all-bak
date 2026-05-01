#!/usr/bin/env python3
"""
Batch convert books to audio using edge-tts with Ava voice.
Cleans text to avoid awkward pauses and skips already converted files.
"""

import os
import asyncio
import sys
import re

# Use en-US-AvaNeural voice as requested.
VOICE = "en-US-AvaNeural"

# Define book configurations. Each config specifies the relative path to the book directory,
# the output prefix for the audio files, and a function to find the text file(s) for a chapter.
# This script handles books with 'chapter_N' subdirectories, where each chapter contains
# either a single 'chapter_N.txt' file or multiple 'page_N.txt' files.
BOOK_CONFIGS = [
    {
        "book_dir_rel": "SP.all-writeez28-b2/0.Sol Pen]bk0/#.book_SP_0.full+aud-b1",
        "output_prefix": "sp",
        # Finds chapter_N.txt directly inside the chapter directory.
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
        # This book has chapter_N_page_N.txt, so need custom logic.
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
    {
        "book_dir_rel": "SP.all-writeez28-b2/11.FOOTRACE.FU]smol/book_template.scp",
        "output_prefix": "ffu",
        "chapter_finder": lambda bp, ch_dir_name, ch_num, total: (os.path.join(bp, ch_dir_name, f"chapter_{ch_num}.txt"), "single_file"),
    },
    {
        "book_dir_rel": "SP.all-writeez28-b2/12.centroid.u.ϕ]2xvsp]big]b1/cent-book.bigword]a0",
        "output_prefix": "cent",
        # Centroid chapters have page_N.txt files.
        "chapter_finder": lambda bp, ch_dir_name, ch_num, total: (os.path.join(bp, ch_dir_name), "multi_page_files"),
    },
    {
        "book_dir_rel": "SP.all-writeez28-b2/333.chatoe.bnb.13]a0]FULL",
        "output_prefix": "chatoe",
        "chapter_finder": lambda bp, ch_dir_name, ch_num, total: (os.path.join(bp, ch_dir_name), "multi_page_files"),
    },
    {
        "book_dir_rel": "SP.all-writeez28-b2/100.%.NGL.MPYRE]bk0/14.book]nglr.mpyre]FULL]b1",
        "output_prefix": "ngl",
        "chapter_finder": lambda bp, ch_dir_name, ch_num, total: (os.path.join(bp, ch_dir_name), "multi_page_files"),
    },
    {
        "book_dir_rel": "SP.all-writeez28-b2/111.tear-it]accidental.witch]y3/book_template.scp",
        "output_prefix": "tearit",
        "chapter_finder": lambda bp, ch_dir_name, ch_num, total: (os.path.join(bp, ch_dir_name, f"chapter_{ch_num}.txt"), "single_file"),
    },
    # CHAR.A.NON books have a different structure and are excluded for now.
    # Example: SP.all-writeez28-a1/-1.MyChara+21]b1/CHAR.A.NON]c3]++/../BOOK]...
]

OUTPUT_SUFFIX = "-ava-v1.0.mp3"
# Set the root directory to the workspace root.
ROOT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+20"

def clean_text_for_pauses(text):
    """
    Removes awkward pauses by:
    1. Replacing newline characters with spaces.
    2. Replacing multiple spaces with a single space.
    3. Stripping leading/trailing whitespace.
    """
    # Replace all newline characters (ASCII 10) with spaces.
    text = text.replace(chr(10), ' ')
    # Replace multiple spaces with a single space
    text = ' '.join(text.split())
    return text

async def convert_chapter(book_path, ch_dir_name, output_path, ch_num, total, book_prefix, chapter_finder_func): # Added chapter_finder_func as parameter
    """Convert a single chapter using edge-tts with text cleaning."""
    
    txt_source, source_type = None, None
    try:
        # Dynamically call the appropriate chapter_finder from the config
        txt_source, source_type = chapter_finder_func(book_path, ch_dir_name, ch_num, total) # Use passed chapter_finder_func
    except Exception as e:
        print(f"[{ch_num:>2}/{total}] ERROR: Could not determine text source for chapter {ch_num} in {os.path.basename(book_path)}: {e}")
        return "fail"

    text = ""
    page_count = 0

    if source_type == "single_file":
        if not os.path.exists(txt_source):
            print(f"[{ch_num:>2}/{total}] SKIP (no text file): {os.path.basename(txt_source)} in {os.path.basename(book_path)}")
            return "skip"
        with open(txt_source, 'r', encoding='utf-8', errors='replace') as f:
            text = f.read()
        page_count = 1
    elif source_type == "multi_page_files":
        # Logic to find page files, handles chapter_N_page_N.txt and page_N.txt
        page_files = sorted(
            [f for f in os.listdir(txt_source) if (f.startswith("page_") or (ch_dir_name and f.startswith(f"chapter_{ch_num}_page_"))) and f.endswith(".txt")],
            key=lambda x: int(re.search(r'_(\d+)\.txt$', x).group(1)) if re.search(r'_(\d+)\.txt$', x) else (int(x.split("_")[1].split(".")[0]) if len(x.split("_")) > 1 else float('inf'))
        )
        if not page_files:
            print(f"[{ch_num:>2}/{total}] SKIP (no pages): chapter_{ch_num} in {os.path.basename(book_path)}")
            return "skip"
        
        combined_text_parts = []
        for pf in page_files:
            pf_path = os.path.join(txt_source, pf)
            with open(pf_path, 'r', encoding='utf-8', errors='replace') as f:
                combined_text_parts.append(f.read().strip())
        # Explicitly use escaped newline characters for joining text parts.
        text = "

".join(combined_text_parts)
        page_count = len(page_files)
    else:
        print(f"[{ch_num:>2}/{total}] ERROR: Unknown source_type '{source_type}' for chapter {ch_num} in {os.path.basename(book_path)}")
        return "fail"

    cleaned_text = clean_text_for_pauses(text.strip())

    if not cleaned_text:
        print(f"[{ch_num:>2}/{total}] SKIP (empty after cleaning): chapter_{ch_num} in {os.path.basename(book_path)}")
        return "skip"

    if os.path.exists(output_path):
        size = os.path.getsize(output_path) / (1024 * 1024)
        print(f"[{ch_num:>2}/{total}] SKIP (exists): {os.path.basename(output_path)} ({size:.1f} MB)")
        return "skip"

    print(f"[{ch_num:>2}/{total}] Converting chapter_{ch_num} ({page_count} parts, {len(cleaned_text)} chars) in {os.path.basename(book_path)}...")

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
        print(f"[{ch_num:>2}/{total}] FAILED: chapter_{ch_num} in {os.path.basename(book_path)}")
        return "fail"

async def main():
    total_converted = 0
    total_skipped = 0
    total_failed = 0
    
    for config in BOOK_CONFIGS:
        book_dir_rel = config["book_dir_rel"]
        output_prefix = config["output_prefix"]
        chapter_finder_func = config["chapter_finder"] # Get the finder function
        
        book_path = os.path.join(ROOT_DIR, book_dir_rel)
        
        if not os.path.isdir(book_path):
            print(f"ERROR: Book dir not found: {book_path}")
            continue

        print(f"
=== Processing Book: {os.path.basename(book_path)} ===")
        
        chapters_dirs_info = []
        chapter_dir_base = book_path # Default base path
        
        # Determine how to find chapter information based on config
        try:
            # Call the chapter_finder function to get the source type.
            # Pass dummy arguments as they are not used in the type detection part.
            finder_result, source_type = chapter_finder_func(book_path, None, None, None)
        except Exception as e:
            print(f"ERROR: Failed to get source type for {os.path.basename(book_path)}: {e}")
            continue

        if source_type == "multi_page_files":
            # For books with chapter directories containing page files
            chapters_dirs_info = sorted(
                [d for d in os.listdir(book_path) if d.startswith("chapter_") and os.path.isdir(os.path.join(book_path, d))],
                key=lambda x: int(x.split("_")[1])
            )
            chapter_dir_base = book_path # Chapters are subdirectories of book_path
        elif source_type == "single_file":
             # For books where chapter files are directly in a 'chapter_N' subdirectory
             chapters_dirs_info = sorted(
                [d for d in os.listdir(book_path) if d.startswith("chapter_") and os.path.isdir(os.path.join(book_path, d))],
                key=lambda x: int(x.split("_")[1])
            )
             chapter_dir_base = book_path
        else:
            # Fallback for books where chapter files might be directly in book_path or have unique naming
            # Try to find chapter files directly in book_path if no chapter directories are found
            direct_chapter_files = sorted(
                [f for f in os.listdir(book_path) if re.match(r"chapter_(\d+)\.txt", f)],
                key=lambda x: int(x.split("_")[1].split(".")[0])
            )
            if direct_chapter_files:
                chapters_dirs_info = direct_chapter_files
                chapter_dir_base = book_path # Files are directly in book_path
            else:
                print(f"No chapter directories or direct chapter files found in {os.path.basename(book_path)}. Skipping book.")
                continue
        
        total_chapters = len(chapters_dirs_info)
        book_done = 0
        book_skipped = 0
        book_failed = 0
        
        if not chapters_dirs_info:
            print(f"No chapters found in {os.path.basename(book_path)}. Skipping book.")
            continue
            
        print(f"Found {total_chapters} chapters. Starting conversion with edge-tts ({VOICE})...")
        
        for i, chapter_info in enumerate(chapters_dirs_info):
            ch_dir_name = None
            ch_num = -1
            
            if isinstance(chapter_info, str) and os.path.isdir(os.path.join(chapter_dir_base, chapter_info)): # It's a chapter directory
                ch_dir_name = chapter_info
                try:
                    ch_num = int(ch_dir_name.split("_")[1])
                except (ValueError, IndexError):
                    print(f"[{i+1:>2}/{total_chapters}] SKIP (invalid chapter number): {ch_dir_name} in {os.path.basename(book_path)}")
                    total_skipped += 1
                    continue
            elif isinstance(chapter_info, str) and chapter_info.endswith(".txt"): # It's a direct chapter file
                ch_num = int(chapter_info.split("_")[1].split(".")[0])
                ch_dir_name = None # No subdirectory
            else:
                 print(f"[{i+1:>2}/{total_chapters}] SKIP (unexpected chapter format): {chapter_info} in {os.path.basename(book_path)}")
                 total_skipped += 1
                 continue
            
            txt_source_path, source_type = chapter_finder_func(chapter_dir_base, ch_dir_name, ch_num, total_chapters)
            
            # Construct output filename. Use dynamic prefix and suffix.
            # The output file path is usually saved within the chapter directory, or book root for direct files.
            if ch_dir_name:
                output_file_path = os.path.join(chapter_dir_base, ch_dir_name, f"{output_prefix}.chapter{ch_num:02d}{OUTPUT_SUFFIX}")
            else: # Direct file
                output_file_path = os.path.join(chapter_dir_base, f"{output_prefix}.chapter{ch_num:02d}{OUTPUT_SUFFIX}")

            # Pass chapter_finder_func to convert_chapter
            result = await convert_chapter(chapter_dir_base, ch_dir_name, output_file_path, ch_num, total_chapters, output_prefix, chapter_finder_func)

            if result == "done":
                book_done += 1
            elif result == "skip":
                book_skipped += 1
            else:
                book_failed += 1
            await asyncio.sleep(1)

        print(f"
=== {os.path.basename(book_path).upper()} DONE ===")
        print(f"Converted: {book_done} | Skipped: {book_skipped} | Failed: {book_failed}")
        
        total_converted += book_done
        total_skipped += book_skipped
        total_failed += book_failed

    print(f"
{'='*60}")
    print(f"BATCH CONVERSION COMPLETE:")
    print(f"Total Converted: {total_converted}")
    print(f"Total Skipped: {total_skipped}")
    print(f"Total Failed: {total_failed}")
    print(f"{'='*60}")

if __name__ == '__main__':
    # Ensure the script runs in the correct root directory
    # This might be necessary if running this script from a different location
    # os.chdir(ROOT_DIR) # Uncomment if needed, but usually CLI handles CWD
    asyncio.run(main())
