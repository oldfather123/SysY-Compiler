#pragma once

#include "Pass.h"
#include "IR.h"
#include "SysYIROptUtils.h"
#include "Dom.h" 
#include "AliasAnalysis.h"
#include "SideEffectAnalysis.h"
#include <unordered_set>
#include <queue>

namespace sysy {

// 前向声明分析结果类，确保在需要时可以引用
// class DominatorTreeAnalysisResult; // Pass.h 中已包含，这里不再需要
class SideEffectInfoAnalysisResult; // 假设有副作用分析结果类

// DCEContext 类，用于封装DCE的内部逻辑和状态
// 这样可以避免静态变量在多线程或多次运行时的冲突，并保持代码的模块化
class DCEContext {
public:
    // 运行DCE的主要方法
    // func: 当前要优化的函数
    // tp: 分析管理器，用于获取其他分析结果（如果需要）
    void run(Function* func, AnalysisManager* AM, bool &changed);

private:
    // 存储活跃指令的集合
    std::unordered_set<Instruction*> alive_insts;
    // 别名分析结果
    AliasAnalysisResult* aliasAnalysis = nullptr;
    // 副作用分析结果
    SideEffectAnalysisResult* sideEffectAnalysis = nullptr;

    // 判断指令是否是"天然活跃"的（即总是保留的）
    // inst: 要检查的指令
    // 返回值: 如果指令是天然活跃的，则为true，否则为false
    bool isAlive(Instruction* inst);

    // 递归地将活跃指令及其依赖加入到 alive_insts 集合中
    // inst: 要标记为活跃的指令
    void addAlive(Instruction* inst);
    
    // 检查Store指令是否可能有副作用（通过别名分析）
    bool mayHaveSideEffect(StoreInst* store);
};

// DCE 优化遍类，继承自 OptimizationPass
class DCE : public OptimizationPass {
public:
    // 构造函数
    DCE() : OptimizationPass("DCE", Granularity::Function) {}

    // 静态成员，作为该遍的唯一ID
    static void *ID;

    // 运行在函数上的优化逻辑
    // F: 当前要优化的函数
    // AM: 分析管理器，用于获取或使分析结果失效
    // 返回值: 如果IR被修改，则为true，否则为false
    bool runOnFunction(Function *F, AnalysisManager& AM) override;

    // 声明该遍的分析依赖和失效信息
    // analysisDependencies: 该遍运行前需要哪些分析结果
    // analysisInvalidations: 该遍运行后会使哪些分析结果失效
    void getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const override;

    // Pass 基类中的纯虚函数，必须实现
    void *getPassID() const override { return &ID; }
};

} // namespace sysy