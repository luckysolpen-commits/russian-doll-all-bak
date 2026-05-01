#!/bin/bash
CONVERTER="./+x/c-convert-tts-2-mp3-d8_04.+x"
LISTFILE="sessions_to_convert.txt"

while IFS= read -r input_file || [[ -n "$input_file" ]]; do
    [[ -z "$input_file" || "$input_file" =~ ^# ]] && continue
    [[ ! -f "$input_file" ]] && { echo "[SKIP] Not found: $input_file"; continue; }

    output_file="${input_file##*/}"
    output_file="${output_file%.txt}.mp3"

    echo "[$(date '+%H:%M:%S')] Converting: $input_file → $output_file"
    "$CONVERTER" "$input_file" "$output_file"

    sleep 3   # gentle pause
done < "$LISTFILE"

echo "Batch conversion finished."
