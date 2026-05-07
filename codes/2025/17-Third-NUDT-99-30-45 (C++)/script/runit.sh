#!/bin/bash

# runit.sh - 用于编译和测试 SysY 程序的脚本
# 此脚本应该位于 mysysy/script/

export ASAN_OPTIONS=detect_leaks=0

# 定义相对于脚本位置的目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
TESTDATA_DIR="${SCRIPT_DIR}/../testdata"
BUILD_BIN_DIR="${SCRIPT_DIR}/../build/bin"
LIB_DIR="${SCRIPT_DIR}/../lib"
TMP_DIR="${SCRIPT_DIR}/tmp"

# 定义编译器和模拟器
SYSYC="${BUILD_BIN_DIR}/sysyc"
LLC_CMD="llc-19"
GCC_RISCV64="riscv64-linux-gnu-gcc"
QEMU_RISCV64="qemu-riscv64"

# --- 状态变量 ---
EXECUTE_MODE=false
IR_EXECUTE_MODE=false
OPTIMIZE_FLAG=""
SYSYC_TIMEOUT=30
LLC_TIMEOUT=10
GCC_TIMEOUT=10
EXEC_TIMEOUT=30
MAX_OUTPUT_LINES=20
MAX_OUTPUT_CHARS=1000
TEST_SETS=()
PERF_RUN_COUNT=1 # 新增: 性能测试运行次数
TOTAL_CASES=0
PASSED_CASES=0
FAILED_CASES_LIST=""
INTERRUPTED=false
PERFORMANCE_MODE=false # 新增: 标记是否进行性能测试

# =================================================================
# --- 函数定义 ---
# =================================================================

# 显示帮助信息的函数
show_help() {
    echo "用法: $0 [选项]"
    echo "此脚本用于按文件名前缀数字升序编译和测试 .sy 文件。"
    echo ""
    echo "选项:"
    echo "  -e, --executable         编译为汇编并运行测试 (sysyc -> gcc -> qemu)。"
    echo "  -eir                     通过IR编译为可执行文件并运行测试 (sysyc -> llc -> gcc -> qemu)。"
    echo "  -c, --clean              清理 'tmp' 目录下的所有生成文件。"
    echo "  -O1                      启用 sysyc 的 -O1 优化。"
    echo "  -set [f|h|p|all]...    指定要运行的测试集 (functional, h_functional, performance)。可多选，默认为 all。"
    echo "                           当包含 'p' 时，会自动记录性能数据到 ${TMP_DIR}/performance_time.csv。"
    echo "  -pt N                    设置 performance 测试集的每个用例运行 N 次取平均值 (默认: 1)。"
    echo "  -sct N                   设置 sysyc 编译超时为 N 秒 (默认: 30)。"
    echo "  -lct N                   设置 llc-19 编译超时为 N 秒 (默认: 10)。"
    echo "  -gct N                   设置 gcc 交叉编译超时为 N 秒 (默认: 10)。"
    echo "  -et N                    设置 qemu 执行超时为 N 秒 (默认: 30)。"
    echo "  -ml N, --max-lines N     当输出对比失败时，最多显示 N 行内容 (默认: 20)。"
    echo "  -mc N, --max-chars N     当输出对比失败时，最多显示 N 个字符 (默认: 1000)。"
    echo "  -h, --help               显示此帮助信息并退出。"
    echo ""
    echo "注意: 默认行为 (无 -e 或 -eir) 是将 .sy 文件同时编译为 .s (汇编) 和 .ll (IR)，不执行。"
    echo "      可在任何时候按 Ctrl+C 来中断测试并显示当前已完成的测例总结。"
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

# 清理临时文件的函数
clean_tmp() {
    echo "正在清理临时目录: ${TMP_DIR}"
    rm -rf "${TMP_DIR}"/*
}

# --- 新增：总结报告函数 ---
print_summary() {
    echo "" # 确保从新的一行开始
    echo "========================================"
    if [ "$INTERRUPTED" = true ]; then
        echo -e "\e[33m测试被中断。正在汇总已完成的结果...\e[0m"
    else
        echo "测试完成"
    fi

    local failed_count
    if [ -n "$FAILED_CASES_LIST" ]; then
        failed_count=$(echo -e -n "${FAILED_CASES_LIST}" | wc -l)
    else
        failed_count=0
    fi
    local executed_count=$((PASSED_CASES + failed_count))

    echo "测试结果: [通过: ${PASSED_CASES}, 失败: ${failed_count}, 已执行: ${executed_count}/${TOTAL_CASES}]"

    if [ -n "$FAILED_CASES_LIST" ]; then
        echo ""
        echo -e "\e[31m未通过的测例:\e[0m"
        printf "%b" "${FAILED_CASES_LIST}"
    fi

    # --- 本次修改点: 提示性能测试结果文件 ---
    if ${PERFORMANCE_MODE}; then
        # --- 本次修改点: 计算并添加总计行 ---
        if [ -f "${PERFORMANCE_CSV_FILE}" ] && [ $(wc -l < "${PERFORMANCE_CSV_FILE}") -gt 1 ]; then
            local total_seconds_sum
            total_seconds_sum=$(awk -F, 'NR > 1 {sum += $3} END {printf "%.5f", sum}' "${PERFORMANCE_CSV_FILE}")
            
            local total_s_int=${total_seconds_sum%.*}
            [[ -z "$total_s_int" ]] && total_s_int=0 # 处理小于1秒的情况
            local total_us_int=$(echo "(${total_seconds_sum} - ${total_s_int}) * 1000000" | bc | cut -d. -f1)
            local total_time_str="${total_s_int}s${total_us_int}us"
            
            echo "all,${total_time_str},${total_seconds_sum}" >> "${PERFORMANCE_CSV_FILE}"
        fi
        echo ""
        echo -e "\e[32m性能测试数据已保存到: ${PERFORMANCE_CSV_FILE}\e[0m"
    fi

    echo "========================================"

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

trap handle_sigint SIGINT
mkdir -p "${TMP_DIR}"

while [[ "$#" -gt 0 ]]; do
    case "$1" in
        -e|--executable) EXECUTE_MODE=true; shift ;;
        -eir) IR_EXECUTE_MODE=true; shift ;;
        -c|--clean) clean_tmp; exit 0 ;;
        -O1) OPTIMIZE_FLAG="-O1"; shift ;;
        -set)
            shift
            while [[ "$#" -gt 0 && ! "$1" =~ ^- ]]; do TEST_SETS+=("$1"); shift; done
            ;;
        -pt) if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then PERF_RUN_COUNT="$2"; shift 2; else echo "错误: -pt 需要一个正整数参数。" >&2; exit 1; fi ;;
        -sct) if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then SYSYC_TIMEOUT="$2"; shift 2; else echo "错误: -sct 需要一个正整数参数。" >&2; exit 1; fi ;;
        -lct) if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then LLC_TIMEOUT="$2"; shift 2; else echo "错误: -lct 需要一个正整数参数。" >&2; exit 1; fi ;;
        -gct) if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then GCC_TIMEOUT="$2"; shift 2; else echo "错误: -gct 需要一个正整数参数。" >&2; exit 1; fi ;;
        -et) if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then EXEC_TIMEOUT="$2"; shift 2; else echo "错误: -et 需要一个正整数参数。" >&2; exit 1; fi ;;
        -ml|--max-lines) if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then MAX_OUTPUT_LINES="$2"; shift 2; else echo "错误: --max-lines 需要一个正整数参数。" >&2; exit 1; fi ;;
        -mc|--max-chars) if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then MAX_OUTPUT_CHARS="$2"; shift 2; else echo "错误: --max-chars 需要一个正整数参数。" >&2; exit 1; fi ;;
        -h|--help) show_help; exit 0 ;;
        *) echo "未知选项: $1"; show_help; exit 1 ;;
    esac
done

if ${EXECUTE_MODE} && ${IR_EXECUTE_MODE}; then
    echo -e "\e[31m错误: -e 和 -eir 选项不能同时使用。\e[0m" >&2
    exit 1
fi

declare -A SET_MAP
SET_MAP[f]="functional"
SET_MAP[h]="h_functional"
SET_MAP[p]="performance"

SEARCH_PATHS=()
if [ ${#TEST_SETS[@]} -eq 0 ] || [[ " ${TEST_SETS[@]} " =~ " all " ]]; then
    SEARCH_PATHS+=("${TESTDATA_DIR}")
    if [ -d "${TESTDATA_DIR}/performance" ]; then PERFORMANCE_MODE=true; fi
else
    for set in "${TEST_SETS[@]}"; do
        if [[ -v SET_MAP[$set] ]]; then
            SEARCH_PATHS+=("${TESTDATA_DIR}/${SET_MAP[$set]}")
            if [[ "$set" == "p" ]]; then
                PERFORMANCE_MODE=true
            fi
        else
            echo -e "\e[33m警告: 未知的测试集 '$set'，已忽略。\e[0m"
        fi
    done
fi

if [ ${#SEARCH_PATHS[@]} -eq 0 ]; then
    echo -e "\e[31m错误: 没有找到有效的测试集目录，测试中止。\e[0m"
    exit 1
fi

echo "SysY 测试运行器启动..."
if [ -n "$OPTIMIZE_FLAG" ]; then echo "优化等级: ${OPTIMIZE_FLAG}"; fi
echo "输入目录: ${SEARCH_PATHS[@]}"
echo "临时目录: ${TMP_DIR}"

RUN_MODE_INFO=""
if ${IR_EXECUTE_MODE}; then
    RUN_MODE_INFO="IR执行模式 (-eir)"
    TIMEOUT_INFO="超时设置: sysyc=${SYSYC_TIMEOUT}s, llc=${LLC_TIMEOUT}s, gcc=${GCC_TIMEOUT}s, qemu=${EXEC_TIMEOUT}s"
elif ${EXECUTE_MODE}; then
    RUN_MODE_INFO="直接执行模式 (-e)"
    TIMEOUT_INFO="超时设置: sysyc=${SYSYC_TIMEOUT}s, gcc=${GCC_TIMEOUT}s, qemu=${EXEC_TIMEOUT}s"
else
    RUN_MODE_INFO="编译模式 (默认)"
    TIMEOUT_INFO="超时设置: sysyc=${SYSYC_TIMEOUT}s"
fi
echo "运行模式: ${RUN_MODE_INFO}"
echo "${TIMEOUT_INFO}"
if ${PERFORMANCE_MODE} && ([ ${EXECUTE_MODE} = true ] || [ ${IR_EXECUTE_MODE} = true ]) && [ ${PERF_RUN_COUNT} -gt 1 ]; then
    echo "性能测试运行次数: ${PERF_RUN_COUNT}"
fi
if ${EXECUTE_MODE} || ${IR_EXECUTE_MODE}; then
    echo "失败输出最大行数: ${MAX_OUTPUT_LINES}"
    echo "失败输出最大字符数: ${MAX_OUTPUT_CHARS}"
fi
echo ""

sy_files=$(find "${SEARCH_PATHS[@]}" -name "*.sy" | sort -V)
if [ -z "$sy_files" ]; then
    echo "在指定目录中未找到任何 .sy 文件。"
    exit 0
fi
TOTAL_CASES=$(echo "$sy_files" | wc -w)

PERFORMANCE_CSV_FILE="${TMP_DIR}/performance_time.csv"
if ${PERFORMANCE_MODE}; then
    echo "Case,Time_String,Time_Seconds" > "${PERFORMANCE_CSV_FILE}"
fi

while IFS= read -r sy_file; do
    is_passed=0 # 0 表示失败, 1 表示通过

    relative_path_no_ext=$(realpath --relative-to="${TESTDATA_DIR}" "${sy_file%.*}")
    output_base_name=$(echo "${relative_path_no_ext}" | tr '/' '_')

    assembly_file_S="${TMP_DIR}/${output_base_name}_sysyc_S.s"
    executable_file_S="${TMP_DIR}/${output_base_name}_sysyc_S"
    output_actual_file_S="${TMP_DIR}/${output_base_name}_sysyc_S.actual_out"
    stderr_file_S="${TMP_DIR}/${output_base_name}_sysyc_S.stderr"

    ir_file="${TMP_DIR}/${output_base_name}_sysyc_ir.ll"
    assembly_file_from_ir="${TMP_DIR}/${output_base_name}_from_ir.s"
    executable_file_from_ir="${TMP_DIR}/${output_base_name}_from_ir"
    output_actual_file_from_ir="${TMP_DIR}/${output_base_name}_from_ir.actual_out"
    stderr_file_from_ir="${TMP_DIR}/${output_base_name}_from_ir.stderr"

    input_file="${sy_file%.*}.in"
    output_reference_file="${sy_file%.*}.out"

    echo "正在处理: $(basename "$sy_file") (路径: ${relative_path_no_ext}.sy)"

    # --- 模式 1: IR 执行模式 (-eir) ---
    if ${IR_EXECUTE_MODE}; then
        step_failed=0
        test_logic_passed=0
        total_time_us=0
        
        echo "  [1/4] 使用 sysyc 编译为 IR (超时 ${SYSYC_TIMEOUT}s)..."
        timeout -s KILL ${SYSYC_TIMEOUT} "${SYSYC}" -s ir "${sy_file}" -o "${ir_file}" ${OPTIMIZE_FLAG}; if [ $? -ne 0 ]; then echo -e "\e[31m错误: SysY (IR) 编译失败或超时\e[0m"; step_failed=1; fi

        if [ "$step_failed" -eq 0 ]; then
            echo "  [2/4] 使用 llc-19 编译为汇编 (超时 ${LLC_TIMEOUT}s)..."
            timeout -s KILL ${LLC_TIMEOUT} ${LLC_CMD} -march=riscv64 -mcpu=generic-rv64 -mattr=+m,+a,+f,+d,+c -filetype=asm "${ir_file}" -o "${assembly_file_from_ir}"; if [ $? -ne 0 ]; then echo -e "\e[31m错误: llc-19 编译失败或超时\e[0m"; step_failed=1; fi
        fi

        if [ "$step_failed" -eq 0 ]; then
            echo "  [3/4] 使用 gcc 编译 (超时 ${GCC_TIMEOUT}s)..."
            timeout -s KILL ${GCC_TIMEOUT} "${GCC_RISCV64}" "${assembly_file_from_ir}" -o "${executable_file_from_ir}" -L"${LIB_DIR}" -lsysy_riscv -static; if [ $? -ne 0 ]; then echo -e "\e[31m错误: GCC 编译失败或超时\e[0m"; step_failed=1; fi
        fi

        if [ "$step_failed" -eq 0 ]; then
            echo "  [4/4] 正在执行 (超时 ${EXEC_TIMEOUT}s)..."
            current_run_failed=0
            for (( i=1; i<=PERF_RUN_COUNT; i++ )); do
                if [ ${PERF_RUN_COUNT} -gt 1 ]; then echo -n "    第 $i/${PERF_RUN_COUNT} 次运行... "; fi
                exec_cmd="${QEMU_RISCV64} \"${executable_file_from_ir}\""
                [ -f "${input_file}" ] && exec_cmd+=" < \"${input_file}\""
                exec_cmd+=" > \"${output_actual_file_from_ir}\" 2> \"${stderr_file_from_ir}\""
                eval "timeout -s KILL ${EXEC_TIMEOUT} ${exec_cmd}"
                ACTUAL_RETURN_CODE=$?
                
                if [ "$ACTUAL_RETURN_CODE" -eq 124 ]; then echo -e "\e[31m超时\e[0m"; current_run_failed=1; break; fi
                if ${PERFORMANCE_MODE}; then
                    TIME_LINE=$(grep "TOTAL:" "${stderr_file_from_ir}")
                    if [ -n "$TIME_LINE" ]; then
                        H=$(echo "$TIME_LINE" | sed -E 's/TOTAL: ([0-9]+)H-.*/\1/')
                        M=$(echo "$TIME_LINE" | sed -E 's/.*-([0-9]+)M-.*/\1/')
                        S=$(echo "$TIME_LINE" | sed -E 's/.*-([0-9]+)S-.*/\1/')
                        US=$(echo "$TIME_LINE" | sed -E 's/.*-([0-9]+)us/\1/')
                        run_time_us=$(( H * 3600000000 + M * 60000000 + S * 1000000 + US ))
                        total_time_us=$(( total_time_us + run_time_us ))
                        if [ ${PERF_RUN_COUNT} -gt 1 ]; then echo "耗时: ${run_time_us}us"; fi
                    else
                        echo -e "\e[31m未找到时间信息\e[0m"; current_run_failed=1; break
                    fi
                fi
            done
            
            if [ "$current_run_failed" -eq 0 ]; then
                test_logic_passed=1
                if [ -f "${output_reference_file}" ]; then
                    LAST_LINE_TRIMMED=$(tail -n 1 "${output_reference_file}" | tr -d '[:space:]')
                    if [[ "$LAST_LINE_TRIMMED" =~ ^[-+]?[0-9]+$ ]]; then
                        EXPECTED_RETURN_CODE="$LAST_LINE_TRIMMED"
                        EXPECTED_STDOUT_FILE="${TMP_DIR}/${output_base_name}_from_ir.expected_stdout"
                        head -n -1 "${output_reference_file}" > "${EXPECTED_STDOUT_FILE}"
                        if [ "$ACTUAL_RETURN_CODE" -ne "$EXPECTED_RETURN_CODE" ]; then echo -e "\e[31m  返回码测试失败: 期望 ${EXPECTED_RETURN_CODE}, 实际 ${ACTUAL_RETURN_CODE}\e[0m"; test_logic_passed=0; fi
                        if ! diff -q <(tr -d '[:space:]' < "${output_actual_file_from_ir}") <(tr -d '[:space:]' < "${EXPECTED_STDOUT_FILE}") >/dev/null 2>&1; then
                            echo -e "\e[31m  标准输出测试失败\e[0m"; test_logic_passed=0
                            display_file_content "${EXPECTED_STDOUT_FILE}" "    \e[36m--- 期望输出 ---\e[0m" "${MAX_OUTPUT_LINES}" "${MAX_OUTPUT_CHARS}"
                            display_file_content "${output_actual_file_from_ir}" "    \e[36m--- 实际输出 ---\e[0m" "${MAX_OUTPUT_LINES}" "${MAX_OUTPUT_CHARS}"
                        fi
                    else
                        if [ $ACTUAL_RETURN_CODE -ne 0 ]; then echo -e "\e[33m警告: 程序以非零状态 ${ACTUAL_RETURN_CODE} 退出 (纯输出比较模式)。\e[0m"; fi
                        if ! diff -q <(tr -d '[:space:]' < "${output_actual_file_from_ir}") <(tr -d '[:space:]' < "${output_reference_file}") >/dev/null 2>&1; then
                            echo -e "\e[31m  失败: 输出不匹配\e[0m"; test_logic_passed=0
                            display_file_content "${output_reference_file}" "    \e[36m--- 期望输出 ---\e[0m" "${MAX_OUTPUT_LINES}" "${MAX_OUTPUT_CHARS}"
                            display_file_content "${output_actual_file_from_ir}" "    \e[36m--- 实际输出 ---\e[0m" "${MAX_OUTPUT_LINES}" "${MAX_OUTPUT_CHARS}"
                        fi
                    fi
                fi
                if [ "$test_logic_passed" -eq 1 ]; then echo -e "\e[32m  测试逻辑通过\e[0m"; fi
            fi
        fi
        if [ "$step_failed" -eq 0 ] && [ "$test_logic_passed" -eq 1 ]; then is_passed=1; fi
        
        if ${PERFORMANCE_MODE}; then
            avg_time_us=0
            if [ "$is_passed" -eq 1 ]; then
                avg_time_us=$(( total_time_us / PERF_RUN_COUNT ))
            fi
            S_AVG=$(( avg_time_us / 1000000 ))
            US_AVG=$(( avg_time_us % 1000000 ))
            TIME_STRING_AVG="${S_AVG}s${US_AVG}us"
            TOTAL_SECONDS_AVG=$(echo "scale=5; ${avg_time_us} / 1000000" | bc)
            echo "$(basename ${sy_file}),${TIME_STRING_AVG},${TOTAL_SECONDS_AVG}" >> "${PERFORMANCE_CSV_FILE}"
        fi

    # --- 模式 2: 直接执行模式 (-e) ---
    elif ${EXECUTE_MODE}; then
        step_failed=0
        test_logic_passed=0
        total_time_us=0

        echo "  [1/3] 使用 sysyc 编译为汇编 (超时 ${SYSYC_TIMEOUT}s)..."
        timeout -s KILL ${SYSYC_TIMEOUT} "${SYSYC}" -S "${sy_file}" -o "${assembly_file_S}" ${OPTIMIZE_FLAG}; if [ $? -ne 0 ]; then echo -e "\e[31m错误: SysY (汇编) 编译失败或超时\e[0m"; step_failed=1; fi

        if [ "$step_failed" -eq 0 ]; then
            echo "  [2/3] 使用 gcc 编译 (超时 ${GCC_TIMEOUT}s)..."
            timeout -s KILL ${GCC_TIMEOUT} "${GCC_RISCV64}" "${assembly_file_S}" -o "${executable_file_S}" -L"${LIB_DIR}" -lsysy_riscv -static; if [ $? -ne 0 ]; then echo -e "\e[31m错误: GCC 编译失败或超时\e[0m"; step_failed=1; fi
        fi

        if [ "$step_failed" -eq 0 ]; then
            echo "  [3/3] 正在执行 (超时 ${EXEC_TIMEOUT}s)..."
            current_run_failed=0
            for (( i=1; i<=PERF_RUN_COUNT; i++ )); do
                if [ ${PERF_RUN_COUNT} -gt 1 ]; then echo -n "    第 $i/${PERF_RUN_COUNT} 次运行... "; fi
                exec_cmd="${QEMU_RISCV64} \"${executable_file_S}\""
                [ -f "${input_file}" ] && exec_cmd+=" < \"${input_file}\""
                exec_cmd+=" > \"${output_actual_file_S}\" 2> \"${stderr_file_S}\""
                eval "timeout -s KILL ${EXEC_TIMEOUT} ${exec_cmd}"
                ACTUAL_RETURN_CODE=$?
                
                if [ "$ACTUAL_RETURN_CODE" -eq 124 ]; then echo -e "\e[31m超时\e[0m"; current_run_failed=1; break; fi
                if ${PERFORMANCE_MODE}; then
                    TIME_LINE=$(grep "TOTAL:" "${stderr_file_S}")
                    if [ -n "$TIME_LINE" ]; then
                        H=$(echo "$TIME_LINE" | sed -E 's/TOTAL: ([0-9]+)H-.*/\1/')
                        M=$(echo "$TIME_LINE" | sed -E 's/.*-([0-9]+)M-.*/\1/')
                        S=$(echo "$TIME_LINE" | sed -E 's/.*-([0-9]+)S-.*/\1/')
                        US=$(echo "$TIME_LINE" | sed -E 's/.*-([0-9]+)us/\1/')
                        run_time_us=$(( H * 3600000000 + M * 60000000 + S * 1000000 + US ))
                        total_time_us=$(( total_time_us + run_time_us ))
                        if [ ${PERF_RUN_COUNT} -gt 1 ]; then echo "耗时: ${run_time_us}us"; fi
                    else
                        echo -e "\e[31m未找到时间信息\e[0m"; current_run_failed=1; break
                    fi
                fi
            done
            
            if [ "$current_run_failed" -eq 0 ]; then
                test_logic_passed=1
                if [ -f "${output_reference_file}" ]; then
                    LAST_LINE_TRIMMED=$(tail -n 1 "${output_reference_file}" | tr -d '[:space:]')
                    if [[ "$LAST_LINE_TRIMMED" =~ ^[-+]?[0-9]+$ ]]; then
                        EXPECTED_RETURN_CODE="$LAST_LINE_TRIMMED"
                        EXPECTED_STDOUT_FILE="${TMP_DIR}/${output_base_name}_sysyc_S.expected_stdout"
                        head -n -1 "${output_reference_file}" > "${EXPECTED_STDOUT_FILE}"
                        if [ "$ACTUAL_RETURN_CODE" -ne "$EXPECTED_RETURN_CODE" ]; then echo -e "\e[31m  返回码测试失败: 期望 ${EXPECTED_RETURN_CODE}, 实际 ${ACTUAL_RETURN_CODE}\e[0m"; test_logic_passed=0; fi
                        if ! diff -q <(tr -d '[:space:]' < "${output_actual_file_S}") <(tr -d '[:space:]' < "${EXPECTED_STDOUT_FILE}") >/dev/null 2>&1; then
                            echo -e "\e[31m  标准输出测试失败\e[0m"; test_logic_passed=0
                            display_file_content "${EXPECTED_STDOUT_FILE}" "    \e[36m--- 期望输出 ---\e[0m" "${MAX_OUTPUT_LINES}" "${MAX_OUTPUT_CHARS}"
                            display_file_content "${output_actual_file_S}" "    \e[36m--- 实际输出 ---\e[0m" "${MAX_OUTPUT_LINES}" "${MAX_OUTPUT_CHARS}"
                        fi
                    else
                        if [ $ACTUAL_RETURN_CODE -ne 0 ]; then echo -e "\e[33m警告: 程序以非零状态 ${ACTUAL_RETURN_CODE} 退出 (纯输出比较模式)。\e[0m"; fi
                        if ! diff -q <(tr -d '[:space:]' < "${output_actual_file_S}") <(tr -d '[:space:]' < "${output_reference_file}") >/dev/null 2>&1; then
                            echo -e "\e[31m  失败: 输出不匹配\e[0m"; test_logic_passed=0
                            display_file_content "${output_reference_file}" "    \e[36m--- 期望输出 ---\e[0m" "${MAX_OUTPUT_LINES}" "${MAX_OUTPUT_CHARS}"
                            display_file_content "${output_actual_file_S}" "    \e[36m--- 实际输出 ---\e[0m" "${MAX_OUTPUT_LINES}" "${MAX_OUTPUT_CHARS}"
                        fi
                    fi
                fi
                if [ "$test_logic_passed" -eq 1 ]; then echo -e "\e[32m  测试逻辑通过\e[0m"; fi
            fi
        fi
        if [ "$step_failed" -eq 0 ] && [ "$test_logic_passed" -eq 1 ]; then is_passed=1; fi
        
        if ${PERFORMANCE_MODE}; then
            avg_time_us=0
            if [ "$is_passed" -eq 1 ]; then
                avg_time_us=$(( total_time_us / PERF_RUN_COUNT ))
            fi
            S_AVG=$(( avg_time_us / 1000000 ))
            US_AVG=$(( avg_time_us % 1000000 ))
            TIME_STRING_AVG="${S_AVG}s${US_AVG}us"
            TOTAL_SECONDS_AVG=$(echo "scale=5; ${avg_time_us} / 1000000" | bc)
            echo "$(basename ${sy_file}),${TIME_STRING_AVG},${TOTAL_SECONDS_AVG}" >> "${PERFORMANCE_CSV_FILE}"
        fi

    # --- 模式 3: 默认编译模式 ---
    else
        s_compile_ok=0
        ir_compile_ok=0

        echo "  [1/2] 使用 sysyc 编译为汇编 (超时 ${SYSYC_TIMEOUT}s)..."
        timeout -s KILL ${SYSYC_TIMEOUT} "${SYSYC}" -S "${sy_file}" -o "${assembly_file_S}" ${OPTIMIZE_FLAG}
        SYSYC_S_STATUS=$?
        if [ $SYSYC_S_STATUS -eq 0 ]; then
            s_compile_ok=1
            echo -e "      \e[32m-> ${assembly_file_S} [成功]\e[0m"
        else
            [ $SYSYC_S_STATUS -eq 124 ] && echo -e "      \e[31m-> [编译超时]\e[0m" || echo -e "      \e[31m-> [编译失败, 退出码: ${SYSYC_S_STATUS}]\e[0m"
        fi

        echo "  [2/2] 使用 sysyc 编译为 IR (超时 ${SYSYC_TIMEOUT}s)..."
        timeout -s KILL ${SYSYC_TIMEOUT} "${SYSYC}" -s ir "${sy_file}" -o "${ir_file}" ${OPTIMIZE_FLAG}
        SYSYC_IR_STATUS=$?
        if [ $SYSYC_IR_STATUS -eq 0 ]; then
            ir_compile_ok=1
            echo -e "      \e[32m-> ${ir_file} [成功]\e[0m"
        else
            [ $SYSYC_IR_STATUS -eq 124 ] && echo -e "      \e[31m-> [编译超时]\e[0m" || echo -e "      \e[31m-> [编译失败, 退出码: ${SYSYC_IR_STATUS}]\e[0m"
        fi

        if [ "$s_compile_ok" -eq 1 ] && [ "$ir_compile_ok" -eq 1 ]; then
            is_passed=1
        fi
    fi

    # --- 统计结果 ---
    if [ "$is_passed" -eq 1 ]; then
        ((PASSED_CASES++))
    else
        # 确保 FAILED_CASES_LIST 的每一项都以换行符结尾
        FAILED_CASES_LIST+="${relative_path_no_ext}.sy\n"
    fi
    echo ""
done <<< "$sy_files"

# --- 修改：调用总结函数 ---
print_summary
