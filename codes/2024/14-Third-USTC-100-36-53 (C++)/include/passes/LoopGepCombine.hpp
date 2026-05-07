#pragma once

#include "AnalysisPass.hpp"
#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "Instruction.hpp"
#include "LoopDetection.hpp"
#include "Module.hpp"
#include "PassManager.hpp"
#include <vector>

using namespace std;

class LoopGepCombine : public Pass<Function> {
  public:
    ~LoopGepCombine() = default;
    void run(Function *func, AnalysisPassManager &APM);

  private:
    struct gep_pair {
        Instruction *base;
        long long idx;
    };
    std::map<Instruction *, gep_pair> work_list;
    void run_on_block(BasicBlock *bb);
};
