#pragma once

#include "Pass.h"
#include "IR.h"
#include "Dom.h"
#include "SideEffectAnalysis.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <sstream>

namespace sysy {

// GVN优化遍的核心逻辑封装类
class GVNContext {
public:
    // 运行GVN优化的主要方法
    void run(Function* func, AnalysisManager* AM, bool& changed);

private:
    // 新的值编号系统
    std::unordered_map<Value*, unsigned> valueToNumber;      // Value -> 值编号
    std::unordered_map<unsigned, Value*> numberToValue;      // 值编号 -> 代表值
    std::unordered_map<std::string, unsigned> expressionToNumber; // 表达式 -> 值编号
    unsigned nextValueNumber = 1;
    
    // 已访问的基本块集合
    std::unordered_set<BasicBlock*> visited;
    
    // 逆后序遍历的基本块列表
    std::vector<BasicBlock*> rpoBlocks;
    
    // 需要删除的指令集合
    std::unordered_set<Instruction*> needRemove;
    
    // 分析结果
    DominatorTree* domTree = nullptr;
    SideEffectAnalysisResult* sideEffectAnalysis = nullptr;
    
    // 计算逆后序遍历
    void computeRPO(Function* func);
    void dfs(BasicBlock* bb);
    
    // 新的值编号方法
    unsigned getValueNumber(Value* value);
    unsigned assignValueNumber(Value* value);
    
    // 基本块处理
    void processBasicBlock(BasicBlock* bb, bool& changed);
    
    // 指令处理
    bool processInstruction(Instruction* inst);
    
    // 表达式构建和查找
    std::string buildExpressionKey(Instruction* inst);
    Value* findExistingValue(const std::string& exprKey, Instruction* inst);
    
    // 支配关系和安全性检查
    bool dominates(Instruction* a, Instruction* b);
    bool isMemorySafe(LoadInst* earlierLoad, LoadInst* laterLoad);
    
    // 清理方法
    void eliminateRedundantInstructions(bool& changed);
    void invalidateMemoryValues(StoreInst* store);
};

// GVN优化遍类
class GVN : public OptimizationPass {
public:
    // 静态成员，作为该遍的唯一ID
    static void* ID;
    
    GVN() : OptimizationPass("GVN", Granularity::Function) {}
    
    // 在函数上运行优化
    bool runOnFunction(Function* func, AnalysisManager& AM) override;
    
    // 返回该遍的唯一ID
    void* getPassID() const override { return ID; }
    
    // 声明分析依赖
    void getAnalysisUsage(std::set<void*>& analysisDependencies, 
                         std::set<void*>& analysisInvalidations) const override;
};

} // namespace sysy
