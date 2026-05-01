#!/usr/bin/env python3
"""
Convert ALL Death-Kryo! screenplay scenes to multi-voice audio.
Handles Family B format: ALL CAPS NAME + plain text dialogue
"""

import asyncio
import edge_tts
import os
import re
import glob

SCENE_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+14/SP.all-writeez28-b2/3.DEATH-KRYO!]big]b2"
OUTPUT_DIR = os.path.join(SCENE_DIR, "audio")

VOICE_MAP = {
    "Narrator": "en-GB-RyanNeural",
    "LUCKY": "en-US-GuyNeural",
    "GEM": "en-US-AvaNeural",
    "ASTRA": "es-MX-DaliaNeural",
    "SPIKE": "en-AU-WilliamNeural",
    "DORCHESTER": "en-IE-EmilyNeural",
    "RAVEN": "en-US-AshleyNeural",
    "QUEEN SOPHIA": "en-GB-MaisieNeural",
    "UN AGENT": "en-GB-OliverNeural",
    "NEW-KOREA GENERAL": "en-ZA-LukeNeural",
    "MER-ELDER": "en-IN-NeerjaNeural",
    "THE ARCHITECT": "en-US-ChristopherNeural",
    "SÁOIRSE": "en-IE-EmilyNeural",
    "SAOIRSE": "en-IE-EmilyNeural",
    "CARC-ZILLA": "en-GB-AlfieNeural",
    "CARCAZOID": "en-GB-AlfieNeural",
    "GUARD": "en-GB-AlfieNeural",
    "MERC": "en-US-JasonNeural",
    "PILOT": "en-US-SteffanNeural",
    "DOCTOR": "en-IN-NeerjaNeural",
    "ANNOUNCER": "en-US-SteffanNeural",
    "BARTENDER": "en-GB-AlfieNeural",
    "CLONE": "en-US-JasonNeural",
}

VOICE_SETTINGS = {
    "en-GB-RyanNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-GuyNeural": {"rate": "+0%", "pitch": "-1Hz"},
    "en-US-AvaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "es-MX-DaliaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-AU-WilliamNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "en-IE-OrlaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-AshleyNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-MaisieNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-OliverNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-ZA-LukeNeural": {"rate": "+0%", "pitch": "-1Hz"},
    "en-IN-NeerjaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-ChristopherNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "en-IE-EmilyNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-AlfieNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "en-US-JasonNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-SteffanNeural": {"rate": "+0%", "pitch": "+0Hz"},
}

def parse_scene(text):
    """Parse Family B format: ALL CAPS NAME + plain text dialogue"""
    segments = []
    current_speaker = "Narrator"
    current_text = []
    
    lines = text.split('\n')
    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()
        
        # Check for ALL CAPS name line (optionally with parenthetical)
        name_match = re.match(r'^([A-Z][A-Z\s\-\'\.]+?)(?:\s*\([^)]*\))?\s*$', stripped)
        if name_match and len(stripped) < 50:
            name = name_match.group(1).strip()
            # Skip if it's a scene heading or transition
            if name.startswith('INT.') or name.startswith('EXT.') or name.startswith('FADE') or name.startswith('SCENE') or name.startswith('SOUND') or name.startswith('CLOSE UP') or name.startswith('FLASHBACK') or name.startswith('END FLASHBACK') or name.startswith('MONTAGE') or name.startswith('THE END'):
                if current_text:
                    segments.append((current_speaker, ' '.join(current_text)))
                    current_text = []
                current_speaker = "Narrator"
                current_text.append(re.sub(r'\*\*', '', stripped))
                i += 1
                continue
            
            # Check if next line is a parenthetical or dialogue
            if current_text:
                segments.append((current_speaker, ' '.join(current_text)))
                current_text = []
            
            # Map name to voice
            matched = False
            for key in VOICE_MAP:
                if key in name or name in key:
                    current_speaker = key
                    matched = True
                    break
            if not matched:
                current_speaker = name
            
            i += 1
            continue
        
        # Check for character intro: ALL CAPS name followed by comma + prose
        intro_match = re.match(r'^([A-Z][A-Z\s\-\'\.]+?)\s*\(\d+s?\),', stripped)
        if intro_match:
            if current_text:
                segments.append((current_speaker, ' '.join(current_text)))
                current_text = []
            current_speaker = "Narrator"
            current_text.append(re.sub(r'\*\*', '', stripped))
            i += 1
            continue
        
        if stripped == '':
            if current_text:
                segments.append((current_speaker, ' '.join(current_text)))
                current_text = []
            current_speaker = "Narrator"
        else:
            clean = re.sub(r'\*\*', '', stripped)
            if clean:
                current_text.append(clean)
        
        i += 1
    
    if current_text:
        segments.append((current_speaker, ' '.join(current_text)))
    
    return segments

async def generate_speech(text, voice, output_path, retries=3):
    settings = VOICE_SETTINGS.get(voice, {"rate": "+0%", "pitch": "+0Hz"})
    for attempt in range(retries):
        try:
            communicate = edge_tts.Communicate(text, voice, rate=settings["rate"], pitch=settings["pitch"])
            await communicate.save(output_path)
            await asyncio.sleep(1)
            return
        except Exception as e:
            if attempt < retries - 1:
                await asyncio.sleep(5)
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
    await asyncio.sleep(10)
    
    scene_files = sorted(glob.glob(os.path.join(SCENE_DIR, "scene_*.txt")))
    
    print(f"Converting {len(scene_files)} Death-Kryo! scenes...")
    
    for scene_file in scene_files:
        basename = os.path.basename(scene_file).replace('.txt', '').replace('scene_', 'deathkryo-scene-').replace('_', '-').lower()
        output_mp3 = os.path.join(OUTPUT_DIR, f"{basename}.mp3")
        
        if os.path.exists(output_mp3):
            print(f"  ⏭️ Skipping {basename}.mp3 (already exists)")
            continue
        
        print(f"  🔄 Converting {os.path.basename(scene_file)}...")
        success = await convert_scene(scene_file, output_mp3)
        if success:
            await asyncio.sleep(3)
    
    # Copy to iphone folder
    iphone_dir = f"/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+14/@.sp.iphone-MAC中/SP.all-writeez28-b2/3.DEATH-KRYO!]big]b2/audio"
    os.makedirs(iphone_dir, exist_ok=True)
    os.system(f'cp "{OUTPUT_DIR}"/*.mp3 "{iphone_dir}/"')
    print(f"\n✅ Copied to iphone folder: {iphone_dir}")
    print(f"✅ Done! All Death-Kryo! scenes converted.")

if __name__ == "__main__":
    asyncio.run(main())
