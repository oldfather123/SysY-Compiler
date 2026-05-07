#pragma once

#include "AnalysisPass.hpp"
#include "Dominators.hpp"
#include "Function.hpp"
#include "LoopDetection.hpp"
#include "Module.hpp"
#include "PassManager.hpp"

// LICM 外提到原本头结点与truebb之间
class LoopRotate : public Pass<Function> {
  public:
    void run(Function *func, AnalysisPassManager &APM);

  private:
    void rotate_loop(Loop *L, AnalysisPassManager &APM);
    void clear_phi(Loop *L);
    bool simplifyLoopLatch(Loop *L);
};
