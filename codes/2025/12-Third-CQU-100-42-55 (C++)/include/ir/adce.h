#pragma once
#include "llvm_ir.h"
#include "cfg.h"
#include "function_analysis.h"

namespace llvm_ir {
namespace adce {
    // ADCE 优化 pass 主入口
    bool runOnModule(Module& module);
    
    // 在单个函数上执行ADCE
    bool runOnFunction(Function& function, function_analysis::FunctionAttributeAnalyzer& analyzer);
    
    // 构建控制依赖图（CDG）
    std::vector<std::vector<BasicBlock*>> buildCDG(Function& function, cfg::DominatorTree& postDomTree);
    
    // 判断指令是否有副作用（不能被删除）
    bool hasSideEffect(Instruction* inst, function_analysis::FunctionAttributeAnalyzer& analyzer);
    
    // 判断函数是否为内置函数（不能被删除）
    bool isBuiltinFunction(const std::string& funcName);
    
} // namespace adce
} // namespace llvm_ir 