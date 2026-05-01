#!/usr/bin/env python3
"""
Multi-voice TTS converter for Horli's tutalage sessions.
Horli Qwin → en-US-JennyNeural
中文老师 → zh-CN-XiaoxiaoNeural
"""

import sys, os, re, asyncio, subprocess, tempfile
from pydub import AudioSegment

VOICE_MAP = {"Horli Qwin": "en-US-JennyNeural", "中文老师": "zh-CN-XiaoxiaoNeural"}

async def tts_segment(text, voice):
    tmp = tempfile.NamedTemporaryFile(suffix='.mp3', delete=False)
    tmp.close()
    proc = await asyncio.create_subprocess_exec(
        'edge-tts', '--voice', voice, '--text', text, '--write-media', tmp.name,
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    await proc.wait()
    if os.path.exists(tmp.name) and os.path.getsize(tmp.name) > 0:
        return tmp.name
    os.unlink(tmp.name)
    return None

def parse_speakers(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        text = f.read()
    lines = text.split('\n')
    clean = []
    for line in lines:
        s = line.strip()
        if not s or s.startswith('#') or s.startswith('---') or s.startswith('**['):
            continue
        if any(s.startswith(p) for p in ['**Topic:', '**Voice:', '**Estimated', '**Next:', '**Series', '**ϕ']):
            continue
        clean.append(s)
    full = '\n'.join(clean)
    # Match **Horli Qwin:** or 中文老师:
    pattern = re.compile(r'(?:\*\*)?(Horli Qwin|中文老师)(?:\*\*)?:\s*')
    segments = []
    parts = pattern.split(full)
    speaker = None
    buf = []
    for part in parts:
        if not part.strip():
            continue
        if part.strip() in VOICE_MAP:
            if speaker and buf:
                t = ' '.join(buf).strip().replace('**', '')
                if t: segments.append((speaker, t))
            speaker = part.strip()
            buf = []
        elif speaker:
            buf.append(part.strip())
    if speaker and buf:
        t = ' '.join(buf).strip().replace('**', '')
        if t: segments.append((speaker, t))
    return segments

async def convert_session(filepath, output_path):
    segments = parse_speakers(filepath)
    if not segments:
        print(f"  No segments found")
        return False
    print(f"  Found {len(segments)} segments")
    combined = AudioSegment.empty()
    ok = 0
    fail = 0
    for i, (spk, txt) in enumerate(segments):
        voice = VOICE_MAP.get(spk, "en-US-JennyNeural")
        disp = txt[:60] + "..." if len(txt) > 60 else txt
        print(f"  [{i+1}/{len(segments)}] {spk}: {disp}")
        tmp = await tts_segment(txt, voice)
        if tmp:
            combined += AudioSegment.from_mp3(tmp)
            os.unlink(tmp)
            ok += 1
        else:
            fail += 1
    combined.export(output_path, format='mp3')
    size = os.path.getsize(output_path) / (1024*1024)
    print(f"  DONE: {os.path.basename(output_path)} ({size:.1f} MB) — {ok} ok, {fail} failed")
    return True

async def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    horli_dir = os.path.join(script_dir, "horli-tutalage")
    files = sorted([f for f in os.listdir(horli_dir) if f.startswith("horli-tutalage-sesh") and f.endswith(".txt")])
    for fname in files:
        fp = os.path.join(horli_dir, fname)
        base = os.path.splitext(fname)[0]
        out = os.path.join(horli_dir, f"{base}.mp3")
        if os.path.exists(out):
            sz = os.path.getsize(out) / (1024*1024)
            print(f"SKIP (exists): {fname} → {sz:.1f} MB")
            continue
        print(f"\nConverting {fname}...")
        await convert_session(fp, out)
        await asyncio.sleep(2)
    print("\n=== DONE ===")

if __name__ == '__main__':
    asyncio.run(main())
