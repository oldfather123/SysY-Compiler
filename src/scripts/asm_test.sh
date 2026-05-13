#!/usr/bin/env bash
if [ -z "${BASH_VERSION:-}" ]; then
    exec bash "$0" "$@"
fi

set -uo pipefail

readonly DEFAULT_TIMEOUT=25s
readonly RV_GCC="${RV_GCC:-riscv64-linux-gnu-gcc}"
readonly RV_ARCH="${RV_ARCH:-rv64imafdc}"
readonly RV_ABI="${RV_ABI:-lp64d}"
readonly QEMU="${QEMU:-qemu-riscv64-static}"
readonly QEMU_STACK_SIZE_BYTES="${QEMU_STACK_SIZE:-67108864}"

usage() {
    cat <<'EOF'
Usage:
  src/scripts/asm_test.sh test/<target-dir>
  src/scripts/asm_test.sh test/<target-dir>/<case>.sy

Cross-compiles each .sy to RISC-V assembly, links with sylib, runs via QEMU,
and compares output against expected .out files.

Environment variables:
  RV_GCC          RISC-V GCC (default: riscv64-linux-gnu-gcc)
  RV_ARCH         march flag (default: rv64imafdc)
  RV_ABI          mabi flag (default: lp64d)
  QEMU            QEMU user binary (default: qemu-riscv64-static)
  QEMU_STACK_SIZE guest stack size passed to QEMU -s (default: 67108864)
  RUN_SY_TIMEOUT  timeout for each case (default: 25s)

Prerequisites:
  sudo apt-get install -y qemu-user-static
EOF
}

die() {
    echo "$*" >&2
    exit 1
}

require_command() {
    local command_name=$1
    if ! command -v "$command_name" >/dev/null 2>&1; then
        die "command not found: $command_name"
    fi
}

parse_args() {
    if [[ $# -ne 1 ]]; then
        usage
        exit 1
    fi
    target=$1
}

readonly script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
readonly src_root=$(cd "$script_dir/.." && pwd)
readonly repo_root=$(cd "$src_root/.." && pwd)
readonly tests_root="$repo_root/test"
readonly compiler="$src_root/build/compiler"
readonly sylib_s="$src_root/asm/sylib.s"
readonly case_timeout="${RUN_SY_TIMEOUT:-$DEFAULT_TIMEOUT}"

target=
target_abs=
suite_dir=
suite_rel=
out_root=

parse_args "$@"

ensure_dependencies() {
    local cmd
    for cmd in awk basename cmake diff dirname find mkdir od realpath rm sort timeout tr; do
        require_command "$cmd"
    done
    require_command "$RV_GCC"
    require_command "$QEMU"
}

ensure_compiler() {
    if [[ ! -d "$src_root/build" ]]; then
        echo "build directory not found: $src_root/build" >&2
        die "try: cmake -S src -B src/build"
    fi

    echo "building compiler..."
    if ! cmake --build "$src_root/build"; then
        die "failed to build compiler"
    fi

    if [[ ! -x "$compiler" ]]; then
        die "compiler not found after build: $compiler"
    fi
}

ensure_sylib() {
    if [[ ! -f "$sylib_s" ]]; then
        die "sylib.s not found: $sylib_s"
    fi
}

resolve_target() {
    if [[ ! -d "$target" && ! -f "$target" ]]; then
        die "test target not found: $target"
    fi

    target_abs=$(realpath "$target")
    if [[ "$target_abs" != "$tests_root" && "$target_abs" != "$tests_root"/* ]]; then
        die "test target must be inside: $tests_root"
    fi

    if [[ -f "$target_abs" && "$target_abs" != *.sy ]]; then
        die "single-file target must be a .sy file: $target"
    fi

    suite_dir=$target_abs
    if [[ -f "$target_abs" ]]; then
        suite_dir=$(dirname "$target_abs")
    fi

    suite_rel=$(realpath --relative-to="$tests_root" "$suite_dir")
    out_root="$src_root/test_output/$suite_rel/asm"
}

ensure_dependencies
ensure_compiler
ensure_sylib
resolve_target

asm_dir="$out_root/asm"
exe_dir="$out_root/exe"
actual_dir="$out_root/out"
err_dir="$out_root/err"
diff_dir="$out_root/diff"
mkdir -p "$asm_dir" "$exe_dir" "$actual_dir" "$err_dir" "$diff_dir"

total=0
passed=0
timed_out=0
failed=0
skipped=0
exit_code=0

case_name() {
    local sy_file=$1
    basename "${sy_file%.sy}"
}

run_case() {
    local sy_file=$1
    local base expected_file asm_file exe_file actual_file err_file diff_file
    local status

    base=$(case_name "$sy_file")
    expected_file="${sy_file%.sy}.out"
    asm_file="$asm_dir/${base}.s"
    exe_file="$exe_dir/${base}"
    actual_file="$actual_dir/${base}.out"
    err_file="$err_dir/${base}.err"
    diff_file="$diff_dir/${base}.diff"

    total=$((total + 1))

    # Clear previous outputs.
    : > "$actual_file"
    : > "$err_file"
    : > "$diff_file"

    local in_file="${sy_file%.sy}.in"
    local input_file=/dev/null
    if [[ -f "$in_file" ]]; then
        input_file=$in_file
    fi

    # Step 1: Compile .sy -> .s via our compiler.
    if ! timeout "$case_timeout" "$compiler" "$sy_file" /dev/null --dump-asm "$asm_file" \
        < /dev/null 2>> "$err_file"; then
        echo "FAIL(compile) $sy_file"
        failed=$((failed + 1))
        exit_code=1
        return
    fi

    # Step 2: Cross-compile .s + sylib.s -> executable.
    if ! "$RV_GCC" -march="$RV_ARCH" -mabi="$RV_ABI" -static \
        -o "$exe_file" "$asm_file" "$sylib_s" 2>> "$err_file"; then
        echo "FAIL(link) $sy_file"
        failed=$((failed + 1))
        exit_code=1
        return
    fi

    # Step 3: Run via QEMU.
    timeout "$case_timeout" "$QEMU" -s "$QEMU_STACK_SIZE_BYTES" "$exe_file" \
        < "$input_file" > "$actual_file" 2>> "$err_file"
    status=$?

    # Append return code to output (same format as interpreter).
    local ret_code=$((status & 255))
    if [[ -s "$actual_file" ]] && \
        [[ "$(tail -c 1 "$actual_file" | od -An -tx1 | tr -d ' \n')" != "0a" ]]; then
        echo >> "$actual_file"
    fi
    echo "$ret_code" >> "$actual_file"

    if [[ $status -eq 124 ]]; then
        echo "TIMEOUT(${case_timeout}) $sy_file"
        timed_out=$((timed_out + 1))
        exit_code=1
        return
    fi

    if [[ ! -f "$expected_file" ]]; then
        echo "SKIP(no .out) $sy_file"
        skipped=$((skipped + 1))
        return
    fi

    # Step 4: Compare output.
    if ! diff -u \
        <(awk '{ sub(/\r$/, ""); print }' "$expected_file") \
        <(awk '{ sub(/\r$/, ""); print }' "$actual_file") \
        > "$diff_file" 2>&1; then
        echo "FAIL(diff) $sy_file"
        echo "  expected: $expected_file"
        echo "  actual:   $actual_file"
        echo "  diff:     $diff_file"
        failed=$((failed + 1))
        exit_code=1
        return
    fi

    echo "PASS $sy_file"
    passed=$((passed + 1))
}

run_target() {
    local sy_file

    if [[ -d "$target_abs" ]]; then
        while IFS= read -r sy_file; do
            run_case "$sy_file"
        done < <(find "$target_abs" -maxdepth 1 -type f -name '*.sy' | sort)
        return
    fi

    run_case "$target_abs"
}

print_summary() {
    echo
    echo "total=$total passed=$passed timeout=$timed_out failed=$failed skipped=$skipped"
}

run_target
print_summary

if [[ $total -eq 0 ]]; then
    echo "no .sy files found: $target" >&2
    exit_code=1
fi

exit "$exit_code"
