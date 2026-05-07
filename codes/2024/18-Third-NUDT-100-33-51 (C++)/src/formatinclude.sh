#!/bin/bash

# 基础目录
BASE_DIR=$(pwd)

# 要处理的目录列表
declare -a directories=("backend" "frontend" "midend" "utils" "include")

# 子目录列表，用于include目录下
declare -a sub_directories=("backend" "frontend" "midend" "utils")

# 处理文件的函数
process_file() {
    local file_path=$1
    local dir_name=$2

    # 获取文件所在目录的名称（例如：backend、include/backend）
    local dir_name_only=${dir_name#"$BASE_DIR/"}

    # 检查是否是include目录下的子目录
    if [[ "$dir_name_only" == include/* ]]; then
        # 获取子目录名称（例如：backend）
        local sub_dir=${dir_name_only#include/}

        # 处理当前子目录下的文件包含，例如：#include "backend/xxx.h" 改为 #include "xxx.h"
        sed -i 's|#include "'"$sub_dir"'/\(.*\)\.h"|#include "\1.h"|g' "$file_path"

        # 遍历子目录列表，对于不同的子目录进行不同的替换
        for d in "${sub_directories[@]}"; do
            if [ "$d" != "$sub_dir" ]; then
                # 处理其他子目录下的文件包含，例如：#include "frontend/xxx.h" 改为 #include "../frontend/xxx.h"
                sed -i 's|#include "'"$d"'/\(.*\)\.h"|#include "../'"$d"'/\1.h"|g' "$file_path"
            fi
        done
    else
        # 处理backend、midend、utils、frontend目录下的文件包含，例如：#include "xxx.h" 改为 #include "../include/xxx.h"
        sed -i 's|#include "\(.*\)\.h"|#include "../include/\1.h"|g' "$file_path"
    fi
}

# 遍历每个目录
for dir in "${directories[@]}"; do
    # 构建目录的完整路径
    full_dir_path="$BASE_DIR/$dir"

    # 检查是否是include目录下的子目录
    if [ "$dir" == "include" ]; then
        # 对于include目录，需要遍历其子目录
        for sub_dir in "${sub_directories[@]}"; do
            process_dir="$full_dir_path/$sub_dir"
            if [ -d "$process_dir" ]; then
                # 找到目录下的所有.cpp和.h文件
                find "$process_dir" -type f \( -name "*.cpp" -o -name "*.h" \) -print0 | while IFS= read -r -d '' file; do
                    process_file "$file" "$file" # 第二个参数同时传递文件的完整路径和目录名
                done
            fi
        done
    else
        # 对于其他目录，直接找到所有.cpp和.h文件
        find "$full_dir_path" -type f \( -name "*.cpp" -o -name "*.h" \) -print0 | while IFS= read -r -d '' file; do
            process_file "$file" "$full_dir_path"
        done
    fi
done

echo "All files processed."