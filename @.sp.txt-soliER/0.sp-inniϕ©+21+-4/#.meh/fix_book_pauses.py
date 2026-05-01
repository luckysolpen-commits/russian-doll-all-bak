
#!/usr/bin/env python3
import os
import asyncio
import re
import sys
import tempfile
import subprocess
import glob
from pydub import AudioSegment

VOICE = "en-US-AvaNeural"
CHUNK_SIZE = 25000  # Safe limit for edge-tts

async def tts_chunk(text, voice, output_path):
    proc = await asyncio.create_subprocess_exec(
        'edge-tts', '--voice', voice, '--text', text, '--write-media', output_path,
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
    )
    await proc.wait()
    return os.path.exists(output_path) and os.path.getsize(output_path) > 0

async def convert_chapter(ch_dir, output_name, voice=VOICE):
    # Delete ALL old mp3 files in the chapter directory
    for old_mp3 in glob.glob(os.path.join(ch_dir, "*.mp3")):
        try:
            os.unlink(old_mp3)
        except Exception as e:
            print(f"  Warning: Could not delete {old_mp3}: {e}")

    # Find all .txt files, sorted numerically if they are page_N.txt
    txt_files = [f for f in os.listdir(ch_dir) if f.endswith(".txt")]
    
    # Sort pages numerically: page_1.txt, page_2.txt...
    def sort_key(name):
        nums = re.findall(r'\d+', name)
        return int(nums[0]) if nums else 0
    
    page_files = sorted([f for f in txt_files if f.startswith("page_")], key=sort_key)
    if not page_files:
        page_files = sorted(txt_files)

    if not page_files:
        return False, "No txt files found"

    combined_text = []
    for pf in page_files:
        with open(os.path.join(ch_dir, pf), 'r', encoding='utf-8', errors='replace') as f:
            content = f.read().strip()
            # Clean newlines to prevent pauses
            content = re.sub(r'\s+', ' ', content)
            combined_text.append(content)

    full_text = " ".join(combined_text).strip()
    if not full_text:
        return False, "Empty text"

    chunks = [full_text[i:i+CHUNK_SIZE] for i in range(0, len(full_text), CHUNK_SIZE)]
    combined_audio = AudioSegment.empty()
    
    for i, chunk in enumerate(chunks):
        with tempfile.NamedTemporaryFile(suffix=".mp3", delete=False) as tmp:
            tmp_path = tmp.name
        
        success = await tts_chunk(chunk, voice, tmp_path)
        if success:
            combined_audio += AudioSegment.from_mp3(tmp_path)
            os.unlink(tmp_path)
        else:
            if os.path.exists(tmp_path): os.unlink(tmp_path)
            return False, f"TTS failed at chunk {i}"
        
        await asyncio.sleep(0.5)

    output_path = os.path.join(ch_dir, output_name)
    combined_audio.export(output_path, format="mp3", bitrate="192k")
    return True, output_path

async def process_book(book_dir, book_name, voice=VOICE):
    if not os.path.isdir(book_dir):
        print(f"Error: {book_dir} is not a directory")
        return

    # Find chapter directories first
    chapter_dirs = sorted([d for d in os.listdir(book_dir) if (d.startswith("chapter_") or d.startswith("Chapter_")) and os.path.isdir(os.path.join(book_dir, d))], 
                          key=lambda x: int(re.findall(r'\d+', x)[0]) if re.findall(r'\d+', x) else 0)

    paths_to_process = []
    if chapter_dirs:
        # If chapter directories are found, use them
        print(f"Found {len(chapter_dirs)} chapter directories.")
        for ch in chapter_dirs:
            paths_to_process.append(os.path.join(book_dir, ch))
    else:
        # If no chapter directories, assume book_dir itself contains the text files
        print(f"No chapter directories found in {book_dir}. Processing text files directly in {book_dir}.")
        paths_to_process.append(book_dir) # Treat the book_dir as the source for text files

    print(f"Processing book: {book_name} (Voice: {voice})")
    print(f"Source directory for text files: {book_dir}") # Clarify source dir

    for ch_dir in paths_to_process: # Iterate through the identified chapter directories or the book_dir itself
        # Determine chapter/source directory name for output naming
        source_name = os.path.basename(ch_dir)
        
        # Try to extract chapter number for naming
        ch_num_match = re.findall(r'\d+', source_name)
        ch_num = ch_num_match[0] if ch_num_match else "X"
        
        # New naming convention: [Book Name] - Chapter [N].mp3 or based on source name if no number
        if source_name.startswith("chapter_") or source_name.startswith("Chapter_"):
            output_name = f"{book_name} - Chapter {ch_num}.mp3"
        else: # If processing directly from book_dir or a non-standard chapter name
            output_name = f"{book_name} - {source_name}.mp3"
            if ch_num == "X": # If no number found and it's not a standard chapter name
                output_name = f"{book_name} - {source_name}.mp3" # Use the source name itself for output
            else:
                output_name = f"{book_name} - Source-{ch_num}.mp3" # Fallback for numbered non-chapter dirs

        print(f"  Processing source: {source_name} -> {output_name}...", end=" ", flush=True)
        
        success, msg = await convert_chapter(ch_dir, output_name, voice)
        if success:
            print(f"DONE")
        else:
            print(f"FAILED: {msg}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 fix_book_pauses.py <book_directory> <book_name> [voice]")
        sys.exit(1)
    
    target_voice = sys.argv[3] if len(sys.argv) > 3 else VOICE
    asyncio.run(process_book(sys.argv[1], sys.argv[2], target_voice))
