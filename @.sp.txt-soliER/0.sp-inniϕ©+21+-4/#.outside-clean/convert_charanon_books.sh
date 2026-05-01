#!/bin/bash
# CHAR.A.NON Books — Batch Converter
# Converts all 5 CHAR.A.NON books with Vivienne voice, no pauses.
# Usage: bash convert_charanon_books.sh

BASE="/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+12"
SCRIPT="$BASE/batch_convert.py"

echo "=== CHAR.A.NON Books Converter ==="
echo "Voice: fr-FR-VivienneMultilingualNeural (CHARA)"
echo "Method: page_*.txt → single space join (no pauses)"
echo ""

# Wave 1: First 2 books (8 processes max)
echo "Wave 1: Books 1010% and 1100..."
python3 "$SCRIPT" "$BASE/SP.all-writeez28-b2/-1.MyChara+21]b1/CHAR.A.NON]c3]++/-1.CHAR.A.NON]1111111111111011/BOOK]1010%" "^Chapter_\d+$" "charanon" &
python3 "$SCRIPT" "$BASE/SP.all-writeez28-b2/-1.MyChara+21]b1/CHAR.A.NON]c3]++/-1.CHAR.A.NON]1111111111111100/book_template.scp]1100" "^chapter_\d+$" "charanon" &
wait

echo "Wave 2: Books 101, 110, 111..."
python3 "$SCRIPT" "$BASE/SP.all-writeez28-b2/-1.MyChara+21]b1/CHAR.A.NON]c3]++/-1.CHAR.A.NON]1111111111111101/#.book_template.scp]101" "^chapter_\d+$" "charanon" &
python3 "$SCRIPT" "$BASE/SP.all-writeez28-b2/-1.MyChara+21]b1/CHAR.A.NON]c3]++/-1.CHAR.A.NON]1111111111111110/#.book_template.scp]110]fix" "^chapter_\d+$" "charanon" &
python3 "$SCRIPT" "$BASE/SP.all-writeez28-b2/-1.MyChara+21]b1/CHAR.A.NON]c3]++/-1.CHAR.A.NON]1111111111111111/#.book_template.scp]111" "^chapter_\d+$" "charanon" &
wait

echo ""
echo "=== CHAR.A.NON DONE ==="
echo "Total converted:"
find "$BASE/SP.all-writeez28-b2/-1.MyChara+21]b1/CHAR.A.NON]c3]++" -name "中*.mp3" -type f 2>/dev/null | wc -l
