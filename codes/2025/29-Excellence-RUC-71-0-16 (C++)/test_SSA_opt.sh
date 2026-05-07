# SSA优化全面测试脚本
echo "=== SSA优化全面测试开始 ==="
echo "测试时间: $(date)"
echo ""

# 测试结果统计
total_tests=0
success_tests=0
failed_tests=0

# 创建测试结果目录
mkdir -p test_results/ssa_all/

# 函数：测试单个文件
test_file() {
    local file=$1
    local filename=$(basename "$file" .sy)
    echo "正在测试: $filename"
    
    # 增加计数
    ((total_tests++))
    
    # 运行编译
    ./compiler --O1 "$file" > "test_results/ssa_all/${filename}.log" 2>&1
    
    # 检查SSA优化是否成功
    if grep -q "SSA optimizations completed" "test_results/ssa_all/${filename}.log"; then
        echo "  ✅ SSA优化成功"
        ((success_tests++))
        
        # 统计优化效果
        eliminated=$(grep -o "Eliminated [0-9]* instructions" "test_results/ssa_all/${filename}.log" | head -1 | grep -o "[0-9]*" || echo "0")
        blocks_eliminated=$(grep -c "Eliminating unreachable block" "test_results/ssa_all/${filename}.log" || echo "0")
        algebraic_opts=$(grep -c "simplifications applied" "test_results/ssa_all/${filename}.log" | grep -v "0 simplifications" | wc -l || echo "0")
        
        echo "    - 消除指令: $eliminated 条"
        echo "    - 消除基本块: $blocks_eliminated 个" 
        echo "    - 代数化简: $algebraic_opts 次"
    else
        echo "  ❌ SSA优化失败"
        ((failed_tests++))
        # 显示错误信息的前几行
        echo "    错误信息:"
        head -10 "test_results/ssa_all/${filename}.log" | sed 's/^/    /'
    fi
    echo ""
}

echo "=== 测试 functional 目录 ==="
echo ""

# 测试所有functional目录的.sy文件

for file in tests/functional/*.sy; do
    if [ -f "$file" ]; then
        test_file "$file"
    fi
done

echo "=== 测试 h_functional 目录 ==="
echo ""

# 测试所有h_functional目录的.sy文件  
for file in tests/h_functional/*.sy; do
    if [ -f "$file" ]; then
        test_file "$file"
    fi
done

echo "=== 测试结果汇总 ==="
echo "总测试数: $total_tests"
echo "成功数: $success_tests"
echo "失败数: $failed_tests"
echo "成功率: $(( success_tests * 100 / total_tests ))%"
echo ""

echo "=== 优化效果统计 ==="

# 统计所有测试的优化效果
total_eliminated=0
total_blocks_eliminated=0
total_algebraic_opts=0

for log_file in test_results/ssa_all/*.log; do
    if [ -f "$log_file" ]; then
        eliminated=$(grep -o "Eliminated [0-9]* instructions" "$log_file" | head -1 | grep -o "[0-9]*" 2>/dev/null || echo "0")
        blocks=$(grep -c "Eliminating unreachable block" "$log_file" 2>/dev/null || echo "0")
        algebraic=$(grep -c "simplifications applied" "$log_file" 2>/dev/null || echo "0")
        
        # 确保数值为有效整数
        if [[ ! "$eliminated" =~ ^[0-9]+$ ]]; then eliminated=0; fi
        if [[ ! "$blocks" =~ ^[0-9]+$ ]]; then blocks=0; fi
        if [[ ! "$algebraic" =~ ^[0-9]+$ ]]; then algebraic=0; fi
        
        total_eliminated=$((total_eliminated + eliminated))
        total_blocks_eliminated=$((total_blocks_eliminated + blocks))
        total_algebraic_opts=$((total_algebraic_opts + algebraic))
    fi
done

echo "总消除指令数: $total_eliminated"
echo "总消除基本块数: $total_blocks_eliminated" 
echo "总代数化简次数: $total_algebraic_opts"
echo ""
echo "=== 测试完成 ==="
echo "详细日志保存在: test_results/ssa_all/"
