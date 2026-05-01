#!/usr/bin/env python3
"""
Batch convert NGL chapters to audio using edge-tts.
Optimized for natural storytelling flow with minimal unnatural pauses.
Uses ChristopherNeural voice at -50Hz for a deep "Architect" tone.
"""

import os
import asyncio
import sys
import re
import edge_tts

# Voice Settings
VOICE = "en-US-ChristopherNeural"
PITCH = "-50Hz"
RATE = "+0%"

# Paths
BOOK_DIR_REL = "SP.all-writeez28-b2/100.%.NGL.MPYRE]bk0/14.book]nglr.mpyre]FULL]b1"
ROOT_DIR = os.path.dirname(os.path.abspath(__file__))

# Output settings
OUTPUT_PREFIX = "ngl"
OUTPUT_SUFFIX = "-sol-v1.0-50.mp3"


def clean_text_for_tts(text: str) -> str:
    """
    Clean text for natural storytelling flow.
    - Collapses excessive newlines to reduce awkward long pauses.
    - Preserves some paragraph breaks for breathing room.
    - Normalizes spacing and punctuation for smoother TTS output.
    """
    if not text:
        return ""

    # Step 1: Normalize all whitespace (newlines, tabs, multiple spaces) → single space
    text = re.sub(r'\s+', ' ', text)

    # Step 2: Restore gentle paragraph breaks (double newline) only where there were clear separations
    # This gives natural breathing points without long silences between every sentence
    text = re.sub(r'([.!?])\s+', r'\1\n\n', text)

    # Step 3: Clean up excessive punctuation
    text = re.sub(r'([.!?]){2,}', r'\1', text)   # !! → !
    text = re.sub(r'[,;]{2,}', ',', text)        # ,, → ,

    # Step 4: Handle common dialogue dashes for better flow
    text = text.replace('—', ', ').replace('–', ', ').replace('―', ', ')

    # Final trim
    text = text.strip()

    return text


async def convert_chapter(ch_dir: str, output_path: str, ch_num: int, total: int):
    """Combine all page_N.txt files in the chapter and convert to speech."""
    
    # Find and sort page files
    try:
        page_files = sorted(
            [f for f in os.listdir(ch_dir) if f.startswith("page_") and f.endswith(".txt")],
            key=lambda x: int(x.split("_")[1].split(".")[0])
        )
    except Exception as e:
        print(f"[{ch_num:>2}/{total}] ERROR listing pages: {e}")
        return "fail"

    if not page_files:
        print(f"[{ch_num:>2}/{total}] SKIP (no pages): chapter_{ch_num}")
        return "skip"

    # Combine all pages into one continuous text
    combined_text_parts = []
    for pf in page_files:
        pf_path = os.path.join(ch_dir, pf)
        try:
            with open(pf_path, 'r', encoding='utf-8', errors='replace') as f:
                content = f.read().strip()
                if content:
                    combined_text_parts.append(content)
        except Exception as e:
            print(f"  ⚠️ Error reading {pf}: {e}")

    if not combined_text_parts:
        print(f"[{ch_num:>2}/{total}] SKIP (empty after reading): chapter_{ch_num}")
        return "skip"

    # Join pages with a single space (or very light break) for continuous storytelling flow
    raw_text = ' '.join(combined_text_parts)
    
    cleaned_text = clean_text_for_tts(raw_text)

    if not cleaned_text:
        print(f"[{ch_num:>2}/{total}] SKIP (empty after cleaning): chapter_{ch_num}")
        return "skip"

    # Skip if output file already exists
    if os.path.exists(output_path):
        size = os.path.getsize(output_path) / (1024 * 1024)
        print(f"[{ch_num:>2}/{total}] SKIP (exists): {os.path.basename(output_path)} ({size:.1f} MB)")
        return "skip"

    print(f"[{ch_num:>2}/{total}] Converting chapter_{ch_num} ({len(page_files)} pages, {len(cleaned_text):,} chars)...")

    try:
        communicate = edge_tts.Communicate(cleaned_text, VOICE, rate=RATE, pitch=PITCH)
        await communicate.save(output_path)
    except Exception as e:
        print(f"[{ch_num:>2}/{total}] FAILED: {e}")
        return "fail"

    if os.path.exists(output_path) and os.path.getsize(output_path) > 0:
        size = os.path.getsize(output_path) / (1024 * 1024)
        print(f"[{ch_num:>2}/{total}] DONE: {os.path.basename(output_path)} ({size:.1f} MB)")
        return "done"
    else:
        print(f"[{ch_num:>2}/{total}] FAILED: chapter_{ch_num} (zero size)")
        return "fail"


async def main():
    book_path = os.path.join(ROOT_DIR, BOOK_DIR_REL)

    if not os.path.isdir(book_path):
        print(f"ERROR: Book directory not found: {book_path}")
        sys.exit(1)

    # Find all chapter directories
    try:
        chapters_dirs = sorted(
            [d for d in os.listdir(book_path) if d.startswith("chapter_") and os.path.isdir(os.path.join(book_path, d))],
            key=lambda x: int(x.split("_")[1])
        )
    except Exception as e:
        print(f"ERROR: Could not list chapters in {book_path}: {e}")
        sys.exit(1)

    total_chapters = len(chapters_dirs)
    if not total_chapters:
        print(f"No chapter directories found in {book_path}.")
        sys.exit(1)

    print(f"Found {total_chapters} chapters. Starting conversion with {VOICE} at {PITCH}...")

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

        # Cleanup old versions to force re-conversion if needed
        for f in os.listdir(ch_path):
            if f.startswith(f"{OUTPUT_PREFIX}.chapter{ch_num:02d}") and f.endswith(".mp3") and f != output_filename:
                print(f"  🗑️ Removing old version: {f}")
                try:
                    os.remove(os.path.join(ch_path, f))
                except Exception as e:
                    print(f"    Failed to remove {f}: {e}")

        # Perform conversion
        result = await convert_chapter(ch_path, output_path, ch_num, total_chapters)

        if result == "done":
            overall_done += 1
        elif result == "skip":
            overall_skipped += 1
        else:
            overall_failed += 1

        await asyncio.sleep(0.5)  # Polite delay between chapters

    print("\n" + "=" * 70)
    print("BATCH CONVERSION COMPLETE - ngl BOOK")
    print(f"Total Chapters Processed: {total_chapters}")
    print(f"Converted : {overall_done:3d} | Skipped : {overall_skipped:3d} | Failed : {overall_failed:3d}")
    print("=" * 70)


if __name__ == '__main__':
    asyncio.run(main())
