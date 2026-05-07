#include "../include/midend/Mem2Reg.h"
#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>
#include <queue>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include "../include/frontend/IR.h"
#include "../include/midend/ActiveVarAnalysis.h"
#include "../include/midend/DataFlowAnalysis.h"

/**
 * @file Mem2Reg.cpp
 *
 * 定义了mem2reg的源文件
 */
namespace sysy {
/**
 * @brief 计算给定变量的定义块集合的迭代支配边界
 *
 * @param [in] blocks 定义块集合
 * @return 返回该定义块集合的迭代支配边界集合
 */
auto Mem2Reg::computeIterDf(const std::unordered_set<BasicBlock *> &blocks) -> std::unordered_set<BasicBlock *> {
  std::unordered_set<BasicBlock *> workList;
  std::unordered_set<BasicBlock *> ret_list;
  workList.insert(blocks.begin(), blocks.end());

  while (!workList.empty()) {
    auto n = workList.begin();
    for (auto c : (*n)->getDFs()) {
      if (ret_list.count(c) == 0U) {
        ret_list.emplace(c);
        workList.emplace(c);
      }
    }
    workList.erase(n);
  }

  return ret_list;
}

/**
 * @brief 计算value2Blocks的映射，包括value2AllocBlocks、value2DefBlocks以及value2UseBlocks
 *
 * 其中value2DefBlocks可用于计算迭代支配边界来插入相应变量的phi结点；
 * 此处不考虑数组和全局变量，因为这些变量不会被mem2reg优化;
 *
 * @param [in] void
 * @return 无返回值，但将value2AllocBlocks、value2DefBlocks以及value2UseBlocks存储在函数成员中
 */
auto Mem2Reg::computeValue2Blocks() -> void {
  auto &functions = pModule->getFunctions();
  for (const auto &function : functions) {
    auto func = function.second.get();
    auto basicBlocks = func->getBasicBlocks();
    for (auto &it : basicBlocks) {
      auto basicBlock = it.get();
      auto &instrs = basicBlock->getInstructions();
      for (auto &instr : instrs) {
        // 如果指令本身就是alloca指令，则加到allocblocks中
        if (instr->isAlloca()) {
          if (!(isArr(instr.get()) || isGlobal(instr.get()))) {
            func->addValue2AllocBlocks(instr.get(), basicBlock);
          }
        } else if (instr->isStore()) {
          // 否则就看Store指令，找到operands里的alloc指令
          auto val = instr->getOperand(1);
          if (!(isArr(val) || isGlobal(val))) {
            func->addValue2DefBlocks(val, basicBlock);
          }
        } else if (instr->isLoad()) {
          // 如果是load指令，那么就是use，看operand(因为IR是reg-reg型，所以use只看load就行)
          auto val = instr->getOperand(0);
          if (!(isArr(val) || isGlobal(val))) {
            func->addValue2UseBlocks(val, basicBlock);
          }
        }
      }
    }
  }
}

/**
 * @brief 级联关系的顺带消除，用于llvm mem2reg类预优化1
 *
 * 采用队列进行模拟，从某种程度上来看其实可以看作是UD链的反向操作；
 *
 * @param [in] instr store指令使用的指令
 * @param [in] changed 不动点法的判断标准，地址传递
 * @param [in] func 指令所在函数
 * @param [in] block 指令所在基本块
 * @param [in] instrs 基本块所在指令集合，地址传递
 * @return 无返回值，但满足条件的情况下会对指令进行删除
 */
auto Mem2Reg::cascade(Instruction *instr, bool &changed, Function *func, BasicBlock *block,
                      std::list<std::unique_ptr<Instruction>> &instrs) -> void {
  if (instr != nullptr) {
    if (instr->isUnary() || instr->isBinary() || instr->isLoad()) {
      std::queue<Instruction *> toRemove;
      toRemove.push(instr);
      while (!toRemove.empty()) {
        auto top = toRemove.front();
        toRemove.pop();
        auto operands = top->getOperands();
        for (const auto &operand : operands) {
          auto elem = dynamic_cast<Instruction *>(operand->getValue());
          if (elem != nullptr) {
            if ((elem->isUnary() || elem->isBinary() || elem->isLoad()) && elem->getUses().size() == 1 &&
                elem->getUses().front()->getUser() == top) {
              toRemove.push(elem);
            } else if (elem->isAlloca()) {
              // value2UseBlock中该block对应次数-1，如果该变量的该useblock中count减为0了，则意味着
              // 该block其他地方也没用到该alloc了，故从value2UseBlock中删除
              auto res = func->removeValue2UseBlock(elem, block);
              // 只要有一次返回了true，就说明有变化
              if (res) {
                changed = true;
              }
            }
          }
        }
        auto tofind =
            std::find_if(instrs.begin(), instrs.end(), [&top](const auto &instr) { return instr.get() == top; });
        assert(tofind != instrs.end());
        usedelete(tofind->get());
        instrs.erase(tofind);
      }
    }
  }
}

/**
 * @brief llvm mem2reg预优化1: 删除不含load的alloc和store
 *
 * 1. 删除不含load的alloc和store；
 * 2. 删除store指令，之前的用于作store指令第0个操作数的那些级联指令就冗余了，也要删除；
 * 3. 删除之后，可能有些变量的load使用恰好又没有了，因此再次从第一步开始循环，这里使用不动点法
 *
 * @note 额外说明：由于删除了级联关系，所以这里的方法有点儿激进；
 * 同时也考虑了级联关系时如果调用了函数，可能会有side effect，所以没有删除调用函数的级联关系；
 * 而且关于函数参数的alloca不会在指令中删除，也不会在value2Alloca中删除;
 * 同样地，我们不考虑数组和global，不过这里的代码是基于value2blocks的，在value2blocks中已经考虑了，所以不用显式指明
 *
 * @param [in] void
 * @return 无返回值，但满足条件的情况下会对指令进行删除
 */
auto Mem2Reg::preOptimize1() -> void {
  auto &functions = pModule->getFunctions();
  for (const auto &function : functions) {
    auto func = function.second.get();
    auto &vToDefB = func->getValue2DefBlocks();
    auto &vToUseB = func->getValue2UseBlocks();
    // 先删除孤零零的alloca，即没有store的alloca
    auto &vToAllocB = func->getValue2AllocBlocks();
    for (auto iter = vToAllocB.begin(); iter != vToAllocB.end();) {
      auto val = iter->first;
      if (vToDefB.count(val) == 0U &&
          std::find(func->getEntryBlock()->getArguments().begin(), func->getEntryBlock()->getArguments().end(), val) ==
              func->getEntryBlock()->getArguments().end()) {
        auto bb = iter->second;
        auto tofind = std::find_if(bb->getInstructions().begin(), bb->getInstructions().end(),
                                   [val](const auto &instr) { return instr.get() == val; });
        usedelete(tofind->get());
        bb->getInstructions().erase(tofind);
        iter = vToAllocB.erase(iter);
      } else {
        ++iter;
      }
    }
    bool changed = true;
    // 不动点法
    while (changed) {
      changed = false;
      for (auto iter = vToDefB.begin(); iter != vToDefB.end();) {
        auto val = iter->first;
        // 找到没有load的变量，删除关于该变量的store
        if (vToUseB.count(val) == 0U) {
          auto blocks = func->getDefBlocksByValue(val);
          for (auto block : blocks) {
            auto &instrs = block->getInstructions();
            for (auto it = instrs.begin(); it != instrs.end();) {
              if (((*it)->isStore() && (*it)->getOperand(1) == val)) {
                // 关于该变量的store指令删除了，那对于之前的用于作store指令第0个操作数的那些指令就冗余了，也要删除
                // 只考虑为指令，不考虑字面量和常数，因为它们与之前的指令无关了
                // 同时考虑指令的话，该指令可能又与前面的指令可能有较强的级联关系，级联之间又有级联，因此又要考虑前面指令的删除
                auto valUsedByStore = dynamic_cast<Instruction *>((*it)->getOperand(0));
                usedelete(it->get());
                // if it is constantvalue which it not instruction
                if (valUsedByStore != nullptr && valUsedByStore->getUses().size() == 1 &&
                    valUsedByStore->getUses().front()->getUser() == (*it).get()) {
                  cascade(valUsedByStore, changed, func, block, instrs);
                }
                it = instrs.erase(it);
              } else {
                ++it;
              }
            }
          }
          // 再删除关于该变量的alloc（如果是函数参数的就不删除，函数参数的alloca在entry块的arguments内）
          if (std::find(func->getEntryBlock()->getArguments().begin(), func->getEntryBlock()->getArguments().end(),
                        val) == func->getEntryBlock()->getArguments().end()) {
            auto bb = func->getAllocBlockByValue(val);
            if (bb != nullptr) {
              func->removeValue2AllocBlock(val);
              auto tofind = std::find_if(bb->getInstructions().begin(), bb->getInstructions().end(),
                                         [val](const auto &instr) { return instr.get() == val; });
              usedelete(tofind->get());
              bb->getInstructions().erase(tofind);
            }
          }
          iter = vToDefB.erase(iter);
        } else {
          ++iter;
        }
      }
    }
  }
}

/**
 * @brief llvm mem2reg预优化2: 针对某个变量的Defblocks只有一个块的情况
 *
 * 1. 该基本块最后一次对该变量的store指令后的所有对该变量的load指令都可以替换为该基本块最后一次store指令的第0个操作数；
 * 2. 以该基本块为必经结点的结点集合中的对该变量的load指令都可以替换为该基本块最后一次对该变量的store指令的第0个操作数；
 * 3.
 * 如果对该变量的所有load均替换掉了，删除该基本块中最后一次store指令，如果这个store指令是唯一的define，那么再删除alloca指令（不删除参数的alloca）；
 * 4.
 * 如果对该value的所有load都替换掉了，对于该变量剩下还有store的话，就转换成了preOptimize1的情况，再调用preOptimize1进行删除；
 *
 * @note 额外说明：同样有点儿激进；
 * 同样不考虑数组和全局变量，因为这些变量不会被mem2reg优化，在value2blocks中已经考虑了，所以不用显式指明；
 * 替换的操作采用了UD链进行简化和效率的提升
 *
 * @param [in] void
 * @return 无返回值，但满足条件的情况下会对指令的操作数进行替换以及对指令进行删除
 */
auto Mem2Reg::preOptimize2() -> void {
  auto &functions = pModule->getFunctions();
  for (const auto &function : functions) {
    auto func = function.second.get();
    auto values = func->getValuesOfDefBlock();
    for (auto val : values) {
      auto blocks = func->getDefBlocksByValue(val);
      // 该val只有一个defining block
      if (blocks.size() == 1) {
        auto block = *blocks.begin();
        auto &instrs = block->getInstructions();
        auto rit = std::find_if(instrs.rbegin(), instrs.rend(),
                                [val](const auto &instr) { return instr->isStore() && instr->getOperand(1) == val; });
        // 注意reverse_iterator求base后是指向下一个指令，因此要减一才是原来的指令
        assert(rit != instrs.rend());
        auto it = --rit.base();
        auto propogationVal = (*it)->getOperand(0);
        // 其实该块中it后对该val的load指令也可以替换掉了
        for (auto curit = std::next(it); curit != instrs.end();) {
          if ((*curit)->isLoad() && (*curit)->getOperand(0) == val) {
            curit->get()->replaceAllUsesWith(propogationVal);
            usedelete(curit->get());
            curit = instrs.erase(curit);
            func->removeValue2UseBlock(val, block);
          } else {
            ++curit;
          }
        }
        // 在支配树后继结点中替换load指令的操作数
        for (auto child : block->getChildren()) {
          auto &childInstrs = child->getInstructions();
          for (auto childIter = childInstrs.begin(); childIter != childInstrs.end();) {
            if ((*childIter)->isLoad() && (*childIter)->getOperand(0) == val) {
              childIter->get()->replaceAllUsesWith(propogationVal);
              usedelete(childIter->get());
              childIter = childInstrs.erase(childIter);
              func->removeValue2UseBlock(val, child);
            } else {
              ++childIter;
            }
          }
        }
        // 如果对该val的所有load均替换掉了，那么对于该val的defining block中的最后一个define也可以删除了
        // 同时该块中前面对于该val的define也变成死代码了，可调用preOptimize1进行删除
        if (func->getUseBlocksByValue(val).empty()) {
          usedelete(it->get());
          instrs.erase(it);
          auto change = func->removeValue2DefBlock(val, block);
          if (change) {
            // 如果define是唯一的，且不是函数参数的alloca，直接删alloca
            if (std::find(func->getEntryBlock()->getArguments().begin(), func->getEntryBlock()->getArguments().end(),
                          val) == func->getEntryBlock()->getArguments().end()) {
              auto bb = func->getAllocBlockByValue(val);
              assert(bb != nullptr);
              auto tofind = std::find_if(bb->getInstructions().begin(), bb->getInstructions().end(),
                                         [val](const auto &instr) { return instr.get() == val; });
              usedelete(tofind->get());
              bb->getInstructions().erase(tofind);
              func->removeValue2AllocBlock(val);
            }
          } else {
            // 如果该变量还有其他的define，那么前面的define也变成死代码了
            assert(!func->getDefBlocksByValue(val).empty());
            assert(func->getUseBlocksByValue(val).empty());
            preOptimize1();
          }
        }
      }
    }
  }
}

/**
 * @brief llvm mem2reg类预优化3：针对某个变量的所有读写都在同一个块中的情况
 *
 * 1. 将每一个load替换成前一个store的值，并删除该load；
 * 2. 如果在load前没有对该变量的store，则不删除该load；
 * 3. 如果一个store后没有任何对改变量的load，则删除该store；
 *
 * @note 额外说明：第二点不用显式处理，因为我们的方法是从找到第一个store开始；
 * 第三点其实可以更激进一步地理解，即每次替换了load之后，它对应地那个store也可以删除了，同时注意这里不要使用preoptimize1进行处理，因为他们的级联关系是有用的：即用来求load的替换值；
 * 同样地，我们这里不考虑数组和全局变量，因为这些变量不会被mem2reg优化，不过这里在计算value2DefBlocks时已经跳过了，所以不需要再显式处理了；
 * 替换的操作采用了UD链进行简化和效率的提升
 *
 * @param [in] void
 * @return 无返回值，但满足条件的情况下会对指令的操作数进行替换以及对指令进行删除
 */
auto Mem2Reg::preOptimize3() -> void {
  auto &functions = pModule->getFunctions();
  for (const auto &function : functions) {
    auto func = function.second.get();
    auto values = func->getValuesOfDefBlock();
    for (auto val : values) {
      auto sblocks = func->getDefBlocksByValue(val);
      auto lblocks = func->getUseBlocksByValue(val);
      if (sblocks.size() == 1 && lblocks.size() == 1 && *sblocks.begin() == *lblocks.begin()) {
        auto block = *sblocks.begin();
        auto &instrs = block->getInstructions();
        auto it = std::find_if(instrs.begin(), instrs.end(),
                               [val](const auto &instr) { return instr->isStore() && instr->getOperand(1) == val; });
        while (it != instrs.end()) {
          auto propogationVal = (*it)->getOperand(0);
          auto last = std::find_if(std::next(it), instrs.end(), [val](const auto &instr) {
            return instr->isStore() && instr->getOperand(1) == val;
          });
          for (auto curit = std::next(it); curit != last;) {
            if ((*curit)->isLoad() && (*curit)->getOperand(0) == val) {
              curit->get()->replaceAllUsesWith(propogationVal);
              usedelete(curit->get());
              curit = instrs.erase(curit);
              func->removeValue2UseBlock(val, block);
            } else {
              ++curit;
            }
          }
          // 替换了load之后，它对应地那个store也可以删除了
          if (!(std::find_if(func->getEntryBlock()->getArguments().begin(), func->getEntryBlock()->getArguments().end(),
                             [val](const auto &instr) { return instr == val; }) !=
                func->getEntryBlock()->getArguments().end()) &&
              last == instrs.end()) {
            usedelete(it->get());
            it = instrs.erase(it);
            if (func->removeValue2DefBlock(val, block)) {
              auto bb = func->getAllocBlockByValue(val);
              if (bb != nullptr) {
                auto tofind = std::find_if(bb->getInstructions().begin(), bb->getInstructions().end(),
                                           [val](const auto &instr) { return instr.get() == val; });
                usedelete(tofind->get());
                bb->getInstructions().erase(tofind);
                func->removeValue2AllocBlock(val);
              }
            }
          }
          it = last;
        }
      }
    }
  }
}

/**
 * @brief 为所有变量的定义块集合的迭代支配边界插入phi结点（剪枝版）
 *
 * insertPhi是mem2reg的核心之一，这里是对所有变量的迭代支配边界的phi结点插入，无参数也无返回值；
 * 同样跳过对数组和全局变量的处理，因为这些变量不会被mem2reg优化，刚好这里在计算value2DefBlocks时已经跳过了，所以不需要再显式处理了；
 * 同时我们进行了剪枝处理，只有在基本块入口活跃的变量，才插入phi函数
 *
 * @param [in] void
 * @return 无返回值，但是会在每个变量的迭代支配边界上插入phi结点
 */
auto Mem2Reg::insertPhi() -> void {
  sysy::DataFlowManager dataFlowManager;
  sysy::ActiveVarAnalysis activeVarAnalysis;
  dataFlowManager.addBackwardDataFlow(&activeVarAnalysis);
  dataFlowManager.backwardAnalyze(pModule);

  auto &functions = pModule->getFunctions();
  for (const auto &function : functions) {
    auto func = function.second.get();
    const auto &vToDefB = func->getValue2DefBlocks();
    for (const auto &map_pair : vToDefB) {
      // 首先为每个变量找到迭代支配边界
      auto val = map_pair.first;
      auto blocks = func->getDefBlocksByValue(val);
      auto itDFs = computeIterDf(blocks);
      // 然后在每个变量相应的迭代支配边界上插入phi结点
      for (auto basicBlock : itDFs) {
        const auto &actiTable = activeVarAnalysis.getActiveTable();
        auto dval = dynamic_cast<User *>(val);
        // 只有在基本块入口活跃的变量，才插入phi函数
        if (actiTable.at(basicBlock).front().count(dval) != 0U) {
          pBuilder->createPhiInst(val->getType(), val, basicBlock);
        }
      }
    }
  }
}

/**
 * @brief 重命名
 *
 * 重命名是mem2reg的核心之二，这里是对单个块的重命名，递归实现
 * 同样跳过对数组和全局变量的处理，因为这些变量不会被mem2reg优化
 *
 * @param [in] block 一个基本块
 * @param [in] count 计数器，用于给变量重命名，地址传递
 * @param [in] stacks 用于存储变量的栈，地址传递
 * @return 无返回值
 */
auto Mem2Reg::rename(BasicBlock *block, std::unordered_map<Value *, int> &count,
                     std::unordered_map<Value *, std::stack<Instruction *>> &stacks) -> void {
  auto &instrs = block->getInstructions();
  std::unordered_map<Value *, int> valPop;
  // 第一大步：对块中的所有指令遍历处理
  for (auto iter = instrs.begin(); iter != instrs.end();) {
    auto instr = iter->get();
    // 对于load指令，变量用最新的那个
    if (instr->isLoad()) {
      auto val = instr->getOperand(0);
      if (!(isArr(val) || isGlobal(val))) {
        if (!stacks[val].empty()) {
          instr->replaceOperand(0, stacks[val].top());
        }
      }
    }
    // 然后对于define的情况，看alloca、store和phi指令
    if (instr->isDefine()) {
      if (instr->isAlloca()) {
        // alloca指令名字不改了，命名就按x，x_1，x_2...来就行
        auto val = instr;
        if (!(isArr(val) || isGlobal(val))) {
          ++valPop[val];
          stacks[val].push(val);
          ++count[val];
        }
      } else if (instr->isPhi()) {
        // Phi指令也是一条特殊的define指令
        auto val = dynamic_cast<PhiInst *>(instr)->getMapVal();
        if (!(isArr(val) || isGlobal(val))) {
          auto i = count[val];
          if (i == 0) {
            // 对还未alloca就有phi的指令的处理，直接删除
            usedelete(iter->get());
            iter = instrs.erase(iter);
            continue;
          }
          auto newname = dynamic_cast<Instruction *>(val)->getName() + "_" + std::to_string(i);
          auto newalloca = pBuilder->createAllocaInstWithoutInsert(val->getType(), {}, block, newname);
          block->getParent()->addIndirectAlloca(newalloca);
          instr->replaceOperand(0, newalloca);
          ++valPop[val];
          stacks[val].push(newalloca);
          ++count[val];
        }
      } else {
        // store指令看operand的名字，我们的实现是规定变量在operand的第二位，用一个新的alloca x_i代替
        auto val = instr->getOperand(1);
        if (!(isArr(val) || isGlobal(val))) {
          auto i = count[val];
          auto newname = dynamic_cast<Instruction *>(val)->getName() + "_" + std::to_string(i);
          auto newalloca = pBuilder->createAllocaInstWithoutInsert(val->getType(), {}, block, newname);
          block->getParent()->addIndirectAlloca(newalloca);
          instr->replaceOperand(1, newalloca);
          ++valPop[val];
          stacks[val].push(newalloca);
          ++count[val];
        }
      }
    }
    ++iter;
  }
  // 第二大步：把所有CFG中的该块的successor的phi指令的相应operand确定
  for (auto succ : block->getSuccessors()) {
    auto position = getPredIndex(block, succ);
    for (auto &instr : succ->getInstructions()) {
      if (instr->isPhi()) {
        auto val = dynamic_cast<PhiInst *>(instr.get())->getMapVal();
        if (!stacks[val].empty()) {
          instr->replaceOperand(position + 1, stacks[val].top());
        }
      } else {
        // phi指令是添加在块的最前面的，因此过了之后就不会有phi了，直接break
        break;
      }
    }
  }
  // 第三大步：递归支配树的后继，支配树才能表示define-use关系
  for (auto sdom : block->getSdoms()) {
    rename(sdom, count, stacks);
  }
  // 第四大步：遍历块中的所有指令，如果涉及到define，就弹栈，这一步是必要的，可以从递归的整体性来思考原因
  // 注意这里count没清理，因为平级之间计数仍然是一直增加的，但是stack要清理，因为define-use关系来自直接
  // 支配结点而不是平级之间，不清理栈会被污染
  // 提前优化：知道变量对应的要弹栈的次数就可以了，没必要遍历所有instr.
  for (auto val_pair : valPop) {
    auto val = val_pair.first;
    for (int i = 0; i < val_pair.second; ++i) {
      stacks[val].pop();
    }
  }
}

/**
 * @brief 重命名所有块
 *
 * 调用rename，自上而下实现所有rename
 *
 * @param [in] void
 * @return 无返回值
 */
auto Mem2Reg::renameAll() -> void {
  auto &functions = pModule->getFunctions();
  for (const auto &function : functions) {
    auto func = function.second.get();
    // 对于每个function都要SSA化，所以count和stacks定义在这并初始化
    std::unordered_map<Value *, int> count;
    std::unordered_map<Value *, std::stack<Instruction *>> stacks;
    for (const auto &map_pair : func->getValue2DefBlocks()) {
      auto val = map_pair.first;
      count[val] = 0;
    }
    rename(func->getEntryBlock(), count, stacks);
  }
}

/**
 * @brief mem2reg，对外的接口
 *
 * 静态单一赋值 + mem2reg等pass的逻辑组合
 *
 * @param [in] void
 * @return 无返回值
 */
auto Mem2Reg::run() -> void {
  // 计算所有valueToBlocks的定义映射
  computeValue2Blocks();
  // 参考llvm的mem2reg遍，在插入phi结点之前，先做些优化
  preOptimize1();
  preOptimize2();
  preOptimize3();
  // 为所有变量插入phi结点
  insertPhi();
  // 重命名
  renameAll();
}

/**
 * @brief 计算块n是块s的第几个前驱
 *
 * helperfunction，没有返回值，但是会将dom和other的交集赋值给dom
 *
 * @param [in] n 基本块，n是s的前驱之一
 * @param [in] s 基本块，s是n的后继之一
 * @return 返回n是s的第几个前驱
 */
auto Mem2Reg::getPredIndex(BasicBlock *n, BasicBlock *s) -> int {
  int index = 0;
  for (auto elem : s->getPredecessors()) {
    if (elem == n) {
      break;
    }
    ++index;
  }
  assert(index < static_cast<int>(s->getPredecessors().size()) && "n is not a predecessor of s.");
  return index;
}

/**
 * @brief 判断一个value是不是全局变量
 *
 * @param [in] val 一个value
 * @return 返回true表示是全局变量，返回false表示不是
 */
auto Mem2Reg::isGlobal(Value *val) -> bool {
  auto gval = dynamic_cast<GlobalValue *>(val);
  return gval != nullptr;
}

/**
 * @brief 判断一个value是不是数组
 *
 * @param [in] val 一个value
 * @return 返回true表示是数组，返回false表示不是
 */
auto Mem2Reg::isArr(Value *val) -> bool {
  auto aval = dynamic_cast<AllocaInst *>(val);
  return aval != nullptr && aval->getNumDims() != 0;
}

/**
 * @brief 删除一个指令的operand对应的value的该条use
 *
 * @param [in] inst 一条指令
 * @return 无返回值
 */
auto Mem2Reg::usedelete(Instruction *instr) -> void {
  for (auto &use : instr->getOperands()) {
    auto val = use->getValue();
    val->removeUse(use);
  }
}
}  // namespace sysy
