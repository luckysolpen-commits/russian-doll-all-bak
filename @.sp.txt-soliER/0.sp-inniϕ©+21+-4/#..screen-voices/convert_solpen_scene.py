#!/usr/bin/env python3
"""
Convert Sol Pen screenplay scenes to multi-voice audio.
Handles Family A format: <center>NAME</center> + > dialogue
"""

import asyncio
import edge_tts
import os
import re

SCENE_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+14/SP.all-writeez28-b2/0.Sol Pen]bk0/0.scenes]ALL+named"
OUTPUT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+14/screen-voices/sol-pen"

VOICE_MAP = {
    "Narrator": "en-GB-RyanNeural",
    "LUCKY": "en-US-GuyNeural",
    "ASTRA": "es-MX-DaliaNeural",
    "GEM": "en-US-AvaNeural",
    "LOONA": "ko-KR-SunHiNeural",
    "SOL": "en-ZA-LukeNeural",
    "MARIA": "es-ES-ElviraNeural",
    "SPIKE": "en-AU-WilliamNeural",
    "DORCHESTER": "en-GB-ThomasNeural",
    "KAITO": "ja-JP-KeitaNeural",
    "AGENT THOMPSON": "en-US-CoraNeural",
    "BLACK QUEEN": "en-NG-EzinneNeural",
    "PRINCESS MEGAN8R": "en-US-AshleyNeural",
    "QUEEN SOPHIA": "en-GB-MaisieNeural",
    "BORIN IRONMUG": "en-GB-AlfieNeural",
    "Guardian": "en-US-RogerNeural",
    "Fuzzball Priestess": "en-IN-NeerjaNeural",
    "GoldQueen_Astra": "en-GB-LibbyNeural",
    # NPCs
    "PIT BOSS": "en-US-JasonNeural",
    "MANAGER": "en-US-SteffanNeural",
    "UN AGENT": "en-GB-OliverNeural",
    "MERC 1": "en-NG-AbeoNeural",
}

VOICE_SETTINGS = {
    "en-GB-RyanNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-GuyNeural": {"rate": "+0%", "pitch": "-1Hz"},
    "es-MX-DaliaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-AvaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-IE-EmilyNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-ZA-LukeNeural": {"rate": "+0%", "pitch": "-1Hz"},
    "es-ES-ElviraNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-AU-WilliamNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "en-GB-ThomasNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "ko-KR-SunHiNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "ja-JP-NanamiNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "ja-JP-KeitaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-CoraNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-NG-EzinneNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-AshleyNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-MaisieNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-AlfieNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "en-US-RogerNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "en-IN-NeerjaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-LibbyNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-JasonNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-SteffanNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-OliverNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-NG-AbeoNeural": {"rate": "+0%", "pitch": "+0Hz"},
}

def parse_scene(text):
    """Parse Family A format: <center>NAME</center> + > dialogue"""
    segments = []
    current_speaker = "Narrator"
    current_text = []
    
    for line in text.split('\n'):
        # Check for <center>NAME</center>
        center_match = re.search(r'<center>([^<]+)</center>', line)
        if center_match:
            # Save previous segment
            if current_text:
                segments.append((current_speaker, ' '.join(current_text)))
                current_text = []
            # Switch speaker
            name = center_match.group(1).strip().upper()
            # Map partial names
            for key in VOICE_MAP:
                if key in name or name in key:
                    current_speaker = key
                    break
            else:
                current_speaker = name
            continue
        
        # Check for > dialogue lines
        if line.strip().startswith('> '):
            dialogue = line.strip()[2:]
            # Skip parentheticals
            if dialogue.startswith('(') and dialogue.endswith(')'):
                continue
            current_text.append(dialogue)
        elif line.strip() == '':
            # Blank line - save current segment
            if current_text:
                segments.append((current_speaker, ' '.join(current_text)))
                current_text = []
            current_speaker = "Narrator"
        else:
            # Narration / action line
            clean = re.sub(r'\*\*', '', line.strip())
            if clean:
                # Check if this is a character intro (ALL CAPS name followed by comma + prose)
                intro_match = re.match(r'^([A-Z][A-Z\s]+)\s*\(\d+s?\),', clean)
                if intro_match:
                    # Character intro - narrator reads it
                    current_text.append(clean)
                else:
                    current_text.append(clean)
    
    # Don't forget the last segment
    if current_text:
        segments.append((current_speaker, ' '.join(current_text)))
    
    return segments

async def generate_speech(text, voice, output_path):
    settings = VOICE_SETTINGS.get(voice, {"rate": "+0%", "pitch": "+0Hz"})
    communicate = edge_tts.Communicate(text, voice, rate=settings["rate"], pitch=settings["pitch"])
    await communicate.save(output_path)

async def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    # Process scene_03 as test
    scene_file = os.path.join(SCENE_DIR, "scene_03_mysterious_package.txt")
    with open(scene_file, 'r', encoding='utf-8') as f:
        text = f.read()
    
    segments = parse_scene(text)
    print(f"Parsed {len(segments)} segments from scene_03")
    
    segment_files = []
    for i, (speaker, seg_text) in enumerate(segments):
        voice = VOICE_MAP.get(speaker, VOICE_MAP.get("Narrator"))
        seg_path = os.path.join(OUTPUT_DIR, f"solpen_s03_seg_{i:03d}.mp3")
        print(f"  [{i+1}/{len(segments)}] {speaker} → {voice}")
        await generate_speech(seg_text, voice, seg_path)
        segment_files.append(seg_path)
    
    if segment_files:
        list_file = os.path.join(OUTPUT_DIR, "solpen_s03_filelist.txt")
        with open(list_file, 'w') as f:
            for seg_path in segment_files:
                f.write(f"file '{seg_path}'\n")
        
        output_mp3 = os.path.join(OUTPUT_DIR, "sol-pen-scene-03-mysterious-package.mp3")
        os.system(f'ffmpeg -y -f concat -safe 0 -i "{list_file}" -c copy "{output_mp3}"')
        print(f"\n✅ Output: {output_mp3}")
        
        for seg_path in segment_files:
            os.remove(seg_path)
        os.remove(list_file)
        
        size_mb = os.path.getsize(output_mp3) / (1024 * 1024)
        print(f"📊 File size: {size_mb:.1f} MB")

if __name__ == "__main__":
    asyncio.run(main())
