#!/usr/bin/env python3
"""
Convert ALL Sol Pen screenplay scenes to multi-voice audio.
Handles Family A format: <center>NAME</center> + > dialogue
"""

import asyncio
import edge_tts
import os
import re
import glob
import time

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
    "DORCHESTER": "en-IE-OrlaNeural",
    "EDGE": "en-AU-WilliamNeural",
    "DANGERMAN": "en-AU-WilliamNeural",
    "SAORSI": "en-IE-EmilyNeural",
    "KAITO": "ja-JP-KeitaNeural",
    "AGENT THOMPSON": "en-US-MichelleNeural",
    "BLACK QUEEN": "en-NG-EzinneNeural",
    "PRINCESS MEGAN8R": "en-US-JennyNeural",
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
    "MERC 2": "en-KE-ChilembaNeural",
    "MERC 3": "en-US-JasonNeural",
    "PIT BOSS": "en-US-SteffanNeural",
    "DIGNITARY": "en-GB-OliverNeural",
}

VOICE_SETTINGS = {
    "en-GB-RyanNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-GuyNeural": {"rate": "+0%", "pitch": "-1Hz"},
    "es-MX-DaliaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-AvaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "ko-KR-SunHiNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-ZA-LukeNeural": {"rate": "+0%", "pitch": "-1Hz"},
    "es-ES-ElviraNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-AU-WilliamNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "en-IE-OrlaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-IE-EmilyNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "ja-JP-KeitaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-CoraNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-MichelleNeural": {"rate": "+0%", "pitch": "+0Hz"},
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
    "en-KE-ChilembaNeural": {"rate": "+0%", "pitch": "+0Hz"},
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
            if current_text:
                segments.append((current_speaker, ' '.join(current_text)))
                current_text = []
            name = center_match.group(1).strip().upper()
            # Map partial names
            matched = False
            for key in VOICE_MAP:
                if key in name or name in key:
                    current_speaker = key
                    matched = True
                    break
            if not matched:
                current_speaker = name
            continue
        
        # Check for > dialogue lines
        if line.strip().startswith('> '):
            dialogue = line.strip()[2:]
            if dialogue.startswith('(') and dialogue.endswith(')'):
                continue
            current_text.append(dialogue)
        elif line.strip() == '':
            if current_text:
                segments.append((current_speaker, ' '.join(current_text)))
                current_text = []
            current_speaker = "Narrator"
        else:
            clean = re.sub(r'\*\*', '', line.strip())
            if clean:
                current_text.append(clean)
    
    if current_text:
        segments.append((current_speaker, ' '.join(current_text)))
    
    return segments

async def generate_speech(text, voice, output_path, retries=3):
    settings = VOICE_SETTINGS.get(voice, {"rate": "+0%", "pitch": "+0Hz"})
    for attempt in range(retries):
        try:
            communicate = edge_tts.Communicate(text, voice, rate=settings["rate"], pitch=settings["pitch"])
            await communicate.save(output_path)
            return
        except Exception as e:
            if attempt < retries - 1:
                print(f"    ⚠️ Retry {attempt+1}/{retries} for {voice}: {e}")
                await asyncio.sleep(2)
            else:
                print(f"    ❌ Failed after {retries} retries for {voice}: {e}")
                raise

async def convert_scene(scene_file, output_mp3):
    """Convert a single scene file to MP3"""
    with open(scene_file, 'r', encoding='utf-8') as f:
        text = f.read()
    
    segments = parse_scene(text)
    if not segments:
        print(f"  ⚠️ No segments found in {os.path.basename(scene_file)}")
        return False
    
    segment_files = []
    for i, (speaker, seg_text) in enumerate(segments):
        voice = VOICE_MAP.get(speaker, VOICE_MAP.get("Narrator"))
        seg_path = output_mp3.replace('.mp3', f'_seg_{i:03d}.mp3')
        await generate_speech(seg_text, voice, seg_path)
        segment_files.append(seg_path)
    
    if segment_files:
        list_file = output_mp3.replace('.mp3', '_filelist.txt')
        with open(list_file, 'w') as f:
            for seg_path in segment_files:
                f.write(f"file '{seg_path}'\n")
        
        os.system(f'ffmpeg -y -f concat -safe 0 -i "{list_file}" -c copy "{output_mp3}" 2>/dev/null')
        
        for seg_path in segment_files:
            os.remove(seg_path)
        os.remove(list_file)
        
        size_mb = os.path.getsize(output_mp3) / (1024 * 1024)
        print(f"  ✅ {os.path.basename(output_mp3)} ({size_mb:.1f} MB, {len(segments)} segments)")
        return True
    
    return False

async def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    # Get all scene files, sorted by scene number
    scene_files = sorted(glob.glob(os.path.join(SCENE_DIR, "scene_*.txt")))
    
    # Skip scene 03 (already done)
    scene_files = [f for f in scene_files if 'scene_03' not in f]
    
    print(f"Converting {len(scene_files)} scenes...")
    
    for scene_file in scene_files:
        basename = os.path.basename(scene_file).replace('.txt', '').replace('scene_', 'sol-pen-scene-').replace('_', '-')
        output_mp3 = os.path.join(OUTPUT_DIR, f"{basename}.mp3")
        
        if os.path.exists(output_mp3):
            print(f"  ⏭️ Skipping {basename}.mp3 (already exists)")
            continue
        
        print(f"  🔄 Converting {os.path.basename(scene_file)}...")
        success = await convert_scene(scene_file, output_mp3)
        if success:
            await asyncio.sleep(1)  # Delay between scenes to avoid rate limits
    
    print(f"\n✅ Done! All scenes converted.")

if __name__ == "__main__":
    asyncio.run(main())
