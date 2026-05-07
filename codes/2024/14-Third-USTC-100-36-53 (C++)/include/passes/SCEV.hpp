#pragma once

#include "AnalysisPass.hpp"
#include "BasicBlock.hpp"
#include "Function.hpp"
#include "LoopDetection.hpp"
#include "Module.hpp"
#include "PassManager.hpp"
#include "Value.hpp"
#include "logging.hpp"
#include <deque>
#include <unordered_map>

using namespace std;
enum class exptype { Constant, AddRec };

struct SCEVexp final {
    std::vector<SCEVexp *> operands;
    int constant;
    exptype instID;
    const Loop *loop;
};

class SCEV : public AnalysisPassBase<Function> {
  public:
    using IRUnit = Function;
    void run(Function *f, AnalysisPassManager &APM) override;
    SCEVexp *addSCEV(Value *val, std::unique_ptr<SCEVexp> scev);
    SCEVexp *get_SCEVexp(Value *val);
    std::unique_ptr<SCEVexp> foldAdd(SCEV &res, SCEVexp *lhs, SCEVexp *rhs);
    int getExitCount(BasicBlock *latch);
    Value *getLoopDisposition(); // for LICM

  private:
    unordered_map<Value *, SCEVexp *> SCEVres;
    vector<std::unique_ptr<SCEVexp>> SCEVexps;
};
