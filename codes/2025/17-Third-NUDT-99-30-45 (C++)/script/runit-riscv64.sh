#!/bin/bash

# runit-riscv64.sh - 用于在 RISC-V 虚拟机内部汇编、链接和测试 SysY 程序的脚本
# 此脚本应位于您的项目根目录 (例如 /home/ubuntu/debug)
# 假设当前运行环境已经是 RISC-V 64 位架构，可以直接执行编译后的程序。

# 定义相对于脚本位置的目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
TMP_DIR="${SCRIPT_DIR}/tmp"
LIB_DIR="${SCRIPT_DIR}/lib"
TESTDATA_DIR="${SCRIPT_DIR}/testdata"

# 定义编译器 (这里假设 gcc 在 VM 内部是可用的)
GCC_NATIVE="gcc" # VM 内部的 gcc

# --- 新增功能: 初始化变量 ---
GCC_TIMEOUT=10        # 默认 gcc 编译超时 (秒)
EXEC_TIMEOUT=5        # 默认运行时超时 (秒)
MAX_OUTPUT_LINES=50   # 对比失败时显示的最大行数
TOTAL_CASES=0
PASSED_CASES=0
FAILED_CASES_LIST=""  # 用于存储未通过的测例列表

# 显示帮助信息的函数
show_help() {
    echo "用法: $0 [选项]"
    echo "此脚本用于在 RISC-V 虚拟机内部，对之前生成的 .s 汇编文件进行汇编、链接和测试。"
    echo "测试会按文件名升序进行。"
    echo ""
    echo "选项:"
    echo "  -c, --clean              清理 'tmp' 目录下的所有生成文件。"
    echo "  -ct M                    设置 gcc 编译的超时时间为 M 秒 (默认: 10)。"
    echo "  -t N                     设置每个测试用例的运行时超时为 N 秒 (默认: 5)。"
    echo "  -ml N, --max-lines N     当输出对比失败时，最多显示 N 行内容 (默认: 50)。"
    echo "  -h, --help               显示此帮助信息并退出。"
}

# 显示文件内容并根据行数截断的函数
display_file_content() {
    local file_path="$1"
    local title="$2"
    local max_lines="$3"

    if [ ! -f "$file_path" ]; then
        return
    fi

    echo -e "$title"
    local line_count
    line_count=$(wc -l < "$file_path")
    
    if [ "$line_count" -gt "$max_lines" ]; then
        head -n "$max_lines" "$file_path"
        echo -e "\e[33m[... 输出已截断，共 ${line_count} 行 ...]\e[0m"
    else
        cat "$file_path"
    fi
}

# 清理临时文件的函数
clean_tmp() {
    echo "正在清理临时目录: ${TMP_DIR}"
    rm -rf "${TMP_DIR}"/*
    echo "清理完成。"
}

# 如果临时目录不存在，则创建它
mkdir -p "${TMP_DIR}"

# 解析命令行参数
while [[ "$#" -gt 0 ]]; do
    case "$1" in
        -c|--clean)
            clean_tmp
            exit 0
            ;;
        -t)
            if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then EXEC_TIMEOUT="$2"; shift; else echo "错误: -t 需要一个正整数参数。" >&2; exit 1; fi
            ;;
        -ct)
            if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then GCC_TIMEOUT="$2"; shift; else echo "错误: -ct 需要一个正整数参数。" >&2; exit 1; fi
            ;;
        -ml|--max-lines)
            if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then MAX_OUTPUT_LINES="$2"; shift; else echo "错误: --max-lines 需要一个正整数参数。" >&2; exit 1; fi
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo "未知选项: $1"
            show_help
            exit 1
            ;;
    esac
    shift # 移过参数名
done

echo "SysY VM 内部测试运行器启动..."
echo "GCC 编译超时设置为: ${GCC_TIMEOUT} 秒"
echo "运行时超时设置为: ${EXEC_TIMEOUT} 秒"
echo "失败输出最大行数: ${MAX_OUTPUT_LINES}"
echo "汇编文件目录: ${TMP_DIR}"
echo ""

# 查找 tmp 目录下的所有 .s 汇编文件并排序
s_files=$(find "${TMP_DIR}" -maxdepth 1 -name "*.s" | sort -V)
TOTAL_CASES=$(echo "$s_files" | wc -w)

# 使用 here-string (<<<) 避免子 shell 问题
while IFS= read -r s_file; do
    is_passed=1 # 1 表示通过, 0 表示失败

    base_name_from_s_file=$(basename "$s_file" .s)
    original_test_name_underscored=$(echo "$base_name_from_s_file" | sed 's/_sysyc_riscv64$//')

    category=$(echo "$original_test_name_underscored" | cut -d'_' -f1)
    test_file_base=$(echo "$original_test_name_underscored" | cut -d'_' -f2-)
    original_relative_path="${category}/${test_file_base}"

    executable_file="${TMP_DIR}/${base_name_from_s_file}"
    input_file="${TESTDATA_DIR}/${original_relative_path}.in"
    output_reference_file="${TESTDATA_DIR}/${original_relative_path}.out"
    output_actual_file="${TMP_DIR}/${base_name_from_s_file}.actual_out"

    echo "正在处理汇编文件: $(basename "$s_file")"
    echo "  对应的测试用例路径: ${original_relative_path}"

    # 步骤 1: 使用 VM 内部的 gcc 编译 .s 到可执行文件
    echo "  使用 gcc 汇编并链接 (超时 ${GCC_TIMEOUT}s)..."
    timeout -s KILL ${GCC_TIMEOUT} "${GCC_NATIVE}" "${s_file}" -o "${executable_file}" -L"${LIB_DIR}" -lsysy_riscv -static -g
    GCC_STATUS=$?
    if [ $GCC_STATUS -eq 124 ]; then
        echo -e "\e[31m错误: GCC 编译/链接 ${s_file} 超时\e[0m"
        is_passed=0
    elif [ $GCC_STATUS -ne 0 ]; then
        echo -e "\e[31m错误: GCC 汇编/链接 ${s_file} 失败，退出码: ${GCC_STATUS}\e[0m"
        is_passed=0
    fi

    # 步骤 2: 只有当编译成功时才执行
    if [ "$is_passed" -eq 1 ]; then
        echo "  生成的可执行文件: ${executable_file}"
        echo "  正在执行 (超时 ${EXEC_TIMEOUT}s)..."

        exec_cmd="\"${executable_file}\""
        if [ -f "${input_file}" ]; then
            exec_cmd+=" < \"${input_file}\""
        fi
        exec_cmd+=" > \"${output_actual_file}\""
        
        eval "timeout -s KILL ${EXEC_TIMEOUT} ${exec_cmd}"
        ACTUAL_RETURN_CODE=$?

        if [ "$ACTUAL_RETURN_CODE" -eq 124 ]; then
            echo -e "\e[31m  执行超时: ${original_relative_path}.sy 运行超过 ${EXEC_TIMEOUT} 秒\e[0m"
            is_passed=0
        else
            if [ -f "${output_reference_file}" ]; then
                LAST_LINE_TRIMMED=$(tail -n 1 "${output_reference_file}" | tr -d '[:space:]')
                
                if [[ "$LAST_LINE_TRIMMED" =~ ^[-+]?[0-9]+$ ]]; then
                    EXPECTED_RETURN_CODE="$LAST_LINE_TRIMMED"
                    EXPECTED_STDOUT_FILE="${TMP_DIR}/${base_name_from_s_file}.expected_stdout"
                    head -n -1 "${output_reference_file}" > "${EXPECTED_STDOUT_FILE}"

                    if [ "$ACTUAL_RETURN_CODE" -eq "$EXPECTED_RETURN_CODE" ]; then
                        echo -e "\e[32m  返回码测试成功: (${ACTUAL_RETURN_CODE}) 与期望值 (${EXPECTED_RETURN_CODE}) 匹配\e[0m"
                    else
                        echo -e "\e[31m  返回码测试失败: 期望: ${EXPECTED_RETURN_CODE}, 实际: ${ACTUAL_RETURN_CODE}\e[0m"
                        is_passed=0
                    fi

                    # --- 本次修改点: 使用 tr 删除所有空白字符后再比较 ---
                    if ! diff -q <(tr -d '[:space:]' < "${output_actual_file}") <(tr -d '[:space:]' < "${EXPECTED_STDOUT_FILE}") >/dev/null 2>&1; then
                        echo -e "\e[31m  标准输出测试失败\e[0m"
                        is_passed=0
                        display_file_content "${EXPECTED_STDOUT_FILE}" "    \e[36m---------- 期望输出 ----------\e[0m" "${MAX_OUTPUT_LINES}"
                        display_file_content "${output_actual_file}" "    \e[36m---------- 实际输出 ----------\e[0m" "${MAX_OUTPUT_LINES}"
                        echo -e "    \e[36m------------------------------\e[0m"
                    fi
                else
                    if [ $ACTUAL_RETURN_CODE -ne 0 ]; then
                        echo -e "\e[33m警告: 程序以非零状态 ${ACTUAL_RETURN_CODE} 退出 (纯输出比较模式)。\e[0m"
                    fi
                    
                    # --- 本次修改点: 使用 tr 删除所有空白字符后再比较 ---
                    if diff -q <(tr -d '[:space:]' < "${output_actual_file}") <(tr -d '[:space:]' < "${output_reference_file}") >/dev/null 2>&1; then
                        echo -e "\e[32m  成功: 输出与参考输出匹配\e[0m"
                    else
                        echo -e "\e[31m  失败: 输出不匹配\e[0m"
                        is_passed=0
                        display_file_content "${output_reference_file}" "    \e[36m---------- 期望输出 ----------\e[0m" "${MAX_OUTPUT_LINES}"
                        display_file_content "${output_actual_file}" "    \e[36m---------- 实际输出 ----------\e[0m" "${MAX_OUTPUT_LINES}"
                        echo -e "    \e[36m------------------------------\e[0m"
                    fi
                fi
            else
                echo "  无参考输出文件。程序返回码: ${ACTUAL_RETURN_CODE}"
            fi
        fi
    fi

    if [ "$is_passed" -eq 1 ]; then
        ((PASSED_CASES++))
    else
        FAILED_CASES_LIST+="${original_relative_path}.sy\n"
    fi
    echo ""
done <<< "$s_files"

echo "========================================"
echo "测试完成"
echo "测试通过率: [${PASSED_CASES}/${TOTAL_CASES}]"

if [ -n "$FAILED_CASES_LIST" ]; then
    echo ""
    echo -e "\e[31m未通过的测例:\e[0m"
    echo -e "${FAILED_CASES_LIST}"
fi

echo "========================================"

if [ "$PASSED_CASES" -eq "$TOTAL_CASES" ]; then
    exit 0
else
    exit 1
fi
