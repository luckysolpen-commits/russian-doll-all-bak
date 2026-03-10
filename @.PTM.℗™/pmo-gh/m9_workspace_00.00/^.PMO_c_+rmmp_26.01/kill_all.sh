#!/bin/bash

# kill_all.sh - TPM Component Cleanup
echo "Killing all TPM components..."
pkill -9 -f '\.\+x'
pkill -9 -f "orchestrator"
pkill -9 -f "chtpm_parser"
pkill -9 -f "renderer"
echo "Cleanup complete."
