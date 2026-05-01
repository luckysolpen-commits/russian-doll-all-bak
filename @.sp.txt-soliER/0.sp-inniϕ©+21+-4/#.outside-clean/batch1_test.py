#!/usr/bin/env python3
"""
BATCH 1: First 3 FU D&D sessions for testing.
"""

import asyncio
import os
import re
import subprocess
import tempfile
from pydub import AudioSegment

EDGE_VOICE_MAP = {
    "DM (Mother Tmojizu)":  "zh-CN-XiaoxiaoNeural",
    "Mother Tmojizu":       "zh-CN-XiaoxiaoNeural",
    "Mother Tomokazu":      "zh-CN-XiaoxiaoNeural",
    "CHARA":                "en-US-AriaNeural",
    "Sol-Supreme":          "en-US-ChristopherNeural",
    "IQabella":             "en-US-AnaNeural",
    "Horli Qwin":           "en-US-JennyNeural",
    "DM":                   "zh-CN-XiaoxiaoNeural",
    "中文老师":             "zh-CN-XiaoxiaoNeural",
}

DEFAULT_VOICE = "zh-CN-XiaoxiaoNeural"
SRC_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+12/SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/1.ffu-ez_learnd&d+chem"
FILES = [
    "fud&d-sesh1-memory-palace-race.txt",
    "fud&d-sesh2-polo.txt",
    "fud&d-sesh3-horse-race.txt",
]

def parse_session(filepath):
    segments = []
    current_speaker = None
    current_text = []
    speaker_pattern = re.compile(r'^\*\*(.+?):\*\*\s*(.*)')
    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.rstrip('\n')
            if line.startswith('# ') or line.startswith('## ') or line.startswith('---'):
                continue
            match = speaker_pattern.match(line)
            if match:
                if current_speaker and current_text:
                    segments.append((current_speaker, ' '.join(current_text).strip()))
                current_speaker = match.group(1).strip()
                current_text = [match.group(2).strip()] if match.group(2).strip() else []
            elif line.strip() == '':
                if current_speaker and current_text:
                    segments.append((current_speaker, ' '.join(current_text).strip()))
                    current_speaker = None
                    current_text = []
            else:
                cleaned = re.sub(r'\*\*(.+?)\*\*', r'\1', line.strip())
                if cleaned:
                    current_text.append(cleaned)
    if current_speaker and current_text:
        segments.append((current_speaker, ' '.join(current_text).strip()))
    return segments

async def edge_tts_segment(text, voice):
    tmp = tempfile.NamedTemporaryFile(suffix='.mp3', delete=False)
    tmp.close()
    proc = await asyncio.create_subprocess_exec(
        'edge-tts', '--voice', voice, '--text', text, '--write-media', tmp.name,
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
    )
    await proc.wait()
    if os.path.getsize(tmp.name) == 0:
        os.unlink(tmp.name)
        return None
    return tmp.name

async def convert_file(filepath, output_path):
    segments = parse_session(filepath)
    print(f"  Found {len(segments)} segments")
    speakers = {}
    for spk, txt in segments:
        speakers[spk] = speakers.get(spk, 0) + 1
    print(f"  Speakers: {speakers}")

    combined = AudioSegment.empty()
    success = 0
    failed = 0

    for i, (speaker, text) in enumerate(segments):
        voice = EDGE_VOICE_MAP.get(speaker, DEFAULT_VOICE)
        if not text.strip():
            continue
        display = text[:60] + "..." if len(text) > 60 else text
        print(f"    [{i+1}/{len(segments)}] {speaker}: {display}")

        try:
            tmp_path = await edge_tts_segment(text, voice)
            if tmp_path:
                audio = AudioSegment.from_mp3(tmp_path)
                combined += audio
                os.unlink(tmp_path)
                success += 1
            else:
                failed += 1
        except Exception as e:
            print(f"    ERROR: {e}")
            failed += 1

    if combined:
        combined.export(output_path, format='mp3', bitrate='192k')
        size = os.path.getsize(output_path) / (1024 * 1024)
        print(f"  DONE: {os.path.basename(output_path)} ({size:.1f} MB) — {success} ok, {failed} failed")
        return True
    return False

async def main():
    print("=" * 60)
    print("BATCH 1: First 3 FU D&D sessions")
    print("=" * 60)

    for fname in FILES:
        filepath = os.path.join(SRC_DIR, fname)
        output_name = f"中{os.path.splitext(fname)[0]}.mp3"
        output_path = os.path.join(SRC_DIR, output_name)

        if os.path.exists(output_path):
            size = os.path.getsize(output_path) / (1024 * 1024)
            print(f"\nSKIP (exists): {output_name} ({size:.1f} MB)")
            continue

        print(f"\n--- {fname} ---")
        await convert_file(filepath, output_path)
        await asyncio.sleep(2)

    print("\n=== BATCH 1 DONE ===")
    print(f"Output dir: {SRC_DIR}")

if __name__ == '__main__':
    asyncio.run(main())
