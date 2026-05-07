#pragma once
#include "llvm_ir.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace llvm_ir {

// 活跃变量分析结果
struct LiveInfo {
    // 每个基本块的 in/out 集合
    std::unordered_map<BasicBlock*, std::unordered_set<Value*>> in, out;
    // 每条指令的活跃变量集合（指令指针 -> 活跃变量集合）
    std::unordered_map<Instruction*, std::unordered_set<Value*>> live_in, live_out;
};

// LLVM风格的逆向数据流活跃变量分析
LiveInfo live_variable_analysis(Function* func);

}
