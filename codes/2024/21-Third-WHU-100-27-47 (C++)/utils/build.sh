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

if [ "$1" != "--nverbose" ]; then
    clear
    echo Building...
    echo
fi

mkdir -p build
cd build
if [ -z "$2" ]; then
    mkdir -p release
    cd release
    cmake -DCMAKE_CXX_FLAGS=\"-O2\" ../..
else
    mkdir -p debug
    cd debug
    # echo "cmake -DCMAKE_CXX_FLAGS=\"-g -O0\" .."
    # # exit 0
    # cmake "-DCMAKE_CXX_FLAGS=\"-g -O0\"" ".."
    cmake -DCMAKE_CXX_FLAGS=\"-O0\" -DCMAKE_BUILD_TYPE=Debug ../..
    # exit 0
fi
make -j18

exitcode=$?

if [ "$1" != "--nverbose" ]; then
    if [ $exitcode -ne 0 ]; then
        echo
        echo Build failed.
    else
        echo
        echo Build successful.
    fi
fi

mkdir -p out

exit $exitcode
