#!/bin/bash

# 配置参数
INPUT_LL_FILE="./test_output/example/temp.out.ll"
DOT_OUTPUT_DIR="graphviz/dot"
PNG_OUTPUT_DIR="graphviz/png"

# 清空并创建输出目录
rm -rf "$DOT_OUTPUT_DIR" "$PNG_OUTPUT_DIR"
mkdir -p "$DOT_OUTPUT_DIR" "$PNG_OUTPUT_DIR"

# 检查输入文件是否存在
if [ ! -f "$INPUT_LL_FILE" ]; then
    echo "错误：输入文件 $INPUT_LL_FILE 不存在"
    exit 1
fi

# 获取所有函数名列表
FUNCTIONS=$(grep '^define' "$INPUT_LL_FILE" | awk -F'[@(]' '{print $2}')

# 为每个函数生成CFG
for FUNC in $FUNCTIONS; do
    echo "正在处理函数: $FUNC"
    
    # 生成dot文件
    DOT_FILE="$DOT_OUTPUT_DIR/$FUNC.dot"
	opt -opaque-pointers=1 -enable-new-pm=0 -dot-cfg -cfg-func-name="$FUNC" "$INPUT_LL_FILE" -f >/dev/null 2>&1

    # 移动生成的dot文件（LLVM会生成隐藏文件）
    mv ."$FUNC".dot "$DOT_FILE" 2>/dev/null
    
    # 转换为PNG
    if [ -f "$DOT_FILE" ]; then
        PNG_FILE="$PNG_OUTPUT_DIR/$FUNC.png"
        dot -Tpng "$DOT_FILE" -o "$PNG_FILE"
        echo "已生成: $PNG_FILE"
    else
        echo "警告：无法为函数 $FUNC 生成dot文件"
    fi
done

echo "处理完成！"
echo "Dot文件保存在: $DOT_OUTPUT_DIR"
echo "PNG图像保存在: $PNG_OUTPUT_DIR"