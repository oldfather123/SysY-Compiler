#include "CFG.hh"

#include "Function.hh"

void CFG::addNode(BasicBlock* bb) {
  blocks.pushBack(bb);
  blkPredMap[bb] = new LinkedList<BasicBlock*>();
  blkSuccMap[bb] = new LinkedList<BasicBlock*>();
}
void CFG::addEdge(BasicBlock* src, BasicBlock* dest) {
  if (blkPredMap.find(src) == blkPredMap.end()) {
    addNode(src);
  }
  if (blkPredMap.find(dest) == blkPredMap.end()) {
    addNode(dest);
  }
  blkPredMap[dest]->pushBack(src);
  blkSuccMap[src]->pushBack(dest);
}

void CFG::eraseNode(BasicBlock* bb) {
  if (bb == exit) {
    return;
  }
  for (BasicBlock* succ : *getSuccOf(bb)) {
    blkPredMap[succ]->remove(bb);
  }
  blkSuccMap.erase(bb);
  for (BasicBlock* pred : *getPredOf(bb)) {
    blkSuccMap[pred]->remove(bb);
  }
  blkPredMap.erase(bb);
  blocks.remove(bb);
}

void CFG::eraseEdge(BasicBlock* src, BasicBlock* dest) {
  auto it = blkSuccMap.find(src);
  if (it != blkSuccMap.end()) it->second->remove(dest);
  it = blkPredMap.find(dest);
  if (it != blkPredMap.end()) it->second->remove(src);
}

// build CFG
CFG::CFG(Function* func) {
  entry = func->getEntry();
  exit = func->getExit();

  for (const auto& bb : *func->getBasicBlocks()) {
    blkPredMap[bb] = new LinkedList<BasicBlock*>();
    blkSuccMap[bb] = new LinkedList<BasicBlock*>();
    addNode(bb);
  }

  auto bbList = func->getBasicBlocks();
  for (auto bb = bbList->begin(); bb != bbList->end(); ++bb) {
    if (*bb == exit) {
      continue;
    }
    Instruction* tailInstr = (*bb)->getTailInstr();
    if (!tailInstr) {
      addEdge(*bb, *(bb + 1));
      continue;
    }
    ValueTag tailType = tailInstr->getValueTag();
    if (tailType == VT_BR) {
      BranchInst* brInstr = static_cast<BranchInst*>(tailInstr);
      BasicBlock* trueBB = static_cast<BasicBlock*>(brInstr->getRValue(1));
      BasicBlock* falseBB = static_cast<BasicBlock*>(brInstr->getRValue(2));
      addEdge(*bb, trueBB);
      addEdge(*bb, falseBB);
    } else if (tailType == VT_JUMP) {
      JumpInst* jumpInst = static_cast<JumpInst*>(tailInstr);
      BasicBlock* destBB = static_cast<BasicBlock*>(jumpInst->getRValue(0));
      addEdge(*bb, destBB);
    } else if (tailType == VT_RET) {
      addEdge(*bb, exit);
    } else {
      addEdge(*bb, *(bb + 1));
    }
  }
  // debug();
}

void CFG::debug() {
  for (const auto& bb : blocks) {
    std::cout << bb->getName() << ":" << std::endl;
    BBListPtr preds = getPredOf(bb);
    BBListPtr succs = getSuccOf(bb);
    for (const auto& pbb : *preds) {
      std::cout << pbb->getName() << " ";
    }
    std::cout << std::endl;
    for (const auto& sbb : *succs) {
      std::cout << sbb->getName() << " ";
    }
    std::cout << std::endl;
  }
}

void CFG::draw() {
  vector<std::pair<string, string>> edges;
  for (auto& item : blkSuccMap) {
    string src = item.first->getName();
    for (auto& dItem : *item.second) {
      edges.push_back({src, dItem->getName()});
    }
  }
  visualizeGraph(edges);
}