#include "ADT/CFG.h"
#include "frontend.h"
#include "loopinfo.h"
#include "sym.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <set>

namespace aaac {
namespace optimization {
class LICMContext {
private:
  frontend::ControlFlowGraph *cfg;
  NaturalLoop *loop;
  std::map<sym::Var *, frontend::BasicBlock::iterator> invariantNodes;
  std::unordered_map<sym::Var *, std::vector<frontend::InsnNode *>> useChain;
  std::set<sym::Var *> loopDefined;
  std::map<sym::Var *, frontend::Operand> varToOperand;
  frontend::BasicBlock *prehead;

  bool isInvariant(sym::Var *var);
  bool canMoveExpression(frontend::BasicBlock::iterator node);
  void init();
  void markInvariant();
  void doCodeMotion();

public:
  LICMContext(frontend::ControlFlowGraph *cfg, NaturalLoop *loop)
      : cfg(cfg), loop(loop), invariantNodes(), useChain(), loopDefined(),
        varToOperand(){};
  void LICM();
};

void LICMContext::init() {
  cfg->buildDomTree();
  this->useChain = cfg->getUseChain();
  for (auto bb : loop->getLoopNodes()) {
    for (auto beg = bb->begin(); beg != bb->end(); ++beg) {
      auto node = *beg;
      if (node->getDefs().size() == 0) {
        continue;
      }
      auto def = node->getDefs().front();
      invariantNodes[def.get()] = beg;
      loopDefined.insert(def.get());
    }
  }

  auto head = loop->getLoopHead();
  for (auto pred_edge : head->getPredEdges()) {
    auto pred = pred_edge.getDest();
    // 断言：head的所有不在循环中的前驱一定是Loop Simplify得到的prehead
    // TODO：加上这个断言
    if (loop->getLoopNodes().count(pred.get()) == 0) {
      this->prehead = pred.get();
      break;
    }
  }
}

bool LICMContext::canMoveExpression(frontend::BasicBlock::iterator iter) {
  auto ptr = *iter;
  // 不能在循环头中
  if (ptr->getParent().get() == loop->getLoopHead())
    return false;

  auto exiting_edges = loop->getExitingEdges();
  std::set<frontend::BasicBlock *> exitBB;
  for (auto [bb, edge] : exiting_edges) {
    // LCSSA转换后可以这么做
    if (std::find_if(bb->begin(), bb->end(),
                     [ptr](std::shared_ptr<frontend::InsnNode> node) {
                       return node->isUse(ptr->getDefs().front().get());
                     }) != bb->end()) {
      exitBB.insert(edge.getDest().get());
    }
  }

  // 支配了所有的活跃出口块
  auto nodeBB = ptr->getParent().get();
  for (auto bb : exitBB) {
    if (!cfg->isDominate(nodeBB, bb))
      return false;
  }
  return true;
}

void LICMContext::markInvariant() {
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto bb : loop->getLoopNodes()) {
      for (auto node : *bb) {
        // std::cout<<"Current node:\t\t"<<*node<<std::endl;
        if (node->getDefs().size() == 0)
          continue;
        auto def = node->getDefs().front();
        if (!isInvariant(def.get()) &&
            this->invariantNodes.count(def.get()) != 0) {
          changed = true;
          this->invariantNodes.erase(def.get());
        }
      }
    }
  }
}

bool LICMContext::isInvariant(sym::Var *var) {
  if (loopDefined.count(var) == 0)
    return true;

  if (invariantNodes.count(var) == 0)
    return false;

  auto node = invariantNodes.at(var);
  auto uses_list = (*node)->getUses();

  if ((*node)->getCode() != frontend::NodeCode::Assign &&
      (*node)->getCode() != frontend::NodeCode::Nop)
    return false; // 当前没有实现对Load 和 Store的支持

  bool invariant = true;
  for (auto use : uses_list) {
    if (!this->isInvariant(use.get())) {
      invariant = false;
    }
  }
  return invariant;
}

void LICMContext::doCodeMotion() {
  auto insert_iter = this->prehead->begin();
  while ((*insert_iter)->getCode() == frontend::NodeCode::Label) {
    ++insert_iter;
  }
  --insert_iter;

  // 顺序
  // for (auto [var, iter] : this->invariantNodes) {
  //   auto inloopBB = (*iter)->getParent();
  //     insert_iter = prehead->insertNodeAfter(insert_iter, *iter);
  //     inloopBB->removeNode(iter);
  // }
  while (!this->invariantNodes.empty()) {
    for (auto beg = invariantNodes.begin(); beg != invariantNodes.end();) {
      bool ready=true;
      auto uses_list=(*(beg->second))->getUses();
      for(auto use:uses_list){
        if(invariantNodes.count(use.get())!=0)
          ready=false;
      }

      if(ready){
        auto iter=beg->second;
        auto inloopBB=(*iter)->getParent();
        insert_iter=prehead->insertNodeAfter(insert_iter, *iter);
        inloopBB->removeNode(iter);
        beg=invariantNodes.erase(beg);
      }else{
        ++beg;
      }
    }
  }
}

void LICMContext::LICM() {
  init();
  markInvariant();
  doCodeMotion();
}

} // namespace optimization
} // namespace aaac

void LICM(aaac::frontend::ControlFlowGraph *cfg,
          aaac::optimization::NaturalLoop *loop) {
  auto context = aaac::optimization::LICMContext(cfg, loop);
  context.LICM();
}
