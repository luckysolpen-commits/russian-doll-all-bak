#!/usr/bin/env python3
"""
Batch convert all D&D sessions to multi-voice audio using edge-tts.
Skips sessions that already have output files.
"""

import asyncio
import os
import sys

# Import from the test script
sys.path.insert(0, os.path.dirname(__file__))
from multi_voice_tts_test import parse_session, generate_edge_tts

# Session definitions: (input_file, output_dir, output_filename)
BASE = "spai.d&d-murderhobozos_4.2/avantasy-expanse+1"

SESSIONS = [
    # (input_path, output_dir, output_filename)
    (
        f"{BASE}/*.avantasy-sesh-#1-2/*.ava-session-#1.txt",
        f"{BASE}/*.avantasy-sesh-#1-2/",
        "ava-session-1_edge-tts.mp3",
    ),
    (
        f"{BASE}/*.avantasy-sesh-#1-2/*.ava-session-#2.txt",
        f"{BASE}/*.avantasy-sesh-#1-2/",
        "ava-session-2_edge-tts.mp3",
    ),
    (
        f"{BASE}/^.tmojio-edt-colab=R&WU+01/tpmojio-sesh#1.txt",
        f"{BASE}/^.tmojio-edt-colab=R&WU+01/",
        "tpmojio-sesh-1_edge-tts.mp3",
    ),
    (
        f"{BASE}/^.tmojio-edt-colab=R&WU+01/tpmojio-sesh#2.txt",
        f"{BASE}/^.tmojio-edt-colab=R&WU+01/",
        "tpmojio-sesh-2_edge-tts.mp3",
    ),
    (
        f"{BASE}/^.tmojio-edt-colab=R&WU+01/rpg-merger-sp/RMMSP_sesh#1.txt",
        f"{BASE}/^.tmojio-edt-colab=R&WU+01/rpg-merger-sp/",
        "RMMSP_sesh-1_edge-tts.mp3",
    ),
    (
        "spai.d&d-murderhobozos_4.2/fuzz-sesh#1-3.5/fuzz-sesh-grok-2-note#1.txt",
        "spai.d&d-murderhobozos_4.2/fuzz-sesh#1-3.5/",
        "fuzz-sesh-1_edge-tts.mp3",
    ),
    (
        "spai.d&d-murderhobozos_4.2/fuzz-sesh#1-3.5/fuzz-sesh-grok-2-note#2.txt",
        "spai.d&d-murderhobozos_4.2/fuzz-sesh#1-3.5/",
        "fuzz-sesh-2_edge-tts.mp3",
    ),
    (
        "spai.d&d-murderhobozos_4.2/fuzz-sesh#1-3.5/fuzz-sesh-grok-2-note#3.txt",
        "spai.d&d-murderhobozos_4.2/fuzz-sesh#1-3.5/",
        "fuzz-sesh-3_edge-tts.mp3",
    ),
]


async def main():
    root = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+1"
    os.chdir(root)

    total = len(SESSIONS)
    done = 0
    skipped = 0
    failed = 0

    for i, (input_file, output_dir, output_name) in enumerate(SESSIONS):
        output_path = os.path.join(output_dir, output_name)

        # Skip if already exists
        if os.path.exists(output_path):
            print(f"[{i+1}/{total}] SKIP (exists): {output_name}")
            skipped += 1
            continue

        if not os.path.exists(input_file):
            print(f"[{i+1}/{total}] ERROR: Input not found: {input_file}")
            failed += 1
            continue

        print(f"\n[{i+1}/{total}] START: {os.path.basename(input_file)}")
        try:
            segments = parse_session(input_file)
            print(f"  Parsed {len(segments)} segments")

            # Show speaker breakdown
            speakers = {}
            for spk, _ in segments:
                speakers[spk] = speakers.get(spk, 0) + 1
            for spk, count in sorted(speakers.items(), key=lambda x: -x[1]):
                print(f"    {spk}: {count}")

            await generate_edge_tts(segments, output_path)
            size = os.path.getsize(output_path) / (1024 * 1024)
            print(f"  ✅ DONE: {output_name} ({size:.1f} MB)")
            done += 1
        except Exception as e:
            print(f"  ❌ FAILED: {e}")
            failed += 1

    print(f"\n{'='*50}")
    print(f"BATCH COMPLETE: {done} converted, {skipped} skipped, {failed} failed")
    print(f"{'='*50}")


if __name__ == '__main__':
    asyncio.run(main())
