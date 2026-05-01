#!/usr/bin/env python3
"""Convert a single session file."""
import asyncio, os, re, subprocess, tempfile, sys
from pydub import AudioSegment

EDGE_VOICE_MAP = {
    "DM (Mother Tmojizu)": "zh-CN-XiaoxiaoNeural", "Mother Tmojizu": "zh-CN-XiaoxiaoNeural",
    "Mother Tomokazu": "zh-CN-XiaoxiaoNeural", "CHARA": "en-US-AriaNeural",
    "Sol-Supreme": "en-US-ChristopherNeural", "IQabella": "en-US-AnaNeural",
    "Horli Qwin": "en-US-JennyNeural", "DM": "zh-CN-XiaoxiaoNeural",
    "中文老师": "zh-CN-XiaoxiaoNeural",
}

def parse_session(filepath):
    segments, current_speaker, current_text = [], None, []
    speaker_pattern = re.compile(r'^\*\*(.+?):\*\*\s*(.*)')
    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.rstrip('\n')
            if line.startswith('# ') or line.startswith('## ') or line.startswith('---'): continue
            match = speaker_pattern.match(line)
            if match:
                if current_speaker and current_text:
                    segments.append((current_speaker, ' '.join(current_text).strip()))
                current_speaker = match.group(1).strip()
                current_text = [match.group(2).strip()] if match.group(2).strip() else []
            elif line.strip() == '':
                if current_speaker and current_text:
                    segments.append((current_speaker, ' '.join(current_text).strip()))
                    current_speaker, current_text = None, []
            else:
                cleaned = re.sub(r'\*\*(.+?)\*\*', r'\1', line.strip())
                if cleaned: current_text.append(cleaned)
    if current_speaker and current_text:
        segments.append((current_speaker, ' '.join(current_text).strip()))
    return segments

async def convert_file(filepath, output_path):
    segments = parse_session(filepath)
    print(f"Found {len(segments)} segments")
    combined = AudioSegment.empty()
    success, failed = 0, 0
    for i, (speaker, text) in enumerate(segments):
        voice = EDGE_VOICE_MAP.get(speaker, "zh-CN-XiaoxiaoNeural")
        if not text.strip(): continue
        display = text[:60] + "..." if len(text) > 60 else text
        print(f"  [{i+1}/{len(segments)}] {speaker}: {display}")
        tmp = tempfile.NamedTemporaryFile(suffix='.mp3', delete=False)
        tmp.close()
        proc = await asyncio.create_subprocess_exec('edge-tts', '--voice', voice, '--text', text, '--write-media', tmp.name, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        await proc.wait()
        if os.path.getsize(tmp.name) > 0:
            combined += AudioSegment.from_mp3(tmp.name)
            os.unlink(tmp.name)
            success += 1
        else:
            os.unlink(tmp.name)
            failed += 1
        await asyncio.sleep(1.5)
    combined.export(output_path, format='mp3', bitrate='192k')
    size = os.path.getsize(output_path) / (1024*1024)
    print(f"DONE: {os.path.basename(output_path)} ({size:.1f} MB) — {success} ok, {failed} failed")

if __name__ == '__main__':
    fname = sys.argv[1]
    src = f"/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+12/SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/1.ffu-ez_learnd&d+chem/{fname}"
    out = src.replace(fname, f"中{os.path.splitext(fname)[0]}.mp3")
    asyncio.run(convert_file(src, out))
