#!/usr/bin/env python3
"""Generate voice samples for all voices in voice-list.txt."""
import asyncio, os, subprocess, tempfile
from pydub import AudioSegment

OUT = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+12/!.INNI.BIZ_META/2.output-2-lucky/voice-samples"
os.makedirs(OUT, exist_ok=True)

TEXT = "Hello, I'm testing this voice for the TPMOJIO platform. We're building a text-based piece-managed operating system with P2P trading, fuzzpets, and user-created games. Welcome to the future of interactive entertainment."

VOICES = [
    # Main characters
    ("zh-CN-XiaoxiaoNeural", "Mother_Tmojizu_MAIN"),
    ("zh-CN-YunxiaNeural", "QOO_MAIN"),
    ("fr-FR-VivienneMultilingualNeural", "CHARA_MAIN"),
    ("fr-FR-DeniseNeural", "Horli_Qwin_MAIN"),
    ("en-US-ChristopherNeural", "Sol_Supreme_MAIN"),
    ("en-US-AnaNeural", "IQabella_MAIN"),
    # English US
    ("en-US-AndrewNeural", "Andrew"),
    ("en-US-AriaNeural", "Aria"),
    ("en-US-AvaNeural", "Ava"),
    ("en-US-BrianNeural", "Brian"),
    ("en-US-EmmaNeural", "Emma"),
    ("en-US-EricNeural", "Eric"),
    ("en-US-GuyNeural", "Guy"),
    ("en-US-JennyNeural", "Jenny"),
    ("en-US-MichelleNeural", "Michelle"),
    ("en-US-RogerNeural", "Roger"),
    # English UK
    ("en-GB-LibbyNeural", "Libby_UK"),
    ("en-GB-MaisieNeural", "Maisie_UK"),
    ("en-GB-RyanNeural", "Ryan_UK"),
    ("en-GB-SoniaNeural", "Sonia_UK"),
    ("en-GB-ThomasNeural", "Thomas_UK"),
    # African
    ("en-NG-AbeoNeural", "Abeo_Nigeria"),
    ("en-NG-EzinneNeural", "Ezinne_Nigeria"),
    ("en-KE-AsiliaNeural", "Asilia_Kenya"),
    ("en-KE-ChilembaNeural", "Chilemba_Kenya"),
    ("en-ZA-LeahNeural", "Leah_SouthAfrica"),
    ("en-ZA-LukeNeural", "Luke_SouthAfrica"),
    # Other
    ("ar-AE-HamdanNeural", "Hamdan_UAE"),
    ("ja-JP-KeitaNeural", "Keita_Japan"),
    ("ru-RU-DmitryNeural", "Dmitry_Russia"),
    ("de-DE-KatjaNeural", "Katja_Germany"),
    ("es-MX-DaliaNeural", "Dalia_Mexico"),
    ("it-IT-DiegoNeural", "Diego_Italy"),
    ("pt-BR-AntonioNeural", "Antonio_Brazil"),
    ("ko-KR-InJoonNeural", "InJoon_Korea"),
]

async def gen(voice, name):
    tmp = tempfile.NamedTemporaryFile(suffix='.mp3', delete=False); tmp.close()
    proc = await asyncio.create_subprocess_exec('edge-tts', '--voice', voice, '--text', TEXT, '--write-media', tmp.name, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    await proc.wait()
    if os.path.getsize(tmp.name) > 0:
        AudioSegment.from_mp3(tmp.name).export(os.path.join(OUT, f"{name}.mp3"), format='mp3', bitrate='128k')
        os.unlink(tmp.name)
        print(f"  ✓ {name}")
    else:
        os.unlink(tmp.name)
        print(f"  ✗ {name}")

async def main():
    print(f"Generating {len(VOICES)} voice samples...")
    for voice, name in VOICES:
        await gen(voice, name)
        await asyncio.sleep(1)
    print(f"\nDone! All samples in: {OUT}")

asyncio.run(main())
