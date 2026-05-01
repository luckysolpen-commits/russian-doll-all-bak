#!/usr/bin/env python3
"""Convert ONE Sol Pen scene to audio with rate limit protection."""

import asyncio
import edge_tts
import os
import re
import sys
import time

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
    # more
    "CHARA": "fr-FR-VivienneMultilingualNeural",
    "IQABELLA": "en-US-AnaNeural",
    "HORLI": "fr-FR-DeniseNeural",
    "RAHWEH": "zh-CN-YunxiaNeural",
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
    "en-GB-ThomasNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "en-AU-WilliamNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "en-NG-AbeoNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-IN-NeerjaNeural": {"rate": "+0%", "pitch": "+0Hz"},
}

def parse_scene(text):
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

async def generate_speech(text, voice, output_path):
    settings = VOICE_SETTINGS.get(voice, {"rate": "+0%", "pitch": "+0Hz"})
    try:
        communicate = edge_tts.Communicate(text, voice, rate=settings["rate"], pitch=settings["pitch"])
        await communicate.save(output_path)
        return True
    except Exception as e:
        print(f"    ⚠️ {e}")
        return False

async def convert_scene(scene_name):
    scene_file = os.path.join(SCENE_DIR, f"{scene_name}.txt")
    if not os.path.exists(scene_file):
        print(f"  ⚠️ File not found: {scene_file}")
        return False

    output_mp3 = os.path.join(OUTPUT_DIR, f"{scene_name}.mp3")
    if os.path.exists(output_mp3):
        print(f"  ⏭️ Already exists, skipping")
        return True

    with open(scene_file, 'r', encoding='utf-8') as f:
        text = f.read()

    segments = parse_scene(text)
    if not segments:
        print(f"  ⚠️ No segments found")
        return False

    print(f"  📝 {len(segments)} segments")

    scene_tmp = os.path.join(OUTPUT_DIR, f"_tmp_{scene_name}")
    os.makedirs(scene_tmp, exist_ok=True)

    seg_files = []
    for i, (speaker, seg_text) in enumerate(segments):
        voice = VOICE_MAP.get(speaker, VOICE_MAP.get("Narrator"))
        seg_path = os.path.join(scene_tmp, f"seg_{i:03d}.mp3")
        print(f"    [{i+1}/{len(segments)}] {speaker}", end="", flush=True)
        ok = await generate_speech(seg_text, voice, seg_path)
        if ok:
            seg_files.append(seg_path)
            print(" ✅")
        else:
            print(" ❌")
        # Rate limit protection - pause between calls
        await asyncio.sleep(0.5)

    if seg_files:
        list_file = os.path.join(scene_tmp, "filelist.txt")
        with open(list_file, 'w') as f:
            for sp in seg_files:
                f.write(f"file '{sp}'\n")
        os.system(f'ffmpeg -y -f concat -safe 0 -i "{list_file}" -c copy "{output_mp3}" 2>/dev/null')
        size_mb = os.path.getsize(output_mp3) / (1024*1024) if os.path.exists(output_mp3) else 0
        print(f"  ✅ {scene_name}.mp3 ({size_mb:.1f} MB)")
        # Cleanup
        import shutil
        shutil.rmtree(scene_tmp, ignore_errors=True)
        return True
    else:
        print(f"  ❌ No audio generated")
        import shutil
        shutil.rmtree(scene_tmp, ignore_errors=True)
        return False

async def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    all_scenes = sorted([f.replace('.txt','') for f in os.listdir(SCENE_DIR) if f.startswith('scene_') and f.endswith('.txt')])
    if len(sys.argv) > 1:
        scenes = sys.argv[1:]
    else:
        scenes = all_scenes
    print(f"Converting {len(scenes)} scenes one at a time...\n")
    done = 0
    for s in scenes:
        print(f"\n{'='*50}")
        print(f"🎬 {s}")
        print(f"{'='*50}")
        if await convert_scene(s):
            done += 1
    print(f"\n✅ {done}/{len(scenes)} converted")

if __name__ == "__main__":
    asyncio.run(main())
