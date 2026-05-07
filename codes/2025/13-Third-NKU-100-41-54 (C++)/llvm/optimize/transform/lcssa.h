#ifndef LCSSA_H
#define LCSSA_H
#include "../../include/ir.h"
#include "../pass.h"
#include "../analysis/loop.h"
#include "../analysis/loopAnalysis.h"


class LCSSAPass : public IRPass {
public:
    LCSSAPass(LLVMIR *IR) : IRPass(IR) {}
    void Execute() override;
private:
    void processLoop(Loop* loop, CFG* cfg);
    std::map<int, BasicInstruction::LLVMType> getUsedOperandOutOfLoop(CFG* cfg, Loop* loop);
};

#endif 