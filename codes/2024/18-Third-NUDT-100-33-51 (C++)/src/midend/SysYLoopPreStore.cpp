/**
 * @file: SysYLoopPreStore.cpp
 * @brief  循环预存，未使用，目的是将一些循环的存指令只在第一次执行时进行
 * @Author : Ixeux email:you@domain.com
 * @Version : 1.0
 * @Creat Date : 2024-08-10
 *
 */
#include "../include/midend/SysYLoopPreStore.h"
#include <cstddef>
#include <iostream>
#include <list>
#include <string>
#include "../include/frontend/IR.h"
#include "../include/midend/CFA.h"
namespace sysy {

/**
 * @brief  循环预存，执行接口
 */
auto SysyLoopPreStore::run() -> void {
  auto &functions = pModule->getFunctions();
  for (auto &func : functions) {
    auto &function = func.second;
    for (auto &topLoop : function->getTopLoops()) {
      runforLoop(topLoop.get());
    }
  }
}

/**
 * @brief  对某个循环进行预存
 * @param  toploop: 循环
 */
auto SysyLoopPreStore::runforLoop(Loop *toploop) -> void {
  for (auto loop : toploop->getSubLoops()) {
    runforLoop(loop);
  }
  sysy::CFA cfa(pModule);
  cfa.run();
  std::list<BasicBlock *> DomiantInBlocks;
  // std::cout << "-------" << std::endl;
  // std::cout << toploop->getLoopID() << std::endl;
  for (auto basicblock : toploop->getBasicBlocks()) {
    if (basicblock == toploop->getHeader()) {
      continue;
    }
    auto label = true;
    // std::cout << basicblock->getName() << std::endl;
    if (toploop->getexitingBlocks().size() == 1) {
      // std::cout << "here1" << std::endl;
      if (toploop->getexitingBlocks().find(toploop->getHeader()) != toploop->getexitingBlocks().end()) {
        auto latchBlock = toploop->getlatchBlock()[0];
        if (latchBlock->getDominants().find(basicblock) == latchBlock->getDominants().end()) {
          label = false;
        }
      }

    } else {
      for (auto exitblock : toploop->getexitBlocks()) {
        if (exitblock->getPredecessors()[0] == toploop->getHeader()) {
          continue;
        }
        if (exitblock->getDominants().find(basicblock) == exitblock->getDominants().end()) {
          label = false;
          break;
        }
      }
    }
    if (label) {
      // std::cout << basicblock->getName() << std::endl;
      DomiantInBlocks.push_back(basicblock);
    }
  }
  // std::cout << "result :" << std::endl;
  // for (auto bb : DomiantInBlocks) {
  //   std::cout << bb->getName() << std::endl;
  // }

  std::set<Value *> arrayNot;
  for (auto basicblock : toploop->getBasicBlocks()) {
    for (auto &inst : basicblock->getInstructions()) {
      if (inst->isStore()) {
        auto storeinst = dynamic_cast<StoreInst *>(inst.get());
        for (auto &indice : storeinst->getIndices()) {
          // if(indice)
          auto constValue = dynamic_cast<ConstantValue *>(indice->getValue());
          if (constValue == nullptr) {
            arrayNot.insert(storeinst->getPointer());
            break;
          }
        }
      }
    }
  }
  // std::cout << "arrayNot :" << std::endl;
  // for (auto a : arrayNot) {
  //   std::cout << a->getName() << std::endl;
  // }
  // auto headerblock = ;
  for (auto iter = toploop->getHeader()->getInstructions().begin();
       iter != toploop->getHeader()->getInstructions().end(); iter++) {
    if (iter->get()->getKind() != Instruction::kPhi) {
      pBuilder->setPosition(toploop->getHeader(), iter);
      auto loopindphi = toploop->getindPhi();

      int index = 1;
      // std::cout << toploop->getPreheaderBlock()->getName() << std::endl;
      for (auto pred : toploop->getHeader()->getPredecessors()) {
        // std::cout << pred->getName() << std::endl;
        if (pred == toploop->getPreheaderBlock()) {
          break;
        }
        (++index);
      }
      // std::cout << toploop->getLoopID() << " + " << index << std::endl;
      // Value *phiPredheader;
      // auto loopbegin =
      if (loopindphi == nullptr) {
        return;
      }

      if (toploop->getPreheaderBlock() == nullptr) {
        return;
      }

      // std::cout << toploop->getLoopID() << std::endl;
      // std::cout << loopindphi->getUses().size() << std::endl;
      for (auto &use : loopindphi->getUses()) {
        if (dynamic_cast<PhiInst *>(use->getUser()) != nullptr && use->getIndex() == 0) {
          // auto phiPreheader = use->;

          auto phiPredheader = dynamic_cast<PhiInst *>(use->getUser())->getOperand(index);

          if (loopindphi->getType() == Type::getPointerType(Type::getFloatType())) {
            // std::cout << "here11" << std::endl;

            std::list<BasicBlock *> BlockBeforeInsert;
            for (auto succheader : toploop->getHeader()->getSuccessors()) {
              if (toploop->isBasicBlockInLoop(succheader)) {
                BlockBeforeInsert.push_back(succheader);
              }
            }
            if (BlockBeforeInsert.size() == 2) {
              return;
            }

            auto condFirsttime = pBuilder->createFCmpEQInst(loopindphi, phiPredheader);
            for (auto BBinset : BlockBeforeInsert) {
              auto CondBlock = condFirsttime->getParent()->getParent()->addBasicBlock(
                  "L_" + std::to_string(toploop->getLoopID()) + "_" + BBinset->getName() + "Con");
              auto thenBlock = condFirsttime->getParent()->getParent()->addBasicBlock(
                  "L_" + std::to_string(toploop->getLoopID()) + "_" + BBinset->getName() + "then");

              // add inst to thenblock
              for (auto Dbasicblock : DomiantInBlocks) {
                std::list<Value *> arrayCanInThis;
                std::list<Value *> arrayCanNotInThis;
                for (auto iter = Dbasicblock->getInstructions().begin();
                     iter != Dbasicblock->getInstructions().end();) {
                  auto inst2check = dynamic_cast<Instruction *>(iter->get());
                  if (inst2check->isStore()) {
                    auto storeInst = dynamic_cast<StoreInst *>(inst2check);
                    if (storeInst->getNumIndices() > 0) {
                      auto pointer = storeInst->getPointer();
                      if (arrayNot.find(pointer) == arrayNot.end()) {
                        // InstList.push_back()
                        bool CanMay = true;
                        for (auto &use : pointer->getUses()) {
                          auto inst = dynamic_cast<Instruction *>(use->getUser());

                          if (inst->isLoad() && toploop->isBasicBlockInLoop(inst->getParent())) {
                            if (inst->getParent()->getDominants().find(Dbasicblock) ==
                                    inst->getParent()->getDominants().end() ||
                                inst->getParent() == Dbasicblock) {
                              CanMay = false;
                            }
                          }
                        }

                        if (CanMay) {
                          // arrayCanInThis.push_back(pointer);
                          iter->get()->setParent(thenBlock);
                          thenBlock->getInstructions().emplace_back(iter->release());
                          iter = Dbasicblock->getInstructions().erase(iter);
                        }
                        //  else {
                        //   arrayCanNotInThis.push_back(pointer);
                        // }

                      } else {
                        iter++;
                      }
                    } else {
                      iter++;
                    }
                  } else {
                    iter++;
                  }
                }
              }

              if (thenBlock->getNumInstructions() == 0) {
                condFirsttime->getParent()->getParent()->removeBasicBlock(thenBlock);
                condFirsttime->getParent()->getParent()->removeBasicBlock(CondBlock);
                continue;
              }
              auto headerblock = toploop->getHeader();
              auto thelast = headerblock->getInstructions().end();
              (thelast--);
              if (dynamic_cast<BasicBlock *>(thelast->get()->getOperand(1)) == BBinset) {
                thelast->get()->replaceOperand(1, CondBlock);
              }
              if (dynamic_cast<BasicBlock *>(thelast->get()->getOperand(2)) == BBinset) {
                thelast->get()->replaceOperand(2, CondBlock);
              }
              headerblock->removeSuccessor(BBinset);
              headerblock->addSuccessor(CondBlock);
              CondBlock->addPredecessor(headerblock);

              pBuilder->setPosition(CondBlock, CondBlock->end());
              pBuilder->createCondBrInst(condFirsttime, thenBlock, BBinset, {}, {});
              CondBlock->addSuccessor(thenBlock);
              CondBlock->addSuccessor(BBinset);
              thenBlock->addPredecessor(CondBlock);
              BBinset->replacePredecessor(headerblock, CondBlock);

              pBuilder->setPosition(thenBlock, thenBlock->end());
              pBuilder->createUncondBrInst(BBinset, {});

              thenBlock->addSuccessor(BBinset);
              BBinset->addPredecessor(thenBlock);
              auto phiindex = 1;

              for (auto predBBinst : BBinset->getPredecessors()) {
                if (predBBinst == CondBlock) {
                  break;
                }
                phiindex++;
              }
              for (auto &phiinst : BBinset->getInstructions()) {
                if (!phiinst->isPhi()) {
                  break;
                }
                phiinst->addOperand(phiinst->getOperand(phiindex));
              }

              CondBlock->setLoop(toploop);
              toploop->addBasicBlock(CondBlock);
              // for(auto parent )
              auto parent = toploop->getParentLoop();
              while (parent != nullptr) {
                parent->addBasicBlock(CondBlock);
                parent = parent->getParentLoop();
              }
              CondBlock->getParent()->addBBToLoop(CondBlock, toploop);

              thenBlock->setLoop(toploop);
              toploop->addBasicBlock(thenBlock);
              // for(auto parent )
              parent = toploop->getParentLoop();
              while (parent != nullptr) {
                parent->addBasicBlock(thenBlock);
                parent = parent->getParentLoop();
              }
              thenBlock->getParent()->addBBToLoop(thenBlock, toploop);
            }

          } else if (loopindphi->getType() == Type::getPointerType(Type::getIntType())) {
            std::list<BasicBlock *> BlockBeforeInsert;
            for (auto succheader : toploop->getHeader()->getSuccessors()) {
              if (toploop->isBasicBlockInLoop(succheader)) {
                BlockBeforeInsert.push_back(succheader);
              }
            }
            if (BlockBeforeInsert.size() == 2) {
              return;
            }

            auto condFirsttime = pBuilder->createICmpEQInst(loopindphi, phiPredheader);
            for (auto BBinset : BlockBeforeInsert) {
              auto CondBlock = condFirsttime->getParent()->getParent()->addBasicBlock(
                  "L_" + std::to_string(toploop->getLoopID()) + "_" + BBinset->getName() + "Con");
              auto thenBlock = condFirsttime->getParent()->getParent()->addBasicBlock(
                  "L_" + std::to_string(toploop->getLoopID()) + "_" + BBinset->getName() + "then");

              // add inst to thenblock
              for (auto Dbasicblock : DomiantInBlocks) {
                std::list<Value *> arrayCanInThis;
                std::list<Value *> arrayCanNotInThis;
                for (auto iter = Dbasicblock->getInstructions().begin();
                     iter != Dbasicblock->getInstructions().end();) {
                  auto inst2check = dynamic_cast<Instruction *>(iter->get());
                  if (inst2check->isStore()) {
                    auto storeInst = dynamic_cast<StoreInst *>(inst2check);
                    if (storeInst->getNumIndices() > 0) {
                      auto pointer = storeInst->getPointer();
                      if (arrayNot.find(pointer) == arrayNot.end()) {
                        // InstList.push_back()
                        bool CanMay = true;
                        for (auto &use : pointer->getUses()) {
                          auto inst = dynamic_cast<Instruction *>(use->getUser());

                          if (inst->isLoad() && toploop->isBasicBlockInLoop(inst->getParent())) {
                            if (inst->getParent()->getDominants().find(Dbasicblock) ==
                                    inst->getParent()->getDominants().end() ||
                                inst->getParent() == Dbasicblock) {
                              CanMay = false;
                            }
                          }
                        }

                        if (CanMay) {
                          // arrayCanInThis.push_back(pointer);
                          iter->get()->setParent(thenBlock);
                          thenBlock->getInstructions().emplace_back(iter->release());
                          iter = Dbasicblock->getInstructions().erase(iter);
                        }
                        //  else {
                        //   arrayCanNotInThis.push_back(pointer);
                        // }

                      } else {
                        iter++;
                      }
                    } else {
                      iter++;
                    }
                  } else {
                    iter++;
                  }
                }
              }

              if (thenBlock->getNumInstructions() == 0) {
                condFirsttime->getParent()->getParent()->removeBasicBlock(thenBlock);
                condFirsttime->getParent()->getParent()->removeBasicBlock(CondBlock);
                continue;
              }
              auto headerblock = toploop->getHeader();
              auto thelast = headerblock->getInstructions().end();
              (thelast--);
              if (dynamic_cast<BasicBlock *>(thelast->get()->getOperand(1)) == BBinset) {
                thelast->get()->replaceOperand(1, CondBlock);
              }
              if (dynamic_cast<BasicBlock *>(thelast->get()->getOperand(2)) == BBinset) {
                thelast->get()->replaceOperand(2, CondBlock);
              }
              headerblock->removeSuccessor(BBinset);
              headerblock->addSuccessor(CondBlock);
              CondBlock->addPredecessor(headerblock);

              pBuilder->setPosition(CondBlock, CondBlock->end());
              pBuilder->createCondBrInst(condFirsttime, thenBlock, BBinset, {}, {});
              CondBlock->addSuccessor(thenBlock);
              CondBlock->addSuccessor(BBinset);
              thenBlock->addPredecessor(CondBlock);
              BBinset->replacePredecessor(headerblock, CondBlock);

              pBuilder->setPosition(thenBlock, thenBlock->end());
              pBuilder->createUncondBrInst(BBinset, {});

              thenBlock->addSuccessor(BBinset);
              BBinset->addPredecessor(thenBlock);
              auto phiindex = 1;

              for (auto predBBinst : BBinset->getPredecessors()) {
                if (predBBinst == CondBlock) {
                  break;
                }
                phiindex++;
              }
              for (auto &phiinst : BBinset->getInstructions()) {
                if (!phiinst->isPhi()) {
                  break;
                }
                phiinst->addOperand(phiinst->getOperand(phiindex));
              }

              CondBlock->setLoop(toploop);
              toploop->addBasicBlock(CondBlock);
              // for(auto parent )
              auto parent = toploop->getParentLoop();
              while (parent != nullptr) {
                parent->addBasicBlock(CondBlock);
                parent = parent->getParentLoop();
              }
              CondBlock->getParent()->addBBToLoop(CondBlock, toploop);

              thenBlock->setLoop(toploop);
              toploop->addBasicBlock(thenBlock);
              // for(auto parent )
              parent = toploop->getParentLoop();
              while (parent != nullptr) {
                parent->addBasicBlock(thenBlock);
                parent = parent->getParentLoop();
              }
              thenBlock->getParent()->addBBToLoop(thenBlock, toploop);
            }
          }
          break;
        }
      }

      // pBuilder->createFCmpEQInst(Value *lhs, Value *rhs)

      break;
    }
  }

  // for (auto basicblock : DomiantInBlocks) {
  //   for (auto &inst : basicblock->getInstructions()) {
  //   }
  // }

  // for(auto )
}

}  // namespace sysy