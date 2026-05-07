#!/bin/bash

echo "=== SYS语言语法分析器批量测试 ==="
echo

passed=0
failed=0

for file in tests/h_functional/*.sy; do
    if [ -f "$file" ]; then
        echo -n "测试 $file ... "
        if ./compiler "$file" -S -o "${file%.sy}.s"; then
            echo "$file 通过"
            ((passed++))
        else
            echo "$file 失败"
            ((failed++))
        fi
    else
        echo "跳过 $file (文件不存在)"
        ((failed++))
    fi
done

# for file in tests/h_functional/*.c; do
#     if [ -f "$file" ]; then
#         riscv64-unknown-elf-gcc "$file" -S -o "${file%.c}_standard.s"; 
#     else
#         echo "跳过 $file (文件不存在)"
#     fi
# done

for file in tests/functional/*.sy; do
    if [ -f "$file" ]; then
        echo -n "测试 $file ... "
        if ./compiler "$file" -S -o "${file%.sy}.s"; then
            echo "$file 通过"
            ((passed++))
        else
            echo "$file 失败"
            ((failed++))
        fi
    else
        echo "跳过 $file (文件不存在)"
        ((failed++))
    fi
done

# for file in tests/functional/*.c; do
#     if [ -f "$file" ]; then
#         riscv64-unknown-elf-gcc "$file" -S -o "${file%.c}_standard.s";
#     else
#         echo "跳过 $file (文件不存在)"
#     fi
# done

echo
echo "=== 测试结果 ==="
echo "通过: $passed"
echo "失败: $failed"

echo
echo "=== 测试完成 ===" 