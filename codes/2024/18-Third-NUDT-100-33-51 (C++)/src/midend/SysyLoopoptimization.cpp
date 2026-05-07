/**
 * @file: SysyLoopoptimization.cpp
 * @brief  循环不变量外提
 * @Author : Ixeux email:you@domain.com
 * @Version : 1.0
 * @Creat Date : 2024-08-10
 *
 */
#include "../include/midend/SysyLoopoptimization.h"
#include <bits/types/FILE.h>
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <unordered_set>
#include "../include/frontend/IR.h"
#include "../include/midend/CFA.h"
#include "../include/midend/FuncAnalysis.h"
namespace sysy {

/**
 * @brief  不变量外提 总接口
 */
auto SysyLoopoptimization::SysyrunInvariant() -> void {
  auto &functions = pModule->getFunctions();
  for (auto &function : functions) {
    // auto &func = function.second;
    for (auto &toploop : function.second->getTopLoops()) {
      SysyInvariantBasicLoop(toploop.get());
    }
    // std::cout << "here" << std::endl;
  }
}

/**
 * @brief  对某个loop进行不变量外提
 * @param  toploop:
 */
auto SysyLoopoptimization::SysyInvariantBasicLoop(Loop *toploop) -> void {
  for (auto loop : toploop->getSubLoops()) {
    SysyInvariantBasicLoop(loop);
  }
  // bool global_consider = true;
  // std::cout << "here" << toploop->getLoopID() << std::endl;
  // bool array_consider = true;
  std::unordered_set<GlobalValue *> changedGlobalValues;

  std::unordered_set<AllocaInst *> changedArrays;
  for (auto basicblock : toploop->getBasicBlocks()) {
    for (auto &inst : basicblock->getInstructions()) {
      if (inst->isStore()) {
        auto Storeinst = dynamic_cast<StoreInst *>(inst.get());
        auto lValinst = dynamic_cast<LVal *>(Storeinst->getPointer());
        auto father = lValinst->getFatherLVal();
        if (father != nullptr) {
          while (father->getFatherLVal() != nullptr) {
            father = father->getFatherLVal();
          }
        } else {
          father = lValinst;
        }

        auto globalValue = dynamic_cast<GlobalValue *>(father);
        if (globalValue != nullptr) {
          changedGlobalValues.insert(globalValue);
        } else {
          auto fatheralloc = dynamic_cast<AllocaInst *>(father);
          if (fatheralloc->getNumDims() != 0) {
            changedArrays.insert(fatheralloc);
          }
        }
      }
      if (inst->isCall()) {
        // array_consider = false;
        // global_consider = false;
        auto callinst = dynamic_cast<CallInst *>(inst.get());
        auto global_call_change = sysy::FuncAnalysis::globalWirttenSet(callinst->getCallee());
        for (auto gbc : global_call_change) {
          changedGlobalValues.insert(dynamic_cast<GlobalValue *>(gbc));
        }
        for (auto &op : callinst->getArguments()) {
          auto LValv = dynamic_cast<LVal *>(op->getValue());
          if (LValv != nullptr) {
            auto constv = dynamic_cast<ConstantValue *>(LValv);
            if (constv != nullptr) {
              continue;
            }
            // auto array = dynamic_cast< *>(LValv);
            auto father = LValv->getFatherLVal();
            if (father == nullptr) {
              father = LValv;
            } else {
              while (father->getFatherLVal() != nullptr) {
                father = father->getFatherLVal();
              }
            }
            auto globalvalue = dynamic_cast<GlobalValue *>(father);
            if (globalvalue != nullptr) {
              changedGlobalValues.insert(globalvalue);
            } else {
              auto fatheralloc = dynamic_cast<AllocaInst *>(father);
              if (fatheralloc->getNumDims() != 0) {
                changedArrays.insert(fatheralloc);
              }
            }
          }
        }
      }
    }
  }

  // 不变量外提
  // std::list<BasicBlock *> BasicDomAll;
  // for (auto basicblock : toploop->getBasicBlocks()) {
  //   bool flag = true;
  //   for (auto exitingblock : toploop->getexitingBlocks()) {
  //     if (exitingblock->getDominants().find(basicblock) == exitingblock->getDominants().end()) {
  //       flag = false;
  //     }
  //   }
  //   if (flag) {
  //     BasicDomAll.push_back(basicblock);
  //     // std::cout << "basicblock: " << basicblock->getName() << std::endl;
  //   }
  // }
  // for (auto global : changedGlobalValues) {
  //   std::cout << "global: " << global->getName() << std::endl;
  // }
  // for (auto array : changedArrays) {
  //   std::cout << "array " << array->getName() << std::endl;
  // }

  bool changed = true;

  while (changed) {
    changed = false;
    // std::cout << "here" << std::endl;
    // for (auto basicblock : toploop->getBasicBlocks()) {
    auto basicblock = toploop->getHeader();
    // std::cout << basicblock->getName() << std::endl;
    for (auto instIter = basicblock->begin(); instIter != basicblock->end();) {
      auto instruction = instIter->get();
      // std::cout << instruction->getKindString() << std::endl;
      bool outOfloop = true;
      if (instruction->isLoad()) {
        auto loadinst = static_cast<LoadInst *>(instruction);

        auto getop = loadinst->getPointer();

        auto lvalv = dynamic_cast<LVal *>(getop);
        auto father = lvalv->getFatherLVal();
        if (father == nullptr) {
          father = lvalv;
        } else {
          while (father->getFatherLVal() != nullptr) {
            father = father->getFatherLVal();
          }
        }

        auto globalValue = dynamic_cast<GlobalValue *>(father);
        if (globalValue != nullptr) {
          // if (global_consider) {
          if (changedGlobalValues.find(globalValue) != changedGlobalValues.end()) {
            outOfloop = false;
          } else {
            for (auto &dim : loadinst->getIndices()) {
              auto dimvalue = dim->getValue();
              auto const_ptr = dynamic_cast<ConstantValue *>(dimvalue);
              if (const_ptr == nullptr) {
                auto diminst = dynamic_cast<Instruction *>(dimvalue);
                if (toploop->isBasicBlockInLoop(diminst->getParent())) {
                  outOfloop = false;
                }
              }
            }
          }

        } else if (loadinst->getNumIndices() != 0) {
          outOfloop = false;
          auto lvalv = dynamic_cast<LVal *>(loadinst->getPointer());
          auto father = lvalv->getFatherLVal();
          if (father == nullptr) {
            father = lvalv;
          } else {
            while (father->getFatherLVal() != nullptr) {
              father = father->getFatherLVal();
            }
          }
          auto array = dynamic_cast<AllocaInst *>(father);
          if (changedArrays.find(array) != changedArrays.end()) {
            outOfloop = false;
          } else {
            for (auto &dim : loadinst->getIndices()) {
              auto dimvalue = dim->getValue();
              auto const_ptr = dynamic_cast<ConstantValue *>(dimvalue);
              if (const_ptr == nullptr) {
                auto diminst = dynamic_cast<Instruction *>(dimvalue);
                if (toploop->isBasicBlockInLoop(diminst->getParent())) {
                  outOfloop = false;
                }
              }
            }
          }
        } else {
          auto uses = getop->getUses();
          for (auto &use : uses) {
            auto useInst = dynamic_cast<Instruction *>(use->getUser());

            if (useInst != nullptr) {
              if (useInst->isStore() || useInst->isPhi()) {
                if (toploop->isBasicBlockInLoop(useInst->getParent())) {
                  outOfloop = false;
                }
              }
            }
          }
          // std::cout << "here" << std::endl;
        }

        // if (loadinst->getNumIndices() != 0) {
        //   // TODO(hsz)数组
        //   // if (!array_consider) {
        //   //   outOfloop = false;
        //   // } else {

        //   // }
        // }
      } else if (instruction->isUnary()) {
        auto op0 = instruction->getOperand(0);
        auto const_ptr = dynamic_cast<ConstantValue *>(op0);
        if (const_ptr == nullptr) {
          auto opinst = dynamic_cast<Instruction *>(op0);
          if (toploop->isBasicBlockInLoop(opinst->getParent())) {
            outOfloop = false;
          }
        }
      } else if (instruction->isBinary()) {
        // instIter++;
        auto op0 = instruction->getOperand(0);
        auto op1 = instruction->getOperand(1);
        auto const_ptr = dynamic_cast<ConstantValue *>(op0);
        if (const_ptr == nullptr) {
          auto opinst = dynamic_cast<Instruction *>(op0);
          if (toploop->isBasicBlockInLoop(opinst->getParent())) {
            outOfloop = false;
          }
          // for (auto &use : op0->getUses()) {
          //   auto useInst = dynamic_cast<Instruction *>(use->getUser());
          //   if (useInst != nullptr) {
          //     if (useInst->isStore() || useInst->isPhi()) {
          //       if (toploop->isBasicBlockInLoop(useInst->getParent())) {
          //         outOfloop = false;
          //       }
          //     }
          //   }
          // }
        }
        const_ptr = dynamic_cast<ConstantValue *>(op1);
        if (const_ptr == nullptr) {
          auto opinst = dynamic_cast<Instruction *>(op1);
          if (toploop->isBasicBlockInLoop(opinst->getParent())) {
            outOfloop = false;
          }
          {
            // for (auto &use : op1->getUses()) {
            //   auto useInst = dynamic_cast<Instruction *>(use->getUser());
            //   // if (useInst != nullptr) {
            //   if (useInst->isStore() || useInst->isPhi()) {
            //     if (toploop->isBasicBlockInLoop(useInst->getParent())) {
            //       outOfloop = false;
            //     }
            //   }
            //   // }
            // }
          }
        }
        // auto op1 = instruction->getOperand(0);
        // auto op2 = instruction->getOperand(1);
      } else if (instruction->isStore()) {
        instIter++;
        continue;
        if (basicblock == toploop->getHeader()) {
          auto op0 = instruction->getOperand(0);
          auto const_ptr = dynamic_cast<ConstantValue *>(op0);
          if (const_ptr == nullptr) {
            auto opinst = dynamic_cast<Instruction *>(op0);
            if (toploop->isBasicBlockInLoop(opinst->getParent())) {
              outOfloop = false;
            }
          }
        } else {
          instIter++;
          continue;
        }
      }
      // else if (instruction->isLa()) {
      //   // instIter++;
      //   // continue;
      //   // ;

      //   auto lainst = dynamic_cast<LaInst *>(instruction);
      //   auto op = lainst->getPointer();
      //   auto opinst = dynamic_cast<Instruction *>(op);
      //   // // if (toploop->isBasicBlockInLoop(opinst->getParent())) {
      //   // //   outOfloop = false;
      //   // // }
      //   // // for(&dim )

      //   for (auto &dim : lainst->getIndices()) {
      //     auto dimvalue = dim->getValue();
      //     auto const_ptr = dynamic_cast<ConstantValue *>(dimvalue);
      //     if (const_ptr == nullptr) {
      //       auto diminst = dynamic_cast<Instruction *>(dimvalue);
      //       if (toploop->isBasicBlockInLoop(diminst->getParent())) {
      //         outOfloop = false;
      //       }
      //     }
      //   }
      // }
      else {
        instIter++;
        continue;
      }

      // 指令外提
      if (outOfloop) {
        // if (toploop->getPreheaderBlock() == nullptr) {
        //   // instIter++;
        //   // continue;
        //   auto predblockadd = basicblock->getParent()->addBasicBlock("");
        //   std::stringstream ss;
        //   ss << "L" << pBuilder->getLabelIndex();
        //   predblockadd->setName(ss.str());
        //   ss.str("");
        //   predblockadd->addSuccessor(toploop->getHeader());

        //   // 获得所有pred的index， 并对predblockadd添加这些前驱块儿
        //   std::vector<int> op_indexs;
        //   std::list<BasicBlock *> pred2dels;
        //   int index = 0;
        //   for (auto &pred : toploop->getHeader()->getPredecessors()) {
        //     index++;
        //     if (toploop->isBasicBlockInLoop(pred)) {
        //       continue;
        //     }
        //     op_indexs.push_back(index);
        //     pred2dels.push_back(pred);
        //     // pred->replacePredecessor(toploop->getHeader(), predblockadd);
        //     // predblockadd->
        //     predblockadd->addPredecessor(pred);
        //     pred->addSuccessor(predblockadd);
        //     pred->removeSuccessor(toploop->getHeader());
        //     auto thelast = pred->getInstructions().end();
        //     (thelast--);
        //     for (auto opblockindex = 0; opblockindex < thelast->get()->getOperands().size(); opblockindex++) {
        //       if (thelast->get()->getOperand(opblockindex) == toploop->getHeader()) {
        //         thelast->get()->replaceOperand(opblockindex, predblockadd);
        //       }
        //     }
        //   }

        //   pBuilder->setPosition(predblockadd, predblockadd->end());
        //   for (auto &phiinst : toploop->getHeader()->getInstructions()) {
        //     if (!phiinst->isPhi()) {
        //       break;
        //     }
        //     // 创建一个变量
        //     auto val = dynamic_cast<PhiInst *>(phiinst.get())->getOperand(0);
        //     auto newname = dynamic_cast<Instruction *>(val)->getName() + "_" + predblockadd->getName();
        //     auto newalloca = pBuilder->createAllocaInstWithoutInsert(val->getType(), {}, predblockadd, newname);
        //     predblockadd->getParent()->addIndirectAlloca(newalloca);

        //     auto addphi = pBuilder->createPhiInst(val->getType(), newalloca, predblockadd);
        //     int i = 1;

        //     for (auto &op_index : op_indexs) {
        //       // pBuilder->createPhiInst(val->getType(), newalloca, predblockadd, op_index);
        //       addphi->replaceOperand(i, phiinst->getOperand(op_index));
        //       i++;
        //     }
        //     int delnum = 0;
        //     int totalopnum = phiinst->getNumOperands();
        //     for (auto opindex2del = 0; opindex2del < totalopnum; opindex2del++) {
        //       if (std::find(op_indexs.begin(), op_indexs.end(), opindex2del) != op_indexs.end()) {
        //         phiinst->removeOperand(opindex2del - delnum);
        //         delnum++;
        //       }
        //     }
        //     phiinst->addOperand(newalloca);
        //   }
        //   pBuilder->setPosition(predblockadd, predblockadd->end());
        //   pBuilder->createUncondBrInst(toploop->getHeader(), {});

        //   for (auto pred2del : pred2dels) {
        //     toploop->getHeader()->removePredecessor(pred2del);
        //   }
        //   toploop->getHeader()->addPredecessor(predblockadd);
        //   toploop->setPredHeader(predblockadd);
        //   auto thisloop = toploop;
        //   while (auto parentloop = thisloop->getParentLoop()) {
        //     if (predblockadd->getParent()->getLoopOfBasicBlock(predblockadd) == nullptr) {
        //       predblockadd->getParent()->addBBToLoop(predblockadd, parentloop);
        //     }
        //     parentloop->addBasicBlock(predblockadd);
        //     thisloop = parentloop;
        //   }

        //   instIter->get()->setParent(predblockadd);
        //   auto thelast = predblockadd->getInstructions().end();
        //   changed = true;
        //   // predblockadd->getInstructions().emplace(predblockadd->getInstructions().end(), instIter->release());
        //   //   instIter = basicblock->getInstructions().erase(instIter);
        //   //   // instIter->get()->setParent(predheaderblock);

        //   (thelast--);
        //   predblockadd->getInstructions().emplace(thelast, instIter->release());
        //   instIter = basicblock->getInstructions().erase(instIter);
        //   //   // instIter->get()->setParent(predheaderblock);

        //   // for(pred)

        // } else {

        // std::cout << "why" << std ::endl;
        // std::cout << instIter->get()->getKindString() << std::endl;
        auto predheaderblock = toploop->getPreheaderBlock();
        instIter->get()->setParent(predheaderblock);
        auto thelast = predheaderblock->getInstructions().end();
        changed = true;
        if (predheaderblock->getNumInstructions() == 0) {
          predheaderblock->getInstructions().emplace(predheaderblock->getInstructions().end(), instIter->release());
          instIter = basicblock->getInstructions().erase(instIter);
          // instIter->get()->setParent(predheaderblock);
        } else {
          (thelast--);
          predheaderblock->getInstructions().emplace(thelast, instIter->release());
          instIter = basicblock->getInstructions().erase(instIter);
          // instIter->get()->setParent(predheaderblock);
        }
        // instIter++;
        // }
      } else {
        instIter++;
      }
      // std::cout << "here" << std::endl;
      // }
    }
  }
  // SysyPrestore(toploop);
}

// auto
auto SysyLoopoptimization::SysyPrestore(Loop *toploop) -> void {
  sysy::CFA cfa(pModule);
  cfa.run();
  std::list<BasicBlock *> DomiantInBlocks;
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
  for (auto Dbasicblock : DomiantInBlocks) {
    std::list<Value *> arrayCanInThis;
    std::list<Value *> arrayCanNotInThis;
    for (auto iter = Dbasicblock->getInstructions().begin(); iter != Dbasicblock->getInstructions().end();) {
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
                if (inst->getParent()->getDominants().find(Dbasicblock) == inst->getParent()->getDominants().end() ||
                    inst->getParent() == Dbasicblock) {
                  CanMay = false;
                }
              }
            }
            auto Constvalue = dynamic_cast<ConstantValue *>(storeInst->getValue());
            if (Constvalue == nullptr) {
              auto inst = dynamic_cast<Instruction *>(storeInst->getValue());
              if (toploop->isBasicBlockInLoop(inst->getParent())) {
                CanMay = false;
              }
            }
            if (CanMay) {
              // arrayCanInThis.push_back(pointer);
              iter->get()->setParent(toploop->getPreheaderBlock());
              auto thelast = toploop->getPreheaderBlock()->getInstructions().end();
              thelast--;
              toploop->getPreheaderBlock()->getInstructions().emplace(thelast, iter->release());
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
}
}  // namespace sysy