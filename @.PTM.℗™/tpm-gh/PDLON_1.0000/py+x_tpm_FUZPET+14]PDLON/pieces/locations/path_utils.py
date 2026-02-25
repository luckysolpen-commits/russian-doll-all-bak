import os
import sys

_project_root = None
_pieces_dir = None

def init_paths():
    global _project_root, _pieces_dir
    
    if _project_root is not None:
        return True
    
    # Try multiple locations
    locations = [
        'pieces/locations/location_kvp',
        '../pieces/locations/location_kvp',
        os.path.join(os.path.dirname(__file__), 'location_kvp'),
    ]
    
    for loc_path in locations:
        if os.path.exists(loc_path):
            with open(loc_path, 'r') as f:
                for line in f:
                    if line.startswith('project_root='):
                        _project_root = line.strip().split('=', 1)[1]
                    elif line.startswith('pieces_dir='):
                        _pieces_dir = line.strip().split('=', 1)[1]
            break
    
    if _project_root is None:
        print("Error: Cannot find location_kvp", file=sys.stderr)
        return False
    
    return True

def get_project_root():
    if not _project_root and not init_paths():
        return None
    return _project_root

def get_pieces_dir():
    if not _pieces_dir and not init_paths():
        return None
    return _pieces_dir

def build_path(relative_path):
    """Build absolute path from project root"""
    root = get_project_root()
    if not root:
        return None
    return os.path.join(root, relative_path)
