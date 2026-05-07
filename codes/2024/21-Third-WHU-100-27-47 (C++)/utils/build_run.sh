#!/bin/bash

# Start from the current directory
dir="$(pwd)"

# Traverse up the directory tree until we find the CMakeLists.txt file
while [ ! -f "$dir/CMakeLists.txt" ]; do
    # If we reach the root directory without finding the file, exit with an error
    if [ "$dir" == "/" ]; then
        echo "Error: CMakeLists.txt not found in any ancestor directory."
        exit 1
    fi
    
    # Move up one directory
    dir="$(dirname "$dir")"
done

# Set the current working directory to the ancestor directory with CMakeLists.txt
cd "$dir"

# Build the project
bash utils/build.sh

if [ $? -eq 0 ]; then
    echo
    echo Launching...
    echo

    mkdir -p out

    if [ -z "$1" ]; then
        echo Usage: sudo bash build_run.sh "<test_case_name>"
        exit 1
    fi

    ./build/release/compiler -S -o out/out.s test_cases/$1.sy -O1 -o-ir out/out.ll
fi
