#include "../include/backend/Mid2EndDAG.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <ostream>
#include <set>
#include <sstream>
#include <stack>
#include <vector>
#include "../include/frontend/IR.h"
#include "../include/frontend/SysYPrinter.h"
#include "../include/utils/ConstantCounter.h"

/**
 * @file Mid2EndDAG.cpp
 * @brief 中端到后端DAG的源文件
 *
 */

namespace mid2EndDAG {
void Function::sortDAGs() {
  std::map<Mid2EndDAG *, uint64_t> DAGOrder;
  std::map<Mid2EndDAG *, bool> visited;
  std::stack<Mid2EndDAG *> visitStack;
  for (const auto &DAG : DAGs) {
    visited.emplace(DAG.get(), false);
  }

  uint64_t order = 0;
  visitStack.push(DAGs.front().get());
  while (!visitStack.empty()) {
    auto curDAG = visitStack.top();
    visitStack.pop();
    visited[curDAG] = true;
    DAGOrder.emplace(curDAG, order);

    auto successors = curDAG->getSuccessors();
    constexpr auto mask = Mid2EndDAGNode::BEQ | Mid2EndDAGNode::BGE | Mid2EndDAGNode::BLT | Mid2EndDAGNode::BGEU |
                          Mid2EndDAGNode::BLTU | Mid2EndDAGNode::BNE;
    if (curDAG->getTerminatorNode() != nullptr && (curDAG->getTerminatorNode()->getNodeType() & mask) != 0) {
      auto riscvCondBrInst = dynamic_cast<Mid2EndRiscvCondBrNode *>(curDAG->getTerminatorNode());
      auto thenDAG = riscvCondBrInst->getThenDAG();
      if (!visited[thenDAG]) {
        visitStack.push(thenDAG);
      }
      for (const auto &succ : successors) {
        if (succ != thenDAG && !visited[succ]) {
          visitStack.push(succ);
        }
      }
    } else {
      for (const auto &succ : successors) {
        if (!visited[succ]) {
          visitStack.push(succ);
        }
      }
    }
    order++;
  }

  auto cmp = [&](const std::unique_ptr<Mid2EndDAG> &lhs, const std::unique_ptr<Mid2EndDAG> &rhs) -> bool {
    return DAGOrder[lhs.get()] < DAGOrder[rhs.get()];
  };
  DAGs.sort(cmp);
}

auto Function::renameDAGs(uint64_t begin) -> uint64_t {
  std::set<Mid2EndDAG *> targetDAGs;
  for (const auto &DAG : DAGs) {
    auto inst = DAG->getTerminatorNode();
    auto condBrInst = dynamic_cast<Mid2EndRiscvCondBrNode *>(inst);
    auto brInst = dynamic_cast<Mid2EndBrNode *>(inst);
    if (condBrInst != nullptr) {
      targetDAGs.insert(condBrInst->getThenDAG());
    } else if (brInst != nullptr) {
      targetDAGs.insert(brInst->getThenDAG());
    }
    targetDAGs.erase(nullptr);
  }

  uint64_t index = begin;
  std::stringstream ss;
  for (const auto &DAG : DAGs) {
    if (targetDAGs.find(DAG.get()) != targetDAGs.end()) {
      ss << ".L" << index;
      DAG->setName(ss.str());
      ss.str("");
      index++;
    }
  }
  getEntryDAG()->setName("");
  return index;
}

}  // namespace mid2EndDAG