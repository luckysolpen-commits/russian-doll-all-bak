import os
import re

search_dirs = ['spai.d-d-murderhobozos_c.0', 'TMOJIO_CAFE[]&']
output_file = 'found_scripts.txt'

exclusions = [
    'plan', 'overview', 'financial-model', 'phased-rollout', 
    'ip-strategy', 'bootstrap-vs-investor', 'community-programming',
    'RPG-CANVAS-SP-BIBLE', 'TPMOS_TEXTBOOK', 'character&stats', 
    'stats', 'character-data', '2do.txt', 'wussup', 'REVISION_LOG',
    'd&d-aiguys-murderhobos10k', 'concept'
]

specific_exclusions = [
    'documentation.txt', 'XNIL.txt', '!.TPMOJIO_BIBLE.txt',
    '#.ava-server-0001.txt', '#.tomo-engine-ava1.txt', 'tmojio-cafe-brick&.txt'
]

found_files = []

for s_dir in search_dirs:
    for root, dirs, files in os.walk(s_dir):
        for file in files:
            if not file.endswith('.txt'):
                continue
            
            path = os.path.join(root, file)
            
            # Exclusion criteria
            lower_path = path.lower()
            excluded = False
            for exc in exclusions:
                if exc.lower() in lower_path:
                    excluded = True
                    break
            if excluded:
                continue
                
            if file in specific_exclusions:
                continue
                
            # Content relevance check (simplified as name analysis for now, but following inclusion list)
            # We want narrative, scripts, lore.
            
            found_files.append(path)

with open(output_file, 'w') as f:
    for f_path in found_files:
        f.write(f'../{f_path}\n')

print(f'Found {len(found_files)} files.')
