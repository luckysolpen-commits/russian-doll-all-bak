#!/usr/bin/env python3
"""
Multi-voice TTS test script.
Converts a D&D session transcript using both edge-tts and pyttsx3.
Each speaker gets a distinct voice.

Usage:
    python3 multi_voice_tts_test.py <session_file> <output_dir>
"""

import sys
import os
import re
import asyncio
import subprocess
import tempfile

# ─── Voice Assignments ───────────────────────────────────────────────────

# edge-tts voices (Neural, high quality)
EDGE_VOICE_MAP = {
    # Main cast
    "DM (Mother Tmojizu)":  "zh-CN-XiaoxiaoNeural",     # Chinese teacher voice
    "Mother Tmojizu":       "zh-CN-XiaoxiaoNeural",     # Same as Chinese teacher
    "CHARA":                "en-US-AriaNeural",         # Female, confident
    "Sol-Supreme":          "en-US-ChristopherNeural",  # Male, DEEP authority/voice
    "IQabella":             "en-US-AnaNeural",          # Female, cute/cartoon
    "Horli Qwin":           "en-US-JennyNeural",        # Female, warm/friendly
    "DM":                   "zh-CN-XiaoxiaoNeural",     # Fallback to Mother voice

    # RMMSP extra speakers
    "Luna":                 "en-US-EmmaNeural",         # Female, cheerful
    "Sora":                 "en-US-EricNeural",         # Male, rational
    "LOONATWILIGHT":        "en-US-JennyNeural",        # Same as DM/Loona
    "RaTheButcher":         "en-US-GuyNeural",          # Male, deep
    "Tronald_The_Mage":     "en-US-ChristopherNeural",  # Male, authority
    "TRONALD_THE_MAGE":     "en-US-ChristopherNeural",
    "BORIN IRONMUG":        "en-US-GuyNeural",          # Male, gruff
    "HOODED FIGURE (LoonaTwilight)": "en-US-JennyNeural",

    # Fuzz sessions
    "Player (The Creator)": "en-US-AndrewNeural",       # Male, warm
    "The Creator":          "en-US-AndrewNeural",
    "Crimson":              "en-US-GuyNeural",          # Male, deep
    "Ember":                "en-US-AriaNeural",         # Female, fiery
    "Glimmer":              "en-US-AnaNeural",          # Female, cute
    "Verdi":                "en-US-EricNeural",         # Male, rational
    "Aqua":                 "en-US-EmmaNeural",         # Female, clear
    "Nebula":               "en-US-MichelleNeural",     # Female, pleasant
    "Void":                 "en-US-ChristopherNeural",  # Male, authority
    "Lumen":                "en-US-JennyNeural",        # Female, friendly
    "Leafsong":             "en-US-AvaNeural",          # Female, expressive

    # Narration / misc
    "Everyone":             "en-US-JennyNeural",        # Group → narrator
    "GOLD COINS":           "en-US-JennyNeural",        # System message → narrator
    "EXPERIENCE POINTS":    "en-US-JennyNeural",
    "RARE ITEMS ACQUIRED":  "en-US-JennyNeural",

    # Chinese learning — Memory Palace
    "中文老师":             "zh-CN-XiaoxiaoNeural",     # Chinese teacher, warm female
    "Chinese Narrator":     "zh-CN-XiaoxiaoNeural",
    "Chinese Voice":        "zh-CN-XiaoxiaoNeural",

    # Clone Chronicles — player clone designations
    "CHR-047":              "en-US-AriaNeural",         # CHARA's clone → same voice
    "SOL-001":              "en-US-GuyNeural",          # Sol-Supreme's clone → same voice
    "IQA-112":              "en-US-AnaNeural",          # IQabella's clone → same voice
    "HQW-006":              "en-US-RogerNeural",        # Horli Qwin's clone → same voice

    # Clone Chronicles — DM typo/caps variants
    "Mother Tmojizu":         "zh-CN-XiaoxiaoNeural",   # Same voice
    "MOTHER TMOJIZU":        "zh-CN-XiaoxiaoNeural",   # All-caps variant

    # Clone Chronicles Part 2 — "Original" versions
    "THE ORIGINAL CHARA":   "en-US-AriaNeural",         # Same as CHARA
    "THE ORIGINAL SOL-SUPREME": "en-US-GuyNeural",      # Same as Sol-Supreme
    "THE ORIGINAL IQABELLA": "en-US-AnaNeural",         # Same as IQabella
    "THE ORIGINAL HORLI QWIN": "en-US-RogerNeural",     # Same as Horli Qwin

    # Clone Chronicles — notable NPCs
    "THE DIABOLUS":         "en-US-ChristopherNeural",  # Male, authority/villain
    "THE HOUSE":            "en-US-JennyNeural",        # Narrator/system
    "THE MOON SIGNAL":      "en-US-EmmaNeural",         # Ethereal, otherworldly
    "BROKER VEX":           "en-US-EricNeural",         # Male, rational/sly
    "BROKER":               "en-US-EricNeural",
    "SCRATCH":              "en-US-RogerNeural",        # Male, energetic
    "OFFICER KRELL":        "en-US-ChristopherNeural",  # Male, authority
    "MASTER FENG":          "en-US-AndrewNeural",       # Male, warm/wise
    "MNEMOSYNE":            "en-US-EmmaNeural",         # Female, ethereal
    "VOSS":                 "en-US-GuyNeural",          # Male, deep
    "CHR-033":              "en-US-AriaNeural",         # Clone → same as CHR-047
    "CHR-022":              "en-US-AriaNeural",         # Clone → same as CHR-047
    "BUILDER CLONE":        "en-US-AndrewNeural",       # Male, warm
    "CHEF CLONE":           "en-US-RogerNeural",        # Male, energetic
    "HACKER CLONE":         "en-US-AnaNeural",          # Female, bright
    "ENFORCER":             "en-US-ChristopherNeural",  # Male, authority
    "UNREGISTERED CLONE":   "en-US-EmmaNeural",         # Female, innocent
    "SCOUT":                "en-US-EricNeural",         # Male, observant

    # Clone Chronicles — narrative/story segments
    "CHR-047's Story":      "en-US-AriaNeural",         # Read as character's voice
    "SOL-001's Story":      "en-US-GuyNeural",
    "IQA-112's Story":      "en-US-AnaNeural",
    "HQW-006's Story":      "en-US-RogerNeural",
}

# pyttsx3 voices (espeak-ng)
PYTTSX3_VOICE_MAP = {
    "DM (Mother Tmojizu)":  "zh-CN-XiaoxiaoNeural",
    "Mother Tmojizu":       "zh-CN-XiaoxiaoNeural",
    "CHARA":                "gmw/en-gb-x-rp",           # British RP
    "Sol-Supreme":          "gmw/en-us",                # American (deeper via rate)
    "IQabella":             "roa/en-us",                # American (higher via pitch)
    "Horli Qwin":           "roa/en-us",                # American (warm via rate)
    "DM":                   "zh-CN-XiaoxiaoNeural",
}

# Speaker-specific pyttsx3 adjustments (rate, pitch)
PYTTSX3_SPEAKER_ADJUST = {
    "DM (Mother Tmojizu)":  {"rate": 160, "pitch": 0},
    "Mother Tmojizu":       {"rate": 160, "pitch": 0},
    "CHARA":                {"rate": 170, "pitch": 10},
    "Sol-Supreme":          {"rate": 140, "pitch": -20},
    "IQabella":             {"rate": 180, "pitch": 20},
    "Horli Qwin":           {"rate": 160, "pitch": 0},
    "DM":                   {"rate": 160, "pitch": 0},
}


# ─── Parser ──────────────────────────────────────────────────────────────

def parse_session(filepath):
    """
    Parse a D&D session transcript.
    Returns list of (speaker, text) tuples.
    Handles formats like:
        **SPEAKER NAME:** text
        **SPEAKER NAME (note):** text
    """
    segments = []
    current_speaker = None
    current_text = []

    # Pattern: **SPEAKER:** or **SPEAKER (note):**
    speaker_pattern = re.compile(r'^\*\*(.+?):\*\*\s*(.*)')

    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.rstrip('\n')

            # Skip markdown headers, blank lines between speakers
            if line.startswith('# ') or line.startswith('## ') or line.startswith('---'):
                continue

            match = speaker_pattern.match(line)
            if match:
                # Save previous segment
                if current_speaker and current_text:
                    segments.append((current_speaker, ' '.join(current_text).strip()))

                current_speaker = match.group(1).strip()
                current_text = [match.group(2).strip()] if match.group(2).strip() else []
            elif line.strip() == '':
                # Blank line - might be end of segment
                if current_speaker and current_text:
                    segments.append((current_speaker, ' '.join(current_text).strip()))
                    current_speaker = None
                    current_text = []
            else:
                # Continuation of current speaker's text
                # Strip markdown bold from content
                cleaned = re.sub(r'\*\*(.+?)\*\*', r'\1', line.strip())
                if cleaned:
                    current_text.append(cleaned)

    # Don't forget the last segment
    if current_speaker and current_text:
        segments.append((current_speaker, ' '.join(current_text).strip()))

    return segments


# ─── edge-tts Engine ─────────────────────────────────────────────────────

async def edge_tts_segment(text, voice):
    """Generate audio for a single segment using edge-tts."""
    tmp = tempfile.NamedTemporaryFile(suffix='.mp3', delete=False)
    tmp.close()

    proc = await asyncio.create_subprocess_exec(
        'edge-tts',
        '--voice', voice,
        '--text', text,
        '--write-media', tmp.name,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    await proc.wait()

    if os.path.getsize(tmp.name) == 0:
        os.unlink(tmp.name)
        return None
    return tmp.name


async def generate_edge_tts(segments, output_path):
    """Generate multi-voice audio using edge-tts."""
    from pydub import AudioSegment
    from pydub.effects import speed

    print(f"[edge-tts] Processing {len(segments)} segments...")
    combined = AudioSegment.empty()
    success = 0
    failed = 0

    for i, (speaker, text) in enumerate(segments):
        voice = EDGE_VOICE_MAP.get(speaker, "zh-CN-XiaoxiaoNeural")
        if not text.strip():
            continue

        # Truncate very long segments to avoid timeouts
        display_text = text[:80] + "..." if len(text) > 80 else text
        print(f"  [{i+1}/{len(segments)}] {speaker}: {display_text}")

        try:
            tmp_path = await edge_tts_segment(text, voice)
            if tmp_path:
                audio = AudioSegment.from_mp3(tmp_path)
                
                # Sol-Supreme: WAY lower pitch - slow down significantly (0.7x speed = much deeper)
                if speaker == "Sol-Supreme":
                    audio = audio.speedup(playback_speed=0.7)
                    # Also lower the pitch by reducing sample rate slightly
                    audio = audio._spawn(audio.raw_data, overrides={"frame_rate": int(audio.frame_rate * 0.85)})
                    audio = audio.set_frame_rate(audio.frame_rate)
                
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
    print(f"[edge-tts] Output: {output_path}")
    return output_path


# ─── pyttsx3 Engine ──────────────────────────────────────────────────────

def generate_pyttsx3(segments, output_path):
    """Generate multi-voice audio using pyttsx3."""
    import pyttsx3
    from pydub import AudioSegment

    print(f"[pyttsx3] Processing {len(segments)} segments...")
    combined = AudioSegment.empty()
    success = 0
    failed = 0

    engine = pyttsx3.init()

    for i, (speaker, text) in enumerate(segments):
        voice_id = PYTTSX3_VOICE_MAP.get(speaker, "roa/en-us")
        adjust = PYTTSX3_SPEAKER_ADJUST.get(speaker, {"rate": 160, "pitch": 0})

        if not text.strip():
            continue

        display_text = text[:80] + "..." if len(text) > 80 else text
        print(f"  [{i+1}/{len(segments)}] {speaker} (voice: {voice_id}): {display_text}")

        try:
            tmp = tempfile.NamedTemporaryFile(suffix='.wav', delete=False)
            tmp.close()

            engine.setProperty('voice', voice_id)
            engine.setProperty('rate', adjust['rate'])
            engine.setProperty('pitch', adjust['pitch'])

            engine.save_to_file(text, tmp.name)
            engine.runAndWait()

            if os.path.getsize(tmp.name) > 0:
                audio = AudioSegment.from_wav(tmp.name)
                combined += audio
                os.unlink(tmp.name)
                success += 1
            else:
                os.unlink(tmp.name)
                failed += 1
        except Exception as e:
            print(f"  ERROR: {e}")
            failed += 1

    print(f"[pyttsx3] Done: {success} succeeded, {failed} failed")
    combined.export(output_path, format='mp3')
    print(f"[pyttsx3] Output: {output_path}")
    return output_path


# ─── Main ────────────────────────────────────────────────────────────────

def main():
    if len(sys.argv) < 3:
        print("Usage: python3 multi_voice_tts_test.py <session_file> <output_dir>")
        sys.exit(1)

    session_file = sys.argv[1]
    output_dir = sys.argv[2]

    if not os.path.exists(session_file):
        print(f"Error: File not found: {session_file}")
        sys.exit(1)

    os.makedirs(output_dir, exist_ok=True)

    # Derive output filename from input file
    base = os.path.splitext(os.path.basename(session_file))[0]
    # Clean up: replace # with nothing, strip leading dots/stars
    base = base.lstrip('.*#').strip()
    output_name = f"{base}_edge-tts.mp3"
    output_path = os.path.join(output_dir, output_name)

    # Parse
    print(f"Parsing: {session_file}")
    segments = parse_session(session_file)
    print(f"Found {len(segments)} speaker segments")

    # Show speaker stats
    speakers = {}
    for spk, txt in segments:
        speakers[spk] = speakers.get(spk, 0) + 1
    print("Speaker breakdown:")
    for spk, count in sorted(speakers.items(), key=lambda x: -x[1]):
        print(f"  {spk}: {count} segments")

    # edge-tts only
    asyncio.run(generate_edge_tts(segments, output_path))

    print("\n=== DONE ===")
    print(f"Output: {output_path}")


if __name__ == '__main__':
    main()
