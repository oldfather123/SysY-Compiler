#!/bin/bash

# runit-riscv64-single.sh - 用于在 RISC-V 虚拟机内部测试单个或少量 .s 文件的脚本
# 模仿 runit-riscv64.sh 的功能，但以具体文件路径作为输入。

# --- 配置区 ---
# 假设此脚本位于项目根目录 (例如 /home/ubuntu/debug)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
LIB_DIR="${SCRIPT_DIR}/lib"
TMP_DIR="${SCRIPT_DIR}/tmp" # 临时可执行文件将存放在这里
TESTDATA_DIR="${SCRIPT_DIR}/testdata" # 用于查找 .in/.out 文件

# 定义编译器
GCC_NATIVE="gcc" # VM 内部的原生 gcc

# --- 初始化变量 ---
CLEAN_MODE=false
GCC_TIMEOUT=10        # gcc 编译超时 (秒)
EXEC_TIMEOUT=5        # 程序自动化执行超时 (秒)
MAX_OUTPUT_LINES=50   # 对比失败时显示的最大行数
S_FILES=()            # 存储用户提供的 .s 文件列表
PASSED_CASES=0
FAILED_CASES_LIST=""

# --- 函数定义 ---
show_help() {
    echo "用法: $0 [文件1.s] [文件2.s] ... [选项]"
    echo "在 VM 内部编译并测试指定的 .s 文件。"
    echo ""
    echo "如果找到对应的 .in/.out 文件，则进行自动化测试。否则，进入交互模式。"
    echo ""
    echo "选项:"
    echo "  -c, --clean              清理 tmp 临时目录下的所有文件。"
    echo "  -ct N                    设置 gcc 编译超时为 N 秒 (默认: 10)。"
    echo "  -t N                     设置程序自动化执行超时为 N 秒 (默认: 5)。"
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

# --- 新增功能: 清理临时文件的函数 ---
clean_tmp() {
    echo "正在清理临时目录: ${TMP_DIR}"
    if [ -d "${TMP_DIR}" ]; then
        rm -rf "${TMP_DIR}"/* 2>/dev/null
        echo "清理完成。"
    else
        echo "临时目录 ${TMP_DIR} 不存在，无需清理。"
    fi
}

# --- 参数解析 ---
# 从参数中分离出 .s 文件和选项
for arg in "$@"; do
  case "$arg" in
    -c|--clean)
      CLEAN_MODE=true
      ;;
    -ct|-t|-ml|--max-lines)
      # 选项和其值将在下一个循环中处理
      ;;
    -h|--help)
      show_help
      exit 0
      ;;
    -*)
      # 检查是否是带值的选项
      if ! [[ ${args_processed+x} ]]; then
          args_processed=true # 标记已处理过参数
          while [[ "$#" -gt 0 ]]; do
              case "$1" in
                  -c|--clean) ;; # 已在外部处理
                  -ct) if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then GCC_TIMEOUT="$2"; shift; else echo "错误: -ct 需要一个正整数参数。" >&2; exit 1; fi ;;
                  -t)  if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then EXEC_TIMEOUT="$2"; shift; else echo "错误: -t 需要一个正整数参数。" >&2; exit 1; fi ;;
                  -ml|--max-lines) if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then MAX_OUTPUT_LINES="$2"; shift; else echo "错误: --max-lines 需要一个正整数参数。" >&2; exit 1; fi ;;
                  *.s) S_FILES+=("$1") ;;
                  *) if ! [[ "$1" =~ ^[0-9]+$ ]]; then echo "未知选项或无效文件: $1"; show_help; exit 1; fi ;;
              esac
              shift
          done
      fi
      ;;
    *.s)
      if [[ -f "$arg" ]]; then
        S_FILES+=("$arg")
      else
        echo "警告: 文件不存在，已忽略: $arg"
      fi
      ;;
  esac
done

# --- 主逻辑开始 ---
if ${CLEAN_MODE}; then
    clean_tmp
    # 如果只提供了 -c 选项，则退出
    if [ ${#S_FILES[@]} -eq 0 ]; then
        exit 0
    fi
fi

if [ ${#S_FILES[@]} -eq 0 ]; then
    echo "错误: 未提供任何 .s 文件作为输入。"
    show_help
    exit 1
fi

mkdir -p "${TMP_DIR}"
TOTAL_CASES=${#S_FILES[@]}

echo "SysY VM 内单例测试运行器启动..."
echo "超时设置: gcc=${GCC_TIMEOUT}s, 运行=${EXEC_TIMEOUT}s"
echo "失败输出最大行数: ${MAX_OUTPUT_LINES}"
echo ""

for s_file in "${S_FILES[@]}"; do
    is_passed=1
    
    # 从 .s 文件名反向推导原始测试用例路径
    base_name_from_s_file=$(basename "$s_file" .s)
    original_test_name_underscored=$(echo "$base_name_from_s_file" | sed 's/_sysyc_riscv64$//')
    category=$(echo "$original_test_name_underscored" | cut -d'_' -f1)
    test_file_base=$(echo "$original_test_name_underscored" | cut -d'_' -f2-)
    original_relative_path="${category}/${test_file_base}"

    executable_file="${TMP_DIR}/${base_name_from_s_file}"
    input_file="${TESTDATA_DIR}/${original_relative_path}.in"
    output_reference_file="${TESTDATA_DIR}/${original_relative_path}.out"
    output_actual_file="${TMP_DIR}/${base_name_from_s_file}.actual_out"

    echo "======================================================================"
    echo "正在处理: ${s_file}"
    echo "  (关联测试用例: ${original_relative_path}.sy)"

    # 步骤 1: GCC 编译
    echo "  使用 gcc 编译 (超时 ${GCC_TIMEOUT}s)..."
    timeout -s KILL ${GCC_TIMEOUT} "${GCC_NATIVE}" "${s_file}" -o "${executable_file}" -L"${LIB_DIR}" -lsysy_riscv -static -g
    if [ $? -ne 0 ]; then
        echo -e "\e[31m错误: GCC 编译失败或超时。\e[0m"
        is_passed=0
    fi

    # 步骤 2: 执行与测试
    if [ "$is_passed" -eq 1 ]; then
        # 检查是自动化测试还是交互模式
        if [ -f "${input_file}" ] || [ -f "${output_reference_file}" ]; then
            # --- 自动化测试模式 ---
            echo "  检测到 .in/.out 文件，进入自动化测试模式..."
            echo "  正在执行 (超时 ${EXEC_TIMEOUT}s)..."
            
            exec_cmd="\"${executable_file}\""
            [ -f "${input_file}" ] && exec_cmd+=" < \"${input_file}\""
            exec_cmd+=" > \"${output_actual_file}\""
            
            eval "timeout -s KILL ${EXEC_TIMEOUT} ${exec_cmd}"
            ACTUAL_RETURN_CODE=$?

            if [ "$ACTUAL_RETURN_CODE" -eq 124 ]; then
                echo -e "\e[31m  执行超时。\e[0m"
                is_passed=0
            else
                if [ -f "${output_reference_file}" ]; then
                    LAST_LINE_TRIMMED=$(tail -n 1 "${output_reference_file}" | tr -d '[:space:]')
                    if [[ "$LAST_LINE_TRIMMED" =~ ^[-+]?[0-9]+$ ]]; then
                        EXPECTED_RETURN_CODE="$LAST_LINE_TRIMMED"
                        EXPECTED_STDOUT_FILE="${TMP_DIR}/${base_name_from_s_file}.expected_stdout"
                        head -n -1 "${output_reference_file}" > "${EXPECTED_STDOUT_FILE}"
                        if [ "$ACTUAL_RETURN_CODE" -ne "$EXPECTED_RETURN_CODE" ]; then echo -e "\e[31m  返回码测试失败: 期望 ${EXPECTED_RETURN_CODE}, 实际 ${ACTUAL_RETURN_CODE}\e[0m"; is_passed=0; fi
                        
                        # --- 本次修改点: 使用 tr 删除所有空白字符后再比较 ---
                        if ! diff -q <(tr -d '[:space:]' < "${output_actual_file}") <(tr -d '[:space:]' < "${EXPECTED_STDOUT_FILE}") >/dev/null 2>&1; then
                            echo -e "\e[31m  标准输出测试失败。\e[0m"; is_passed=0
                            display_file_content "${EXPECTED_STDOUT_FILE}" "    \e[36m--- 期望输出 ---\e[0m" "${MAX_OUTPUT_LINES}"
                            display_file_content "${output_actual_file}" "    \e[36m--- 实际输出 ---\e[0m" "${MAX_OUTPUT_LINES}"
                            echo -e "    \e[36m----------------\e[0m"
                        fi
                    else
                        # --- 本次修改点: 使用 tr 删除所有空白字符后再比较 ---
                        if diff -q <(tr -d '[:space:]' < "${output_actual_file}") <(tr -d '[:space:]' < "${output_reference_file}") >/dev/null 2>&1; then
                            echo -e "\e[32m  标准输出测试成功。\e[0m"
                        else
                            echo -e "\e[31m  标准输出测试失败。\e[0m"; is_passed=0
                            display_file_content "${output_reference_file}" "    \e[36m--- 期望输出 ---\e[0m" "${MAX_OUTPUT_LINES}"
                            display_file_content "${output_actual_file}" "    \e[36m--- 实际输出 ---\e[0m" "${MAX_OUTPUT_LINES}"
                            echo -e "    \e[36m----------------\e[0m"
                        fi
                    fi
                else
                    echo "  无参考输出文件。程序返回码: ${ACTUAL_RETURN_CODE}"
                fi
            fi
        else
            # --- 交互模式 ---
            echo -e "\e[33m"
            echo "  **********************************************************"
            echo "  ** 未找到 .in 或 .out 文件，进入交互模式。             **"
            echo "  ** 程序即将运行，你可以直接在终端中输入。             **"
            echo "  ** 按下 Ctrl+D (EOF) 或以其他方式结束程序以继续。 **"
            echo "  **********************************************************"
            echo -e "\e[0m"
            "${executable_file}"
            INTERACTIVE_RET_CODE=$?
            echo -e "\e[33m\n  交互模式执行完毕，程序返回码: ${INTERACTIVE_RET_CODE}\e[0m"
            echo "  注意: 交互模式的结果未经验证。"
        fi
    fi

    if [ "$is_passed" -eq 1 ]; then
        echo -e "\e[32m状态: 通过\e[0m"
        ((PASSED_CASES++))
    else
        echo -e "\e[31m状态: 失败\e[0m"
        FAILED_CASES_LIST+="${original_relative_path}.sy\n"
    fi
done

# --- 打印最终总结 ---
echo "======================================================================"
echo "所有测试完成"
echo "测试通过率: [${PASSED_CASES}/${TOTAL_CASES}]"

if [ -n "$FAILED_CASES_LIST" ]; then
    echo ""
    echo -e "\e[31m未通过的测例:\e[0m"
    echo -e "${FAILED_CASES_LIST}"
fi

echo "======================================================================"

if [ "$PASSED_CASES" -eq "$TOTAL_CASES" ]; then
    exit 0
else
    exit 1
fi
