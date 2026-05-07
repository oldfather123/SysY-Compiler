#pragma once

#include "Instruction.hpp"
#include "PassManager.hpp"

class SimplifyCFG : public Pass<Function> {
    void simplify(Function *f);
    bool remove_dead_blocks(Function *f);
    bool merge_into_single_predecessor(Function *f);
    bool remove_unused_phi(Function *f);

    static void remove_dead_phi(BasicBlock *bb1, BasicBlock *bb2);
    static void replace_bb(BasicBlock *bb1, BasicBlock *bb2);

  public:
    // SimplifyCFG(Module *m) : Pass(m) {}
    ~SimplifyCFG() = default;

    void run(Function *module, AnalysisPassManager &APM) override;

    static void remove_edge(BasicBlock *bb1, BasicBlock *bb2);
    static void remove_bb(BasicBlock *bb);
};
