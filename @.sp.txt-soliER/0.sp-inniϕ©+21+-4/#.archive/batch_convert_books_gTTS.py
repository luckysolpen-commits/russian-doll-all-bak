#!/usr/bin/env python3
"""
Batch convert Sol Pen Book 0 chapters to audio using gTTS (single voice).
Skips chapters that already have output files.
"""

import os
import subprocess
import sys
import time
import random

BOOK_DIR = "!.SP.all-write]ez28+a1/0.Sol Pen]bk0/#.book_SP_0.full+aud-b1"
OUTPUT_PREFIX = "sp.chapter"
OUTPUT_SUFFIX = "-v1.0.mp3"
DELAY_MIN = 3  # seconds between requests (avoid rate limiting)
DELAY_MAX = 6
MAX_RETRIES = 3

def main():
    root = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+1"
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

    print(f"Found {total} chapters. Starting conversion...\n")

    for ch_dir in chapters:
        ch_num = ch_dir.split("_")[1]
        ch_path = os.path.join(book_path, ch_dir)
        txt_file = os.path.join(ch_path, f"chapter_{ch_num}.txt")
        output_file = os.path.join(ch_path, f"{OUTPUT_PREFIX}{ch_num}{OUTPUT_SUFFIX}")

        # Skip if output already exists
        if os.path.exists(output_file):
            size = os.path.getsize(output_file) / (1024 * 1024)
            print(f"[{ch_num:>2}/{total}] SKIP (exists): {OUTPUT_PREFIX}{ch_num}{OUTPUT_SUFFIX} ({size:.1f} MB)")
            skipped += 1
            continue

        # Check if source text exists
        if not os.path.exists(txt_file):
            print(f"[{ch_num:>2}/{total}] ERROR: No text file: {txt_file}")
            failed += 1
            continue

        # Get text size for info
        txt_size = os.path.getsize(txt_file) / 1024
        print(f"[{ch_num:>2}/{total}] CONVERTING: chapter_{ch_num} ({txt_size:.0f} KB text) ... ", end="", flush=True)

        success = False
        for attempt in range(1, MAX_RETRIES + 1):
            try:
                result = subprocess.run(
                    ["gtts-cli", "-f", txt_file, "--output", output_file],
                    capture_output=True,
                    text=True,
                    timeout=300
                )

                if result.returncode == 0 and os.path.exists(output_file) and os.path.getsize(output_file) > 0:
                    size = os.path.getsize(output_file) / (1024 * 1024)
                    if attempt > 1:
                        print(f"✅ {size:.1f} MB (retry {attempt-1})")
                    else:
                        print(f"✅ {size:.1f} MB")
                    done += 1
                    success = True
                    break
                else:
                    err = result.stderr.strip() if result.stderr else "empty output"
                    if "429" in err or "Too Many Requests" in err:
                        wait = random.uniform(DELAY_MIN, DELAY_MAX) * attempt
                        print(f"⏳ rate limited, waiting {wait:.0f}s (attempt {attempt}/{MAX_RETRIES}) ... ", end="", flush=True)
                        time.sleep(wait)
                    else:
                        print(f"❌ FAILED: {err}")
                        failed += 1
                        break
            except subprocess.TimeoutExpired:
                print(f"⏳ timeout, retrying ({attempt}/{MAX_RETRIES}) ... ", end="", flush=True)
                time.sleep(DELAY_MIN * attempt)
            except Exception as e:
                print(f"❌ ERROR: {e}")
                failed += 1
                break

        if not success and attempt == MAX_RETRIES:
            # Check if we printed the retry prompt
            print(f"❌ FAILED after {MAX_RETRIES} retries")
            failed += 1

        # Delay between successful conversions
        if success:
            time.sleep(random.uniform(DELAY_MIN, DELAY_MAX))

    print(f"\n{'='*50}")
    print(f"BATCH COMPLETE: {done} converted, {skipped} skipped, {failed} failed")
    print(f"{'='*50}")

if __name__ == '__main__':
    main()
