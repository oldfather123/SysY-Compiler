#!/bin/bash

source "$(dirname "${BASH_SOURCE[0]}")/../../scripts_utils/libbash.sh" 2>/dev/null || true
source "$(dirname "${BASH_SOURCE[0]}")/../test/"*.sh 2>/dev/null || true


TESTS_DIR="tests/公开样例与运行时库"
TESTS_SELF_DIR="tests_self"
SYLIB_DIR="tests_2025"

# 所有要编译的测试目录
declare -a TEST_DIRS=(
    # "tests/公开样例与运行时库/functional"
    # "tests/公开样例与运行时库/performance"
    # "tests/公开样例与运行时库/hidden_functional"
    # "tests/公开样例与运行时库/final_performance"
    "tests_2025/functional"
    "tests_2025/h_functional"
    "tests_2025/performance"
    "tests_self"
)

# 要跳过的测试
declare -a SKIP_TESTS=(
    "65_color.sy" # 127.56s - 133.57s
    # "86_long_code2.sy" # 9985ms
    # "29_long_line.sy" # 51248ms
    # "13_LCA.sy"
    # "39_fp_params.sy" # 浮点问题
    # "30_many_dimensions.sy" # 21387ms
    # "34_multi_loop.sy" # 342010ms
    # "38_light2d.sy" # 183442ms
    # "00_bitset1.sy" # 3578552ms
    # "00_bitset2.sy" # 7066447ms
    # "00_bitset3.sy"

    # "62_percolation.sy"
    # "04_break_continue.sy"
    # "26_scope4.sy"
    # "33_multi_branch.sy"
    # "35_math.sy"
)

suffix_by() {
    local stage=$1
    case "$stage" in
        "parser")
            echo "ast"
            ;; 
        "ssa")
            echo "ssa"
            ;;
        "interp")
            echo "interp"
            ;;
        "asm")
            echo "S"
            ;;
        *)
            echo "ast"
            ;;
    esac
}

# QEMU 执行函数
run_qemu_single() {
    local sy_file="$1"
    local base="${sy_file%.sy}"
    
    # 文件定义
    local asm_file="$base.S"
    local exe_file="$base.E"
    local in_file="$base.in"
    local expected_out="$base.out"
    local actual_out="$base.OUT"
    local log_file="$base.log"
    
    # 记录编译状态
    local compile_success=0
    local link_success=0
    
    # 1. 生成汇编
    echo "Generating assembly..." > "$log_file"
    local timing_line=""
    if ! RUST_BACKTRACE=1 RUST_LOG=info RUSTFLAGS="-A warnings" cargo run --release -- "$sy_file" --stage asm -o "$asm_file" >> "$log_file" 2>&1; then
        echo "Assembly generation failed" >> "$log_file"
        return 1
    fi
    compile_success=1
    echo "Assembly generation succeeded" >> "$log_file"
    # 获取编译器时间信息
    timing_line=$(grep -E '\\e\[90m\[.*ms\]' "$log_file" | tail -1)
    
    # 2. 编译链接
    echo "Compiling to executable..." >> "$log_file"
    if ! riscv64-linux-gnu-gcc -static -march=rv64gc \
        "$asm_file" \
        "$SYLIB_DIR/sylib.c" \
        "$SYLIB_DIR/libsysy.a" \
        -o "$exe_file" >> "$log_file" 2>&1; then
        echo "Linking failed" >> "$log_file"
        return 1
    fi
    link_success=1
    echo "Linking succeeded" >> "$log_file"
    
    # 3. 执行并计时
    echo "Executing with QEMU..." >> "$log_file"
    local start_time=$(date +%s.%N)
    local qemu_exit_code=0
    
    # 执行程序并捕获输出和退出码
    # 将 stderr 包含TOTAL时间信息）重定向到临时文件
    local stderr_file="$base.stderr"
    if [ -f "$in_file" ]; then
        timeout 30s qemu-riscv64 "$exe_file" < "$in_file" > "$actual_out" 2>"$stderr_file"
        qemu_exit_code=$?
    else
        timeout 30s qemu-riscv64 "$exe_file" > "$actual_out" 2>"$stderr_file"
        qemu_exit_code=$?
    fi
    
    # 将 stderr 内容追加到日志（包含 TOTAL 时间信息）
    if [ -f "$stderr_file" ]; then
        cat "$stderr_file" >> "$log_file"
        rm -f "$stderr_file"
    fi
    
    # 处理超时情况
    if [ $qemu_exit_code -eq 124 ]; then
        echo "Program execution timed out" >> "$log_file"
        return 1
    fi
    
    # 将返回码追加到输出文件（作为输出的一部分）
    # 先确保输出文件以换行结束
    if [ -s "$actual_out" ] && [ "$(tail -c 1 "$actual_out")" != "" ]; then
        echo "" >> "$actual_out"
    fi
    echo "$qemu_exit_code" >> "$actual_out"
    
    local end_time=$(date +%s.%N)
    # 使用 bc 计算浮点数，如果没有 bc 则使用 awk
    if command -v bc >/dev/null 2>&1; then
        local runtime=$(echo "$end_time - $start_time" | bc)
    else
        local runtime=$(awk "BEGIN {print $end_time - $start_time}")
    fi
    
    # 4. 记录时间
    echo "Execution time: ${runtime}s" >> "$log_file"
    echo "Exit code: $qemu_exit_code" >> "$log_file"
    
    # 追加 QEMU 执行时间到 timing_line（毫秒）
    local runtime_ms=$(awk "BEGIN {printf \"%.0f\", $runtime * 1000}")
    if [ -n "$timing_line" ]; then
        # 去掉末尾的转义序列，添加 qemu 时间
        timing_line=$(echo "$timing_line" | sed 's/\\e\[0m$//')
        timing_line="${timing_line}, qemu: ${runtime_ms}ms\e[0m"
    else
        timing_line="qemu: ${runtime_ms}ms"
    fi
    
    # 将 timing_line 保存到临时文件供调用者读取
    echo "$timing_line" > "$base.timing"
    
    # 5. 比较输出（如果有期望输出）
    if [ -f "$expected_out" ]; then
        if ! diff -u "$expected_out" "$actual_out" >> "$log_file" 2>&1; then
            echo "Output mismatch detected" >> "$log_file"
            return 1
        fi
    fi
    
    # 6. 成功返回
    return 0
}

# 处理单个测试目录
process_test_directory() {
    local test_dir="$1"
    local stage="$2"
    local suffix="$3"
    local success_count_ref="$4"
    local failure_count_ref="$5"
    local failed_tests_ref="$6"
    
    local dir_name=$(basename "$test_dir")
    NOTE "----- $dir_name ----"
    
    if [ -d "$test_dir" ]; then
        for sy_file in "$test_dir"/*.sy; do
            if [ -f "$sy_file" ]; then
                local target_file="${sy_file%.sy}.${suffix}"
                local log_file="${sy_file%.sy}.log"
                local test_name
                test_name=$(basename "$sy_file")
                
                # 检查是否需要跳过测试
                if [[ " ${SKIP_TESTS[@]} " =~ " $test_name " ]]; then
                    WARN "$test_name -- Skipped"
                    continue
                fi
                
                # 解释器批量测试模式：需要.out文件才执行
                if [ "$stage" = "interp" ]; then
                    local out_file="${sy_file%.sy}.out"
                    if [ ! -f "$out_file" ]; then
                        WARN "$test_name -- Skipped -- no .out"
                        continue
                    fi
                fi
                
                # 构造额外参数
                local extra_args=""
                if [ "$stage" = "interp" ]; then
                    local in_file="${sy_file%.sy}.in"
                    local out_file="${sy_file%.sy}.out"
                    if [ -f "$in_file" ]; then
                        extra_args="$extra_args --target-stdin $in_file"
                    fi
                    extra_args="$extra_args --target-stdout $out_file"
                fi
                if RUST_BACKTRACE=1 RUST_LOG=info RUSTFLAGS="-A warnings" cargo run --release -- "$sy_file" --stage "$stage" -o "$target_file" $extra_args > "$log_file" 2>&1; then
                    local timing_line
                    timing_line=$(grep -E '\\e\[90m\[.*ms\]' "$log_file" | tail -1)
                    if [ -n "$timing_line" ]; then
                        SUCCESS "$test_name -- $timing_line"
                    else
                        SUCCESS "$test_name -- No timing information found"
                    fi
                    eval "$success_count_ref=\$((\$$success_count_ref + 1))"
                else
                    ERROR "$test_name -- Compilation failed! See $log_file"
                    # 打印日志文件的最后几行以快速查看错误
                    # tail -n 10 "$log_file"
                    eval "$failure_count_ref=\$((\$$failure_count_ref + 1))"
                    eval "$failed_tests_ref+=('$test_name')"
                fi
            fi
        done
    fi
}

compile_tests() {
    local start_time=$SECONDS
    local stage=${1:-"parser"}
    local suffix=$(suffix_by "$stage")
    RUSTFLAGS="-A warnings" cargo build --release || { ERROR "编译器构建失败"; return 1; }

    local success_count=0
    local failure_count=0
    local failed_tests=()

    # 遍历所有测试目录
    for test_dir in "${TEST_DIRS[@]}"; do
        process_test_directory "$test_dir" "$stage" "$suffix" "success_count" "failure_count" "failed_tests"
    done

    local duration=$((SECONDS - start_time))
    local skipped_count=${#SKIP_TESTS[@]}
    # 打印总结报告
    echo
    ECHO "BLUE_BOLD" "TEST SUMMARY:"
    SUCCESS "Passed: $success_count"
    WARN "Skipped: $skipped_count"
    ERROR "Failed: $failure_count"
    echo ""

    if [ $skipped_count -gt 0 ]; then
        WARN "Skipped tests:"
        for test in "${SKIP_TESTS[@]}"; do
            echo -e "  - $test"
        done
    fi

    if [ $failure_count -gt 0 ]; then
        ERROR "Failed tests:"
        for test in "${failed_tests[@]}"; do
            echo -e "  - $test"
        done
    fi

    ECHO "YELLOW_BOLD" "[Time] Total time: ${duration}s"
}

# 处理单个测试目录的 QEMU 测试
process_qemu_test_directory() {
    local test_dir="$1"
    local success_count_ref="$2"
    local failure_count_ref="$3"
    local failed_tests_ref="$4"
    
    local dir_name=$(basename "$test_dir")
    NOTE "----- $dir_name ----"
    
    if [ -d "$test_dir" ]; then
        for sy_file in "$test_dir"/*.sy; do
            if [ -f "$sy_file" ]; then
                local test_name
                test_name=$(basename "$sy_file")
                
                # 检查是否需要跳过测试
                if [[ " ${SKIP_TESTS[@]} " =~ " $test_name " ]]; then
                    WARN "$test_name -- Skipped"
                    continue
                fi
                
                # QEMU 批量测试模式：需要.out文件才执行
                local out_file="${sy_file%.sy}.out"
                if [ ! -f "$out_file" ]; then
                    WARN "$test_name -- Skipped -- no .out"
                    continue
                fi
                
                # 执行 QEMU 测试
                if run_qemu_single "$sy_file"; then
                    local base="${sy_file%.sy}"
                    local timing_file="$base.timing"
                    
                    # 读取 timing 信息
                    if [ -f "$timing_file" ]; then
                        local timing_line=$(cat "$timing_file")
                        rm -f "$timing_file"  # 清理临时文件
                        SUCCESS "$test_name -- $timing_line"
                    else
                        SUCCESS "$test_name -- No timing information found"
                    fi
                    eval "$success_count_ref=\$((\$$success_count_ref + 1))"
                else
                    ERROR "$test_name -- QEMU test failed! See ${sy_file%.sy}.log"
                    eval "$failure_count_ref=\$((\$$failure_count_ref + 1))"
                    eval "$failed_tests_ref+=('$test_name')"
                fi
            fi
        done
    fi
}

qemu_tests() {
    local start_time=$SECONDS
    RUSTFLAGS="-A warnings" cargo build --release || { ERROR "编译器构建失败"; return 1; }

    local success_count=0
    local failure_count=0
    local failed_tests=()

    # 遍历所有测试目录
    for test_dir in "${TEST_DIRS[@]}"; do
        process_qemu_test_directory "$test_dir" "success_count" "failure_count" "failed_tests"
    done

    local duration=$((SECONDS - start_time))
    local skipped_count=${#SKIP_TESTS[@]}
    # 打印总结报告
    echo
    ECHO "BLUE_BOLD" "QEMU TEST SUMMARY:"
    SUCCESS "Passed: $success_count"
    WARN "Skipped: $skipped_count"
    ERROR "Failed: $failure_count"
    echo ""

    if [ $skipped_count -gt 0 ]; then
        WARN "Skipped tests:"
        for test in "${SKIP_TESTS[@]}"; do
            echo -e "  - $test"
        done
    fi

    if [ $failure_count -gt 0 ]; then
        ERROR "Failed tests:"
        for test in "${failed_tests[@]}"; do
            echo -e "  - $test"
        done
    fi

    ECHO "YELLOW_BOLD" "[Time] Total time: ${duration}s"
}

clean_tests() {
    # 清理所有测试目录
    for test_dir in "${TEST_DIRS[@]}"; do
        if [ -d "$test_dir" ]; then
            rm -f *.ast *.log *.ssa *.interp *.lir
        fi
    done
    SUCCESS "Cleaned up all!"
}

main() {
    case "$1" in
        "clean")
            clean_tests
            return $?
            ;;
        "parser")
            compile_tests "parser"
            return $?
            ;;
        "ssa")
            compile_tests "ssa"
            return $?
            ;;
        "interp")
            compile_tests "interp"
            return $?
            ;;
        "asm")
            compile_tests "asm"
            return $?
            ;;
        "run-file")
            local stage=${2:-"parser"}
            
            # 特殊处理 qemu 模式
            if [ "$stage" = "qemu" ]; then
                local sy_file="${3:-$TESTS_DIR/functional/00_main.sy}"
                
                # 检查文件是否存在
                if [ ! -f "$sy_file" ]; then
                    ERROR "File not found: $sy_file"
                    return 1
                fi
                
                local base="${sy_file%.sy}"
                local log_file="$base.log"
                
                # 清空日志文件
                > "$log_file"
                
                # 执行 QEMU 测试（包含完整的 asm 编译）
                if run_qemu_single "$sy_file"; then
                    SUCCESS "QEMU test passed: $sy_file"
                    SUCCESS "Output saved to: $base.OUT"
                    SUCCESS "Log saved to: $log_file"
                    return 0
                else
                    ERROR "QEMU test failed! See $log_file for details"
                    return 1
                fi
            fi
            
            # 处理其他编译阶段
            local sy_file="${3:-$TESTS_DIR/functional/00_main.sy}"
            local suffix=$(suffix_by "$stage")

            local target_file="${sy_file%.sy}.${suffix}"
            local log_file="${sy_file%.sy}.log"
            
            # 构造解释器参数
            local extra_args=""
            if [ "$stage" = "interp" ]; then
                local in_file="${sy_file%.sy}.in"
                local out_file="${sy_file%.sy}.out"
                if [ -f "$in_file" ]; then
                    extra_args="$extra_args --target-stdin $in_file"
                fi
                if [ -f "$out_file" ]; then
                    extra_args="$extra_args --target-stdout $out_file"
                fi
            fi
            
            if RUST_BACKTRACE=1 RUST_LOG=debug RUSTFLAGS="-A warnings" cargo run -- "$sy_file" --stage "$stage" -o "$target_file" $extra_args > "$log_file" 2>&1; then
                SUCCESS "Run successful: $target_file (log saved to $log_file)"
                return 0
            else
                ERROR "Run failed: see $log_file for details"
                return 1
            fi
            ;;
        # 生成文件并同时输出到终端
        "run")
            local stage=${2:-"parser"}
            
            # 特殊处理 qemu 模式
            if [ "$stage" = "qemu" ]; then
                local sy_file="${3:-$TESTS_DIR/functional/00_main.sy}"
                local base="${sy_file%.sy}"
                local log_file="$base.log"
                
                # 清空日志文件
                > "$log_file"
                
                # 执行 QEMU 测试（包含完整的 asm 编译）
                echo "Running QEMU test for $sy_file..."
                if run_qemu_single "$sy_file"; then
                    # 显示汇编代码
                    echo "========== Assembly =========="
                    cat "$base.S"
                    echo ""
                    # 显示程序输出
                    echo "========== Program Output =========="
                    if [ -f "$base.out" ]; then
                        # 比较输出
                        if diff -q "$base.out" "$base.OUT" > /dev/null 2>&1; then
                            SUCCESS "✓ 输出校验通过"
                        else
                            ERROR "输出不匹配:"
                            diff -u "$base.out" "$base.OUT"
                        fi
                    else
                        # 没有期望输出文件，直接显示输出
                        cat "$base.OUT"
                    fi
                    
                    # 显示生成的文件和时间信息
                    echo "========== Generated Files =========="
                    echo "Assembly: $base.S"
                    echo "Executable: $base.E"
                    echo "Output: $base.OUT"
                    echo "Log: $log_file"
                    
                    # 读取并显示 timing 信息
                    if [ -f "$base.timing" ]; then
                        local timing_line=$(cat "$base.timing")
                        rm -f "$base.timing"
                        SUCCESS "$(basename "$sy_file") -- $timing_line"
                    fi
                    
                    return 0
                else
                    ERROR "QEMU test failed! See $log_file for details"
                    
                    # 检查日志中的成功标志来判断在哪个阶段失败
                    if grep -q "Assembly generation succeeded" "$log_file"; then
                        # 汇编生成成功
                        if grep -q "Linking succeeded" "$log_file"; then
                            # 编译链接都成功，但执行失败，显示汇编
                            echo "========== Generated Assembly (execution failed) =========="
                            cat "$base.S"
                            echo ""
                        else
                            # 汇编生成成功但链接失败，显示汇编
                            echo "========== Generated Assembly (linking failed) =========="
                            cat "$base.S"
                            echo ""
                        fi
                    else
                        # 汇编生成失败
                        echo "Compilation failed at assembly generation stage."
                    fi

                    echo "========== Last 20 lines of log =========="
                    tail -n 20 "$log_file"
                    echo "=========================================="
                    return 1
                fi
            fi
            
            # 处理其他编译阶段
            local sy_file="${3:-$TESTS_DIR/functional/00_main.sy}"
            local suffix=$(suffix_by "$stage")

            local target_file="${sy_file%.sy}.${suffix}"
            local log_file="${sy_file%.sy}.log"
            
            # 构造解释器参数（run模式不需要输出文件，直接输出到stdout）
            local extra_args=""
            if [ "$stage" = "interp" ]; then
                local in_file="${sy_file%.sy}.in"
                local out_file="${sy_file%.sy}.out"
                if [ -f "$in_file" ]; then
                    extra_args="$extra_args --target-stdin $in_file"
                fi
                if [ -f "$out_file" ]; then
                    extra_args="$extra_args --target-stdout $out_file"
                fi
                # 解释器交互模式也输出到.interp文件
                target_file="${sy_file%.sy}.interp"
            fi
            
            # 清空日志文件并运行编译器，将日志输出到文件和终端
            > "$log_file"
            RUST_BACKTRACE=1 RUST_LOG=debug RUSTFLAGS="-A warnings" cargo run -- "$sy_file" --stage "$stage" -o "$target_file" $extra_args 2>&1 | tee "$log_file"
            # 获取管道中第一个命令的退出码
            local compile_exit_code=${PIPESTATUS[0]}
            
            if [ $compile_exit_code -eq 0 ]; then
                # 编译成功后，输出目标文件内容到终端
                echo "========== Output Content =========="
                cat "$target_file"
                echo "===================================="
                SUCCESS "Run successful: $sy_file (output saved to $target_file, log saved to $log_file)"
                return 0
            else
                ERROR "Run failed (log saved to $log_file)"
                return 1
            fi
            ;;
        "qemu")
            qemu_tests
            return $?
            ;;
        *)
            compile_tests
            return $?
            ;;
    esac
}

main "$@"
exit $?
