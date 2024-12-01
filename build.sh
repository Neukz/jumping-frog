#!/bin/bash

# Utility script for compiling the program to uniquely identified executables

OUTPUT_FILE="./builds/main_$(date +%s%N)"
gcc *.c -o "$OUTPUT_FILE" -lncurses

if [ $? -eq 0 ]; then
    echo "Output file: $OUTPUT_FILE"
    ./"$OUTPUT_FILE"
fi