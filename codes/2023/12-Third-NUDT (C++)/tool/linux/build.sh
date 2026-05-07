#!/usr/bin/bash
./tool/linux/antlr4.sh
if [ -d "./build" ]; then
    rm -rf ./build
fi
mkdir build && cd ./build || exit
cmake ..
make -j16
make format
make format_check
cd .. || exit
