#!/usr/bin/env python3
"""
Convert Centroid Transition Session 4 (The Dance and the Tournament) to multi-voice audio.
"""

import asyncio
import edge_tts
import os
import re

OUTPUT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+14/SP.all-writeez28-b2/12.centroid.u.ϕ]2xvsp]big]b1/4.d&party-centroid"
INPUT_FILE = os.path.join(OUTPUT_DIR, "session-4-dance-and-tournament.txt")
OUTPUT_MP3 = os.path.join(OUTPUT_DIR, "session-4-dance-and-tournament.mp3")

VOICE_MAP = {
    "MOTHER TMOJIZU": "zh-CN-XiaoxiaoNeural",
    "CHARA": "fr-FR-VivienneMultilingualNeural",
    "SOL-SUPREME": "en-US-ChristopherNeural",
    "IQABELLA": "en-US-AnaNeural",
    "HORLI": "fr-FR-DeniseNeural",
    "PROFESSOR PYLORUS": "en-GB-SoniaNeural",
    "LUCKY": "en-US-GuyNeural",
    "MYSTERIOUS STUDENT": "ar-AE-HamdanNeural",
}

VOICE_SETTINGS = {
    "zh-CN-XiaoxiaoNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "fr-FR-VivienneMultilingualNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-ChristopherNeural": {"rate": "+0%", "pitch": "-2Hz"},
    "en-US-AnaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "fr-FR-DeniseNeural": {"rate": "+5%", "pitch": "+2Hz"},
    "en-GB-SoniaNeural": {"rate": "+0%", "pitch": "+0Hz"},
    "en-US-GuyNeural": {"rate": "+0%", "pitch": "-1Hz"},
    "ar-AE-HamdanNeural": {"rate": "+0%", "pitch": "-3Hz"},
}

def parse_segments(text):
    segments = []
    current_speaker = None
    current_text = []
    
    for line in text.split('\n'):
        speaker_match = None
        for speaker in VOICE_MAP:
            if line.startswith(f"**{speaker}:") or line.startswith(f"**{speaker} **"):
                speaker_match = speaker
                break
        
        if speaker_match:
            if current_speaker and current_text:
                segments.append((current_speaker, ' '.join(current_text)))
            current_speaker = speaker_match
            text_part = line.split(':', 1)[1].strip()
            text_part = re.sub(r'\*\*', '', text_part)
            current_text = [text_part] if text_part else []
        elif line.strip() == '':
            if current_speaker and current_text:
                segments.append((current_speaker, ' '.join(current_text)))
                current_speaker = None
                current_text = []
        else:
            if current_speaker:
                line_clean = re.sub(r'\*\*', '', line.strip())
                if line_clean:
                    current_text.append(line_clean)
    
    if current_speaker and current_text:
        segments.append((current_speaker, ' '.join(current_text)))
    
    return segments

async def generate_speech(text, voice, output_path):
    settings = VOICE_SETTINGS.get(voice, {"rate": "+0%", "pitch": "+0Hz"})
    communicate = edge_tts.Communicate(
        text, voice, rate=settings["rate"], pitch=settings["pitch"]
    )
    await communicate.save(output_path)

async def main():
    with open(INPUT_FILE, 'r', encoding='utf-8') as f:
        text = f.read()
    
    segments = parse_segments(text)
    print(f"Parsed {len(segments)} segments")
    
    segment_files = []
    for i, (speaker, seg_text) in enumerate(segments):
        voice = VOICE_MAP[speaker]
        seg_path = os.path.join(OUTPUT_DIR, f"centroid_s4_seg_{i:03d}.mp3")
        print(f"Generating segment {i+1}/{len(segments)}: {speaker} ({voice})")
        await generate_speech(seg_text, voice, seg_path)
        segment_files.append(seg_path)
    
    if segment_files:
        list_file = os.path.join(OUTPUT_DIR, "centroid_s4_filelist.txt")
        with open(list_file, 'w') as f:
            for seg_path in segment_files:
                f.write(f"file '{seg_path}'\n")
        
        os.system(f'ffmpeg -y -f concat -safe 0 -i "{list_file}" -c copy "{OUTPUT_MP3}"')
        print(f"\n✅ Output saved to: {OUTPUT_MP3}")
        
        for seg_path in segment_files:
            os.remove(seg_path)
        os.remove(list_file)
        
        size_mb = os.path.getsize(OUTPUT_MP3) / (1024 * 1024)
        print(f"📊 File size: {size_mb:.1f} MB")

if __name__ == "__main__":
    asyncio.run(main())
