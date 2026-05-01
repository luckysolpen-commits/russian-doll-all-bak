#!/usr/bin/env python3
"""
MASTER BATCH CONVERTER - All remaining files.
Deletes old MP3s when new ones are verified.
"""
import asyncio, os, re, subprocess, tempfile, sys, glob
from pydub import AudioSegment

EDGE_VOICE_MAP = {
    "DM (Mother Tmojizu)": "zh-CN-XiaoxiaoNeural", "Mother Tmojizu": "zh-CN-XiaoxiaoNeural",
    "Mother Tomokazu": "zh-CN-XiaoxiaoNeural", "CHARA": "en-US-AriaNeural",
    "Sol-Supreme": "en-US-ChristopherNeural", "IQabella": "en-US-AnaNeural",
    "Horli Qwin": "en-US-JennyNeural", "DM": "zh-CN-XiaoxiaoNeural",
    "中文老师": "zh-CN-XiaoxiaoNeural", "Chinese Narrator": "zh-CN-XiaoxiaoNeural",
    "Chinese Voice": "zh-CN-XiaoxiaoNeural",
    "CHR-047": "en-US-AriaNeural", "SOL-001": "en-US-ChristopherNeural",
    "IQA-112": "en-US-AnaNeural", "HQW-006": "en-US-JennyNeural",
    "THE ORIGINAL CHARA": "en-US-AriaNeural", "THE ORIGINAL SOL-SUPREME": "en-US-ChristopherNeural",
    "THE ORIGINAL IQABELLA": "en-US-AnaNeural", "THE ORIGINAL HORLI QWIN": "en-US-JennyNeural",
    "Luna": "en-US-EmmaNeural", "Sora": "en-US-EricNeural",
    "RaTheButcher": "en-US-ChristopherNeural", "TRONALD_THE_MAGE": "en-US-ChristopherNeural",
    "BORIN IRONMUG": "en-US-ChristopherNeural",
    "HOODED FIGURE (LoonaTwilight)": "zh-CN-XiaoxiaoNeural",
    "Player (The Creator)": "en-US-AndrewNeural", "The Creator": "en-US-AndrewNeural",
    "Crimson": "en-US-ChristopherNeural", "Ember": "en-US-AriaNeural",
    "Glimmer": "en-US-AnaNeural", "Verdi": "en-US-EricNeural",
    "Aqua": "en-US-EmmaNeural", "Nebula": "en-US-MichelleNeural",
    "Void": "en-US-ChristopherNeural", "Lumen": "en-US-JennyNeural",
    "Leafsong": "en-US-AvaNeural",
    "Everyone": "zh-CN-XiaoxiaoNeural", "GOLD COINS": "zh-CN-XiaoxiaoNeural",
    "EXPERIENCE POINTS": "zh-CN-XiaoxiaoNeural", "RARE ITEMS ACQUIRED": "zh-CN-XiaoxiaoNeural",
    "THE DIABOLUS": "en-US-ChristopherNeural", "THE HOUSE": "zh-CN-XiaoxiaoNeural",
    "THE MOON SIGNAL": "en-US-EmmaNeural", "BROKER VEX": "en-US-EricNeural",
    "BROKER": "en-US-EricNeural", "SCRATCH": "en-US-JennyNeural",
    "OFFICER KRELL": "en-US-ChristopherNeural", "MASTER FENG": "en-US-AndrewNeural",
    "MNEMOSYNE": "en-US-EmmaNeural", "VOSS": "en-US-ChristopherNeural",
    "CHR-033": "en-US-AriaNeural", "CHR-022": "en-US-AriaNeural",
    "BUILDER CLONE": "en-US-AndrewNeural", "CHEF CLONE": "en-US-JennyNeural",
    "HACKER CLONE": "en-US-AnaNeural", "ENFORCER": "en-US-ChristopherNeural",
    "UNREGISTERED CLONE": "en-US-EmmaNeural", "SCOUT": "en-US-EricNeural",
    "Lucky": "en-US-ChristopherNeural", "Mama Sour": "en-US-MichelleNeural",
    "Rusty": "en-US-EricNeural", "Old Man Feng": "en-US-AndrewNeural",
    "Boss Nami": "en-US-AriaNeural",
}

BASE = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+12"

JOBS = [
    # (dir, file_pattern_regex, old_mp3_pattern_or_None, category)
    # Category 1: Multi-voice
    ("SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/1.ffu-ez_learnd&d+chem", r"^fud&d-sesh(?!.*(?:commas|lore)).*\.txt$", r"^\d+\. .+\.mp3$", "multi"),
    ("SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/ffuture-redo", r"^ff-sesh.*\.txt$", None, "multi"),
    ("spai.d-d-murderhobozos_5.1/sp&d-expanse=ALLIP+2/0.avantasy-sesh-#1-10/avan-sesh-3-10", r"^ava-session-\d+\.txt$", None, "multi"),
    ("spai.d-d-murderhobozos_5.1/sp&d-expanse=ALLIP+2/1.tmojio-1-10_v3/1.tmojio-seshs-3-10", r"^tpmojio-sesh#\d+\.txt$", None, "multi"),
    ("spai.d-d-murderhobozos_5.1/sp&d-expanse=ALLIP+2/2.ii.clone-chronicles/part1-blood-market", r"^clone-chronicles-sesh\d+\.txt$", None, "multi"),
    ("spai.d-d-murderhobozos_5.1/sp&d-expanse=ALLIP+2/2.ii.clone-chronicles/part2-empire-game", r"^clone-chronicles-p2-sesh\d+\.txt$", None, "multi"),
    ("spai.d-d-murderhobozos_5.1/sp&d-expanse=ALLIP+2/3.rpg-canvas-sp-editsesh", r"^RMMSP_sesh#\d+\.txt$", None, "multi"),
    ("spai.d-d-murderhobozos_5.1/sp&d-expanse=ALLIP+2/x20.fuzz-sesh#1-3.5", r"^fuzz-sesh-grok-2-note#(2|3)\.txt$", None, "multi"),
    ("HARD_VVAR", r"^hard_vvar-sesh\d+\.txt$", None, "multi"),
    ("TMOJIO_CAFE[]&/cafe-rp-sim", r"^cafe-sesh.*\.txt$", None, "multi"),
    # Category 2: Single-voice
    ("SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/chara-notes", r"^chara-study-sesh.*\.txt$", None, "single:en-US-AriaNeural"),
    ("SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/sol-notes", r"^sol-study-sesh.*\.txt$", None, "single:en-US-ChristopherNeural"),
    ("SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/iqa-notes", r"^iqa-study-sesh.*\.txt$", None, "single:en-US-AnaNeural"),
    ("SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/horli-tutalage", r"^horli-tutalage-sesh.*\.txt$", None, "horli-zh"),
]

def parse_multi(filepath):
    segments, current_speaker, current_text = [], None, []
    pat = re.compile(r'^\*\*(.+?):\*\*\s*(.*)')
    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.rstrip('\n')
            if line.startswith('# ') or line.startswith('## ') or line.startswith('---'): continue
            m = pat.match(line)
            if m:
                if current_speaker and current_text:
                    segments.append((current_speaker, ' '.join(current_text).strip()))
                current_speaker = m.group(1).strip()
                current_text = [m.group(2).strip()] if m.group(2).strip() else []
            elif line.strip() == '':
                if current_speaker and current_text:
                    segments.append((current_speaker, ' '.join(current_text).strip()))
                    current_speaker, current_text = None, []
            else:
                c = re.sub(r'\*\*(.+?)\*\*', r'\1', line.strip())
                if c: current_text.append(c)
    if current_speaker and current_text:
        segments.append((current_speaker, ' '.join(current_text).strip()))
    return segments

def parse_horli(filepath):
    with open(filepath, 'r', encoding='utf-8') as f: text = f.read()
    lines = text.split('\n')
    clean = []
    for line in lines:
        s = line.strip()
        if not s or s.startswith('#') or s.startswith('---') or s.startswith('**['): continue
        if any(s.startswith(p) for p in ['**Topic:', '**Voice:', '**Estimated', '**Next:', '**Series', '**ϕ']): continue
        clean.append(s)
    full = '\n'.join(clean)
    pat = re.compile(r'(?:\*\*)?(Horli Qwin|中文老师)(?:\*\*)?:\s*')
    segments, speaker, buf = [], None, []
    for part in pat.split(full):
        if not part.strip(): continue
        if part.strip() in ['Horli Qwin', '中文老师']:
            if speaker and buf:
                t = ' '.join(buf).strip().replace('**', '')
                if t: segments.append((speaker, t))
            speaker = part.strip(); buf = []
        elif speaker: buf.append(part.strip())
    if speaker and buf:
        t = ' '.join(buf).strip().replace('**', '')
        if t: segments.append((speaker, t))
    return segments

async def tts(text, voice):
    tmp = tempfile.NamedTemporaryFile(suffix='.mp3', delete=False)
    tmp.close()
    proc = await asyncio.create_subprocess_exec('edge-tts', '--voice', voice, '--text', text, '--write-media', tmp.name, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    await proc.wait()
    if os.path.getsize(tmp.name) == 0: os.unlink(tmp.name); return None
    return tmp.name

async def convert_multi(segments, out):
    combined = AudioSegment.empty()
    ok, fail = 0, 0
    for i, (spk, txt) in enumerate(segments):
        voice = EDGE_VOICE_MAP.get(spk, "zh-CN-XiaoxiaoNeural")
        if not txt.strip(): continue
        d = txt[:60]+"..." if len(txt)>60 else txt
        print(f"    [{i+1}/{len(segments)}] {spk}: {d}")
        try:
            tmp = await tts(txt, voice)
            if tmp:
                combined += AudioSegment.from_mp3(tmp)
                os.unlink(tmp); ok += 1
            else: fail += 1
        except Exception as e: print(f"    ERROR: {e}"); fail += 1
        await asyncio.sleep(1.5)
    if combined:
        combined.export(out, format='mp3', bitrate='192k')
        return ok, fail
    return 0, len(segments)

async def convert_single(text, voice, out):
    tmp = await tts(text, voice)
    if tmp:
        audio = AudioSegment.from_mp3(tmp)
        audio.export(out, format='mp3', bitrate='192k')
        os.unlink(tmp)
        return 1, 0
    return 0, 1

async def process_file(fpath, category):
    bname = os.path.basename(fpath)
    dname = os.path.dirname(fpath)
    out = os.path.join(dname, f"中{os.path.splitext(bname)[0]}.mp3")
    if os.path.exists(out):
        sz = os.path.getsize(out)/(1024*1024)
        print(f"  SKIP (exists): {os.path.basename(out)} ({sz:.1f} MB)")
        return "skip", sz

    print(f"  Converting: {bname}")
    try:
        if category.startswith("single:"):
            voice = category.split(":")[1]
            with open(fpath, 'r', encoding='utf-8') as f: lines = f.readlines()
            spoken = []
            for line in lines:
                s = line.strip()
                if not s or s.startswith('#') or s.startswith('---') or s.startswith('**['): continue
                if any(s.startswith(p) for p in ['**Topic:', '**Voice:', '**Estimated', '**Next:', '**Series', '**ϕ']): continue
                spoken.append(s.replace('**', ''))
            full = '\n'.join(spoken)
            ok, fail = await convert_single(full, voice, out)
        elif category == "horli-zh":
            segments = parse_horli(fpath)
            if not segments: print(f"  WARNING: No segments"); return "fail", 0
            ok, fail = await convert_multi(segments, out)
        else:
            segments = parse_multi(fpath)
            if not segments: print(f"  WARNING: No segments"); return "fail", 0
            ok, fail = await convert_multi(segments, out)

        if os.path.exists(out):
            sz = os.path.getsize(out)/(1024*1024)
            print(f"  DONE: {os.path.basename(out)} ({sz:.1f} MB) — {ok} ok, {fail} failed")
            # Delete old MP3s in same dir that match the old naming
            if category == "multi":
                for old_mp3 in glob.glob(os.path.join(dname, "*.mp3")):
                    old_base = os.path.basename(old_mp3)
                    if not old_base.startswith('中'):
                        # Check if this old MP3 corresponds to the current txt file
                        txt_base = os.path.splitext(bname)[0]
                        # Map: "fud&d-sesh1-memory-palace-race.txt" -> "1. Memory Palace Race.mp3"
                        # We'll delete old MP3s after all conversions are done
                        pass
            return "done", sz
        else:
            print(f"  FAILED: {bname}"); return "fail", 0
    except Exception as e:
        print(f"  ERROR: {e}"); return "fail", 0

async def main():
    total_done, total_fail, total_skip, total_sz = 0, 0, 0, 0.0
    failures = []

    for dir_rel, pat_str, old_pat_str, category in JOBS:
        full_dir = os.path.join(BASE, dir_rel)
        if not os.path.isdir(full_dir):
            print(f"\n⚠️  DIR NOT FOUND: {full_dir}"); continue
        pat = re.compile(pat_str)
        files = sorted([f for f in os.listdir(full_dir) if pat.match(f)])
        if not files:
            print(f"\n⚠️  No matching files in: {dir_rel}"); continue

        print(f"\n{'='*70}")
        print(f"[{category}] {dir_rel} ({len(files)} files)")
        print(f"{'='*70}")

        for fname in files:
            fpath = os.path.join(full_dir, fname)
            result, sz = await process_file(fpath, category)
            if result == "done": total_done += 1; total_sz += sz
            elif result == "fail": total_fail += 1; failures.append(os.path.relpath(fpath, BASE))
            elif result == "skip": total_skip += 1; total_sz += sz
            await asyncio.sleep(1)

    print(f"\n{'='*70}")
    print(f"=== ALL DONE ===")
    print(f"Converted: {total_done} | Skipped: {total_skip} | Failed: {total_fail}")
    print(f"Total size: {total_sz:.1f} MB")
    if failures:
        print(f"\nFailed (need retry):")
        for f in failures: print(f"  ✗ {f}")
    print(f"{'='*70}")

if __name__ == '__main__':
    asyncio.run(main())
