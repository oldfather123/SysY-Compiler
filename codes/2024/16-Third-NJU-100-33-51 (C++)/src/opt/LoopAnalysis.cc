// This file includes code inspired by the LLVM Project's loop analysis implementation.
// LLVM Project: https://llvm.org
// GitHub Repository: https://github.com/llvm/llvm-project
// 
// The original code is licensed under the Apache License v2.0 with LLVM Exceptions.
// See the LICENSE.txt file in the LLVM repository for details.
// 
// Modifications and extensions have been made to integrate and adapt the implementation for our project's needs.
// 
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Original Copyright 2023 LLVM Project

#include "LoopAnalysis.hh"

bool LoopAnalysis::runOnModule(ANTPIE::Module* module) {
  for (Function* func : *module->getFunctions()) {
    runOnFunction(func);
  }
  return true;
}

bool LoopAnalysis::runOnFunction(Function* func) {
  CFG* cfg = func->getCFG();
  if (!cfg) cfg = func->buildCFG();
  DomTree* dt = func->getDT();
  if (!dt) dt = func->buildDT();

  LoopInfoBase* loopInfoBase = new LoopInfoBase();

  BBListPtr postOrderDT = dt->postOrder();

  for (BasicBlock* header : *postOrderDT) {
    vector<BasicBlock*> latches;

    for (BasicBlock* latch : *cfg->getPredOf(header)) {
      if (dt->dominates(header, latch)) {
        latches.push_back(latch);
      }
    }

    if (!latches.empty()) {
      LoopInfo* loopInfo = new LoopInfo(header);
      loopInfo->setLatches(latches);
      discoverAndMapSubloop(loopInfo, latches, loopInfoBase, dt, cfg);
      loopInfoBase->addLoopInfo(loopInfo);
      for (BasicBlock* block : loopInfo->blocks) {
        Instruction* tailInstr = block->getTailInstr();
        if (JumpInst* jumpInst = dynamic_cast<JumpInst*>(tailInstr)) {
          BasicBlock* succBlock = (BasicBlock*)jumpInst->getRValue(0);
          if (!loopInfo->containBlockInChildren(succBlock)) {
            loopInfo->addExiting(block);
            loopInfo->addExit(succBlock);
          }
        } else if (BranchInst* branchInst =
                       dynamic_cast<BranchInst*>(tailInstr)) {
          for (int i = 1; i <= 2; i++) {
            BasicBlock* succBlock = (BasicBlock*)branchInst->getRValue(i);
            if (!loopInfo->containBlockInChildren(succBlock)) {
              loopInfo->addExiting(block);
              loopInfo->addExit(succBlock);
            }
          }
        }
      }
    }
  }
  loopInfoBase->calculateDepth();
  func->setLoopInfoBase(loopInfoBase);

  loopInfoBase->analyseSimpleLoop();
  // #ifdef DEBUG_MODE
  //   loopInfoBase->dump();
  // #endif

  return true;
}

void LoopAnalysis::discoverAndMapSubloop(LoopInfo* loopInfo,
                                         vector<BasicBlock*>& latches,
                                         LoopInfoBase* li, DomTree* dt,
                                         CFG* cfg) {
  LinkedList<BasicBlock*> workList;
  for (BasicBlock* latch : latches) {
    workList.pushBack(latch);
  }
  unordered_set<BasicBlock*> visited;
  while (!workList.isEmpty()) {
    BasicBlock* pred = workList.popFront();
    if (visited.count(pred)) continue;
    visited.insert(pred);
    LoopInfo* subloop = li->getLoopOf(pred);
    if (!subloop) {
      loopInfo->addBlock(pred);
      if (pred == loopInfo->header) {
        continue;
      }
      for (BasicBlock* ppred : *cfg->getPredOf(pred)) {
        if (!loopInfo->containBlock(ppred)) {
          workList.pushBack(ppred);
        }
      }
    } else {
      subloop = subloop->getRootLoop();

      if (subloop == loopInfo) {
        continue;
      }

      loopInfo->addSubLoop(subloop);

      BasicBlock* subHeader = subloop->header;
      for (BasicBlock* shPred : *cfg->getPredOf(subHeader)) {
        if (!subloop->containBlock(shPred)) {
          workList.pushBack(shPred);
        }
      }
    }
  }
}
