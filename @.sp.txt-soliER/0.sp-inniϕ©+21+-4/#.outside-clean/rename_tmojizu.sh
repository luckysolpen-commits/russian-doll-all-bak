#!/bin/bash
# Rename "Mother Tomokazu" to "Mother Tmojizu" in all session files

cd "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+12/SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/ffu++chinese-learning/1.ffu-ez_learnd&d+chem"

# Count occurrences
echo "Counting occurrences of 'Mother Tomokazu'..."
count=$(grep -r "Mother Tomokazu" *.txt | wc -l)
echo "Found $count occurrences"

# Replace in all .txt files
echo "Replacing 'Mother Tomokazu' with 'Mother Tmojizu'..."
sed -i 's/Mother Tomokazu/Mother Tmojizu/g' *.txt

# Verify
echo "Verifying replacements..."
remaining=$(grep -r "Mother Tomokazu" *.txt | wc -l)
new_count=$(grep -r "Mother Tmojizu" *.txt | wc -l)
echo "Remaining 'Mother Tomokazu': $remaining"
echo "New 'Mother Tmojizu' occurrences: $new_count"
echo "Done!"
