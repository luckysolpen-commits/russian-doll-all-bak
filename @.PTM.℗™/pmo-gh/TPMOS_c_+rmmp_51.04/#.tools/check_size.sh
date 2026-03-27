#!/bin/sh
# check_size.sh - Show disk usage of files and folders

cd "$(dirname "$0")/.."

echo "=== TOP 20 LARGEST FILES/FOLDERS ==="
du -ah . 2>/dev/null | sort -rh | head -n 20
