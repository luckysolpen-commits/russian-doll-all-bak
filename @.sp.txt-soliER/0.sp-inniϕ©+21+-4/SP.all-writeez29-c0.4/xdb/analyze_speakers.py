import re
import os
from collections import Counter

SCENE_DIR = "2.RedRock.AndROLL]ING]mid]a1/rr-screenplay"

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
    "ZARA": "en-US-ElviraNeural",
    "QUEEN": "en-US-EzinneNeural",
    "VANCE": "en-US-ConnorNeural",
    "CHAIRMAN": "en-US-GiuseppeMultilingualNeural",
    "RATCHET": "en-US-AndrewNeural",
    "DR. ARIS": "en-US-AndrewNeural",
}

def parse_scene_for_speakers(text: str) -> list:
    """
    Robust speaker extraction from scene text.
    Supports multiple common screenplay formats.
    """
    speakers = []
    lines = text.split('\n')
    
    for line in lines:
        line = line.strip()
        if not line:
            continue

        # 1. Original <center> tag format
        center_match = re.search(r'<center>([^<]+)</center>', line, re.IGNORECASE)
        if center_match:
            name = center_match.group(1).strip().upper()
            if name:
                speakers.append(name)
            continue

        # 2. ALL CAPS on its own line (common in screenplays)
        if re.match(r'^[A-Z\s0-9\.\-\'’&]+$', line) and len(line) > 2 and len(line) < 60:
            # Avoid matching scene headings like INT. or EXT.
            if not re.match(r'^(INT\.|EXT\.|FADE|CUT TO|DISSOLVE)', line, re.IGNORECASE):
                name = line.strip().upper()
                speakers.append(name)
                continue

        # 3. Speaker: dialogue format (e.g. LUCKY: Hello)
        colon_match = re.match(r'^([A-Z\s0-9\.\-\'’&]+?)\s*:\s*', line)
        if colon_match:
            name = colon_match.group(1).strip().upper()
            if name and len(name) > 1:
                speakers.append(name)
                continue

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

    for scene_file_name in sorted(scene_files):  # sorted for consistent output
        scene_file_path = os.path.join(scene_dir, scene_file_name)
        try:
            with open(scene_file_path, 'r', encoding='utf-8') as f:
                scene_text = f.read()

            speakers_in_scene = parse_scene_for_speakers(scene_text)
            
            for speaker in speakers_in_scene:
                all_speakers_found[speaker] += 1
                if speaker not in existing_voice_map_keys:
                    unmapped_speakers[speaker] += 1

            # Optional: show per-scene summary (uncomment if you want)
            # if speakers_in_scene:
            #     print(f"{scene_file_name}: {len(set(speakers_in_scene))} unique speakers")

        except Exception as e:
            print(f"Error processing {scene_file_name}: {e}")

    return all_speakers_found, unmapped_speakers


if __name__ == "__main__":
    existing_map_keys = set(EXISTING_VOICE_MAP.keys())
    all_speakers, unmapped_speakers = analyze_scenes(SCENE_DIR, existing_map_keys)

    if all_speakers is None:
        exit(1)

    print("--- All Speakers Found (and their total occurrences) ---")
    if all_speakers:
        for speaker, count in all_speakers.most_common():
            print(f"- {speaker}: {count}")
    else:
        print("No speakers were identified in any scene files.")
    print("-" * 70)

    print("\n--- Unmapped Speakers (need voice assignment) ---")
    if unmapped_speakers:
        for speaker, count in unmapped_speakers.most_common():
            print(f"- {speaker}: {count} occurrences")
    else:
        print("✅ All identified speakers are already mapped in the VOICE_MAP.")
    print("-" * 70)

    if unmapped_speakers:
        print("\nTo add new voices, reply with mappings in this format:")
        print("   'CHARACTER NAME': 'voice-id'")
        print("Example:")
        print("   'NEW REBEL': 'en-US-GuyNeural'")
        print("   'QUEEN OF HEARTS': 'en-GB-SoniaNeural'")
    else:
        print("\n🎉 All speakers are mapped! You're ready to generate audio.")
