# /usr/bin/sh

mkdir -p build

for dir in `ls -d src/*/`; do
    dname=`basename $dir`
    for f in `ls $dir/*.cpp`; do
        echo build/$dname-`basename $f .cpp`.o
        clang++ -std=c++17 -O2 -c -o "build/$dname-`basename $f .cpp`.o" $f
        if [ $? -ne 0 ]; then exit 1; fi
    done
done

clang++ -std=c++17 -O2 -c -o build/main.o src/main.cpp
if [ $? -ne 0 ]; then exit 1; fi

clang++ -lm -o build/compiler build/*.o
