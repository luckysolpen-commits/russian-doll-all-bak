#!/usr/bin/env python3
"""
Batch convert IQabella's study notes to audio using edge-tts.
Single voice throughout: IQabella → en-US-AnaNeural
"""

import sys
import os
import asyncio
import subprocess

VOICE = "en-US-AnaNeural"

async def convert_file(filepath, output_path, idx, total):
    with open(filepath, 'r', encoding='utf-8') as f:
        text = f.read()

    # Strip markdown headers and metadata — keep only spoken text
    lines = text.split('\n')
    spoken_lines = []
    for line in lines:
        line = line.strip()
        if not line:
            continue
        if line.startswith('#'):
            continue
        if line.startswith('---'):
            continue
        if line.startswith('**[') and line.endswith(']**'):
            continue
        if line.startswith('**Topic:') or line.startswith('**Voice:') or line.startswith('**Estimated') or line.startswith('**Next:') or line.startswith('**Series') or line.startswith('**ϕ'):
            continue
        # Clean markdown bold
        line = line.replace('**', '')
        spoken_lines.append(line)

    full_text = '\n'.join(spoken_lines)
    print(f"[{idx}/{total}] Converting {os.path.basename(filepath)} ({len(full_text)} chars)...")

    proc = await asyncio.create_subprocess_exec(
        'edge-tts', '--voice', VOICE, '--text', full_text, '--write-media', output_path,
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
    )
    await proc.wait()

    if os.path.exists(output_path) and os.path.getsize(output_path) > 0:
        size = os.path.getsize(output_path) / (1024 * 1024)
        print(f"[{idx}/{total}] DONE: {os.path.basename(output_path)} ({size:.1f} MB)")
        return "done", size
    else:
        print(f"[{idx}/{total}] FAILED: {os.path.basename(filepath)}")
        return "fail", 0

async def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    source_dir = os.path.join(script_dir, "iqa-notes")

    files = sorted([f for f in os.listdir(source_dir) if f.startswith("iqa-study-sesh") and f.endswith(".txt")])

    if not files:
        print("No session files found!")
        sys.exit(1)

    print(f"Found {len(files)} study sessions. Converting with {VOICE}...\n")

    total_size = 0
    done_count = 0
    fail_count = 0

    for idx, fname in enumerate(files, 1):
        filepath = os.path.join(source_dir, fname)
        base = os.path.splitext(fname)[0]
        output_name = f"{base}.mp3"
        output_path = os.path.join(source_dir, output_name)

        if os.path.exists(output_path):
            size = os.path.getsize(output_path) / (1024 * 1024)
            print(f"[{idx}/{len(files)}] SKIP (exists): {output_name} ({size:.1f} MB)")
            total_size += size
            done_count += 1
            continue

        result, size = await convert_file(filepath, output_path, idx, len(files))
        total_size += size
        if result == "done":
            done_count += 1
        else:
            fail_count += 1

        await asyncio.sleep(2)

    print(f"\n{'='*60}")
    print(f"=== BATCH DONE ===")
    print(f"Converted: {done_count} | Failed: {fail_count}")
    print(f"Total audio size: {total_size:.1f} MB")
    print(f"{'='*60}")

if __name__ == '__main__':
    asyncio.run(main())
