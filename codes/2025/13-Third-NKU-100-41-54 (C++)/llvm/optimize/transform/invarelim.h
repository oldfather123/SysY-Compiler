#ifndef INVAR_ELIM_H
#define INVAR_ELIM_H

#include <vector>
#include <map>
#include <set>
#include <string>
#include <iostream>
#include "../../../include/ir.h"
#include "../../../include/cfg.h"
#include "../../../include/basic_block.h"
#include "../../../include/Instruction.h"
#include "../analysis/ScalarEvolution.h"
#include "../analysis/loop.h"

// 调试宏定义
// #define INVAR_ELIM_DEBUG

#ifdef INVAR_ELIM_DEBUG
#define INVAR_ELIM_DEBUG_PRINT(x) do { std::cerr << "[INVAR_ELIM] " << x << std::endl; } while(0)
#else
#define INVAR_ELIM_DEBUG_PRINT(x) do {} while(0)
#endif

class InvariantVariableEliminationPass {
private:
    LLVMIR* llvmIR;
    
    void processFunction(CFG* C);
    void processLoop(Loop* loop, CFG* C, ScalarEvolution* SE);
    
    // 分析循环中的归纳变量，返回按模式分组的变量
    std::map<std::string, std::vector<Operand>> analyzeInductionVariables(Loop* loop, CFG* C, ScalarEvolution* SE);
    
    // 从一组变量中选择代表变量
    Operand selectRepresentativeVariable(const std::vector<Operand>& variables, Loop* loop);
    
    // 检查变量是否在循环控制中使用
    bool isUsedInLoopControl(Operand var, Loop* loop);
    
    // 替换变量的所有使用
    void replaceVariableUses(Operand oldVar, Operand newVar, CFG* C);
    
    // 删除phi指令
    void removePhiInstruction(Operand var, Loop* loop);
    
    // 删除操作数定义
    void removeOperandDefinition(Operand operand, Loop* loop);
    
    // 检查操作数是否还在使用
    bool isOperandStillUsed(Operand operand, Loop* loop);

public:
    InvariantVariableEliminationPass(LLVMIR* ir) : llvmIR(ir) {}
    
    // 执行归纳变量消除优化
    void Execute();
};

#endif // INVAR_ELIM_H
