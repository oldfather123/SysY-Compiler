/**
 * @file: SysYLoopUnroll.cpp
 * @brief  循环展开 cpp
 * @Author : Ixeux email:you@domain.com
 * @Version : 1.0
 * @Creat Date : 2024-08-10
 *
 */
#include "../include/midend/SysYLoopUnroll.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
#include <utility>
#include "../include/frontend/IR.h"
#include "../include/frontend/IRBuilder.h"
namespace sysy {

/**
 * @brief Use 关系删除
 * @param  instr:
 */
auto SysYLoopUnroll::usedelete(Instruction *instr) -> void {
  for (auto &use1 : instr->getOperands()) {
    auto val1 = use1->getValue();
    // std::cout << val1->getName() << std::endl;
    val1->removeUse(use1);
  }
}

/**
 * @brief  循环展开，对外接口
 */
auto SysYLoopUnroll::run() -> void {
  auto &functions = pModule->getFunctions();
  for (auto &func : functions) {
    auto &function = func.second;
    for (auto &toploop : function->getTopLoops()) {
      UnRollforLoop(toploop.get());
    }
  }
  // return;
}

/**
 * @brief  对某一循环进行展开
 * @param  toploop: 要展开的loop
 */
auto SysYLoopUnroll::UnRollforLoop(Loop *toploop) -> void {
  auto subloops = toploop->getSubLoops();

  if (!subloops.empty()) {
    for (auto subloop : subloops) {
      UnRollforLoop(subloop);
    }
  } else {
    if (toploop->getindPhi() == nullptr) {
      return;
    }
    auto indphi = toploop->getindPhi();
    if (indphi == nullptr) {
      return;
    }
    auto indendValue = toploop->getindEnd();
    auto indend = dynamic_cast<ConstantValue *>(indendValue);
    auto mode = 0;

    if (indend == nullptr) {
      auto load = dynamic_cast<LoadInst *>(indendValue);
      if (load != nullptr) {
        auto pointer = load->getPointer();
        auto gb = dynamic_cast<GlobalValue *>(pointer);
        if (load->getNumIndices() != 0) {
          return;
        }
        if (gb == nullptr) {
          if (toploop->isBasicBlockInLoop(load->getParent())) {
            return;
          }
          mode = 1;

        } else {
          if (toploop->getGlobalValuechange().find(gb) != toploop->getGlobalValuechange().end()) {
            // statements
            return;
          }

          mode = 1;
        }
      } else {
        return;
      }
    }

    auto indBegin = toploop->getindBegin();
    if (indBegin == nullptr) {
      return;
    }

    auto indStep = toploop->getindStep();

    if (indStep == nullptr) {
      return;
    }

    if (toploop->getexitingBlocks().size() != 1) {
      return;
    }
    if (toploop->getexitingBlocks().find(toploop->getHeader()) == toploop->getexitingBlocks().end()) {
      return;
    }

    auto Icmp = toploop->getIcmpKind();
    if (Icmp == Instruction::kICmpLE || Icmp == Instruction::kICmpLT) {
      // LE 小于等于， LT，小于

      if (toploop->getStepType() != 1) {
        return;
      }
      int UnRollMaxTime = 0;
      int lefttime = 0;
      if (mode == 0) {
        if (indend->isFloat() || indBegin->isFloat() || indStep->isFloat()) {
          return;
        }

        auto Times = indend->getInt() - indBegin->getInt();
        if (Icmp == Instruction::kICmpLE) {
          Times = Times + indStep->getInt();
        }

        // 需要执行的次数
        int unrolltime_1 = Times / indStep->getInt();
        int LoopInstNum = 0;
        for (auto Basicblock : toploop->getBasicBlocks()) {
          LoopInstNum = LoopInstNum + Basicblock->getNumInstructions();
        }
        // // 1000条的限制

        auto unrolltime_2 = 500 / LoopInstNum + 1;

        auto sqrt_num = static_cast<int>(std::sqrt(unrolltime_1));

        // 展开次数
        UnRollMaxTime = 10;
        if (unrolltime_2 < UnRollMaxTime) {
          UnRollMaxTime = unrolltime_2;
        }
        if (sqrt_num < UnRollMaxTime) {
          UnRollMaxTime = sqrt_num;
        }
        if (UnRollMaxTime <= 1) {
          return;
        }

        // UnRollMaxTime = 4;
        lefttime = (unrolltime_1) % UnRollMaxTime;
      } else {
        UnRollMaxTime = 4;
      }
      // UnRollMaxTime = 15;
      // std::vector<int> unrollmaxtimes = {unrolltime_1, unrolltime_2, 15};
      // auto  = std::min(unrollmaxtimes.begin(), unrollmaxtimes.end());

      // cp 次序
      std::list<BasicBlock *> CpBBorder;
      auto &loopbasicblocks = toploop->getBasicBlocks();
      auto BBnum = loopbasicblocks.size();
      auto headblock = toploop->getHeader();
      while (CpBBorder.size() != BBnum) {
        for (auto basicblockOrdercheck : loopbasicblocks) {
          // basicblockOrdercheck
          if (basicblockOrdercheck == headblock) {
            CpBBorder.push_back(basicblockOrdercheck);
            continue;
          }
          bool CanAdd = true;
          for (auto predOfCheck : basicblockOrdercheck->getPredecessors()) {
            if (std::find(CpBBorder.begin(), CpBBorder.end(), predOfCheck) == CpBBorder.end()) {
              CanAdd = false;
              break;
            }
          }
          if (CanAdd) {
            CpBBorder.push_back(basicblockOrdercheck);
          }
        }
      }

      // 展开
      BasicBlock *oldlatch = toploop->getlatchBlock()[0];
      for (int i = 0; i < UnRollMaxTime - 1; i++) {
        auto newblock = CpBlocks(CpBBorder, toploop, std::to_string(i), oldlatch, 1);

        auto newheader = newblock.first;
        auto newlatch = newblock.second;
        auto oldlatch = toploop->getlatchBlock()[0];

        oldlatch->removeSuccessor(toploop->getHeader());
        oldlatch->addSuccessor(newheader);
        auto thelast = oldlatch->getInstructions().end();
        thelast--;
        auto thebrinst = dynamic_cast<UncondBrInst *>(thelast->get());
        if (thebrinst == nullptr) {
          throw;
        }
        thebrinst->replaceOperand(0, newheader);

        newheader->addPredecessor(oldlatch);

        toploop->getHeader()->replacePredecessor(oldlatch, newlatch);
        newlatch->addSuccessor(toploop->getHeader());
        pBuilder->setPosition(newlatch, newlatch->end());
        pBuilder->createUncondBrInst(toploop->getHeader(), {});
        toploop->getlatchBlock().clear();
        toploop->addLatchBlock(newlatch);
      }

      if (lefttime != 0 || mode == 1) {
        auto newblock = CpBlocks(CpBBorder, toploop, "out", oldlatch, 2);
        auto exitheader = newblock.first;
        auto exitlatch = newblock.second;

        auto thelast = toploop->getHeader()->getInstructions().end();
        thelast--;

        auto condbrinstheader = dynamic_cast<CondBrInst *>(thelast->get());

        if (condbrinstheader == nullptr) {
          throw;
        }

        BasicBlock *exitoriginblock = nullptr;
        for (unsigned int exitblockindex = 1; exitblockindex < condbrinstheader->getNumOperands(); exitblockindex++) {
          if (toploop->getexitBlocks().find(dynamic_cast<BasicBlock *>(condbrinstheader->getOperand(exitblockindex))) !=
              toploop->getexitBlocks().end()) {
            exitoriginblock = dynamic_cast<BasicBlock *>(condbrinstheader->getOperand(exitblockindex));
            condbrinstheader->replaceOperand(exitblockindex, exitheader);
          }
        }
        toploop->getHeader()->removeSuccessor(exitoriginblock);
        toploop->getHeader()->addSuccessor(exitheader);
        exitoriginblock->replacePredecessor(toploop->getHeader(), exitheader);
        exitheader->addSuccessor(exitoriginblock);
        // exitoriginblock->

        // auto loopOriginexit = toploop->
        int preheaderindex = 0;
        for (auto pred : toploop->getHeader()->getPredecessors()) {
          if (pred == toploop->getPreheaderBlock()) {
            break;
          }
          preheaderindex++;
        }

        if (preheaderindex == 0) {
          exitheader->addPredecessor(toploop->getHeader());
          exitheader->addPredecessor(exitlatch);
        } else {
          exitheader->addPredecessor(exitlatch);
          exitheader->addPredecessor(toploop->getHeader());
        }

        exitlatch->addSuccessor(exitheader);
        pBuilder->setPosition(exitlatch, exitlatch->end());
        pBuilder->createUncondBrInst(exitheader, {});

        if (Icmp == Instruction::kICmpLT) {
          if (mode == 0) {
            auto LTinst = dynamic_cast<BinaryInst *>(toploop->getindCondVar());
            if (LTinst == nullptr) {
              throw;
            }
            auto op2 = dynamic_cast<ConstantValue *>(LTinst->getOperand(1));
            if (op2 == nullptr) {
              throw;
            }
            auto op2change = indend->getInt() - lefttime * indStep->getInt();
            LTinst->replaceOperand(1, ConstantValue::get(op2change));
          } else {
            // std::cout << "here" << toploop->getLoopID() << std::endl;
            // pBuilder->cr, Value *rhs)
            auto thelast = toploop->getHeader()->getInstructions().end();
            thelast--;
            thelast--;
            pBuilder->setPosition(toploop->getHeader(), thelast);
            auto subinst = pBuilder->createSubInst(toploop->getindEnd(), ConstantValue::get(4));
            auto LTinst = dynamic_cast<BinaryInst *>(toploop->getindCondVar());
            if (LTinst == nullptr) {
              throw;
            }
            LTinst->replaceOperand(1, subinst);
          }
        } else {
          if (mode == 0) {
            auto LEinst = dynamic_cast<BinaryInst *>(toploop->getindCondVar());
            if (LEinst == nullptr) {
              throw;
            }
            auto op2 = dynamic_cast<ConstantValue *>(LEinst->getOperand(1));
            if (op2 == nullptr) {
              throw;
            }
            auto op2change = indend->getInt() - UnRollMaxTime;
            LEinst->replaceOperand(1, ConstantValue::get(op2change));
          } else {
            auto thelast = toploop->getPreheaderBlock()->getInstructions().end();
            thelast--;
            pBuilder->setPosition(toploop->getPreheaderBlock(), thelast);
            auto subinst = pBuilder->createSubInst(toploop->getindEnd(), ConstantValue::get(4));
            auto LTinst = dynamic_cast<BinaryInst *>(toploop->getindCondVar());
            if (LTinst == nullptr) {
              throw;
            }
            LTinst->replaceOperand(1, subinst);
          }
        }
      }
    } else if (Icmp == Instruction::kICmpGE || Icmp == Instruction::kICmpGT) {
      // GE 大于等于， GT，大于
      if (toploop->getStepType() != 2) {
        return;
      }
      int UnRollMaxTime = 0;
      int lefttime = 0;
      if (mode == 0) {
        if (indend->isFloat() || indBegin->isFloat() || indStep->isFloat()) {
          return;
        }

        auto Times = indBegin->getInt() - indend->getInt();
        if (Icmp == Instruction::kICmpGE) {
          Times = Times + indStep->getInt();
        }

        // 需要执行的次数
        int unrolltime_1 = Times / indStep->getInt();
        int LoopInstNum = 0;
        for (auto Basicblock : toploop->getBasicBlocks()) {
          LoopInstNum = LoopInstNum + Basicblock->getNumInstructions();
        }
        // // 1000条的限制

        auto unrolltime_2 = 500 / LoopInstNum + 1;

        auto sqrt_num = static_cast<int>(std::sqrt(unrolltime_1));

        // 展开次数
        UnRollMaxTime = 10;
        if (unrolltime_2 < UnRollMaxTime) {
          UnRollMaxTime = unrolltime_2;
        }
        if (sqrt_num < UnRollMaxTime) {
          UnRollMaxTime = sqrt_num;
        }
        if (UnRollMaxTime <= 1) {
          return;
        }

        // UnRollMaxTime = 4;
        lefttime = (unrolltime_1) % UnRollMaxTime;
      } else {
        UnRollMaxTime = 4;
      }
      // UnRollMaxTime = 15;
      // std::vector<int> unrollmaxtimes = {unrolltime_1, unrolltime_2, 15};
      // auto  = std::min(unrollmaxtimes.begin(), unrollmaxtimes.end());

      // cp 次序
      std::list<BasicBlock *> CpBBorder;
      auto &loopbasicblocks = toploop->getBasicBlocks();
      auto BBnum = loopbasicblocks.size();
      auto headblock = toploop->getHeader();
      while (CpBBorder.size() != BBnum) {
        for (auto basicblockOrdercheck : loopbasicblocks) {
          // basicblockOrdercheck
          if (basicblockOrdercheck == headblock) {
            CpBBorder.push_back(basicblockOrdercheck);
            continue;
          }
          bool CanAdd = true;
          for (auto predOfCheck : basicblockOrdercheck->getPredecessors()) {
            if (std::find(CpBBorder.begin(), CpBBorder.end(), predOfCheck) == CpBBorder.end()) {
              CanAdd = false;
              break;
            }
          }
          if (CanAdd) {
            CpBBorder.push_back(basicblockOrdercheck);
          }
        }
      }

      // 展开
      BasicBlock *oldlatch = toploop->getlatchBlock()[0];
      for (int i = 0; i < UnRollMaxTime - 1; i++) {
        auto newblock = CpBlocks(CpBBorder, toploop, std::to_string(i), oldlatch, 1);

        auto newheader = newblock.first;
        auto newlatch = newblock.second;
        auto oldlatch = toploop->getlatchBlock()[0];

        oldlatch->removeSuccessor(toploop->getHeader());
        oldlatch->addSuccessor(newheader);
        auto thelast = oldlatch->getInstructions().end();
        thelast--;
        auto thebrinst = dynamic_cast<UncondBrInst *>(thelast->get());
        if (thebrinst == nullptr) {
          throw;
        }
        thebrinst->replaceOperand(0, newheader);

        newheader->addPredecessor(oldlatch);

        toploop->getHeader()->replacePredecessor(oldlatch, newlatch);
        newlatch->addSuccessor(toploop->getHeader());
        pBuilder->setPosition(newlatch, newlatch->end());
        pBuilder->createUncondBrInst(toploop->getHeader(), {});
        toploop->getlatchBlock().clear();
        toploop->addLatchBlock(newlatch);
      }

      if (lefttime != 0 || mode == 1) {
        auto newblock = CpBlocks(CpBBorder, toploop, "out", oldlatch, 2);
        auto exitheader = newblock.first;
        auto exitlatch = newblock.second;

        auto thelast = toploop->getHeader()->getInstructions().end();
        thelast--;

        auto condbrinstheader = dynamic_cast<CondBrInst *>(thelast->get());

        if (condbrinstheader == nullptr) {
          throw;
        }

        BasicBlock *exitoriginblock = nullptr;
        for (unsigned int exitblockindex = 1; exitblockindex < condbrinstheader->getNumOperands(); exitblockindex++) {
          if (toploop->getexitBlocks().find(dynamic_cast<BasicBlock *>(condbrinstheader->getOperand(exitblockindex))) !=
              toploop->getexitBlocks().end()) {
            exitoriginblock = dynamic_cast<BasicBlock *>(condbrinstheader->getOperand(exitblockindex));
            condbrinstheader->replaceOperand(exitblockindex, exitheader);
          }
        }
        toploop->getHeader()->removeSuccessor(exitoriginblock);
        toploop->getHeader()->addSuccessor(exitheader);
        exitoriginblock->replacePredecessor(toploop->getHeader(), exitheader);
        exitheader->addSuccessor(exitoriginblock);
        // exitoriginblock->

        // auto loopOriginexit = toploop->
        int preheaderindex = 0;
        for (auto pred : toploop->getHeader()->getPredecessors()) {
          if (pred == toploop->getPreheaderBlock()) {
            break;
          }
          preheaderindex++;
        }

        if (preheaderindex == 0) {
          exitheader->addPredecessor(toploop->getHeader());
          exitheader->addPredecessor(exitlatch);
        } else {
          exitheader->addPredecessor(exitlatch);
          exitheader->addPredecessor(toploop->getHeader());
        }

        exitlatch->addSuccessor(exitheader);
        pBuilder->setPosition(exitlatch, exitlatch->end());
        pBuilder->createUncondBrInst(exitheader, {});

        if (Icmp == Instruction::kICmpLT) {
          if (mode == 0) {
            auto LTinst = dynamic_cast<BinaryInst *>(toploop->getindCondVar());
            if (LTinst == nullptr) {
              throw;
            }
            auto op2 = dynamic_cast<ConstantValue *>(LTinst->getOperand(1));
            if (op2 == nullptr) {
              throw;
            }
            auto op2change = indend->getInt() + lefttime * indStep->getInt();
            LTinst->replaceOperand(1, ConstantValue::get(op2change));
          } else {
            // std::cout << "here" << toploop->getLoopID() << std::endl;
            // pBuilder->cr, Value *rhs)
            auto thelast = toploop->getHeader()->getInstructions().end();
            thelast--;
            thelast--;
            pBuilder->setPosition(toploop->getHeader(), thelast);
            auto addinst = pBuilder->createAddInst(toploop->getindEnd(), ConstantValue::get(4));
            auto LTinst = dynamic_cast<BinaryInst *>(toploop->getindCondVar());
            if (LTinst == nullptr) {
              throw;
            }
            LTinst->replaceOperand(1, addinst);
          }
        } else {
          if (mode == 0) {
            auto LEinst = dynamic_cast<BinaryInst *>(toploop->getindCondVar());
            if (LEinst == nullptr) {
              throw;
            }
            auto op2 = dynamic_cast<ConstantValue *>(LEinst->getOperand(1));
            if (op2 == nullptr) {
              throw;
            }
            auto op2change = indend->getInt() + UnRollMaxTime;
            LEinst->replaceOperand(1, ConstantValue::get(op2change));
          } else {
            auto thelast = toploop->getPreheaderBlock()->getInstructions().end();
            thelast--;
            pBuilder->setPosition(toploop->getPreheaderBlock(), thelast);
            auto addinst = pBuilder->createAddInst(toploop->getindEnd(), ConstantValue::get(4));
            auto LTinst = dynamic_cast<BinaryInst *>(toploop->getindCondVar());
            if (LTinst == nullptr) {
              throw;
            }
            LTinst->replaceOperand(1, addinst);
          }
        }
      }
    }
  }
}

/**
 * @brief  复制块
 * @param  Blocks2Cp: 要复制的块集合
 * @param  toploop: 所在循环
 * @param  time: 第几次展开
 * @param  oldlatch: 原循环的回边
 * @param  headerBlockMode: 复制模式，若为1则为尾循环复制，为0则为展开复制
 * @return std::pair<BasicBlock *, BasicBlock *>:
 */
auto SysYLoopUnroll ::CpBlocks(std::list<BasicBlock *> &Blocks2Cp, Loop *toploop, std::string time,
                               BasicBlock *oldlatch, int headerBlockMode) -> std::pair<BasicBlock *, BasicBlock *> {
  std::unordered_map<Instruction *, Instruction *> IOriginMapCopy;
  std::unordered_map<BasicBlock *, BasicBlock *> BOriginMapCopy;
  // for(ba)
  // copy inst

  for (auto basicblock2cp : Blocks2Cp) {
    auto bbcopy = basicblock2cp->getParent()->addBasicBlock(basicblock2cp->getName() + "_unroll_" + time);
    BOriginMapCopy[basicblock2cp] = bbcopy;
    toploop->addBasicBlock(bbcopy);
    pBuilder->setPosition(bbcopy, bbcopy->end());

    for (auto &inst2cp : basicblock2cp->getInstructions()) {
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
        auto phiinstcp =
            pBuilder->createPhiInst(phiinstorigin->getType(), phiinstorigin->getPointer(), pBuilder->getBasicBlock());
        int index = 0;
        for (auto &phiargm : phiinstorigin->getOperands()) {
          if (index == 0) {
            index++;
            continue;
          }
          phiinstcp->addOperand(phiargm->getValue());
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
            auto binaryinstcp = pBuilder->createBinaryInst(binaryinstorigin->getKind(), binaryinstorigin->getType(),
                                                           op0const, op1const);
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
    // std::cout << "here" << Blocks2Cp.size() << std::endl;
  }

  // 更改前驱后继和condbr

  // std::list<Blocks2Cp
  BasicBlock *newheader;
  BasicBlock *newlatch;

  for (auto B2BSet : BOriginMapCopy) {
    auto originblock = B2BSet.first;
    auto cpblock = B2BSet.second;
    if (originblock == toploop->getHeader()) {
      if (headerBlockMode == 1) {
        int index = 0;
        if (originblock->getPredecessors()[0] == toploop->getPreheaderBlock()) {
          index = 1;
        }
        if (originblock->getPredecessors()[1] == toploop->getPreheaderBlock()) {
          index = 2;
        }
        for (auto institer = cpblock->getInstructions().begin(); institer != cpblock->getInstructions().end();) {
          auto phiinst = institer->get();
          if (phiinst->isPhi()) {
            institer->get()->removeOperand(index);
            institer++;
            continue;
          }
          usedelete(institer->get());
          institer++;
        }
        for (auto institer = cpblock->getInstructions().begin(); institer != cpblock->getInstructions().end();) {
          auto phiinst = institer->get();
          if (phiinst->isPhi()) {
            institer++;
            continue;
          }
          // usedelete(institer->get());
          institer = cpblock->getInstructions().erase(institer);
        }

        int predindex = 0;
        for (auto succ : originblock->getSuccessors()) {
          if (toploop->getexitBlocks().find(succ) != toploop->getexitBlocks().end()) {
            predindex++;
          } else {
            break;
          }
        }

        auto cpnextblock = BOriginMapCopy[originblock->getSuccessors()[predindex]];
        pBuilder->setPosition(cpblock, cpblock->end());
        pBuilder->createUncondBrInst(cpnextblock, {});
        // originblock->
        cpblock->addSuccessor(cpnextblock);

        newheader = cpblock;
      } else {
        int index = 0;
        if (originblock->getPredecessors()[0] == toploop->getPreheaderBlock()) {
          index = 1;
        }
        if (originblock->getPredecessors()[1] == toploop->getPreheaderBlock()) {
          index = 2;
        }
        for (auto institer = cpblock->getInstructions().begin(); institer != cpblock->getInstructions().end();) {
          auto phiinst = institer->get();
          if (phiinst->isPhi()) {
            institer->get()->replaceOperand(index, phiinst->getOperand(0));
            institer++;
            continue;
          }
          institer++;
        }

        BasicBlock *exitblock = nullptr;
        BasicBlock *innerblock = nullptr;
        for (auto succ : toploop->getHeader()->getSuccessors()) {
          if (toploop->getexitBlocks().find(succ) != toploop->getexitBlocks().end()) {
            exitblock = succ;
          } else {
            innerblock = succ;
          }
        }

        auto thelast = cpblock->getInstructions().end();
        thelast--;
        assert(thelast->get()->getKind() == Instruction::kCondBr);

        for (unsigned int blockindex = 1; blockindex < thelast->get()->getNumOperands(); blockindex++) {
          if (dynamic_cast<BasicBlock *>(thelast->get()->getOperand(blockindex)) == exitblock) {
            continue;
          }
          thelast->get()->replaceOperand(blockindex, BOriginMapCopy[innerblock]);
        }

        cpblock->addSuccessor(BOriginMapCopy[innerblock]);

        newheader = cpblock;
      }
    } else if (originblock == oldlatch) {
      for (auto pred : originblock->getPredecessors()) {
        cpblock->addPredecessor(BOriginMapCopy[pred]);
      }
      auto thelast = cpblock->getInstructions().end();
      thelast--;
      usedelete(thelast->get());
      cpblock->getInstructions().erase(thelast);
      newlatch = cpblock;
    } else {
      for (auto pred : originblock->getPredecessors()) {
        cpblock->addPredecessor(BOriginMapCopy[pred]);
      }
      for (auto succ : originblock->getSuccessors()) {
        cpblock->addSuccessor(BOriginMapCopy[succ]);
      }
      auto thelast = cpblock->getInstructions().end();
      thelast--;
      if (thelast->get()->isConditional()) {
        auto thenblockorigin = thelast->get()->getOperand(1);
        auto elseblockorigin = thelast->get()->getOperand(2);

        thelast->get()->replaceOperand(1, BOriginMapCopy[dynamic_cast<BasicBlock *>(thenblockorigin)]);
        thelast->get()->replaceOperand(2, BOriginMapCopy[dynamic_cast<BasicBlock *>(elseblockorigin)]);
      } else if (thelast->get()->isBranch()) {
        auto thenblockorigin = thelast->get()->getOperand(0);
        thelast->get()->replaceOperand(0, BOriginMapCopy[dynamic_cast<BasicBlock *>(thenblockorigin)]);
      }
    }
  }
  return std::make_pair(newheader, newlatch);
}
}  // namespace sysy