#!/bin/sh
# check_size.sh - Show disk usage of files and folders
# Usage: ./#.tools/check_size.sh > #.tools/size_list.txt

cd "$(dirname "$0")/.."

echo "=== TPMOS SIZE AUDIT ===" 
echo "Date: $(date)"
echo ""

echo "=== ROOT LEVEL DIRECTORIES ==="
du -sh */ 2>/dev/null | sort -hr
echo ""

echo "=== HIDDEN DIRECTORIES ==="
du -sh #.*/ 2>/dev/null | sort -hr
echo ""

echo "=== TOP 30 LARGEST DIRECTORIES ==="
find . -type d -exec du -sh {} \; 2>/dev/null | sort -hr | head -30
echo ""

echo "=== TOP 30 LARGEST FILES ==="
find . -type f -exec du -h {} \; 2>/dev/null | sort -hr | head -30
echo ""

echo "=== LEGACY ARCHIVE (candidates for deletion) ==="
du -sh projects/trunk/legacy_archive/* 2>/dev/null | sort -hr
echo ""

echo "=== INSTALLED APPS ==="
du -sh pieces/apps/installed/* 2>/dev/null | sort -hr
echo ""

echo "=== TOTAL PROJECT SIZE ==="
du -sh . 2>/dev/null
