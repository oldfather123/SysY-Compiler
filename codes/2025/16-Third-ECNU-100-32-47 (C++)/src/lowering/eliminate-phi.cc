#include "ADT/CFG.h"
#include "common.h"
#include "frontend.h"
#include "sym.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <utility>

namespace aaac {
namespace frontend {
struct BBPairHash {
  size_t operator()(const std::pair<BasicBlock *, BasicBlock *> &p) const {
    size_t seed = 0;
    seed ^= std::hash<BasicBlock *>{}(p.first) + 0x9e3779b9 + (seed << 6) +
            (seed >> 2);
    seed ^= std::hash<BasicBlock *>{}(p.second) + 0x9e3779b9 + (seed << 6) +
            (seed >> 2);
    return seed;
  }
};

void ControlFlowGraph::eliminatePhi() {
  std::unordered_map<std::pair<BasicBlock *, BasicBlock *>, BasicBlock *,
                     BBPairHash>
      edgeToBB;
  auto origin_bbs=this->bbs;
  auto insertCopies = [this, &edgeToBB](std::shared_ptr<BasicBlock> bb,
                                        std::shared_ptr<PhiNode> phi) {
    auto destVar = phi->getDestVar();
    for (auto [from, srcVar] : phi->getOperands()) {
      auto edgePair = std::make_pair(from, bb.get());
      BasicBlock *insertBB = nullptr;
      // 先前尚未在edgeToBB中“缓存”
      if (edgeToBB.find(edgePair) == edgeToBB.end()) {
        // Critical Edge Split
        if (from->getSuccEdges().size() > 1 && bb->getPredEdges().size() > 1) {
          auto phiBB = this->createBasicBlock();
          phiBB->push_back(LabelNode::generateTemp("CES_"));
          phiBB->push_back(JumpNode::create(bb->getLabel()));
          edgeToBB[std::make_pair(from, bb.get())] = phiBB.get();
        } else {
          edgeToBB[std::make_pair(from, bb.get())] = from;
        }
      }

      insertBB = edgeToBB[edgePair];
      Assert(insertBB, "insertBB is null");
      // undef
      std::visit([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::shared_ptr<aaac::sym::Var>>) {
          if (arg != nullptr) {
            insertBB->insertNodeBefore(
                --insertBB->end(),
                frontend::CopyAssignNode::create(destVar, OperandList{arg}));
          }
        }else{ // int and float
            insertBB->insertNodeBefore(
                --insertBB->end(),
                frontend::CopyAssignNode::create(destVar, OperandList{arg}));
        }
      }, srcVar);
    }
  };
  for (auto &bb : origin_bbs) {
    for (auto node = bb->begin(); node != bb->end();) {
      if (auto phiNode = std::dynamic_pointer_cast<frontend::PhiNode>(*node)) {
        insertCopies(bb, phiNode);
        node = bb->removeNode(node);
      } else {
        ++node;
      }
    }
  }

  // 将新建的BB与已经在CFG中的BB建立连接
  for (auto [edgePair, phibb_idx] : edgeToBB) {
    auto phiBB_idx = phibb_idx;
    auto from_idx = edgePair.first;
    auto to_idx = edgePair.second;
    if (from_idx != phiBB_idx) {
      Assert(from_idx->back()->getCode() == NodeCode::Condition,
             "Pred doesn't have multiple successors.");
      auto from = std::find_if(bbs.begin(), bbs.end(), [from_idx](auto &bb) {
        return bb.get() == from_idx;
      });
      auto phiBB = std::find_if(bbs.begin(), bbs.end(), [phiBB_idx](auto &bb) {
        return bb.get() == phiBB_idx;
      });
      auto to = std::find_if(bbs.begin(), bbs.end(),
                             [to_idx](auto &bb) { return bb.get() == to_idx; });
      Assert(from != bbs.end() && phiBB != bbs.end() && to != bbs.end(),
             "Invalid edge or block");
      auto edgelist = (*from)->getSuccEdges();
      auto edgeIter = std::find_if(
          edgelist.begin(), edgelist.end(),
          [to](BasicBlock::Edge &edge) { return edge.getDest() == *to; });
      Assert(edgeIter != (*from)->getSuccEdges().end(), "Invalid edge");
      auto lastNode =
          std::dynamic_pointer_cast<ConditionNode>(*(--(*from)->end()));

      // true_label -> THEN edge
      // false_label -> ELSE edge
      if (edgeIter->getFlag() == BasicBlock::EdgeFlag::THEN) {
        lastNode->patchTrueLabel((*phiBB)->getLabel());
      } else {
        lastNode->patchFalseLabel((*phiBB)->getLabel());
      }

      (*from)->connect(*phiBB, edgeIter->getFlag());
      (*phiBB)->connect(*to, BasicBlock::EdgeFlag::JUMP);
      (*from)->disconnect(*to);
    }
  }
}
} // namespace frontend
} // namespace aaac