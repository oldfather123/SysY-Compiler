#pragma once

#include "Pass.h"
#include "IR.h"
#include "AliasAnalysis.h"
#include "CallGraphAnalysis.h"
#include <unordered_set>
#include <unordered_map>

namespace sysy {

// 副作用类型枚举
enum class SideEffectType {
    NO_SIDE_EFFECT,      // 无副作用
    MEMORY_WRITE,        // 内存写入（store、memset）
    FUNCTION_CALL,       // 函数调用（可能有任意副作用）
    IO_OPERATION,        // I/O操作（printf、scanf等）
    UNKNOWN              // 未知副作用
};

// 副作用信息结构
struct SideEffectInfo {
    SideEffectType type = SideEffectType::NO_SIDE_EFFECT;
    bool mayModifyGlobal = false;      // 可能修改全局变量
    bool mayModifyMemory = false;      // 可能修改内存
    bool mayCallFunction = false;      // 可能调用函数
    bool isPure = true;                // 是否为纯函数（无副作用且结果只依赖参数）
    
    // 合并两个副作用信息
    SideEffectInfo merge(const SideEffectInfo& other) const {
        SideEffectInfo result;
        result.type = (type == SideEffectType::NO_SIDE_EFFECT) ? other.type : type;
        result.mayModifyGlobal = mayModifyGlobal || other.mayModifyGlobal;
        result.mayModifyMemory = mayModifyMemory || other.mayModifyMemory;
        result.mayCallFunction = mayCallFunction || other.mayCallFunction;
        result.isPure = isPure && other.isPure;
        return result;
    }
};

// 副作用分析结果类
class SideEffectAnalysisResult : public AnalysisResultBase {
private:
    // 指令级别的副作用信息
    std::unordered_map<Instruction*, SideEffectInfo> instructionSideEffects;
    
    // 函数级别的副作用信息
    std::unordered_map<Function*, SideEffectInfo> functionSideEffects;
    
    // 已知的SysY标准库函数副作用信息
    std::unordered_map<std::string, SideEffectInfo> knownFunctions;

public:
    SideEffectAnalysisResult();
    virtual ~SideEffectAnalysisResult() noexcept override = default;
    
    // 获取指令的副作用信息
    const SideEffectInfo& getInstructionSideEffect(Instruction* inst) const;
    
    // 获取函数的副作用信息
    const SideEffectInfo& getFunctionSideEffect(Function* func) const;
    
    // 设置指令的副作用信息
    void setInstructionSideEffect(Instruction* inst, const SideEffectInfo& info);
    
    // 设置函数的副作用信息
    void setFunctionSideEffect(Function* func, const SideEffectInfo& info);
    
    // 检查指令是否有副作用
    bool hasSideEffect(Instruction* inst) const;
    
    // 检查指令是否可能修改内存
    bool mayModifyMemory(Instruction* inst) const;
    
    // 检查指令是否可能修改全局状态
    bool mayModifyGlobal(Instruction* inst) const;
    
    // 检查函数是否为纯函数
    bool isPureFunction(Function* func) const;
    
    // 获取已知函数的副作用信息
    const SideEffectInfo* getKnownFunctionSideEffect(const std::string& funcName) const;
    
    // 初始化已知函数的副作用信息
    void initializeKnownFunctions();
    
private:
};

// 副作用分析遍类 - Module级别分析
class SysYSideEffectAnalysisPass : public AnalysisPass {
public:
    // 静态成员，作为该遍的唯一ID
    static void* ID;
    
    SysYSideEffectAnalysisPass() : AnalysisPass("SysYSideEffectAnalysis", Granularity::Module) {}
    
    // 在模块上运行分析
    bool runOnModule(Module* M, AnalysisManager& AM) override;
    
    // 获取分析结果
    std::unique_ptr<AnalysisResultBase> getResult() override;
    
    // Pass 基类中的纯虚函数，必须实现
    void* getPassID() const override { return &ID; }

private:
    // 分析结果
    std::unique_ptr<SideEffectAnalysisResult> result;
    
    // 调用图分析结果
    CallGraphAnalysisResult* callGraphAnalysis = nullptr;
    
    // 分析单个函数的副作用（Module级别的内部方法）
    SideEffectInfo analyzeFunction(Function* func, AnalysisManager& AM);
    
    // 分析单个指令的副作用
    SideEffectInfo analyzeInstruction(Instruction* inst, Function* currentFunc, AnalysisManager& AM);
    
    // 分析函数调用指令的副作用（利用调用图）
    SideEffectInfo analyzeCallInstruction(CallInst* call, Function* currentFunc, AnalysisManager& AM);
    
    // 分析存储指令的副作用
    SideEffectInfo analyzeStoreInstruction(StoreInst* store, Function* currentFunc, AnalysisManager& AM);
    
    // 分析内存设置指令的副作用
    SideEffectInfo analyzeMemsetInstruction(MemsetInst* memset, Function* currentFunc, AnalysisManager& AM);
    
    // 使用不动点算法分析递归函数群
    void analyzeStronglyConnectedComponent(const std::vector<Function*>& scc, AnalysisManager& AM);
    
    // 检查函数间副作用传播的收敛性
    bool hasConverged(const std::unordered_map<Function*, SideEffectInfo>& oldEffects,
                      const std::unordered_map<Function*, SideEffectInfo>& newEffects) const;
};

} // namespace sysy
