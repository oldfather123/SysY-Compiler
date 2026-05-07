#include "SysYIRCFGOpt.h"
#include "SysYIROptUtils.h"
#include <cassert>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <queue> // 引入队列，SysYDelNoPreBLock需要
#include <string>

namespace sysy {

// 定义静态ID
void *SysYDelInstAfterBrPass::ID = (void *)&SysYDelInstAfterBrPass::ID;
void *SysYDelEmptyBlockPass::ID = (void *)&SysYDelEmptyBlockPass::ID;
void *SysYDelNoPreBLockPass::ID = (void *)&SysYDelNoPreBLockPass::ID;
void *SysYBlockMergePass::ID = (void *)&SysYBlockMergePass::ID;
void *SysYAddReturnPass::ID = (void *)&SysYAddReturnPass::ID;
void *SysYCondBr2BrPass::ID = (void *)&SysYCondBr2BrPass::ID;

// ======================================================================
// SysYCFGOptUtils: 辅助工具类，包含实际的CFG优化逻辑
// ======================================================================

// 删除br后的无用指令
bool SysYCFGOptUtils::SysYDelInstAfterBr(Function *func) {
  bool changed = false;

  auto basicBlocks = func->getBasicBlocks();
  for (auto &basicBlock : basicBlocks) {
    bool Branch = false;
    auto &instructions = basicBlock->getInstructions();
    auto Branchiter = instructions.end();
    for (auto iter = instructions.begin(); iter != instructions.end(); ++iter) {
      if ((*iter)->isTerminator()) {
        Branch = true;
        Branchiter = iter;
        break;
      }
    }
    if (Branchiter != instructions.end())
      ++Branchiter;
    while (Branchiter != instructions.end()) {
      changed = true;
      Branchiter = SysYIROptUtils::usedelete(Branchiter); // 删除指令
    }

    if (Branch) { // 更新前驱后继关系
      auto thelastinstinst = basicBlock->terminator();
      auto &Successors = basicBlock->getSuccessors();
      for (auto iterSucc = Successors.begin(); iterSucc != Successors.end();) {
        (*iterSucc)->removePredecessor(basicBlock.get());
        basicBlock->removeSuccessor(*iterSucc);
      }
      if (thelastinstinst->get()->isUnconditional()) {
        auto brinst = dynamic_cast<UncondBrInst *>(thelastinstinst->get());
        BasicBlock *branchBlock = dynamic_cast<BasicBlock *>(brinst->getBlock());
        basicBlock->addSuccessor(branchBlock);
        branchBlock->addPredecessor(basicBlock.get());
      } else if (thelastinstinst->get()->isConditional()) {
        auto brinst = dynamic_cast<CondBrInst *>(thelastinstinst->get());
        BasicBlock *thenBlock = dynamic_cast<BasicBlock *>(brinst->getThenBlock());
        BasicBlock *elseBlock = dynamic_cast<BasicBlock *>(brinst->getElseBlock());
        basicBlock->addSuccessor(thenBlock);
        basicBlock->addSuccessor(elseBlock);
        thenBlock->addPredecessor(basicBlock.get());
        elseBlock->addPredecessor(basicBlock.get());
      }
    }
  }

  return changed;
}

// 合并基本块
bool SysYCFGOptUtils::SysYBlockMerge(Function *func) {
  bool changed = false;

  for (auto blockiter = func->getBasicBlocks().begin(); blockiter != func->getBasicBlocks().end();) {
    // 检查当前块是是不是entry块
    if( blockiter->get() == func->getEntryBlock() ) {
      blockiter++;
      continue; // 跳过入口块
    }
    if (blockiter->get()->getNumSuccessors() == 1) {
      // 如果当前块只有一个后继块
      // 且后继块只有一个前驱块
      // 则将当前块和后继块合并
      if (((blockiter->get())->getSuccessors()[0])->getNumPredecessors() == 1) {
        // std::cout << "merge block: " << blockiter->get()->getName() << std::endl;
        BasicBlock *block = blockiter->get();
        BasicBlock *nextBlock = blockiter->get()->getSuccessors()[0];
        // auto nextarguments = nextBlock->getArguments();
        // 删除block的br指令
        if (block->getNumInstructions() != 0) {
          auto thelastinstinst = block->terminator();
          if (thelastinstinst->get()->isUnconditional()) {
            thelastinstinst = SysYIROptUtils::usedelete(thelastinstinst);
          } else if (thelastinstinst->get()->isConditional()) {
            // 按道理不会走到这个分支
            // 如果是条件分支，查看then else是否相同
            auto brinst = dynamic_cast<CondBrInst *>(thelastinstinst->get());
            if (brinst->getThenBlock() == brinst->getElseBlock()) {
              thelastinstinst = SysYIROptUtils::usedelete(thelastinstinst);
            }
            else{
              assert(false && "SysYBlockMerge: unexpected conditional branch with different then and else blocks");
            }
          }
        }
        // 将后继块的指令移动到当前块
        // 并将后继块的父指针改为当前块
        for (auto institer = nextBlock->begin(); institer != nextBlock->end();) {
          // institer->get()->setParent(block);
          // block->getInstructions().emplace_back(institer->release());
          // 用usedelete删除会导致use关系被删除我只希望移动指令到当前块
          // institer = SysYIROptUtils::usedelete(institer);
          // institer = nextBlock->getInstructions().erase(institer);
          institer = nextBlock->moveInst(institer, block->getInstructions().end(), block);
          
        }
        // 更新前驱后继关系，类似树节点操作
        block->removeSuccessor(nextBlock);
        nextBlock->removePredecessor(block);
        std::list<BasicBlock *> succshoulddel;
        for (auto &succ : nextBlock->getSuccessors()) {
          block->addSuccessor(succ);
          succ->replacePredecessor(nextBlock, block);
          succshoulddel.push_back(succ);
        }
        for (auto del : succshoulddel) {
          nextBlock->removeSuccessor(del);
        }

        func->removeBasicBlock(nextBlock);
        changed = true;

      } else {
        blockiter++;
      }
    } else {
      blockiter++;
    }
  }

  return changed;
}

// 删除无前驱块，兼容SSA后的处理
bool SysYCFGOptUtils::SysYDelNoPreBLock(Function *func) {
  bool changed = false;                   // 标记是否有基本块被删除
  std::set<BasicBlock *> reachableBlocks; // 用于存储所有可达的基本块
  std::queue<BasicBlock *> blockQueue;    // BFS 遍历队列

  BasicBlock *entryBlock = func->getEntryBlock();
  if (entryBlock) {                     // 确保函数有入口块
    reachableBlocks.insert(entryBlock); // 将入口块标记为可达
    blockQueue.push(entryBlock);        // 入口块入队
  }
  // 如果没有入口块（比如一个空函数），则没有块是可达的，所有块都将被删除。

  while (!blockQueue.empty()) { // BFS 遍历：只要队列不空
    BasicBlock *currentBlock = blockQueue.front();
    blockQueue.pop(); // 取出当前块

    for (auto &succ : currentBlock->getSuccessors()) { // 遍历当前块的所有后继
      // 如果后继块不在 reachableBlocks 中（即尚未被访问过）
      if (reachableBlocks.find(succ) == reachableBlocks.end()) {
        reachableBlocks.insert(succ); // 标记为可达
        blockQueue.push(succ);        // 入队，以便继续遍历
      }
    }
  }

  std::vector<BasicBlock *> blocksToDelete; // 用于存储所有不可达的基本块

  for (auto &blockPtr : func->getBasicBlocks()) {
    BasicBlock *block = blockPtr.get();
    // 如果当前块不在 reachableBlocks 集合中，说明它是不可达的
    if (reachableBlocks.find(block) == reachableBlocks.end()) {
      blocksToDelete.push_back(block); // 将其加入待删除列表
      changed = true;                  // 只要找到一个不可达块，就说明函数发生了改变
    }
  }

  for (BasicBlock *unreachableBlock : blocksToDelete) {
    // 遍历不可达块中的所有指令，并删除它们
    for (auto instIter = unreachableBlock->getInstructions().begin();
         instIter != unreachableBlock->getInstructions().end();) {
      instIter = SysYIROptUtils::usedelete(instIter);
    }
  }

  for (BasicBlock *unreachableBlock : blocksToDelete) {
    for (BasicBlock *succBlock : unreachableBlock->getSuccessors()) {
      // 只有当后继块自身是可达的（没有被删除）时才需要处理
      if (reachableBlocks.count(succBlock)) {
        for (auto &phiInstPtr : succBlock->getInstructions()) {
          // Phi 指令总是在基本块的开头。一旦遇到非 Phi 指令即可停止。
          if (phiInstPtr->getKind() != Instruction::kPhi) {
            break;
          }
          // 将这个 Phi 节点中来自不可达前驱（unreachableBlock）的输入参数删除
          dynamic_cast<PhiInst *>(phiInstPtr.get())->removeIncomingBlock(unreachableBlock);
        }
      }
    }
  }

  for (auto blockIter = func->getBasicBlocks().begin(); blockIter != func->getBasicBlocks().end();) {
    BasicBlock *currentBlock = blockIter->get();
    // 如果当前块不在可达块集合中，则将其从函数中移除
    if (reachableBlocks.find(currentBlock) == reachableBlocks.end()) {
      // func->removeBasicBlock 应该返回下一个有效的迭代器
      func->removeBasicBlock((blockIter++)->get());
    } else {
      blockIter++; // 如果可达，则移动到下一个块
    }
  }

  return changed;
}

bool SysYCFGOptUtils::SysYDelEmptyBlock(Function *func, IRBuilder *pBuilder) {
  bool changed = false;

  // 步骤 1: 识别并映射所有符合“空块”定义的基本块及其目标后继
  // 使用 std::map 来存储 <空块, 空块跳转目标>
  // 这样可以处理空块链：A -> B -> C，如果 B 是空块，A 应该跳到 C
  std::map<BasicBlock *, BasicBlock *> emptyBlockRedirectMap;

  // 为了避免在遍历 func->getBasicBlocks() 时修改它导致迭代器失效，
  // 我们先收集所有的基本块。
  std::vector<BasicBlock *> allBlocks;
  for (auto &blockPtr : func->getBasicBlocks()) {
    allBlocks.push_back(blockPtr.get());
  }

  for (BasicBlock *block : allBlocks) {
    // 入口块通常不应该被认为是空块并删除，除非它没有实际指令且只有一个后继，
    // 但为了安全起见，通常会跳过入口块的删除。
    // 如果入口块是空的，它应该被合并到它的后继，但处理起来更复杂，这里先不处理入口块为空的情况
    if (block == func->getEntryBlock()) {
      continue;
    }

    // 检查基本块是否是空的：除了Phi指令外，只包含一个终止指令 (Terminator)
    // 且该终止指令必须是无条件跳转。
    // 空块必须只有一个后继才能被简化
    if (block->getNumSuccessors() == 1) {
      bool hasNonPhiNonTerminator = false;
      // 遍历除了最后一个指令之外的指令
      for (auto instIter = block->getInstructions().begin(); instIter != block->getInstructions().end();) {
        // 如果是终止指令（例如 br, ret），且不是最后一个指令，则该块有问题
        if ((*instIter)->isTerminator() && instIter != block->terminator()) {
          hasNonPhiNonTerminator = true;
          break;
        }
        // 如果不是 Phi 指令且不是终止指令
        if (!(*instIter)->isPhi() && !(*instIter)->isTerminator()) {
          hasNonPhiNonTerminator = true;
          break;
        }
        ++instIter;
        if (!hasNonPhiNonTerminator &&
            instIter == block->getInstructions().end()) { // 如果块中只有 Phi 指令和一个 Terminator
          // 确保最后一个指令是无条件跳转
          auto lastInst = block->terminator()->get();
          if (lastInst && lastInst->isUnconditional()) {
            emptyBlockRedirectMap[block] = block->getSuccessors().front();
          }
        }
      }
    }
  }

  // 步骤 2: 遍历 emptyBlockRedirectMap，处理空块链
  // 确保每个空块都直接重定向到其最终的非空后继块
  for (auto const &[emptyBlock, directSucc] : emptyBlockRedirectMap) {
    BasicBlock *targetBlock = directSucc;
    // 沿着空块链一直找到最终的非空块目标
    while (emptyBlockRedirectMap.count(targetBlock)) {
      targetBlock = emptyBlockRedirectMap[targetBlock];
    }
    emptyBlockRedirectMap[emptyBlock] = targetBlock; // 更新映射到最终目标
  }

  // 步骤 3: 遍历所有基本块，重定向其终止指令，绕过空块
  // 注意：这里需要再次遍历所有块，包括可能成为新目标的块
  for (BasicBlock *currentBlock : allBlocks) {
    // 如果 currentBlock 本身就是个空块，它会通过其前驱的重定向被处理，这里跳过
    if (emptyBlockRedirectMap.count(currentBlock)) {
      continue;
    }

    // 获取当前块的最后一个指令（终止指令）
    if (currentBlock->getInstructions().empty()) {
      // 理论上，除了入口块和可能被合并的空块外，所有块都应该有终止指令
      // 如果这里碰到空块，可能是逻辑错误或者需要特殊处理
      continue;
    }

    std::function<Value *(Value *, BasicBlock *)> getUltimateSourceValue = [&](Value *val, BasicBlock *currentDefBlock) -> Value * {
      
      if(!dynamic_cast<Instruction *>(val)) {
        // 如果 val 不是指令，直接返回它
        return val;
      }
      Instruction *inst = dynamic_cast<Instruction *>(val);
      // 如果定义指令不在任何空块中，它就是最终来源
      if (!emptyBlockRedirectMap.count(currentDefBlock)) {
        return val;
      }

      // 如果是 Phi 指令，且它在空块中，则继续追溯其在空块链中前驱的传入值
      if (inst->getKind() == Instruction::kPhi) {
        PhiInst *phi = dynamic_cast<PhiInst *>(inst);
        // 查找哪个前驱是空块链中的上一个块
        for (size_t i = 0; i < phi->getNumOperands(); i += 2) {
          BasicBlock *incomingBlock = dynamic_cast<BasicBlock *>(phi->getOperand(i + 1));
          // 检查 incomingBlock 是否是当前空块的前驱，且也在空块映射中（或就是 P）
          // 找到在空块链中导致 currentDefBlock 的那个前驱块
          if (emptyBlockRedirectMap.count(incomingBlock) || incomingBlock == currentBlock) {
            // 递归追溯该传入值
            return getUltimateSourceValue(phi->getValfromBlk(incomingBlock), incomingBlock);
          }
        }
      }
      // 如果是其他指令或者无法追溯到Phi链，则认为它在空块中产生，无法安全传播，返回null或原值
      // 在严格的空块定义下，除了Phi和Terminator，不应有其他指令产生值。
      return val; // Fallback: If not a Phi, or unable to trace, return itself (may be dangling)
    };

    auto lastInst = currentBlock->getInstructions().back().get();

    if (lastInst->isUnconditional()) { // 无条件跳转
      UncondBrInst *brInst = dynamic_cast<UncondBrInst *>(lastInst);
      BasicBlock *oldTarget = dynamic_cast<BasicBlock *>(brInst->getBlock()); // 原始跳转目标

      if (emptyBlockRedirectMap.count(oldTarget)) {               // 如果目标是空块
        BasicBlock *newTarget = emptyBlockRedirectMap[oldTarget]; // 获取最终目标

        // 更新 CFG 关系
        currentBlock->removeSuccessor(oldTarget);
        oldTarget->removePredecessor(currentBlock);

        brInst->replaceOperand(0, newTarget); // 更新跳转指令的操作数
        currentBlock->addSuccessor(newTarget);
        newTarget->addPredecessor(currentBlock);

        changed = true; // 标记发生改变

        for (auto &phiInstPtr : newTarget->getInstructions()) {
          if (phiInstPtr->getKind() == Instruction::kPhi) {
            PhiInst *phiInst = dynamic_cast<PhiInst *>(phiInstPtr.get());
            BasicBlock *actualEmptyPredecessorOfS = nullptr;
            for (size_t i = 0; i < phiInst->getNumOperands(); i += 2) {
              BasicBlock *incomingBlock = dynamic_cast<BasicBlock *>(phiInst->getOperand(i + 1));
              if (incomingBlock && emptyBlockRedirectMap.count(incomingBlock) &&
                  emptyBlockRedirectMap[incomingBlock] == newTarget) {
                actualEmptyPredecessorOfS = incomingBlock;
                break;
              }
            }

            if (actualEmptyPredecessorOfS) {
              // 获取 Phi 节点原本从 actualEmptyPredecessorOfS 接收的值
              Value *valueFromEmptyPredecessor = phiInst->getValfromBlk(actualEmptyPredecessorOfS);

              // 追溯这个值，找到它在非空块中的最终来源
              // currentBlock 是 P
              // oldTarget 是 E1 (链的起点)
              // actualEmptyPredecessorOfS 是 En (链的终点，S 的前驱)
              Value *ultimateSourceValue = getUltimateSourceValue(valueFromEmptyPredecessor, actualEmptyPredecessorOfS);

              // 替换 Phi 节点的传入块和传入值
              if (ultimateSourceValue) { // 确保成功追溯到有效来源
                // phiInst->replaceIncoming(actualEmptyPredecessorOfS, currentBlock, ultimateSourceValue);
                phiInst->replaceIncomingBlock(actualEmptyPredecessorOfS, currentBlock, ultimateSourceValue);
              } else {
                assert(false && "[DelEmptyBlock] Unable to trace a valid source for Phi instruction");
                // 无法追溯到有效来源，这可能是个错误或特殊情况
                // 此时可能需要移除该 Phi 项，或者插入一个 undef 值
                phiInst->getValfromBlk(actualEmptyPredecessorOfS);
              }
            }
          } else {
            break;
          }
        }
      }

    } else if (lastInst->getKind() == Instruction::kCondBr) { // 条件跳转
      CondBrInst *condBrInst = dynamic_cast<CondBrInst *>(lastInst);
      BasicBlock *oldThenTarget = dynamic_cast<BasicBlock *>(condBrInst->getThenBlock());
      BasicBlock *oldElseTarget = dynamic_cast<BasicBlock *>(condBrInst->getElseBlock());

      bool thenPathChanged = false;
      bool elsePathChanged = false;

      // 处理 Then 分支
      if (emptyBlockRedirectMap.count(oldThenTarget)) {
        BasicBlock *newThenTarget = emptyBlockRedirectMap[oldThenTarget];
        condBrInst->replaceOperand(1, newThenTarget); // 更新跳转指令操作数

        currentBlock->removeSuccessor(oldThenTarget);
        oldThenTarget->removePredecessor(currentBlock);
        currentBlock->addSuccessor(newThenTarget);
        newThenTarget->addPredecessor(currentBlock);
        thenPathChanged = true;
        changed = true;

        // 处理新 Then 目标块中的 Phi 指令
        // for (auto &phiInstPtr : newThenTarget->getInstructions()) {
        //   if (phiInstPtr->getKind() == Instruction::kPhi) {
        //     dynamic_cast<PhiInst *>(phiInstPtr.get())->delBlk(oldThenTarget);
        //   } else {
        //     break;
        //   }
        // }
        for (auto &phiInstPtr : newThenTarget->getInstructions()) {
          if (phiInstPtr->getKind() == Instruction::kPhi) {
            PhiInst *phiInst = dynamic_cast<PhiInst *>(phiInstPtr.get());
            BasicBlock *actualEmptyPredecessorOfS = nullptr;
            for (size_t i = 0; i < phiInst->getNumOperands(); i += 2) {
              BasicBlock *incomingBlock = dynamic_cast<BasicBlock *>(phiInst->getOperand(i + 1));
              if (incomingBlock && emptyBlockRedirectMap.count(incomingBlock) &&
                  emptyBlockRedirectMap[incomingBlock] == newThenTarget) {
                actualEmptyPredecessorOfS = incomingBlock;
                break;
              }
            }

            if (actualEmptyPredecessorOfS) {
              // 获取 Phi 节点原本从 actualEmptyPredecessorOfS 接收的值
              Value *valueFromEmptyPredecessor = phiInst->getValfromBlk(actualEmptyPredecessorOfS);

              // 追溯这个值，找到它在非空块中的最终来源
              // currentBlock 是 P
              // oldTarget 是 E1 (链的起点)
              // actualEmptyPredecessorOfS 是 En (链的终点，S 的前驱)
              Value *ultimateSourceValue = getUltimateSourceValue(valueFromEmptyPredecessor, actualEmptyPredecessorOfS);

              // 替换 Phi 节点的传入块和传入值
              if (ultimateSourceValue) { // 确保成功追溯到有效来源
                // phiInst->replaceIncoming(actualEmptyPredecessorOfS, currentBlock, ultimateSourceValue);
                phiInst->replaceIncomingBlock(actualEmptyPredecessorOfS, currentBlock, ultimateSourceValue);
              } else {
                assert(false && "[DelEmptyBlock] Unable to trace a valid source for Phi instruction");
                // 无法追溯到有效来源，这可能是个错误或特殊情况
                // 此时可能需要移除该 Phi 项，或者插入一个 undef 值
                phiInst->removeIncomingBlock(actualEmptyPredecessorOfS);
              }
            }
          } else {
            break;
          }
        }

      }

      // 处理 Else 分支
      if (emptyBlockRedirectMap.count(oldElseTarget)) {
        BasicBlock *newElseTarget = emptyBlockRedirectMap[oldElseTarget];
        condBrInst->replaceOperand(2, newElseTarget); // 更新跳转指令操作数

        currentBlock->removeSuccessor(oldElseTarget);
        oldElseTarget->removePredecessor(currentBlock);
        currentBlock->addSuccessor(newElseTarget);
        newElseTarget->addPredecessor(currentBlock);
        elsePathChanged = true;
        changed = true;

        // 处理新 Else 目标块中的 Phi 指令
        // for (auto &phiInstPtr : newElseTarget->getInstructions()) {
        //   if (phiInstPtr->getKind() == Instruction::kPhi) {
        //     dynamic_cast<PhiInst *>(phiInstPtr.get())->delBlk(oldElseTarget);
        //   } else {
        //     break;
        //   }
        // }
        for (auto &phiInstPtr : newElseTarget->getInstructions()) {
          if (phiInstPtr->getKind() == Instruction::kPhi) {
            PhiInst *phiInst = dynamic_cast<PhiInst *>(phiInstPtr.get());
            BasicBlock *actualEmptyPredecessorOfS = nullptr;
            for (size_t i = 0; i < phiInst->getNumOperands(); i += 2) {
              BasicBlock *incomingBlock = dynamic_cast<BasicBlock *>(phiInst->getOperand(i + 1));
              if (incomingBlock && emptyBlockRedirectMap.count(incomingBlock) &&
                  emptyBlockRedirectMap[incomingBlock] == newElseTarget) {
                actualEmptyPredecessorOfS = incomingBlock;
                break;
              }
            }

            if (actualEmptyPredecessorOfS) {
              // 获取 Phi 节点原本从 actualEmptyPredecessorOfS 接收的值
              Value *valueFromEmptyPredecessor = phiInst->getValfromBlk(actualEmptyPredecessorOfS);

              // 追溯这个值，找到它在非空块中的最终来源
              // currentBlock 是 P
              // oldTarget 是 E1 (链的起点)
              // actualEmptyPredecessorOfS 是 En (链的终点，S 的前驱)
              Value *ultimateSourceValue = getUltimateSourceValue(valueFromEmptyPredecessor, actualEmptyPredecessorOfS);

              // 替换 Phi 节点的传入块和传入值
              if (ultimateSourceValue) { // 确保成功追溯到有效来源
                // phiInst->replaceIncoming(actualEmptyPredecessorOfS, currentBlock, ultimateSourceValue);
                phiInst->replaceIncomingBlock(actualEmptyPredecessorOfS, currentBlock, ultimateSourceValue);
              } else {
                assert(false && "[DelEmptyBlock] Unable to trace a valid source for Phi instruction");
                // 无法追溯到有效来源，这可能是个错误或特殊情况
                // 此时可能需要移除该 Phi 项，或者插入一个 undef 值
                phiInst->removeIncomingBlock(actualEmptyPredecessorOfS);
              }
            }
          } else {
            break;
          }
        }
      }

      // 额外处理：如果条件跳转的两个分支现在指向同一个块，则可以简化为无条件跳转
      if (condBrInst->getThenBlock() == condBrInst->getElseBlock()) {
        BasicBlock *commonTarget = dynamic_cast<BasicBlock *>(condBrInst->getThenBlock());
        SysYIROptUtils::usedelete(lastInst); // 删除旧的条件跳转指令
        pBuilder->setPosition(currentBlock, currentBlock->end());
        pBuilder->createUncondBrInst(commonTarget); // 插入新的无条件跳转指令

        // 更安全地更新 CFG 关系
        std::set<BasicBlock *> currentSuccessors;
        currentSuccessors.insert(oldThenTarget);
        currentSuccessors.insert(oldElseTarget);

        // 移除旧的后继关系
        for (BasicBlock *succ : currentSuccessors) {
          currentBlock->removeSuccessor(succ);
          succ->removePredecessor(currentBlock);
        }
        // 添加新的后继关系
        currentBlock->addSuccessor(commonTarget);
        commonTarget->addPredecessor(currentBlock);

        changed = true;
      }
    }
  }

  // 步骤 4: 真正地删除空基本块
  // 注意：只能在所有跳转和 Phi 指令都更新完毕后才能删除这些块
  for (auto blockIter = func->getBasicBlocks().begin(); blockIter != func->getBasicBlocks().end();) {
    BasicBlock *currentBlock = blockIter->get();
    if (emptyBlockRedirectMap.count(currentBlock)) { // 如果在空块映射中
      // 入口块不应该被删除，即使它符合空块定义，因为函数需要一个入口
      if (currentBlock == func->getEntryBlock()) {
        ++blockIter;
        continue;
      }

      // 在删除块之前，确保其内部指令被正确删除（虽然这类块指令很少）
      for (auto instIter = currentBlock->getInstructions().begin();
           instIter != currentBlock->getInstructions().end();) {
        instIter = SysYIROptUtils::usedelete(instIter);
      }

      // 移除块
      func->removeBasicBlock((blockIter++)->get());
      changed = true;
    } else {
      ++blockIter;
    }
  }

  return changed;
}

// 如果函数没有返回指令，则添加一个默认返回指令(主要解决void函数没有返回指令的问题)
bool SysYCFGOptUtils::SysYAddReturn(Function *func, IRBuilder *pBuilder) {
  bool changed = false;
  auto basicBlocks = func->getBasicBlocks();
  for (auto &block : basicBlocks) {
    if (block->getNumSuccessors() == 0) {
      // 如果基本块没有后继块，则添加一个返回指令
      if (block->getNumInstructions() == 0) {
        pBuilder->setPosition(block.get(), block->end());
        pBuilder->createReturnInst();
        changed = true; // 标记IR被修改
      } else {
        auto thelastinst = block->getInstructions().end();
        --thelastinst;
        if (thelastinst->get()->getKind() != Instruction::kReturn) {
          // std::cout << "Warning: Function " << func->getName() << " has no return instruction, adding default
          // return." << std::endl;

          pBuilder->setPosition(block.get(), block->end());
          // TODO: 如果int float函数缺少返回值是否需要报错
          if (func->getReturnType()->isInt()) {
            pBuilder->createReturnInst(ConstantInteger::get(0));
          } else if (func->getReturnType()->isFloat()) {
            pBuilder->createReturnInst(ConstantFloating::get(0.0F));
          } else {
            pBuilder->createReturnInst();
          }
          changed = true; // 标记IR被修改
        }
      }
    }
  }

  return changed;
}

// 条件分支转换为无条件分支
// 主要针对已知条件值的分支转换为无条件分支
// 例如 if (cond) { ... } else { ... } 中的 cond 已经
// 确定为 true 或 false 的情况
bool SysYCFGOptUtils::SysYCondBr2Br(Function *func, IRBuilder *pBuilder) {
  bool changed = false;

  for (auto &basicblock : func->getBasicBlocks()) {
    if (basicblock->getNumInstructions() == 0)
      continue;

    auto thelast = basicblock->terminator();

    if (thelast->get()->isConditional()) {
      auto condBrInst = dynamic_cast<CondBrInst *>(thelast->get());
      ConstantValue *constOperand = dynamic_cast<ConstantValue *>(condBrInst->getCondition());
      std::string opname;
      int constint = 0;
      float constfloat = 0.0F;
      bool constint_Use = false;
      bool constfloat_Use = false;
      if (constOperand != nullptr) {
        if (constOperand->isFloat()) {
          constfloat = constOperand->getFloat();
          constfloat_Use = true;
        } else {
          constint = constOperand->getInt();
          constint_Use = true;
        }
      }
      // 如果可以计算
      if (constfloat_Use || constint_Use) {
        changed = true;

        auto thenBlock = dynamic_cast<BasicBlock *>(condBrInst->getThenBlock());
        auto elseBlock = dynamic_cast<BasicBlock *>(condBrInst->getElseBlock());
        thelast = SysYIROptUtils::usedelete(thelast);
        if ((constfloat_Use && constfloat == 1.0F) || (constint_Use && constint == 1)) {
          // cond为true或非0
          pBuilder->setPosition(basicblock.get(), basicblock->end());
          pBuilder->createUncondBrInst(thenBlock);

          // 更新CFG关系
          basicblock->removeSuccessor(elseBlock);
          elseBlock->removePredecessor(basicblock.get());

          // 删除elseBlock的phi指令中对应的basicblock.get()的传入值
          for (auto &phiinst : elseBlock->getInstructions()) {
            if (phiinst->getKind() != Instruction::kPhi) {
              break;
            }
            // 使用 delBlk 方法删除 basicblock.get() 对应的传入值
            dynamic_cast<PhiInst *>(phiinst.get())->removeIncomingBlock(basicblock.get());
          }

        } else { // cond为false或0

          pBuilder->setPosition(basicblock.get(), basicblock->end());
          pBuilder->createUncondBrInst(elseBlock);

          // 更新CFG关系
          basicblock->removeSuccessor(thenBlock);
          thenBlock->removePredecessor(basicblock.get());

          // 删除thenBlock的phi指令中对应的basicblock.get()的传入值
          for (auto &phiinst : thenBlock->getInstructions()) {
            if (phiinst->getKind() != Instruction::kPhi) {
              break;
            }
            // 使用 delBlk 方法删除 basicblock.get() 对应的传入值
            dynamic_cast<PhiInst *>(phiinst.get())->removeIncomingBlock(basicblock.get());
          }
        }
      }
    }
  }

  return changed;
}

// ======================================================================
// 独立的CFG优化遍的实现
// ======================================================================

bool SysYDelInstAfterBrPass::runOnFunction(Function *F, AnalysisManager &AM) {
  return SysYCFGOptUtils::SysYDelInstAfterBr(F);
}

bool SysYDelEmptyBlockPass::runOnFunction(Function *F, AnalysisManager &AM) {
  return SysYCFGOptUtils::SysYDelEmptyBlock(F, pBuilder);
}

bool SysYDelNoPreBLockPass::runOnFunction(Function *F, AnalysisManager &AM) {
  return SysYCFGOptUtils::SysYDelNoPreBLock(F);
}

bool SysYBlockMergePass::runOnFunction(Function *F, AnalysisManager &AM) { 
  return SysYCFGOptUtils::SysYBlockMerge(F); 
}

bool SysYAddReturnPass::runOnFunction(Function *F, AnalysisManager &AM) {
  return SysYCFGOptUtils::SysYAddReturn(F, pBuilder);
}

bool SysYCondBr2BrPass::runOnFunction(Function *F, AnalysisManager &AM) {
  return SysYCFGOptUtils::SysYCondBr2Br(F, pBuilder);
}

} // namespace sysy