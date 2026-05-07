#pragma once

#include "AnalysisPass.hpp"
#include "BasicBlock.hpp"
#include "Function.hpp"
#include "LoopDetection.hpp"
#include "Module.hpp"
#include "PassManager.hpp"

using namespace std;

class LoopSimplify : public Pass<Function> {
  public:
    using PhiPair = pair<Value *, BasicBlock *>;
    void run(Function *func, AnalysisPassManager &APM);

  private:
    pair<vector<PhiPair>, vector<PhiPair>> split_phi_op(PhiInst *phi,
                                                        Loop &loop);
    BasicBlock *preheader_insertion(Loop &loop);
    BasicBlock *exit_insertion(BasicBlock *exiting, BasicBlock *outer_bb);
    static BasicBlock *insert_unique_backedgeblock(std::shared_ptr<Loop> L);
    bool normalize_exit(Loop *L);
};
