/**
 * @file: SysyLoopspecificate.cpp
 * @brief  循环规范化
 * @Author : Ixeux email:you@domain.com
 * @Version : 1.0
 * @Creat Date : 2024-08-10
 *
 */
#include "../include/midend/SysyLoopspecificate.h"
#include <string>
#include "../include/frontend/IR.h"
namespace sysy {

/**
 * @brief  循环规范化，对外接口
 */
auto SysYLoopspecificator::run() -> void {
  auto &functions = pModule->getFunctions();
  for (auto &func : functions) {
    auto &function = func.second;
    for (auto &toploop : function->getTopLoops()) {
      Latch2One(toploop.get(), function.get());
      exitBlockinsert(toploop.get(), function.get());
      preHeaderOne(toploop.get(), function.get());
      // preIfBfloop(toploop.get(), function.get());
    }
  }
}

/**
 * @brief  preheader归一
 * @param  toploop: 被执行循环
 * @param  parentfunction: 所在function
 */
auto SysYLoopspecificator::preHeaderOne(Loop *toploop, Function *parentfunction) -> void {
  for (auto subloop : toploop->getSubLoops()) {
    preHeaderOne(subloop, parentfunction);
  }

  if (toploop->getPreheaderBlock() == nullptr) {
    auto predblockadd = parentfunction->addBasicBlock("");
    std::stringstream ss;
    ss << "L" << pBuilder->getLabelIndex();
    predblockadd->setName(ss.str());
    ss.str("");
    predblockadd->addSuccessor(toploop->getHeader());

    // 获得所有pred的index， 并对predblockadd添加这些前驱块儿
    std::vector<int> op_indexs;
    std::list<BasicBlock *> pred2dels;
    int index = 0;
    for (auto &pred : toploop->getHeader()->getPredecessors()) {
      index++;
      if (toploop->isBasicBlockInLoop(pred)) {
        continue;
      }
      op_indexs.push_back(index);
      pred2dels.push_back(pred);
      // pred->replacePredecessor(toploop->getHeader(), predblockadd);
      // predblockadd->
      predblockadd->addPredecessor(pred);
      pred->addSuccessor(predblockadd);
      pred->removeSuccessor(toploop->getHeader());
      auto thelast = pred->getInstructions().end();
      (thelast--);
      for (auto opblockindex = 0; opblockindex < thelast->get()->getOperands().size(); opblockindex++) {
        if (thelast->get()->getOperand(opblockindex) == toploop->getHeader()) {
          thelast->get()->replaceOperand(opblockindex, predblockadd);
        }
      }
    }

    pBuilder->setPosition(predblockadd, predblockadd->end());
    for (auto &phiinst : toploop->getHeader()->getInstructions()) {
      if (!phiinst->isPhi()) {
        break;
      }
      // 创建一个变量
      auto val = dynamic_cast<PhiInst *>(phiinst.get())->getOperand(0);
      auto newname = dynamic_cast<Instruction *>(val)->getName() + "_" + predblockadd->getName();
      auto newalloca = pBuilder->createAllocaInstWithoutInsert(val->getType(), {}, predblockadd, newname);
      predblockadd->getParent()->addIndirectAlloca(newalloca);

      auto addphi = pBuilder->createPhiInst(val->getType(), newalloca, predblockadd);
      int i = 1;

      for (auto &op_index : op_indexs) {
        // pBuilder->createPhiInst(val->getType(), newalloca, predblockadd, op_index);
        addphi->replaceOperand(i, phiinst->getOperand(op_index));
        i++;
      }
      int delnum = 0;
      int totalopnum = phiinst->getNumOperands();
      for (auto opindex2del = 0; opindex2del < totalopnum; opindex2del++) {
        if (std::find(op_indexs.begin(), op_indexs.end(), opindex2del) != op_indexs.end()) {
          phiinst->removeOperand(opindex2del - delnum);
          delnum++;
        }
      }
      phiinst->addOperand(newalloca);
    }
    pBuilder->setPosition(predblockadd, predblockadd->end());
    pBuilder->createUncondBrInst(toploop->getHeader(), {});

    for (auto pred2del : pred2dels) {
      toploop->getHeader()->removePredecessor(pred2del);
    }
    toploop->getHeader()->addPredecessor(predblockadd);
    toploop->setPredHeader(predblockadd);
    auto thisloop = toploop;
    while (auto parentloop = thisloop->getParentLoop()) {
      if (predblockadd->getParent()->getLoopOfBasicBlock(predblockadd) == nullptr) {
        predblockadd->getParent()->addBBToLoop(predblockadd, parentloop);
      }
      parentloop->addBasicBlock(predblockadd);
      thisloop = parentloop;
    }
  }
}

/**
 * @brief  latch边归一
 * @param  toploop: 被执行循环
 * @param  parentfunction: function
 */
auto SysYLoopspecificator::Latch2One(Loop *toploop, Function *parentfunction) -> void {
  for (auto subloop : toploop->getSubLoops()) {
    Latch2One(subloop, parentfunction);
  }

  if (toploop->getlatchBlock().size() != 1) {
    auto newlatchblock = parentfunction->addBasicBlock();
    auto parentloop = toploop->getParentLoop();
    if (parentloop != nullptr) {
      parentloop->addBasicBlock(newlatchblock);
      parentloop = parentloop->getParentLoop();
    }

    std::list<int> phiindexs;
    int phiindex = 1;
    for (auto pred : toploop->getHeader()->getPredecessors()) {
      if (std::find(toploop->getlatchBlock().begin(), toploop->getlatchBlock().end(), pred) !=
          toploop->getlatchBlock().end()) {
        phiindexs.push_back(phiindex);
        newlatchblock->addPredecessor(pred);
      }
      phiindex++;
    }
    newlatchblock->addSuccessor(toploop->getHeader());
    newlatchblock->setName("loop" + std::to_string(toploop->getLoopID()) + "_latch");
    for (auto latchbefore : toploop->getlatchBlock()) {
      toploop->getHeader()->removePredecessor(latchbefore);
      auto thelast = latchbefore->getInstructions().end();
      (thelast--);
      if (thelast->get()->getKind() == Instruction::kBr) {
        thelast->get()->replaceOperand(0, newlatchblock);

      } else if (thelast->get()->getKind() == Instruction::kCondBr) {
        if (dynamic_cast<BasicBlock *>(thelast->get()->getOperand(1)) == toploop->getHeader()) {
          thelast->get()->replaceOperand(1, newlatchblock);
        }
        if (dynamic_cast<BasicBlock *>(thelast->get()->getOperand(2)) == toploop->getHeader()) {
          thelast->get()->replaceOperand(2, newlatchblock);
        }
      }
      latchbefore->removeSuccessor(toploop->getHeader());
      latchbefore->addSuccessor(newlatchblock);
    }
    toploop->getHeader()->addPredecessor(newlatchblock);

    pBuilder->setPosition(newlatchblock, newlatchblock->end());
    for (auto &phiinst : toploop->getHeader()->getInstructions()) {
      if (!phiinst->isPhi()) {
        break;
      }
      pBuilder->setPosition(newlatchblock, newlatchblock->end());
      auto newPhi0 = pBuilder->createAllocaInstWithoutInsert(phiinst->getOperand(0)->getType(), {}, newlatchblock);
      newlatchblock->getParent()->addIndirectAlloca(newPhi0);
      newPhi0->setName(phiinst->getOperand(0)->getName() + "latch");
      pBuilder->setPosition(newlatchblock, newlatchblock->end());
      auto newPhiInLatch = pBuilder->createPhiInst(newPhi0->getType(), newPhi0, newlatchblock);
      // auto newphiindex = 1;
      auto removephinum = 0;
      for (auto index : phiindexs) {
        newPhiInLatch->replaceOperand(removephinum + 1, phiinst->getOperand(index - removephinum));
        phiinst->removeOperand(index - removephinum);
        removephinum++;
        // newphiindex++;
      }
      phiinst->addOperand(newPhi0);
    }

    pBuilder->setPosition(newlatchblock, newlatchblock->end());
    pBuilder->createUncondBrInst(toploop->getHeader(), {});
    newlatchblock->setLoop(toploop);
    newlatchblock->setLoopDepth(toploop->getLoopDepth());
    parentfunction->addBBToLoop(newlatchblock, toploop);
    // parentfunction->add

    toploop->addBasicBlock(newlatchblock);
    toploop->getlatchBlock().clear();
    toploop->addLatchBlock(newlatchblock);
  }
}

/**
 * @brief  保证循环退出块只有一个前驱
 * @param  toploop: 被执行loop
 * @param  parentfunction: 父funciton
 */
auto SysYLoopspecificator::exitBlockinsert(Loop *toploop, Function *parentfunction) -> void {
  for (auto subloop : toploop->getSubLoops()) {
    exitBlockinsert(subloop, parentfunction);
  }
  std::list<BasicBlock *> exitBlock2change;
  for (auto exitBlock : toploop->getexitBlocks()) {
    if (exitBlock->getNumPredecessors() != 1) {
      exitBlock2change.push_back(exitBlock);
    }
  }

  for (auto exitblockinchange : exitBlock2change) {
    for (auto pred : exitblockinchange->getPredecessors()) {
      if (toploop->isBasicBlockInLoop(pred)) {
        auto exitaddblock =
            parentfunction->addBasicBlock(pred->getName() + std::to_string(toploop->getLoopID()) + "exitadd");
        auto thelast = pred->getInstructions().end();
        (thelast--);
        if (thelast->get()->getKind() == Instruction::kBr) {
          thelast->get()->replaceOperand(0, exitaddblock);

        } else if (thelast->get()->getKind() == Instruction::kCondBr) {
          if (dynamic_cast<BasicBlock *>(thelast->get()->getOperand(1)) == exitblockinchange) {
            thelast->get()->replaceOperand(1, exitaddblock);
          }
          if (dynamic_cast<BasicBlock *>(thelast->get()->getOperand(2)) == exitblockinchange) {
            thelast->get()->replaceOperand(2, exitaddblock);
          }
        }
        exitaddblock->addPredecessor(pred);
        pred->removeSuccessor(exitblockinchange);
        pred->addSuccessor(exitaddblock);

        exitblockinchange->replacePredecessor(pred, exitaddblock);
        exitaddblock->addSuccessor(exitblockinchange);

        pBuilder->setPosition(exitaddblock, exitaddblock->end());
        pBuilder->createUncondBrInst(exitblockinchange, {});

        toploop->addExitBlocks(exitaddblock);
        exitaddblock->setLoop(exitblockinchange->getLoop());
        exitaddblock->setLoopDepth(exitblockinchange->getLoopDepth());
        // exitblockinchange-
        if (exitblockinchange->getLoop() != nullptr) {
          parentfunction->addBBToLoop(exitaddblock, exitblockinchange->getLoop());
        }
      }
    }

    toploop->getexitBlocks().erase(exitblockinchange);
  }
}

auto SysYLoopspecificator::preIfBfloop(Loop *toploop, Function *parentfunction) -> void {
  for (auto subloop : toploop->getSubLoops()) {
    preIfBfloop(subloop, parentfunction);
  }

  auto headerblock = toploop->getHeader();

  auto predheaderblock = toploop->getPreheaderBlock();
  int phiindex = 1;
  for (auto pred : headerblock->getPredecessors()) {
    if (pred == predheaderblock) {
      break;
    }
    phiindex++;
  }
  // if(predheaderblock->getNumArguments())
  auto predindex = 0;
  for (auto pred : headerblock->getPredecessors()) {
    if (pred == predheaderblock) {
      break;
    }
    predindex++;
  }

  auto predIfblock = parentfunction->addBasicBlock(std::to_string(toploop->getLoopID()) + "IfBf");

  auto thelast = predheaderblock->getInstructions().end();
  thelast--;
  if (thelast->get()->getKind() == Instruction::kBr) {
    thelast->get()->replaceOperand(0, predIfblock);
  } else if (thelast->get()->getKind() == Instruction::kCondBr) {
    if (dynamic_cast<BasicBlock *>(thelast->get()->getOperand(1)) == headerblock) {
      thelast->get()->replaceOperand(1, predIfblock);
    }
    if (dynamic_cast<BasicBlock *>(thelast->get()->getOperand(2)) == headerblock) {
      thelast->get()->replaceOperand(2, predIfblock);
    }
  }
  predIfblock->addPredecessor(predheaderblock);
  predheaderblock->removeSuccessor(headerblock);
  predheaderblock->addSuccessor(predIfblock);
  auto predthenblock = parentfunction->addBasicBlock(std::to_string(toploop->getLoopID()) + "thenBf");

  CpHeader(headerblock, predIfblock, phiindex);

  thelast = predIfblock->getInstructions().end();
  thelast--;
  if (!thelast->get()->isConditional()) {
    throw;
  }
  for (int i = 1; i <= 2; i++) {
    auto basicblock = dynamic_cast<BasicBlock *>(thelast->get()->getOperand(i));
    if (toploop->isBasicBlockInLoop(basicblock)) {
      thelast->get()->replaceOperand(i, predthenblock);
    } else {
      int phiindex = 1;
      for (auto pred : basicblock->getPredecessors()) {
        if (pred == headerblock) {
          break;
        }
        phiindex++;
      }
      for (auto &phiinst : basicblock->getInstructions()) {
        if (phiinst->isPhi()) {
          phiinst->addOperand(phiinst->getOperand(phiindex));
        } else {
          break;
        }
      }
      predIfblock->addSuccessor(basicblock);
      basicblock->addPredecessor(predIfblock);
    }
  }

  predIfblock->addSuccessor(predthenblock);
  predthenblock->addPredecessor(predIfblock);
  pBuilder->setPosition(predthenblock, predthenblock->end());
  pBuilder->createUncondBrInst(headerblock, {});
  predthenblock->addSuccessor(headerblock);
  headerblock->replacePredecessor(predheaderblock, predthenblock);
  toploop->setPredHeader(predthenblock);

  // auto parentloop = predheaderblock->getLoop();
  // while (parentloop != nullptr) {
  //   predthenblock->getLoop();
  // }
}

auto SysYLoopspecificator ::CpHeader(BasicBlock *header2cp, BasicBlock *cp2, int phiindex) -> void {
  std::unordered_map<Instruction *, Instruction *> IOriginMapCopy;
  pBuilder->setPosition(cp2, cp2->end());
  for (auto &inst2cp : header2cp->getInstructions()) {
    if (inst2cp->isAlloca()) {
      ;
      ;
    } else if (inst2cp->isLoad()) {
      auto Loadinst = dynamic_cast<LoadInst *>(inst2cp.get());
      auto op1Origin = Loadinst->getPointer();
      auto load = pBuilder->createLoadInst(op1Origin);
      for (auto &indice : Loadinst->getIndices()) {
        auto ConstValue = dynamic_cast<ConstantValue *>(indice->getValue());
        if (ConstValue != nullptr) {
          load->addOperand(ConstValue);
        } else {
          // 原Inst
          auto instindiceOrigin = dynamic_cast<Instruction *>(indice->getValue());
          auto instindiceAfter = IOriginMapCopy[instindiceOrigin];
          if (instindiceAfter == nullptr) {
            load->addOperand(instindiceOrigin);
          } else {
            load->addOperand(instindiceAfter);
          }
        }
      }
      IOriginMapCopy[inst2cp.get()] = load;
      // std::cout << inst2cp->getName() << "  " << load->getName() << std::endl;
    } else if (inst2cp->isStore()) {
      auto storeinst = dynamic_cast<StoreInst *>(inst2cp.get());

      auto opOrigin = inst2cp->getOperand(0);
      auto Const = dynamic_cast<ConstantValue *>(opOrigin);
      if (Const != nullptr) {
        auto op0 = Const;
        auto pointer = storeinst->getPointer();
        auto storecp = pBuilder->createStoreInst(op0, pointer);
        for (auto &indice : storeinst->getIndices()) {
          auto ConstValue = dynamic_cast<ConstantValue *>(indice->getValue());
          if (ConstValue != nullptr) {
            storecp->addOperand(ConstValue);
          } else {
            // 原Inst
            auto instindiceOrigin = dynamic_cast<Instruction *>(indice->getValue());
            auto instindiceAfter = IOriginMapCopy[instindiceOrigin];
            if (instindiceAfter == nullptr) {
              storecp->addOperand(instindiceOrigin);
            } else {
              storecp->addOperand(instindiceAfter);
            }
          }
        }
        IOriginMapCopy[inst2cp.get()] = storecp;
      } else {
        auto op0 = IOriginMapCopy[dynamic_cast<Instruction *>(opOrigin)];
        if (op0 == nullptr) {
          op0 = dynamic_cast<Instruction *>(opOrigin);
        }
        auto pointer = storeinst->getPointer();
        auto storecp = pBuilder->createStoreInst(op0, pointer);
        for (auto &indice : storeinst->getIndices()) {
          auto ConstValue = dynamic_cast<ConstantValue *>(indice->getValue());
          if (ConstValue != nullptr) {
            storecp->addOperand(ConstValue);
          } else {
            // 原Inst
            auto instindiceOrigin = dynamic_cast<Instruction *>(indice->getValue());
            auto instindiceAfter = IOriginMapCopy[instindiceOrigin];
            if (instindiceAfter == nullptr) {
              storecp->addOperand(instindiceOrigin);
            } else {
              storecp->addOperand(instindiceAfter);
            }
          }
        }
        IOriginMapCopy[inst2cp.get()] = storecp;
      }

    }
    //  else if (inst2cp->isLa()) {
    //   auto lainstOrigin = dynamic_cast<LaInst *>(inst2cp.get());
    //   auto pointer = lainstOrigin->getPointer();
    //   auto lainstcp = pBuilder->createLaInst(pointer);
    //   for (auto &indice : lainstOrigin->getIndices()) {
    //     auto ConstValue = dynamic_cast<ConstantValue *>(indice->getValue());
    //     if (ConstValue != nullptr) {
    //       lainstcp->addOperand(ConstValue);
    //     } else {
    //       // 原Inst
    //       auto instindiceOrigin = dynamic_cast<Instruction *>(indice->getValue());
    //       auto instindiceAfter = IOriginMapCopy[instindiceOrigin];
    //       if (instindiceAfter == nullptr) {
    //         lainstcp->addOperand(instindiceOrigin);
    //       } else {
    //         lainstcp->addOperand(instindiceAfter);
    //       }
    //     }
    //   }
    //   IOriginMapCopy[inst2cp.get()] = lainstcp;
    // }
    else if (inst2cp->isMemset()) {
      auto MemsetinstOrigin = dynamic_cast<MemsetInst *>(inst2cp.get());
      auto pointer = MemsetinstOrigin->getPointer();
      auto begin = MemsetinstOrigin->getBegin();
      auto size = MemsetinstOrigin->getSize();
      auto value = MemsetinstOrigin->getValue();

      auto beginConst = dynamic_cast<ConstantValue *>(begin);
      auto sizeConst = dynamic_cast<ConstantValue *>(size);
      auto valueConst = dynamic_cast<ConstantValue *>(value);
      if (beginConst != nullptr) {
        if (sizeConst != nullptr) {
          if (valueConst != nullptr) {
            auto MemsetInstcp = pBuilder->createMemsetInst(pointer, beginConst, sizeConst, valueConst);
            IOriginMapCopy[inst2cp.get()] = MemsetInstcp;
          } else {
            auto valueinst = IOriginMapCopy[dynamic_cast<Instruction *>(value)];
            if (valueinst == nullptr) {
              valueinst = dynamic_cast<Instruction *>(value);
            }
            auto MemsetInstcp = pBuilder->createMemsetInst(pointer, beginConst, sizeConst, valueinst);
            IOriginMapCopy[inst2cp.get()] = MemsetInstcp;
          }
        } else {
          auto sizeinst = IOriginMapCopy[dynamic_cast<Instruction *>(size)];
          if (sizeinst == nullptr) {
            sizeinst = dynamic_cast<Instruction *>(size);
          }
          if (valueConst != nullptr) {
            auto MemsetInstcp = pBuilder->createMemsetInst(pointer, beginConst, sizeinst, valueConst);
            IOriginMapCopy[inst2cp.get()] = MemsetInstcp;
          } else {
            auto valueinst = IOriginMapCopy[dynamic_cast<Instruction *>(value)];
            if (valueinst == nullptr) {
              valueinst = dynamic_cast<Instruction *>(value);
            }
            auto MemsetInstcp = pBuilder->createMemsetInst(pointer, beginConst, sizeinst, valueinst);
            IOriginMapCopy[inst2cp.get()] = MemsetInstcp;
          }
        }

      } else {
        auto begininst = IOriginMapCopy[dynamic_cast<Instruction *>(begin)];
        if (begininst == nullptr) {
          begininst = dynamic_cast<Instruction *>(begin);
        }
        if (sizeConst != nullptr) {
          if (valueConst != nullptr) {
            auto MemsetInstcp = pBuilder->createMemsetInst(pointer, begininst, sizeConst, valueConst);
            IOriginMapCopy[inst2cp.get()] = MemsetInstcp;
          } else {
            auto valueinst = IOriginMapCopy[dynamic_cast<Instruction *>(value)];
            if (valueinst == nullptr) {
              valueinst = dynamic_cast<Instruction *>(value);
            }
            auto MemsetInstcp = pBuilder->createMemsetInst(pointer, begininst, sizeConst, valueinst);
            IOriginMapCopy[inst2cp.get()] = MemsetInstcp;
          }
        } else {
          auto sizeinst = IOriginMapCopy[dynamic_cast<Instruction *>(size)];
          if (sizeinst == nullptr) {
            sizeinst = dynamic_cast<Instruction *>(size);
          }
          if (valueConst != nullptr) {
            auto MemsetInstcp = pBuilder->createMemsetInst(pointer, begininst, sizeinst, valueConst);
            IOriginMapCopy[inst2cp.get()] = MemsetInstcp;
          } else {
            auto valueinst = IOriginMapCopy[dynamic_cast<Instruction *>(value)];
            if (valueinst == nullptr) {
              valueinst = dynamic_cast<Instruction *>(value);
            }
            auto MemsetInstcp = pBuilder->createMemsetInst(pointer, begininst, sizeinst, valueinst);
            IOriginMapCopy[inst2cp.get()] = MemsetInstcp;
          }
        }
      }

    } else if (inst2cp->isPhi()) {
      // TODO(hsz);
      // auto thefirst
      auto phiinstorigin = dynamic_cast<PhiInst *>(inst2cp.get());
      auto source = phiinstorigin->getOperand(phiindex);
      auto destination = phiinstorigin->getOperand(0);
      if (source->isInt() || source->isFloat()) {
        pBuilder->createStoreInst(source, destination);
      } else {
        auto loadInst = pBuilder->createLoadInst(source);
        pBuilder->createStoreInst(loadInst, destination);
      }
    } else if (inst2cp->isUnary()) {
      auto unaryinstOrigin = dynamic_cast<UnaryInst *>(inst2cp.get());
      auto op0Origin = unaryinstOrigin->getOperand();
      auto opconst = dynamic_cast<ConstantValue *>(op0Origin);
      if (opconst != nullptr) {
        auto unaryinstcp = pBuilder->createUnaryInst(inst2cp->getKind(), inst2cp->getType(), opconst);
        IOriginMapCopy[inst2cp.get()] = unaryinstcp;
      } else {
        auto opinst = IOriginMapCopy[dynamic_cast<Instruction *>(op0Origin)];
        if (opinst == nullptr) {
          opinst = dynamic_cast<Instruction *>(op0Origin);
        }
        auto unaryinstcp = pBuilder->createUnaryInst(inst2cp->getKind(), inst2cp->getType(), opinst);
        IOriginMapCopy[inst2cp.get()] = unaryinstcp;
      }
    } else if (inst2cp->isBinary()) {
      auto binaryinstorigin = dynamic_cast<BinaryInst *>(inst2cp.get());
      auto op0orgin = binaryinstorigin->getOperand(0);
      auto op1orgin = binaryinstorigin->getOperand(1);
      auto op0const = dynamic_cast<ConstantValue *>(op0orgin);
      auto op1const = dynamic_cast<ConstantValue *>(op1orgin);
      if (op0const != nullptr) {
        if (op1const != nullptr) {
          auto binaryinstcp =
              pBuilder->createBinaryInst(binaryinstorigin->getKind(), binaryinstorigin->getType(), op0const, op1const);
          IOriginMapCopy[inst2cp.get()] = binaryinstcp;
        } else {
          auto op1inst = IOriginMapCopy[dynamic_cast<Instruction *>(op1orgin)];
          if (op1inst == nullptr) {
            op1inst = dynamic_cast<Instruction *>(op1orgin);
          }
          auto binaryinstcp =
              pBuilder->createBinaryInst(binaryinstorigin->getKind(), binaryinstorigin->getType(), op0const, op1inst);
          IOriginMapCopy[inst2cp.get()] = binaryinstcp;
        }
      } else {
        auto op0inst = IOriginMapCopy[dynamic_cast<Instruction *>(op0orgin)];
        if (op0inst == nullptr) {
          op0inst = dynamic_cast<Instruction *>(op0orgin);
        }
        if (op1const != nullptr) {
          auto binaryinstcp =
              pBuilder->createBinaryInst(binaryinstorigin->getKind(), binaryinstorigin->getType(), op0inst, op1const);
          IOriginMapCopy[inst2cp.get()] = binaryinstcp;
        } else {
          auto op1inst = IOriginMapCopy[dynamic_cast<Instruction *>(op1orgin)];
          if (op1inst == nullptr) {
            op1inst = dynamic_cast<Instruction *>(op1orgin);
          }
          auto binaryinstcp =
              pBuilder->createBinaryInst(binaryinstorigin->getKind(), binaryinstorigin->getType(), op0inst, op1inst);
          IOriginMapCopy[inst2cp.get()] = binaryinstcp;
        }
      }

    } else if (inst2cp->isConditional()) {
      auto condbrinstOrigin = dynamic_cast<CondBrInst *>(inst2cp.get());
      auto conditionOrigin = condbrinstOrigin->getCondition();
      auto conditionconst = dynamic_cast<ConstantValue *>(conditionOrigin);
      if (conditionconst != nullptr) {
        auto condbrinstcp = pBuilder->createCondBrInst(conditionconst, condbrinstOrigin->getThenBlock(),
                                                       condbrinstOrigin->getElseBlock(), {}, {});
        IOriginMapCopy[inst2cp.get()] = condbrinstcp;
      } else {
        auto conditioninst = IOriginMapCopy[dynamic_cast<Instruction *>(conditionOrigin)];
        if (conditioninst == nullptr) {
          conditioninst = dynamic_cast<Instruction *>(conditionOrigin);
        }
        auto condbrinstcp = pBuilder->createCondBrInst(conditioninst, condbrinstOrigin->getThenBlock(),
                                                       condbrinstOrigin->getElseBlock(), {}, {});
        IOriginMapCopy[inst2cp.get()] = condbrinstcp;
      }

    } else if (inst2cp->isBranch()) {
      auto brinstorigin = dynamic_cast<UncondBrInst *>(inst2cp.get());
      auto brinstcp = pBuilder->createUncondBrInst(brinstorigin->getBlock(), {});
      IOriginMapCopy[inst2cp.get()] = brinstcp;

    } else if (inst2cp->isReturn()) {
      auto returninstorigin = dynamic_cast<ReturnInst *>(inst2cp.get());
      if (returninstorigin->hasReturnValue()) {
        auto returnvalueorigin = returninstorigin->getReturnValue();
        auto returnvalueconst = dynamic_cast<ConstantValue *>(returnvalueorigin);
        if (returnvalueconst != nullptr) {
          auto returninstcp = pBuilder->createReturnInst(returnvalueconst);
          IOriginMapCopy[inst2cp.get()] = returninstcp;
        } else {
          auto returnvalueinst = IOriginMapCopy[dynamic_cast<Instruction *>(returnvalueorigin)];
          if (returnvalueinst == nullptr) {
            returnvalueinst = dynamic_cast<Instruction *>(returnvalueorigin);
          }
          auto returninstcp = pBuilder->createReturnInst(returnvalueinst);
          IOriginMapCopy[inst2cp.get()] = returninstcp;
        }
      } else {
        auto returninstcp = pBuilder->createReturnInst();
        IOriginMapCopy[inst2cp.get()] = returninstcp;
      }

    } else if (inst2cp->isCall()) {
      auto callinstorigin = dynamic_cast<CallInst *>(inst2cp.get());
      auto Callee = callinstorigin->getCallee();
      auto callinstcp = pBuilder->createCallInst(Callee, {});
      for (auto &callargmorigin : callinstorigin->getArguments()) {
        // auto globalvalue =

        auto callargmconst = dynamic_cast<ConstantValue *>(callargmorigin->getValue());
        if (callargmconst != nullptr) {
          callinstcp->addOperand(callargmconst);
        } else {
          auto callargminst = IOriginMapCopy[dynamic_cast<Instruction *>(callargmorigin->getValue())];
          if (callargminst == nullptr) {
            callinstcp->addOperand(callargmorigin->getValue());
            // std::cout << toploop->getLoopID() << std::endl;
            // std::cout << callinstorigin->getName() << std::endl;
            // std::cout << "here wrong" << callargmorigin->getValue()->getName() << std::endl;
          } else {
            callinstcp->addOperand(callargminst);
          }
        }
      }
      IOriginMapCopy[inst2cp.get()] = callinstcp;
    } else if (inst2cp->isGetSubArray()) {
      ;
      auto subarrayorigin = dynamic_cast<GetSubArrayInst *>(inst2cp.get());

      auto fatherarray = subarrayorigin->getFatherArray();
      // auto subarray = subarrayorigin->getChildArray();
      // auto subarraycp = pBuilder->createGetSubArrayInst(fatherarray, subarray);
      auto subarraycp = pBuilder->createGetSubArray(dynamic_cast<LVal *>(fatherarray), {});
      for (auto &indice : subarrayorigin->getIndices()) {
        auto ConstValue = dynamic_cast<ConstantValue *>(indice->getValue());
        if (ConstValue != nullptr) {
          subarraycp->addOperand(ConstValue);
        } else {
          // 原Inst
          auto instindiceOrigin = dynamic_cast<Instruction *>(indice->getValue());
          auto instindiceAfter = IOriginMapCopy[instindiceOrigin];
          if (instindiceAfter == nullptr) {
            subarraycp->addOperand(instindiceOrigin);
          } else {
            subarraycp->addOperand(instindiceAfter);
          }
        }
      }
      // std::cout << "here" << subarraycp->getName() << std::endl;
      IOriginMapCopy[inst2cp.get()] = subarraycp;
      auto chridren = dynamic_cast<Instruction *>(subarrayorigin->getChildArray());
      IOriginMapCopy[chridren] = dynamic_cast<Instruction *>(subarraycp->getChildArray());

      // IOriginMapCopy[]
    }
  }
}

}  // namespace sysy