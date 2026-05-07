#include "ADT/CFG.h"
#include "frontend.h"
#include "loopinfo.h"
#include "sym.h"
#include <algorithm>

namespace aaac {
namespace optimization {
class LCSSAContext {
private:
  frontend::ControlFlowGraph *cfg;
  NaturalLoop *loop;
  std::unordered_map<sym::Var *, std::vector<frontend::InsnNode *>> useChain;
  std::unordered_map<sym::Var *, std::vector<frontend::InsnNode *>>
      outOfLoopUses;
  std::unordered_map<sym::Var *, frontend::Operand> varToOperand;
  void init();
  void markUsesOutOfLoop();
  void createLCSSA();

public:
  LCSSAContext(frontend::ControlFlowGraph *cfg, NaturalLoop *loop)
      : cfg(cfg), loop(loop), useChain(), outOfLoopUses(), varToOperand(){};
  void LCSSA();
};

void LCSSAContext::init() {
  cfg->buildDomTree();
  useChain = cfg->getUseChain();
}

void LCSSAContext::markUsesOutOfLoop() {
  for (auto bb : this->loop->getLoopNodes()) {
    for (auto node : *bb) {
      if (node->getDefs().size() == 0)
        continue;
      auto def = node->getDefs().front();
      varToOperand[def.get()] = def;
      auto use_list = this->useChain.at(def.get());
      for (auto use_node : use_list) {
        // node不在循环中
        if (loop->getLoopNodes().count(use_node->getParent().get()) == 0) {
          outOfLoopUses[def.get()].push_back(use_node);
        }
      }
    }
  }
}

void LCSSAContext::createLCSSA() {
  // TODO: 感觉这段可以抽出来单独作为一个辅助函数
  auto exiting_edges = loop->getExitingEdges();
  std::map<frontend::BasicBlock *, std::set<frontend::BasicBlock *>>
      exit_to_exitings;
  for (auto &[exitingBB, edge] : exiting_edges) {
    auto exitBB = edge.getDest();
    exit_to_exitings[exitBB.get()].insert(exitingBB);
  }
  
  for (auto [exitBB, exiting_list] : exit_to_exitings) {
    for (auto [var, chain] : this->outOfLoopUses) {
      auto lcssa_var = sym::Var::generateTemp(var->getType(), "LCSSA_");
      auto lcssa_phi = frontend::PhiNode::create(lcssa_var);
      for(auto exitingBB:exiting_list)
        lcssa_phi->addOperand(varToOperand[var], exitingBB->shared_from_this());
      auto insert_place = exitBB->begin();
      while ((*insert_place)->getCode() == frontend::NodeCode::Label) {
        ++insert_place;
      }
      exitBB->insertNodeBefore(insert_place, lcssa_phi);
      for (auto use : chain) {
        if (cfg->isDominate(exitBB, use->getParent().get())) {
          use->replaceUsesWith(varToOperand[var], lcssa_var);
        }
      }
    }
  }
}

void LCSSAContext::LCSSA() {
  init();
  markUsesOutOfLoop();
  createLCSSA();
}

} // namespace optimization
} // namespace aaac

void LCSSA(aaac::frontend::ControlFlowGraph *cfg,
           aaac::optimization::NaturalLoop *loop) {
  auto context = aaac::optimization::LCSSAContext(cfg, loop);
  context.LCSSA();
}
