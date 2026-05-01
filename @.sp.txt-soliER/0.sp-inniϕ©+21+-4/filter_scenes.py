import os
import re

search_dir = 'SP.all-writeez28-c0.3/'
output_file = 'fixed.txt'

def is_screenplay_scene(filepath):
    filename = os.path.basename(filepath)
    
    # User's primary criteria: "scene" type files
    if 'scene' not in filename.lower():
        return False
    
    # Exclude technical/meta files that might have 'scene' in name (unlikely but safe)
    exclude_keywords = ['plan', 'overview', 'stats', '2do', 'wussup', 'revision', 'docs', 'template', 'manifest']
    if any(k in filename.lower() for k in exclude_keywords):
        return False

    # Verify content looks like a screenplay (sluglines, character dialogue)
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read(2000)
            # Look for sluglines like INT. or EXT.
            if re.search(r'^(INT\.|EXT\.)', content, re.MULTILINE):
                return True
            # Look for character names followed by dialogue
            if re.search(r'^[A-Z][A-Z ]+\n', content, re.MULTILINE):
                return True
    except:
        pass
    
    return False

found_files = []

for root, dirs, files in os.walk(search_dir):
    # Skip known technical/book-only directories to be safe
    if 'book_template.scp' in root:
        continue
        
    for file in files:
        if file.endswith('.txt'):
            rel_path = os.path.join(root, file)
            if is_screenplay_scene(rel_path):
                # Format according to instructions: Prepend ../
                formatted_path = '../' + rel_path
                found_files.append(formatted_path)

with open(output_file, 'w') as f:
    for path in sorted(found_files):
        f.write(path + '\n')

print(f"Found {len(found_files)} scene files. Written to {output_file}")
