#!/usr/bin/env python3
"""
Convert ALL Red Rock & Roll screenplay scenes to multi-voice audio.
Handles Family A format: <center>NAME</center> + > dialogue
"""

import asyncio
import edge_tts
import os
import re
import glob

SCENE_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+14/SP.all-writeez28-b2/2.RedRock.AndROLL]ING]mid]a1"
OUTPUT_DIR = os.path.join(SCENE_DIR, "audio")

VOICE_MAP = {
    "Narrator": "en-GB-RyanNeural",
    "LUCKY": "en-US-GuyNeural",
    "LUCKY STARR": "en-US-GuyNeural",
    "GEM": "en-US-AvaNeural",
    "VANCE": "en-US-MichelleNeural",
    "RATCHET": "en-US-SteffanNeural",
    "QUEEN": "en-NG-EzinneNeural",
    "RED QUEEN": "en-NG-EzinneNeural",
    "ZARA": "en-GB-SoniaNeural",
    "COMMANDER 1": "en-US-JasonNeural",
    "COMMANDER 2": "en-GB-OliverNeural",
    "COMMANDER 3": "en-US-JasonNeural",
    "GUARD": "en-GB-AlfieNeural",
    "MERC 1": "en-GB-OliverNeural",
    "MERC 2": "en-KE-ChilembaNeural",
    "ENVOY": "en-GB-MaisieNeural",
    "PILOT": "en-US-SteffanNeural",
    "DOCTOR": "en-IN-NeerjaNeural",
    "NURSE": "en-CA-ClaraNeural",
    "ANNOUNCER": "en-US-SteffanNeural",
    "BARTENDER": "en-GB-AlfieNeural",
    "CLONE": "en-US-JasonNeural",
}

VOICE_SETTINGS = {
    "en-GB-RyanNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-GuyNeural": {"rate": "+0%", "pitch": "-1Hz"},
    "en-US-AvaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-MichelleNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-JasonNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-NG-EzinneNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-SoniaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-OliverNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-AlfieNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "en-KE-ChilembaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-MaisieNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-SteffanNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-IN-NeerjaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-CA-ClaraNeural": {"rate": "+0%", "pitch": "+0Hz"},
}

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
            matched = False
            for key in VOICE_MAP:
                if key in name or name in key:
                    current_speaker = key
                    matched = True
                    break
            if not matched:
                current_speaker = name
            continue
        
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
            await asyncio.sleep(1)  # Longer delay between segments
            return
        except Exception as e:
            if attempt < retries - 1:
                await asyncio.sleep(5)  # Longer retry delay
            else:
                raise

async def convert_scene(scene_file, output_mp3):
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
        try:
            await generate_speech(seg_text, voice, seg_path)
            segment_files.append(seg_path)
        except Exception as e:
            print(f"    ⚠️ Failed segment {i+1} ({speaker}), skipping...")
            continue
    
    if segment_files:
        list_file = output_mp3.replace('.mp3', '_filelist.txt')
        with open(list_file, 'w') as f:
            for seg_path in segment_files:
                f.write(f"file '{seg_path}'\n")
        
        os.system(f'ffmpeg -y -f concat -safe 0 -i "{list_file}" -c copy "{output_mp3}" 2>/dev/null')
        
        for seg_path in segment_files:
            os.remove(seg_path)
        if os.path.exists(list_file):
            os.remove(list_file)
        
        size_mb = os.path.getsize(output_mp3) / (1024 * 1024)
        print(f"  ✅ {os.path.basename(output_mp3)} ({size_mb:.1f} MB, {len(segment_files)} segments)")
        return True
    
    return False

async def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    await asyncio.sleep(10)  # Longer initial delay
    
    scene_files = sorted(glob.glob(os.path.join(SCENE_DIR, "scene_*.txt")))
    
    print(f"Converting {len(scene_files)} Red Rock & Roll scenes...")
    
    for scene_file in scene_files:
        basename = os.path.basename(scene_file).replace('.txt', '').replace('scene_', 'redrock-scene-').replace('_INT_', '-').replace('_EXT_', '-').replace('_DAY', '').replace('_NIGHT', '').replace('_MONTAGE', '').lower()
        while '--' in basename:
            basename = basename.replace('--', '-')
        if basename.endswith('-'):
            basename = basename[:-1]
        output_mp3 = os.path.join(OUTPUT_DIR, f"{basename}.mp3")
        
        if os.path.exists(output_mp3):
            print(f"  ⏭️ Skipping {basename}.mp3 (already exists)")
            continue
        
        print(f"  🔄 Converting {os.path.basename(scene_file)}...")
        success = await convert_scene(scene_file, output_mp3)
        if success:
            await asyncio.sleep(3)  # Longer delay between scenes
    
    # Copy to iphone folder
    iphone_dir = f"/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+14/@.sp.iphone-MAC中/SP.all-writeez28-b2/2.RedRock.AndROLL]ING]mid]a1/audio"
    os.makedirs(iphone_dir, exist_ok=True)
    os.system(f'cp "{OUTPUT_DIR}"/*.mp3 "{iphone_dir}/"')
    print(f"\n✅ Copied to iphone folder: {iphone_dir}")
    print(f"✅ Done! All Red Rock & Roll scenes converted.")

if __name__ == "__main__":
    asyncio.run(main())
