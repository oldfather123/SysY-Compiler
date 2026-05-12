#!/usr/bin/env bash
if [ -z "${BASH_VERSION:-}" ]; then
    exec bash "$0" "$@"
fi

set -uo pipefail

readonly DEFAULT_TIMEOUT=25s

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
<<<<<<< HEAD

Environment variables:
  RUN_SY_TIMEOUT   timeout for each case (default: 25s)
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
readonly case_timeout="${RUN_SY_TIMEOUT:-$DEFAULT_TIMEOUT}"

target=
target_abs=
suite_dir=
suite_rel=
out_root=
out_dir=
err_dir=
diff_dir=
ast_dir=

parse_args "$@"

ensure_dependencies() {
    local command_name
    for command_name in awk basename cmake diff dirname find mkdir realpath rm sort timeout; do
        require_command "$command_name"
    done
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
    out_root="$src_root/test_output/$suite_rel"
    out_dir="$out_root/out"
    err_dir="$out_root/err"
    diff_dir="$out_root/diff"
    ast_dir="$out_root/ast"
}

ensure_dependencies
ensure_compiler
resolve_target

mkdir -p "$out_dir" "$err_dir" "$diff_dir" "$ast_dir"
=======
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
>>>>>>> of666

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

<<<<<<< HEAD
clear_case_outputs() {
    local actual_file=$1
    local err_file=$2
    local diff_file=$3
    local ast_file=$4

    : > "$actual_file"
    : > "$err_file"
    : > "$diff_file"
    rm -f "$ast_file"
}

print_case_paths() {
    local actual_file=$1
    local err_file=$2
    local diff_file=$3
    local ast_file=$4
    local show_err=$5
    local show_diff=$6

    echo "  out:  $actual_file"
    if [[ $show_err == true ]]; then
        echo "  err:  $err_file"
    fi
    if [[ $show_diff == true ]]; then
        echo "  diff: $diff_file"
    fi
    echo "  ast:  $ast_file"
}

cleanup_optional_files() {
    local err_file=$1
    local diff_file=$2

    if [[ -n "$err_file" && ! -s "$err_file" ]]; then
        rm -f "$err_file"
    fi
    if [[ -n "$diff_file" && ! -s "$diff_file" ]]; then
        rm -f "$diff_file"
    fi
}

=======
>>>>>>> of666
run_with_input() {
    local sy_file=$1
    local out_file=$2
    local ast_file=$3
<<<<<<< HEAD
    local err_file=$4
    local in_file="${sy_file%.sy}.in"
    local input_file=/dev/null

=======
    local ir_file=$4
    local err_file=$5

    local in_file="${sy_file%.sy}.in"
    local input_file=/dev/null
>>>>>>> of666
    if [[ -f "$in_file" ]]; then
        input_file=$in_file
    fi

<<<<<<< HEAD
    timeout "$case_timeout" "$compiler" "$sy_file" "$out_file" --dump-ast "$ast_file" \
        < "$input_file" 2>> "$err_file"
}

normalized_diff() {
    local expected_file=$1
    local actual_file=$2
    local diff_file=$3

    diff -u \
        <(awk '{ sub(/\r$/, ""); print }' "$expected_file") \
        <(awk '{ sub(/\r$/, ""); print }' "$actual_file") \
        > "$diff_file"
}

record_timeout() {
    local sy_file=$1
    local actual_file=$2
    local err_file=$3
    local ast_file=$4

    echo "TIMEOUT(${case_timeout}) $sy_file"
    echo "timeout after ${case_timeout}: $sy_file" > "$err_file"
    print_case_paths "$actual_file" "$err_file" "" "$ast_file" true false
    timed_out=$((timed_out + 1))
    exit_code=1
}

record_run_failure() {
    local sy_file=$1
    local actual_file=$2
    local err_file=$3
    local ast_file=$4

    echo "FAIL(run) $sy_file"
    print_case_paths "$actual_file" "$err_file" "" "$ast_file" true false
    failed=$((failed + 1))
    exit_code=1
}

record_missing_expected() {
    local sy_file=$1
    local expected_file=$2
    local actual_file=$3
    local err_file=$4
    local diff_file=$5
    local ast_file=$6
    local show_err=false

    echo "SKIP(no .out) $sy_file"
    echo "missing expected file: $expected_file" > "$diff_file"
    if [[ -s "$err_file" ]]; then
        show_err=true
    fi
    print_case_paths "$actual_file" "$err_file" "" "$ast_file" "$show_err" false
    cleanup_optional_files "$err_file" ""
    skipped=$((skipped + 1))
}

record_diff_failure() {
    local sy_file=$1
    local actual_file=$2
    local err_file=$3
    local diff_file=$4
    local ast_file=$5
    local show_err=false

    echo "FAIL(diff) $sy_file"
    if [[ -s "$err_file" ]]; then
        show_err=true
    fi
    print_case_paths "$actual_file" "$err_file" "$diff_file" "$ast_file" "$show_err" true
    cleanup_optional_files "$err_file" "$diff_file"
    failed=$((failed + 1))
    exit_code=1
}

record_success() {
    local sy_file=$1
    local err_file=$2
    local diff_file=$3

    echo "PASS $sy_file"
    cleanup_optional_files "$err_file" "$diff_file"
    passed=$((passed + 1))
=======
    timeout "$case_timeout" "$compiler" "$sy_file" "$out_file" --dump-ast "$ast_file" --dump-ir "$ir_file" \
        < "$input_file" > /dev/null 2>> "$err_file"
}

cleanup_err_file() {
    local err_file=$1
    if [[ -s "$err_file" ]]; then
        return
    fi
    rm -f "$err_file"
>>>>>>> of666
}

run_case() {
    local sy_file=$1
    local base
<<<<<<< HEAD
    local expected_file
    local actual_file
    local err_file
    local diff_file
    local ast_file
    local status

    base=$(case_name "$sy_file")
    expected_file="${sy_file%.sy}.out"
    actual_file="$out_dir/${base}.out"
    err_file="$err_dir/${base}.err"
    diff_file="$diff_dir/${base}.diff"
    ast_file="$ast_dir/${base}.ast"

    total=$((total + 1))
    clear_case_outputs "$actual_file" "$err_file" "$diff_file" "$ast_file"

    run_with_input "$sy_file" "$actual_file" "$ast_file" "$err_file"
    status=$?

    if [[ $status -eq 124 ]]; then
        record_timeout "$sy_file" "$actual_file" "$err_file" "$ast_file"
=======
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
>>>>>>> of666
        return
    fi

    if [[ $status -ne 0 ]]; then
<<<<<<< HEAD
        record_run_failure "$sy_file" "$actual_file" "$err_file" "$ast_file"
=======
        echo "FAIL(run) $sy_file"
        echo "  out:  $actual_file"
        echo "  err:  $err_file"
        echo "  ast:  $ast_file"
        echo "  ir:   $ir_file"
        failed=$((failed + 1))
        exit_code=1
>>>>>>> of666
        return
    fi

    if [[ ! -f "$expected_file" ]]; then
<<<<<<< HEAD
        record_missing_expected "$sy_file" "$expected_file" "$actual_file" "$err_file" \
            "$diff_file" "$ast_file"
        return
    fi

    if ! normalized_diff "$expected_file" "$actual_file" "$diff_file"; then
        record_diff_failure "$sy_file" "$actual_file" "$err_file" "$diff_file" "$ast_file"
        return
    fi

    record_success "$sy_file" "$err_file" "$diff_file"
}

run_target() {
    local printed_case=false
    local sy_file

    if [[ -d "$target_abs" ]]; then
        while IFS= read -r sy_file; do
            if [[ $printed_case == true ]]; then
                echo
            fi
            run_case "$sy_file"
            printed_case=true
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
=======
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
>>>>>>> of666

if [[ $total -eq 0 ]]; then
    echo "no .sy files found: $target" >&2
    exit_code=1
fi

exit "$exit_code"
