#!/usr/bin/env python3
"""
Batch convert Footrace.FU Book 2 chapters to audio using edge-tts with dual voices.
- English: en-US-AvaNeural
- Chinese: zh-CN-YunxiaNeural
Skips chapters that already have output files.
"""

import os
import asyncio
import sys
import re

# Voices
VOICE_EN = "zh-CN-YunxiaNeural"
VOICE_ZH = "zh-CN-YunxiaNeural"

# Paths
BOOK_DIR_REL = "SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/book_template.scp"
ROOT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+20"

# Output suffixes
OUTPUT_SUFFIX_EN = "-nin-v1.0.mp3"
OUTPUT_SUFFIX_ZH = "-yunxia-v1.0.mp3"


async def convert_chapter(chapter_path: str, ch_num: int, total: int, book_prefix: str,
                          text: str, voice: str, lang_label: str, suffix: str):
    """Convert text to speech for one language and save as MP3."""
    output_file = os.path.join(chapter_path, f"{book_prefix}.chapter{ch_num:02d}{suffix}")

    if not text:
        return "skip"

    if os.path.exists(output_file):
        size = os.path.getsize(output_file) / (1024 * 1024)
        print(f"[{ch_num:>2}/{total}] SKIP (exists, {lang_label}): {os.path.basename(output_file)} ({size:.1f} MB)")
        return "skip"

    print(f"[{ch_num:>2}/{total}] Converting chapter_{ch_num} ({lang_label}, {len(text)} chars)...")

    proc = await asyncio.create_subprocess_exec(
        'edge-tts',
        '--voice', voice,
        '--text', text,
        '--write-media', output_file,
        stdout=asyncio.subprocess.DEVNULL,
        stderr=asyncio.subprocess.DEVNULL,
    )
    await proc.wait()

    if os.path.exists(output_file) and os.path.getsize(output_file) > 0:
        size = os.path.getsize(output_file) / (1024 * 1024)
        print(f"[{ch_num:>2}/{total}] DONE ({lang_label}): {os.path.basename(output_file)} ({size:.1f} MB)")
        return "done"
    else:
        print(f"[{ch_num:>2}/{total}] FAILED ({lang_label}): chapter_{ch_num}")
        return "fail"


def clean_text_for_tts(text: str) -> str:
    """Clean text to reduce unnatural pauses in edge-tts."""
    if not text:
        return ""

    # Normalize whitespace
    text = re.sub(r'\s+', ' ', text.strip())

    # Optional: Add slight pause between paragraphs (edge-tts respects \n\n better)
    # If your original text has paragraph breaks, this helps
    text = re.sub(r'\n\s*\n', '\n\n', text)   # keep double newlines as pauses

    return text.strip()


async def main():
    book_path = os.path.join(ROOT_DIR, BOOK_DIR_REL)

    if not os.path.isdir(book_path):
        print(f"ERROR: Book directory not found: {book_path}")
        sys.exit(1)

    # Find chapter directories
    chapters_dirs = sorted(
        [d for d in os.listdir(book_path) if d.startswith("chapter_") and os.path.isdir(os.path.join(book_path, d))],
        key=lambda x: int(x.split("_")[1])
    )

    total_chapters = len(chapters_dirs)
    if not total_chapters:
        print(f"No chapter directories found in {book_path}.")
        sys.exit(1)

    print(f"Found {total_chapters} chapters. Starting dual-voice conversion (Ava + Yunxia)...")

    overall_done = 0
    overall_skipped = 0
    overall_failed = 0

    # Extract clean book prefix for filenames
    try:
        name_part = os.path.basename(BOOK_DIR_REL)
        name_part = name_part.split(']')[0].split('[')[0].split('(')[0]
        name_part = re.sub(r'[^a-zA-Z0-9._-]', '_', name_part).strip('_').lower()
        book_prefix = name_part if name_part else "footrace_b2"
    except Exception:
        book_prefix = "footrace_b2"

    for i, ch_dir_name in enumerate(chapters_dirs):
        ch_num = int(ch_dir_name.split("_")[1])
        ch_path = os.path.join(book_path, ch_dir_name)

        # Read texts
        text_en = ""
        text_zh = ""

        txt_en = os.path.join(ch_path, f"chapter_{ch_num}.txt")
        if os.path.exists(txt_en):
            with open(txt_en, 'r', encoding='utf-8', errors='replace') as f:
                text_en = f.read().strip()

        txt_zh = os.path.join(ch_path, f"chapter_{ch_num}_zh.txt")
        if os.path.exists(txt_zh):
            with open(txt_zh, 'r', encoding='utf-8', errors='replace') as f:
                text_zh = f.read().strip()

        if not text_en and not text_zh:
            print(f"[{ch_num:>2}/{total_chapters}] SKIP (no text found)")
            overall_skipped += 1
            continue

        # Clean texts to reduce unnatural pauses
        cleaned_en = clean_text_for_tts(text_en)
        cleaned_zh = clean_text_for_tts(text_zh)

        # Convert English
        status_en = await convert_chapter(
            chapter_path=ch_path,
            ch_num=ch_num,
            total=total_chapters,
            book_prefix=book_prefix,
            text=cleaned_en,
            voice=VOICE_EN,
            lang_label="EN",
            suffix=OUTPUT_SUFFIX_EN
        )

        # Convert Chinese
        status_zh = await convert_chapter(
            chapter_path=ch_path,
            ch_num=ch_num,
            total=total_chapters,
            book_prefix=book_prefix,
            text=cleaned_zh,
            voice=VOICE_ZH,
            lang_label="ZH",
            suffix=OUTPUT_SUFFIX_ZH
        )

        if status_en == "done" or status_zh == "done":
            overall_done += 1
        elif status_en == "skip" and status_zh == "skip":
            overall_skipped += 1
        else:
            overall_failed += 1

        await asyncio.sleep(0.8)  # polite delay between chapters

    print("\n" + "="*70)
    print("BATCH CONVERSION COMPLETE - FOOTRACE BOOK 2")
    print(f"Total Chapters: {total_chapters}")
    print(f"Converted : {overall_done:3d} | Skipped : {overall_skipped:3d} | Failed : {overall_failed:3d}")
    print("="*70)


if __name__ == '__main__':
    asyncio.run(main())
