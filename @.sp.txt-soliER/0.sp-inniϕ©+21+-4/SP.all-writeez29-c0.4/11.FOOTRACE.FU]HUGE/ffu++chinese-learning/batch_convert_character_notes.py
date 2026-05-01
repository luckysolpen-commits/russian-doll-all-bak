#!/usr/bin/env python3
"""
Batch convert character study notes to multi-voice audio using edge-tts.
Handles chara-notes/, sol-notes/, and horli-tutalage/ with correct voice assignments.
"""

import sys
import os
import re
import asyncio
import subprocess
import tempfile

# ─── Voice Assignments ───────────────────────────────────────────────────

EDGE_VOICE_MAP = {
    "CHARA":                "en-US-AriaNeural",
    "Sol-Supreme":          "en-US-ChristopherNeural",
    "Horli Qwin":           "en-US-JennyNeural",
    "Mother Tmojizu":       "zh-CN-XiaoxiaoNeural",
    "中文老师":             "zh-CN-XiaoxiaoNeural",
}

async def convert_file(filepath, output_path, idx, total):
    # Determine voice based on which directory the file is in
    parent_dir = os.path.basename(os.path.dirname(filepath))
    if "chara" in parent_dir.lower():
        voice = EDGE_VOICE_MAP["CHARA"]
    elif "sol" in parent_dir.lower():
        voice = EDGE_VOICE_MAP["Sol-Supreme"]
    elif "horli" in parent_dir.lower():
        voice = EDGE_VOICE_MAP["Horli Qwin"]
    else:
        voice = EDGE_VOICE_MAP["CHARA"]  # default
    
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
        'edge-tts', '--voice', voice, '--text', full_text, '--write-media', output_path,
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
    dirs = ["chara-notes", "sol-notes", "horli-tutalage"]

    all_files = []
    for d in dirs:
        dir_path = os.path.join(script_dir, d)
        if os.path.isdir(dir_path):
            for f in sorted(os.listdir(dir_path)):
                if f.endswith(".txt") and "master-plan" not in f:
                    all_files.append(os.path.join(dir_path, f))

    if not all_files:
        print("No session files found!")
        sys.exit(1)

    print(f"Found {len(all_files)} study sessions. Converting...\n")

    total_size = 0
    done_count = 0
    fail_count = 0

    for idx, filepath in enumerate(all_files, 1):
        base = os.path.splitext(os.path.basename(filepath))[0]
        output_name = f"{base}.mp3"
        output_path = os.path.join(os.path.dirname(filepath), output_name)

        if os.path.exists(output_path):
            size = os.path.getsize(output_path) / (1024 * 1024)
            print(f"[{idx}/{len(all_files)}] SKIP (exists): {output_name} ({size:.1f} MB)")
            total_size += size
            done_count += 1
            continue

        result, size = await convert_file(filepath, output_path, idx, len(all_files))
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
