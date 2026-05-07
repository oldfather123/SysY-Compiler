#ifndef MEMDEP_H
#define MEMDEP_H
#include "../../../include/Instruction.h"
#include "../pass.h"
#include "AliasAnalysis.h"
#include "dominator_tree.h"
class CFG;
class LLVMIR;


// 简易内存依赖分析器
class SimpleMemDepAnalyser {
private:
    // 获取可能影响当前加载指令(I)的前序内存操作（加载/存储）
    std::set<Instruction> GetLoadClobbers(Instruction I, CFG *C);

    // 获取可能被当前存储指令(I)影响的后继加载指令
    std::set<Instruction> GetStorePostClobbers(Instruction I, CFG *C);

    // 判断从指令 I1 到 I2 的所有路径中是否存在对 I1 指针的存储
    // 前提: I1 支配 I2 (I1 Dom I2)
    bool IsNoStore(Instruction I1, Instruction I2, CFG *C);

    // 判断外部函数调用是否读取指针操作数
    bool IsExternalCallReadPtr(CallInstruction *I, Operand ptr, CFG *C);

    AliasAnalysisPass *alias_analyser;  // 别名分析器实例
    LLVMIR* IR;  
    DomAnalysis *domtrees;                       // LLVM 中间表示
    Instruction FindCriticalInstInBlock(
    LLVMBlock block, Operand ptr,int startIdx,
    int endIdx,int step,bool isLoadMode,CFG* cfg
);
    bool CheckInstForClobber(
    Instruction inst, 
    Operand ptr, 
    CFG* cfg, 
    bool isLoadMode
);

public:
    // 判断两条加载/存储指令是否访问相同内存区域
    // 假设: 所有有副作用的调用指令会写入全部内存
    bool isLoadSameMemory(Instruction a, Instruction b, CFG *C);

    // 判断两个存储指令是否被相同的加载指令使用 (存储值的使用分析)
    // 要求: a 和 b 必须是存储指令
    bool isStoreBeUsedSame(Instruction a, Instruction b, CFG *C);

    // 判断存储指令的结果是否未被使用
    bool isStoreNotUsed(Instruction a, CFG *C);

    // 测试函数 (开发调试用)
    void MemDepTest();

    SimpleMemDepAnalyser(LLVMIR *ir, AliasAnalysisPass * aa,DomAnalysis *dom)
        : IR(ir), alias_analyser(aa),domtrees(dom) {}
};

// 内存依赖分析 Pass
class MemDepAnalysisPass : public IRPass { 
private:
    AliasAnalysisPass *alias_analyser;  // 依赖的别名分析器
    DomAnalysis *domtrees;
public:

    MemDepAnalysisPass(LLVMIR *IR, AliasAnalysisPass * aa,DomAnalysis *dom)
        : IRPass(IR), alias_analyser(aa),domtrees(dom) {}
    
    // Pass 执行入口 (除了建立cfgTable之外，没有任何实际意义)
    void Execute();
};

#endif