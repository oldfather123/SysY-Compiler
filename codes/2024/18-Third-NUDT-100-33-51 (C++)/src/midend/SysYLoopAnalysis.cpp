/**
 * @file: SysYLoopAnalysis.cpp
 * @brief  循环分析
 * @Author : Ixeux email:you@domain.com
 * @Version : 1.0
 * @Creat Date : 2024-08-10
 *
 */
#include "../include/midend/SysYLoopAnalysis.h"
#include <cstddef>
#include <iostream>
#include <set>
#include <stack>
#include <unordered_map>
#include <vector>
#include "../include/frontend/IR.h"
#include "../include/utils/SysyDebugger.h"
namespace sysy {

/**
 * @brief  循环分析，用在SSA之前
 */
auto SysYLoopAnalysis::runBfSSA() -> void {
  Loopclear();
  LoopAnalysize();
}

/**
 * @brief  循环分析，用在SSA之后
 */
auto SysYLoopAnalysis::runAfSSA() -> void {
  Loopclear();
  LoopAnalysize();
  LoopfindSimpleLoopVariables();
  LoopfindGlobalchange();
}

/**
 * @brief  寻找循环内改变的Globalchange
 */
auto SysYLoopAnalysis::LoopfindGlobalchange() -> void {
  auto &functions = pModule->getFunctions();
  for (auto &func : functions) {
    auto &function = func.second;
    for (auto &toploop : function->getTopLoops()) {
      for (auto basicblock : toploop->getBasicBlocks()) {
        for (auto &inst : basicblock->getInstructions()) {
          // for(auto )
          if (inst->isStore()) {
            auto storeinst = dynamic_cast<StoreInst *>(inst.get());

            auto pointer = storeinst->getPointer();
            auto glob = dynamic_cast<GlobalValue *>(pointer);
            if (pointer != nullptr) {
              toploop->addGlobalValuechange(glob);
            }
          } else if (inst->isCall()) {
            // TODO(hsz)
          }
        }
      }
    }
    for (auto &loop : function->getTopLoops()) {
      for (auto basicblock : loop->getBasicBlocks()) {
        for (auto &inst : basicblock->getInstructions()) {
          // for(auto )
          if (inst->isStore()) {
            auto storeinst = dynamic_cast<StoreInst *>(inst.get());

            auto pointer = storeinst->getPointer();
            auto glob = dynamic_cast<GlobalValue *>(pointer);
            if (pointer != nullptr) {
              loop->addGlobalValuechange(glob);
            }
          } else if (inst->isCall()) {
            // TODO(hsz)
          }
        }
      }
    }
  }
}

/**
 * @brief  找循环开始和循环步长
 */
auto SysYLoopAnalysis::LoopfindPhiBeginAndStep() -> void {
  auto &functions = pModule->getFunctions();
  for (auto &func : functions) {
    auto &function = func.second;
    for (auto &toploop : function->getTopLoops()) {
      auto loopphi = toploop->getindPhi();
      if (loopphi == nullptr) {
        continue;
      }
      for (auto &use : loopphi->getUses()) {
        auto phiinst = dynamic_cast<PhiInst *>(use->getUser());
        if (phiinst != nullptr) {
          int index = 1;
          for (auto pred : toploop->getHeader()->getPredecessors()) {
            if (pred == toploop->getPreheaderBlock()) {
              break;
            }
            index++;
          }
          // std::cout << "Loop " << toploop->getLoopID() << std::endl;
          // std::cout << index << std::endl;

          // std::cout << phiinst->getNumOperands() << std::endl;
          auto beginConst = dynamic_cast<ConstantValue *>(phiinst->getOperand(index));
          if (beginConst != nullptr) {
            toploop->setindBegin(beginConst);
          }

          index = 1;
          for (auto pred : toploop->getHeader()->getPredecessors()) {
            if (pred != toploop->getPreheaderBlock()) {
              break;
            }
            index++;
          }
          auto lastOp = phiinst->getOperand(index);
          auto Global = dynamic_cast<GlobalValue *>(lastOp);
          if (Global != nullptr) {
            continue;
          }

          auto allocLast = dynamic_cast<AllocaInst *>(lastOp);

          if (allocLast != nullptr) {
            for (auto &use : allocLast->getUses()) {
              auto storeinst = dynamic_cast<StoreInst *>(use->getUser());

              if (storeinst != nullptr) {
                auto opresult = storeinst->getOperand(0);
                auto Binary = dynamic_cast<BinaryInst *>(opresult);

                if (Binary != nullptr) {
                  auto thefirst = dynamic_cast<LoadInst *>(Binary->getOperand(0));
                  if (thefirst == nullptr) {
                    continue;
                  }
                  // std::cout << toploop->getLoopID() << std::endl;
                  // std::cout << thefirst->getName() << std::endl;
                  auto theAlloc = dynamic_cast<AllocaInst *>(thefirst->getOperand(0));
                  // std::cout << theAlloc->getName() << std::endl;
                  if (theAlloc == loopphi) {
                    if (Binary->getKind() == Instruction::kAdd) {
                      auto StepConst = dynamic_cast<ConstantValue *>(Binary->getOperand(1));
                      if (StepConst != nullptr) {
                        toploop->setindStep(StepConst);
                        toploop->setStepType(1);
                      }

                      // if(Binary)
                    }
                    if (Binary->getKind() == Instruction::kSub) {
                      auto StepConst = dynamic_cast<ConstantValue *>(Binary->getOperand(1));
                      if (StepConst != nullptr) {
                        toploop->setindStep(StepConst);
                        toploop->setStepType(2);
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    for (auto &loop : function->getLoops()) {
      auto loopphi = loop->getindPhi();
      if (loopphi == nullptr) {
        continue;
      }

      for (auto &use : loopphi->getUses()) {
        auto phiinst = dynamic_cast<PhiInst *>(use->getUser());
        if (phiinst != nullptr) {
          int index = 1;
          for (auto pred : loop->getHeader()->getPredecessors()) {
            if (pred == loop->getPreheaderBlock()) {
              break;
            }
            index++;
          }
          auto beginConst = dynamic_cast<ConstantValue *>(phiinst->getOperand(index));
          if (beginConst != nullptr) {
            loop->setindBegin(beginConst);
          }

          index = 1;
          for (auto pred : loop->getHeader()->getPredecessors()) {
            if (pred != loop->getPreheaderBlock()) {
              break;
            }
            index++;
          }
          auto lastOp = phiinst->getOperand(index);
          auto Global = dynamic_cast<GlobalValue *>(lastOp);
          if (Global != nullptr) {
            continue;
          }

          auto allocLast = dynamic_cast<AllocaInst *>(lastOp);

          if (allocLast != nullptr) {
            for (auto &use : allocLast->getUses()) {
              auto storeinst = dynamic_cast<StoreInst *>(use->getUser());

              if (storeinst != nullptr) {
                auto opresult = storeinst->getOperand(0);
                auto Binary = dynamic_cast<BinaryInst *>(opresult);

                if (Binary != nullptr) {
                  auto thefirst = dynamic_cast<LoadInst *>(Binary->getOperand(0));
                  if (thefirst == nullptr) {
                    continue;
                  }
                  // std::cout << loop->getLoopID() << std::endl;
                  // std::cout << thefirst->getName() << std::endl;
                  auto theAlloc = dynamic_cast<AllocaInst *>(thefirst->getOperand(0));
                  // std::cout << theAlloc->getName() << std::endl;
                  if (theAlloc == loopphi) {
                    if (Binary->getKind() == Instruction::kAdd) {
                      auto StepConst = dynamic_cast<ConstantValue *>(Binary->getOperand(1));
                      if (StepConst != nullptr) {
                        loop->setindStep(StepConst);
                        loop->setStepType(1);
                      }

                      // if(Binary)
                    }
                    if (Binary->getKind() == Instruction::kSub) {
                      auto StepConst = dynamic_cast<ConstantValue *>(Binary->getOperand(1));
                      if (StepConst != nullptr) {
                        loop->setindStep(StepConst);
                        loop->setStepType(2);
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

/**
 * @brief 循环清除
 */
auto SysYLoopAnalysis::Loopclear() -> void {
  auto &functions = pModule->getFunctions();
  for (auto &function : functions) {
    function.second->getLoops().clear();
    function.second->getTopLoops().clear();
    function.second->getBBToLoopRef().clear();

    for (auto &basicblock : function.second->getBasicBlocks()) {
      basicblock->setLoopDepth(0);
      basicblock->setLoop(nullptr);
    }
  }
}

/**
 * @brief  发现循环A的子循环
 * @param  loop: 循环A
 * @param  function: 父function
 * @param  BackedgeTo: 回边
 * @return auto:
 */
auto SysYLoopAnalysis::discoverAndMapSubloop(Loop *loop, Function *function, std::stack<BasicBlock *> &BackedgeTo) {
  while (!BackedgeTo.empty()) {
    auto pred = BackedgeTo.top();
    BackedgeTo.pop();
    // std::cout << "pred:" << pred->getName() << std::endl;
    // loop->addSubloop(pred->getLoop());
    auto subloop = function->getLoopOfBasicBlock(pred);

    if (subloop == nullptr) {
      function->addBBToLoop(pred, loop);
      if (pred == loop->getHeader()) {
        continue;
      }
      for (auto predofPred : pred->getPredecessors()) {
        BackedgeTo.push(predofPred);
      }
    } else {
      while ((subloop->getParentLoop()) != nullptr) {
        subloop = subloop->getParentLoop();
        // subloop = parent;
      }
      if (subloop == loop) {
        continue;
      }
      subloop->setParentLoop(loop);
      pred = subloop->getHeader();
      for (auto predOfPred : pred->getPredecessors()) {
        if ((function->getLoopOfBasicBlock(predOfPred) == nullptr) ||
            function->getLoopOfBasicBlock(predOfPred) != subloop) {
          BackedgeTo.push(predOfPred);
        }
      }
    }
  }
}

/**
 * @brief  循环分析
 */
auto SysYLoopAnalysis::LoopAnalysize() -> void {
  Loopclear();
  // 遍历的顺序深度优先搜索
  auto &functions = pModule->getFunctions();
  for (auto &function : functions) {
    auto entryBlock = function.second->getEntryBlock();
    std::vector<BasicBlock *> visitOrder;  // 深度遍历后的顺序
    std::set<BasicBlock *> visited;        // 深度遍历过程中的
    visited.clear();
    DFSBasicBlock2VisitOrder(entryBlock, visited, visitOrder);
    // std::cout << visitOrder.size() << std::endl;
    // 对每个块进行 寻找loop 和标记其所在的parentloop， sublopp

    std::stack<BasicBlock *> BackedgeTo;
    for (auto header : visitOrder) {
      // std::cout << header->getName() << std::endl;
      for (auto pred : header->getPredecessors()) {
        // std::cout << "    " << pred->getName() << std::endl;
        if (pred->getDominants().find(header) != pred->getDominants().end()) {
          BackedgeTo.push(pred);
          // std::cout << "BackedgeTo" << pred->getName() << std::endl;
        }
      }
      if (!BackedgeTo.empty()) {
        // std::cout << "herebegin" << std::endl;
        // std::cout << BackedgeTo.top()->getName() << std::endl;
        // auto loop = new (pModule) Loop(header, BackedgeTo.top());
        auto loop = new Loop(header);
        loop->setloopID();
        // looptodelete.push_back(loop);
        // function.second->addLoop(loop);
        discoverAndMapSubloop(loop, function.second.get(), BackedgeTo);
      }
    }
    visited.clear();
    // std::cout << "here" << std::endl;
    // sysy::SysyDebugger::printLoopInfo(pModule);

    // for (auto &basicblock : function.second->getBasicBlocks()) {
    //   if ((function.second)->getLoopOfBasicBlock(basicblock.get()) != nullptr) {
    //     std::cout << basicblock->getName() << (function.second)->getLoopOfBasicBlock(basicblock.get())->getLoopID()
    //               << std::endl;
    //   }
    // }
    DFSBasicBlock2GetsubLoop(entryBlock, visited);

    // std::stack<std::vector<Loop *>> loopStack(function.second->getTopLoops());

    std::stack<Loop *> loopstack;
    for (auto &TopLoop : function.second->getTopLoops()) {
      loopstack.push(TopLoop.get());
    }
    while (!loopstack.empty()) {
      auto looptoop = loopstack.top();
      loopstack.pop();
      bool label = false;
      for (auto &iter : function.second->getTopLoops()) {
        if (iter.get() == looptoop) {
          label = true;
          break;
        }
      }
      if (!label) {
        function.second->addLoop(looptoop);
      }

      for (auto subloop : looptoop->getSubLoops()) {
        subloop->setLoopDepth(looptoop->getLoopDepth() + 1);
        loopstack.push(subloop);
      }
    }
    // std::cout << function.second->getLoops().size() << std::endl;

    // 计算ExitingBlock
    for (auto &loop : function.second->getLoops()) {
      for (auto &basicblock : loop->getBasicBlocks()) {
        auto thelast = basicblock->getInstructions().end();
        if (basicblock->getNumInstructions() == 0) {
          continue;
        }
        (thelast--);
        auto lastinst = thelast->get();
        if (lastinst->getKind() == Instruction::kCondBr) {
          auto thenblock = dynamic_cast<CondBrInst *>(lastinst)->getThenBlock();
          auto elseblock = dynamic_cast<CondBrInst *>(lastinst)->getElseBlock();
          // if(!lo)
          if (!loop->isBasicBlockInLoop(thenblock) || !loop->isBasicBlockInLoop(elseblock)) {
            loop->addExitingBlocks(basicblock);
          }
        }
      }
    }
    for (auto &loop : function.second->getTopLoops()) {
      for (auto &basicblock : loop->getBasicBlocks()) {
        auto thelast = basicblock->getInstructions().end();
        if (basicblock->getNumInstructions() == 0) {
          continue;
        }
        (thelast--);
        auto lastinst = thelast->get();
        if (lastinst->getKind() == Instruction::kCondBr) {
          auto thenblock = dynamic_cast<CondBrInst *>(lastinst)->getThenBlock();
          auto elseblock = dynamic_cast<CondBrInst *>(lastinst)->getElseBlock();
          // if(!lo)
          if (!loop->isBasicBlockInLoop(thenblock) || !loop->isBasicBlockInLoop(elseblock)) {
            loop->addExitingBlocks(basicblock);
          }
        }
      }
    }

    // 计算ExitBlocks
    for (auto &loop : function.second->getLoops()) {
      for (auto basicblock : loop->getBasicBlocks()) {
        for (auto succBasicBlock : basicblock->getSuccessors()) {
          if (!loop->isBasicBlockInLoop(succBasicBlock)) {
            loop->addExitBlocks(succBasicBlock);
          }
        }
      }
    }
    for (auto &loop : function.second->getTopLoops()) {
      for (auto basicblock : loop->getBasicBlocks()) {
        for (auto succBasicBlock : basicblock->getSuccessors()) {
          if (!loop->isBasicBlockInLoop(succBasicBlock)) {
            loop->addExitBlocks(succBasicBlock);
          }
        }
      }
    }

    // 计算Latch block
    for (auto &loop : function.second->getLoops()) {
      // BasicBlock *latch = nullptr;
      for (auto predblock : loop->getHeader()->getPredecessors()) {
        if (loop->isBasicBlockInLoop(predblock)) {
          // latch = predblock;
          loop->addLatchBlock(predblock);
        }
      }
    }
    for (auto &loop : function.second->getTopLoops()) {
      // BasicBlock *latch = nullptr;
      for (auto predblock : loop->getHeader()->getPredecessors()) {
        if (loop->isBasicBlockInLoop(predblock)) {
          // latch = predblock;
          loop->addLatchBlock(predblock);
        }
      }
    }
    // 计算preheader
    for (auto &loop : function.second->getLoops()) {
      BasicBlock *predHeader = nullptr;
      bool setif = false;
      for (auto predblock : loop->getHeader()->getPredecessors()) {
        if (!loop->isBasicBlockInLoop(predblock)) {
          if (!setif) {
            predHeader = predblock;
            setif = true;
          } else {
            setif = false;
            predHeader = nullptr;
            break;
          }
        }
      }
      // assert(predHeader != nullptr && predHeader->getNumSuccessors() == 1);
      loop->setPredHeader(setif ? predHeader : nullptr);
    }
    for (auto &loop : function.second->getTopLoops()) {
      bool setif = false;
      BasicBlock *predHeader = nullptr;
      for (auto predblock : loop->getHeader()->getPredecessors()) {
        if (!loop->isBasicBlockInLoop(predblock)) {
          if (!setif) {
            predHeader = predblock;
            setif = true;
          } else {
            setif = false;
            predHeader = nullptr;
            break;
          }
        }
      }
      // assert(predHeader != nullptr && predHeader->getNumSuccessors() == 1);
      loop->setPredHeader(setif ? predHeader : nullptr);
    }

    // 计算循环变量

    // loop2bb
    for (auto &basicblock : function.second->getBasicBlocks()) {
      basicblock->setLoop(function.second->getLoopOfBasicBlock(basicblock.get()));
      if (basicblock->getLoop() != nullptr) {
        basicblock->setLoopDepth(basicblock->getLoop()->getLoopDepth());
      }
    }
  }
}

/**
 * @brief  查找循环变量
 */
auto SysYLoopAnalysis::LoopfindSimpleLoopVariables() -> void {
  auto &functions = pModule->getFunctions();
  for (auto &function : functions) {
    for (auto &loop : function.second->getTopLoops()) {
      auto header = loop->getHeader();
      auto thelastinst = header->getInstructions().end();
      (thelastinst--);
      if (thelastinst->get()->getKind() == Instruction::kCondBr) {
        // TODO(hsz)
        // std::cout << "here" << std::endl;
        auto thecondition = dynamic_cast<CondBrInst *>(thelastinst->get())->getCondition();
        if (dynamic_cast<Instruction *>(thecondition) == nullptr) {
          continue;
        }

        if (dynamic_cast<Instruction *>(thecondition)->isCmp()) {
          // std::cout << "here11" << std::endl;
          loop->setIndexCondInstr(dynamic_cast<Instruction *>(thecondition));
          loop->setIcmpKind(dynamic_cast<Instruction *>(thecondition)->getKind());
        } else {
          continue;
        }
        auto icmpLhs = dynamic_cast<Instruction *>(thecondition)->getOperand(0);
        auto icmpRhs = dynamic_cast<Instruction *>(thecondition)->getOperand(1);

        auto LhsConstvalue = dynamic_cast<ConstantValue *>(icmpLhs);

        if (LhsConstvalue != nullptr) {
          // std::cout << loop->getLoopID() << std::endl;
          loop->setindEnd(icmpLhs);
          auto instr = dynamic_cast<Instruction *>(icmpRhs);
          if (instr != nullptr) {
            auto phi = getPhi(instr, loop.get());
            if (phi != nullptr) {
              loop->setindPhi(phi);
            }
          }
          continue;
        }

        auto RhsConstvalue = dynamic_cast<ConstantValue *>(icmpRhs);
        if (RhsConstvalue != nullptr) {
          // std::cout << loop->getLoopID() << std::endl;
          loop->setindEnd(icmpRhs);
          auto instr = dynamic_cast<Instruction *>(icmpLhs);
          if (instr != nullptr) {
            auto phi = getPhi(instr, loop.get());
            if (phi != nullptr) {
              loop->setindPhi(phi);
            }
          }
          continue;
        }

        if (loop->isSimpleLoopInvariant(icmpLhs)) {
          loop->setindEnd(icmpLhs);
          auto instr = dynamic_cast<Instruction *>(icmpRhs);
          if (instr != nullptr && !loop->isSimpleLoopInvariant(icmpRhs)) {
            auto phi = getPhi(instr, loop.get());
            if (phi != nullptr) {
              loop->setindPhi(phi);
            }
          }
        } else if (loop->isSimpleLoopInvariant(icmpRhs)) {
          loop->setindEnd(icmpRhs);

          auto instr = dynamic_cast<Instruction *>(icmpLhs);
          if (instr != nullptr && !loop->isSimpleLoopInvariant(icmpLhs)) {
            auto phi = getPhi(instr, loop.get());

            if (phi != nullptr) {
              // std::cout << "here111" << std::endl;
              // std::cout << "here" << std::endl;
              loop->setindPhi(phi);
            }
          }
        }
      }
    }
    for (auto &loop : function.second->getLoops()) {
      auto header = loop->getHeader();
      auto thelastinst = header->getInstructions().end();
      (thelastinst--);
      if (thelastinst->get()->getKind() == Instruction::kCondBr) {
        // TODO(hsz)

        // std::cout << "here" << std::endl;
        auto thecondition = dynamic_cast<CondBrInst *>(thelastinst->get())->getCondition();
        if (dynamic_cast<Instruction *>(thecondition) == nullptr) {
          continue;
        }

        if (dynamic_cast<Instruction *>(thecondition)->isCmp()) {
          // std::cout << "here11" << std::endl;
          loop->setIndexCondInstr(dynamic_cast<Instruction *>(thecondition));
          loop->setIcmpKind(dynamic_cast<Instruction *>(thecondition)->getKind());
        } else {
          continue;
        }
        auto icmpLhs = dynamic_cast<Instruction *>(thecondition)->getOperand(0);
        auto icmpRhs = dynamic_cast<Instruction *>(thecondition)->getOperand(1);

        auto LhsConstvalue = dynamic_cast<ConstantValue *>(icmpLhs);

        if (LhsConstvalue != nullptr) {
          // std::cout << loop->getLoopID() << std::endl;
          loop->setindEnd(icmpLhs);
          auto instr = dynamic_cast<Instruction *>(icmpRhs);
          if (instr != nullptr) {
            auto phi = getPhi(instr, loop.get());
            if (phi != nullptr) {
              loop->setindPhi(phi);
            }
          }
          continue;
        }

        auto RhsConstvalue = dynamic_cast<ConstantValue *>(icmpRhs);
        if (RhsConstvalue != nullptr) {
          // std::cout << loop->getLoopID() << std::endl;
          loop->setindEnd(icmpRhs);
          auto instr = dynamic_cast<Instruction *>(icmpLhs);
          if (instr != nullptr) {
            auto phi = getPhi(instr, loop.get());
            if (phi != nullptr) {
              loop->setindPhi(phi);
            }
          }
          continue;
        }
        // std::cout << "here11" << std::endl;
        if (loop->isSimpleLoopInvariant(icmpLhs)) {
          loop->setindEnd(icmpLhs);
          auto instr = dynamic_cast<Instruction *>(icmpRhs);
          if (instr != nullptr && !loop->isSimpleLoopInvariant(icmpRhs)) {
            auto phi = getPhi(instr, loop.get());
            if (phi != nullptr) {
              loop->setindPhi(phi);
            }
          }
        } else if (loop->isSimpleLoopInvariant(icmpRhs)) {
          loop->setindEnd(icmpRhs);

          auto instr = dynamic_cast<Instruction *>(icmpLhs);
          if (instr != nullptr && !loop->isSimpleLoopInvariant(icmpLhs)) {
            auto phi = getPhi(instr, loop.get());

            if (phi != nullptr) {
              loop->setindPhi(phi);
            }
          }
        }
      }
    }
  }
}

/**
 * @brief  Basicblock 深度遍历，逆拓扑
 * @param  entry: entryblock
 * @param  visited: 已经遍历过的
 * @param  visitOrder: 逆拓扑顺序
 */
auto SysYLoopAnalysis::DFSBasicBlock2VisitOrder(BasicBlock *entry, std::set<BasicBlock *> &visited,
                                                std::vector<BasicBlock *> &visitOrder) -> void {
  if (visited.count(entry) != 0) {
    return;
  }
  visited.insert(entry);
  // std ::cout << entry->getSdoms().size() << std::endl;
  for (auto succ : entry->getSdoms()) {
    // std::cout << "herereach" << std::endl;
    DFSBasicBlock2VisitOrder(succ, visited, visitOrder);
  }
  // std::cout << "pushing" << entry->getName() << std::endl;
  visitOrder.push_back(entry);
}

/**
 * @brief 深度遍历找子循环
 * @param  entry: entryblock
 * @param  visited: 已经遍历过的
 */
auto SysYLoopAnalysis::DFSBasicBlock2GetsubLoop(BasicBlock *entry, std::set<BasicBlock *> &visited) -> void {
  if (visited.find(entry) != visited.end()) {
    return;
  }
  visited.insert(entry);
  for (auto succ : entry->getSuccessors()) {
    DFSBasicBlock2GetsubLoop(succ, visited);
  }
  auto subloop = entry->getParent()->getLoopOfBasicBlock(entry);
  // // std::cout << entry->getName() << std::endl;
  if (subloop != nullptr && entry == subloop->getHeader()) {
    if (subloop->getParentLoop() != nullptr) {
      subloop->getParentLoop()->addSubLoop(subloop);
    } else {
      entry->getParent()->addTopLoop(subloop);
      subloop->setLoopDepth(1);  // topLoop is 1
    }

    //   // reverse but no header
    reverse(subloop->getBasicBlocks().begin() + 1, subloop->getBasicBlocks().end());
    reverse(subloop->getSubLoops().begin(), subloop->getSubLoops().end());
    subloop = subloop->getParentLoop();
  }

  // computeLoopBB
  for (; subloop != nullptr; subloop = subloop->getParentLoop()) {
    subloop->addBasicBlock(entry);
  }
}

/**
 * @brief  清除循环，没用到
 */
auto SysYLoopAnalysis::clean() -> void {
  while (!looptodelete.empty()) {
    auto tmp = looptodelete.front();
    looptodelete.erase(looptodelete.begin());
    delete (tmp);
  }
}

/**
 * @brief  获得循环phi节点alloc
 * @param  instr: ICMP指令
 * @param  loop: 循环phi节点所在的loop
 * @return AllocaInst*: phi节点alloc
 */
auto SysYLoopAnalysis::getPhi(Instruction *instr, Loop *loop) -> AllocaInst * {
  // std::cout << instr->getKindString() << std::endl;
  // std::cout << loop->getLoopID() << std::endl;
  if (auto alloc = dynamic_cast<AllocaInst *>(instr)) {
    for (auto &use : alloc->getUses()) {
      if (auto phi = dynamic_cast<PhiInst *>(use->getUser())) {
        if (use->getIndex() == 0) {
          return alloc;
        }
      }
    }
  }

  if (auto binary = dynamic_cast<BinaryInst *>(instr)) {
    auto lhs = binary->getOperand(0);
    auto rhs = binary->getOperand(1);
    if (lhs != nullptr && !loop->isSimpleLoopInvariant(lhs)) {
      if (auto phi = getPhi(dynamic_cast<Instruction *>(lhs), loop)) {
        return phi;
      }
    }
    if (rhs != nullptr && !loop->isSimpleLoopInvariant(rhs)) {
      if (auto phi = getPhi(dynamic_cast<Instruction *>(rhs), loop)) {
        return phi;
      }
    }
  }

  if (auto unary = dynamic_cast<UnaryInst *>(instr)) {
    auto lhs = unary->getOperand();
    if (lhs != nullptr && !loop->isSimpleLoopInvariant(lhs)) {
      if (auto phi = getPhi(dynamic_cast<Instruction *>(lhs), loop)) {
        return phi;
      }
    }
  }
  if (auto load = dynamic_cast<LoadInst *>(instr)) {
    auto ptr = load->getPointer();
    // std::cout << ptr->getName() << std::endl;
    // auto alloc = dynamic_cast<AllocaInst *>(ptr);
    // std::cout << alloc->getParent()->getName() << std::endl;
    // if (!loop->isSimpleLoopInvariant(ptr)) {
    //   std::cout << "nullptr" << std::endl;
    // }
    // std::cout << :
    if (ptr != nullptr && !loop->isSimpleLoopInvariant(ptr)) {
      // std::cout << "here" << std::endl;

      if (auto phi = getPhi(dynamic_cast<Instruction *>(ptr), loop)) {
        // std::cout << "here" << std::endl;
        return phi;
      }
    }
  }
  return nullptr;
}

// auto SysYLoopAnalysis::ScalarEvolution(Loop *loop) -> void {
//   if (loop == nullptr) {
//     return;
//   }

//   for (auto subloop : loop->getSubLoops()) {
//     ScalarEvolution(subloop);
//   }
//   loop->cleanSCEV();

//   // FindAndRegisterBIV(loop);
//   bool fixed = false;
// }

// auto FindAndRegisterBIV(Loop *loop2Find) {
//   auto header = loop2Find->getHeader();
//   std::vector<PhiInst *> philist;
//   for (auto &instr : header->getInstructions()) {
//     if (instr->isPhi()) {
//       auto phi = dynamic_cast<PhiInst *>(instr.get());
//       philist.push_back(phi);
//     } else {
//       break;
//     }
//   }

//   for (auto phi : philist) {
//   }
// }

auto SysYLoopAnalysis::LoopParallelCheck() -> void {
  auto &functions = pModule->getFunctions();

  for (auto &func : functions) {
    auto &function = func.second;
    for (auto &toploop : function->getTopLoops()) {
      bool parallelable = true;

      // 判断有无循环变量
      if (toploop->getindPhi() == nullptr) {
        toploop->setParallelableFalse();
        break;
      }
      auto headerblock = toploop->getHeader();
      int phiinstnum = 0;
      // 判断多少个phi
      for (auto &phiinst : headerblock->getInstructions()) {
        if (phiinst->isPhi()) {
          phiinstnum++;
          if (phiinstnum > 1) {
            toploop->setParallelableFalse();
            parallelable = false;
            break;
          }
        }
      }
      if (!parallelable) {
        continue;
      }
      std::unordered_map<AllocaInst *, std::list<Instruction *>> localloaddic;
      std::unordered_map<AllocaInst *, std::list<Instruction *>> localstoredic;
      std::unordered_map<GlobalValue *, std::list<Instruction *>> globalloaddic;
      std::unordered_map<GlobalValue *, std::list<Instruction *>> globalstoredic;
      for (auto basicblock : toploop->getBasicBlocks()) {
        for (auto &inst : basicblock->getInstructions()) {
          if (inst->isLoad()) {
            auto loadinst = dynamic_cast<LoadInst *>(inst.get());
            auto indicenum = loadinst->getNumIndices();
            if (indicenum == 0) {
              continue;
            }
            auto pointer = loadinst->getPointer();

            auto globalpointer = dynamic_cast<GlobalValue *>(pointer);
            if (globalpointer != nullptr) {
              // parallelable = false;
              // toploop->setParallelableFalse();
              // break;
              globalloaddic[globalpointer].push_back(loadinst);
              continue;
            }
            auto localpointer = dynamic_cast<AllocaInst *>(pointer);
            localloaddic[localpointer].push_back(loadinst);
          } else if (inst->isStore()) {
            auto storeinst = dynamic_cast<StoreInst *>(inst.get());
            auto indicenum = storeinst->getNumIndices();
            if (indicenum == 0) {
              auto glb = dynamic_cast<GlobalValue *>(storeinst->getPointer());
              if (glb != nullptr) {
                toploop->setParallelableFalse();
                parallelable = false;
              }
              continue;
            }
            auto pointer = storeinst->getPointer();

            auto globalpointer = dynamic_cast<GlobalValue *>(pointer);
            if (globalpointer != nullptr) {
              // parallelable = false;
              // toploop->setParallelableFalse();
              // break;
              globalstoredic[globalpointer].push_back(storeinst);
            }
            auto localpointer = dynamic_cast<AllocaInst *>(pointer);
            localstoredic[localpointer].push_back(storeinst);
          } else if (inst->isCall() || inst->isGetSubArray()) {
            // auto callinst = dynamic_cast<CallInst *>(inst.get());
            // auto callee = callinst->getCallee();
            // TODO(ljx) 是否为纯函数
            toploop->setParallelableFalse();
            parallelable = false;
          }
        }
        if (!parallelable) {
          break;
        }
      }

      if (!parallelable) {
        continue;
      }
      if (localstoredic.empty() && globalstoredic.empty()) {
        toploop->setParallelableFalse();
        parallelable = false;
        continue;
      }

      for (auto &localload : localloaddic) {
        auto localvalue = localload.first;
        // 如果只有load, 说明对该数组只有读，没有写。
        if (localstoredic.find(localvalue) == localstoredic.end()) {
          continue;
        }
        // 如果还有store, 需要store a[i][j] 且 load 啊 a[i][j]，即，读写应一致
        auto indphi = toploop->getindPhi();

        // for (aut)

        // 找到store 中的第几个下标为i
        auto storeiter = localstoredic.find(localvalue);
        int phiindics = -1;
        for (auto storei : storeiter->second) {
          auto storeinst = dynamic_cast<StoreInst *>(storei);
          int index = 0;
          for (auto &indice : storeinst->getIndices()) {
            // if()
            auto loadindice = dynamic_cast<LoadInst *>(indice->getValue());
            if (loadindice == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }

            auto globalalloc = dynamic_cast<GlobalValue *>(loadindice->getPointer());
            if (globalalloc != nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }

            auto alloc = dynamic_cast<AllocaInst *>(loadindice->getPointer());
            if (alloc == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            if (alloc == indphi) {
              if (phiindics == -1) {
                phiindics = index;
              } else {
                if (phiindics != index) {
                  parallelable = false;
                  toploop->setParallelableFalse();
                }
              }
            }
            index++;
          }
          if (phiindics == -1 || !parallelable) {
            toploop->setParallelableFalse();
            parallelable = false;
            break;
          }
        }

        localstoredic.erase(storeiter);
        if (!parallelable) {
          break;
        }
        // 在loadinst中，判断是否还是这几个下标

        for (auto loadi : localload.second) {
          auto loadinst = dynamic_cast<LoadInst *>(loadi);
          int index = 0;
          for (auto &indice : loadinst->getIndices()) {
            auto loadindice = dynamic_cast<LoadInst *>(indice->getValue());
            if (loadindice == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            auto globalalloc = dynamic_cast<GlobalValue *>(loadindice->getPointer());
            if (globalalloc != nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            auto alloc = dynamic_cast<AllocaInst *>(loadindice->getPointer());
            if (alloc == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            if (alloc == indphi) {
              if (phiindics != index) {
                parallelable = false;
                toploop->setParallelableFalse();
              }
            }
            index++;
          }
        }
        if (!parallelable) {
          break;
        }
      }

      auto indphi = toploop->getindPhi();
      for (auto &storeload : localstoredic) {
        int phiindics = -1;
        int index = 0;
        for (auto storei : storeload.second) {
          auto storeinst = dynamic_cast<StoreInst *>(storei);
          int index = 0;
          for (auto &indice : storeinst->getIndices()) {
            // if()
            auto loadindice = dynamic_cast<LoadInst *>(indice->getValue());
            if (loadindice == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }

            auto globalalloc = dynamic_cast<GlobalValue *>(loadindice->getPointer());
            if (globalalloc != nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }

            auto alloc = dynamic_cast<AllocaInst *>(loadindice->getPointer());
            if (alloc == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            if (alloc == indphi) {
              if (phiindics == -1) {
                phiindics = index;
              } else {
                if (phiindics != index) {
                  parallelable = false;
                  toploop->setParallelableFalse();
                }
              }
            }
            index++;
          }
          if (phiindics == -1 || !parallelable) {
            toploop->setParallelableFalse();
            parallelable = false;
            break;
          }
        }

        // toploop->setParallelableFalse();
        // parallelable = false;
      }

      for (auto &globalload : globalloaddic) {
        auto globalvalue = globalload.first;
        // 如果只有load, 说明对该数组只有读，没有写。
        if (globalstoredic.find(globalvalue) == globalstoredic.end()) {
          continue;
        }
        // 如果还有store, 需要store a[i][j] 且 load 啊 a[i][j]，即，读写应一致
        auto indphi = toploop->getindPhi();

        // for (aut)

        // 找到store 中的第几个下标为i
        auto storeiter = globalstoredic.find(globalvalue);

        int phiindics = -1;
        for (auto storei : storeiter->second) {
          auto storeinst = dynamic_cast<StoreInst *>(storei);
          int index = 0;
          for (auto &indice : storeinst->getIndices()) {
            // if()
            auto loadindice = dynamic_cast<LoadInst *>(indice->getValue());
            if (loadindice == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }

            auto globalalloc = dynamic_cast<GlobalValue *>(loadindice->getPointer());
            if (globalalloc != nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }

            auto alloc = dynamic_cast<AllocaInst *>(loadindice->getPointer());
            if (alloc == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            if (alloc == indphi) {
              if (phiindics == -1) {
                phiindics = index;
              } else {
                if (phiindics != index) {
                  parallelable = false;
                  toploop->setParallelableFalse();
                }
              }
            }
            index++;
          }
          if (phiindics == -1 || !parallelable) {
            toploop->setParallelableFalse();
            parallelable = false;
            break;
          }
        }

        globalstoredic.erase(storeiter);
        if (!parallelable) {
          break;
        }
        // 在loadinst中，判断是否还是这几个下标

        for (auto loadi : globalload.second) {
          auto loadinst = dynamic_cast<LoadInst *>(loadi);
          int index = 0;
          for (auto &indice : loadinst->getIndices()) {
            auto loadindice = dynamic_cast<LoadInst *>(indice->getValue());
            if (loadindice == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            auto globalalloc = dynamic_cast<GlobalValue *>(loadindice->getPointer());
            if (globalalloc != nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            auto alloc = dynamic_cast<AllocaInst *>(loadindice->getPointer());
            if (alloc == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            if (alloc == indphi) {
              if (phiindics != index) {
                parallelable = false;
                toploop->setParallelableFalse();
              }
            }
            index++;
          }
        }
        if (!parallelable) {
          break;
        }
      }

      // auto indphi = toploop->getindPhi();
      // std:: cout <<

      for (auto &storeload : globalstoredic) {
        int phiindics = -1;
        int index = 0;
        for (auto storei : storeload.second) {
          auto storeinst = dynamic_cast<StoreInst *>(storei);
          int index = 0;
          for (auto &indice : storeinst->getIndices()) {
            // if()
            auto loadindice = dynamic_cast<LoadInst *>(indice->getValue());
            if (loadindice == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }

            auto globalalloc = dynamic_cast<GlobalValue *>(loadindice->getPointer());
            if (globalalloc != nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }

            auto alloc = dynamic_cast<AllocaInst *>(loadindice->getPointer());
            if (alloc == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            if (alloc == indphi) {
              if (phiindics == -1) {
                phiindics = index;
              } else {
                if (phiindics != index) {
                  parallelable = false;
                  toploop->setParallelableFalse();
                }
              }
            }
            index++;
          }
          if (phiindics == -1 || !parallelable) {
            toploop->setParallelableFalse();
            parallelable = false;
            break;
          }
        }

        // toploop->setParallelableFalse();
        // parallelable = false;
      }

      if (parallelable) {
        toploop->setParallelableTrue();
      }
      // for(auto &a)
    }

    for (auto &toploop : function->getLoops()) {
      bool parallelable = true;

      // 判断有无循环变量
      if (toploop->getindPhi() == nullptr) {
        toploop->setParallelableFalse();
        break;
      }
      auto headerblock = toploop->getHeader();
      int phiinstnum = 0;
      // 判断多少个phi
      for (auto &phiinst : headerblock->getInstructions()) {
        if (phiinst->isPhi()) {
          phiinstnum++;
          if (phiinstnum > 1) {
            toploop->setParallelableFalse();
            parallelable = false;
            break;
          }
        }
      }
      if (!parallelable) {
        continue;
      }
      std::unordered_map<AllocaInst *, std::list<Instruction *>> localloaddic;
      std::unordered_map<AllocaInst *, std::list<Instruction *>> localstoredic;
      std::unordered_map<GlobalValue *, std::list<Instruction *>> globalloaddic;
      std::unordered_map<GlobalValue *, std::list<Instruction *>> globalstoredic;
      for (auto basicblock : toploop->getBasicBlocks()) {
        for (auto &inst : basicblock->getInstructions()) {
          if (inst->isLoad()) {
            auto loadinst = dynamic_cast<LoadInst *>(inst.get());
            auto indicenum = loadinst->getNumIndices();
            if (indicenum == 0) {
              continue;
            }
            auto pointer = loadinst->getPointer();

            auto globalpointer = dynamic_cast<GlobalValue *>(pointer);
            if (globalpointer != nullptr) {
              // parallelable = false;
              // toploop->setParallelableFalse();
              // break;
              globalloaddic[globalpointer].push_back(loadinst);
              continue;
            }
            auto localpointer = dynamic_cast<AllocaInst *>(pointer);
            localloaddic[localpointer].push_back(loadinst);
          } else if (inst->isStore()) {
            auto storeinst = dynamic_cast<StoreInst *>(inst.get());
            auto indicenum = storeinst->getNumIndices();
            if (indicenum == 0) {
              auto glb = dynamic_cast<GlobalValue *>(storeinst->getPointer());
              if (glb != nullptr) {
                toploop->setParallelableFalse();
                parallelable = false;
              }
              continue;
            }
            auto pointer = storeinst->getPointer();

            auto globalpointer = dynamic_cast<GlobalValue *>(pointer);
            if (globalpointer != nullptr) {
              // parallelable = false;
              // toploop->setParallelableFalse();
              // break;
              globalstoredic[globalpointer].push_back(storeinst);
              continue;
            }
            auto localpointer = dynamic_cast<AllocaInst *>(pointer);
            localstoredic[localpointer].push_back(storeinst);
          } else if (inst->isCall() || inst->isGetSubArray()) {
            // auto callinst = dynamic_cast<CallInst *>(inst.get());
            // auto callee = callinst->getCallee();
            // TODO(ljx) 是否为纯函数
            toploop->setParallelableFalse();
            parallelable = false;
          }
        }
        if (!parallelable) {
          break;
        }
      }

      if (!parallelable) {
        continue;
      }
      if (localstoredic.empty() && globalstoredic.empty()) {
        toploop->setParallelableFalse();
        parallelable = false;
        continue;
      }
      for (auto &localload : localloaddic) {
        auto localvalue = localload.first;
        // 如果只有load, 说明对该数组只有读，没有写。
        if (localstoredic.find(localvalue) == localstoredic.end()) {
          continue;
        }
        // 如果还有store, 需要store a[i][j] 且 load 啊 a[i][j]，即，读写应一致
        auto indphi = toploop->getindPhi();

        // for (aut)

        // 找到store 中的第几个下标为i
        auto storeiter = localstoredic.find(localvalue);
        int phiindics = -1;
        for (auto storei : storeiter->second) {
          auto storeinst = dynamic_cast<StoreInst *>(storei);
          int index = 0;
          for (auto &indice : storeinst->getIndices()) {
            // if()
            auto loadindice = dynamic_cast<LoadInst *>(indice->getValue());
            if (loadindice == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }

            auto globalalloc = dynamic_cast<GlobalValue *>(loadindice->getPointer());
            if (globalalloc != nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }

            auto alloc = dynamic_cast<AllocaInst *>(loadindice->getPointer());
            if (alloc == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            if (alloc == indphi) {
              if (phiindics == -1) {
                phiindics = index;
              } else {
                if (phiindics != index) {
                  parallelable = false;
                  toploop->setParallelableFalse();
                }
              }
            }
            index++;
          }
          if (phiindics == -1 || !parallelable) {
            toploop->setParallelableFalse();
            parallelable = false;
            break;
          }
        }

        localstoredic.erase(storeiter);
        if (!parallelable) {
          break;
        }
        // 在loadinst中，判断是否还是这几个下标

        for (auto loadi : localload.second) {
          auto loadinst = dynamic_cast<LoadInst *>(loadi);
          int index = 0;
          for (auto &indice : loadinst->getIndices()) {
            auto loadindice = dynamic_cast<LoadInst *>(indice->getValue());
            if (loadindice == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            auto globalalloc = dynamic_cast<GlobalValue *>(loadindice->getPointer());
            if (globalalloc != nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            auto alloc = dynamic_cast<AllocaInst *>(loadindice->getPointer());
            if (alloc == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            if (alloc == indphi) {
              if (phiindics != index) {
                parallelable = false;
                toploop->setParallelableFalse();
              }
            }
            index++;
          }
        }
        if (!parallelable) {
          break;
        }
      }

      auto indphi = toploop->getindPhi();
      for (auto &storeload : localstoredic) {
        int phiindics = -1;
        int index = 0;
        for (auto storei : storeload.second) {
          auto storeinst = dynamic_cast<StoreInst *>(storei);
          int index = 0;
          for (auto &indice : storeinst->getIndices()) {
            // if()
            auto loadindice = dynamic_cast<LoadInst *>(indice->getValue());
            if (loadindice == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }

            auto globalalloc = dynamic_cast<GlobalValue *>(loadindice->getPointer());
            if (globalalloc != nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }

            auto alloc = dynamic_cast<AllocaInst *>(loadindice->getPointer());
            if (alloc == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            if (alloc == indphi) {
              if (phiindics == -1) {
                phiindics = index;
              } else {
                if (phiindics != index) {
                  parallelable = false;
                  toploop->setParallelableFalse();
                }
              }
            }
            index++;
          }
          if (phiindics == -1 || !parallelable) {
            toploop->setParallelableFalse();
            parallelable = false;
            break;
          }
        }

        // toploop->setParallelableFalse();
        // parallelable = false;
      }

      for (auto &globalload : globalloaddic) {
        auto globalvalue = globalload.first;
        // 如果只有load, 说明对该数组只有读，没有写。
        if (globalstoredic.find(globalvalue) == globalstoredic.end()) {
          continue;
        }
        // 如果还有store, 需要store a[i][j] 且 load 啊 a[i][j]，即，读写应一致
        auto indphi = toploop->getindPhi();

        // for (aut)

        // 找到store 中的第几个下标为i
        auto storeiter = globalstoredic.find(globalvalue);

        int phiindics = -1;
        for (auto storei : storeiter->second) {
          auto storeinst = dynamic_cast<StoreInst *>(storei);
          int index = 0;
          for (auto &indice : storeinst->getIndices()) {
            // if()
            auto loadindice = dynamic_cast<LoadInst *>(indice->getValue());
            if (loadindice == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }

            auto globalalloc = dynamic_cast<GlobalValue *>(loadindice->getPointer());
            if (globalalloc != nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }

            auto alloc = dynamic_cast<AllocaInst *>(loadindice->getPointer());
            if (alloc == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            if (alloc == indphi) {
              if (phiindics == -1) {
                phiindics = index;
              } else {
                if (phiindics != index) {
                  parallelable = false;
                  toploop->setParallelableFalse();
                }
              }
            }
            index++;
          }
          if (phiindics == -1 || !parallelable) {
            toploop->setParallelableFalse();
            parallelable = false;
            break;
          }
        }

        globalstoredic.erase(storeiter);
        if (!parallelable) {
          break;
        }
        // 在loadinst中，判断是否还是这几个下标

        for (auto loadi : globalload.second) {
          auto loadinst = dynamic_cast<LoadInst *>(loadi);
          int index = 0;
          for (auto &indice : loadinst->getIndices()) {
            auto loadindice = dynamic_cast<LoadInst *>(indice->getValue());
            if (loadindice == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            auto globalalloc = dynamic_cast<GlobalValue *>(loadindice->getPointer());
            if (globalalloc != nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            auto alloc = dynamic_cast<AllocaInst *>(loadindice->getPointer());
            if (alloc == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            if (alloc == indphi) {
              if (phiindics != index) {
                parallelable = false;
                toploop->setParallelableFalse();
              }
            }
            index++;
          }
        }
        if (!parallelable) {
          break;
        }
      }

      // auto indphi = toploop->getindPhi();
      // std:: cout <<

      for (auto &storeload : globalstoredic) {
        int phiindics = -1;
        int index = 0;
        for (auto storei : storeload.second) {
          auto storeinst = dynamic_cast<StoreInst *>(storei);
          int index = 0;
          for (auto &indice : storeinst->getIndices()) {
            // if()
            auto loadindice = dynamic_cast<LoadInst *>(indice->getValue());
            if (loadindice == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }

            auto globalalloc = dynamic_cast<GlobalValue *>(loadindice->getPointer());
            if (globalalloc != nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }

            auto alloc = dynamic_cast<AllocaInst *>(loadindice->getPointer());
            if (alloc == nullptr) {
              parallelable = false;
              toploop->setParallelableFalse();
              break;
            }
            if (alloc == indphi) {
              if (phiindics == -1) {
                phiindics = index;
              } else {
                if (phiindics != index) {
                  parallelable = false;
                  toploop->setParallelableFalse();
                }
              }
            }
            index++;
          }
          if (phiindics == -1 || !parallelable) {
            toploop->setParallelableFalse();
            parallelable = false;
            break;
          }
        }

        // toploop->setParallelableFalse();
        // parallelable = false;
      }

      if (parallelable) {
        toploop->setParallelableTrue();
      }
    }
  }
}

}  // namespace sysy