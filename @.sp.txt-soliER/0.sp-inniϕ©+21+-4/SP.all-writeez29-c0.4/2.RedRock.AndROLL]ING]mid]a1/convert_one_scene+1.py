#!/usr/bin/env python3
"""Convert ONE Sol Pen scene to audio with rate limit protection and silent fallback."""

import asyncio
import edge_tts
import os
import re
import sys
import shutil
from edge_tts.exceptions import NoAudioReceived

SCENE_DIR = "./rr-screenplay"
OUTPUT_DIR = "./rr-screenplay/audio-output-50"

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
    # screen
    "ZARA": "en-US-ElviraNeural",
    "QUEEN": "en-US-EzinneNeural",
    "VANCE": "en-US-ConnorNeural",
    "CHAIRMAN": "en-US-GiuseppeMultilingualNeural",
    "RATCHET": "en-US-AndrewNeural",
    "DR. ARIS": "en-US-AndrewNeural",
    #charas
    "VICTOR LAGRAINE": "en-US-AndrewNeural",
    "THOMPSON": "en-US-AndrewNeural",
    "MAN": "en-US-AndrewNeural",
    "WOMAN": "en-US-AndrewNeural",
    "---": "en-US-AndrewNeural",
    "SISTER": "en-US-AndrewNeural",

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


async def create_silent_segment(output_path, duration=1.0):
    """Create a silent MP3 using ffmpeg (very reliable fallback)"""
    try:
        cmd = f'ffmpeg -y -f lavfi -i anullsrc=r=24000:cl=mono -t {duration} -c:a libmp3lame -b:a 32k "{output_path}" 2>/dev/null'
        os.system(cmd)
        return os.path.exists(output_path) and os.path.getsize(output_path) > 100
    except:
        return False


async def generate_speech(text, voice, output_path, max_retries=2):
    settings = VOICE_SETTINGS.get(voice, {"rate": "+0%", "pitch": "+0Hz"})
    for attempt in range(max_retries + 1):
        try:
            communicate = edge_tts.Communicate(
                text, voice, rate=settings["rate"], pitch=settings["pitch"]
            )
            await communicate.save(output_path)
            
            if os.path.exists(output_path) and os.path.getsize(output_path) > 300:
                return True
            else:
                raise NoAudioReceived("File too small")
                
        except Exception as e:
            if attempt < max_retries:
                wait = 1.5 + (attempt * 1.2)
                print(f" ⚠️ retry {attempt+1}", end="", flush=True)
                await asyncio.sleep(wait)
                continue
            print(" ❌", end="")
            return False
    return False


async def convert_scene(scene_name):
    scene_file = os.path.join(SCENE_DIR, f"{scene_name}.txt")
    if not os.path.exists(scene_file):
        print(f"  ⚠️ File not found: {scene_file}")
        return False

    output_mp3 = os.path.join(OUTPUT_DIR, f"{scene_name}.mp3")
    if os.path.exists(output_mp3) and os.path.getsize(output_mp3) > 1000:
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
    failed_count = 0

    for i, (speaker, seg_text) in enumerate(segments):
        voice = VOICE_MAP.get(speaker, VOICE_MAP["Narrator"])
        seg_path = os.path.join(scene_tmp, f"seg_{i:03d}.mp3")

        print(f"    [{i+1:2d}/{len(segments)}] {speaker:<15}", end="", flush=True)

        success = await generate_speech(seg_text, voice, seg_path)

        if success and os.path.exists(seg_path) and os.path.getsize(seg_path) > 300:
            seg_files.append(seg_path)
            print(" ✅")
        else:
            failed_count += 1
            # Create silent fallback
            silent_ok = await create_silent_segment(seg_path, duration=1.2)
            if silent_ok:
                seg_files.append(seg_path)
                print(" 🔇 (silent)")
            else:
                print(" ❌ (failed + no silent)")

        # Rate limit
        await asyncio.sleep(0.65 if success else 1.8)

    if not seg_files:
        print(f"  ❌ No segments at all")
        shutil.rmtree(scene_tmp, ignore_errors=True)
        return False

    # Build filelist
    list_file = os.path.join(scene_tmp, "filelist.txt")
    with open(list_file, 'w', encoding='utf-8') as f:
        for sp in seg_files:
            rel_path = os.path.basename(sp)
            f.write(f"file '{rel_path}'\n")

    print(f"  🔗 Concatenating {len(seg_files)}/{len(segments)} segments...")

    # Run ffmpeg with error logging
    ffmpeg_log = os.path.join(scene_tmp, "ffmpeg.log")
    cmd = f'ffmpeg -y -f concat -safe 0 -i "{list_file}" -c copy "{output_mp3}" 2>"{ffmpeg_log}"'
    os.system(cmd)

    # Check result
    if os.path.exists(output_mp3) and os.path.getsize(output_mp3) > 2000:
        size_mb = os.path.getsize(output_mp3) / (1024 * 1024)
        print(f"  ✅ {scene_name}.mp3 ({size_mb:.1f} MB) — {failed_count} silent/fallback")
        success = True
    else:
        print(f"  ❌ Final concat failed or file too small")
        if os.path.exists(ffmpeg_log):
            print("     → Check error log:", ffmpeg_log)
            with open(ffmpeg_log, 'r') as f:
                print(f.read()[-500:])  # show last part of log
        success = False

    # Cleanup
    shutil.rmtree(scene_tmp, ignore_errors=True)
    return success


async def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    all_scenes = sorted([f.replace('.txt', '') 
                        for f in os.listdir(SCENE_DIR) 
                        if f.startswith('scene_') and f.endswith('.txt')])

    if len(sys.argv) > 1:
        scenes = [s for s in sys.argv[1:] if s]
    else:
        scenes = all_scenes

    print(f"Converting {len(scenes)} scenes...\n")
    done = 0
    for s in scenes:
        print(f"\n{'='*65}")
        print(f"🎬 {s}")
        print(f"{'='*65}")
        if await convert_scene(s):
            done += 1

    print(f"\n✅ Finished: {done}/{len(scenes)} scenes processed successfully")


if __name__ == "__main__":
    asyncio.run(main())
