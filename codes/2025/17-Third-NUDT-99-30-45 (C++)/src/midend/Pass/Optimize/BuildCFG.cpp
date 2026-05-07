#include "BuildCFG.h"
#include "Dom.h"
#include "Liveness.h"
#include <iostream>
#include <queue>
#include <set>

namespace sysy {

void *BuildCFG::ID = (void *)&BuildCFG::ID; // 定义唯一的 Pass ID

// 声明Pass的分析使用
void BuildCFG::getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const {
  // BuildCFG不依赖其他分析
  // analysisDependencies.insert(&DominatorTreeAnalysisPass::ID); // 错误的例子

  // BuildCFG会使所有依赖于CFG的分析结果失效，所以它必须声明这些失效
  analysisInvalidations.insert(&DominatorTreeAnalysisPass::ID);
  analysisInvalidations.insert(&LivenessAnalysisPass::ID);
}

bool BuildCFG::runOnFunction(Function *F, AnalysisManager &AM) {
  if (DEBUG) {
    std::cout << "Running BuildCFG pass on function: " << F->getName() << std::endl;
  }

  bool changed = false;

  // 1. 清空所有基本块的前驱和后继列表
  for (auto &bb : F->getBasicBlocks()) {
    bb->clearPredecessors();
    bb->clearSuccessors();
  }

  // 2. 遍历每个基本块，重建CFG
  for (auto &bb : F->getBasicBlocks()) {
    // 获取基本块的最后一条指令
    auto &inst = *bb->terminator();
    Instruction *termInst = inst.get();
    // 确保基本块有终结指令
    if (!termInst) {
      continue;
    }

    // 根据终结指令类型，建立前驱后继关系
    if (termInst->isBranch()) {
      // 无条件跳转
      if (termInst->isUnconditional()) {
        auto brInst = dynamic_cast<UncondBrInst *>(termInst);
        BasicBlock *succ = dynamic_cast<BasicBlock *>(brInst->getBlock());
        assert(succ && "Branch instruction's target must be a BasicBlock");
        bb->addSuccessor(succ);
        succ->addPredecessor(bb.get());
        changed = true;

        // 条件跳转
      } else if (termInst->isConditional()) {
        auto brInst = dynamic_cast<CondBrInst *>(termInst);
        BasicBlock *trueSucc = dynamic_cast<BasicBlock *>(brInst->getThenBlock());
        BasicBlock *falseSucc = dynamic_cast<BasicBlock *>(brInst->getElseBlock());

        assert(trueSucc && falseSucc && "Branch instruction's targets must be BasicBlocks");

        bb->addSuccessor(trueSucc);
        trueSucc->addPredecessor(bb.get());
        bb->addSuccessor(falseSucc);
        falseSucc->addPredecessor(bb.get());
        changed = true;
      }
    } else if (auto retInst = dynamic_cast<ReturnInst *>(termInst)) {
      // RetInst没有后继，无需处理
      // ...
    }
  }

  return changed;
}

} // namespace sysy