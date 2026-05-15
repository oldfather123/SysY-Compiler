#!/usr/bin/env bash
if [ -z "${BASH_VERSION:-}" ]; then
    exec bash "$0" "$@"
fi

set -uo pipefail

readonly DEFAULT_TIMEOUT=300s

usage() {
    cat <<'EOF'
Usage:
  src/scripts/ast_diff.sh test/<target-dir>
  src/scripts/ast_diff.sh test/<target-dir>/<case>.sy
  src/scripts/ast_diff.sh test/<target-dir> true
  src/scripts/ast_diff.sh test/<target-dir>/<case>.sy true

For each .sy case:
  - Clang AST is saved to src/test_output/<target-dir>/ast_clang/<case>.ast.
  - Compiler AST is saved to src/test_output/<target-dir>/ast/<case>.ast.
  - AST diff is saved to src/test_output/<target-dir>/ast_diff/<case>.diff.

By default the diff is normalized for semantic-structure comparison.
Pass true as the second argument to diff the raw generated AST files.

Environment variables:
  AST_DIFF_TIMEOUT   timeout for each command (default: 300s)
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
    raw_diff=false

    if [[ $# -eq 2 && $2 == true ]]; then
        raw_diff=true
        set -- "$1"
    fi

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
<<<<<<< HEAD
readonly compiler="$src_root/build/compiler"
=======
readonly compiler="$repo_root/compiler"
>>>>>>> bfba3aa6ee1bb3392cc33b1f19553b1007093821
readonly case_timeout="${AST_DIFF_TIMEOUT:-$DEFAULT_TIMEOUT}"

target=
target_abs=
suite_dir=
suite_rel=
out_root=
clang_ast_dir=
compiler_ast_dir=
diff_dir=

parse_args "$@"

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
    clang_ast_dir="$out_root/ast_clang"
    compiler_ast_dir="$out_root/ast"
    diff_dir="$out_root/ast_diff"
}

ensure_dependencies() {
    local command_name
    for command_name in awk clang cmake diff find mktemp realpath sed sort timeout; do
        require_command "$command_name"
    done
}

ensure_dependencies
ensure_compiler

if [[ ! -x "$compiler" ]]; then
    die "compiler not found: $compiler"
fi

resolve_target
mkdir -p "$clang_ast_dir" "$compiler_ast_dir" "$diff_dir"

total=0
same=0
different=0
timed_out=0
exit_code=0

write_sysy_prelude() {
    cat <<'EOF'
int getint(void), getch(void), getarray(int a[]);
float getfloat(void);
int getfarray(float a[]);

void putint(int a), putch(int a), putarray(int n, int a[]);
void putfloat(float a);
void putfarray(int n, float a[]);
void putf(char a[], ...);

void _sysy_starttime(int lineno);
void _sysy_stoptime(int lineno);

#define starttime() _sysy_starttime(__LINE__)
#define stoptime() _sysy_stoptime(__LINE__)
EOF
}

strip_clang_prelude_decls() {
    awk '
    function indent_len(line,    i, c) {
        for (i = 1; i <= length(line); ++i) {
            c = substr(line, i, 1);
            if (c != " " && c != "|" && c != "`" && c != "-") {
                return i - 1;
            }
        }
        return length(line);
    }

    function is_prelude_function(line) {
        if (line !~ /FunctionDecl/) {
            return 0;
        }

        return index(line, " getint '\''") ||
               index(line, " getch '\''") ||
               index(line, " getarray '\''") ||
               index(line, " getfloat '\''") ||
               index(line, " getfarray '\''") ||
               index(line, " putint '\''") ||
               index(line, " putch '\''") ||
               index(line, " putarray '\''") ||
               index(line, " putfloat '\''") ||
               index(line, " putfarray '\''") ||
               index(line, " putf '\''") ||
               index(line, " _sysy_starttime '\''") ||
               index(line, " _sysy_stoptime '\''");
    }

    {
        indent = indent_len($0);

        if (skip_decl && indent > skip_indent) {
            next;
        }
        skip_decl = 0;

        if (is_prelude_function($0)) {
            skip_decl = 1;
            skip_indent = indent;
            next;
        }

        print;
    }
    '
}

run_clang_ast() {
    local sy_file=$1
    local ast_file=$2
    local tmp_source
    local status

    tmp_source=$(mktemp /tmp/ast_diff_clang_XXXXXX.c)
    trap 'rm -f "$tmp_source"' RETURN

    {
        write_sysy_prelude
        printf '#line 1 "%s"\n' "$sy_file"
        cat "$sy_file"
    } > "$tmp_source"

    timeout "$case_timeout" \
        clang \
        -x c \
        -std=c11 \
        -fsyntax-only \
        -fno-builtin \
        -fno-color-diagnostics \
        -w \
        -Xclang -ast-dump \
        "$tmp_source" \
        | sed -E 's/\x1B\[[0-9;]*m//g' \
        | strip_clang_prelude_decls \
        > "$ast_file"
    status=$?

    trap - RETURN
    rm -f "$tmp_source"
    return "$status"
}

run_compiler_ast() {
    local sy_file=$1
    local ast_file=$2
    local in_file="${sy_file%.sy}.in"
    local input_file=/dev/null

    if [[ -f "$in_file" ]]; then
        input_file=$in_file
    fi

    timeout "$case_timeout" \
        "$compiler" \
        "$sy_file" \
        /dev/null \
        --dump-ast "$ast_file" \
        < "$input_file" > /dev/null
}

normalize_clang_ast() {
    local ast_file=$1

    awk '
    function strip_location_brackets(line,    changed) {
        changed = 1;
        while (changed) {
            changed = 0;
            if (gsub(/<<invalid sloc>>/, "", line)) {
                changed = 1;
            }
            if (gsub(/<invalid sloc>/, "", line)) {
                changed = 1;
            }
            if (gsub(/<col:[0-9]+>/, "", line)) {
                changed = 1;
            }
            if (gsub(/<line:[0-9]+:[0-9]+>/, "", line)) {
                changed = 1;
            }
            if (gsub(/<col:[0-9]+, line:[0-9]+:[0-9]+>/, "", line)) {
                changed = 1;
            }
            if (gsub(/<line:[0-9]+:[0-9]+, col:[0-9]+>/, "", line)) {
                changed = 1;
            }
            if (gsub(/<[^<>]*:[0-9]+:[0-9]+, line:[0-9]+:[0-9]+>/, "", line)) {
                changed = 1;
            }
        }
        return line;
    }

    function indent_len(line,    i, c) {
        for (i = 1; i <= length(line); ++i) {
            c = substr(line, i, 1);
            if (c != " " && c != "|" && c != "`" && c != "-") {
                return i - 1;
            }
        }
        return length(line);
    }

    function strip_decl_state_flags(line) {
        if (line ~ /VarDecl[ ]+(used|referenced)[ ]+/ &&
            line !~ /VarDecl[ ]+(used|referenced)[ ]+\047/) {
            sub(/VarDecl[ ]+(used|referenced)[ ]+/, "VarDecl ", line);
        }
        if (line ~ /ParmVarDecl[ ]+(used|referenced)[ ]+/ &&
            line !~ /ParmVarDecl[ ]+(used|referenced)[ ]+\047/) {
            sub(/ParmVarDecl[ ]+(used|referenced)[ ]+/, "ParmVarDecl ", line);
        }
        if (line ~ /FunctionDecl[ ]+(used|referenced)[ ]+/ &&
            line !~ /FunctionDecl[ ]+(used|referenced)[ ]+\047/) {
            sub(/FunctionDecl[ ]+(used|referenced)[ ]+/, "FunctionDecl ", line);
        }
        return line;
    }

    function function_callee_name(line,    name) {
        if (line !~ /DeclRefExpr/ || line !~ / Function \047/) {
            return "";
        }

        name = line;
        sub(/^.* Function \047/, "", name);
        sub(/\047.*$/, "", name);
        return name;
    }

    function node_pos(line,    i, c) {
        for (i = 1; i <= length(line); ++i) {
            c = substr(line, i, 1);
            if (c ~ /[A-Za-z_]/) {
                return i;
            }
        }
        return 1;
    }

    function replace_node_text(line, replacement) {
        return substr(line, 1, node_pos(line) - 1) replacement;
    }

    function call_expr_with_name(line, name) {
        if (line ~ /\047[^ \047]+\047[ ]+\047[^\047]+\047$/) {
            return line;
        }
        return line " \047" name "\047";
    }

    function skip_function_callee(call_index,    j, name) {
        if (call_index + 1 > line_count ||
            lines[call_index + 1] !~ /ImplicitCastExpr/ ||
            lines[call_index + 1] !~ /<FunctionToPointerDecay>/) {
            return 0;
        }

        for (j = call_index + 2; j <= line_count && j <= call_index + 5; ++j) {
            name = function_callee_name(lines[j]);
            if (name != "") {
                lines[call_index] = call_expr_with_name(lines[call_index], name);
                return j - call_index;
            }
        }
        return 0;
    }

    function collect_const_int(line, next_line,    name, value) {
        if (line !~ /VarDecl / || line !~ /\047const int\047/) {
            return;
        }
        if (next_line !~ /IntegerLiteral \047int\047 -?[0-9]+/) {
            return;
        }

        name = line;
        sub(/^.*VarDecl /, "", name);
        sub(/ .*/, "", name);

        value = next_line;
        sub(/^.*IntegerLiteral \047int\047 /, "", value);
        sub(/ .*/, "", value);
        const_int[name] = value + 0;
    }

    function tokenize_expr(expr,    i, c, text) {
        delete tokens;
        token_count = 0;
        for (i = 1; i <= length(expr); ) {
            c = substr(expr, i, 1);
            if (c ~ /[[:space:]]/) {
                ++i;
                continue;
            }
            if (c ~ /[0-9]/) {
                text = c;
                ++i;
                while (i <= length(expr) && substr(expr, i, 1) ~ /[0-9]/) {
                    text = text substr(expr, i, 1);
                    ++i;
                }
                tokens[++token_count] = text;
                continue;
            }
            if (c ~ /[A-Za-z_]/) {
                text = c;
                ++i;
                while (i <= length(expr) && substr(expr, i, 1) ~ /[A-Za-z0-9_]/) {
                    text = text substr(expr, i, 1);
                    ++i;
                }
                if (!(text in const_int)) {
                    eval_ok = 0;
                    return;
                }
                tokens[++token_count] = const_int[text];
                continue;
            }
            if (c ~ /[()+\-*\/%]/) {
                tokens[++token_count] = c;
                ++i;
                continue;
            }
            eval_ok = 0;
            return;
        }
    }

    function parse_factor(    token, value) {
        token = tokens[token_pos];
        if (token == "+") {
            ++token_pos;
            return parse_factor();
        }
        if (token == "-") {
            ++token_pos;
            return -parse_factor();
        }
        if (token == "(") {
            ++token_pos;
            value = parse_expr();
            if (tokens[token_pos] != ")") {
                eval_ok = 0;
                return 0;
            }
            ++token_pos;
            return value;
        }
        if (token ~ /^-?[0-9]+$/) {
            ++token_pos;
            return token + 0;
        }
        eval_ok = 0;
        return 0;
    }

    function parse_term(    value, rhs, op) {
        value = parse_factor();
        while (eval_ok && token_pos <= token_count &&
               (tokens[token_pos] == "*" || tokens[token_pos] == "/" ||
                tokens[token_pos] == "%")) {
            op = tokens[token_pos++];
            rhs = parse_factor();
            if (op == "*") {
                value *= rhs;
            } else if (op == "/") {
                if (rhs == 0) {
                    eval_ok = 0;
                    return 0;
                }
                value = int(value / rhs);
            } else {
                if (rhs == 0) {
                    eval_ok = 0;
                    return 0;
                }
                value %= rhs;
            }
        }
        return value;
    }

    function parse_expr(    value, rhs, op) {
        value = parse_term();
        while (eval_ok && token_pos <= token_count &&
               (tokens[token_pos] == "+" || tokens[token_pos] == "-")) {
            op = tokens[token_pos++];
            rhs = parse_term();
            if (op == "+") {
                value += rhs;
            } else {
                value -= rhs;
            }
        }
        return value;
    }

    function eval_dim_expr(expr,    value) {
        eval_ok = 1;
        tokenize_expr(expr);
        if (!eval_ok || token_count == 0) {
            return expr;
        }
        token_pos = 1;
        value = parse_expr();
        if (!eval_ok || token_pos <= token_count) {
            return expr;
        }
        return value;
    }

    function normalize_array_dims(line,    result, rest, start, end, expr, value) {
        result = "";
        rest = line;
        while (match(rest, /\[[^][]+\]/)) {
            start = RSTART;
            end = RSTART + RLENGTH - 1;
            expr = substr(rest, start + 1, RLENGTH - 2);
            value = eval_dim_expr(expr);
            result = result substr(rest, 1, start - 1) "[" value "]";
            rest = substr(rest, end + 1);
        }
        return result rest;
    }

    {
        raw = $0;
        gsub(/\033\[[0-9;]*m/, "", raw);
        indent = indent_len(raw);

        if (skip_decl && indent > skip_indent) {
            next;
        }
        skip_decl = 0;

        if (raw ~ /TypedefDecl/) {
            skip_decl = 1;
            skip_indent = indent;
            next;
        }

        line = raw;
        gsub(/\r$/, "", line);
        line = strip_location_brackets(line);
        gsub(/(^|[[:space:]])0x[0-9a-fA-F]+([[:space:]]|$)/, " ", line);
        gsub(/ line:[0-9]+:[0-9]+/, "", line);
        gsub(/ col:[0-9]+/, "", line);
        gsub(/<[^<>]*,>/, "", line);
        gsub(/<>/, "", line);
        if (line ~ /VarDecl /) {
            gsub(/ cinit/, "", line);
        }
        gsub(/\047double\047/, "\047float\047", line);
        gsub(/  +/, " ", line);
        line = strip_decl_state_flags(line);
        sub(/ $/, "", line);

        if (pending_function_callee) {
            name = function_callee_name(line);
            if (name != "") {
                lines[line_count] = call_expr_with_name(lines[line_count], name);
                pending_function_callee = 0;
                next;
            }
            pending_function_callee = 0;
        }

        if (line ~ /ImplicitCastExpr/ && line ~ /<FunctionToPointerDecay>/ &&
            line_count > 0 && lines[line_count] ~ /CallExpr/) {
            pending_function_callee = 1;
            next;
        }

        lines[++line_count] = line;
    }

    END {
        for (i = 1; i <= line_count; ++i) {
            collect_const_int(lines[i], lines[i + 1]);
        }

        for (i = 1; i <= line_count; ++i) {
            lines[i] = normalize_array_dims(lines[i]);
        }

        for (i = 1; i <= line_count; ++i) {
            line = lines[i];
            if (line ~ /CallExpr \047void\047 \047_sysy_starttime\047/) {
                sub(/\047_sysy_starttime\047/, "\047starttime\047", line);
                print line;
                if (i + 1 <= line_count &&
                    lines[i + 1] ~ /IntegerLiteral .* \047int\047/) {
                    ++i;
                }
                continue;
            }
            if (line ~ /CallExpr \047void\047 \047_sysy_stoptime\047/) {
                sub(/\047_sysy_stoptime\047/, "\047stoptime\047", line);
                print line;
                if (i + 1 <= line_count &&
                    lines[i + 1] ~ /IntegerLiteral .* \047int\047/) {
                    ++i;
                }
                continue;
            }

            if (line ~ /CallExpr/) {
                skipped = skip_function_callee(i);
                if (skipped > 0) {
                    print lines[i];
                    i += skipped;
                    continue;
                }
            }

            if (line ~ /ImplicitCastExpr \047float\047 <FloatingCast>/ &&
                i + 1 <= line_count &&
                lines[i + 1] ~ /\047float\047/ &&
                lines[i + 1] !~ /<IntegralToFloating>/) {
                print replace_node_text(line, substr(lines[i + 1], node_pos(lines[i + 1])));
                ++i;
                continue;
            }

            if (line ~ /ImplicitCastExpr .* <BitCast>/ &&
                i + 1 <= line_count &&
                lines[i + 1] ~ /ImplicitCastExpr .* <(ArrayToPointerDecay|LValueToRValue)>/) {
                print replace_node_text(line, substr(lines[i + 1], node_pos(lines[i + 1])));
                ++i;
                continue;
            }

            print line;
        }
    }
    ' "$ast_file"
}

normalize_compiler_ast() {
    local ast_file=$1

    awk '
    {
        line = $0;
        gsub(/\033\[[0-9;]*m/, "", line);
        gsub(/\r$/, "", line);
        gsub(/ <line:[0-9]+:[0-9]+>/, "", line);
        gsub(/  +/, " ", line);
        sub(/ $/, "", line);
        print line;
    }
    ' "$ast_file"
}

generate_diff() {
    local clang_ast=$1
    local compiler_ast=$2
    local diff_file=$3

    if [[ $raw_diff == true ]]; then
        if diff --color=never -u "$clang_ast" "$compiler_ast" > "$diff_file"; then
            rm -f "$diff_file"
            return 0
        fi

        return 1
    fi

    if diff --color=never -u <(normalize_clang_ast "$clang_ast") \
        <(normalize_compiler_ast "$compiler_ast") > "$diff_file"; then
        rm -f "$diff_file"
        return 0
    fi

    return 1
}

mark_timeout() {
    timed_out=$((timed_out + 1))
    exit_code=1
}

handle_run_status() {
    local tool_name=$1
    local status=$2
    local sy_file=$3

    if [[ $status -eq 0 ]]; then
        return 0
    fi

    if [[ $status -eq 124 ]]; then
        echo "TIMEOUT(${tool_name} ${case_timeout}) $sy_file"
        mark_timeout
        return 1
    fi

    echo "FAIL(${tool_name}) $sy_file"
    exit_code=1
    return 1
}

collect_cases() {
    if [[ -f "$target_abs" ]]; then
        printf '%s\n' "$target_abs"
        return
    fi

    find "$target_abs" -maxdepth 1 -type f -name '*.sy' | sort
}

run_case() {
    local sy_file=$1
    local base
    local clang_ast_file compiler_ast_file diff_file
    local clang_status compiler_status

    base=$(basename "${sy_file%.sy}")
    clang_ast_file="$clang_ast_dir/${base}.ast"
    compiler_ast_file="$compiler_ast_dir/${base}.ast"
    diff_file="$diff_dir/${base}.diff"

    total=$((total + 1))
    rm -f "$clang_ast_file" "$compiler_ast_file" "$diff_file"

    run_clang_ast "$sy_file" "$clang_ast_file"
    clang_status=$?
    if ! handle_run_status clang "$clang_status" "$sy_file"; then
        return
    fi

    run_compiler_ast "$sy_file" "$compiler_ast_file"
    compiler_status=$?
    if ! handle_run_status compiler "$compiler_status" "$sy_file"; then
        return
    fi

    if generate_diff "$clang_ast_file" "$compiler_ast_file" "$diff_file"; then
        echo "SAME $sy_file"
        same=$((same + 1))
    else
        echo "DIFF $sy_file"
        echo "      diff: $diff_file"
        different=$((different + 1))
        exit_code=1
    fi
}

run_all_cases() {
    local sy_file
    local printed_case=false

    while IFS= read -r sy_file; do
        [[ -z "$sy_file" ]] && continue
        if [[ $printed_case == true ]]; then
            echo
        fi
        run_case "$sy_file"
        printed_case=true
    done < <(collect_cases)
}

print_summary() {
    echo
    echo "total=$total same=$same diff=$different timeout=$timed_out"

    if [[ $total -eq 0 ]]; then
        echo "no .sy files found: $target" >&2
        exit_code=1
    fi
}

run_all_cases
print_summary

exit "$exit_code"
