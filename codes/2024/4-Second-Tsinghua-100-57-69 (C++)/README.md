# SysY2022 Compiler

### How to start
- you can cmake this project using the following command:
```sh
mkdir build
cd build
cmake ..
make
```
- then you get a executable file named **compiler**

### How to build and display an ast tree / ir / riscv code
- assume you are under the **build** directory
- create a file **test.cpp** that you want to compile in the same directory
- use the following command
```sh
./compiler test.cpp --ast
./compiler test.cpp --ir
./compiler test.cpp -O2
```
- you will get the ast on your screen

### Compile riscv to executable

- still in **build** directory
```sh
riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 -O3 -S test.c -o test.s
riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 test.s -o test.o
```

### Run executable

- still in **build** directory
```sh
../spike-pk/spike --isa=RV32G ../spike-pk/pk test.o
# TODO: qemu
```

### Test

- test cases are in the **testcases** directory
- You can automatically test using the following command under the main directory, change test.sh if you want to test different items.
```sh
sh test.sh
```

Team Member: @youw21, @yangkai21, @choucy21
