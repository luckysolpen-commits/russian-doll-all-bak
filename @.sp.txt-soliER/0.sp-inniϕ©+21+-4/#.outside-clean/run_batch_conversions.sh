
#!/bin/bash

echo "Starting batch conversion process for pending books..."
echo "-----------------------------------"

# --- Kill any existing conversion processes ---
echo "Checking for and terminating any existing fix_book_pauses.py processes..."
# Use pkill -f to match against the full command line. This is safer for ensuring we kill the right processes.
# The pattern "python3.*fix_book_pauses.py" should target the specific script.
pkill -f "python3.*fix_book_pauses.py"
echo "Existing processes terminated (if any)."
echo "-----------------------------------"
# --- End of process killing ---

CONVERSION_SCRIPT="./fix_book_pauses.py"
BOOK_BASE_DIR="./SP.all-writeez28-b2"

# Array of books to process, mapped to their directories and display names
# Format: "Progress_Name:(Directory_Path, Display_Name, Optional_Voice)"
# Only includes books not marked as COMPLETED in @progress-a17.txt
# The default voice will be used if no voice is specified for a book.

BOOKS_TO_PROCESS=(
    "6. Hellride:SP.all-writeez28-b2/6.hellride-bk0]BIG]b1,Hellride,"
    "7. Avanta C:SP.all-writeez28-b2/7.AVANTA.C]mid]c6,Avanta C,"
    "8. Digi Sol Suicider:SP.all-writeez28-b2/8.Digi_Sol.Suicider]BIG]b1,Digi Sol Suicider,"
    "9. Metal Sun:SP.all-writeez28-b2/9. METAL SUN[METAL]wcsm+wtf]a1,Metal Sun,"
    "10. XO Files:SP.all-writeez28-b2/10. xo.files]wc.smol]a0,XO Files,"
    "11. Footrace FU:SP.all-writeez28-b2/11. FOOTRACE.FU]HUGE,Footrace FU,"
    "12. Centroid:SP.all-writeez28-b2/12. centroid.u.ϕ]2xvsp]big]b1,Centroid,"
    "13. Chatoe BnB:SP.all-writeez28-b2/333.chatoe.bnb.13]a0]FULL,Chatoe BnB,EmmaMultilingualNeural"
    "100. Angler Empire (NGL MPYRE):SP.all-writeez28-b2/100.%.NGL.MPYRE]bk0,Angler Empire (NGL MPYRE),en-GB-RyanNeural"
    "111. Tear-it:SP.all-writeez28-b2/111.tear-it]accidental.witch]y3,Tear-it,ga-IE-OrlaNeural"
)

for book_info in "${BOOKS_TO_PROCESS[@]}"; do
    # Parse the book info string
    IFS=: read -r progress_name detail <<< "$book_info"
    
    # Extract book_dir, book_name, and voice_override using string manipulation for robustness
    book_dir="${detail%%,*}" # Part before the first comma
    remainder="${detail#*,}" # Part after the first comma

    # Handle cases with and without voice_override
    if [[ "$remainder" == *","* ]]; then
        book_name="${remainder%%,*}" # Part before the last comma in remainder
        voice_override="${remainder##*,}" # Part after the last comma in remainder
    else
        book_name="$remainder" # Remainder is just the book name if no voice override
        voice_override=""
    fi
    
    # Remove any trailing parenthesis from voice_override if it exists (safeguard)
    voice_override=${voice_override%')'}

    echo "Processing book: $book_name"
    echo "  Directory: "$book_dir""
    if [ -n "$voice_override" ]; then
        echo "  Voice: $voice_override"
    fi

    # Construct the command with the optional voice argument and properly quoted directory/name
    CMD="python3 "$CONVERSION_SCRIPT" "$book_dir" "$book_name""
    if [ -n "$voice_override" ]; then
        CMD="$CMD "$voice_override""
    fi

    # Execute the command
    eval "$CMD"

    # Check the exit code of the python script
    if [ $? -eq 0 ]; then
        echo "Finished: $book_name - SUCCESS"
    else
        echo "Finished: $book_name - FAILED"
    fi
    echo "-----------------------------------"
done

echo "Batch conversion process completed."
