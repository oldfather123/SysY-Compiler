#ifndef LOOP_IDIOM_RECOGNIZE_PASS_H
#define LOOP_IDIOM_RECOGNIZE_PASS_H

#include "../pass.h"
#include "../analysis/ScalarEvolution.h"
#include "../analysis/loop.h"
#include "../../include/cfg.h"
#include <functional>
#include <vector>
#include <map>
#include <set>

/**
 * 循环习语识别Pass
 * 
 * 主要识别和优化的循环习语：
 * 1. memset习语：for(int i = 0; i <= n; i++) a[i] = const; -> memset(a, const, size)
 * 2. 算术级数求和：sum = 0; for(int i = 1; i <= const; i++) { sum += i; } -> sum = const*(const+1)/2
 * 3. 模运算习语：sum = 0; for(int i = 1; i <= const; i++) { sum = (sum + i) % p; } -> 直接计算
 * 4. 乘法习语：prod = 1; for(int i = 1; i <= const; i++) { prod *= i; } -> 阶乘计算
 * 5. 异或习语：xor_sum = 0; for(int i = 1; i <= const; i++) { xor_sum ^= i; } -> 直接计算
 */

// 使用ScalarEvolution中定义的结构体
using LoopParams = ScalarEvolution::LoopParams;
using GepParams = ScalarEvolution::GepParams;
using HoistingCandidate = ScalarEvolution::HoistingCandidate;

class LoopIdiomRecognizePass : public IRPass {
public:
    LoopIdiomRecognizePass(LLVMIR* IR) : IRPass(IR) {}
    void Execute() override;

private:
    struct LoopHoistingInfo {
        std::vector<HoistingCandidate> all_candidates;        // 所有可以外提的变量
        std::vector<HoistingCandidate> induction_candidates;  // 可以外提的induction变量
        std::vector<HoistingCandidate> other_candidates;      // 可以外提的其他变量
        
        bool can_recognize_memset;     // 是否可以识别为memset习语
        bool all_hoistable;            // 是否所有变量都能外提
    };
    
    // 分析循环的外提信息
    LoopHoistingInfo analyzeLoopHoisting(Loop* loop, CFG* C, ScalarEvolution* SE);
    
    // 执行部分外提优化（指定候选变量）
    void executePartialHoisting(Loop* loop, CFG* C, ScalarEvolution* SE, const std::vector<HoistingCandidate>& candidates);
    
    // 执行memset优化并外提变量
    bool executeMemsetWithHoisting(Loop* loop, CFG* C, ScalarEvolution* SE, const LoopHoistingInfo& info);
    
    // 检查是否可以识别memset习语
    bool canRecognizeMemsetIdiom(Loop* loop, CFG* C, ScalarEvolution* SE);

private:
    void processFunction(CFG* C);
    void processLoop(Loop* loop, CFG* C, ScalarEvolution* SE);
    

    std::vector<HoistingCandidate> findHoistingCandidates(Loop* loop, CFG* C, ScalarEvolution* SE);
    bool canCalculateFinalValue(const HoistingCandidate& candidate, Loop* loop, CFG* C, ScalarEvolution* SE);
    int calculateFinalValue(const HoistingCandidate& candidate, const LoopParams& params, Loop* loop, CFG* C, ScalarEvolution* SE);
    void hoistVariable(Loop* loop, CFG* C, const HoistingCandidate& candidate, int finalValue);

    bool isVariableOnlyUsedForSelfIteration(Operand op, Loop* loop, CFG* C);
    
    // 检查是否为induction variable
    bool isInductionVariable(Operand op, Loop* loop);
    
    bool isLinearArrayAccess(SCEV* array_scev, Operand induction_var, Loop* loop, ScalarEvolution* SE);
    bool isArithmeticOperation(Instruction inst, BasicInstruction::LLVMIROpcode op, Operand& lhs, Operand& rhs);
    
    // transform function
    void replaceWithMemset(Loop* loop, CFG* C, Operand array, Operand value, GepParams gep_params);
};

#endif // LOOP_IDIOM_RECOGNIZE_PASS_H
