# /usr/bin/sh

pushd src

for dir in `ls -d */`; do
    for f in `ls $dir`; do
        sed 's/#include \"/#include \"..\//g' -i $dir$f
    done
done

rm -f magic_enum.hpp clipp.h dbg_macro.h
for f in `ls *.cpp *.h`; do
    sed 's/<magic_enum.hpp>/\"magic_enum.hpp\"/g' -i $f
    sed 's/<clipp.h>/\"clipp.h\"/g' -i $f
    sed 's/<dbg_macro.h>/\"dbg_macro.h\"/g' -i $f
done
ln -s ../include/* .

popd
