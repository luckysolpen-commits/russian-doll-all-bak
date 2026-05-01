#!/usr/bin/env python3
"""Batch converter for a single directory. Usage: python3 batch_convert.py <dir> <pattern> <category>"""
import asyncio, os, re, subprocess, tempfile, sys, glob
from pydub import AudioSegment

EDGE_VOICE_MAP = {
    "DM (Mother Tmojizu)": "zh-CN-XiaoxiaoNeural", "Mother Tmojizu": "zh-CN-XiaoxiaoNeural",
    "Mother Tomokazu": "zh-CN-XiaoxiaoNeural", "CHARA": "fr-FR-VivienneMultilingualNeural",
    "Sol-Supreme": "en-US-ChristopherNeural", "IQabella": "en-US-AnaNeural",
    "Horli Qwin": "fr-FR-DeniseNeural", "DM": "zh-CN-XiaoxiaoNeural",
    "QOO": "zh-CN-YunxiaNeural", "中文老师": "zh-CN-YunxiaNeural",
    "Chinese Narrator": "zh-CN-YunxiaNeural", "Chinese Voice": "zh-CN-YunxiaNeural",
    "CHR-047": "fr-FR-VivienneMultilingualNeural", "SOL-001": "en-US-ChristopherNeural",
    "IQA-112": "en-US-AnaNeural", "HQW-006": "fr-FR-DeniseNeural",
    "THE ORIGINAL CHARA": "fr-FR-VivienneMultilingualNeural", "THE ORIGINAL SOL-SUPREME": "en-US-ChristopherNeural",
    "THE ORIGINAL IQABELLA": "en-US-AnaNeural", "THE ORIGINAL HORLI QWIN": "fr-FR-DeniseNeural",
    "Luna": "en-US-EmmaNeural", "Sora": "en-US-EricNeural",
    "RaTheButcher": "en-US-ChristopherNeural", "TRONALD_THE_MAGE": "en-US-ChristopherNeural",
    "BORIN IRONMUG": "en-US-ChristopherNeural",
    "HOODED FIGURE (LoonaTwilight)": "zh-CN-XiaoxiaoNeural",
    "Player (The Creator)": "en-US-AndrewNeural", "The Creator": "en-US-AndrewNeural",
    "Crimson": "en-US-ChristopherNeural", "Ember": "fr-FR-VivienneMultilingualNeural",
    "Glimmer": "en-US-AnaNeural", "Verdi": "en-US-EricNeural",
    "Aqua": "en-US-EmmaNeural", "Nebula": "en-US-MichelleNeural",
    "Void": "en-US-ChristopherNeural", "Lumen": "en-KE-AsiliaNeural",
    "Leafsong": "en-US-AvaNeural",
    "Everyone": "zh-CN-XiaoxiaoNeural", "GOLD COINS": "zh-CN-XiaoxiaoNeural",
    "EXPERIENCE POINTS": "zh-CN-XiaoxiaoNeural", "RARE ITEMS ACQUIRED": "zh-CN-XiaoxiaoNeural",
    # African accents — main NPCs (not clones)
    "THE DIABOLUS": "ar-AE-HamdanNeural", "THE HOUSE": "ru-RU-DmitryNeural",
    "THE MOON SIGNAL": "en-US-EmmaNeural", "BROKER VEX": "en-NG-AbeoNeural",
    "BROKER": "en-NG-AbeoNeural", "SCRATCH": "en-KE-AsiliaNeural",
    "OFFICER KRELL": "en-KE-ChilembaNeural", "MASTER FENG": "ja-JP-KeitaNeural",
    "MNEMOSYNE": "en-KE-AsiliaNeural", "VOSS": "en-NG-EzinneNeural",
    "SCOUT": "en-NG-AbeoNeural",
    # British accents — clones/clowns (Mars inhabitants)
    "CHR-033": "fr-FR-VivienneMultilingualNeural", "CHR-022": "fr-FR-VivienneMultilingualNeural",
    "BUILDER CLONE": "en-GB-MaisieNeural", "CHEF CLONE": "en-GB-RyanNeural",
    "HACKER CLONE": "en-GB-SoniaNeural", "ENFORCER": "en-GB-ThomasNeural",
    "UNREGISTERED CLONE": "en-GB-LibbyNeural",
    "Lucky": "en-US-ChristopherNeural", "Mama Sour": "en-US-MichelleNeural",
    "Rusty": "en-US-EricNeural", "Old Man Feng": "en-US-AndrewNeural",
    "Boss Nami": "fr-FR-VivienneMultilingualNeural",
}

def parse_multi(fp):
    segs, cur_spk, cur_txt = [], None, []
    pat = re.compile(r'^\*\*(.+?):\*\*\s*(.*)')
    with open(fp, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.rstrip('\n')
            if line.startswith('# ') or line.startswith('## ') or line.startswith('---'): continue
            m = pat.match(line)
            if m:
                if cur_spk and cur_txt: segs.append((cur_spk, ' '.join(cur_txt).strip()))
                cur_spk = m.group(1).strip(); cur_txt = [m.group(2).strip()] if m.group(2).strip() else []
            elif line.strip() == '':
                if cur_spk and cur_txt: segs.append((cur_spk, ' '.join(cur_txt).strip())); cur_spk, cur_txt = None, []
            else:
                c = re.sub(r'\*\*(.+?)\*\*', r'\1', line.strip())
                if c: cur_txt.append(c)
    if cur_spk and cur_txt: segs.append((cur_spk, ' '.join(cur_txt).strip()))
    return segs

def parse_horli(fp):
    with open(fp, 'r', encoding='utf-8') as f: text = f.read()
    lines = text.split('\n'); clean = []
    for line in lines:
        s = line.strip()
        if not s or s.startswith('#') or s.startswith('---') or s.startswith('**['): continue
        if any(s.startswith(p) for p in ['**Topic:', '**Voice:', '**Estimated', '**Next:', '**Series', '**ϕ']): continue
        clean.append(s)
    full = '\n'.join(clean)
    pat = re.compile(r'(?:\*\*)?(Horli Qwin|QOO|中文老师)(?:\*\*)?:\s*')
    segs, spk, buf = [], None, []
    for part in pat.split(full):
        if not part.strip(): continue
        if part.strip() in ['Horli Qwin', 'QOO', '中文老师']:
            if spk and buf:
                t = ' '.join(buf).strip().replace('**', '')
                if t: segs.append((spk, t))
            spk = part.strip(); buf = []
        elif spk: buf.append(part.strip())
    if spk and buf:
        t = ' '.join(buf).strip().replace('**', '')
        if t: segs.append((spk, t))
    return segs

async def tts(text, voice):
    tmp = tempfile.NamedTemporaryFile(suffix='.mp3', delete=False); tmp.close()
    proc = await asyncio.create_subprocess_exec('edge-tts', '--voice', voice, '--text', text, '--write-media', tmp.name, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    await proc.wait()
    if os.path.getsize(tmp.name) == 0: os.unlink(tmp.name); return None
    return tmp.name

async def convert_multi(segments, out):
    combined = AudioSegment.empty(); ok, fail = 0, 0
    for i, (spk, txt) in enumerate(segments):
        voice = EDGE_VOICE_MAP.get(spk, "zh-CN-XiaoxiaoNeural")
        if not txt.strip(): continue
        try:
            tmp = await tts(txt, voice)
            if tmp: combined += AudioSegment.from_mp3(tmp); os.unlink(tmp); ok += 1
            else: fail += 1
        except: fail += 1
        await asyncio.sleep(1.5)
    if combined: combined.export(out, format='mp3', bitrate='192k'); return ok, fail
    return 0, len(segments)

async def convert_single(text, voice, out):
    tmp = await tts(text, voice)
    if tmp:
        AudioSegment.from_mp3(tmp).export(out, format='mp3', bitrate='192k'); os.unlink(tmp); return 1, 0
    return 0, 1

async def convert_charanon_chapter(chapter_dir, voice, out):
    """Convert a CHAR.A.NON chapter: read all page_*.txt, strip blanks, join with spaces."""
    page_files = sorted([f for f in os.listdir(chapter_dir) if f.startswith("page_") and f.endswith(".txt")])
    if not page_files:
        # Try scene files instead
        scene_files = sorted([f for f in os.listdir(chapter_dir) if f.startswith("scene_") and f.endswith(".txt")])
        if scene_files:
            all_text = []
            for sf in scene_files:
                with open(os.path.join(chapter_dir, sf), 'r', encoding='utf-8', errors='replace') as f:
                    lines = f.readlines()
                spoken = [l.strip() for l in lines if l.strip()]
                all_text.extend(spoken)
            text = ' '.join(all_text)
        else:
            return 0, 1
    else:
        all_text = []
        for pf in page_files:
            with open(os.path.join(chapter_dir, pf), 'r', encoding='utf-8', errors='replace') as f:
                lines = f.readlines()
            # Strip blank lines, keep only non-empty lines
            spoken = [l.strip() for l in lines if l.strip()]
            all_text.extend(spoken)
        # Join with single space — NO paragraph breaks = NO pauses
        text = ' '.join(all_text)

    if not text.strip():
        return 0, 1

    # Split into chunks if too long (edge-tts has limits)
    chunk_size = 3000
    chunks = [text[i:i+chunk_size] for i in range(0, len(text), chunk_size)]
    combined = AudioSegment.empty()
    ok, fail = 0, 0
    for i, chunk in enumerate(chunks):
        tmp = await tts(chunk, voice)
        if tmp:
            combined += AudioSegment.from_mp3(tmp)
            os.unlink(tmp); ok += 1
        else:
            fail += 1
        await asyncio.sleep(1.5)
    if combined:
        combined.export(out, format='mp3', bitrate='192k')
        return ok, fail
    return 0, 1

async def main():
    src_dir = sys.argv[1]
    pattern = re.compile(sys.argv[2])
    category = sys.argv[3] if len(sys.argv) > 3 else "multi"

    files = sorted([f for f in os.listdir(src_dir) if pattern.match(f)])
    print(f"[{os.path.basename(src_dir)}] {len(files)} files to convert")

    done, fail = 0, 0
    for fname in files:
        fpath = os.path.join(src_dir, fname)
        out = os.path.join(src_dir, f"非{os.path.splitext(fname)[0]}.mp3")
        if os.path.exists(out):
            print(f"  SKIP: {fname}"); done += 1; continue
        print(f"  [{done+fail+1}/{len(files)}] {fname}")
        try:
            if category == "charanon":
                # CHAR.A.NON: each file is a chapter dir with page_*.txt inside
                chapter_dir = fpath
                voice = "fr-FR-VivienneMultilingualNeural"
                ok, f = await convert_charanon_chapter(chapter_dir, voice, out)
            elif category.startswith("single:"):
                voice = category.split(":")[1]
                with open(fpath, 'r', encoding='utf-8') as f: lines = f.readlines()
                spoken = [l.strip().replace('**','') for l in lines if l.strip() and not l.strip().startswith('#') and not l.strip().startswith('---') and not l.strip().startswith('**[') and not any(l.strip().startswith(p) for p in ['**Topic:', '**Voice:', '**Estimated', '**Next:', '**Series', '**ϕ'])]
                ok, f = await convert_single('\n'.join(spoken), voice, out)
            elif category == "horli-zh":
                segs = parse_horli(fpath)
                ok, f = await convert_multi(segs, out) if segs else (0, 1)
            else:
                segs = parse_multi(fpath)
                ok, f = await convert_multi(segs, out) if segs else (0, 1)
            if ok > 0:
                done += 1
                # Delete old MP3s in dir that don't start with 非
                for old in glob.glob(os.path.join(src_dir, "*.mp3")):
                    if not os.path.basename(old).startswith('非'):
                        try: os.unlink(old)
                        except: pass
            else: fail += 1
        except Exception as e:
            print(f"  ERROR: {e}"); fail += 1
        await asyncio.sleep(1)

    print(f"[{os.path.basename(src_dir)}] DONE: {done} ok, {fail} failed")

if __name__ == '__main__':
    asyncio.run(main())
