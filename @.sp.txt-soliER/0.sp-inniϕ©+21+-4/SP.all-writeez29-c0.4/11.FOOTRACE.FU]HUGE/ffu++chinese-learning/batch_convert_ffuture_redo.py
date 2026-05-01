#!/usr/bin/env python3
"""
Batch convert FFUTURE-REDO sessions to multi-voice audio using edge-tts.
Converts all 3 session files in ffuture-redo/ directory.
"""

import sys
import os
import re
import asyncio
import subprocess
import tempfile

# ─── Voice Assignments ───────────────────────────────────────────────────

EDGE_VOICE_MAP = {
    "Mother Tmojizu":       "zh-CN-XiaoxiaoNeural",
    "CHARA":                "en-US-AriaNeural",
    "Sol-Supreme":          "en-US-ChristopherNeural",
    "IQabella":             "en-US-AnaNeural",
    "Horli Qwin":           "en-US-JennyNeural",
    "DM":                   "zh-CN-XiaoxiaoNeural",
    "中文老师":             "zh-CN-XiaoxiaoNeural",
}

# ─── Parser ──────────────────────────────────────────────────────────────

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

# ─── edge-tts Engine ─────────────────────────────────────────────────────

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

async def generate_edge_tts(segments, output_path):
    from pydub import AudioSegment
    print(f"[edge-tts] Processing {len(segments)} segments...")
    combined = AudioSegment.empty()
    success = 0
    failed = 0

    for i, (speaker, text) in enumerate(segments):
        voice = EDGE_VOICE_MAP.get(speaker, "zh-CN-XiaoxiaoNeural")
        if not text.strip():
            continue
        display_text = text[:80] + "..." if len(text) > 80 else text
        print(f"  [{i+1}/{len(segments)}] {speaker}: {display_text}")
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
            print(f"  ERROR: {e}")
            failed += 1

    print(f"[edge-tts] Done: {success} succeeded, {failed} failed")
    combined.export(output_path, format='mp3')
    size = os.path.getsize(output_path) / (1024 * 1024)
    print(f"[edge-tts] Output: {output_path} ({size:.1f} MB)")
    return output_path

# ─── Main ────────────────────────────────────────────────────────────────

async def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    source_dir = os.path.join(script_dir, "ffuture-redo")
    output_dir = os.path.join(script_dir, "ffuture-redo")

    session_files = sorted([
        f for f in os.listdir(source_dir)
        if f.startswith("ff-sesh") and f.endswith(".txt")
    ])

    if not session_files:
        print("No session files found!")
        sys.exit(1)

    print(f"Found {len(session_files)} session files.\n")

    total_success = 0
    total_fail = 0
    total_size = 0

    for session_file in session_files:
        filepath = os.path.join(source_dir, session_file)
        base = os.path.splitext(session_file)[0]
        output_name = f"{base}.mp3"
        output_path = os.path.join(output_dir, output_name)

        if os.path.exists(output_path):
            size = os.path.getsize(output_path) / (1024 * 1024)
            print(f"SKIP (exists): {output_name} ({size:.1f} MB)")
            total_size += size
            continue

        print(f"\n{'='*60}")
        print(f"Processing: {session_file}")
        print(f"{'='*60}")

        segments = parse_session(filepath)
        print(f"Found {len(segments)} speaker segments")

        speakers = {}
        for spk, txt in segments:
            speakers[spk] = speakers.get(spk, 0) + 1
        print("Speaker breakdown:")
        for spk, count in sorted(speakers.items(), key=lambda x: -x[1]):
            print(f"  {spk}: {count} segments")

        await generate_edge_tts(segments, output_path)

        if os.path.exists(output_path):
            size = os.path.getsize(output_path) / (1024 * 1024)
            total_size += size
            total_success += 1
        else:
            total_fail += 1

        await asyncio.sleep(2)

    print(f"\n{'='*60}")
    print(f"=== BATCH DONE ===")
    print(f"Converted: {total_success} | Failed: {total_fail}")
    print(f"Total audio size: {total_size:.1f} MB")
    print(f"{'='*60}")

if __name__ == '__main__':
    asyncio.run(main())
