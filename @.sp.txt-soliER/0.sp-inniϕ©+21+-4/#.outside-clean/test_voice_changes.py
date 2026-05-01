#!/usr/bin/env python3
"""
Test script to verify voice mapping changes.
Generates a short audio sample with each character to confirm voices are correct.
"""

import asyncio
import subprocess
import os
import tempfile
from pydub import AudioSegment

# Updated voice mappings
VOICE_MAP = {
    "Mother Tmojizu": "zh-CN-XiaoxiaoNeural",
    "QOO (Chinese Teacher)": "zh-CN-YunxiaNeural",
    "Horli Qwin": "fr-FR-VivienneMultilingualNeural",
    "Sol-Supreme": "en-US-ChristopherNeural",
    "CHARA": "en-US-AriaNeural",
    "IQabella": "en-US-AnaNeural",
    "中文老师": "zh-CN-YunxiaNeural",
}

TEST_TEXTS = {
    "Mother Tmojizu": "Welcome to the session, ducklings. Let us begin our journey.",
    "QOO (Chinese Teacher)": "今天我们学习新的词汇。准备好了吗？",
    "Horli Qwin": "Yo, let's get this party started! I'm ready to roll!",
    "Sol-Supreme": "I will analyze the situation systematically. Efficiency is key.",
    "CHARA": "I've already mapped out the optimal strategy. Let's move.",
    "IQabella": "I'm here to support everyone. Let's do our best together!",
}

OUTPUT_DIR = "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+12/voice_test_output"

async def generate_test_audio():
    """Generate test audio for each character."""
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    print("=" * 60)
    print("VOICE MAPPING TEST")
    print("=" * 60)
    
    combined = AudioSegment.empty()
    
    for speaker, text in TEST_TEXTS.items():
        voice = VOICE_MAP[speaker]
        print(f"\n{speaker} → {voice}")
        print(f"  Text: {text}")
        
        # Generate audio
        tmp_file = tempfile.NamedTemporaryFile(suffix='.mp3', delete=False)
        tmp_file.close()
        
        proc = await asyncio.create_subprocess_exec(
            'edge-tts',
            '--voice', voice,
            '--text', text,
            '--write-media', tmp_file.name,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        await proc.wait()
        
        if os.path.exists(tmp_file.name) and os.path.getsize(tmp_file.name) > 0:
            audio = AudioSegment.from_mp3(tmp_file.name)
            
            # Add small pause
            combined += audio + AudioSegment.silent(duration=500)
            
            size = os.path.getsize(tmp_file.name) / (1024 * 1024)
            print(f"  ✓ Generated ({size:.2f} MB)")
            
            os.unlink(tmp_file.name)
        else:
            print(f"  ✗ FAILED")
    
    # Save combined test file
    output_path = os.path.join(OUTPUT_DIR, "中voice_test_all.mp3")
    combined.export(output_path, format='mp3', bitrate='192k')
    total_size = os.path.getsize(output_path) / (1024 * 1024)
    
    print(f"\n{'=' * 60}")
    print(f"TEST COMPLETE")
    print(f"Output: {output_path}")
    print(f"Total size: {total_size:.2f} MB")
    print(f"{'=' * 60}")
    
    # Also save individual files
    print(f"\nGenerating individual speaker files...")
    
    for speaker, text in TEST_TEXTS.items():
        voice = VOICE_MAP[speaker]
        safe_name = speaker.replace(' ', '_').replace('(', '').replace(')', '').replace('中', 'zh')
        output_file = os.path.join(OUTPUT_DIR, f"中test_{safe_name}.mp3")
        
        tmp_file = tempfile.NamedTemporaryFile(suffix='.mp3', delete=False)
        tmp_file.close()
        
        proc = await asyncio.create_subprocess_exec(
            'edge-tts',
            '--voice', voice,
            '--text', text,
            '--write-media', tmp_file.name,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        await proc.wait()
        
        if os.path.exists(tmp_file.name) and os.path.getsize(tmp_file.name) > 0:
            audio = AudioSegment.from_mp3(tmp_file.name)
            audio.export(output_file, format='mp3', bitrate='192k')
            size = os.path.getsize(output_file) / (1024 * 1024)
            print(f"  ✓ {speaker}: {output_file} ({size:.2f} MB)")
            
            os.unlink(tmp_file.name)

if __name__ == '__main__':
    asyncio.run(generate_test_audio())
