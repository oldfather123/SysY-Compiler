#!/bin/bash

# runit-single.sh - 用于编译和测试单个或少量 SysY 程序的脚本
# 模仿 runit.sh 的功能，但以具体文件路径作为输入。
# 此脚本应该位于 mysysy/script/

export ASAN_OPTIONS=detect_leaks=0

# --- 配置区 ---
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
BUILD_BIN_DIR="${SCRIPT_DIR}/../build/bin"
LIB_DIR="${SCRIPT_DIR}/../lib"
TMP_DIR="${SCRIPT_DIR}/tmp"

# 定义编译器和模拟器
SYSYC="${BUILD_BIN_DIR}/sysyc"
LLC_CMD="llc-19" # 新增
GCC_RISCV64="riscv64-linux-gnu-gcc"
QEMU_RISCV64="qemu-riscv64"

# --- 初始化变量 ---
EXECUTE_MODE=false
IR_EXECUTE_MODE=false
CLEAN_MODE=false
OPTIMIZE_FLAG=""
SYSYC_TIMEOUT=30
LLC_TIMEOUT=10
GCC_TIMEOUT=10
EXEC_TIMEOUT=30
MAX_OUTPUT_LINES=20
MAX_OUTPUT_CHARS=1000
SY_FILES=()
PASSED_CASES=0
FAILED_CASES_LIST=""
INTERRUPTED=false

# =================================================================
# --- 函数定义 ---
# =================================================================
show_help() {
    echo "用法: $0 [文件1.sy] [文件2.sy] ... [选项]"
    echo "编译并测试指定的 .sy 文件。必须提供 -e 或 -eir 之一。"
    echo ""
    echo "选项:"
    echo "  -e                       通过汇编运行测试 (sysyc -> gcc -> qemu)。"
    echo "  -eir                     通过IR运行测试 (sysyc -> llc -> gcc -> qemu)。"
    echo "  -c, --clean              清理 tmp 临时目录下的所有文件。"
    echo "  -O1                      启用 sysyc 的 -O1 优化。"
    echo "  -sct N                   设置 sysyc 编译超时为 N 秒 (默认: 30)。"
    echo "  -lct N                   设置 llc-19 编译超时为 N 秒 (默认: 10)。"
    echo "  -gct N                   设置 gcc 交叉编译超时为 N 秒 (默认: 10)。"
    echo "  -et N                    设置 qemu 自动化执行超时为 N 秒 (默认: 30)。"
    echo "  -ml N, --max-lines N     当输出对比失败时，最多显示 N 行内容 (默认: 20)。"
    echo "  -mc N, --max-chars N     当输出对比失败时，最多显示 N 个字符 (默认: 1000)。"
    echo "  -h, --help               显示此帮助信息并退出。"
    echo ""
    echo "可在任何时候按 Ctrl+C 来中断测试并显示当前已完成的测例总结。"
}

# 显示文件内容并根据行数和字符数截断的函数
display_file_content() {
    local file_path="$1"
    local title="$2"
    local max_lines="$3"
    local max_chars="$4" # 新增参数
    if [ ! -f "$file_path" ]; then return; fi
    echo -e "$title"
    local line_count
    local char_count
    line_count=$(wc -l < "$file_path")
    char_count=$(wc -c < "$file_path")

    if [ "$line_count" -gt "$max_lines" ]; then
        head -n "$max_lines" "$file_path"
        echo -e "\e[33m[... 输出因行数过多 (共 ${line_count} 行) 而截断 ...]\e[0m"
    elif [ "$char_count" -gt "$max_chars" ]; then
        head -c "$max_chars" "$file_path"
        echo -e "\n\e[33m[... 输出因字符数过多 (共 ${char_count} 字符) 而截断 ...]\e[0m"
    else
        cat "$file_path"
    fi
}

# --- 新增：总结报告函数 ---
print_summary() {
    local total_cases=${#SY_FILES[@]}
    echo ""
    echo "======================================================================"
    if [ "$INTERRUPTED" = true ]; then
        echo -e "\e[33m测试被中断。正在汇总已完成的结果...\e[0m"
    else
        echo "所有测试完成"
    fi

    local failed_count
    if [ -n "$FAILED_CASES_LIST" ]; then
        failed_count=$(echo -e -n "${FAILED_CASES_LIST}" | wc -l)
    else
        failed_count=0
    fi
    local executed_count=$((PASSED_CASES + failed_count))

    echo "测试结果: [通过: ${PASSED_CASES}, 失败: ${failed_count}, 已执行: ${executed_count}/${total_cases}]"

    if [ -n "$FAILED_CASES_LIST" ]; then
        echo ""
        echo -e "\e[31m未通过的测例:\e[0m"
        printf "%b" "${FAILED_CASES_LIST}"
    fi
    echo "======================================================================"

    if [ "$failed_count" -gt 0 ]; then
        exit 1
    else
        exit 0
    fi
}

# --- 新增：SIGINT 信号处理函数 ---
handle_sigint() {
    INTERRUPTED=true
    print_summary
}

# =================================================================
# --- 主逻辑开始 ---
# =================================================================

# --- 新增：设置 trap 来捕获 SIGINT ---
trap handle_sigint SIGINT

# --- 参数解析 ---
while [[ "$#" -gt 0 ]]; do
    case "$1" in
        -e|--executable) EXECUTE_MODE=true; shift ;;
        -eir) IR_EXECUTE_MODE=true; shift ;; # 新增
        -c|--clean) CLEAN_MODE=true; shift ;;
        -O1) OPTIMIZE_FLAG="-O1"; shift ;;
        -lct) if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then LLC_TIMEOUT="$2"; shift 2; else echo "错误: -lct 需要一个正整数参数。" >&2; exit 1; fi ;; # 新增
        -sct) if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then SYSYC_TIMEOUT="$2"; shift 2; else echo "错误: -sct 需要一个正整数参数。" >&2; exit 1; fi ;;
        -gct) if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then GCC_TIMEOUT="$2"; shift 2; else echo "错误: -gct 需要一个正整数参数。" >&2; exit 1; fi ;;
        -et) if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then EXEC_TIMEOUT="$2"; shift 2; else echo "错误: -et 需要一个正整数参数。" >&2; exit 1; fi ;;
        -ml|--max-lines) if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then MAX_OUTPUT_LINES="$2"; shift 2; else echo "错误: --max-lines 需要一个正整数参数。" >&2; exit 1; fi ;;
        -mc|--max-chars) if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then MAX_OUTPUT_CHARS="$2"; shift 2; else echo "错误: --max-chars 需要一个正整数参数。" >&2; exit 1; fi ;;
        -h|--help) show_help; exit 0 ;;
        -*) echo "未知选项: $1"; show_help; exit 1 ;;
        *)
            if [[ -f "$1" && "$1" == *.sy ]]; then
                SY_FILES+=("$1")
            else
                echo "警告: 无效文件或不是 .sy 文件，已忽略: $1"
            fi
            shift
            ;;
    esac
done

if ${CLEAN_MODE}; then
  echo "检测到 -c/--clean 选项，正在清空 ${TMP_DIR}..."
  if [ -d "${TMP_DIR}" ]; then
    rm -rf "${TMP_DIR}"/* 2>/dev/null
    echo "清理完成。"
  else
    echo "临时目录 ${TMP_DIR} 不存在，无需清理。"
  fi
  if [ ${#SY_FILES[@]} -eq 0 ] && ! ${EXECUTE_MODE} && ! ${IR_EXECUTE_MODE}; then
    exit 0
  fi
fi

if ! ${EXECUTE_MODE} && ! ${IR_EXECUTE_MODE}; then
    echo "错误: 请提供 -e 或 -eir 选项来运行测试。"
    show_help
    exit 1
fi

if ${EXECUTE_MODE} && ${IR_EXECUTE_MODE}; then
    echo -e "\e[31m错误: -e 和 -eir 选项不能同时使用。\e[0m" >&2
    exit 1
fi

if [ ${#SY_FILES[@]} -eq 0 ]; then
    echo "错误: 未提供任何 .sy 文件作为输入。"
    show_help
    exit 1
fi

mkdir -p "${TMP_DIR}"
TOTAL_CASES=${#SY_FILES[@]}

echo "SysY 单例测试运行器启动..."
if [ -n "$OPTIMIZE_FLAG" ]; then echo "优化等级: ${OPTIMIZE_FLAG}"; fi
echo "超时设置: sysyc=${SYSYC_TIMEOUT}s, llc=${LLC_TIMEOUT}s, gcc=${GCC_TIMEOUT}s, qemu=${EXEC_TIMEOUT}s"
echo "失败输出最大行数: ${MAX_OUTPUT_LINES}"
echo "失败输出最大字符数: ${MAX_OUTPUT_CHARS}"
echo ""

for sy_file in "${SY_FILES[@]}"; do
    is_passed=1
    compilation_ok=1
    base_name=$(basename "${sy_file}" .sy)
    source_dir=$(dirname "${sy_file}")

    ir_file="${TMP_DIR}/${base_name}.ll"
    assembly_file="${TMP_DIR}/${base_name}.s"
    executable_file="${TMP_DIR}/${base_name}"
    input_file="${source_dir}/${base_name}.in"
    output_reference_file="${source_dir}/${base_name}.out"
    output_actual_file="${TMP_DIR}/${base_name}.actual_out"

    echo "======================================================================"
    echo "正在处理: ${sy_file}"

    # --- 编译阶段 ---
    if ${IR_EXECUTE_MODE}; then
        # 路径1: sysyc -> llc -> gcc
        echo "  [1/3] 使用 sysyc 编译为 IR (超时 ${SYSYC_TIMEOUT}s)..."
        timeout -s KILL ${SYSYC_TIMEOUT} "${SYSYC}" -s ir "${sy_file}" ${OPTIMIZE_FLAG} -o "${ir_file}"
        if [ $? -ne 0 ]; then echo -e "\e[31m错误: SysY (IR) 编译失败或超时。\e[0m"; compilation_ok=0; fi

        if [ "$compilation_ok" -eq 1 ]; then
            echo "  [2/3] 使用 llc 编译为汇编 (超时 ${LLC_TIMEOUT}s)..."
            timeout -s KILL ${LLC_TIMEOUT} "${LLC_CMD}" -O3 -march=riscv64 -mcpu=generic-rv64 -mattr=+m,+a,+f,+d,+c -filetype=asm "${ir_file}" -o "${assembly_file}"
            if [ $? -ne 0 ]; then echo -e "\e[31m错误: llc 编译失败或超时。\e[0m"; compilation_ok=0; fi
        fi

        if [ "$compilation_ok" -eq 1 ]; then
            echo "  [3/3] 使用 gcc 编译 (超时 ${GCC_TIMEOUT}s)..."
            timeout -s KILL ${GCC_TIMEOUT} "${GCC_RISCV64}" "${assembly_file}" -O3 -o "${executable_file}" -L"${LIB_DIR}" -lsysy_riscv -static
            if [ $? -ne 0 ]; then echo -e "\e[31m错误: GCC 编译失败或超时。\e[0m"; compilation_ok=0; fi
        fi
    else # EXECUTE_MODE
        # 路径2: sysyc -> gcc
        echo "  [1/2] 使用 sysyc 编译为汇编 (超时 ${SYSYC_TIMEOUT}s)..."
        timeout -s KILL ${SYSYC_TIMEOUT} "${SYSYC}" -S "${sy_file}" ${OPTIMIZE_FLAG} -o "${assembly_file}"
        if [ $? -ne 0 ]; then echo -e "\e[31m错误: SysY (汇编) 编译失败或超时。\e[0m"; compilation_ok=0; fi

        if [ "$compilation_ok" -eq 1 ]; then
            echo "  [2/2] 使用 gcc 编译 (超时 ${GCC_TIMEOUT}s)..."
            timeout -s KILL ${GCC_TIMEOUT} "${GCC_RISCV64}" "${assembly_file}" -O3 -o "${executable_file}" -L"${LIB_DIR}" -lsysy_riscv -static
            if [ $? -ne 0 ]; then echo -e "\e[31m错误: GCC 编译失败或超时。\e[0m"; compilation_ok=0; fi
        fi
    fi

    # --- 执行与测试阶段 (公共逻辑) ---
    if [ "$compilation_ok" -eq 1 ]; then
        if [ -f "${input_file}" ] || [ -f "${output_reference_file}" ]; then
            # --- 自动化测试模式 ---
            echo "  检测到 .in/.out 文件，进入自动化测试模式..."
            echo "  正在执行 (超时 ${EXEC_TIMEOUT}s)..."
            
            exec_cmd="${QEMU_RISCV64} \"${executable_file}\""
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
                        EXPECTED_STDOUT_FILE="${TMP_DIR}/${base_name}.expected_stdout"
                        head -n -1 "${output_reference_file}" > "${EXPECTED_STDOUT_FILE}"
                        
                        ret_ok=1
                        if [ "$ACTUAL_RETURN_CODE" -ne "$EXPECTED_RETURN_CODE" ]; then echo -e "\e[31m  返回码测试失败: 期望 ${EXPECTED_RETURN_CODE}, 实际 ${ACTUAL_RETURN_CODE}\e[0m"; ret_ok=0; fi
                        
                        out_ok=1
                        if ! diff -q <(tr -d '[:space:]' < "${output_actual_file}") <(tr -d '[:space:]' < "${EXPECTED_STDOUT_FILE}") >/dev/null 2>&1; then
                            echo -e "\e[31m  标准输出测试失败。\e[0m"; out_ok=0
                            display_file_content "${EXPECTED_STDOUT_FILE}" "    \e[36m--- 期望输出 ---\e[0m" "${MAX_OUTPUT_LINES}" "${MAX_OUTPUT_CHARS}"
                            display_file_content "${output_actual_file}" "    \e[36m--- 实际输出 ---\e[0m" "${MAX_OUTPUT_LINES}" "${MAX_OUTPUT_CHARS}"
                        fi

                        if [ "$ret_ok" -eq 1 ] && [ "$out_ok" -eq 1 ]; then echo -e "\e[32m  返回码与标准输出测试成功。\e[0m"; else is_passed=0; fi

                    else
                        if diff -q <(tr -d '[:space:]' < "${output_actual_file}") <(tr -d '[:space:]' < "${output_reference_file}") >/dev/null 2>&1; then
                            echo -e "\e[32m  标准输出测试成功。\e[0m"
                        else
                            echo -e "\e[31m  标准输出测试失败。\e[0m"; is_passed=0
                            display_file_content "${output_reference_file}" "    \e[36m--- 期望输出 ---\e[0m" "${MAX_OUTPUT_LINES}" "${MAX_OUTPUT_CHARS}"
                            display_file_content "${output_actual_file}" "    \e[36m--- 实际输出 ---\e[0m" "${MAX_OUTPUT_LINES}" "${MAX_OUTPUT_CHARS}"
                        fi
                    fi
                else
                    echo "  无参考输出文件。程序返回码: ${ACTUAL_RETURN_CODE}"
                fi
            fi
        else
            # --- 交互模式 ---
            echo -e "\e[33m\n  未找到 .in 或 .out 文件，进入交互模式...\e[0m"
            "${QEMU_RISCV64}" "${executable_file}"
            INTERACTIVE_RET_CODE=$?
            echo -e "\e[33m\n  交互模式执行完毕，程序返回码: ${INTERACTIVE_RET_CODE} (此结果未经验证)\e[0m"
        fi
    else
      is_passed=0
    fi

    # --- 状态总结 ---
    if [ "$is_passed" -eq 1 ]; then
        echo -e "\e[32m状态: 通过\e[0m"
        ((PASSED_CASES++))
    else
        echo -e "\e[31m状态: 失败\e[0m"
        FAILED_CASES_LIST+="${sy_file}\n"
    fi
done

# --- 打印最终总结 ---
print_summary
