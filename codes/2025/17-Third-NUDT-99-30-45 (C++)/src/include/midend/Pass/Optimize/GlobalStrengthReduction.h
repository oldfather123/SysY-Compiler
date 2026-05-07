#pragma once

#include "Pass.h"
#include "IR.h"
#include "SideEffectAnalysis.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>

namespace sysy {

// 魔数乘法结构，用于除法优化
struct MagicNumber {
    uint32_t multiplier;
    int shift;
    bool needAdd;
    
    MagicNumber(uint32_t m, int s, bool add = false) 
        : multiplier(m), shift(s), needAdd(add) {}
};

// 全局强度削弱优化遍的核心逻辑封装类
class GlobalStrengthReductionContext {
public:
    // 构造函数，接受IRBuilder参数
    explicit GlobalStrengthReductionContext(IRBuilder* builder) : builder(builder) {}
    
    // 运行优化的主要方法
    void run(Function* func, AnalysisManager* AM, bool& changed);

private:
    IRBuilder* builder;  // IR构建器
    
    // 分析结果
    SideEffectAnalysisResult* sideEffectAnalysis = nullptr;
    
    // 优化计数
    int algebraicOptCount = 0;
    int strengthReductionCount = 0;
    int divisionOptCount = 0;
    
    // 主要优化方法
    bool processBasicBlock(BasicBlock* bb);
    bool processInstruction(Instruction* inst);
    
    // 代数优化方法
    bool tryAlgebraicOptimization(Instruction* inst);
    bool optimizeAddition(BinaryInst* inst);
    bool optimizeSubtraction(BinaryInst* inst);
    bool optimizeMultiplication(BinaryInst* inst);
    bool optimizeDivision(BinaryInst* inst);
    bool optimizeComparison(BinaryInst* inst);
    bool optimizeLogical(BinaryInst* inst);
    
    // 强度削弱方法
    bool tryStrengthReduction(Instruction* inst);
    bool reduceMultiplication(BinaryInst* inst);
    bool reduceDivision(BinaryInst* inst);
    bool reducePower(CallInst* inst);
    
    // 复杂乘法强度削弱方法
    bool tryComplexMultiplication(BinaryInst* inst, Value* variable, int constant);
    bool findOptimalShiftDecomposition(int constant, std::vector<int>& shifts);
    Value* createShiftDecomposition(BinaryInst* inst, Value* variable, const std::vector<int>& shifts);
    
    // 魔数乘法相关方法
    MagicNumber computeMagicNumber(uint32_t divisor);
    std::pair<int, int> computeMulhMagicNumbers(int divisor);
    Value* createMagicDivision(BinaryInst* divInst, uint32_t divisor, const MagicNumber& magic);
    Value* createMagicDivisionLibdivide(BinaryInst* divInst, int divisor);
    bool isPowerOfTwo(uint32_t n);
    int log2OfPowerOfTwo(uint32_t n);
    
    // 辅助方法
    bool isConstantInt(Value* val, int& constVal);
    bool isConstantInt(Value* val, uint32_t& constVal);
    ConstantInteger* getConstantInt(int val);
    bool hasOnlyLocalUses(Instruction* inst);
    void replaceWithOptimized(Instruction* original, Value* replacement);
};

// 全局强度削弱优化遍类
class GlobalStrengthReduction : public OptimizationPass {
private:
    IRBuilder* builder;  // IR构建器，用于创建新指令

public:
    // 静态成员，作为该遍的唯一ID
    static void* ID;
    
    // 构造函数，接受IRBuilder参数
    explicit GlobalStrengthReduction(IRBuilder* builder) 
        : OptimizationPass("GlobalStrengthReduction", Granularity::Function), builder(builder) {}
    
    // 在函数上运行优化
    bool runOnFunction(Function* func, AnalysisManager& AM) override;
    
    // 返回该遍的唯一ID
    void* getPassID() const override { return ID; }
    
    // 声明分析依赖
    void getAnalysisUsage(std::set<void*>& analysisDependencies, 
                         std::set<void*>& analysisInvalidations) const override;
};

} // namespace sysy
