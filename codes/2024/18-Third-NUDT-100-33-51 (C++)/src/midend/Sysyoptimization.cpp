/**
 * @file: Sysyoptimization.cpp
 * @brief  CFG优化
 * @Author : Ixeux email:you@domain.com
 * @Version : 1.0
 * @Creat Date : 2024-08-10
 *
 */
#include "../include/midend/Sysyoptimization.h"
#include <cassert>
#include <list>
#include <map>
#include <memory>
#include <string>
#include "../include/frontend/IR.h"
#include "../include/frontend/IRBuilder.h"
// #include "../include/midend/SysySSA.h"
namespace sysy {

/**
 * @brief  在有phi的情况下即SSA后进行CFG优化
 */
auto SysyOptimization::SysyOpforCE() -> void {
  SysyCondBr2BrPhi();
  SysyDelNoPreBLock();
  Sysy1phiDelete();
  SysyBlockMerge();
}

/**
 * @brief  use删除
 * @param  instr: 要删除的指令
 */
auto SysyOptimization::usedelete(Instruction *instr) -> void {
  for (auto &use1 : instr->getOperands()) {
    auto val1 = use1->getValue();
    // std::cout << val1->getName() << std::endl;
    val1->removeUse(use1);
  }
}
/**
 * @brief 删除br后面的指令
 */
auto SysyOptimization::SysyDelAfterBr() -> void {
  auto &functions = pModule->getFunctions();
  for (auto &function : functions) {
    auto basicBlocks = function.second->getBasicBlocks();
    for (auto &basicBlock : basicBlocks) {
      bool Branch = false;
      // bool DeleteInstLabel = false;
      auto &instructions = basicBlock->getInstructions();
      // std::cout << basicBlock->getName() << std::endl;s
      for (auto iter = basicBlock->begin(); iter != basicBlock->end();) {
        // auto &instruction = *iter;
        if (Branch) {
          // std ::cout << iter->get()->getKindString() << std::endl;
          usedelete(iter->get());
          // iter = instructions.erase(iter);
          iter++;
        } else {
          if ((*iter)->isTerminator()) {  // 碰见第一个branch指令
            Branch = true;
          }
          ++iter;
        }
      }
      Branch = false;
      for (auto iter = basicBlock->begin(); iter != basicBlock->end();) {
        // auto &instruction = *iter;
        if (Branch) {
          // std ::cout << iter->get()->getKindString() << std::endl;
          // usedelete(iter->get());
          iter = instructions.erase(iter);
          // iter++;
        } else {
          if ((*iter)->isTerminator()) {  // 碰见第一个branch指令
            Branch = true;
          }
          ++iter;
        }
      }
      if (Branch) {  // 修改前驱后继关系
        auto thelast = basicBlock->getInstructions().end();
        (--thelast);
        auto &Successors = basicBlock->getSuccessors();
        for (auto iterSucc = Successors.begin(); iterSucc != Successors.end();) {
          (*iterSucc)->removePredecessor(basicBlock.get());
          basicBlock->removeSuccessor(*iterSucc);
        }
        if (thelast->get()->isUnconditional()) {
          auto branchBlock = dynamic_cast<sysy::BasicBlock *>(thelast->get()->getOperand(0));
          basicBlock->addSuccessor(branchBlock);
          branchBlock->addPredecessor(basicBlock.get());
        } else if (thelast->get()->isConditional()) {
          auto thenBlock = dynamic_cast<sysy::BasicBlock *>(thelast->get()->getOperand(1));
          auto elseBlock = dynamic_cast<sysy::BasicBlock *>(thelast->get()->getOperand(2));
          basicBlock->addSuccessor(thenBlock);
          basicBlock->addSuccessor(elseBlock);
          thenBlock->addPredecessor(basicBlock.get());
          elseBlock->addPredecessor(basicBlock.get());
        } else {
        }
      }
    }
  }
}

/**
 * @brief condbr 1/0 删除
 */
auto SysyOptimization::SysyCondbr2Br() -> void {
  auto &functions = pModule->getFunctions();
  for (auto &function : functions) {
    auto basicBlocks = function.second->getBasicBlocks();

    for (auto &basicBlock : basicBlocks) {
      // std::cout << "here" << std::endl;
      // std::cout << basicBlock->getName() << std::endl;
      // std::cout << distance(basicBlock->begin(), basicBlock->end()) << std::endl;
      // std::cout << basicBlock->getNumInstructions() << std::endl;
      if (distance(basicBlock->begin(), basicBlock->end()) == 0) {
        continue;
      }
      auto thelast = basicBlock->getInstructions().end();
      (--thelast);
      if (thelast->get()->getKind() == Instruction::kCondBr) {
        ConstantValue *constOperand = nullptr;
        constOperand = dynamic_cast<ConstantValue *>(thelast->get()->getOperand(0));
        std::string opname;
        int constint = 0;
        bool constint_Use = false;
        float constfloat = 0;
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
        // std::cout << opname << std::endl;
        if (constfloat_Use || constint_Use) {
          auto thenBlock = dynamic_cast<BasicBlock *>(thelast->get()->getOperand(1));
          auto elseBlock = dynamic_cast<BasicBlock *>(thelast->get()->getOperand(2));
          // auto condition = dynamic_cast<ConstantValue *>(thelast->get()->getOperand(0));
          // std::cout << "here" << std::endl;
          // TODO(hsz) 修改use
          usedelete(thelast->get());
          thelast = basicBlock->getInstructions().erase(thelast);
          if ((constfloat_Use && constfloat == 1.0F) || (constint_Use && constint == 1)) {
            // std::cout << "here1" << std::endl;
            pBuilder->setPosition(basicBlock.get(), basicBlock->end());
            pBuilder->createUncondBrInst(thenBlock, {});
            basicBlock->removeSuccessor(elseBlock);
            elseBlock->removePredecessor(basicBlock.get());
          } else {
            // std::cout << "here1" << std::endl;
            pBuilder->setPosition(basicBlock.get(), basicBlock->end());
            pBuilder->createUncondBrInst(elseBlock, {});
            basicBlock->removeSuccessor(thenBlock);
            thenBlock->removePredecessor(basicBlock.get());
          }
        }
      }
    }
  }
}

/**
 * @brief 链合并
 */
auto SysyOptimization::SysyBlockMerge() -> void {
  auto &functions = pModule->getFunctions();
  for (auto &function : functions) {
    // auto basicBlocks = function.second->getBasicBlocks();
    for (auto blockiter = function.second->getBasicBlocks().begin();
         blockiter != function.second->getBasicBlocks().end();) {
      if (blockiter->get()->getNumSuccessors() == 1) {
        if (((blockiter->get())->getSuccessors()[0])->getNumPredecessors() == 1) {
          auto block = blockiter->get();
          auto nextBlock = blockiter->get()->getSuccessors()[0];
          auto nextarguments = nextBlock->getArguments();
          // 删除br指令(若有)
          if (block->getNumInstructions() != 0) {
            auto thelast = block->end();
            (--thelast);
            if (thelast->get()->getKind() == Instruction::kBr) {
              // TODO(hsz) 修改use
              usedelete(thelast->get());
              block->getInstructions().erase(thelast);
            } else if (thelast->get()->getKind() == Instruction::kCondBr) {
              if (thelast->get()->getOperand(1)->getName() == thelast->get()->getOperand(1)->getName()) {
                // std::cout << thelast->get()->getOperand(1)->getName() << thelast->get()->getOperand(2)->getName()
                //           << std::endl;
                // TODO(hsz) 修改use
                usedelete(thelast->get());
                block->getInstructions().erase(thelast);
              }
            }
          }
          for (auto institer = nextBlock->begin(); institer != nextBlock->end();) {
            institer->get()->setParent(block);
            block->getInstructions().emplace_back(institer->release());
            institer = nextBlock->getInstructions().erase(institer);
          }
          for (auto &argm : nextarguments) {
            argm->setParent(block);
            block->insertArgument(argm);
          }
          // // // 改变block 和 next 之间的关系
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

          // // // 添加block 和 next 的后继 之间的关系
          // block->addSuccessor(nextBlock->getSuccessors()[0]);
          // nextBlock->getSuccessors()[0]->addPredecessor(blockiter->get());
          // // // 删除next 和 next后继的关系
          // nextBlock->getSuccessors()[0]->removePredecessor(nextBlock);
          // nextBlock->removeSuccessor(nextBlock->getSuccessors()[0]);
          // std::cout << nextBlock->getName() << std::endl;

          function.second->removeBasicBlock(nextBlock);
          // ++blockiter;
        } else {
          blockiter++;
        }
      } else {
        blockiter++;
      }
    }
  }
}

/**
 * @brief  空块删除
 */
auto SysyOptimization::SysyDelEmptyBlock() -> void {
  auto &functions = pModule->getFunctions();
  for (auto &function : functions) {
    auto basicBlocks = function.second->getBasicBlocks();
    std::map<sysy::BasicBlock *, sysy ::BasicBlock *>
        EmptyBlockMap;  // 空块儿和他们后边的块儿的映射，当一个块儿只有br指令时，也会被添加到这里
    for (auto &basicBlock : basicBlocks) {
      if (basicBlock->getNumInstructions() == 0) {
        if (basicBlock->getNumSuccessors() == 1) {
          EmptyBlockMap[basicBlock.get()] = basicBlock->getSuccessors().front();  // 若后面有后继
        }
        // if (basicBlock->getNumSuccessors() == 0) {
        //   EmptyBlockMap[basicBlock.get()] = nullptr;  // 若后面无后继
        // }
      }
      if (basicBlock->getNumInstructions() == 1 &&
          basicBlock->getInstructions().begin()->get()->getKind() == Instruction::kBr) {
        EmptyBlockMap[basicBlock.get()] = basicBlock->getSuccessors().front();  // 只有br指令
      }
    }

    for (auto &basicBlock : basicBlocks) {
      // std::cout << "here" << std::endl;
      if (distance(basicBlock->begin(), basicBlock->end()) == 0) {
        if (basicBlock->getNumSuccessors() == 0) {
          continue;
        }
        if (basicBlock->getNumSuccessors() > 1) {
          assert("");
        }
        pBuilder->setPosition(basicBlock.get(), basicBlock->end());
        pBuilder->createUncondBrInst(basicBlock->getSuccessors()[0], {});
        continue;
      }
      auto thelast = basicBlock->getInstructions().end();
      (--thelast);
      if (thelast->get()->getKind() == Instruction::kBr) {
        auto OldBrBlock = dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(0));
        sysy::BasicBlock *thelastOld = nullptr;
        while (EmptyBlockMap.find(dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(0))) !=
               EmptyBlockMap.end()) {
          thelastOld = dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(0));
          thelast->get()->replaceOperand(
              0, EmptyBlockMap[dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(0))]);
          // (*thelast)->setOperand(0, EmptyBlockMap[dynamic_cast<sysy ::BasicBlock *>((*iter)->getOperand(0))]);
        }
        // std::cout << thelast->get()->getOperand(0)->getName() << std::endl;
        basicBlock->removeSuccessor(OldBrBlock);
        OldBrBlock->removePredecessor(basicBlock.get());
        basicBlock->addSuccessor(dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(0)));
        dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(0))->addPredecessor(basicBlock.get());
        // auto indexInNew = dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(0))->getPredecessors().
        if (thelastOld != nullptr) {
          int index = 0;
          for (auto &pred : dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(0))->getPredecessors()) {
            if (pred == thelastOld) {
              break;
            }
            index++;
          }

          for (auto &InstInNew : dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(0))->getInstructions()) {
            if (InstInNew->isPhi()) {
              dynamic_cast<PhiInst *>(InstInNew.get())
                  ->addOperand(dynamic_cast<PhiInst *>(InstInNew.get())->getOperand(index + 1));
            } else {
              break;
            }
          }
        }

      } else if (thelast->get()->getKind() == Instruction::kCondBr) {
        auto OldThenBlock = dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(1));
        auto OldElseBlock = dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(2));
        sysy::BasicBlock *thelastOld = nullptr;
        while (EmptyBlockMap.find(dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(1))) !=
               EmptyBlockMap.end()) {
          thelastOld = dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(1));
          thelast->get()->replaceOperand(
              1, EmptyBlockMap[dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(1))]);
        }
        basicBlock->removeSuccessor(OldThenBlock);
        OldThenBlock->removePredecessor(basicBlock.get());
        if (dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(1)) ==
            dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(2))) {
          auto thebrBlock = dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(1));
          usedelete(thelast->get());
          thelast = basicBlock->getInstructions().erase(thelast);
          pBuilder->setPosition(basicBlock.get(), basicBlock->end());
          pBuilder->createUncondBrInst(thebrBlock, {});
          continue;
        }
        basicBlock->addSuccessor(dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(1)));
        dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(1))->addPredecessor(basicBlock.get());
        // auto indexInNew = dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(0))->getPredecessors().

        if (thelastOld != nullptr) {
          int index = 0;
          for (auto &pred : dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(1))->getPredecessors()) {
            if (pred == thelastOld) {
              break;
            }
            index++;
          }
          for (auto &InstInNew : dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(1))->getInstructions()) {
            if (InstInNew->isPhi()) {
              dynamic_cast<PhiInst *>(InstInNew.get())
                  ->addOperand(dynamic_cast<PhiInst *>(InstInNew.get())->getOperand(index + 1));
            } else {
              break;
            }
          }
        }
        thelastOld = nullptr;
        while (EmptyBlockMap.find(dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(2))) !=
               EmptyBlockMap.end()) {
          thelastOld = dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(2));
          thelast->get()->replaceOperand(
              2, EmptyBlockMap[dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(2))]);
        }
        basicBlock->removeSuccessor(OldElseBlock);
        OldElseBlock->removePredecessor(basicBlock.get());
        if (dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(1)) ==
            dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(2))) {
          auto thebrBlock = dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(1));
          usedelete(thelast->get());
          thelast = basicBlock->getInstructions().erase(thelast);
          pBuilder->setPosition(basicBlock.get(), basicBlock->end());
          pBuilder->createUncondBrInst(thebrBlock, {});
          continue;
        }
        basicBlock->addSuccessor(dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(2)));
        dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(2))->addPredecessor(basicBlock.get());

        if (thelastOld != nullptr) {
          int index = 0;
          for (auto &pred : dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(2))->getPredecessors()) {
            if (pred == thelastOld) {
              break;
            }
            index++;
          }
          for (auto &InstInNew : dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(2))->getInstructions()) {
            if (InstInNew->isPhi()) {
              dynamic_cast<PhiInst *>(InstInNew.get())
                  ->addOperand(dynamic_cast<PhiInst *>(InstInNew.get())->getOperand(index + 1));
            } else {
              break;
            }
          }
        }
      } else {
        if (basicBlock->getNumSuccessors() == 1) {
          pBuilder->setPosition(basicBlock.get(), basicBlock->end());
          pBuilder->createUncondBrInst(basicBlock->getSuccessors()[0], {});
          auto thelast = basicBlock->getInstructions().end();
          (--thelast);
          auto OldBrBlock = dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(0));
          sysy::BasicBlock *thelastOld = nullptr;
          while (EmptyBlockMap.find(dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(0))) !=
                 EmptyBlockMap.end()) {
            thelastOld = dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(0));

            thelast->get()->replaceOperand(
                0, EmptyBlockMap[dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(0))]);
            // (*thelast)->setOperand(0, EmptyBlockMap[dynamic_cast<sysy ::BasicBlock *>((*iter)->getOperand(0))]);
          }
          // std::cout << thelast->get()->getOperand(0)->getName() << std::endl;
          basicBlock->removeSuccessor(OldBrBlock);
          OldBrBlock->removePredecessor(basicBlock.get());
          basicBlock->addSuccessor(dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(0)));
          dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(0))->addPredecessor(basicBlock.get());
          if (thelastOld != nullptr) {
            int index = 0;
            for (auto &pred : dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(0))->getPredecessors()) {
              if (pred == thelastOld) {
                break;
              }
              index++;
            }
            for (auto &InstInNew :
                 dynamic_cast<sysy ::BasicBlock *>(thelast->get()->getOperand(0))->getInstructions()) {
              if (InstInNew->isPhi()) {
                dynamic_cast<PhiInst *>(InstInNew.get())
                    ->addOperand(dynamic_cast<PhiInst *>(InstInNew.get())->getOperand(index + 1));
              } else {
                break;
              }
            }
          }
          // EmptyBlockMap[basicBlock.get()] = nullptr;
        }
      }
      // 正常流
    }
    // std::cout << EmptyBlockMap.size() << std::endl;
    // std::cout << "here11" << std::endl;
    // auto &basicBLocks_Norange = function.second->getBasicBlocks_NoRange();

    for (auto iter = function.second->getBasicBlocks().begin(); iter != function.second->getBasicBlocks().end();) {
      // std::cout << "here" << EmptyBlockMap.size() << std::endl;
      if (EmptyBlockMap.find(iter->get()) != EmptyBlockMap.end()) {
        if (iter->get() == function.second->getEntryBlock()) {
          ++iter;
          continue;
        }

        for (auto &iterInst : iter->get()->getInstructions()) {
          usedelete(iterInst.get());
        }

        for (auto &succ : iter->get()->getSuccessors()) {
          int index = 0;
          // int index = 0;
          for (auto &pred : succ->getPredecessors()) {
            if (pred == iter->get()) {
              break;
            }
            index++;
          }
          for (auto &instinsucc : succ->getInstructions()) {
            if (instinsucc->isPhi()) {
              dynamic_cast<PhiInst *>(instinsucc.get())->removeOperand(index);
            } else {
              break;
            }
          }
        }

        function.second->removeBasicBlock((iter++)->get());
        // iter = basicBLocks_Norange.erase(iter);
      } else {
        ++iter;
      }
    }
  }
}

/**
 * @brief  删除无前驱空块，也就是删除无法到达的块
 */
auto SysyOptimization::SysyDelNoPreBLock() -> void {
  auto &functions = pModule->getFunctions();
  for (auto &function : functions) {
    auto &func = function.second;

    for (auto &block : func->getBasicBlocks()) {
      block->setreachableFalse();
    }
    auto entryBlock = func->getEntryBlock();
    entryBlock->setreachableTrue();
    std::queue<BasicBlock *> blockqueue;
    blockqueue.push(entryBlock);
    while (!blockqueue.empty()) {
      auto block = blockqueue.front();
      blockqueue.pop();
      for (auto &succ : block->getSuccessors()) {
        if (!succ->getreachable()) {
          succ->setreachableTrue();
          blockqueue.push(succ);
        }
      }
    }
    for (auto blockIter = func->getBasicBlocks().begin(); blockIter != func->getBasicBlocks().end();) {
      if (!blockIter->get()->getreachable()) {
        for (auto &iterInst : blockIter->get()->getInstructions()) {
          usedelete(iterInst.get());
        }

        blockIter++;
        // func->removeBasicBlock((blockIter++)->get());
        // function.second->removeBasicBlock((iter++)->get());
      } else {
        blockIter++;
      }
    }
    for (auto blockIter = func->getBasicBlocks().begin(); blockIter != func->getBasicBlocks().end();) {
      if (!blockIter->get()->getreachable()) {
        // for (auto &iterInst : blockIter->get()->getInstructions()) {
        //   usedelete(iterInst.get());
        // }
        for (auto succblock : blockIter->get()->getSuccessors()) {
          int indexphi = 1;
          for (auto pred : succblock->getPredecessors()) {
            if (pred == blockIter->get()) {
              break;
            }
            indexphi++;
          }
          // std::cout << indexphi << std::endl;
          for (auto &phiinst : succblock->getInstructions()) {
            if (phiinst->getKind() != Instruction::kPhi) {
              break;
            }
            phiinst->removeOperand(indexphi);
          }
          //   succblock->removePredecessor(blockIter->get());
        }

        func->removeBasicBlock((blockIter++)->get());
        // function.second->removeBasicBlock((iter++)->get());
      } else {
        blockIter++;
      }
    }
  }
}

/**
 * @brief  没有后继的块加return
 */
auto SysyOptimization::SysyAddReturn() -> void {
  auto &functions = pModule->getFunctions();
  for (auto &function : functions) {
    auto &func = function.second;
    for (auto &block : func->getBasicBlocks()) {
      if (block->getNumSuccessors() == 0) {
        if (block->getNumInstructions() == 0) {
          pBuilder->setPosition(block.get(), block->end());
          pBuilder->createReturnInst({});
        }
        auto thelast = block->getInstructions().end();
        (--thelast);
        if (thelast->get()->getKind() != Instruction::kReturn) {
          pBuilder->setPosition(block.get(), block->end());

          if (func->getReturnType()->isInt()) {
            pBuilder->createReturnInst(ConstantValue::get(0));
          } else if (func->getReturnType()->isFloat()) {
            pBuilder->createReturnInst(ConstantValue::get(0.0F));
          } else {
            pBuilder->createReturnInst({});
          }
        }
      }
    }
  }
}

/**
 * @brief  优化全部，SSA前使用
 */
auto SysyOptimization::SysyOptimizateAll() -> void {
  SysyDelAfterBr();
  SysyCondbr2Br();
  SysyDelEmptyBlock();
  SysyDelNoPreBLock();
  SysyBlockMerge();
  SysyAddReturn();
  // auto &functions = pModule->getFunctions();
  // for (auto &function : functions) {
  //   auto basicBlocks = function.second->getBasicBlocks();
  //   for (auto &basicblock : basicBlocks) {
  //     std::cout << basicblock->getName() << basicblock->getNumInstructions() << std::endl;
  //   }
  // }
  // SysyBlockMerge();
  // auto &functions = pModule->getFunctions();
  // for (auto &function : functions) {
  //   auto basicBlocks = function.second->getBasicBlocks();
  //   for (auto &basicblock : basicBlocks) {
  //     std::cout << basicblock->getName() << basicblock->getNumInstructions() << std::endl;
  //   }
  // }
}

/**
 * @brief  循环优化，已不用
 */
auto SysyOptimization::optimizationforop() -> void {
  // SysyDelAfterBr();
  // SysyCondbr2Br();
  // SysyDelEmptyBlock();
  SysyDelNoPreBLock();
  SysyBlockMerge();
  SysyAddReturn();
}

/**
 * @brief  在有phi的情况下(SSA后) 进行CondBr变成Br
 */
auto SysyOptimization::SysyCondBr2BrPhi() -> void {
  auto &functions = pModule->getFunctions();
  for (auto &func : functions) {
    auto &function = func.second;
    for (auto &basicblock : function->getBasicBlocks()) {
      if (basicblock->getNumInstructions() == 0) {
        continue;
      }
      auto thelast = basicblock->getInstructions().end();
      (--thelast);
      // auto thelastinst = dynamic_cast<Instruction *>(thelast->get());
      if (thelast->get()->getKind() == Instruction::kCondBr) {
        ConstantValue *constOperand = nullptr;
        constOperand = dynamic_cast<ConstantValue *>(thelast->get()->getOperand(0));
        std::string opname;
        int constint = 0;
        bool constint_Use = false;
        float constfloat = 0;
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
        if (constfloat_Use || constint_Use) {
          auto thenBlock = dynamic_cast<BasicBlock *>(thelast->get()->getOperand(1));
          auto elseBlock = dynamic_cast<BasicBlock *>(thelast->get()->getOperand(2));
          // auto condition = dynamic_cast<ConstantValue *>(thelast->get()->getOperand(0));
          // std::cout << "here" << std::endl;
          // TODO(hsz) 修改use
          usedelete(thelast->get());
          thelast = basicblock->getInstructions().erase(thelast);
          if ((constfloat_Use && constfloat == 1.0F) || (constint_Use && constint == 1)) {
            // std::cout << "here1" << std::endl;
            pBuilder->setPosition(basicblock.get(), basicblock->end());
            pBuilder->createUncondBrInst(thenBlock, {});
            int phiindex = 0;
            for (auto pred : elseBlock->getPredecessors()) {
              phiindex++;
              if (pred == basicblock.get()) {
                break;
              }
            }

            for (auto &phiinst : elseBlock->getInstructions()) {
              if (phiinst->getKind() != Instruction::kPhi) {
                break;
              }
              phiinst->removeOperand(phiindex);
            }
            basicblock->removeSuccessor(elseBlock);
            elseBlock->removePredecessor(basicblock.get());
          } else {
            // std::cout << "here1" << std::endl;

            pBuilder->setPosition(basicblock.get(), basicblock->end());
            pBuilder->createUncondBrInst(elseBlock, {});
            int phiindex = 0;
            for (auto pred : thenBlock->getPredecessors()) {
              phiindex++;
              if (pred == basicblock.get()) {
                break;
              }
            }

            for (auto &phiinst : thenBlock->getInstructions()) {
              if (phiinst->getKind() != Instruction::kPhi) {
                break;
              }
              phiinst->removeOperand(phiindex);
            }
            basicblock->removeSuccessor(thenBlock);
            thenBlock->removePredecessor(basicblock.get());
          }
        }
      }
    }
  }
}

/**
 * @brief  phi 后只有一个的情况删除
 */
auto SysyOptimization::Sysy1phiDelete() -> void {
  auto &functions = pModule->getFunctions();
  for (auto &func : functions) {
    auto &function = func.second;

    for (auto &basicblock : function->getBasicBlocks()) {
      if (basicblock->getNumPredecessors() != 1) {
        continue;
      }

      auto thefirst = basicblock->getInstructions().begin();
      if (!thefirst->get()->isPhi()) {
        continue;
      }
      for (auto iter = basicblock->getInstructions().begin(); iter != basicblock->getInstructions().end();) {
        if (iter->get()->isPhi()) {
          pBuilder->setPosition(basicblock.get(), basicblock->begin());
          auto loadfrom = iter->get()->getOperand(1);
          auto loadto = iter->get()->getOperand(0);
          usedelete(iter->get());
          iter = basicblock->getInstructions().erase(iter);
          // std::cout << iter->get()->getKindString() << std::endl;
          // iter++;
          if (auto constvalue = dynamic_cast<ConstantValue *>(loadfrom)) {
            loadfrom = constvalue;
            pBuilder->setPosition(basicblock.get(), basicblock->begin());
            pBuilder->createStoreInst(loadfrom, loadto);

          } else {
            pBuilder->setPosition(basicblock.get(), basicblock->begin());
            auto loadinst = pBuilder->createLoadInst(loadfrom);
            pBuilder->createStoreInst(loadinst, loadto);
          }

          // auto loadinst = pBuilder->createLoadInst(loadfrom);
          // pBuilder->createStoreInst(loadinst, loadfrom);
          // iter = basicblock->getInstructions().erase(iter);
        } else {
          iter++;
        }
      }
    }
  }
}

// auto SysyOptimization::SysyDeadInstAndBlockDelete() -> void {
//   auto &functions = pModule->getFunctions();
//   for (auto &function : functions) {
//     auto basicBlocks = function.second->getBasicBlocks();
//     std::map<sysy::BasicBlock *, sysy ::BasicBlock *>
//         EmptyBlockMap;  // 空块儿和他们后边的块儿的映射，当一个块儿只有br指令时，也会被添加到这里
//     for (auto &basicBlock : basicBlocks) {
//       if (basicBlock->getNumInstructions() == 0) {
//         if (basicBlock->getNumSuccessors() == 1) {
//           EmptyBlockMap[basicBlock.get()] = basicBlock->getSuccessors().front();  // 若后面有后继
//         }
//         // if (basicBlock->getNumSuccessors() == 0) {
//         //   EmptyBlockMap[basicBlock.get()] = nullptr;  // 若后面无后继
//         // }
//       }
//       if (basicBlock->getNumInstructions() == 1 &&
//           basicBlock->getInstructions().begin()->get()->getKind() == Instruction::kBr) {
//         EmptyBlockMap[basicBlock.get()] = basicBlock->getSuccessors().front();  // 只有br指令
//       }
//     }
//     for (auto &basicBlock : basicBlocks) {
//       bool Branch = false;
//       // bool DeleteInstLabel = false;
//       auto &instructions = basicBlock->getInstructions();
//       for (auto iter = basicBlock->begin(); iter != basicBlock->end();) {
//         // auto &instruction = *iter;
//         if (Branch) {
//           iter = instructions.erase(iter);
//           // DeleteInstLabel = true;
//         } else {
//           if ((*iter)->isBranch()) {  // 碰见第一个branch指令
//             Branch = true;
//             if ((*iter)->getKind() == Instruction::kBr) {  //
//             检查br是否跳转到空块儿，若是则修改跳转目标直到不是空块儿
//               while (EmptyBlockMap.find(dynamic_cast<sysy ::BasicBlock *>((*iter)->getOperand(0))) !=
//                      EmptyBlockMap.end()) {
//                 (*iter)->setOperand(0, EmptyBlockMap[dynamic_cast<sysy ::BasicBlock *>((*iter)->getOperand(0))]);
//               }
//             } else {  // 检查condbr是否跳转到空块儿，若是则修改跳转目标直到不是空块儿
//               // 如果是 1/0 则变成无跳转跳转指令。
//               // 否则
//               while (EmptyBlockMap.find(dynamic_cast<sysy ::BasicBlock *>((*iter)->getOperand(1))) !=
//                      EmptyBlockMap.end()) {
//                 (*iter)->setOperand(1, EmptyBlockMap[dynamic_cast<sysy ::BasicBlock *>((*iter)->getOperand(0))]);
//               }
//               while (EmptyBlockMap.find(dynamic_cast<sysy ::BasicBlock *>((*iter)->getOperand(2))) !=
//                      EmptyBlockMap.end()) {
//                 (*iter)->setOperand(2, EmptyBlockMap[dynamic_cast<sysy ::BasicBlock *>((*iter)->getOperand(0))]);
//               }
//             }
//           }
//           ++iter;
//         }
//       }
//       if (Branch) {
//         auto thelast = basicBlock->getInstructions().end();
//         (--thelast);
//         auto &Successors = basicBlock->getSuccessors();
//         for (auto iterSucc = Successors.begin(); iterSucc != Successors.end();) {
//           (*iterSucc)->removePredecessor(basicBlock.get());
//           basicBlock->removeSuccessor(*iterSucc);
//         }
//         if (thelast->get()->isUnconditional()) {
//           auto branchBlock = dynamic_cast<sysy::BasicBlock *>(thelast->get()->getOperand(0));
//           basicBlock->addSuccessor(branchBlock);
//           branchBlock->addPredecessor(basicBlock.get());
//         } else if (thelast->get()->isConditional()) {
//           auto thenBlock = dynamic_cast<sysy::BasicBlock *>(thelast->get()->getOperand(1));
//           auto elseBlock = dynamic_cast<sysy::BasicBlock *>(thelast->get()->getOperand(2));
//           basicBlock->addSuccessor(thenBlock);
//           basicBlock->addSuccessor(elseBlock);
//           thenBlock->addPredecessor(basicBlock.get());
//           elseBlock->addPredecessor(basicBlock.get());
//         }
//       }
//     }
//     for (auto iter = basicBlocks.begin(); iter != basicBlocks.end();) {
//       if (EmptyBlockMap.find(iter->get()) != EmptyBlockMap.end()) {
//         // iter = basicBlocks.erase(iter);
//         ;
//       } else {
//         ++iter;
//       }
//     }
//   }
// }
}  // namespace sysy
