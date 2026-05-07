#include "frontend.h"
#include "loopinfo.h"
#include <functional>

extern void LoopSimplify(aaac::frontend::ControlFlowGraph *cfg,
                         aaac::optimization::NaturalLoop *loop);
extern void LCSSA(aaac::frontend::ControlFlowGraph *cfg,
                  aaac::optimization::NaturalLoop *loop);
extern void printCFGPass(aaac::frontend::ControlFlowGraph *cfg);
extern void LICM(aaac::frontend::ControlFlowGraph *cfg,
                 aaac::optimization::NaturalLoop *loop);

void runLoopPassOn(aaac::frontend::ControlFlowGraph *cfg,
                   aaac::optimization::NaturalLoop *loop,
                   std::function<void(aaac::frontend::ControlFlowGraph *,
                                      aaac::optimization::NaturalLoop *)>
                       pass) {
  for (auto nested_loop : loop->getNestedLoops()) {
    runLoopPassOn(cfg, nested_loop, pass);
  }
  pass(cfg, loop);
}

void loopPassManager(aaac::frontend::ControlFlowGraph *cfg) {
  auto analyzer = aaac::optimization::NaturalLoopAnalyzer(cfg);
  auto loop_set = analyzer.getAllLoops();
  for (auto &loop : loop_set) {
    runLoopPassOn(cfg, loop.get(), LoopSimplify);
    runLoopPassOn(cfg, loop.get(), LCSSA);
    runLoopPassOn(cfg, loop.get(), LICM);
  }
  cfg->buildDomTree();
}