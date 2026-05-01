#!/usr/bin/env python3
"""
Convert META TRAILER scenes to multi-voice audio using edge-tts.
10 scenes. 7 voices. Cinematic trailer style.

Voice mapping:
- Mr. Know-It-All: en-GB-RyanNeural (cinematic trailer narrator)
- Mother Tmojizu: zh-CN-XiaoxiaoNeural (DM + Q00 vessel)
- CHARA: fr-FR-VivienneMultilingualNeural (paranoid ex-CIA)
- Sol-Supreme: en-US-ChristopherNeural (deep, bio-tech genius)
- IQabella: en-US-AnaNeural (bubbly, empathetic)
- Horli Qwin: fr-FR-DeniseNeural (trap rapper, funny)


Usage: python3 batch_convert_meta_trailer.py
"""

import asyncio
import edge_tts
import os
import re
import sys
from pydub import AudioSegment
import tempfile

# ─── Configuration ───────────────────────────────────────────────────
BASE_DIR = "./"
OUTPUT_DIR = os.path.join(BASE_DIR, "meta-trailer-audio")

VOICE_MAP = {
    "MR. KNOW-IT-ALL": "en-GB-RyanNeural",
    "MOTHER TMOJIZU": "zh-CN-XiaoxiaoNeural",
    "CHARA": "fr-FR-VivienneMultilingualNeural",
    "SOL-SUPREME": "en-US-ChristopherNeural",
    "IQABELLA": "en-US-AnaNeural",
    "HORLI": "fr-FR-DeniseNeural",
    "RAHWEH": "zh-CN-YunxiaNeural",
    "PROFESSOR PYLORUS": "en-GB-SoniaNeural",
    "MOTHER TMOJIZU / Q00": "zh-CN-XiaoxiaoNeural",
}

VOICE_SETTINGS = {
    "zh-CN-YunxiaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-RyanNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "zh-CN-XiaoxiaoNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "fr-FR-VivienneMultilingualNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-ChristopherNeural": {"rate": "+0%", "pitch": "-50Hz"},
    "en-US-AnaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "fr-FR-DeniseNeural": {"rate": "+5%", "pitch": "+2Hz"},
    "en-US-JennyNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-SoniaNeural": {"rate": "+0%", "pitch": "+0Hz"},
}

SCENE_FILES = [
    "scene_01_the_piece.txt",
    "scene_02_eight_sparks.txt",
    "scene_03_pillars_awakening.txt",
    "scene_04_pillars_descent.txt",
    "scene_05_pillars_ascension.txt",
    "scene_06_stable_loop.txt",
    "scene_07_meta_game.txt",
    "scene_08_centroid_falls.txt",
    "scene_09_exo_os_xirqus.txt",
    "scene_10_eternal_spiral.txt",
]


def parse_segments(text):
    """Parse text into (speaker, text) segments."""
    segments = []
    current_speaker = None
    current_text = []

    speaker_pattern = re.compile(r'^\*\*(.+?):\*\*\s*(.*)')

    for line in text.split('\n'):
        line = line.rstrip()

        # Skip comment lines, headers, blank lines
        if (line.startswith('# ') or line.startswith('## ') or
            line.startswith('### ') or line.startswith('---') or
            line.startswith('* ') or line.startswith('SOUND:')):
            continue

        match = speaker_pattern.match(line)
        if match:
            if current_speaker and current_text:
                full_text = ' '.join(current_text).strip()
                if full_text:
                    segments.append((current_speaker, full_text))
            current_speaker = match.group(1).strip()
            current_text = [match.group(2).strip()] if match.group(2).strip() else []
        elif line.strip() == '':
            if current_speaker and current_text:
                full_text = ' '.join(current_text).strip()
                if full_text:
                    segments.append((current_speaker, full_text))
                current_speaker = None
                current_text = []
        else:
            cleaned = re.sub(r'\*\*(.+?)\*\*', r'\1', line.strip())
            cleaned = re.sub(r'[`*_]', '', cleaned)
            if cleaned:
                current_text.append(cleaned)

    if current_speaker and current_text:
        full_text = ' '.join(current_text).strip()
        if full_text:
            segments.append((current_speaker, full_text))

    return segments


async def tts_segment(text, voice):
    """Generate audio for a single segment using edge-tts."""
    if not text.strip():
        return None
    tmp = tempfile.NamedTemporaryFile(suffix='.mp3', delete=False)
    tmp.close()
    settings = VOICE_SETTINGS.get(voice, {"rate": "+0%", "pitch": "+0Hz"})
    communicate = edge_tts.Communicate(
        text, voice, rate=settings["rate"], pitch=settings["pitch"]
    )
    await communicate.save(tmp.name)
    if os.path.exists(tmp.name) and os.path.getsize(tmp.name) > 0:
        return tmp.name
    if os.path.exists(tmp.name):
        os.unlink(tmp.name)
    return None


async def convert_scene(scene_file, output_path):
    """Convert a single scene file to multi-voice audio."""
    input_path = os.path.join(BASE_DIR, scene_file)
    if not os.path.exists(input_path):
        print(f"  SKIP: {scene_file} (not found)")
        return False

    with open(input_path, 'r', encoding='utf-8') as f:
        text = f.read()

    segments = parse_segments(text)
    if not segments:
        print(f"  SKIP: {scene_file} (no segments parsed)")
        return False

    scene_name = scene_file.replace('.txt', '')
    print(f"\n{'='*60}")
    print(f"Scene: {scene_name}")
    print(f"Segments: {len(segments)}")
    print(f"{'='*60}")

    combined = AudioSegment.empty()
    success = 0
    failed = 0

    for i, (speaker, seg_text) in enumerate(segments):
        voice = VOICE_MAP.get(speaker)
        if not voice:
            print(f"  WARNING: Unknown speaker '{speaker}', skipping")
            failed += 1
            continue

        if not seg_text.strip():
            continue

        display = seg_text[:60] + "..." if len(seg_text) > 60 else seg_text
        voice_short = voice.split('-')[2] if '-' in voice else voice[:5]
        print(f"  [{i+1}/{len(segments)}] {speaker} ({voice_short}): {display}")

        try:
            tmp_path = await tts_segment(seg_text, voice)
            if tmp_path:
                audio = AudioSegment.from_mp3(tmp_path)
                combined += audio + AudioSegment.silent(duration=400)
                os.unlink(tmp_path)
                success += 1
            else:
                print(f"  ERROR: No audio generated for segment {i+1}")
                failed += 1
        except Exception as e:
            print(f"  ERROR: {e}")
            failed += 1

    if combined:
        combined.export(output_path, format='mp3', bitrate='192k')
        size = os.path.getsize(output_path) / (1024 * 1024)
        print(f"\n  DONE: {scene_name} → {size:.1f} MB ({success} ok, {failed} failed)")
        return True
    else:
        print(f"\n  FAILED: {scene_name} (no audio generated)")
        return False


async def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    total = len(SCENE_FILES)
    done = 0
    failed = 0

    print(f"Found {total} scenes. Starting conversion with edge-tts...\n")

    for scene_file in SCENE_FILES:
        scene_name = scene_file.replace('.txt', '')
        output_path = os.path.join(OUTPUT_DIR, f"{scene_name}.mp3")

        # Skip if already exists
        if os.path.exists(output_path) and os.path.getsize(output_path) > 0:
            size = os.path.getsize(output_path) / (1024 * 1024)
            print(f"SKIP (exists): {scene_name} ({size:.1f} MB)")
            done += 1
            continue

        result = await convert_scene(scene_file, output_path)
        if result:
            done += 1
        else:
            failed += 1

        await asyncio.sleep(0.5)

    print(f"\n{'='*60}")
    print(f"ALL SCENES DONE!")
    print(f"Converted: {done} | Failed: {failed}")
    print(f"Output directory: {OUTPUT_DIR}")
    print(f"{'='*60}")


if __name__ == '__main__':
    asyncio.run(main())
