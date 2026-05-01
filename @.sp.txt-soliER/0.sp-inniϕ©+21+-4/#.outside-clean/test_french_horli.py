#!/usr/bin/env python3
"""Test all 3 French female voices for Horli."""
import asyncio, os, subprocess, tempfile
from pydub import AudioSegment

TEXT = "Yo, let's get this party started! I'm ready to roll! I'm the class clown, CHA 17, performance plus seven. Watch me steal the show."
VOICES = ["fr-FR-DeniseNeural", "fr-FR-EloiseNeural", "fr-FR-VivienneMultilingualNeural"]
OUT = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+12/voice_test_output"
os.makedirs(OUT, exist_ok=True)

async def tts(text, voice, out):
    tmp = tempfile.NamedTemporaryFile(suffix='.mp3', delete=False); tmp.close()
    proc = await asyncio.create_subprocess_exec('edge-tts', '--voice', voice, '--text', text, '--write-media', tmp.name, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    await proc.wait()
    if os.path.getsize(tmp.name) > 0:
        AudioSegment.from_mp3(tmp.name).export(out, format='mp3', bitrate='192k')
        os.unlink(tmp.name)
        print(f"✓ {voice} → {os.path.basename(out)}")

async def main():
    for v in VOICES:
        name = v.replace('fr-FR-','').replace('Neural','')
        await tts(TEXT, v, os.path.join(OUT, f"horli_test_{name}.mp3"))
    print("\nAll 3 Horli French voice tests done!")

asyncio.run(main())
