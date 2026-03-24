#!/bin/bash
# Quick test: Select man-ops project
cd /home/no/Desktop/Piecemark-IT/\[CALENDARS\]02.01/zesy/mar_11+0/mar_11+2q/\^.PMO_c_+rmmp_32.23

# Simulate selecting Project Manager (1), then man-ops (1)
echo "LOAD_PROJECT:man-ops" >> pieces/keyboard/history.txt

sleep 2
echo "=== After loading man-ops ==="
cat pieces/apps/player_app/manager/state.txt
echo ""
echo "=== Current layout ==="
cat pieces/display/current_layout.txt
