#!/bin/bash
# test_joy.sh - Simulate joystick event
echo "[$(date '+%Y-%m-%d %H:%M:%S')] KEY_PRESSED: 2103" >> pieces/keyboard/history.txt
echo "Simulated J-DOWN (2103) in history.txt"
sleep 1
tail -n 10 pieces/display/current_frame.txt
