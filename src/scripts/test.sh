#!/usr/bin/env bash
set -uo pipefail

usage() {
    cat <<'EOF'
Usage:
  src/scripts/test.sh test/<target-dir>
  src/scripts/test.sh test/<target-dir>/<case>.sy

For each .sy case:
  - Output is saved to src/test_output/<target-dir>/out/<case>.out.
  - Non-empty stderr is saved to src/test_output/<target-dir>/err/<case>.err.
  - Non-empty diff is saved to src/test_output/<target-dir>/diff/<case>.diff.
  - AST is always dumped to src/test_output/<target-dir>/ast/<case>.ast.
  - IR is always dumped to src/test_output/<target-dir>/ir/<case>.ir.
  - Default timeout is 25s; override with RUN_SY_TIMEOUT.
EOF
}

if [[ $# -ne 1 ]]; then
    usage
    exit 1
fi

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
src_root=$(cd "$script_dir/.." && pwd)
repo_root=$(cd "$src_root/.." && pwd)
tests_root="$repo_root/test"

target=$1
compiler="$src_root/build/compiler"
case_timeout=${RUN_SY_TIMEOUT:-25s}

if [[ ! -x "$compiler" ]]; then
    echo "compiler not found: $compiler" >&2
    echo "try: cmake --build src/build" >&2
    exit 1
fi

if [[ ! -d "$target" && ! -f "$target" ]]; then
    echo "test target not found: $target" >&2
    exit 1
fi

target_abs=$(realpath "$target")
if [[ "$target_abs" != "$tests_root" && "$target_abs" != "$tests_root"/* ]]; then
    echo "test target must be inside: $tests_root" >&2
    exit 1
fi

if [[ -f "$target_abs" && "$target_abs" != *.sy ]]; then
    echo "single-file target must be a .sy file: $target" >&2
    exit 1
fi

suite_dir=$target_abs
if [[ -f "$target_abs" ]]; then
    suite_dir=$(dirname "$target_abs")
fi

suite_rel=$(realpath --relative-to="$tests_root" "$suite_dir")
out_root="$src_root/test_output/$suite_rel"
out_dir="$out_root/out"
err_dir="$out_root/err"
diff_dir="$out_root/diff"
ast_dir="$out_root/ast"
ir_dir="$out_root/ir"

mkdir -p "$out_dir" "$err_dir" "$diff_dir" "$ast_dir" "$ir_dir"

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

run_with_input() {
    local sy_file=$1
    local out_file=$2
    local ast_file=$3
    local ir_file=$4
    local err_file=$5

    local in_file="${sy_file%.sy}.in"
    local input_file=/dev/null
    if [[ -f "$in_file" ]]; then
        input_file=$in_file
    fi

    timeout "$case_timeout" "$compiler" "$sy_file" "$out_file" --dump-ast "$ast_file" --dump-ir "$ir_file" \
        < "$input_file" > /dev/null 2>> "$err_file"
}

cleanup_err_file() {
    local err_file=$1
    if [[ -s "$err_file" ]]; then
        return
    fi
    rm -f "$err_file"
}

run_case() {
    local sy_file=$1
    local base
    base=$(case_name "$sy_file")
    local expected_file="${sy_file%.sy}.out"
    local actual_file="$out_dir/${base}.out"
    local err_file="$err_dir/${base}.err"
    local diff_file="$diff_dir/${base}.diff"
    local ast_file="$ast_dir/${base}.ast"
    local ir_file="$ir_dir/${base}.ir"
    local status

    total=$((total + 1))
    : > "$actual_file"
    : > "$err_file"
    : > "$diff_file"
    rm -f "$ast_file"
    rm -f "$ir_file"

    run_with_input "$sy_file" "$actual_file" "$ast_file" "$ir_file" "$err_file"
    status=$?

    if [[ $status -eq 124 ]]; then
        echo "TIMEOUT(${case_timeout}) $sy_file"
        echo "timeout after ${case_timeout}: $sy_file" > "$err_file"
        echo "  err:  $err_file"
        echo "  out:  $actual_file"
        echo "  ast:  $ast_file"
        echo "  ir:   $ir_file"
        timed_out=$((timed_out + 1))
        exit_code=1
        return
    fi

    if [[ $status -ne 0 ]]; then
        echo "FAIL(run) $sy_file"
        echo "  out:  $actual_file"
        echo "  err:  $err_file"
        echo "  ast:  $ast_file"
        echo "  ir:   $ir_file"
        failed=$((failed + 1))
        exit_code=1
        return
    fi

    if [[ ! -f "$expected_file" ]]; then
        echo "SKIP(no .out) $sy_file"
        echo "missing expected file: $expected_file" > "$diff_file"
        echo "  out:  $actual_file"
        if [[ -s "$err_file" ]]; then
            echo "  err:  $err_file"
        else
            cleanup_err_file "$err_file"
        fi
        echo "  ast:  $ast_file"
        echo "  ir:   $ir_file"
        skipped=$((skipped + 1))
        return
    fi

    if diff -u \
        <(awk '{ sub(/\r$/, ""); print }' "$expected_file") \
        <(awk '{ sub(/\r$/, ""); print }' "$actual_file") \
        > "$diff_file"; then
        echo "PASS $sy_file"
        rm -f "$diff_file"
        cleanup_err_file "$err_file"
        passed=$((passed + 1))
    else
        echo "FAIL(diff) $sy_file"
        echo "  out:  $actual_file"
        if [[ -s "$err_file" ]]; then
            echo "  err:  $err_file"
        else
            cleanup_err_file "$err_file"
        fi
        echo "  diff: $diff_file"
        echo "  ast:  $ast_file"
        echo "  ir:   $ir_file"
        failed=$((failed + 1))
        exit_code=1
    fi
}

if [[ -d "$target_abs" ]]; then
    while IFS= read -r sy_file; do
        run_case "$sy_file"
    done < <(find "$target_abs" -maxdepth 1 -type f -name '*.sy' | sort)
else
    run_case "$target_abs"
fi

echo
echo "total=$total passed=$passed timeout=$timed_out failed=$failed skipped=$skipped"

if [[ $total -eq 0 ]]; then
    echo "no .sy files found: $target" >&2
    exit_code=1
fi

exit "$exit_code"
