#!/usr/bin/env python3
"""
Universal Screenplay Converter — converts ALL remaining books.
Auto-detects Family A (<center>NAME</center>) vs Family B (ALL CAPS NAME) format.
"""

import asyncio
import edge_tts
import os
import re
import glob
import time

BASE = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+14/SP.all-writeez28-b2"
IPHONE_BASE = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+14/@.sp.iphone-MAC中/SP.all-writeez28-b2"

# Voice maps per book
BOOKS = {
    "4.moonshine]big]b1": {
        "prefix": "moonshine",
        "voices": {
            "Narrator": "en-GB-RyanNeural",
            "LUCKY": "en-US-GuyNeural",
            "ASTRA": "es-MX-DaliaNeural",
            "GEM": "en-US-AvaNeural",
            "EDGE": "en-AU-WilliamNeural",
            "DANGERMAN": "en-AU-WilliamNeural",
            "EDGE DANGERMAN": "en-AU-WilliamNeural",
            "LOONA": "ko-KR-SunHiNeural",
            "LOONA TWILIGHT": "ko-KR-SunHiNeural",
            "SAOIRSE": "en-IE-EmilyNeural",
            "SÁOIRSE": "en-IE-EmilyNeural",
            "DIABOLUS": "ar-AE-HamdanNeural",
            "ARCHITECT": "en-US-ChristopherNeural",
            "CAPTAIN": "en-ZA-LukeNeural",
            "GUARD": "en-GB-AlfieNeural",
            "SOLDIER": "en-US-JasonNeural",
            "PILOT": "en-US-SteffanNeural",
            "DOCTOR": "en-IN-NeerjaNeural",
            "ANNOUNCER": "en-US-SteffanNeural",
            "MERC": "en-GB-OliverNeural",
            "CLONE": "en-US-JasonNeural",
            "DORCHESTER": "en-IE-EmilyNeural",
            "SPIKE": "en-AU-WilliamNeural",
            "RAVEN": "en-US-AshleyNeural",
            "QUEEN SOPHIA": "en-GB-MaisieNeural",
            "UN AGENT": "en-GB-OliverNeural",
            "NEW-KOREA GENERAL": "en-ZA-LukeNeural",
            "MER-ELDER": "en-IN-NeerjaNeural",
        },
    },
    "5.SUPERWAR.ikrs]FULwc]bk0": {
        "prefix": "superwar",
        "voices": {
            "Narrator": "en-GB-RyanNeural",
            "LUCKY": "en-US-GuyNeural",
            "ASTRA": "es-MX-DaliaNeural",
            "GEM": "en-US-AvaNeural",
            "EDGE": "en-AU-WilliamNeural",
            "DANGERMAN": "en-AU-WilliamNeural",
            "EDGE DANGERMAN": "en-AU-WilliamNeural",
            "LOONA": "ko-KR-SunHiNeural",
            "LOONA TWILIGHT": "ko-KR-SunHiNeural",
            "SAOIRSE": "en-IE-EmilyNeural",
            "SÁOIRSE": "en-IE-EmilyNeural",
            "DIABOLUS": "ar-AE-HamdanNeural",
            "ARCHITECT": "en-US-ChristopherNeural",
            "SOL": "en-US-ChristopherNeural",
            "SUPREME EMPEROR SOL": "en-US-ChristopherNeural",
            "CAPTAIN": "en-ZA-LukeNeural",
            "GUARD": "en-GB-AlfieNeural",
            "SOLDIER": "en-US-JasonNeural",
            "PILOT": "en-US-SteffanNeural",
            "DOCTOR": "en-IN-NeerjaNeural",
            "ANNOUNCER": "en-US-SteffanNeural",
            "MERC": "en-GB-OliverNeural",
            "CLONE": "en-US-JasonNeural",
            "DORCHESTER": "en-IE-EmilyNeural",
            "SPIKE": "en-AU-WilliamNeural",
            "RAVEN": "en-US-AshleyNeural",
            "QUEEN SOPHIA": "en-GB-MaisieNeural",
            "UN AGENT": "en-GB-OliverNeural",
            "NEW-KOREA GENERAL": "en-ZA-LukeNeural",
            "MER-ELDER": "en-IN-NeerjaNeural",
        },
    },
    "6.hellride-bk0]BIG]b1": {
        "prefix": "hellride",
        "voices": {
            "Narrator": "en-GB-RyanNeural",
            "LUCKY": "en-US-GuyNeural",
            "ASTRA": "es-MX-DaliaNeural",
            "GEM": "en-US-AvaNeural",
            "EDGE": "en-AU-WilliamNeural",
            "DANGERMAN": "en-AU-WilliamNeural",
            "EDGE DANGERMAN": "en-AU-WilliamNeural",
            "LOONA": "ko-KR-SunHiNeural",
            "LOONA TWILIGHT": "ko-KR-SunHiNeural",
            "SAOIRSE": "en-IE-EmilyNeural",
            "SÁOIRSE": "en-IE-EmilyNeural",
            "DIABOLUS": "ar-AE-HamdanNeural",
            "ARCHITECT": "en-US-ChristopherNeural",
            "CAPTAIN": "en-ZA-LukeNeural",
            "GUARD": "en-GB-AlfieNeural",
            "SOLDIER": "en-US-JasonNeural",
            "PILOT": "en-US-SteffanNeural",
            "DOCTOR": "en-IN-NeerjaNeural",
            "ANNOUNCER": "en-US-SteffanNeural",
            "MERC": "en-GB-OliverNeural",
            "CLONE": "en-US-JasonNeural",
            "DORCHESTER": "en-IE-EmilyNeural",
            "SPIKE": "en-AU-WilliamNeural",
            "RAVEN": "en-US-AshleyNeural",
            "QUEEN SOPHIA": "en-GB-MaisieNeural",
            "UN AGENT": "en-GB-OliverNeural",
        },
    },
    "8.Digi_Sol.Suicider]BIG]b1": {
        "prefix": "digisol",
        "voices": {
            "Narrator": "en-GB-RyanNeural",
            "LUCKY": "en-US-GuyNeural",
            "ASTRA": "es-MX-DaliaNeural",
            "GEM": "en-US-AvaNeural",
            "EDGE": "en-AU-WilliamNeural",
            "DANGERMAN": "en-AU-WilliamNeural",
            "EDGE DANGERMAN": "en-AU-WilliamNeural",
            "LOONA": "ko-KR-SunHiNeural",
            "LOONA TWILIGHT": "ko-KR-SunHiNeural",
            "SAOIRSE": "en-IE-EmilyNeural",
            "SÁOIRSE": "en-IE-EmilyNeural",
            "DIABOLUS": "ar-AE-HamdanNeural",
            "ARCHITECT": "en-US-ChristopherNeural",
            "SOL": "en-US-ChristopherNeural",
            "CAPTAIN": "en-ZA-LukeNeural",
            "GUARD": "en-GB-AlfieNeural",
            "SOLDIER": "en-US-JasonNeural",
            "PILOT": "en-US-SteffanNeural",
            "DOCTOR": "en-IN-NeerjaNeural",
            "ANNOUNCER": "en-US-SteffanNeural",
            "MERC": "en-GB-OliverNeural",
            "CLONE": "en-US-JasonNeural",
            "DORCHESTER": "en-IE-EmilyNeural",
            "SPIKE": "en-AU-WilliamNeural",
            "RAVEN": "en-US-AshleyNeural",
        },
    },
    "9.METAL SUN[METAL]wcsm+wtf]a1": {
        "prefix": "metalsun",
        "voices": {
            "Narrator": "en-GB-RyanNeural",
            "LUCKY": "en-US-GuyNeural",
            "ASTRA": "es-MX-DaliaNeural",
            "GEM": "en-US-AvaNeural",
            "EDGE": "en-AU-WilliamNeural",
            "DANGERMAN": "en-AU-WilliamNeural",
            "EDGE DANGERMAN": "en-AU-WilliamNeural",
            "LOONA": "ko-KR-SunHiNeural",
            "LOONA TWILIGHT": "ko-KR-SunHiNeural",
            "SAOIRSE": "en-IE-EmilyNeural",
            "SÁOIRSE": "en-IE-EmilyNeural",
            "DIABOLUS": "ar-AE-HamdanNeural",
            "ARCHITECT": "en-US-ChristopherNeural",
            "SOL": "en-US-ChristopherNeural",
            "CAPTAIN": "en-ZA-LukeNeural",
            "GUARD": "en-GB-AlfieNeural",
            "SOLDIER": "en-US-JasonNeural",
            "PILOT": "en-US-SteffanNeural",
            "DOCTOR": "en-IN-NeerjaNeural",
            "ANNOUNCER": "en-US-SteffanNeural",
            "MERC": "en-GB-OliverNeural",
            "CLONE": "en-US-JasonNeural",
            "DORCHESTER": "en-IE-EmilyNeural",
            "SPIKE": "en-AU-WilliamNeural",
            "RAVEN": "en-US-AshleyNeural",
        },
    },
    "10.xo.files]wc.smol]a0": {
        "prefix": "xofiles",
        "voices": {
            "Narrator": "en-GB-RyanNeural",
            "LUCKY": "en-US-GuyNeural",
            "ASTRA": "es-MX-DaliaNeural",
            "GEM": "en-US-AvaNeural",
            "EDGE": "en-AU-WilliamNeural",
            "DANGERMAN": "en-AU-WilliamNeural",
            "EDGE DANGERMAN": "en-AU-WilliamNeural",
            "LOONA": "ko-KR-SunHiNeural",
            "LOONA TWILIGHT": "ko-KR-SunHiNeural",
            "SAOIRSE": "en-IE-EmilyNeural",
            "SÁOIRSE": "en-IE-EmilyNeural",
            "DIABOLUS": "ar-AE-HamdanNeural",
            "ARCHITECT": "en-US-ChristopherNeural",
            "CAPTAIN": "en-ZA-LukeNeural",
            "GUARD": "en-GB-AlfieNeural",
            "SOLDIER": "en-US-JasonNeural",
            "PILOT": "en-US-SteffanNeural",
            "DOCTOR": "en-IN-NeerjaNeural",
            "ANNOUNCER": "en-US-SteffanNeural",
            "MERC": "en-GB-OliverNeural",
            "CLONE": "en-US-JasonNeural",
            "DORCHESTER": "en-IE-EmilyNeural",
            "SPIKE": "en-AU-WilliamNeural",
            "RAVEN": "en-US-AshleyNeural",
        },
    },
    "11.FOOTRACE.FU]HUGE": {
        "prefix": "footrace",
        "voices": {
            "Narrator": "en-GB-RyanNeural",
            "LUCKY": "en-US-GuyNeural",
            "ASTRA": "es-MX-DaliaNeural",
            "GEM": "en-US-AvaNeural",
            "EDGE": "en-AU-WilliamNeural",
            "DANGERMAN": "en-AU-WilliamNeural",
            "EDGE DANGERMAN": "en-AU-WilliamNeural",
            "LOONA": "ko-KR-SunHiNeural",
            "LOONA TWILIGHT": "ko-KR-SunHiNeural",
            "SAOIRSE": "en-IE-EmilyNeural",
            "SÁOIRSE": "en-IE-EmilyNeural",
            "DIABOLUS": "ar-AE-HamdanNeural",
            "ARCHITECT": "en-US-ChristopherNeural",
            "CAPTAIN": "en-ZA-LukeNeural",
            "GUARD": "en-GB-AlfieNeural",
            "SOLDIER": "en-US-JasonNeural",
            "PILOT": "en-US-SteffanNeural",
            "DOCTOR": "en-IN-NeerjaNeural",
            "ANNOUNCER": "en-US-SteffanNeural",
            "MERC": "en-GB-OliverNeural",
            "CLONE": "en-US-JasonNeural",
            "DORCHESTER": "en-IE-EmilyNeural",
            "SPIKE": "en-AU-WilliamNeural",
            "RAVEN": "en-US-AshleyNeural",
        },
    },
    "12.centroid.u.ϕ]2xvsp]big]b1": {
        "prefix": "centroid",
        "voices": {
            "Narrator": "en-GB-RyanNeural",
            "LUCKY": "en-US-GuyNeural",
            "ASTRA": "es-MX-DaliaNeural",
            "GEM": "en-US-AvaNeural",
            "EDGE": "en-AU-WilliamNeural",
            "DANGERMAN": "en-AU-WilliamNeural",
            "EDGE DANGERMAN": "en-AU-WilliamNeural",
            "LOONA": "ko-KR-SunHiNeural",
            "LOONA TWILIGHT": "ko-KR-SunHiNeural",
            "SAOIRSE": "en-IE-EmilyNeural",
            "SÁOIRSE": "en-IE-EmilyNeural",
            "DIABOLUS": "ar-AE-HamdanNeural",
            "ARCHITECT": "en-US-ChristopherNeural",
            "CAPTAIN": "en-ZA-LukeNeural",
            "GUARD": "en-GB-AlfieNeural",
            "SOLDIER": "en-US-JasonNeural",
            "PILOT": "en-US-SteffanNeural",
            "DOCTOR": "en-IN-NeerjaNeural",
            "ANNOUNCER": "en-US-SteffanNeural",
            "MERC": "en-GB-OliverNeural",
            "CLONE": "en-US-JasonNeural",
            "DORCHESTER": "en-IE-EmilyNeural",
            "SPIKE": "en-AU-WilliamNeural",
            "RAVEN": "en-US-AshleyNeural",
            "CHARA": "fr-FR-VivienneMultilingualNeural",
            "SOL-SUPREME": "en-US-ChristopherNeural",
            "IQABELLA": "en-US-AnaNeural",
            "HORLI": "fr-FR-DeniseNeural",
            "PROFESSOR PYLORUS": "en-GB-SoniaNeural",
            "PROFESSOR AURORA": "en-US-JennyNeural",
        },
    },
}

VOICE_SETTINGS = {
    "en-GB-RyanNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-GuyNeural": {"rate": "+0%", "pitch": "-1Hz"},
    "es-MX-DaliaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-AvaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-AU-WilliamNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "ko-KR-SunHiNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-IE-EmilyNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "ar-AE-HamdanNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-ChristopherNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "en-ZA-LukeNeural": {"rate": "+0%", "pitch": "-1Hz"},
    "en-GB-AlfieNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "en-US-JasonNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-SteffanNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-IN-NeerjaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-OliverNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-AshleyNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "fr-FR-VivienneMultilingualNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-AnaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "fr-FR-DeniseNeural": {"rate": "+5%", "pitch": "+2Hz"},
    "en-GB-SoniaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-JennyNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-MaisieNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-NG-EzinneNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-IE-OrlaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-MichelleNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-LibbyNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-GB-BellaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-CA-ClaraNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-KE-ChilembaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "ja-JP-KeitaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "ja-JP-NanamiNeural": {"rate": "+0%", "pitch": "+0Hz"},
}

def detect_format(text):
    """Detect Family A (<center>) vs Family B (ALL CAPS name)"""
    if '<center>' in text:
        return 'A'
    return 'B'

def parse_scene_a(text, voice_map):
    """Parse Family A format"""
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
            for key in voice_map:
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

def parse_scene_b(text, voice_map):
    """Parse Family B format"""
    segments = []
    current_speaker = "Narrator"
    current_text = []
    
    lines = text.split('\n')
    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()
        
        name_match = re.match(r'^([A-Z][A-Z\s\-\'\.]+?)(?:\s*\([^)]*\))?\s*$', stripped)
        if name_match and len(stripped) < 50:
            name = name_match.group(1).strip()
            if name.startswith('INT.') or name.startswith('EXT.') or name.startswith('FADE') or name.startswith('SCENE') or name.startswith('SOUND') or name.startswith('CLOSE UP') or name.startswith('FLASHBACK') or name.startswith('END FLASHBACK') or name.startswith('MONTAGE') or name.startswith('THE END'):
                if current_text:
                    segments.append((current_speaker, ' '.join(current_text)))
                    current_text = []
                current_speaker = "Narrator"
                current_text.append(re.sub(r'\*\*', '', stripped))
                i += 1
                continue
            
            if current_text:
                segments.append((current_speaker, ' '.join(current_text)))
                current_text = []
            
            matched = False
            for key in voice_map:
                if key in name or name in key:
                    current_speaker = key
                    matched = True
                    break
            if not matched:
                current_speaker = name
            
            i += 1
            continue
        
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

async def convert_scene(scene_file, output_mp3, voice_map):
    with open(scene_file, 'r', encoding='utf-8') as f:
        text = f.read()
    
    fmt = detect_format(text)
    if fmt == 'A':
        segments = parse_scene_a(text, voice_map)
    else:
        segments = parse_scene_b(text, voice_map)
    
    if not segments:
        return False
    
    segment_files = []
    for i, (speaker, seg_text) in enumerate(segments):
        voice = voice_map.get(speaker, voice_map.get("Narrator", "en-GB-RyanNeural"))
        seg_path = output_mp3.replace('.mp3', f'_seg_{i:03d}.mp3')
        try:
            await generate_speech(seg_text, voice, seg_path)
            segment_files.append(seg_path)
        except Exception:
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
        print(f"    ✅ {os.path.basename(output_mp3)} ({size_mb:.1f} MB, {len(segment_files)} segs)")
        return True
    
    return False

async def convert_book(book_dir, book_name, book_config):
    prefix = book_config["prefix"]
    voice_map = book_config["voices"]
    
    scene_files = sorted(glob.glob(os.path.join(book_dir, "scene_*.txt")))
    if not scene_files:
        print(f"  ⚠️ No scene files found in {book_name}")
        return
    
    output_dir = os.path.join(book_dir, "audio")
    os.makedirs(output_dir, exist_ok=True)
    
    print(f"  📚 {book_name}: {len(scene_files)} scenes (Format: {detect_format(open(scene_files[0]).read())})")
    
    await asyncio.sleep(5)
    
    for scene_file in scene_files:
        basename = os.path.basename(scene_file).replace('.txt', '').replace('scene_', f'{prefix}-scene-').replace('_', '-').lower()
        while '--' in basename:
            basename = basename.replace('--', '-')
        if basename.endswith('-'):
            basename = basename[:-1]
        output_mp3 = os.path.join(output_dir, f"{basename}.mp3")
        
        if os.path.exists(output_mp3):
            continue
        
        print(f"    🔄 {os.path.basename(scene_file)}...")
        success = await convert_scene(scene_file, output_mp3, voice_map)
        if success:
            await asyncio.sleep(3)
    
    # Copy to iphone
    iphone_dir = os.path.join(IPHONE_BASE, book_name, "audio")
    os.makedirs(iphone_dir, exist_ok=True)
    os.system(f'cp "{output_dir}"/*.mp3 "{iphone_dir}/" 2>/dev/null')
    
    count = len(glob.glob(os.path.join(output_dir, "*.mp3")))
    size = sum(os.path.getsize(f) for f in glob.glob(os.path.join(output_dir, "*.mp3"))) / (1024*1024)
    print(f"  ✅ {book_name}: {count} scenes, {size:.1f} MB")

async def main():
    for book_name, book_config in BOOKS.items():
        book_dir = os.path.join(BASE, book_name)
        if os.path.exists(book_dir):
            await convert_book(book_dir, book_name, book_config)
        else:
            print(f"  ⚠️ Directory not found: {book_name}")
    
    print(f"\n✅ ALL BOOKS CONVERTED!")

if __name__ == "__main__":
    asyncio.run(main())
