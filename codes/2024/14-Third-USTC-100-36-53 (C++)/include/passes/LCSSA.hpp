#pragma once

#include "AnalysisPass.hpp"
#include "Function.hpp"
#include "LoopDetection.hpp"
#include "Module.hpp"
#include "PassManager.hpp"

using namespace std;

class LCSSA : public Pass<Function> {
  public:
    using IRUnit = Module;
    using PhiPair = pair<Value *, BasicBlock *>;
    void run(Function *func, AnalysisPassManager &APM);

  private:
};
