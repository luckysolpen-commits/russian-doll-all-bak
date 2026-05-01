#!/usr/bin/env python3
"""
Batch convert Centroid "chat" chapters to audio using edge-tts with en-US-EmmaMultilingualNeural.
Each chapter has multiple page_N.txt files — combines them before conversion.
Improved empty file detection + retry logic.
"""

import os
import asyncio
import sys
import re

# Voice
VOICE = "en-US-EmmaMultilingualNeural"

# Paths
BOOK_DIR_REL = "SP.all-writeez28-b2/12.centroid.u.ϕ]2xvsp]big]b1/cent-book.bigword]a0"
ROOT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+20"

# Output settings
OUTPUT_PREFIX = "chat"
OUTPUT_SUFFIX = "-chat-v1.0.mp3"

# Delay & retry settings
BASE_DELAY = 2.5      # seconds between successful conversions
FAIL_DELAY = 6.0      # longer delay after failure/empty file
MAX_RETRIES = 2


def clean_text_for_tts(text: str) -> str:
    """Clean text to reduce unnatural long pauses while preserving paragraph breaks."""
    if not text:
        return ""

    text = re.sub(r'\n\s*\n+', '\n\n', text)
    
    lines = []
    for line in text.split('\n'):
        cleaned_line = ' '.join(line.split())
        lines.append(cleaned_line)
    
    text = '\n\n'.join(lines)
    return text.strip()


def is_valid_audio_file(filepath: str) -> bool:
    """Return True only if file exists AND has reasonable size (> 100 KB)."""
    if not os.path.exists(filepath):
        return False
    size = os.path.getsize(filepath)
    return size > 100 * 1024  # at least 100 KB (real audio files are usually several MB)


async def convert_chapter(ch_dir: str, output_path: str, ch_num: int, total: int, retry_count: int = 0) -> str:
    """Combine pages and convert to speech with retry support."""
    
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

    raw_text = '\n\n'.join(combined_text_parts)
    cleaned_text = clean_text_for_tts(raw_text)

    if not cleaned_text:
        print(f"[{ch_num:>2}/{total}] SKIP (empty after cleaning): chapter_{ch_num}")
        return "skip"

    # === IMPROVED CHECK: Delete empty/invalid files and re-convert ===
    if os.path.exists(output_path):
        if is_valid_audio_file(output_path):
            size_mb = os.path.getsize(output_path) / (1024 * 1024)
            print(f"[{ch_num:>2}/{total}] SKIP (exists, valid): {os.path.basename(output_path)} ({size_mb:.1f} MB)")
            return "skip"
        else:
            # Empty or tiny file → delete it and re-convert
            size_kb = os.path.getsize(output_path) / 1024
            print(f"[{ch_num:>2}/{total}] FOUND INVALID FILE (only {size_kb:.1f} KB) → deleting and re-converting...")
            try:
                os.remove(output_path)
            except Exception as e:
                print(f"    Warning: Could not delete invalid file: {e}")

    print(f"[{ch_num:>2}/{total}] Converting chapter_{ch_num} ({len(page_files)} pages, {len(cleaned_text)} chars)...")

    proc = await asyncio.create_subprocess_exec(
        'edge-tts',
        '--voice', VOICE,
        '--text', cleaned_text,
        '--write-media', output_path,
        stdout=asyncio.subprocess.DEVNULL,
        stderr=asyncio.subprocess.PIPE,
    )
    _, stderr = await proc.communicate()

    if is_valid_audio_file(output_path):
        size_mb = os.path.getsize(output_path) / (1024 * 1024)
        print(f"[{ch_num:>2}/{total}] DONE: {os.path.basename(output_path)} ({size_mb:.1f} MB)")
        return "done"
    else:
        error_msg = stderr.decode('utf-8', errors='replace').strip() if stderr else "Unknown error"
        print(f"[{ch_num:>2}/{total}] FAILED (attempt {retry_count+1}): chapter_{ch_num}")
        if error_msg:
            print(f"    Error: {error_msg[:250]}...")
        return "fail"


async def main():
    book_path = os.path.join(ROOT_DIR, BOOK_DIR_REL)

    if not os.path.isdir(book_path):
        print(f"ERROR: Book directory not found: {book_path}")
        sys.exit(1)

    chapters_dirs = sorted(
        [d for d in os.listdir(book_path) if d.startswith("chapter_") and os.path.isdir(os.path.join(book_path, d))],
        key=lambda x: int(x.split("_")[1])
    )

    total_chapters = len(chapters_dirs)
    if not total_chapters:
        print(f"No chapter directories found in {book_path}.")
        sys.exit(1)

    print(f"Found {total_chapters} chapters. Starting conversion with {VOICE}...")
    print(f"Base delay: {BASE_DELAY}s | Fail delay: {FAIL_DELAY}s | Max retries: {MAX_RETRIES}\n")

    overall_done = 0
    overall_skipped = 0
    overall_failed = 0

    for i, ch_dir_name in enumerate(chapters_dirs):
        try:
            ch_num = int(ch_dir_name.split("_")[1])
        except (ValueError, IndexError):
            print(f"[{i+1:>2}/{total_chapters}] SKIP (invalid chapter name): {ch_dir_name}")
            overall_skipped += 1
            await asyncio.sleep(BASE_DELAY)
            continue

        ch_path = os.path.join(book_path, ch_dir_name)

        output_filename = f"{OUTPUT_PREFIX}.chapter{ch_num:02d}{OUTPUT_SUFFIX}"
        output_path = os.path.join(ch_path, output_filename)

        # Try conversion with retries
        result = "fail"
        for attempt in range(MAX_RETRIES + 1):
            result = await convert_chapter(ch_path, output_path, ch_num, total_chapters, attempt)
            
            if result in ("done", "skip"):
                break
                
            if attempt < MAX_RETRIES:
                wait_time = FAIL_DELAY + (attempt * 3)
                print(f"    → Retrying chapter_{ch_num} in {wait_time:.1f}s...")
                await asyncio.sleep(wait_time)

        # Count results
        if result == "done":
            overall_done += 1
        elif result == "skip":
            overall_skipped += 1
        else:
            overall_failed += 1

        # Delay before next chapter
        delay = FAIL_DELAY if result == "fail" else BASE_DELAY
        await asyncio.sleep(delay)

    print("\n" + "=" * 75)
    print("BATCH CONVERSION COMPLETE - CHAT BOOK")
    print(f"Total Chapters Processed : {total_chapters}")
    print(f"Successfully Converted   : {overall_done:3d}")
    print(f"Skipped (valid)          : {overall_skipped:3d}")
    print(f"Failed                   : {overall_failed:3d}")
    print("=" * 75)

    if overall_failed > 0:
        print(f"\n⚠️  {overall_failed} chapter(s) still failed. Try running the script again.")


if __name__ == '__main__':
    asyncio.run(main())
