/* implementation of a PASS to simplify CFG*/

#include "ADT/CFG.h"
#include "common.h"
#include "frontend.h"
#include "sym.h"
#include <algorithm>
#include <array>
#include <memory>
#include <set>
#include <unordered_set>
namespace aaac {
using namespace frontend;
namespace optimization {

class CFGSimplifyContext {
private:
  frontend::ControlFlowGraph *cfg;
  void removeUnreachableBB();
  void removeRedundantPhi();
  void joinOneToOneBB();
  void removeOnlyJump();
  void removeUnusedLabel();

public:
  void simplifyCFG();
  void simplifyCFG_AfterSSA();
  CFGSimplifyContext(frontend::ControlFlowGraph *cfg) : cfg(cfg) {};
};

void CFGSimplifyContext::removeUnreachableBB() {
  std::unordered_set<std::shared_ptr<frontend::BasicBlock>> visited;
  ControlFlowGraph::TraverseHandler handler =
      [&](std::shared_ptr<sym::Function> func, std::shared_ptr<BasicBlock> bb) {
        visited.insert(bb);
      };
  cfg->forwardTraverse(handler, ControlFlowGraph::dummyHandler);
  for (auto it = cfg->getBBList().begin(); it != cfg->getBBList().end();) {
    auto bb = *it;
    if (visited.count(bb)) {
      it++;
      continue;
    }
    auto edge_list = bb->getPredEdges();
    for (auto pred_edge : edge_list) {
      auto pred = pred_edge.getDest();
      pred->disconnect(bb);
    }
    edge_list = bb->getSuccEdges();
    for (auto succ_edge : edge_list) {
      auto succ = succ_edge.getDest();
      bb->disconnect(succ);
      succ->removeFromInPhi(bb);
    }
    // std::cout << "remove unreachable BB " << bb->getLabel()->getName() << "\n";
    it = cfg->removeBasicBlock(bb);
  }
}

void CFGSimplifyContext::removeRedundantPhi() {
  for (auto bb : cfg->getBBList()) {
    auto it = bb->getLastLabel();
    it++;
    while ((*it)->getCode() == NodeCode::Phi) {
      auto phi = std::dynamic_pointer_cast<PhiNode>(*it);
      if (phi->getOperands().size() == 1) {
        auto def = phi->getDestVar();
        Operand var = phi->getUses().front();
        it = bb->removeNode(it);
        it = bb->insertNodeBefore(it, 
                    CopyAssignNode::create(def, OperandList({var})));
      }
      it++;
    }
  }
}

void CFGSimplifyContext::joinOneToOneBB() {
  std::unordered_set<std::shared_ptr<frontend::BasicBlock>> targetBBs;
  for (auto bb : cfg->getBBList()) {
    // one-one BB
    auto can_join = [&]() -> bool {
      if (bb != cfg->getStartBasicBlock() && // 防止开头的循环
          bb->getPredEdges().size() == 1 &&
          bb->getPredEdge().getDest()->getSuccEdges().size() == 1) {
        // 检查当前块的phi是否包含前驱
        for (auto it = bb->begin(); it != bb->end(); it++) {
          auto node = *it;
          auto pred = bb->getPredEdge().getDest();
          if (auto phi = std::dynamic_pointer_cast<PhiNode>(node)) {
            // 由于当前基本块只有一个前驱，此时的phi节点相当于一个赋值
            auto def = phi->getDestVar();
            Operand var = phi->getOperands().at(pred.get());
            it = bb->removeNode(it);
            it = bb->insertNodeBefore(
                it, CopyAssignNode::create(def, OperandList({var})));
          }
        }
        return true;
      }
      return false;
    };
    if (can_join()) {
      targetBBs.insert(bb);
    }
  }

  for (auto curBB : targetBBs) {
    auto pred_edge = curBB->getPredEdge();
    auto pred = pred_edge.getDest();
    BasicBlock::EdgeFlag flag = pred->getEdgeFlag(curBB);
    auto cur_jump =
        std::dynamic_pointer_cast<JumpNode>(*std::prev(curBB->end()));
    pred->disconnect(curBB);
    for (auto succ : curBB->getSuccEdges()) {
      pred->connect(succ.getDest(), succ.getFlag());
      succ.getDest()->replaceFromInPhi(curBB, pred);
    }
    while (curBB->getSuccEdges().size()) {
      curBB->disconnect(curBB->getSuccEdge().getDest());
    }
    assert(flag == BasicBlock::EdgeFlag::JUMP);
    auto pred_jump = std::prev(pred->end());
    pred->removeNode(pred_jump);
    for (auto node : *curBB) {
      if (node->isLabel()) {
        pred->push_front(node);
      } else if (node->getCode() == NodeCode::Phi) {
        auto lastlabel = pred->begin();
        while (lastlabel != pred->end() && (*lastlabel)->isLabel())
          lastlabel++;
        pred->insertNodeBefore(lastlabel, node);
      } else {
        pred->push_back(node);
      }
    }
    if (cfg->getExitBasicBlock() == curBB) {
      cfg->setExitBasicBlock(pred);
    }
    // std::cout << "remove 1-1 BB " << curBB->getLabel()->getName() << "\n";
    cfg->removeBasicBlock(curBB);
  }
}

// TODO: buggy
void CFGSimplifyContext::removeOnlyJump() {
  /* 上一个函数中已经将所有的一对一函数进行了合并
    这个函数中将处理只包含Jump的BB的情况
    1. 可能存在多个前驱
    2. 前驱的CTI可能是ConditionNode
            ConditionNode
            /           \
          Jump         Other
            |
            BB
  */
  auto is_only_jump = [this](std::shared_ptr<BasicBlock> bb) -> bool {
    if (cfg->getStartBasicBlock() == bb) {
      return false;
    }
    for (auto insn : *bb) {
      if (insn->isLabel()) continue;
      if (insn->getCode() == NodeCode::Jump) {
        return true;
      }
      return false;
    }
  };

  std::unordered_set<std::shared_ptr<BasicBlock>> targets;
  for (auto bb : cfg->getBBList()) {
    if (is_only_jump(bb)) {
      targets.insert(bb);
    }
  }
  for (auto cur : targets) {
    // std::cout << "only-jump target " << cur->getLabel()->getName() << "\n";
    auto succ_edge = cur->getSuccEdge(); // Jump只有一个succ
    auto succ = succ_edge.getDest();
    auto preds = cur->getPredEdges();
    for (auto pred_edge : preds) {
      auto pred = pred_edge.getDest();
      auto flag = pred->getEdgeFlag(cur);
      if (flag == BasicBlock::EdgeFlag::JUMP) {
        pred->disconnect(cur);
        pred->connect(succ, flag);
        succ->replaceFromInPhi(cur, pred);
        auto jump = std::dynamic_pointer_cast<JumpNode>(*std::prev(pred->end()));
        jump->patchLabel(succ->getLabel());
        // std::cout << "replace jump BB " << pred->getLabel()->getName() << "\n";
      } else if (flag == BasicBlock::EdgeFlag::THEN || flag == BasicBlock::EdgeFlag::ELSE) {
        auto another_flag = flag == BasicBlock::EdgeFlag::THEN? 
                      BasicBlock::EdgeFlag::ELSE: BasicBlock::EdgeFlag::THEN;
        BasicBlock::Edge another_edge;
        std::shared_ptr<BasicBlock> another;
        for (auto edge : pred->getSuccEdges()) {
          if (edge.getFlag() == another_flag) {
            another_edge = edge;
            break;
          }
        }
        another = another_edge.getDest();
        auto it = succ->getLastLabel();
        it++;
        bool can_replace = true;
        while((*it)->getCode() == NodeCode::Phi) {
          auto phi = std::dynamic_pointer_cast<PhiNode>(*it);
          int count = 0;
          for (auto &[bb, opnd] : phi->getOperands()) {
            if (bb == cur.get() || bb == another.get()) {
              count++;
            }
          }
          if (count >= 1) {
            can_replace = false;
          }
          it++;
        }
        if (can_replace) {
          pred->disconnect(cur);
          pred->connect(succ,flag);
          succ->replaceFromInPhi(cur, pred);
          auto cond = std::dynamic_pointer_cast<ConditionNode>(*(std::prev(pred->end())));
          if (flag == BasicBlock::EdgeFlag::THEN) {
            cond->patchTrueLabel(succ->getLabel());
          } else {
            cond->patchFalseLabel(succ->getLabel());
          }
          // std::cout << "replace cond BB " << pred->getLabel()->getName() << "\n";
        }
      } else {
        assert(false);
      }
    }

    if (cur->getPredEdges().size() == 0) {
      // std::cout << "remove only-jump BB " << cur->getLabel()->getName() << "\n";
      if (cur == cfg->getStartBasicBlock()) {
        cfg->setStartBasicBlock(succ);
      }
      cur->disconnect(succ);
      cfg->removeBasicBlock(cur);
    }
  }
}

void CFGSimplifyContext::removeUnusedLabel() {
  std::map<std::string, std::string> label_map;
  for (auto bb : cfg->getBBList()) {
    std::string master = bb->getLabel()->getName();
    auto it = bb->begin();
    while ((*it)->getCode() == frontend::NodeCode::Label) {
      auto label = std::static_pointer_cast<LabelNode>(*it);
      // label_map[master] = label->getName();
      label_map[label->getName()] = master;
      it++;
      std::cout << master << " -> " << label->getName() << "\n";
    }
    it = bb->begin();
    it++;
    while ((*it)->getCode() == frontend::NodeCode::Label) {
      it = bb->removeNode(it);
    }
  }
  for (auto bb : cfg->getBBList()) {
    auto last = *std::prev(bb->end());
    if (auto jump = std::dynamic_pointer_cast<JumpNode>(last)) {
      auto label = label_map[jump->getLabel()->getName()];
      jump->patchLabel(LabelNode::create(label));
    } else if (auto cond = std::dynamic_pointer_cast<ConditionNode>(last)) {
      auto true_ = label_map[cond->getTrueLabel()->getName()];
      cond->patchTrueLabel(LabelNode::create(true_));
      auto false_ = label_map[cond->getFalseLabel()->getName()];
      cond->patchFalseLabel(LabelNode::create(false_));
    }
  }
  
}

void CFGSimplifyContext::simplifyCFG() {
  removeUnreachableBB();
  // removeRedundantPhi();
  joinOneToOneBB();
  // removeOnlyJump();
}

void CFGSimplifyContext::simplifyCFG_AfterSSA() {
  // removeUnreachableBB();
  // removeRedundantPhi();
  joinOneToOneBB();
  removeOnlyJump();
  removeUnusedLabel();
}

} // namespace optimization
} // namespace aaac

void simplifyCFG(aaac::frontend::ControlFlowGraph *cfg) {
  auto context = aaac::optimization::CFGSimplifyContext(cfg);
  context.simplifyCFG();
}

void simplifyCFG_AfterSSA(aaac::frontend::ControlFlowGraph *cfg) {
  auto context = aaac::optimization::CFGSimplifyContext(cfg);
  context.simplifyCFG_AfterSSA();
}
