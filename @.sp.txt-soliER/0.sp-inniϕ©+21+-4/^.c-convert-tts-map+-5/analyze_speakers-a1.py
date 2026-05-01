import re
import os
from collections import Counter
from datetime import datetime

SCENE_DIR = "./111.tear-it]accidental.witch]y3/tearit-scenes"

EXISTING_VOICE_MAP = {
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
    "PIT BOSS": "ru-RU-DmitriNeural",
    "MANAGER": "en-US-SteffanNeural",
    "UN AGENT": "en-GB-OliverNeural",
    "MERC 1": "en-NG-AbeoNeural",
    "CHARA": "fr-FR-VivienneMultilingualNeural",
    "IQABELLA": "en-US-AnaNeural",
    "HORLI": "fr-FR-DeniseNeural",
    "RAHWEH": "zh-CN-YunxiaNeural",
     # screen
    "ZARA": "es-ES-ElviraNeural",
    "QUEEN": "en-NG-EzinneNeural",
    "VANCE": "en-IE-ConnorNeural",
    "CHAIRMAN": "it-IT-GiuseppeMultilingualNeural",
    "RATCHET": "de-DE-ConradNeural",
    "DR. ARIS": "hi-IN-SwaraNeural",
    #charas
    "VICTOR LAGRAINE": "en-US-EricNeural",
    "THOMPSON": "en-US-SteffanNeural",
    "MAN": "en-US-AndrewNeural",
    "WOMAN": "en-US-AriaNeural",
    "---": "pt-PT-DuarteNeural",
    "SISTER": "en-CA-ClaraNeural",
    "EDGE": "en-AU-WilliamNeural"
    
}

def parse_scene_for_speakers(text: str) -> list:
    """Robust speaker extraction supporting multiple formats"""
    speakers = []
    lines = text.split('\n')
    
    for line in lines:
        line = line.strip()
        if not line:
            continue

        # 1. <center> tag
        center_match = re.search(r'<center>([^<]+)</center>', line, re.IGNORECASE)
        if center_match:
            name = center_match.group(1).strip().upper()
            if name:
                speakers.append(name)
            continue

        # 2. ALL CAPS line (screenplay character name)
        if re.match(r'^[A-Z0-9\s\.\-\'’&]+$', line) and 2 < len(line) < 60:
            if not re.match(r'^(INT\.|EXT\.|FADE|CUT TO|DISSOLVE|TRANSITION)', line, re.IGNORECASE):
                speakers.append(line.strip().upper())
                continue

        # 3. CHARACTER: dialogue format
        colon_match = re.match(r'^([A-Z0-9\s\.\-\'’&]+?)\s*:\s*', line)
        if colon_match:
            name = colon_match.group(1).strip().upper()
            if name and len(name) > 1:
                speakers.append(name)

    return speakers


def analyze_scenes(scene_dir: str, existing_voice_map_keys: set):
    all_speakers_found = Counter()
    unmapped_speakers = Counter()

    if not os.path.isdir(scene_dir):
        print(f"Error: Scene directory '{scene_dir}' not found.")
        return None, None

    try:
        scene_files = [f for f in os.listdir(scene_dir) 
                      if f.startswith('scene_') and f.endswith('.txt')]
        if not scene_files:
            print(f"No 'scene_*.txt' files found in '{scene_dir}'.")
            return None, None
    except OSError as e:
        print(f"Error listing directory '{scene_dir}': {e}")
        return None, None

    print(f"Found {len(scene_files)} scene files to analyze.\n")

    for scene_file_name in sorted(scene_files):
        scene_file_path = os.path.join(scene_dir, scene_file_name)
        try:
            with open(scene_file_path, 'r', encoding='utf-8') as f:
                scene_text = f.read()

            speakers_in_scene = parse_scene_for_speakers(scene_text)
            
            for speaker in speakers_in_scene:
                all_speakers_found[speaker] += 1
                if speaker not in existing_voice_map_keys:
                    unmapped_speakers[speaker] += 1

        except Exception as e:
            print(f"Error processing {scene_file_name}: {e}")

    return all_speakers_found, unmapped_speakers


def write_unmapped_to_file(unmapped_speakers: Counter, scene_dir: str):
    """Write unmapped speakers to file WITHOUT numbering - clean format"""
    if not unmapped_speakers:
        print("✅ No unmapped speakers. All characters already have voice assignments.")
        return

    # Create safe filename from scene directory
    safe_name = os.path.basename(scene_dir.rstrip('/\\')).replace('/', '_').replace('\\', '_')
    output_filename = f"{safe_name}_voices.txt"

    try:
        with open(output_filename, 'w', encoding='utf-8') as f:
            f.write(f"# Voice mappings for: {scene_dir}\n")
            f.write(f"# Generated on: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write("# Copy and paste these lines directly into your EXISTING_VOICE_MAP dictionary\n\n")
            
            for speaker, count in unmapped_speakers.most_common():
                # Default suggestion - change as needed
                suggested_voice = "en-US-AndrewNeural"
                
                line = f'    "{speaker}": "{suggested_voice}",'
                f.write(line + '\n')
                print(line)

        print(f"\n✅ Successfully created: {output_filename}")
        print(f"   → {len(unmapped_speakers)} unmapped speakers written.")
        print("   → No numbering (clean format as requested)")
        print("\n   Edit the voice IDs in this file as needed, then copy-paste into EXISTING_VOICE_MAP.")

    except Exception as e:
        print(f"Error writing to file: {e}")


if __name__ == "__main__":
    existing_map_keys = set(EXISTING_VOICE_MAP.keys())
    all_speakers, unmapped_speakers = analyze_scenes(SCENE_DIR, existing_map_keys)

    if all_speakers is None:
        exit(1)

    print("--- All Speakers Found (total occurrences) ---")
    for speaker, count in all_speakers.most_common():
        print(f"- {speaker}: {count}")
    print("-" * 70)

    print("\n--- Unmapped Speakers (need voice assignment) ---")
    write_unmapped_to_file(unmapped_speakers, SCENE_DIR)
    
    print("\n" + "="*70)
    print("Next step: Open the generated .txt file,")
    print("replace the voice IDs with your preferred ones,")
    print("then send me the updated list so I can add them to your EXISTING_VOICE_MAP.")
