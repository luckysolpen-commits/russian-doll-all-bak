#!/usr/bin/env python3
"""
Rename files to remove problematic special characters.
Keeps: letters, numbers, underscores, dots, hyphens
Removes: emoji, symbols, and other problematic characters (!, #, @, ^, &, etc.)
"""
import os
import re
import unicodedata

def sanitize_filename(name):
    """Remove problematic characters while keeping the filename recognizable."""
    # Map of common special chars to their safe replacements
    replacements = {
        '@': '_',
        '&': '-',
        '=': '-',
        '+': '-',
    }
    
    # Apply character replacements
    for old, new in replacements.items():
        name = name.replace(old, new)
    
    # Remove emoji, symbols, and other non-ASCII special characters
    # Keep only: ASCII letters, digits, underscore, dot, hyphen
    safe_chars = []
    for char in name:
        # Keep ASCII alphanumeric, underscore, dot, hyphen
        if re.match(r'[a-zA-Z0-9_\.\-]', char):
            safe_chars.append(char)
        # Skip everything else (emoji, symbols, non-ASCII chars)
        else:
            continue
    
    name = ''.join(safe_chars)
    
    # Clean up multiple consecutive dots or hyphens
    name = re.sub(r'\.{2,}', '.', name)
    name = re.sub(r'-{2,}', '-', name)
    name = re.sub(r'_{2,}', '_', name)
    
    # Remove leading/trailing dots, hyphens, underscores
    name = name.strip('.-_')
    
    # If name becomes empty, use a fallback
    if not name or name == '':
        name = 'unnamed'
    
    return name

def rename_files_in_directory(directory):
    """Rename all files and directories in the given directory."""
    renamed = []
    skipped = []
    
    # Get all items in directory
    items = sorted(os.listdir(directory))
    
    for old_name in items:
        # Skip __pycache__ and hidden files starting with .
        if old_name == '__pycache__':
            continue
            
        old_path = os.path.join(directory, old_name)
        new_name = sanitize_filename(old_name)
        
        # If name didn't change, skip
        if new_name == old_name:
            skipped.append(old_name)
            continue
        
        # Handle name conflicts
        new_path = os.path.join(directory, new_name)
        if os.path.exists(new_path):
            base, ext = os.path.splitext(new_name)
            counter = 1
            while os.path.exists(new_path):
                new_name = f"{base}_{counter}{ext}"
                new_path = os.path.join(directory, new_name)
                counter += 1
        
        try:
            os.rename(old_path, new_path)
            renamed.append((old_name, new_name))
            print(f"✓ {old_name}")
            print(f"  → {new_name}")
        except Exception as e:
            print(f"✗ Failed to rename {old_name}: {e}")
    
    return renamed, skipped

if __name__ == '__main__':
    target_dir = '/home/no/Desktop/Piecemark-IT/中ϕ_00.00/sp-inniϕ©+5'
    
    print(f"Sanitizing filenames in: {target_dir}\n")
    renamed, skipped = rename_files_in_directory(target_dir)
    
    print(f"\n{'='*60}")
    print(f"Renamed: {len(renamed)} items")
    print(f"Skipped (already safe): {len(skipped)} items")
    
    if renamed:
        print(f"\nSummary of changes:")
        for old, new in renamed:
            print(f"  {old} → {new}")
