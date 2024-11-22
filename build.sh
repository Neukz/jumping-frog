#!/bin/bash

# Utility script for compiling the program to uniquely identified executables

SOURCE_FILE_NAME="main"
OUTPUT_FILE="./builds/${SOURCE_FILE_NAME}_$(date +%s%N)"
gcc "${SOURCE_FILE_NAME}.c" -o "$OUTPUT_FILE" -lncurses

if [ $? -eq 0 ]; then
    echo "Output file: $OUTPUT_FILE"
    ./"$OUTPUT_FILE"
fi