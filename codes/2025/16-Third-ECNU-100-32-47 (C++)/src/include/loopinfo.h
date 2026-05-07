#ifndef AAAC_LOOPINFO_H
#define AAAC_LOOPINFO_H

#include "ADT/CFG.h"
#include "frontend.h"
#include <map>
#include <memory>
#include <set>
#include <utility>
namespace aaac {
namespace optimization {

class NaturalLoop {
private:
  frontend::BasicBlock *head;
  std::set<frontend::BasicBlock *> tail;
  std::set<aaac::frontend::BasicBlock *> loopBlocks;
  NaturalLoop *parent;
  std::set<std::unique_ptr<NaturalLoop>> nestedLoops;

public:
  NaturalLoop() = default;
  NaturalLoop(frontend::BasicBlock *head,
              const std::set<frontend::BasicBlock *> &tail)
      : head(head), tail(tail), loopBlocks(), parent(), nestedLoops(){};
  NaturalLoop *getParentLoop() { return parent; }
  frontend::BasicBlock *getLoopHead() { return head; }
  const std::set<frontend::BasicBlock *> &getLoopTail() { return this->tail; }
  void setLoopTail(const std::set<frontend::BasicBlock *> &tail) {
    this->tail = tail;
  }
  void setParentLoop(NaturalLoop *parent) { this->parent = parent; }
  void insertLoopNode(frontend::BasicBlock *bb) { this->loopBlocks.insert(bb); }
  const std::set<frontend::BasicBlock *> &getLoopNodes() {
    return this->loopBlocks;
  }
  void addNestedLoop(std::unique_ptr<NaturalLoop> nestedLoop) {
    nestedLoop->setParentLoop(this);
    this->nestedLoops.insert(std::move(nestedLoop));
  };
  std::set<NaturalLoop *> getNestedLoops() {
    std::set<NaturalLoop *> loops;
    for (auto &ptr : nestedLoops) {
      loops.insert(ptr.get());
    }
    return loops;
  }
  std::map<frontend::BasicBlock *, frontend::BasicBlock::Edge>
  getExitingEdges() {
    std::map<frontend::BasicBlock *, frontend::BasicBlock::Edge> edges;
    for (auto bb : this->getLoopNodes()) {
      for (auto succ_edge : bb->getSuccEdges()) {
        if (this->getLoopNodes().count(succ_edge.getDest().get()) == 0) {
          edges[bb] = succ_edge;
        }
      }
    }
    return edges;
  }
};

class NaturalLoopAnalyzer {
private:
  frontend::ControlFlowGraph *cfg;

public:
  NaturalLoopAnalyzer(frontend::ControlFlowGraph *cfg) : cfg(cfg){};
  std::set<std::unique_ptr<NaturalLoop>> getAllLoops();
};

} // namespace optimization
} // namespace aaac

#endif
