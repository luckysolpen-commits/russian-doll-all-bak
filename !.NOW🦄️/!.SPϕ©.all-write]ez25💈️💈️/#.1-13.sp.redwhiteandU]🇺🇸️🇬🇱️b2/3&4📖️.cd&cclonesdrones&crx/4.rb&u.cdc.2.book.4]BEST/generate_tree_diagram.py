import re
import os

directory = '/home/no/Desktop/qwen/nov-24/cd&c/cdc-short/rb&u.cyoa_book4]cd&c2/'
page_data = {}

for filename in os.listdir(directory):
    if filename.startswith('page_') and filename.endswith('.txt'):
        page_num_match = re.search(r'page_(\d+)\.txt', filename)
        if not page_num_match:
            continue

        page_num = int(page_num_match.group(1))
        
        is_ending = False

        with open(os.path.join(directory, filename), 'r') as f:
            content = f.read()
            
            # Use findall for all choices in the content
            # Using re.MULTILINE to allow '^' to match at the start of each line
            # Using '\s*' to account for variable whitespace
            choices = [int(p) for p in re.findall(r'^\s*\*\s*If you.*?turn to page (\d+)\.', content, re.MULTILINE)]
            
            # Check for ending
            if 'THE END' in content:
                is_ending = True
        
        page_data[page_num] = {'choices': sorted(list(set(choices))), 'is_ending': is_ending}

# Sort page numbers for consistent output
sorted_page_nums = sorted(page_data.keys())

# Generate the tree diagram
diagram_lines = []
diagram_lines.append('CYOA Book 4 Branch Overview (Page Connections):')
diagram_lines.append('----------------------------------------------')
diagram_lines.append('')

for page_num in sorted_page_nums:
    data = page_data.get(page_num)
    if not data:
        diagram_lines.append(f'PAGE {page_num}: (ERROR: Page data missing)')
        continue

    line_parts = [f'PAGE {page_num}:']
    
    if data['is_ending']:
        line_parts.append('(END)')
    
    if data['choices']:
        targets = ', '.join(f'PAGE {t}' for t in data['choices'])
        line_parts.append(f'-> {targets}')
    elif not data['is_ending']: # This condition means no choices AND not an ending, so it's truly a missing link
        line_parts.append('(DEAD END / MISSING CONTINUATION - NO CHOICES AND NOT AN ENDING)')
    
    diagram_lines.append(' '.join(line_parts))

# This part is for writing the output to a file, which I'd run if the user allowed
# output_filename = os.path.join(directory, 'book4.tree_diagram.txt')
# with open(output_filename, 'w') as f:
#     f.write('\n'.join(diagram_lines))

# print(f'Tree diagram created at {output_filename}')
# Instead of writing to file, I'll just print the diagram directly to the user
print('\n'.join(diagram_lines))
