#include "ADT/CFG.h"
#include "common.h"
#include "frontend.h"
#include "loopinfo.h"
#include "sym.h"
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace aaac {
namespace optimization {
static frontend::BasicBlock *
splitPredecessor(frontend::ControlFlowGraph *cfg, frontend::BasicBlock *bb,
                 const std::set<frontend::BasicBlock *> &preds);
class LoopSimplifyContext {
private:
  frontend::ControlFlowGraph *cfg;
  NaturalLoop *loop;

  void addPreheadBlock();
  void unifyExitBlock();
  void unifyBackEdge();
  void updateLoopNodes(NaturalLoop *lowest, frontend::BasicBlock *bb);

public:
  LoopSimplifyContext(frontend::ControlFlowGraph *cfg, NaturalLoop *loop)
      : cfg(cfg), loop(loop){};
  void LoopSimplify();
};

static std::map<frontend::BasicBlock *, frontend::BasicBlock::Edge>
getExitingEdges(NaturalLoop *loop) {
  return loop->getExitingEdges();
}

void LoopSimplifyContext::LoopSimplify() {
  this->addPreheadBlock();
  this->unifyExitBlock();
  this->unifyBackEdge();
}

void LoopSimplifyContext::addPreheadBlock() {
  auto head = loop->getLoopHead();

  // 收集所有前驱块
  std::set<frontend::BasicBlock *> preds;
  for (auto pred_edge : head->getPredEdges()) {
    // 避免把BackEdge加入进来
    if (loop->getLoopNodes().count(pred_edge.getDest().get()) == 0)
      preds.insert(pred_edge.getDest().get());
  }

  frontend::BasicBlock *prehead = nullptr;
  if (preds.size() == 0) {
    prehead = cfg->createBasicBlock().get();
    prehead->connect(head->shared_from_this(),
                     frontend::BasicBlock::EdgeFlag::JUMP);
    prehead->push_back(frontend::JumpNode::create(head->getLabel()));
    cfg->setStartBasicBlock(prehead->shared_from_this());
  } else {
    prehead = splitPredecessor(cfg, head, preds);
  }

  // 添加prehead标签
  auto prehead_label = frontend::LabelNode::generateTemp("prehead_");
  prehead->push_front(prehead_label);

  this->updateLoopNodes(loop->getParentLoop(), prehead);
}

void LoopSimplifyContext::unifyExitBlock() {
  auto exitingEdges = getExitingEdges(this->loop);
  if (exitingEdges.empty())
    return;

  // 按exit target分组
  std::map<frontend::BasicBlock *, std::set<frontend::BasicBlock *>>
      exit_to_exitings;
  for (auto &[exitingBB, edge] : exitingEdges) {
    auto exitBB = edge.getDest();
    // auto exitTarget = exitingBB->getSuccEdges()[0].getDest();
    // exit_to_exitings[exitTarget.get()].insert(exitingBB.get());
    exit_to_exitings[exitBB.get()].insert(exitingBB);
  }

  // 为每个exit target使用SplitPredecessor分割前驱
  for (auto &[exitTarget, exitingBBs] : exit_to_exitings) {
    auto newBB = splitPredecessor(this->cfg, exitTarget, exitingBBs);
    // exit不一定算循环内的节点，甚至连外层循环也不一定，例如：直接return
    auto current = loop->getParentLoop();
    while (current != nullptr) {
      if (current->getLoopNodes().count(exitTarget)) {
        current->insertLoopNode(newBB);
      }
      current = current->getParentLoop();
    }
  }
}

void LoopSimplifyContext::unifyBackEdge() {
  // 所有的回边首先需要跳转到这里创建的一个新的BasicBlock
  auto tails = loop->getLoopTail();
  auto head = loop->getLoopHead();
  // 然后再通过这个BasicBlock跳转到head
  auto new_tail = splitPredecessor(cfg, head, tails);
  loop->setLoopTail({new_tail});
  updateLoopNodes(loop, new_tail);
}

frontend::BasicBlock *
splitPredecessor(frontend::ControlFlowGraph *cfg, frontend::BasicBlock *bb,
                 const std::set<frontend::BasicBlock *> &preds) {
  if (preds.empty())
    return nullptr;

  // 创建新的中间基本块
  auto midBB = cfg->createBasicBlock();
  auto midLabel = frontend::LabelNode::generateTemp("split_");
  midBB->push_back(midLabel);
  midBB->push_back(frontend::JumpNode::create(bb->getLabel()));

  midBB->connect(bb->shared_from_this(), frontend::BasicBlock::JUMP);

  // 处理Phi函数
  auto iter = bb->begin();
  while (iter != bb->end()) {
    if ((*iter)->getCode() != frontend::NodeCode::Phi) {
      ++iter;
      continue;
    }

    auto phi = std::dynamic_pointer_cast<frontend::PhiNode>(*iter);
    if (preds.size() >
        1) { // 只有在preds.size>1时才有创建新的phi节点的必要，否则直接修改srcBlock即可
      auto phi_var = sym::Var::generateTemp(phi->getDestVar()->getType(),
                                            phi->getDestVar()->getName());
      auto new_phi = frontend::PhiNode::create(phi_var);

      for (auto pred : preds) {
        Assert(pred->getEdgeFlag(bb->shared_from_this()) !=
                   frontend::BasicBlock::ILLEGAL,
               "Not a predecessor");
        auto val = phi->getValueBySrcBlock(pred);
        new_phi->addOperand(val, pred->shared_from_this());
        phi->removeOperandByBasicBlock(pred);
      }
      phi->addOperand(phi_var, midBB);
      midBB->insertNodeAfter(midBB->begin(), new_phi);
    } else if (preds.size() == 1) { // preds.size() == 1
      auto pred = *preds.begin();
      Assert(pred->getEdgeFlag(bb->shared_from_this()) !=
                 frontend::BasicBlock::ILLEGAL,
             "Not a predecessor");
      auto val = phi->getValueBySrcBlock(pred);
      phi->removeOperandByBasicBlock(pred);
      phi->addOperand(val, midBB->shared_from_this());
    }
    ++iter;
  }

  // 重定向前驱块
  for (auto pred : preds) {
    auto lastNode = pred->back();
    auto flag = pred->getEdgeFlag(bb->shared_from_this());
    pred->connect(midBB, flag);
    pred->disconnect(bb->shared_from_this());

    if (auto jump = std::dynamic_pointer_cast<frontend::JumpNode>(lastNode)) {
      *jump = frontend::JumpNode(midLabel);
    } else if (auto cond = std::dynamic_pointer_cast<frontend::ConditionNode>(
                   lastNode)) {
      // 不可以使用下面这种方式比较
      // 因为当前一个基本块可能对应很多Label
      // FIXME:统一LabelNode或者改进getLabel函数
      // if (cond->getTrueLabel() == bb->getLabel()) {
      if (flag == frontend::BasicBlock::THEN) {
        cond->patchTrueLabel(midLabel);
      } else {
        cond->patchFalseLabel(midLabel);
      }
    } else {
      Assert(false, "Invalid Flag");
    }
  }

  return midBB.get();
}

void LoopSimplifyContext::updateLoopNodes(NaturalLoop *lowest,
                                          frontend::BasicBlock *bb) {
  auto current = lowest;
  while (current != nullptr) {
    current->insertLoopNode(bb);
    current = current->getParentLoop();
  }
}

} // namespace optimization
} // namespace aaac

void LoopSimplify(aaac::frontend::ControlFlowGraph *cfg,
                  aaac::optimization::NaturalLoop *loop) {
  auto context = aaac::optimization::LoopSimplifyContext(cfg, loop);
  context.LoopSimplify();
}
