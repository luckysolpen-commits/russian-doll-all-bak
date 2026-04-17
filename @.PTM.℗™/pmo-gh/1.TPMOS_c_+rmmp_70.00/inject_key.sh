#!/bin/bash
# inject_key.sh - Inject a key into TPMOS history
# Usage: ./inject_key.sh <key_code>
BASE="/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+16/TPMOS-TRAINING/#.reference/TPMOS_c_+rmmp_66.01"
echo "[$(date '+%Y-%m-%d %H:%M:%S')] KEY_PRESSED: $1" >> "$BASE/pieces/keyboard/history.txt"
