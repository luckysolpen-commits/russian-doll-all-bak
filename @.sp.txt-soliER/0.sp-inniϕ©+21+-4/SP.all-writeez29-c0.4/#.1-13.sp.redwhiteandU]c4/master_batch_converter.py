#!/usr/bin/env python3
"""
MASTER BATCH TTS CONVERTER - All directories with corrected voice mappings.
Category 1: Multi-voice (voice swap needed)
Category 2: Single-voice (correct voice reconvert)

Output: 中-prefixed MP3s alongside source .txt files
"""

import sys
import os
import re
import asyncio
import subprocess
import tempfile
from pydub import AudioSegment

# ─── Voice Assignments ───────────────────────────────────────────────────

EDGE_VOICE_MAP = {
    # Main cast
    "DM (Mother Tmojizu)":  "zh-CN-XiaoxiaoNeural",
    "Mother Tmojizu":       "zh-CN-XiaoxiaoNeural",
    "Mother Tomokazu":      "zh-CN-XiaoxiaoNeural",  # legacy fallback
    "CHARA":                "en-US-AriaNeural",
    "Sol-Supreme":          "en-US-ChristopherNeural",
    "IQabella":             "en-US-AnaNeural",
    "Horli Qwin":           "en-US-JennyNeural",
    "DM":                   "zh-CN-XiaoxiaoNeural",
    "中文老师":             "zh-CN-XiaoxiaoNeural",
    "Chinese Narrator":     "zh-CN-XiaoxiaoNeural",
    "Chinese Voice":        "zh-CN-XiaoxiaoNeural",
    # Clone Chronicles
    "CHR-047":              "en-US-AriaNeural",
    "SOL-001":              "en-US-ChristopherNeural",
    "IQA-112":              "en-US-AnaNeural",
    "HQW-006":              "en-US-JennyNeural",
    "THE ORIGINAL CHARA":   "en-US-AriaNeural",
    "THE ORIGINAL SOL-SUPREME": "en-US-ChristopherNeural",
    "THE ORIGINAL IQABELLA": "en-US-AnaNeural",
    "THE ORIGINAL HORLI QWIN": "en-US-JennyNeural",
    # RMMSP extras
    "Luna":                 "en-US-EmmaNeural",
    "Sora":                 "en-US-EricNeural",
    "RaTheButcher":         "en-US-ChristopherNeural",
    "TRONALD_THE_MAGE":     "en-US-ChristopherNeural",
    "BORIN IRONMUG":        "en-US-ChristopherNeural",
    "HOODED FIGURE (LoonaTwilight)": "zh-CN-XiaoxiaoNeural",
    # Fuzz sessions
    "Player (The Creator)": "en-US-AndrewNeural",
    "The Creator":          "en-US-AndrewNeural",
    "Crimson":              "en-US-ChristopherNeural",
    "Ember":                "en-US-AriaNeural",
    "Glimmer":              "en-US-AnaNeural",
    "Verdi":                "en-US-EricNeural",
    "Aqua":                 "en-US-EmmaNeural",
    "Nebula":               "en-US-MichelleNeural",
    "Void":                 "en-US-ChristopherNeural",
    "Lumen":                "en-US-JennyNeural",
    "Leafsong":             "en-US-AvaNeural",
    # Narration/misc
    "Everyone":             "zh-CN-XiaoxiaoNeural",
    "GOLD COINS":           "zh-CN-XiaoxiaoNeural",
    "EXPERIENCE POINTS":    "zh-CN-XiaoxiaoNeural",
    "RARE ITEMS ACQUIRED":  "zh-CN-XiaoxiaoNeural",
    # Clone Chronicles NPCs
    "THE DIABOLUS":         "en-US-ChristopherNeural",
    "THE HOUSE":            "zh-CN-XiaoxiaoNeural",
    "THE MOON SIGNAL":      "en-US-EmmaNeural",
    "BROKER VEX":           "en-US-EricNeural",
    "BROKER":               "en-US-EricNeural",
    "SCRATCH":              "en-US-JennyNeural",
    "OFFICER KRELL":        "en-US-ChristopherNeural",
    "MASTER FENG":          "en-US-AndrewNeural",
    "MNEMOSYNE":            "en-US-EmmaNeural",
    "VOSS":                 "en-US-ChristopherNeural",
    "CHR-033":              "en-US-AriaNeural",
    "CHR-022":              "en-US-AriaNeural",
    "BUILDER CLONE":        "en-US-AndrewNeural",
    "CHEF CLONE":           "en-US-JennyNeural",
    "HACKER CLONE":         "en-US-AnaNeural",
    "ENFORCER":             "en-US-ChristopherNeural",
    "UNREGISTERED CLONE":   "en-US-EmmaNeural",
    "SCOUT":                "en-US-EricNeural",
    # TPMOJIO Cafe
    "Lucky":                "en-US-ChristopherNeural",
    "Mama Sour":            "en-US-MichelleNeural",
    "Rusty":                "en-US-EricNeural",
    "Old Man Feng":         "en-US-AndrewNeural",
    "Boss Nami":            "en-US-AriaNeural",
}

DEFAULT_VOICE = "zh-CN-XiaoxiaoNeural"

BASE_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+12"

# ─── Conversion Jobs ─────────────────────────────────────────────────────
# Each job: (source_dir, file_pattern, output_dir, category, single_voice?)

CONVERSION_JOBS = [
    # Category 1: Multi-voice
    {
        "dir": os.path.join(BASE_DIR, "SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/1.ffu-ez_learnd&d+chem"),
        "pattern": r"^fud&d-sesh\d+.*\.txt$",
        "category": "CAT1-MULTI",
    },
    {
        "dir": os.path.join(BASE_DIR, "SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/ffuture-redo"),
        "pattern": r"^ff-sesh\d+.*\.txt$",
        "category": "CAT1-MULTI",
    },
    {
        "dir": os.path.join(BASE_DIR, "spai.d-d-murderhobozos_5.1/sp&d-expanse=ALLIP+2/0.avantasy-sesh-#1-10/avan-sesh-3-10"),
        "pattern": r"^ava-session-\d+\.txt$",
        "category": "CAT1-MULTI",
    },
    {
        "dir": os.path.join(BASE_DIR, "spai.d-d-murderhobozos_5.1/sp&d-expanse=ALLIP+2/1.tmojio-1-10_v3/1.tmojio-seshs-3-10"),
        "pattern": r"^tpmojio-sesh#\d+\.txt$",
        "category": "CAT1-MULTI",
    },
    {
        "dir": os.path.join(BASE_DIR, "spai.d-d-murderhobozos_5.1/sp&d-expanse=ALLIP+2/2.ii.clone-chronicles/part1-blood-market"),
        "pattern": r"^clone-chronicles-sesh\d+\.txt$",
        "category": "CAT1-MULTI",
    },
    {
        "dir": os.path.join(BASE_DIR, "spai.d-d-murderhobozos_5.1/sp&d-expanse=ALLIP+2/2.ii.clone-chronicles/part2-empire-game"),
        "pattern": r"^clone-chronicles-p2-sesh\d+\.txt$",
        "category": "CAT1-MULTI",
    },
    {
        "dir": os.path.join(BASE_DIR, "spai.d-d-murderhobozos_5.1/sp&d-expanse=ALLIP+2/3.rpg-canvas-sp-editsesh"),
        "pattern": r"^RMMSP_sesh#\d+\.txt$",
        "category": "CAT1-MULTI",
    },
    {
        "dir": os.path.join(BASE_DIR, "spai.d-d-murderhobozos_5.1/sp&d-expanse=ALLIP+2/x20.fuzz-sesh#1-3.5"),
        "pattern": r"^fuzz-sesh-grok-2-note#(2|3)\.txt$",
        "category": "CAT1-MULTI",
    },
    {
        "dir": os.path.join(BASE_DIR, "HARD_VVAR"),
        "pattern": r"^hard_vvar-sesh\d+\.txt$",
        "category": "CAT1-MULTI",
    },
    {
        "dir": os.path.join(BASE_DIR, "TMOJIO_CAFE[]&/cafe-rp-sim"),
        "pattern": r"^cafe-sesh\d+.*\.txt$",
        "category": "CAT1-MULTI",
    },
    # Category 2: Single-voice
    {
        "dir": os.path.join(BASE_DIR, "SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/chara-notes"),
        "pattern": r"^chara-study-sesh\d+.*\.txt$",
        "category": "CAT2-SINGLE",
        "single_voice": "en-US-AriaNeural",
    },
    {
        "dir": os.path.join(BASE_DIR, "SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/sol-notes"),
        "pattern": r"^sol-study-sesh\d+.*\.txt$",
        "category": "CAT2-SINGLE",
        "single_voice": "en-US-ChristopherNeural",
    },
    {
        "dir": os.path.join(BASE_DIR, "SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/iqa-notes"),
        "pattern": r"^iqa-study-sesh\d+.*\.txt$",
        "category": "CAT2-SINGLE",
        "single_voice": "en-US-AnaNeural",
    },
    {
        "dir": os.path.join(BASE_DIR, "SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/horli-tutalage"),
        "pattern": r"^horli-tutalage-sesh\d+.*\.txt$",
        "category": "CAT2-HORLI-ZH",  # Horli + 中文老师 (2-voice)
    },
]

# ─── Parser ──────────────────────────────────────────────────────────────

def parse_session(filepath):
    """Parse a D&D session transcript into (speaker, text) segments."""
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

def parse_horli_tutalage(filepath):
    """Parse horli-tutalage files (Horli + 中文老师 dialogue, no ** markers)."""
    with open(filepath, 'r', encoding='utf-8') as f:
        text = f.read()

    # Strip headers/metadata
    lines = text.split('\n')
    clean_lines = []
    for line in lines:
        s = line.strip()
        if not s or s.startswith('#') or s.startswith('---') or s.startswith('**['):
            continue
        if any(s.startswith(p) for p in ['**Topic:', '**Voice:', '**Estimated', '**Next:', '**Series', '**ϕ']):
            continue
        clean_lines.append(s)
    full = '\n'.join(clean_lines)

    # Match Horli Qwin: or 中文老师:
    pattern = re.compile(r'(?:\*\*)?(Horli Qwin|中文老师)(?:\*\*)?:\s*')
    segments = []
    parts = pattern.split(full)
    speaker = None
    buf = []
    for part in parts:
        if not part.strip():
            continue
        if part.strip() in ['Horli Qwin', '中文老师']:
            if speaker and buf:
                t = ' '.join(buf).strip().replace('**', '')
                if t:
                    segments.append((speaker, t))
            speaker = part.strip()
            buf = []
        elif speaker:
            buf.append(part.strip())
    if speaker and buf:
        t = ' '.join(buf).strip().replace('**', '')
        if t:
            segments.append((speaker, t))
    return segments

# ─── TTS Engine ──────────────────────────────────────────────────────────

async def edge_tts_segment(text, voice):
    """Generate audio for a single segment using edge-tts."""
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

async def convert_multi_voice(segments, output_path):
    """Convert multi-voice segments to audio with Sol pitch lowering."""
    combined = AudioSegment.empty()
    success = 0
    failed = 0

    for i, (speaker, text) in enumerate(segments):
        voice = EDGE_VOICE_MAP.get(speaker, DEFAULT_VOICE)
        if not text.strip():
            continue

        display_text = text[:80] + "..." if len(text) > 80 else text
        print(f"    [{i+1}/{len(segments)}] {speaker}: {display_text}")

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
        return success, failed
    return 0, len(segments)

async def convert_single_voice(text, voice, output_path):
    """Convert single-voice text to audio."""
    print(f"    Converting with voice: {voice}")
    tmp_path = await edge_tts_segment(text, voice)
    if tmp_path:
        audio = AudioSegment.from_mp3(tmp_path)
        combined = audio
        combined.export(output_path, format='mp3', bitrate='192k')
        os.unlink(tmp_path)
        return 1, 0
    return 0, 1

# ─── Main ────────────────────────────────────────────────────────────────

async def process_file(filepath, category, single_voice=None):
    """Process a single file for conversion."""
    basename = os.path.basename(filepath)
    output_name = f"中{os.path.splitext(basename)[0]}.mp3"
    output_path = os.path.join(os.path.dirname(filepath), output_name)

    if os.path.exists(output_path):
        size = os.path.getsize(output_path) / (1024 * 1024)
        print(f"  SKIP (exists): {output_name} ({size:.1f} MB)")
        return "skip", size

    print(f"  Converting: {basename}")

    try:
        if single_voice:
            # Single-voice: read entire file, strip metadata, convert
            with open(filepath, 'r', encoding='utf-8') as f:
                lines = f.readlines()
            spoken = []
            for line in lines:
                s = line.strip()
                if not s or s.startswith('#') or s.startswith('---') or s.startswith('**['):
                    continue
                if any(s.startswith(p) for p in ['**Topic:', '**Voice:', '**Estimated', '**Next:', '**Series', '**ϕ']):
                    continue
                s = s.replace('**', '')
                spoken.append(s)
            full_text = '\n'.join(spoken)
            success, failed = await convert_single_voice(full_text, single_voice, output_path)
        elif category == "CAT2-HORLI-ZH":
            # Horli + 中文老师
            segments = parse_horli_tutalage(filepath)
            if not segments:
                print(f"  WARNING: No segments found")
                return "fail", 0
            success, failed = await convert_multi_voice(segments, output_path)
        else:
            # Multi-voice D&D
            segments = parse_session(filepath)
            if not segments:
                print(f"  WARNING: No segments found")
                return "fail", 0
            success, failed = await convert_multi_voice(segments, output_path)

        if os.path.exists(output_path):
            size = os.path.getsize(output_path) / (1024 * 1024)
            print(f"  DONE: {output_name} ({size:.1f} MB) — {success} ok, {failed} failed")
            return "done", size
        else:
            print(f"  FAILED: {basename}")
            return "fail", 0
    except Exception as e:
        print(f"  ERROR: {e}")
        return "fail", 0

async def main():
    print("=" * 70)
    print("MASTER BATCH TTS CONVERTER")
    print("=" * 70)
    print(f"\nVoice Mapping:")
    print(f"  Mother Tmojizu → zh-CN-XiaoxiaoNeural")
    print(f"  Horli Qwin     → en-US-JennyNeural")
    print(f"  Sol-Supreme    → en-US-ChristopherNeural (pitch lowered)")
    print(f"  CHARA          → en-US-AriaNeural")
    print(f"  IQabella       → en-US-AnaNeural")
    print(f"  中文老师        → zh-CN-XiaoxiaoNeural")
    print(f"\n{'=' * 70}\n")

    total_done = 0
    total_fail = 0
    total_skip = 0
    total_size = 0
    failures = []

    for job in CONVERSION_JOBS:
        src_dir = job["dir"]
        pattern = re.compile(job["pattern"])
        category = job["category"]
        single_voice = job.get("single_voice")

        if not os.path.isdir(src_dir):
            print(f"\n⚠️  DIRECTORY NOT FOUND: {src_dir}")
            continue

        files = sorted([f for f in os.listdir(src_dir) if pattern.match(f)])
        if not files:
            print(f"\n⚠️  No matching files in: {src_dir}")
            continue

        print(f"\n{'=' * 70}")
        print(f"[{category}] {src_dir}")
        print(f"Files: {len(files)}")
        print(f"{'=' * 70}")

        for fname in files:
            filepath = os.path.join(src_dir, fname)
            result, size = await process_file(filepath, category, single_voice)

            if result == "done":
                total_done += 1
                total_size += size
            elif result == "fail":
                total_fail += 1
                failures.append(os.path.relpath(filepath, BASE_DIR))
            elif result == "skip":
                total_skip += 1
                total_size += size

            await asyncio.sleep(1.5)  # rate limit

    print(f"\n{'=' * 70}")
    print(f"=== BATCH COMPLETE ===")
    print(f"Converted: {total_done}")
    print(f"Skipped (exists): {total_skip}")
    print(f"Failed: {total_fail}")
    print(f"Total audio size: {total_size:.1f} MB")
    if failures:
        print(f"\nFailed files (need retry):")
        for f in failures:
            print(f"  ✗ {f}")
    print(f"{'=' * 70}")

if __name__ == '__main__':
    asyncio.run(main())
