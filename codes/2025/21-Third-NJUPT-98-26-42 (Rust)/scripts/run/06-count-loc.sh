#!/bin/bash
#
# 统计 LycorisRecompile 项目有效代码行数
# 排除测试、文档、临时文件、第三方代码
#

# 获取脚本所在目录并切换到项目根目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/../.." || exit 1

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# 统计函数
count_lines() {
    local pattern="$1"
    local exclude_pattern="$2"
    
    if [ -z "$exclude_pattern" ]; then
        find . -path "$pattern" 2>/dev/null | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}'
    else
        find . -path "$pattern" -not -path "$exclude_pattern" 2>/dev/null | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}'
    fi
}

# 清屏并打印标题
clear
echo -e "${BOLD}${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}${CYAN}║     LycorisRecompile 有效代码行数统计 (Lines of Code)      ║${NC}"
echo -e "${BOLD}${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
echo

# 统计各模块
echo -e "${BOLD}${YELLOW}【编译器核心模块】${NC}"
echo

# Rust 代码统计
lexer_lines=$(find compiler/src/lexer -name "*.rs" 2>/dev/null | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
parser_lines=$(find compiler/src/parser -name "*.rs" 2>/dev/null | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
ssa_lines=$(find compiler/src/ssa -name "*.rs" -not -path "*/tests/*" 2>/dev/null | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
asm_lines=$(find compiler/src/asm -name "*.rs" -not -path "*/generated/*" -not -path "*_bak*" 2>/dev/null | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
interp_lines=$(find compiler/src/interpreter -name "*.rs" 2>/dev/null | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
utils_lines=$(find compiler/src/utils -name "*.rs" 2>/dev/null | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
main_lines=$(ls compiler/src/*.rs 2>/dev/null | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
generated_lines=$(find compiler/src/asm/generated -name "*.rs" 2>/dev/null | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')

# 格式化输出
printf "  ${GREEN}├─${NC} 词法分析器 (lexer):        %6d 行\n" "$lexer_lines"
printf "  ${GREEN}├─${NC} 语法分析器 (parser):       %6d 行\n" "$parser_lines"
printf "  ${GREEN}├─${NC} SSA IR (ssa):             %6d 行\n" "$ssa_lines"
printf "  ${GREEN}├─${NC} 汇编后端 (asm):           %6d 行\n" "$asm_lines"
printf "  ${GREEN}├─${NC} 解释器 (interpreter):      %6d 行\n" "$interp_lines"
printf "  ${GREEN}├─${NC} 工具模块 (utils):          %6d 行\n" "$utils_lines"
printf "  ${GREEN}├─${NC} 管线与主程序:              %6d 行\n" "$main_lines"
printf "  ${GREEN}└─${NC} ISLE生成器:                %6d 行\n" "$generated_lines"

echo
echo -e "${BOLD}${YELLOW}【ISLE 规则定义】${NC}"
echo

# ISLE 规则统计
ssa_inst_lines=$(find compiler/src/asm/generated -name "c-ssa-inst.isle" 2>/dev/null | xargs wc -l 2>/dev/null | awk '{print $1}')
rv_inst_lines=$(find compiler/src/asm/generated -name "c-rv-inst.isle" 2>/dev/null | xargs wc -l 2>/dev/null | awk '{print $1}')
lower_lines=$(find compiler/src/asm/generated -name "c-lower-core.isle" 2>/dev/null | xargs wc -l 2>/dev/null | awk '{print $1}')

printf "  ${BLUE}├─${NC} SSA指令定义:               %6d 行\n" "$ssa_inst_lines"
printf "  ${BLUE}├─${NC} RV指令定义:                %6d 行\n" "$rv_inst_lines"
printf "  ${BLUE}└─${NC} 降级规则:                  %6d 行\n" "$lower_lines"

# Cranelift 参考文档（不计入统计）
docs_isle_lines=$(find compiler/src/asm/generated/docs -name "*.isle" 2>/dev/null | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
if [ ! -z "$docs_isle_lines" ]; then
    echo
    printf "  ${MAGENTA}※${NC} Cranelift参考文档:        %6d 行 ${MAGENTA}(不计入统计)${NC}\n" "$docs_isle_lines"
fi

echo
echo -e "${BOLD}${YELLOW}【辅助库与脚本】${NC}"
echo

# 辅助库统计
refkey_lines=$(find crates/refkey-utils/src -name "*.rs" 2>/dev/null | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
simple_lines=$(find crates/simple-compiler/src -name "*.rs" 2>/dev/null | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
scripts_lines=$(find scripts -name "*.sh" 2>/dev/null | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')

printf "  ${CYAN}├─${NC} refkey-utils:              %6d 行\n" "$refkey_lines"
printf "  ${CYAN}├─${NC} simple-compiler (示例):    %6d 行\n" "$simple_lines"
printf "  ${CYAN}└─${NC} Shell 脚本:                %6d 行\n" "$scripts_lines"

# 计算总计
rust_core=$((lexer_lines + parser_lines + ssa_lines + asm_lines + interp_lines + utils_lines + main_lines + generated_lines))
rust_total=$((rust_core + refkey_lines + simple_lines))
isle_total=$((ssa_inst_lines + rv_inst_lines + lower_lines))
total=$((rust_total + isle_total + scripts_lines))

echo
echo -e "${BOLD}${CYAN}════════════════════════════════════════════════════════════${NC}"
echo -e "${BOLD}【汇总统计】${NC}"
echo

printf "  ${BOLD}• Rust 代码总计:${NC}            %7d 行\n" "$rust_total"
printf "  ${BOLD}• ISLE 规则总计:${NC}            %7d 行\n" "$isle_total"
printf "  ${BOLD}• Shell 脚本总计:${NC}           %7d 行\n" "$scripts_lines"

echo -e "${CYAN}────────────────────────────────────────────────────────────${NC}"
printf "  ${BOLD}${GREEN}• 有效代码总行数:${NC}           ${BOLD}%7d 行${NC}\n" "$total"
echo -e "${BOLD}${CYAN}════════════════════════════════════════════════════════════${NC}"

# 计算百分比
if [ "$total" -gt 0 ]; then
    echo
    echo -e "${BOLD}【代码分布比例】${NC}"
    echo
    
    rust_percent=$(echo "scale=1; $rust_total * 100 / $total" | bc)
    isle_percent=$(echo "scale=1; $isle_total * 100 / $total" | bc)
    scripts_percent=$(echo "scale=1; $scripts_lines * 100 / $total" | bc)
    
    printf "  • Rust 代码:     ${GREEN}%5.1f%%${NC}  " "$rust_percent"
    # 打印进度条
    bars=$(echo "scale=0; $rust_percent / 2" | bc)
    printf "${GREEN}"
    for ((i=0; i<bars; i++)); do printf "█"; done
    printf "${NC}\n"
    
    printf "  • ISLE 规则:     ${BLUE}%5.1f%%${NC}  " "$isle_percent"
    bars=$(echo "scale=0; $isle_percent / 2" | bc)
    printf "${BLUE}"
    for ((i=0; i<bars; i++)); do printf "█"; done
    printf "${NC}\n"
    
    printf "  • Shell 脚本:    ${CYAN}%5.1f%%${NC}  " "$scripts_percent"
    bars=$(echo "scale=0; $scripts_percent / 2" | bc)
    printf "${CYAN}"
    for ((i=0; i<bars; i++)); do printf "█"; done
    printf "${NC}\n"
fi

echo
echo -e "${BOLD}【模块规模分析】${NC}"
echo

# 找出最大的三个模块
echo -e "  ${YELLOW}Top 3 最大模块:${NC}"
echo "    1. SSA IR:        $(printf "%6d" $ssa_lines) 行 ($(echo "scale=1; $ssa_lines * 100 / $rust_total" | bc)% of Rust)"
echo "    2. 解释器:        $(printf "%6d" $interp_lines) 行 ($(echo "scale=1; $interp_lines * 100 / $rust_total" | bc)% of Rust)"
echo "    3. 汇编后端:      $(printf "%6d" $asm_lines) 行 ($(echo "scale=1; $asm_lines * 100 / $rust_total" | bc)% of Rust)"

echo
echo -e "${BOLD}${GREEN}统计完成！${NC}"
echo -e "${CYAN}════════════════════════════════════════════════════════════${NC}"