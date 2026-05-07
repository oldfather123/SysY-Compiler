#!/bin/bash

source "$(dirname "${BASH_SOURCE[0]}")/../../scripts_utils/libbash.sh" 2>/dev/null || true

TARGET_FILE="compiler/src/asm/generated/selection.rs"

run_isle() {
    local -n includes=$1

    # rm "$TARGET_FILE"
    cargo run -p islec -- -o "$TARGET_FILE" "${includes[@]}"
    local exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        SUCCESS "Target file: $TARGET_FILE"
    else
        ERROR "Failed exit code: $exit_code"
    fi
    
    return $exit_code
}

main() {
    case "$1" in
        "t")
            declare -a ISLE_INCLUDES=(
                "compiler/src/asm/generated/t-ssa.isle"
                "compiler/src/asm/generated/t-riscv.isle"
                "compiler/src/asm/generated/t-prelude.isle"
                "compiler/src/asm/generated/t-ssa-impl.isle"
            )
            run_isle ISLE_INCLUDES
            return $?
            ;;
        "m")
            declare -a ISLE_INCLUDES=(
            )
            run_isle ISLE_INCLUDES
            return $?
            ;;
        *)
            declare -a ISLE_INCLUDES=()
            while IFS= read -r -d '' file; do
                ISLE_INCLUDES+=("$file")
            done < <(find compiler/src/asm/generated -name "c-*.isle" -print0)

            run_isle ISLE_INCLUDES
            return $?
            ;;
    esac
}

main "$@"
exit $?
