#pragma once

#include "Pass.h" // 包含Pass的基类定义
#include "IR.h"   // 包含IR相关的定义，如Instruction, Function, BasicBlock, AllocaInst, LoadInst, StoreInst, PhiInst等
#include "Dom.h"  // 假设支配树分析的头文件，提供 DominatorTreeAnalysisResult
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stack> // 用于变量重命名阶段的SSA值栈

namespace sysy {

// 前向声明分析结果类，确保在需要时可以引用
class DominatorTree;

// Mem2RegContext 类，封装 mem2reg 遍的核心逻辑和状态
// 这样可以避免静态变量在多线程或多次运行时的冲突，并保持代码的模块化
class Mem2RegContext {
public:

    Mem2RegContext(IRBuilder *builder) : builder(builder) {}
    // 运行 mem2reg 优化的主要方法
    // func: 当前要优化的函数
    // tp: 分析管理器，用于获取支配树等分析结果
    void run(Function* func, AnalysisManager* tp);

private:
    IRBuilder *builder; // IR 构建器，用于插入指令
    // 存储所有需要被提升的 AllocaInst
    std::vector<AllocaInst*> promotableAllocas;

    // 存储每个 AllocaInst 对应的 Phi 指令列表
    // 键是 AllocaInst，值是该 AllocaInst 在各个基本块中插入的 Phi 指令的列表
    // (实际上，一个 AllocaInst 在一个基本块中只会有一个 Phi)
    std::unordered_map<AllocaInst*, std::unordered_map<BasicBlock*, PhiInst*>> allocaToPhiMap;

    // 存储每个 AllocaInst 对应的当前活跃 SSA 值栈
    // 用于在变量重命名阶段追踪每个 AllocaInst 在不同控制流路径上的最新值
    std::unordered_map<AllocaInst*, std::stack<Value*>> allocaToValueStackMap;

    // 辅助映射，存储每个 AllocaInst 的所有 store 指令
    std::unordered_map<AllocaInst*, std::unordered_set<StoreInst*>> allocaToStoresMap;

    // 辅助映射，存储每个 AllocaInst 对应的定义基本块（包含 store 指令的块）
    std::unordered_map<AllocaInst*, std::unordered_set<BasicBlock*>> allocaToDefBlocksMap;

    // 支配树分析结果，用于 Phi 插入和变量重命名
    DominatorTree* dt;

    // --------------------------------------------------------------------
    // 阶段1: 识别可提升的 AllocaInst
    // --------------------------------------------------------------------

    // 判断一个 AllocaInst 是否可以被提升到寄存器
    // alloca: 要检查的 AllocaInst
    // 返回值: 如果可以提升，则为 true，否则为 false
    bool isPromotableAlloca(AllocaInst* alloca);

    // 收集所有对给定 AllocaInst 进行存储的 StoreInst
    // alloca: 目标 AllocaInst
    void collectStores(AllocaInst* alloca);

    // --------------------------------------------------------------------
    // 阶段2: 插入 Phi 指令 (Phi Insertion)
    // --------------------------------------------------------------------

    // 为给定的 AllocaInst 插入必要的 Phi 指令
    // alloca: 目标 AllocaInst
    // defBlocks: 包含对该 AllocaInst 进行 store 操作的基本块集合
    void insertPhis(AllocaInst* alloca, const std::unordered_set<BasicBlock*>& defBlocks);

    // --------------------------------------------------------------------
    // 阶段3: 变量重命名 (Variable Renaming)
    // --------------------------------------------------------------------

    // 对支配树进行深度优先遍历，重命名变量并替换 load/store 指令
    void renameVariables(BasicBlock* currentBB);

    // --------------------------------------------------------------------
    // 阶段4: 清理
    // --------------------------------------------------------------------

    // 删除所有原始的 AllocaInst、LoadInst 和 StoreInst
    void cleanup();
};

// Mem2Reg 优化遍类，继承自 OptimizationPass
// 粒度为 Function，表示它在每个函数上独立运行
class Mem2Reg : public OptimizationPass {
private:
    IRBuilder *builder;

public:
    // 构造函数
    Mem2Reg(IRBuilder *builder) : OptimizationPass("Mem2Reg", Granularity::Function), builder(builder) {}

    // 静态成员，作为该遍的唯一ID
    static void *ID;

    // 运行在函数上的优化逻辑
    // F: 当前要优化的函数
    // AM: 分析管理器，用于获取支配树等分析结果，或使分析结果失效
    // 返回值: 如果IR被修改，则为true，否则为false
    bool runOnFunction(Function *F, AnalysisManager& AM) override;

    // 声明该遍的分析依赖和失效信息
    // analysisDependencies: 该遍运行前需要哪些分析结果
    // analysisInvalidations: 该遍运行后会使哪些分析结果失效
    void getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const override;
    void *getPassID() const override { return &ID; }
};

} // namespace sysy