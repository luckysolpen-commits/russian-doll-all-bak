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
    "CLONE SOLDIER": "en-US-JasonNeural",
    "SILVER-BAND CLONE": "en-US-JasonNeural",
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
    "VOICE": "en-NG-AdekunleNeural", # Changed from en-US-JasonNeural
    "Guardian": "en-US-RogerNeural",
    # NPCs
    "PIT BOSS": "ru-RU-DmitriNeural", # Changed from en-US-JasonNeural
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
    "en-US-JasonNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-SteffanNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-ThomasNeural": {"rate": "+0%", "pitch": "-20Hz"},
    "en-AU-WilliamNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "en-NG-AbeoNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-OliverNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-IN-NeerjaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-RogerNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "en-US-ChristopherNeural": {"rate": "+0%", "pitch": "-50Hz"},
}

def clean_text(text):
    """Cleans text by removing **, *, _, ` characters."""
    # Remove **
    text = text.replace('**', '')
    # Remove *, _, `
    text = text.replace('*', '').replace('_', '').replace('`', '')
    return text

def parse_scene(text):
    """Parse Family A format: <center>NAME</center> + > dialogue"""
    segments = []
    current_speaker = "Narrator"
    current_text = []

    for line in text.split('\n'):
        center_match = re.search(r'<center>([^<]+)</center>', line)
        if center_match:
            if current_text:
                segments.append((current_speaker, ' '.join(current_text)))
                current_text = []
            name = center_match.group(1).strip().upper()
            for key in VOICE_MAP:
                if key in name or name in key:
                    current_speaker = key
                    break
            else:
                current_speaker = name
            continue

        if line.strip().startswith('> '):
            dialogue = line.strip()[2:]
            if dialogue.startswith('(') and dialogue.endswith(')'):
                continue
            # Apply text cleaning to the dialogue
            cleaned_dialogue = clean_text(dialogue)
            if cleaned_dialogue: # Only append if not empty after cleaning
                current_text.append(cleaned_dialogue)
        elif line.strip() == '':
            if current_text:
                segments.append((current_speaker, ' '.join(current_text)))
                current_text = []
            current_speaker = "Narrator"
        else:
            # This block seems to be for non-dialogue lines and currently has incomplete cleaning.
            # For consistency, we'll apply cleaning here too, though it might not be strictly necessary for dialogue.
            # If this were for narration, it might be different. Given it's `else`, it might be part of dialogue if not starting with '> '.
            # Based on C code, cleaning is applied to dialogue. Let's refine:
            # If it's not a center tag, not a blank line, and not starting with '>', it's likely part of a multi-line dialogue segment.
            # The original code's `clean = re.sub(r'\*\*', '', line.strip())` and `current_text.append(clean)` in the `else` block was strange.
            # Let's ensure dialogue lines are cleaned.
            pass # The cleaning is now applied to dialogue lines directly.

    if current_text:
        segments.append((current_speaker, ' '.join(current_text)))

    return segments

async def generate_speech(text, voice, output_path):
    settings = VOICE_SETTINGS.get(voice, {"rate": "+0%", "pitch": "+0Hz"})
    MAX_RETRIES = 5
    BASE_BACKOFF_SECONDS = 2 # Initial backoff in seconds, base for exponential calculation

    for attempt in range(MAX_RETRIES + 1): # Total attempts = initial call + MAX_RETRIES
        try:
            communicate = edge_tts.Communicate(text, voice, rate=settings["rate"], pitch=settings["pitch"])
            await communicate.save(output_path)
            return True
        except Exception as e:
            error_message = str(e)
            # Check for the specific error message indicating rate limits or transient issues
            if "No audio was received. Please verify that your parameters are correct." in error_message and attempt < MAX_RETRIES:
                # Exponential backoff: BASE_BACKOFF * 2^attempt (attempt is 0-indexed retry number)
                delay = BASE_BACKOFF_SECONDS * (1 << attempt) 
                print(f"    ⚠️ Attempt {attempt + 1}/{MAX_RETRIES + 1}: Rate limit or transient error encountered for {voice}. Retrying in {delay} seconds...")
                await asyncio.sleep(delay)
            else:
                print(f"    ⚠️ Error: {e}")
                return False # Return False on non-retryable error or after max retries

    return False # Should not be reached if MAX_RETRIES >= 0, but for safety
async def convert_scene(scene_file, scene_name):
    """Convert a single scene to audio."""
    
    output_mp3 = os.path.join(OUTPUT_DIR, f"{scene_name}.mp3")
    
    if os.path.exists(output_mp3):
        print(f"\n{'='*60}")
        print(f"ℹ️ Skipping: {scene_name} (File already exists: {output_mp3})")
        print(f"{'='*60}")
        return False # Indicate that conversion was skipped

    print(f"\n{'='*60}")
    print(f"🎬 Converting: {scene_name}")
    print(f"{'='*60}")

    with open(scene_file, 'r', encoding='utf-8') as f:
        text = f.read()

    segments = parse_scene(text)
    print(f"  Parsed {len(segments)} segments")

    if not segments:
        print("  ⚠️ No dialogue segments found. Skipping.")
        return False

    scene_output_dir = os.path.join(OUTPUT_DIR, scene_name)
    os.makedirs(scene_output_dir, exist_ok=True)

    segment_files = []
    success_count = 0
    for i, (speaker, seg_text) in enumerate(segments):
        voice = VOICE_MAP.get(speaker, VOICE_MAP.get("Narrator"))
        seg_path = os.path.join(scene_output_dir, f"seg_{i:03d}.mp3")
        print(f"  [{i+1}/{len(segments)}] {speaker} → {voice}")
        if voice == "en-US-JasonNeural":
            print(f"    DEBUG: Text for {voice}: '{seg_text}'")
        success = await generate_speech(seg_text, voice, seg_path)
        if success:
            segment_files.append(seg_path)
            success_count += 1

    if segment_files:
        list_file = os.path.join(scene_output_dir, "filelist.txt")
        with open(list_file, 'w') as f:
            for seg_path in segment_files:
                f.write(f"file '{os.path.basename(seg_path)}'\n")

        output_mp3 = os.path.join(OUTPUT_DIR, f"{scene_name}.mp3")
        os.system(f'ffmpeg -y -f concat -safe 0 -i "{list_file}" -c copy "{output_mp3}"')

        # Clean up segment files
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

        size_mb = os.path.getsize(output_mp3) / (1024 * 1024)
        print(f"  ✅ Output: {output_mp3} ({size_mb:.1f} MB)")
        return True
    else:
        print("  ⚠️ No segments generated. Skipping.")
        return False

async def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    # Find all scene files
    scene_files = sorted([
        f for f in os.listdir(SCENE_DIR)
        if f.startswith('scene_') and f.endswith('.txt')
    ])

    if not scene_files:
        print("No scene files found!")
        return

    print(f"Found {len(scene_files)} scenes to convert")

    # Allow specifying specific scenes
    if len(sys.argv) > 1:
        target_scenes = sys.argv[1:]
        scene_files = [f for f in scene_files if f.replace('.txt', '') in target_scenes or f in target_scenes]
        print(f"Filtering to {len(scene_files)} specified scenes")

    success_count = 0
    for scene_file in scene_files:
        scene_name = scene_file.replace('.txt', '')
        scene_path = os.path.join(SCENE_DIR, scene_file)
        if await convert_scene(scene_path, scene_name):
            success_count += 1

    print(f"\n{'='*60}")
    print(f"✅ Done! {success_count}/{len(scene_files)} scenes converted")
    print(f"{'='*60}")

if __name__ == "__main__":
    asyncio.run(main())
