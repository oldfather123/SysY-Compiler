/**
 * @file InstructionSchedule.cpp
 *
 * @brief 定义指令调度的cpp文件
 *
 * 1. 指令调度采用表调度算法
 * 2. 寄存器分配后的调度，优先考虑节点的深度加延迟，其次是考虑当前路径上的总时间
 * 3. 寄存器分配前的调度，只考虑以减少生命周期为目标，减少寄存器溢出
 *
 */

#include "../include/backend/InstructionSchedule.h"
#include <algorithm>
#include <cassert>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include "../include/backend/Riscv.h"
#include "../include/backend/RiscvPrinter.h"

namespace riscv {
/**
 * @brief 总执行函数
 *
 * @param filename  打印出调度后的指令序列的文件名
 * @param pModule   模块指针
 * @param mode      调度模式，0是前调度，1是后调度
 * @param ifprint   是否打印出调度后的指令序列
 */
void InstructionSchedule::scheduleModule(std::string filename, Module *pModule, int mode,
                                         bool ifprint) {  //< 总的执行函数
  buildInitTable(pModule);
  buildDependGraph(pModule, mode);
  delUselessEdge();
  auto globalGraph = getDependGraph();
  for (const auto &func : pModule->getFunctions()) {
    for (const auto &block : func.second.get()->getBlocks()) {
      auto blockGraph = globalGraph.at(block.get());
      if (mode == 1) {
        scheduleBlock(block.get(), blockGraph);
      } else if (mode == 0) {
        scheduleBlockBefore(block.get(), blockGraph);
      }
    }
  }
  replaceInsts(pModule);
  if (ifprint) printNewModule(filename, pModule);
}
/**
 * @brief 寄存器分配前调度
 *
 * @param block     基本块指针
 * @param blockGraph 基本块对应的块内依赖图
 */
void InstructionSchedule::scheduleBlockBefore(
    BasicBlock *block, std::list<std::pair<Instruction *, std::shared_ptr<DependTreeGraph>>> blockGraph) {
  computePriority(block);  //< 计算块内节点优先级
  initRootNodes(block);    //< 初始化ready和active
  //< 开始调度,
  int cycle = 1;
  int pipe = 1;
  riscv::RiscvCodePrinter debug;
  while (!readyBefore.empty() || !newactive.empty()) {
    //< 当前周期，将active执行完的指令弹出
    finish.clear();
    while (!newactive.empty()) {
      auto node = newactive.top();
      blockScheduledInst.push_back(node->getCurInst());
      newactive.pop();
      node->setSchedule();
      for (const auto &child : node->getTrueChilds()) {  //先压有真依赖的子节点，就是生命周期离得近的
        if (child->ifinActive == false) {                //<对于还未调度且未准备执行的指令，
          //< 如果父节点全是已经调度的，则加入ready
          bool flag = true;
          for (const auto &parent : child->getParents()) {  // 对于一个节点，分析他的父节点是否都执行完成
            if (parent->getStatus() == false) {
              flag = false;
              break;
            }
          }
          if (flag && child->ifinActive == false) {
            newactive.push(child);
            child->ifinActive = true;
          }
        }
      }
    }
    updateReadyFront(blockGraph);
    //< 压入active新指令
    while (!readyBefore.empty()) {
      auto node = readyBefore.front();
      readyBefore.pop();
      newactive.push(node);
      // pipe--;
      node->setExec();
      node->setStartTime(cycle);
    }

    //< 执行周期，周期数+1
    cycle++;
  }
  //< 本基本块调度结束，记录调度后的指令顺序
  globalScheduleInstructions.emplace(std::make_pair(block, blockScheduledInst));
}
/**
 * @brief 寄存器分配后调度
 *
 * @param block 基本块
 * @param blockGraph 基本块图
 */
void InstructionSchedule::scheduleBlock(
    BasicBlock *block, std::list<std::pair<Instruction *, std::shared_ptr<DependTreeGraph>>> blockGraph) {
  computePriority(block);  //< 计算块内节点优先级
  initRootNodes(block);    //< 初始化ready和active
  //< 开始调度,
  int cycle = 1;
  int pipe = 1;
  while (!ready.empty() || !active.empty()) {
    //< 当前周期，将ready的指令调度到active中, 直接弹就是优先级最高的
    auto oldSize = active.size();
    if (!active.empty()) {
      for (auto iter = active.begin(); iter != active.end(); iter++) {
        auto iterTime = iter->get()->getEndTime();
        if (cycle > iterTime) {
          iter->get()->setSchedule();  //执行完成
          blockScheduledInst.push_back(iter->get()->getCurInst());
          iter = active.erase(iter);
        }
      }
    }
    // //< 根据更新的完成指令,只有active列表发生变化时,更新ready列表
    auto newSize = active.size();
    if (oldSize != newSize) updateReady(blockGraph);

    //< 压入新指令
    while (!ready.empty() && pipe > 0) {
      auto node = ready.top();
      ready.pop();
      active.push_back(node);

      node->setExec();
      // node->setSchedule();
      node->setStartTime(cycle);
    }

    //< 执行周期，周期数+1
    cycle++;
  }
  //< 本基本块调度结束，记录调度后的指令顺序
  globalScheduleInstructions.emplace(std::make_pair(block, blockScheduledInst));
}

/**
 * @brief 替换调度完的指令到Module中
 *
 * @param pModule 模块
 */
void InstructionSchedule::replaceInsts(Module *pModule) {
  // riscv::RiscvSymPrinter moduleprint;
  auto scheduledInst = getGloabalScheduledInsts();
  for (const auto &func : pModule->getFunctions()) {
    for (auto &block : func.second.get()->getBlocks()) {
      auto schedBlockInst = scheduledInst.at(block.get());
      auto &instructions = block.get()->getInstructions();
      std::unordered_map<Instruction *, std::unique_ptr<Instruction>> instMap;
      for (auto &instPtr : instructions) {
        instMap[instPtr.get()] = std::move(instPtr);  // 移动 unique_ptr 到映射
      }
      instructions.clear();  // 清空原始指令列表
      for (Instruction *schedInst : schedBlockInst) {
        auto it = instMap.find(schedInst);
        if (it != instMap.end()) {
          instructions.push_back(std::move(it->second));
        }
      }
    }
  }
}

/**
 * @brief 打印模块
 *
 * @param fileName 文件名
 * @param pModule 模块
 */
void InstructionSchedule::printNewModule(const std::string &fileName, Module *pModule) {
  auto scheduledInst = getGloabalScheduledInsts();
  std::streambuf *oldCoutBuf;
  std::ofstream fon;

  riscv::RiscvCodePrinter riscvPrint;

  if (!fileName.empty()) {
    oldCoutBuf = std::cout.rdbuf();
    fon.open(fileName);

    if (!fon.is_open()) {
      assert(false);
    }

    std::cout.rdbuf(fon.rdbuf());
  }

  std::cout << "  "
            << ".file " << '"' << fileName << '"' << std::endl;
  std::cout << "  "
            << ".option "
            << "pic" << std::endl;
  // RiscvSymPrinter::printGlobal(pModule);
  riscvPrint.printGlobal(pModule);
  std::cout << "  "
            << ".text" << std::endl;
  for (const auto &functionItem : pModule->getFunctions()) {
    if (!functionItem.second->isExternalFucntion()) {
      printNewFunc(functionItem.second.get());
    }
  }

  if (!fileName.empty()) {
    std::cout.rdbuf(oldCoutBuf);
    fon.close();
  }
}

/**
 * @brief 打印函数
 *
 * @param function 函数
 */
void InstructionSchedule::printNewFunc(Function *function) {
  auto instScheduledTable = getGloabalScheduledInsts();
  riscv::RiscvCodePrinter printer;
  std::cout << "  "
            << ".align 1" << std::endl;
  std::cout << "  "
            << ".global " << function->getName() << std::endl;
  std::cout << "  "
            << ".type " << function->getName() << ", "
            << "@function" << std::endl;
  std::cout << function->getName() << ": " << std::endl;
  for (const auto &block : function->getBlocks()) {
    if (!block->getName().empty()) {
      std::cout << block->getName() << ":" << std::endl;
    }
    auto insts = instScheduledTable.at(block.get());
    for (const auto &inst : insts) {
      std::cout << "  ";
      printer.printInst(inst);
    }
  }
  std::cout << "  "
            << ".size " << function->getName() << ", "
            << ".-" << function->getName() << std::endl;
}

/**
 * @brief 初始化全局变量
 *
 * @param pModule 模块
 */
void InstructionSchedule::buildInitTable(Module *pModule) {
  //< build Delay Table
  for (int kindInt = riscv::Instruction::InstKind::kSll; kindInt != riscv::Instruction::InstKind::kCall + 1;
       kindInt++) {
    auto kind = static_cast<riscv::Instruction::InstKind>(kindInt);
    int instDelay = getDelayByInst(kind);
    timeDelayTable.emplace(kind, instDelay);
  }
  //< build Inst Order Table，build initial DependTreeGraph
  // int idx = 0;
  for (const auto &func : pModule->getFunctions()) {
    for (const auto &block : func.second->getBlocks()) {
      //< 建立空的依赖图, 这里先建立了一个空指针的依赖图
      auto tmpGraph = buildBlockDependTree(block.get());
      globalDependGraph.emplace(block.get(), tmpGraph);
    }
  }
}

/**
 * @brief 构建依赖图
 *
 * @param pModule 模块
 * @param mode 模式
 *
 * @note 寄存器前调度，需要保持物理寄存器所在的指令不调度
 *
 * 满足三种依赖关系，写后读，写后写，以及读后写
 *
 * 地址依赖关系，需要保证地址指令对相同内存的存取不能调度，存后取，取后存，存后存
 *
 * 函数调用依赖关系，需要保证函数调用指令不能调度，调度前的传参指令不能调度
 */
void InstructionSchedule::buildDependGraph(Module *pModule, int mode) {
  for (const auto &graphPair : globalDependGraph) {
    auto tmpGraph = graphPair.second;  //< 当前块对应的依赖图

    for (auto iter1 = tmpGraph.begin(); iter1 != tmpGraph.end(); iter1++) {
      auto inst1 = iter1->first;
      auto order1 = iter1->second.get()->getIndex();
      auto desReg1 = inst1->getDestination();
      auto sourceRegs1 = inst1->getSources();
      //< mode 0
      if (mode == 0) {
        if (ifPhyReg(inst1)) {
          for (auto iter2 = std::next(iter1); iter2 != tmpGraph.end(); iter2++) {
            iter1->second.get()->addChild(iter2->second.get());
            iter2->second.get()->addParent(iter1->second.get());
          }
          continue;
        }
      }

      //< call指令以及跳转指令对之后的所有指令构成依赖
      if (inst1->isPcInst()) {
        for (auto iter2 = std::next(iter1); iter2 != tmpGraph.end(); iter2++) {
          iter1->second.get()->addChild(iter2->second.get());
          iter2->second.get()->addParent(iter1->second.get());
        }
        continue;
      }

      for (auto iter2 = std::next(iter1); iter2 != tmpGraph.end(); iter2++) {
        auto inst2 = iter2->first;
        // mode 0
        if (mode == 0) {
          if (ifPhyReg(inst2)) {
            iter1->second.get()->addChild(iter2->second.get());
            iter2->second.get()->addParent(iter1->second.get());
            continue;
          }
        }
        if (inst2->isPcInst()) {  // call指令与之前的所有指令构成依赖
          iter1->second.get()->addChild(iter2->second.get());
          iter2->second.get()->addParent(iter1->second.get());
          continue;
        }

        auto order2 = iter2->second.get()->getIndex();
        auto desReg2 = inst2->getDestination();
        auto sourceRegs2 = inst2->getSources();
        if (desReg1 == nullptr && desReg2 == nullptr) {
          if (inst1->isStoreInst() && inst2->isStoreInst()) {
            auto addr1 = getInstOpAddr(inst1);
            auto addr2 = getInstOpAddr(inst2);
            if (addr1.second == addr2.second) {
              iter1->second.get()->addChild(iter2->second.get());
              iter2->second.get()->addParent(iter1->second.get());
            }
          }
          continue;
        }
        auto rawIter = std::find(sourceRegs2.begin(), sourceRegs2.end(), desReg1);  //< 写后读  真依赖
        auto wraIter = std::find(sourceRegs1.begin(), sourceRegs1.end(), desReg2);  //< 读后写  反向依赖
        auto wrwIter = (desReg1 == desReg2);                                        //< 写后写  输出依赖
        if ((order1 < order2) && rawIter != sourceRegs2.end()) {                    //< RAW
          //< 添加依赖关系, A2B and B2A
          auto distance = iter2->second.get()->getIndex() - iter1->second.get()->getIndex();
          iter1->second.get()->addChild(iter2->second.get());
          iter2->second.get()->addParent(iter1->second.get());
          iter1->second.get()->addTrueChild(iter2->second.get());
          iter2->second.get()->addTrueParent(iter1->second.get());
          iter2->second.get()->setLifeDistance(std::max(distance, iter2->second.get()->getLifeDistance()));
        } else if ((order1 < order2) && wraIter != sourceRegs1.end()) {  //< WAR
          iter1->second.get()->addChild(iter2->second.get());
          iter2->second.get()->addParent(iter1->second.get());
        } else if ((order1 < order2) && wrwIter) {  //< WAW
          iter1->second.get()->addChild(iter2->second.get());
          iter2->second.get()->addParent(iter1->second.get());
        }
        //< 地址依赖，写在后面，保证不重复添加依赖关系 ,地址也有真依赖，lw和sw
        //< todo 先把ld 和 st之间都构成依赖
        else if (inst1->isAddrInst() && inst2->isAddrInst()) {
          if (mode == 1) {  //对于两个都是非栈指针直接依赖,有栈操作的就不用依赖，可以考虑真地址
            if (inst1->isStoreInst() && inst2->isLoadInst()) {  //< 寄存器分配后的模式中，地址依赖全部依赖上
              iter1->second.get()->addChild(iter2->second.get());
              iter2->second.get()->addParent(iter1->second.get());
            } else if (inst1->isStoreInst() && inst2->isStoreInst()) {
              iter1->second.get()->addChild(iter2->second.get());
              iter2->second.get()->addParent(iter1->second.get());
            } else if (inst1->isLoadInst() && inst2->isStoreInst()) {
              iter1->second.get()->addChild(iter2->second.get());
              iter2->second.get()->addParent(iter1->second.get());
            }
            continue;
          } else {  // 对于寄存器分配前的模式
            auto addr1 = getInstOpAddr(inst1);
            auto addr2 = getInstOpAddr(inst2);
            // todo 改变真假地址依赖的地方
            if (addr1.second == addr2.second) {
              if (inst1->isStoreInst() && inst2->isStoreInst()) {
                std::cout << "store store" << std::endl;
              }

              if (inst1->isStoreInst() && inst2->isLoadInst()) {  //< 写后读,加真依赖
                auto distance = iter2->second.get()->getIndex() - iter1->second.get()->getIndex();
                iter1->second.get()->addChild(iter2->second.get());
                iter2->second.get()->addParent(iter1->second.get());
                // 在都相等时加上真依赖
                if (addr1.first == addr2.first) {
                  iter1->second.get()->addTrueChild(iter2->second.get());
                  iter2->second.get()->addTrueParent(iter1->second.get());
                  iter2->second.get()->setLifeDistance(std::max(distance, iter2->second.get()->getLifeDistance()));
                }
              } else if (inst1->isStoreInst() && inst2->isStoreInst()) {
                std::cout << "store store" << std::endl;
                iter1->second.get()->addChild(iter2->second.get());
                iter2->second.get()->addParent(iter1->second.get());
              } else if (inst1->isLoadInst() && inst2->isStoreInst()) {
                // std::cout << "load store" << std::endl;
                iter1->second.get()->addChild(iter2->second.get());
                iter2->second.get()->addParent(iter1->second.get());
              }
            }
          }
        }
      }
    }
  }
}

/**
 * @brief 初始化根节点
 *
 * @param block 基本块
 */
void InstructionSchedule::initRootNodes(BasicBlock *block) {
  auto globalGraph = getDependGraph();
  auto tmpGraph = globalGraph.at(block);
  while (!ready.empty()) {
    ready.pop();
  }
  while (!readyBefore.empty()) {
    readyBefore.pop();
  }
  active.clear();
  blockScheduledInst.clear();
  regAllocaTable.clear();
  for (const auto &pair : tmpGraph) {
    if (pair.second.get()->isTop() && pair.second->ifinReady == false) {
      ready.push(pair.second);
      readyBefore.push(pair.second.get());
      pair.second->ifinReady = true;
      pair.second.get()->setExec();
    }
  }
  for (const auto &inst : block->getInstructions()) {
    // add 寄存器表
    if (inst->getKind() == Instruction::InstKind::kAdd) {
      auto tmpTuple = std::make_tuple<Register *, Register *, Instruction::InstKind>(
          inst->getSource(0), inst->getSource(1), Instruction::InstKind::kAdd);
      auto tmoPair = std::make_pair(inst->getDestination(), tmpTuple);
      regAllocaTable.emplace(tmoPair);
    }
  }
}

/**
 * @brief 构建基本块依赖图
 * @param block 基本块
 * @return std::list<std::pair<Instruction *, std::shared_ptr<DependTreeGraph>>>
 */
auto InstructionSchedule::buildBlockDependTree(BasicBlock *block) const
    -> std::list<std::pair<Instruction *, std::shared_ptr<DependTreeGraph>>> {
  std::list<std::pair<Instruction *, std::shared_ptr<DependTreeGraph>>> tmpDependGraph;
  int index = 0;
  for (const auto &inst : block->getInstructions()) {
    auto dependTreeNode = getNodeByInst(inst.get());
    dependTreeNode->setIndex(++index);
    tmpDependGraph.emplace_back(std::make_pair(inst.get(), dependTreeNode));
  }

  return tmpDependGraph;
}

/**
 * @brief 计算优先级
 * @param block 基本块
 */
void InstructionSchedule::computePriority(BasicBlock *block) {
  auto globalGraph = getDependGraph();
  auto tmpGraph = globalGraph.at(block);
  std::queue<DependTreeGraph *> nodeQueue;
  std::set<DependTreeGraph *> visited;

  while (!ready.empty()) {
    ready.pop();
  }
  active.clear();
  blockScheduledInst.clear();

  for (const auto &pair : tmpGraph) {
    auto node = pair.second.get();
    if (node->isTop()) {
      nodeQueue.push(node);
      node->setPriority(1);
      node->addAccumuDelay(node->getSelfDelay());
      visited.insert(node);
    }
  }
  int depth = 1;
  while (!nodeQueue.empty()) {
    auto length = nodeQueue.size();
    for (size_t i = 0; i < length; i++) {
      auto curnode = nodeQueue.front();
      nodeQueue.pop();
      auto curnodeDelay = curnode->getSelfDelay();
      // int priority = curnode->getSelfDelay() + curnode->getPriority();
      // @todo 这里的优先级设置需要考虑一下是否加入延迟
      int priority = depth - curnodeDelay;
      for (const auto &child : curnode->getChilds()) {
        if (visited.find(child) == visited.end()) {  //< 第一次访问的话就直接加入优先级
          child->setPriority(priority + 1);
          child->addAccumuDelay(curnodeDelay + child->getSelfDelay());
          nodeQueue.push(child);
          visited.insert(child);
        } else {  //< 重复访问的话就找对该节点来说最大的深度, 最小的总延迟
          child->setPriority(std::max(priority + 1, child->getPriority()));
          child->setAccumuDelay(std::min(child->getAccumuDelay(), curnodeDelay + child->getSelfDelay()));
        }
      }
    }
    depth++;  //< 遍历完当前层之后深度加一
  }
}

/**
 * @brief 比较两个寄存器是否是同一个计算方法
 *
 * @param reg1
 * @param reg2
 * @return true
 * @return false
 */
auto InstructionSchedule::compareTuple(Register *reg1, Register *reg2) const -> bool {
  bool result = false;
  auto tuple1 = regAllocaTable.find(reg1);
  auto tuple2 = regAllocaTable.find(reg2);
  if (tuple1 != regAllocaTable.end() && tuple2 != regAllocaTable.end()) {
    auto op1 = tuple1->second;
    auto op2 = tuple2->second;
    return op1 == op2;
  }
  return result;
}

/**
 * @brief 更新ready
 *
 * @param blockGraph
 */
void InstructionSchedule::updateReady(
    std::list<std::pair<Instruction *, std::shared_ptr<DependTreeGraph>>> blockGraph) {
  for (const auto &pair : blockGraph) {
    auto node = pair.second.get();
    if (node->getStatus() == false && node->getExec() == false) {  //<对于还未调度且未准备执行的指令，
      //< 如果父节点全是已经调度的，则加入ready
      bool flag = true;
      for (const auto &parent : pair.second.get()->getParents()) {  // 对于一个节点，分析他的父节点是否都执行完成
        if (parent->getStatus() == false) {
          flag = false;
          break;
        }
      }
      if (flag) {
        ready.push(pair.second);
      }
    }
  }
  //< 按priority大小对ready里的元素进行排序,升序
  //< @todo 这里排序的时候还有优先级相同的指令，需要深度排序, 依据总延迟？
  //< done 换了优先级队列的数据结构，不必手动排序了
  // ready.sort(compareNodes);
}

/**
 * @brief 更新readyFront
 *
 * @param blockGraph
 * @return void
 */
void InstructionSchedule::updateReadyFront(
    std::list<std::pair<Instruction *, std::shared_ptr<DependTreeGraph>>> blockGraph) {
  for (const auto &pair : blockGraph) {
    auto node = pair.second.get();
    if (node->getStatus() == false && node->getExec() == false) {  //<对于还未调度且未准备执行的指令，
      //< 如果父节点全是已经调度的，则加入ready
      bool flag = true;
      for (const auto &parent : pair.second.get()->getParents()) {  // 对于一个节点，分析他的父节点是否都执行完成
        if (parent->getStatus() == false) {
          flag = false;
          break;
        }
      }
      if (flag) {
        readyBefore.push(pair.second.get());
      }
    }
  }
}
/**
 * @brief 获取地址指令的寄存器和偏移量
 *
 * @param inst
 * @return std::pair<Register *, int>
 */
auto InstructionSchedule::getInstOpAddr(Instruction *inst) const -> std::pair<Register *, int> {
  auto Linst = dynamic_cast<IInst *>(inst);
  auto Sinst = dynamic_cast<SInst *>(inst);
  auto FLinst = dynamic_cast<FLInst *>(inst);
  auto FSinst = dynamic_cast<FSInst *>(inst);
  Register *reg;  // 使用智能指针来管理 Register 对象
  int offset;     // 使用智能指针来管理 int 对象
  if (Linst != nullptr) {
    reg = Linst->getSource(0);
    offset = Linst->getImm();
  } else if (Sinst != nullptr) {
    reg = Sinst->getSource(1);
    offset = Sinst->getImm();
  } else if (FLinst != nullptr) {
    reg = FLinst->getSource(0);
    offset = FLinst->getImm();
  } else if (FSinst != nullptr) {
    reg = FSinst->getSource(1);
    offset = FSinst->getImm();
  } else {
    assert(false);  // 如果以上条件都不满足，触发断言失败
  }

  auto result = std::make_pair(std::move(reg), std::move(offset));
  return result;
}

/**
 * @brief 判断指令是否是load指令
 *
 * @param inst
 * @return true
 * @return false
 */
auto InstructionSchedule::ifStackReg(Instruction *inst) -> bool {
  auto result = false;
  if (inst->isLoadInst()) {
    auto lInst = dynamic_cast<IInst *>(inst);
    if (lInst != nullptr) {
      auto sourreg = dynamic_cast<IntPhyRegister *>(lInst->getSource(0));
      if (sourreg) {
        if (sourreg->getIntPhyRegisterKind() == IntPhyRegister::fp) {
          result = true;
        }
      }
    }
  } else if (inst->isStoreInst()) {
    auto sInst = dynamic_cast<SInst *>(inst);
    if (sInst) {
      auto reg = dynamic_cast<IntPhyRegister *>(sInst->getSource(1));
      if (reg) {
        if (reg->getIntPhyRegisterKind() == IntPhyRegister::fp) {
          result = true;
        }
      }
    }
  } else {
    return result;
  }
  return result;
}

/**
 * @brief 判断指令是否是load指令
 *
 * @param inst
 * @return true
 * @return false
 */
auto InstructionSchedule::ifPhyReg(Instruction *inst) -> bool {
  auto rInst = dynamic_cast<RInst *>(inst);
  auto iInst = dynamic_cast<IInst *>(inst);
  auto sInst = dynamic_cast<SInst *>(inst);
  auto bInst = dynamic_cast<BInst *>(inst);
  auto jInst = dynamic_cast<JInst *>(inst);
  auto liInst = dynamic_cast<LiInst *>(inst);
  auto llaInst = dynamic_cast<LlaInst *>(inst);
  auto sextInst = dynamic_cast<Sext_wInst *>(inst);

  auto frInst = dynamic_cast<FRInst *>(inst);
  auto fr2iInst = dynamic_cast<FR2IInst *>(inst);
  auto f2iInst = dynamic_cast<F2IInst *>(inst);
  auto i2fInst = dynamic_cast<I2FInst *>(inst);
  auto flInst = dynamic_cast<FLInst *>(inst);
  auto fsInst = dynamic_cast<FSInst *>(inst);
  auto flongInst = dynamic_cast<FLongInst *>(inst);

  auto result = false;
  if (rInst) {
    auto desReg = dynamic_cast<IntPhyRegister *>(rInst->getDestination());
    auto &sources = rInst->getSources();
    if (desReg && desReg->getIntPhyRegisterKind() != IntPhyRegister::zero) {
      return true;
    }
    for (const auto &source : sources) {
      auto reg = dynamic_cast<IntPhyRegister *>(source);
      if (reg && reg->getIntPhyRegisterKind() != IntPhyRegister::zero) {
        return true;
      }
    }
  } else if (iInst) {
    auto desReg = dynamic_cast<IntPhyRegister *>(iInst->getDestination());
    auto source = iInst->getSource(0);
    if (desReg && desReg->getIntPhyRegisterKind() != IntPhyRegister::zero) {
      return true;
    }
    auto reg = dynamic_cast<IntPhyRegister *>(source);
    if (reg && reg->getIntPhyRegisterKind() != IntPhyRegister::zero) {
      return true;
    }
  } else if (sInst) {
    auto source = sInst->getSource(0);
    auto source2 = sInst->getSource(1);
    auto reg = dynamic_cast<IntPhyRegister *>(source);
    auto reg2 = dynamic_cast<IntPhyRegister *>(source2);
    if (reg && reg->getIntPhyRegisterKind() != IntPhyRegister::zero) {
      return true;
    }
    if (reg2 && reg2->getIntPhyRegisterKind() != IntPhyRegister::zero) {
      return true;
    }
  } else if (bInst) {
    auto source = bInst->getSource(0);
    auto source2 = bInst->getSource(1);
    auto reg = dynamic_cast<IntPhyRegister *>(source);
    auto reg2 = dynamic_cast<IntPhyRegister *>(source2);
    if (reg && reg->getIntPhyRegisterKind() != IntPhyRegister::zero) {
      return true;
    }
    if (reg2 && reg2->getIntPhyRegisterKind() != IntPhyRegister::zero) {
      return true;
    }
  } else if (jInst) {
    auto source = jInst->getDestination();
    auto reg = dynamic_cast<IntPhyRegister *>(source);
    if (reg && reg->getIntPhyRegisterKind() != IntPhyRegister::zero) {
      return true;
    }
  } else if (liInst) {  // add li Inst
    auto reg = liInst->getDestination();
    auto reg1 = dynamic_cast<IntPhyRegister *>(reg);
    if (reg1 && reg1->getIntPhyRegisterKind() != IntPhyRegister::zero) {
      return true;
    }
  } else if (llaInst) {
    auto desReg = llaInst->getDestination();
    auto desReg1 = dynamic_cast<IntPhyRegister *>(desReg);
    if (desReg1 && desReg1->getIntPhyRegisterKind() != IntPhyRegister::zero) {
      return true;
    }
  } else if (sextInst) {
    auto desreg = sextInst->getDestination();
    auto sourcereg = sextInst->getSource(0);
    auto desReg1 = dynamic_cast<IntPhyRegister *>(desreg);
    auto sourceReg1 = dynamic_cast<IntPhyRegister *>(sourcereg);
    if (desReg1 && desReg1->getIntPhyRegisterKind() != IntPhyRegister::zero) {
      return true;
    }
    if (sourceReg1 && sourceReg1->getIntPhyRegisterKind() != IntPhyRegister::zero) {
      return true;
    }
  } else if (frInst) {
    auto des = frInst->getDestination();
    auto src1 = frInst->getSource(0);
    auto src2 = frInst->getSource(1);
    auto desReg = dynamic_cast<FloatPhyRegister *>(des);
    auto srcReg1 = dynamic_cast<FloatPhyRegister *>(src1);
    auto srcReg2 = dynamic_cast<FloatPhyRegister *>(src2);
    if (desReg || srcReg1 || srcReg2) {
      return true;
    }
  } else if (fr2iInst) {
    auto des = fr2iInst->getDestination();  // int
    auto src = fr2iInst->getSource(0);      // float
    auto src2 = fr2iInst->getSource(1);     // float
    auto desReg = dynamic_cast<IntPhyRegister *>(des);
    auto srcReg = dynamic_cast<FloatPhyRegister *>(src);
    auto srcReg2 = dynamic_cast<FloatPhyRegister *>(src2);
    if (desReg || srcReg || srcReg2) {
      return true;
    }
  } else if (f2iInst) {
    auto des = f2iInst->getDestination();  // int
    auto src = f2iInst->getSource(0);      // float
    auto desReg = dynamic_cast<IntPhyRegister *>(des);
    auto srcReg = dynamic_cast<FloatPhyRegister *>(src);
    if (desReg || srcReg) {
      return true;
    }
  } else if (i2fInst) {
    auto des = i2fInst->getDestination();  // float
    auto src = i2fInst->getSource(0);      // int
    auto desReg = dynamic_cast<FloatPhyRegister *>(des);
    auto srcReg = dynamic_cast<IntPhyRegister *>(src);
    if (desReg || srcReg) {
      return true;
    }
  } else if (flInst) {
    auto des = flInst->getDestination();  // float
    auto src1 = flInst->getSource(0);     // int
    auto desReg = dynamic_cast<FloatPhyRegister *>(des);
    auto srcReg1 = dynamic_cast<IntPhyRegister *>(src1);
    if (desReg || srcReg1) {
      return true;
    }
  } else if (fsInst) {
    auto src1 = fsInst->getSource(0);  // float
    auto src2 = fsInst->getSource(1);  // float
    auto srcReg1 = dynamic_cast<FloatPhyRegister *>(src1);
    auto srcReg2 = dynamic_cast<FloatPhyRegister *>(src2);
    if (srcReg1 || srcReg2) {
      return true;
    }
  } else if (flongInst) {
    auto des = flongInst->getDestination();  // float
    auto src1 = flongInst->getSource(0);     // float
    auto src2 = flongInst->getSource(1);     // float
    auto src3 = flongInst->getSource(2);     // float
    auto desReg = dynamic_cast<FloatPhyRegister *>(des);
    auto srcReg1 = dynamic_cast<FloatPhyRegister *>(src1);
    auto srcReg2 = dynamic_cast<FloatPhyRegister *>(src2);
    auto srcReg3 = dynamic_cast<FloatPhyRegister *>(src3);
    if (desReg || srcReg1 || srcReg2 || srcReg3) {
      return true;
    }
  }
  return false;
}
/**
 * @brief get delay by inst kind
 *
 * @param kind
 * @return int
 */
auto InstructionSchedule::getDelayByInst(const Instruction::InstKind kind) const -> int {
  int delayResult = 0;
  switch (kind) {
    //< int instruction
    // shift
    case Instruction::InstKind::kSll:
      delayResult = 3;
      break;
    case Instruction::InstKind::kSlli:
      delayResult = 3;
      break;
    case Instruction::InstKind::kSrl:
      delayResult = 3;
      break;
    case Instruction::InstKind::kSrli:
      delayResult = 3;
      break;
    case Instruction::InstKind::kSra:
      delayResult = 3;
      break;
    case Instruction::InstKind::kSrai:
      delayResult = 3;
      break;
    case Instruction::InstKind::kSllw:
      delayResult = 3;
      break;
    case Instruction::InstKind::kSlliw:
      delayResult = 3;
      break;
    case Instruction::InstKind::kSrlw:
      delayResult = 3;
      break;
    case Instruction::InstKind::kSrliw:
      delayResult = 3;
      break;
    case Instruction::InstKind::kSraw:
      delayResult = 3;
      break;
    case Instruction::InstKind::kSraiw:
      delayResult = 3;
      break;
    //< arithmetic
    case Instruction::InstKind::kAdd:
      delayResult = 3;
      break;
    case Instruction::InstKind::kAddi:
      delayResult = 3;
      break;
    case Instruction::InstKind::kSub:
      delayResult = 3;
      break;
    case Instruction::InstKind::kAddw:
      delayResult = 3;
      break;
    case Instruction::InstKind::kAddiw:
      delayResult = 3;
      break;
    case Instruction::InstKind::kSubw:
      delayResult = 3;
      break;
    case Instruction::InstKind::kLui:
      delayResult = 3;
      break;
    case Instruction::InstKind::kAuipc:
      delayResult = 3;
      break;
    // logical
    case Instruction::InstKind::kXor:
      delayResult = 3;
      break;
    case Instruction::InstKind::kXori:
      delayResult = 3;
      break;
    case Instruction::InstKind::kOr:
      delayResult = 3;
      break;
    case Instruction::InstKind::kOri:
      delayResult = 3;
      break;
    case Instruction::InstKind::kAnd:
      delayResult = 3;
      break;
    case Instruction::InstKind::kAndi:
      delayResult = 3;
      break;
    // compare
    case Instruction::InstKind::kSlt:
      delayResult = 3;
      break;
    case Instruction::InstKind::kSlti:
      delayResult = 3;
      break;
    case Instruction::InstKind::kSltu:
      delayResult = 3;
      break;
    case Instruction::InstKind::kSltiu:
      delayResult = 3;
      break;
    // branches
    case Instruction::InstKind::kBeq:
      delayResult = 3;
      break;
    case Instruction::InstKind::kBne:
      delayResult = 3;
      break;
    case Instruction::InstKind::kBlt:
      delayResult = 3;
      break;
    case Instruction::InstKind::kBge:
      delayResult = 3;
      break;
    case Instruction::InstKind::kBgeu:
      delayResult = 3;
      break;
    // jump&links
    case Instruction::InstKind::kJal:
      delayResult = 3;
      break;
    case Instruction::InstKind::kJalr:
      delayResult = 3;
      break;
    // loads  hit
    case Instruction::InstKind::kLb:
      delayResult = 5;
      break;
    case Instruction::InstKind::kLh:
      delayResult = 5;
      break;
    case Instruction::InstKind::kLbu:
      delayResult = 5;
      break;
    case Instruction::InstKind::kLhu:
      delayResult = 5;
      break;
    case Instruction::InstKind::kLw:
      delayResult = 5;
      break;
    case Instruction::InstKind::kLwu:
      delayResult = 5;
      break;
    case Instruction::InstKind::kLd:
      delayResult = 5;
      break;
    case Instruction::InstKind::kSb:
      delayResult = 5;
      break;
    case Instruction::InstKind::kSh:
      delayResult = 5;
      break;
    case Instruction::InstKind::kSw:
      delayResult = 5;
      break;
    case Instruction::InstKind::kSd:
      delayResult = 5;
      break;
    // multiple
    case Instruction::InstKind::kMul:
      delayResult = 3;
      break;
    case Instruction::InstKind::kMulh:
      delayResult = 3;
      break;
    case Instruction::InstKind::kMulhu:
      delayResult = 3;
      break;
    case Instruction::InstKind::kMulhsu:
      delayResult = 3;
      break;
    case Instruction::InstKind::kMulw:
      delayResult = 3;
      break;
    // div rem
    case Instruction::InstKind::kDiv:
      delayResult = 34;
      break;
    case Instruction::InstKind::kDivu:
      delayResult = 34;
      break;
    case Instruction::InstKind::kDivw:
      delayResult = 34;
      break;
    case Instruction::InstKind::kRem:
      delayResult = 34;
      break;
    case Instruction::InstKind::kRemu:
      delayResult = 34;
      break;
    case Instruction::InstKind::kRemw:
      delayResult = 34;
      break;
    case Instruction::InstKind::kRemuw:
      delayResult = 34;
      break;

    //< float inst delay
    //< mv
    case Instruction::InstKind::kFmv_w_x:
      delayResult = 2;
      break;
    case Instruction::InstKind::kFmv_d_x:
      delayResult = 2;
      break;
    case Instruction::InstKind::kFmv_x_d:
      delayResult = 1;
      break;
    case Instruction::InstKind::kFmv_x_w:
      delayResult = 1;
      break;
    //< convert
    case Instruction::InstKind::kFcvt_s_w:
      delayResult = 2;
      break;
    case Instruction::InstKind::kFcvt_s_wu:
      delayResult = 2;
      break;
    case Instruction::InstKind::kFcvt_w_s:
      delayResult = 4;
      break;
    case Instruction::InstKind::kFcvt_wu_s:
      delayResult = 4;
      break;
    case Instruction::InstKind::kFcvt_s_l:
      delayResult = 4;
      break;
    case Instruction::InstKind::kFcvt_s_lu:
      delayResult = 4;
      break;
    case Instruction::InstKind::kFcvt_l_s:
      delayResult = 4;
      break;
    case Instruction::InstKind::kFcvt_lu_s:
      delayResult = 4;
      break;
    //< load store
    case Instruction::InstKind::kFlw:
      delayResult = 2;
      break;
    case Instruction::InstKind::kFsw:
      delayResult = 4;
      break;
    //< arithmetic
    case Instruction::InstKind::kFadd_s:
      delayResult = 5;
      break;
    case Instruction::InstKind::kFsub_s:
      delayResult = 5;
      break;
    case Instruction::InstKind::kFmul_s:
      delayResult = 5;
      break;
    case Instruction::InstKind::kFdiv_s:
      delayResult = 30;
      break;
    //< mul-add
    case Instruction::InstKind::kFmadd_s:
      delayResult = 5;
      break;
    case Instruction::InstKind::kFmsub_s:
      delayResult = 5;
      break;
    case Instruction::InstKind::kFnmadd_s:
      delayResult = 5;
      break;
    case Instruction::InstKind::kFnmsub_s:
      delayResult = 5;
      break;
    //< sign injection
    case Instruction::InstKind::kFsgnj_s:
      delayResult = 2;
      break;
    case Instruction::InstKind::kFsgnjn_s:
      delayResult = 2;
      break;
    case Instruction::InstKind::kFsgnjx_s:
      delayResult = 2;
      break;
    //< min max
    case Instruction::InstKind::kFmin_s:
      delayResult = 2;
      break;
    case Instruction::InstKind::kFmax_s:
      delayResult = 2;
      break;
    //< compare
    case Instruction::InstKind::kFeq_s:
      delayResult = 4;
      break;
    case Instruction::InstKind::kFlt_s:
      delayResult = 4;
      break;
    case Instruction::InstKind::kFle_s:
      delayResult = 4;
      break;
    case Instruction::InstKind::kFclass_s:
      delayResult = 4;
      break;
    //< 伪指令 伪指令的延迟是根据两条相加所得的
    case Instruction::InstKind::kLla:
      delayResult = 3;
      break;
    case Instruction::InstKind::kLi:
      delayResult = 2;
      break;
    case Instruction::InstKind::kSext_w:
      delayResult = 2;
      break;
    case Instruction::InstKind::kCall:
      delayResult = 6;
      break;

    default:
      delayResult = 2;
      break;
  }
  return delayResult;
}
auto InstructionSchedule::getNodeByInst(Instruction *inst) const -> std::shared_ptr<DependTreeGraph> {
  auto delay = getDelayByInst(inst->getKind());
  auto instTreeNode = std::make_shared<DependTreeGraph>(inst, delay);
  auto tmpInstNode = instTreeNode.get();
  tmpInstNode->init();
  return instTreeNode;
}

/**
 * @brief 删除无用边
 * @return void
 */
void InstructionSchedule::delUselessEdge() {
  int loopindex = 0;
  auto globalGraph = getDependGraph();
  for (const auto &blockGraph : globalGraph) {
    auto tmpGraph = blockGraph.second;
    for (auto &pair : tmpGraph) {
      auto childs = pair.second.get()->getChilds();
      std::unordered_set<DependTreeGraph *> delNode;
      std::unordered_set<DependTreeGraph *> visitedChilds(childs.begin(), childs.end());

      loopindex++;  //< @todo 插个标记
      if (loopindex > 10000) {
        break;
      }

      for (auto childIt = childs.begin(); childIt != childs.end(); ++childIt) {
        auto grandChilds = (*childIt)->getChilds();
        for (const auto &grand : grandChilds) {
          // 检查grand是否已经在childs中，如果是，则标记为删除
          if (visitedChilds.find(grand) != visitedChilds.end()) {
            delNode.insert(grand);
          }
        }
      }

      // 统一删除标记的节点
      for (const auto &delChild : delNode) {
        pair.second.get()->removeChild(delChild);
        delChild->removeParent(pair.second.get());
      }
    }
    if (loopindex > 10000) {
      break;
    }  //< 插个标记
  }
}

/**
 * @brief 清空全局依赖图
 * @note 清空全局依赖图，并清空时间延迟表
 */
void InstructionSchedule::clear() {
  globalDependGraph.clear();

  timeDelayTable.clear();
}
}  //  namespace riscv
