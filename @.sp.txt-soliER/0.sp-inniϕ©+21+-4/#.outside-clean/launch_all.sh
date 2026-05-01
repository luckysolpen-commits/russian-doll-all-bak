#!/bin/bash
BASE="/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+12"
SCRIPT="$BASE/batch_convert.py"

echo "Step 1: Renaming Mother Tomokazu → Mother Tmojizu in all source files..."
find "$BASE" -name "*.txt" -type f -exec sed -i 's/Mother Tomokazu/Mother Tmojizu/g' {} +
echo "Done renaming."

echo "Step 2: Launching WAVE 1 (8 batches)..."

# Wave 1: 8 batches
python3 "$SCRIPT" "$BASE/SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/1.ffu-ez_learnd&d+chem" "^fud&d-sesh(?!.*(?:commas|lore)).*\.txt$" "multi" &
python3 "$SCRIPT" "$BASE/SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/ffuture-redo" "^ff-sesh.*\.txt$" "multi" &
python3 "$SCRIPT" "$BASE/spai.d-d-murderhobozos_5.1/sp&d-expanse=ALLIP+2/0.avantasy-sesh-#1-10/avan-sesh-3-10" "^ava-session-\d+\.txt$" "multi" &
python3 "$SCRIPT" "$BASE/spai.d-d-murderhobozos_5.1/sp&d-expanse=ALLIP+2/1.tmojio-1-10_v3/1.tmojio-seshs-3-10" "^tpmojio-sesh#\d+\.txt$" "multi" &
python3 "$SCRIPT" "$BASE/spai.d-d-murderhobozos_5.1/sp&d-expanse=ALLIP+2/2.ii.clone-chronicles/part1-blood-market" "^clone-chronicles-sesh\d+\.txt$" "multi" &
python3 "$SCRIPT" "$BASE/spai.d-d-murderhobozos_5.1/sp&d-expanse=ALLIP+2/2.ii.clone-chronicles/part2-empire-game" "^clone-chronicles-p2-sesh\d+\.txt$" "multi" &
python3 "$SCRIPT" "$BASE/spai.d-d-murderhobozos_5.1/sp&d-expanse=ALLIP+2/3.rpg-canvas-sp-editsesh" "^RMMSP_sesh#\d+\.txt$" "multi" &
python3 "$SCRIPT" "$BASE/spai.d-d-murderhobozos_5.1/sp&d-expanse=ALLIP+2/x20.fuzz-sesh#1-3.5" "^fuzz-sesh-grok-2-note#(2|3)\.txt$" "multi" &

wait
echo "WAVE 1 DONE. Launching WAVE 2 (8 batches)..."

# Wave 2: 8 batches
python3 "$SCRIPT" "$BASE/HARD_VVAR" "^hard_vvar-sesh\d+\.txt$" "multi" &
python3 "$SCRIPT" "$BASE/TMOJIO_CAFE[]&/cafe-rp-sim" "^cafe-sesh.*\.txt$" "multi" &
python3 "$SCRIPT" "$BASE/SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/chara-notes" "^chara-study-sesh.*\.txt$" "single:fr-FR-VivienneMultilingualNeural" &
python3 "$SCRIPT" "$BASE/SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/sol-notes" "^sol-study-sesh.*\.txt$" "single:en-US-ChristopherNeural" &
python3 "$SCRIPT" "$BASE/SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/iqa-notes" "^iqa-study-sesh.*\.txt$" "single:en-US-AnaNeural" &
python3 "$SCRIPT" "$BASE/SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/horli-tutalage" "^horli-tutalage-sesh.*\.txt$" "horli-zh" &
# CHAR.A.NON books (chapter dirs with page_*.txt inside, no pauses)
python3 "$SCRIPT" "$BASE/SP.all-writeez28-b2/-1.MyChara+21]b1/CHAR.A.NON]c3]++/-1.CHAR.A.NON]1111111111111011/BOOK]1010%" "^Chapter_\d+$" "charanon" &
python3 "$SCRIPT" "$BASE/SP.all-writeez28-b2/-1.MyChara+21]b1/CHAR.A.NON]c3]++/-1.CHAR.A.NON]1111111111111100/book_template.scp]1100" "^chapter_\d+$" "charanon" &

wait
echo "WAVE 2 DONE. Launching WAVE 3 (remaining CHAR.A.NON books)..."

# Wave 3: remaining CHAR.A.NON books
python3 "$SCRIPT" "$BASE/SP.all-writeez28-b2/-1.MyChara+21]b1/CHAR.A.NON]c3]++/-1.CHAR.A.NON]1111111111111101/#.book_template.scp]101" "^chapter_\d+$" "charanon" &
python3 "$SCRIPT" "$BASE/SP.all-writeez28-b2/-1.MyChara+21]b1/CHAR.A.NON]c3]++/-1.CHAR.A.NON]1111111111111110/#.book_template.scp]110]fix" "^chapter_\d+$" "charanon" &
python3 "$SCRIPT" "$BASE/SP.all-writeez28-b2/-1.MyChara+21]b1/CHAR.A.NON]c3]++/-1.CHAR.A.NON]1111111111111111/#.book_template.scp]111" "^chapter_\d+$" "charanon" &

wait
echo "ALL WAVES DONE!"
