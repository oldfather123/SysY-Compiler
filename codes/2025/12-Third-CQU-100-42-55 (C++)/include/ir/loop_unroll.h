#ifndef LLVM_IR_LOOP_UNROLL_H
#define LLVM_IR_LOOP_UNROLL_H

#include "llvm_ir.h"
#include "loop_analysis.h"
#include "mem2reg.h"
#include "scev.h"
#include <vector>
#include <memory>
#include <map>
#include <set>

namespace llvm_ir {
namespace loop_unroll {

// 辅助结构体：循环边界信息
struct LoopBoundInfo {
    int initvalue;
    int lowerBound;
    int upperBound;
    int step;
    ICmpCond condition;
    bool isUpperBoundClosed;
    int tripCount;

    SCEVLoopVariableInfo* primaryVarInfo;
    
    LoopBoundInfo() : initvalue(0), lowerBound(0), upperBound(0), step(1), condition(ICmpCond::SLT), isUpperBoundClosed(false), tripCount(-1), primaryVarInfo(nullptr) {}
};
// 循环展开器
class LoopUnroller {
public:
    int unrollFactor;

    LoopUnroller(Function& F, int unrollFactor) : function(F), unrollFactor(unrollFactor) {}
    
    // 检查循环结束条件
    static bool isLoopEnd(int i, int ub, ICmpCond cond);

    // 部分/完全展开（由fullyUnrollMode控制）
    bool partiallyUnrollLoop(Loop* loop, Value* primaryVar, const SCEVLoopVariableInfo& primaryVarInfo, LoopBoundInfo info, bool fullyUnrollMode = false);

    // 设置插入基本块的起始位置（位于Function.basicBlocks中的索引）
    void setInsertionIndex(size_t idx) { insertionIndex = idx; }

private:
    Function& function;
    
    // 创建新的基本块
    BasicBlock* createNewBlock(const std::string& label);
    
    // 复制指令
    std::unique_ptr<Instruction> copyInstruction(Instruction* inst, std::string newName);
    
    // 复制基本块
    BasicBlock* copyBasicBlock(BasicBlock* original, const std::string& newLabel, int iteration);

    // 生成Guard逻辑
    bool generateGuardLogic(Loop* loop, Value* primaryVar, const SCEVLoopVariableInfo& primaryVarInfo, LoopBoundInfo info);
    
    // 生成唯一的标签名称
    std::string generateUniqueLabel(const std::string& base);
    
    // 全局计数器
    static int blockCounter;
    static int guardCounter;

    // 按顺序插入用的索引（指向Function.basicBlocks中的位置）。每插入一个块自增。
    size_t insertionIndex = 0;
};

// 主循环展开Pass
class LoopUnrollPass {
public:
    LoopUnrollPass(Module& M) : module(M) {}

    // 分析循环是否为简单循环
    static bool isSimpleLoop(Loop* loop, LoopBoundInfo info);
    
    // 检查循环是否可以展开
    static bool canFullyUnrollLoop(Loop* loop, LoopBoundInfo info);
    
    // 运行循环展开
    bool run();
    
    // 在单个函数上运行
    bool runOnFunction(Function& F);

private:
    Module& module;
    
    // DFS遍历循环树
    void dfsUnroll(Function& F, Loop* loop, bool& isUnrolled, LoopAnalysis& loopAnalysis);
};

} // namespace loop_unroll
} // namespace llvm_ir

#endif // LLVM_IR_LOOP_UNROLL_H 