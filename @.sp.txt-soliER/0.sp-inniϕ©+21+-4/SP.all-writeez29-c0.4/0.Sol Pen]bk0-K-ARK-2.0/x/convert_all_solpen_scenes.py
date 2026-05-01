#!/usr/bin/env python3
"""
Convert ALL Sol Pen screenplay scenes to multi-voice audio.
Handles Family A format: <center>NAME</center> + > dialogue
"""

import asyncio
import edge_tts
import os
import re
import sys

SCENE_DIR = "./0.scenes-K-ARK-2.1"
OUTPUT_DIR = "./audio-output-50"

VOICE_MAP = {
    "Narrator": "en-GB-RyanNeural",
    "LUCKY": "en-US-BrianNeural",
    "ASTRA": "es-MX-DaliaNeural",
    "GEM": "en-US-AvaNeural",
    "LOONA": "ko-KR-SunHiNeural",
    "SOL": "en-US-ChristopherNeural",
    "KAITO": "ja-JP-KeitaNeural",
    "AGENT THOMPSON": "en-US-CoraNeural",
    "BLACK QUEEN": "en-NG-EzinneNeural",
    "RED QUEEN": "en-NG-EzinneNeural",
    "PRINCESS MEGAN8R": "en-US-AshleyNeural",
    "CLONE SOLDIER": "en-US-ThomasNeural",
    "SILVER-BAND CLONE": "en-US-ThomasNeural",
    "CLONE TECHNICIAN": "en-US-AvaNeural",
    "CLONE 2": "en-US-AvaNeural",
    "CLONE 3": "en-GB-LibbyNeural",
    "LAUREN": "en-GB-LibbyNeural",
    "TERRORIST LEADER": "en-NG-AbeoNeural",
    "SHADOW SYNDICATE GUARD": "en-NG-AbeoNeural",
    "USSF COMMANDER": "en-GB-ThomasNeural",
    "REBEL SCIENTIST 1": "en-US-SteffanNeural",
    "REBEL PILOT": "en-US-SteffanNeural",
    "CLOWN REBEL": "en-AU-WilliamNeural",
    "MESSAGE": "en-ZA-LukeNeural",
    "DR. PATEL": "en-IN-NeerjaNeural",
    "FAKE LUCKY": "en-US-BrianNeural",
    "ELDER REPTILIAN": "en-GB-ThomasNeural",
    "HUMANOID ROBOT": "en-US-AnaNeural",
    "VOICE": "en-NG-RogerNeural",
    "Guardian": "en-US-AdekunleNeural",
    # NPCs
    "PIT BOSS": "ru-RU-DmitriNeural",
    "MANAGER": "en-US-SteffanNeural",
    "UN AGENT": "en-GB-OliverNeural",
    "MERC 1": "en-NG-AbeoNeural",
}

VOICE_SETTINGS = {
    "en-GB-RyanNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-GuyNeural": {"rate": "+0%", "pitch": "-1Hz"},
    "es-MX-DaliaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-AvaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-ZA-LukeNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "ko-KR-SunHiNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "ja-JP-KeitaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-CoraNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-NG-EzinneNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-AshleyNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-LibbyNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-SteffanNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-ThomasNeural": {"rate": "+0%", "pitch": "-20Hz"},
    "en-AU-WilliamNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "en-NG-AbeoNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-OliverNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-IN-NeerjaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-RogerNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "en-US-ChristopherNeural": {"rate": "+0%", "pitch": "-50Hz"},
}


def clean_text_for_pauses(text: str) -> str:
    """Normalizes whitespace: replaces any whitespace (newlines, tabs, multiple spaces) with single spaces."""
    text = re.sub(r'\s+', ' ', text)
    return text.strip()


def clean_text(text: str) -> str:
    """Cleans text by removing markdown characters **, *, _, `."""
    text = text.replace('**', '')
    text = text.replace('*', '').replace('_', '').replace('`', '')
    return text


def parse_scene(text: str):
    """Parse Family A format: <center>NAME</center> + > dialogue."""
    segments = []
    current_speaker = "Narrator"
    lines = text.splitlines()

    i = 0
    while i < len(lines):
        line = lines[i].strip()

        # Handle center tag (speaker name)
        center_match = re.search(r'<center>([^<]+)</center>', lines[i])
        if center_match:
            name = center_match.group(1).strip().upper()
            current_speaker = "Narrator"

            # Find matching voice key (case-insensitive)
            for key in VOICE_MAP:
                if key.upper() in name or name in key.upper():
                    current_speaker = key
                    break
            else:
                current_speaker = name  # fallback

            i += 1
            continue

        # Handle dialogue lines starting with >
        if line.startswith('> '):
            dialogue = line[2:].strip()

            # Skip parenthetical asides like (action)
            if dialogue.startswith('(') and dialogue.endswith(')'):
                i += 1
                continue

            cleaned_dialogue = clean_text(dialogue)
            normalized_dialogue = clean_text_for_pauses(cleaned_dialogue)

            if normalized_dialogue:
                # Merge with previous segment if same speaker
                if segments and segments[-1][0] == current_speaker:
                    segments[-1] = (current_speaker, segments[-1][1] + " " + normalized_dialogue)
                else:
                    segments.append((current_speaker, normalized_dialogue))

        elif line == '':
            # Blank line resets to Narrator for potential narration
            current_speaker = "Narrator"

        # Else: ignore other lines (non-dialogue, non-center)

        i += 1

    # Final merge pass (just in case)
    final_segments = []
    for speaker, dialogue in segments:
        if final_segments and final_segments[-1][0] == speaker:
            final_segments[-1] = (speaker, final_segments[-1][1] + " " + dialogue)
        else:
            final_segments.append((speaker, dialogue))

    return final_segments


async def generate_speech(text: str, voice: str, output_path: str):
    settings = VOICE_SETTINGS.get(voice, {"rate": "+0%", "pitch": "+0Hz"})
    MAX_RETRIES = 5
    BASE_BACKOFF_SECONDS = 10

    for attempt in range(MAX_RETRIES + 1):
        try:
            communicate = edge_tts.Communicate(
                text, 
                voice, 
                rate=settings["rate"], 
                pitch=settings["pitch"]
            )
            await communicate.save(output_path)
            return True
        except Exception as e:
            error_message = str(e)
            if "No audio was received" in error_message and attempt < MAX_RETRIES:
                delay = BASE_BACKOFF_SECONDS * (1 << attempt)
                print(f" ⚠️ Attempt {attempt + 1}/{MAX_RETRIES + 1}: Retrying in {delay}s for {voice}")
                await asyncio.sleep(delay)
            else:
                print(f" ❌ Error generating speech for {voice}: {e}")
                return False

    return False


async def convert_scene(scene_file: str, scene_name: str):
    output_mp3 = os.path.join(OUTPUT_DIR, f"{scene_name}.mp3")

    if os.path.exists(output_mp3):
        print(f"\n{'='*60}")
        print(f"ℹ️ Skipping: {scene_name} (already exists)")
        print(f"{'='*60}")
        return False

    print(f"\n{'='*60}")
    print(f"🎬 Converting: {scene_name}")
    print(f"{'='*60}")

    with open(scene_file, 'r', encoding='utf-8') as f:
        text = f.read()

    segments = parse_scene(text)
    print(f" Parsed {len(segments)} segments")

    if not segments:
        print(" ⚠️ No dialogue segments found. Skipping.")
        return False

    scene_output_dir = os.path.join(OUTPUT_DIR, scene_name)
    os.makedirs(scene_output_dir, exist_ok=True)

    segment_files = []
    for i, (speaker, seg_text) in enumerate(segments):
        voice = VOICE_MAP.get(speaker, VOICE_MAP.get("Narrator"))
        seg_path = os.path.join(scene_output_dir, f"seg_{i:03d}.mp3")

        print(f" [{i+1}/{len(segments)}] {speaker} → {voice}")
        success = await generate_speech(seg_text, voice, seg_path)

        if success:
            segment_files.append(seg_path)

    if segment_files:
        # Concatenate using ffmpeg
        list_file = os.path.join(scene_output_dir, "filelist.txt")
        with open(list_file, 'w', encoding='utf-8') as f:
            for seg_path in segment_files:
                f.write(f"file '{os.path.basename(seg_path)}'\n")

        final_output = os.path.join(OUTPUT_DIR, f"{scene_name}.mp3")
        os.system(f'ffmpeg -y -f concat -safe 0 -i "{list_file}" -c copy "{final_output}"')

        # Cleanup
        for seg_path in segment_files:
            if os.path.exists(seg_path):
                os.remove(seg_path)
        if os.path.exists(list_file):
            os.remove(list_file)
        if os.path.exists(scene_output_dir):
            try:
                os.rmdir(scene_output_dir)
            except OSError:
                import shutil
                shutil.rmtree(scene_output_dir, ignore_errors=True)

        size_mb = os.path.getsize(final_output) / (1024 * 1024)
        print(f" ✅ Done: {final_output} ({size_mb:.1f} MB)")
        return True
    else:
        print(" ⚠️ No segments generated.")
        return False


async def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    scene_files = sorted([
        f for f in os.listdir(SCENE_DIR)
        if f.startswith('scene_') and f.endswith('.txt')
    ])

    if not scene_files:
        print("No scene files found!")
        return

    print(f"Found {len(scene_files)} scenes to convert")

    # Allow running specific scenes via command line
    if len(sys.argv) > 1:
        target = sys.argv[1:]
        scene_files = [f for f in scene_files if f.replace('.txt', '') in target or f in target]
        print(f"Filtered to {len(scene_files)} scenes")

    success_count = 0
    for scene_file in scene_files:
        scene_name = scene_file.replace('.txt', '')
        scene_path = os.path.join(SCENE_DIR, scene_file)
        if await convert_scene(scene_path, scene_name):
            success_count += 1

    print(f"\n{'='*60}")
    print(f"✅ Finished! {success_count}/{len(scene_files)} scenes converted")
    print(f"{'='*60}")


if __name__ == "__main__":
    asyncio.run(main())
