#ifndef LOOP_UNROLL_PASS_H
#define LOOP_UNROLL_PASS_H

#include "../pass.h"
#include "../analysis/ScalarEvolution.h"
#include "../analysis/loop.h"
#include "../../include/cfg.h"
#include <functional>
#include <vector>
#include <map>

class LoopUnrollPass : public IRPass {
public:
    LoopUnrollPass(LLVMIR* IR) : IRPass(IR) {}
    void Execute() override;

private:
    void LoopUnroll(CFG *C);
    void processLoop(Loop* loop, CFG* C, ScalarEvolution* SE);
    bool canUnrollLoop(Loop* loop, CFG* C, ScalarEvolution* SE);
    bool isConstantLoop(SCEV* scev1, SCEV* scev2, Loop* loop, ScalarEvolution* SE);
    void unrollLoop(Loop* loop, CFG* C, ScalarEvolution* SE);
    
    // 辅助方法
    int getConstantValue(SCEV* scev);
    int calculateIterations(int start, int bound, int step, BasicInstruction::IcmpCond cond);
    void createUnrolledBlocks(Loop* loop, CFG* C, int unroll_count, std::vector<LLVMBlock>& unrolled_blocks, ScalarEvolution* SE);
    void copyInstructionsWithRenaming(LLVMBlock src, LLVMBlock dst, CFG* C, int iteration,
                                     std::map<int, std::map<int, int>>& iteration_reg_maps,
                                     Loop* loop, ScalarEvolution* SE);
    Instruction cloneInstruction(Instruction inst, CFG* C, std::map<int, int>& reg_map, int iteration, Loop* loop = nullptr, ScalarEvolution* SE = nullptr);

    // 控制流更新
    void updateControlFlow(Loop* loop, CFG* C, int unroll_count, int remaining_iterations,
                          const std::vector<LLVMBlock>& unrolled_blocks,
                          Operand induction_var, int step_val, Operand bound, 
                          BasicInstruction::IcmpCond cond, ScalarEvolution* SE);
    LLVMBlock createRemainingLoop(Loop* loop, CFG* C, int remaining_iterations,
                                 Operand induction_var, int step_val, Operand bound,
                                 BasicInstruction::IcmpCond cond, ScalarEvolution* SE);
    void updatePreheaderJump(LLVMBlock preheader, LLVMBlock first_unrolled_block);
    void connectUnrolledBlocks(const std::vector<LLVMBlock>& unrolled_blocks, Loop* loop, CFG* C);
    void connectToRemainingLoop(LLVMBlock last_unrolled, LLVMBlock remaining_loop, CFG* C);
    void connectToExit(LLVMBlock last_unrolled, LLVMBlock exit, CFG* C);
    void removeOriginalLoopBlocks(Loop* loop, CFG* C);
};

#endif // LOOP_UNROLL_PASS_H 