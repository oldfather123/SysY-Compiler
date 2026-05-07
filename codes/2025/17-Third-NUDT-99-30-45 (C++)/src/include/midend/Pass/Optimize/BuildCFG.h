#pragma once

#include "IR.h"
#include "Pass.h"
#include <queue>
#include <set>

namespace sysy {

class BuildCFG : public OptimizationPass {
public:
  static void *ID;
  BuildCFG() : OptimizationPass("BuildCFG", Granularity::Function) {}
  bool runOnFunction(Function *F, AnalysisManager &AM) override;
  void getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const override;
  void *getPassID() const override { return &ID; }

};

} // namespace sysy