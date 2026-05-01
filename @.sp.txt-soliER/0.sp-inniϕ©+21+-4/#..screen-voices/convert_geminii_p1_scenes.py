#!/usr/bin/env python3
"""
Convert ALL Geminii Part 1 screenplay scenes to multi-voice audio.
Handles Family A format: <center>NAME</center> + > dialogue
"""

import asyncio
import edge_tts
import os
import re
import glob

SCENE_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+14/SP.all-writeez28-b2/1.ii.geminii]2part]a0/1.GEMINI_PART_1]Tarantin0"
OUTPUT_DIR = os.path.join(SCENE_DIR, "audio")

VOICE_MAP = {
    "Narrator": "en-GB-RyanNeural",
    "LUCKY": "en-US-GuyNeural",
    "ASTRA": "es-MX-DaliaNeural",
    "GEM": "en-US-AvaNeural",
    "ELARA": "en-GB-LibbyNeural",
    "LYRA": "en-GB-MaisieNeural",
}

VOICE_SETTINGS = {
    "en-GB-RyanNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-GuyNeural": {"rate": "+0%", "pitch": "-1Hz"},
    "es-MX-DaliaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-AvaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-BellaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-LibbyNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-MaisieNeural": {"rate": "+0%", "pitch": "+0Hz"},
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
            await asyncio.sleep(0.5)  # Small delay between segments
            return
        except Exception as e:
            if attempt < retries - 1:
                await asyncio.sleep(3)
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
    
    # Initial delay to let rate limits reset
    await asyncio.sleep(5)
    
    scene_files = sorted(glob.glob(os.path.join(SCENE_DIR, "scene_*.txt")))
    
    print(f"Converting {len(scene_files)} Geminii Part 1 scenes...")
    
    for scene_file in scene_files:
        basename = os.path.basename(scene_file).replace('.txt', '').replace('scene_', 'gemini-p1-scene-').replace('_GEM_I_Martian_Gambit_', '-').replace('EXT_', '').replace('INT_', '').replace('CONTINUOUS', '').lower()
        # Clean up double dashes
        while '--' in basename:
            basename = basename.replace('--', '-')
        output_mp3 = os.path.join(OUTPUT_DIR, f"{basename}.mp3")
        
        if os.path.exists(output_mp3):
            print(f"  ⏭️ Skipping {basename}.mp3 (already exists)")
            continue
        
        print(f"  🔄 Converting {os.path.basename(scene_file)}...")
        success = await convert_scene(scene_file, output_mp3)
        if success:
            await asyncio.sleep(2)  # Longer delay to avoid rate limits
    
    # Copy to iphone folder
    iphone_dir = f"/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+14/@.sp.iphone-MAC中/SP.all-writeez28-b2/1.ii.geminii]2part]a0/1.GEMINI_PART_1]Tarantin0/audio"
    os.makedirs(iphone_dir, exist_ok=True)
    os.system(f'cp "{OUTPUT_DIR}"/*.mp3 "{iphone_dir}/"')
    print(f"\n✅ Copied to iphone folder: {iphone_dir}")
    print(f"✅ Done! All Geminii Part 1 scenes converted.")

if __name__ == "__main__":
    asyncio.run(main())
