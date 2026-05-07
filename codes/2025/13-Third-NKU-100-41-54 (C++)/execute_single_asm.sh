#!/bin/bash

# 固定路径定义
SOURCE_SY_FILE="./test_output/example/temp.sy"       # 源文件路径
OUTPUT_ASM_FILE="./test_output/example/temp.out.S"   # 生成的汇编文件路径
OUTPUT_OBJ_FILE="./test_output/example/tmp.o"        # 目标文件路径
LIB_PATH="lib/libsysy_riscv.a"                      # 静态库路径
INPUT_FILE="./testcase/functional_test/Advanced/lisp2.in"  # 默认输入样例路径

# 询问是否使用输入重定向
read -p "是否需要从文件输入测试样例? (y/n, 默认n): " use_input
use_input=${use_input:-n}  # 默认值为n

# 1. 生成汇编文件
./compiler -S -o "$OUTPUT_ASM_FILE" "$SOURCE_SY_FILE" -O1
if [ $? -ne 0 ]; then
    echo "Error: Failed to generate assembly."
    exit 1
fi

# 2. 汇编 -> 目标文件
riscv64-unknown-linux-gnu-gcc -c "$OUTPUT_ASM_FILE" -o "$OUTPUT_OBJ_FILE"
if [ $? -ne 0 ]; then
    echo "Error: Failed to assemble."
    exit 1
fi

# 3. 链接 -> 可执行文件
riscv64-unknown-linux-gnu-gcc -mcmodel=medany -static "$OUTPUT_OBJ_FILE" "$LIB_PATH"
if [ $? -ne 0 ]; then
    echo "Error: Failed to link."
    exit 1
fi

# 4. 用 QEMU 运行
echo -e "\nRunning a.out with QEMU:"
if [ "$use_input" = "y" ] || [ "$use_input" = "Y" ]; then
    echo "使用输入文件: $INPUT_FILE"
    qemu-riscv64 ./a.out < "$INPUT_FILE"
else
    echo "使用标准输入"
    qemu-riscv64 ./a.out
fi

# 5. 输出返回值
echo -e "\nExit code: $?"