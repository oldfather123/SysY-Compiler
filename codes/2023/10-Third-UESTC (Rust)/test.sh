#/bin/bash

# Test script for the program

if [ $# -ne 1 ]; then
    echo "Usage: sh test.sh <filename in test/functional>"
    exit 1
fi

src_path=./test/functional/$1
output_path=./test/output/${1%%.sy}.s
exe_path=./test/executable/${1%%.sy}

# compile the program to get riscv assembly
./target/release/compiler $src_path > $output_path

# compile the riscv assembly to get riscv machine code
rm -rf $exe_path
riscv64-linux-gnu-as $output_path -o $exe_path.o
riscv64-linux-gnu-gcc -c ./test/functional_return/sylib.c -o ./test/functional_return/sylib.o
riscv64-linux-gnu-gcc $exe_path.o ./test/functional_return/sylib.o -o $exe_path -static

# run the riscv machine code in qemu
qemu-riscv64 $exe_path

# print the exit code
res=$?

echo "Exit code: $res"

# retrive correct res
res_file=./test/functional_return/${1%.sy}.res

correct=$(cut -d '' -f1 $res_file)

echo "Correct exit code: $correct"

if [ $res -eq $correct ]; then
    echo "Test passed"
else
    echo "Test failed"
fi
