#include "../include/utils/SysyDebugger.h"
#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include "../include/backend/BackEndActiveAnalysis.h"
#include "../include/backend/InstructionSchedule.h"
#include "../include/backend/Riscv.h"
#include "../include/backend/RiscvPrinter.h"
#include "../include/frontend/IR.h"
#include "../include/frontend/SysYPrinter.h"
#include "../include/midend/ActiveVarAnalysis.h"
#include "../include/midend/DataFlowAnalysis.h"
#include "../include/midend/FuncAnalysis.h"

namespace sysy {

// auto
auto SysyDebugger::printLoopInfo(Module *module) -> void {
  std::cout << "----------- Loop Analysis ---------- " << std::endl;
  for (auto &function : module->getFunctions()) {
    auto &func = function.second;
    // for (auto &basicblock : func->getBasicBlocks()) {
    // }

    for (auto &loop : func->getTopLoops()) {
      std::cout << "LoopID:" << loop->getLoopID() << std::endl;
      std::cout << "     LoopHeader: " << loop->getHeader()->getName() << std::endl;
      if (loop->getParentLoop() != nullptr) {
        std::cout << "     LoopParent" << loop->getParentLoop()->getLoopID() << std::endl;
      } else {
        std::cout << "     No Parent" << std::endl;
      }
      std::cout << "     exiting block:" << std::endl;
      for (auto &block : loop->getexitingBlocks()) {
        std::cout << "     " << block->getName() << std::endl;
      }
      std::cout << "     exit block:" << std::endl;
      for (auto &block : loop->getexitBlocks()) {
        std::cout << "     " << block->getName() << std::endl;
      }
      std::cout << "     latch block:" << std::endl;
      for (auto &block : loop->getlatchBlock()) {
        std::cout << "     " << block->getName() << std::endl;
      }
      if (loop->getPreheaderBlock() != nullptr) {
        std::cout << "     pred headerblock" << loop->getPreheaderBlock()->getName() << std::endl;
      }
      std::cout << "     basicblock:" << std::endl;
      for (auto &block : loop->getBasicBlocks()) {
        std::cout << "     " << block->getName() << std::endl;
      }

      auto indphi = loop->getindPhi();
      if (indphi != nullptr) {
        std::cout << "     indphi : " << indphi->getName() << std::endl;
      } else {
        std::cout << "no phi " << std::endl;
      }
      auto indend = loop->getindEnd();
      if (indend != nullptr) {
        auto global_value = dynamic_cast<GlobalValue *>(indend);
        auto const_value = dynamic_cast<ConstantValue *>(indend);
        auto inst = dynamic_cast<Instruction *>(indend);
        if (const_value != nullptr) {
          if (const_value->isInt()) {
            std::cout << "     indend : " << const_value->getInt() << std::endl;
          } else {
            std::cout << "     indend : " << const_value->getFloat() << std::endl;
          }
        } else if (global_value != nullptr) {
          std::cout << "     indend : " << global_value->getName() << std::endl;
        } else if (inst != nullptr) {
          // auto inst = dynamic_cast<Instruction *>(indend);
          std::cout << "     indend : " << inst->getName() << std::endl;
        }
      }
      auto indbegin = loop->getindBegin();
      auto indStep = loop->getindStep();
      auto indStepType = loop->getStepType();

      if (indbegin != nullptr) {
        if (indbegin->isInt()) {
          std::cout << "     Phi begin : " << indbegin->getInt() << std::endl;
        }
      }
      if (indStep != nullptr) {
        if (indStep->isInt()) {
          std::cout << "     Phi Step" << indStep->getInt() << std::endl;
        }
      }
      if (indStepType != 0) {
        if (indStepType == 1) {
          std::cout << "     add " << std::endl;
        }
        if (indStepType == 2) {
          std::cout << "     sub " << std::endl;
        }
      }
      if (loop->getParallelable()) {
        std::cout << "     loop parallelable" << std::endl;
      } else {
        std::cout << "     loop unparallelable" << std::endl;
      }
    }
    for (auto &loop : func->getLoops()) {
      std::cout << "LoopID:" << loop->getLoopID() << std::endl;
      std::cout << "     LoopHeader: " << loop->getHeader()->getName() << std::endl;
      if (loop->getParentLoop() != nullptr) {
        std::cout << "     LoopParent" << loop->getParentLoop()->getLoopID() << std::endl;
      } else {
        std::cout << "     No Parent" << std::endl;
      }
      std::cout << "     exiting block:" << std::endl;
      for (auto &block : loop->getexitingBlocks()) {
        std::cout << "     " << block->getName() << std::endl;
      }
      std::cout << "     exit block:" << std::endl;
      for (auto &block : loop->getexitBlocks()) {
        std::cout << "     " << block->getName() << std::endl;
      }
      std::cout << "     latch block:" << std::endl;
      for (auto &block : loop->getlatchBlock()) {
        std::cout << "     " << block->getName() << std::endl;
      }
      if (loop->getPreheaderBlock() != nullptr) {
        std::cout << "     pred headerblock" << loop->getPreheaderBlock()->getName() << std::endl;
      }
      std::cout << "     basicblock:" << std::endl;
      for (auto &block : loop->getBasicBlocks()) {
        std::cout << "     " << block->getName() << std::endl;
      }
      auto indphi = loop->getindPhi();
      if (indphi != nullptr) {
        std::cout << "     indphi : " << indphi->getName() << std::endl;
      } else {
        std::cout << "no phi " << std::endl;
      }
      auto indend = loop->getindEnd();
      if (indend != nullptr) {
        auto const_value = dynamic_cast<ConstantValue *>(indend);
        auto inst = dynamic_cast<Instruction *>(indend);
        auto global_value = dynamic_cast<GlobalValue *>(indend);
        if (const_value != nullptr) {
          if (const_value->isInt()) {
            std::cout << "     indend : " << const_value->getInt() << std::endl;
          } else {
            std::cout << "     indend : " << const_value->getFloat() << std::endl;
          }
        } else if (global_value != nullptr) {
          std::cout << "     indend : " << global_value->getName() << std::endl;
        } else if (inst != nullptr) {
          // auto inst = dynamic_cast<Instruction *>(indend);
          std::cout << "     indend : " << inst->getName() << std::endl;
        }
      }
      auto indbegin = loop->getindBegin();
      auto indStep = loop->getindStep();
      auto indStepType = loop->getStepType();

      if (indbegin != nullptr) {
        if (indbegin->isInt()) {
          std::cout << "     Phi begin : " << indbegin->getInt() << std::endl;
        }
      }
      if (indStep != nullptr) {
        if (indStep->isInt()) {
          std::cout << "     Phi Step" << indStep->getInt() << std::endl;
        }
      }
      if (indStepType != 0) {
        if (indStepType == 1) {
          std::cout << "     add " << std::endl;
        }
        if (indStepType == 2) {
          std::cout << "     sub " << std::endl;
        }
      }
      if (loop->getParallelable()) {
        std::cout << "     loop parallelable" << std::endl;
      } else {
        std::cout << "     loop unparallelable" << std::endl;
      }
    }
  }
}

// 打印控制图前驱和后继
auto SysyDebugger::printCFG(Module *module) -> void {
  std::cout << "---------- CFG ----------" << std::endl;
  auto &functions = module->getFunctions();
  for (const auto &function : functions) {
    std::cout << function.first << ": " << std::endl;
    auto func = function.second.get();
    for (auto &basicblock : func->getBasicBlocks()) {
      std::cout << basicblock->getName() << ":";
      std::cout << "(predecessors:) ";
      for (auto pred : basicblock->getPredecessors()) {
        std::cout << pred->getName() << " ";
      }
      std::cout << "(successors:) ";
      for (auto succ : basicblock->getSuccessors()) {
        std::cout << succ->getName() << " ";
      }
      std::cout << std::endl;
    }
  }
  std::cout << "---------- End ----------" << std::endl;
  std::cout << std::endl;
}

// 打印从入口到达该块的必经结点集合、该块的支配边界、支配前驱和支配后继
auto SysyDebugger::printDomTree(Module *module) -> void {
  std::cout << "---------- DomTree ----------" << std::endl;
  auto &functions = module->getFunctions();
  for (const auto &function : functions) {
    std::cout << function.first << ": " << std::endl;
    auto func = function.second.get();
    for (auto &basicblock : func->getBasicBlocks()) {
      std::cout << basicblock->getName() << ":";
      std::cout << "(dominants:) ";
      for (auto dom : basicblock->getDominants()) {
        std::cout << dom->getName() << " ";
      }
      std::cout << "(dominant_frontiers:) ";
      for (auto domFrontier : basicblock->getDFs()) {
        std::cout << domFrontier->getName() << " ";
      }
      std::cout << "(idom:) ";
      auto idom = basicblock->getIdom();
      if (idom == nullptr) {
        std::cout << "nullptr ";
      } else {
        std::cout << idom->getName() << " ";
      }
      std::cout << "(successor doms:) ";
      for (auto sdom : basicblock->getSdoms()) {
        std::cout << sdom->getName() << " ";
      }
      std::cout << "(all dom children:) ";
      for (auto domChild : basicblock->getChildren()) {
        std::cout << domChild->getName() << " ";
      }
      std::cout << std::endl;
    }
  }
  std::cout << "---------- End ----------" << std::endl;
  std::cout << std::endl;
}

// 打印value2DefBlocks和value2UseBlocks
auto SysyDebugger::printValue2Blocks(Module *module) -> void {
  std::cout << "---------- Value2Blocks ---------- " << std::endl;
  std::cout << "format: || value: def/use block -- def/use times ||" << std::endl;
  auto &functions = module->getFunctions();
  for (const auto &function : functions) {
    std::cout << function.first << ": " << std::endl;
    auto func = function.second.get();
    std::cout << "value2AllocBlocks: " << std::endl;
    for (const auto &value2AllocBlock : func->getValue2AllocBlocks()) {
      auto elem = dynamic_cast<User *>(value2AllocBlock.first);
      if (elem != nullptr) {
        std::cout << elem->getName() << ": ";
        std::cout << value2AllocBlock.second->getName() << std::endl;
      }
    }
    std::cout << "value2DefBlocks: " << std::endl;
    for (const auto &value2DefBlock : func->getValue2DefBlocks()) {
      auto elem = dynamic_cast<User *>(value2DefBlock.first);
      if (elem != nullptr) {
        std::cout << elem->getName() << ": ";
        for (const auto &block_pair : value2DefBlock.second) {
          auto block = block_pair.first;
          std::cout << block->getName() << "--" << block_pair.second << " ";
        }
        std::cout << std::endl;
      }
    }
    std::cout << "value2UseBlocks: " << std::endl;
    for (const auto &value2UseBlock : func->getValue2UseBlocks()) {
      auto elem = dynamic_cast<User *>(value2UseBlock.first);
      if (elem != nullptr) {
        std::cout << elem->getName() << ": ";
        for (const auto &block_pair : value2UseBlock.second) {
          auto block = block_pair.first;
          std::cout << block->getName() << "--" << block_pair.second << " ";
        }
        std::cout << std::endl;
      }
    }
  }
  std::cout << "---------- End ----------" << std::endl;
  std::cout << std::endl;
}

// 打印指令的格式，用于理解和debug，便于知道需要该指令第几个操作数
// 注意这里没有考虑字面量和常数，它们没有打印出来，"^"隐式代表操作数序号
// 所以看有几个"^"就知道有几个操作数
// 同时打印指令被谁使用
auto SysyDebugger::printIR4Info(Module *module) -> void {
  std::cout << "---------- IR4Info(Before any operations) ----------" << std::endl;
  auto &functions = module->getFunctions();
  for (const auto &function : functions) {
    std::cout << function.first << ": " << std::endl;
    auto func = function.second.get();
    std::cout << "func: " << func->getName() << std::endl;
    auto basicBlocks = func->getBasicBlocks();
    for (auto &it : basicBlocks) {
      auto basicBlock = it.get();
      auto &instrs = basicBlock->getInstructions();
      for (auto &instr : instrs) {
        // 打印出来，用于理解和debug
        std::cout << "=================================================" << std::endl;
        printSingleIR(instr.get());
        printSingleIRUsedBy(instr.get());
      }
    }
    std::cout << "---------- Indirect Alloca: ----------" << std::endl;
    for (const auto &indirectAlloca : func->getIndirectAllocas()) {
      printSingleIR(indirectAlloca.get());
      printSingleIRUsedBy(indirectAlloca.get());
    }
    std::cout << std::endl;
  }
  std::cout << "---------- End ----------" << std::endl;
  std::cout << std::endl;
}

auto SysyDebugger::printSingleIR(Instruction *instr) -> void {
  auto operands = instr->getOperands();
  std::cout << instr->getKindString() << "..^" << instr->getName() << "..||..";
  for (const auto &operand : operands) {
    std::cout << "^" << getOpName(operand->getValue()) << "..";
  }
  std::cout << std::endl;
}

auto SysyDebugger::printSingleIRUsedBy(Instruction *instr) -> void {
  const auto &uses = instr->getUses();
  std::cout << "# used by " << uses.size() << " item(s): " << std::endl;
  for (const auto &use : uses) {
    auto iuser = dynamic_cast<Instruction *>(use->getUser());
    if (iuser != nullptr) {
      std::cout << "- ";
      printSingleIR(iuser);
    } else {
      std::cout << "- " << getOpName(use->getUser()) << std::endl;
    }
  }
}

auto SysyDebugger::getOpName(Value *operand) -> std::string {
  ConstantValue *constOperand = nullptr;
  GlobalValue *globalOperand = nullptr;
  ConstantVariable *constVarOperand = nullptr;
  AllocaInst *localOperand = nullptr;
  Instruction *tmpOperand = nullptr;
  BasicBlock *bbOperand = nullptr;
  Function *funcOperand = nullptr;

  constOperand = dynamic_cast<ConstantValue *>(operand);
  if (constOperand != nullptr) {
    if (constOperand->isFloat()) {
      return std::to_string(constOperand->getFloat());
    }
    return std::to_string(constOperand->getInt());
  }
  constVarOperand = dynamic_cast<ConstantVariable *>(operand);
  if (constVarOperand != nullptr) {
    return constVarOperand->getName();
  }
  globalOperand = dynamic_cast<GlobalValue *>(operand);
  if (globalOperand != nullptr) {
    return globalOperand->getName();
  }
  localOperand = dynamic_cast<AllocaInst *>(operand);
  if (localOperand != nullptr) {
    return localOperand->getName();
  }
  tmpOperand = dynamic_cast<Instruction *>(operand);
  if (tmpOperand != nullptr) {
    return tmpOperand->getName();
  }
  bbOperand = dynamic_cast<BasicBlock *>(operand);
  if (bbOperand != nullptr) {
    return bbOperand->getName();
  }
  funcOperand = dynamic_cast<Function *>(operand);
  if (funcOperand != nullptr) {
    return funcOperand->getName();
  }
  assert(false);
  return "error";
}

auto SysyDebugger::printExternalFunc(Module *module) -> void {
  std::cout << "---------- External Function ----------" << std::endl;
  for (const auto &externalFunc : module->getExternalFunctions()) {
    std::cout << externalFunc.first << std::endl;
  }
  std::cout << "---------- End ----------" << std::endl;
  std::cout << std::endl;
}

auto SysyDebugger::printRecursiveFunc(Module *module) -> void {
  std::cout << "---------- Recursive Function ----------" << std::endl;
  for (const auto &function : module->getFunctions()) {
    auto func = function.second.get();
    if ((func->getAttribute() & Function::FunctionAttribute::SelfRecursive) != 0U) {
      std::cout << func->getName() << std::endl;
    }
  }
  std::cout << "---------- End ----------" << std::endl;
  std::cout << std::endl;
}

auto SysyDebugger::printCallGraph(Module *module) -> void {
  std::cout << "---------- Call Graph ----------" << std::endl;
  auto &functions = module->getFunctions();
  for (const auto &function : functions) {
    const auto &[name, func] = function;
    std::cout << name << " -> ";
    for (const auto &callee : func->getCallees()) {
      std::cout << callee->getName() << " ";
    }
    std::cout << std::endl;
    // func被哪些函数调用
    std::cout << name << " <- ";
    std::cout << ": " << func->getUses().size() << " == ";
    for (const auto &use : func->getUses()) {
      auto user = use->getUser();
      auto userInst = dynamic_cast<Instruction *>(user);
      if (userInst != nullptr) {
        std::cout << userInst->getParent()->getParent()->getName() << " ";
      }
    }
    std::cout << std::endl;
  }
  std::cout << "---------- End ----------" << std::endl;
  std::cout << std::endl;
}

auto SysyDebugger::printEachFuncCodeSize(Module *module) -> void {
  std::cout << "---------- Each Function Code Size ----------" << std::endl;
  for (const auto &function : module->getFunctions()) {
    const auto &[name, func] = function;
    std::cout << name << ": " << sysy::FuncAnalysis::getFuncCodeSize(func.get()) << std::endl;
  }
  std::cout << "---------- End ----------" << std::endl;
  std::cout << std::endl;
}

auto SysyDebugger::printFuncAttributes(Module *module) -> void {
  std::cout << "---------- Function Attributes ----------" << std::endl;
  for (const auto &function : module->getFunctions()) {
    const auto &[name, func] = function;
    std::cout << name << ": ";
    if ((func->getAttribute() & Function::FunctionAttribute::SelfRecursive) != 0U) {
      std::cout << "SelfRecursive ";
    }
    if ((func->getAttribute() & Function::FunctionAttribute::SideEffect) != 0U) {
      std::cout << "SideEffect ";
    }
    if ((func->getAttribute() & Function::FunctionAttribute::NoPureCauseMemRead) != 0U) {
      std::cout << "NoPureCauseMemRead ";
    }
    if ((func->getAttribute() & Function::FunctionAttribute::Pure) != 0U) {
      std::cout << "Pure ";
    }
    std::cout << std::endl;
  }
  std::cout << "---------- End ----------" << std::endl;
  std::cout << std::endl;
}

auto SysyDebugger::printGlobalWirttenSet(Module *module) -> void {
  std::cout << "---------- Global Written Set ----------" << std::endl;
  for (const auto &function : module->getFunctions()) {
    const auto &[name, func] = function;
    std::cout << name << ": " << std::endl;
    auto globalWrittenSet = sysy::FuncAnalysis::globalWirttenSet(func.get());
    for (const auto &global : globalWrittenSet) {
      std::cout << global->getName() << " ";
    }
    std::cout << std::endl;
  }
  std::cout << "---------- End ----------" << std::endl;
  std::cout << std::endl;
}

auto SysyDebugger::printOriFuncIRInfoAndCLoneFuncIRInfo(Module *module) -> void {
  std::cout << "---------- Ori Func IR Info and Clone Func IR Info ----------" << std::endl;
  std::cout << "Ori Func IR Info: " << std::endl;
  printIR4Info(module);
  std::list<std::unique_ptr<Function>> record;
  for (const auto &function : module->getFunctions()) {
    const auto &[name, func] = function;
    auto funcClone = func.get()->clone();
    funcClone->setName(name + "_clone");
    record.emplace_back(funcClone);
  }
  std::cout << "Clone Func IR Info: " << std::endl;
  std::cout << "---------- IR4Info(Before any operations) ----------" << std::endl;
  for (const auto &function : record) {
    std::cout << "func: " << function->getName() << std::endl;
    auto basicBlocks = function->getBasicBlocks();
    for (auto &it : basicBlocks) {
      auto basicBlock = it.get();
      auto &instrs = basicBlock->getInstructions();
      for (auto &instr : instrs) {
        std::cout << "=================================================" << std::endl;
        printSingleIR(instr.get());
        printSingleIRUsedBy(instr.get());
      }
    }
    std::cout << "---------- Indirect Alloca: ----------" << std::endl;
    for (const auto &indirectAlloca : function->getIndirectAllocas()) {
      printSingleIR(indirectAlloca.get());
      printSingleIRUsedBy(indirectAlloca.get());
    }
    std::cout << std::endl;
  }
  std::cout << "---------- End ----------" << std::endl;
  std::cout << std::endl;
}

auto SysyDebugger::printDataFlow(DataFlowManager &dataFlowManager, Module *module) -> void {
  std::cout << "---------- DataFlowAnalysis Result ----------" << std::endl;
  // std::cout << "------ forwardAnalysis ------" << std::endl;
  // for (auto &fdataFlow : dataFlowManager.copyForwardDataFlowList()) {

  // }
  std::cout << "------ backwardAnalysis ------" << std::endl;
  for (auto &bdataFlow : dataFlowManager.copyBackwardDataFlowList()) {
    const auto specific = dynamic_cast<ActiveVarAnalysis *>(bdataFlow);
    if (specific != nullptr) {
      std::cout << "---- ActiveVarAnalysis ----" << std::endl;
      const auto &functions = module->getFunctions();
      for (const auto &function : functions) {
        std::cout << function.first << ": " << std::endl;
        const auto func = function.second.get();
        for (const auto &block : func->getBasicBlocks()) {
          std::cout << block->getName() << ": " << std::endl;
          const auto &instructions = block->getInstructions();
          const auto &activeTable = specific->getActiveTable();
          auto instructionIter = instructions.begin();
          for (unsigned i = 0; i < instructions.size(); i++) {
            const auto &activeSet = activeTable.at(block.get())[i];
            std::vector<std::string> activeNames;
            activeNames.reserve(activeSet.size());
            for (const auto &active : activeSet) {
              activeNames.emplace_back(active->getName());
            }
            std::sort(activeNames.begin(), activeNames.end());
            std::cout << "      ";
            for (const auto &name : activeNames) {
              std::cout << name << " ";
            }
            std::cout << std::endl;
            std::cout << "===========================================" << std::endl;
            std::cout << "    ";
            // 这里打印这条instruction
            SysYPrinter::printInst(instructionIter->get());
            std::cout << "===========================================" << std::endl;
            instructionIter++;
          }
          const auto &endActiveSet = activeTable.at(block.get())[instructions.size()];
          std::vector<std::string> endActiveNames;
          endActiveNames.reserve(endActiveSet.size());
          for (const auto &active : endActiveSet) {
            endActiveNames.emplace_back(active->getName());
          }
          std::sort(endActiveNames.begin(), endActiveNames.end());
          std::cout << "      ";
          for (const auto &name : endActiveNames) {
            std::cout << name << " ";
          }
          std::cout << std::endl;
        }
      }
    }
  }
  std::cout << "---------- End ----------" << std::endl;
  std::cout << std::endl;
}

auto SysyDebugger::printBackEndActiveAnalysis(riscv::ActiveVarAnalysis &activeAnalysis, riscv::Module *module) -> void {
  std::cout << "---------- DataFlowAnalysis Result ----------" << std::endl;
  // std::cout << "------ forwardAnalysis ------" << std::endl;
  // for (auto &fdataFlow : dataFlowManager.copyForwardDataFlowList()) {

  // }
  std::cout << "---- ActiveVarAnalysis ----" << std::endl;
  const auto &functions = module->getFunctions();
  for (const auto &function : functions) {
    if (function.second->isExternalFucntion()) {
      continue;
    }
    std::cout << function.first << ": " << std::endl;
    const auto func = function.second.get();
    for (const auto &block : func->getBlocks()) {
      std::cout << block->getName() << ": " << std::endl;
      const auto &instructions = block->getInstructions();
      const auto &activeTable = activeAnalysis.getActiveTable();
      auto instructionIter = instructions.begin();
      for (unsigned i = 0; i < instructions.size(); i++) {
        const auto &activeSet = activeTable.at(block.get())[i];
        std::vector<std::string> activeNames;
        activeNames.reserve(activeSet.size());
        for (const auto &active : activeSet) {
          activeNames.emplace_back(active->getName());
        }
        std::sort(activeNames.begin(), activeNames.end());
        std::cout << "      ";
        for (const auto &name : activeNames) {
          std::cout << name << " ";
        }
        std::cout << std::endl;
        std::cout << "===========================================" << std::endl;
        std::cout << "    ";
        // 这里打印这条instruction
        riscv::RiscvCodePrinter::printInst(instructionIter->get());
        std::cout << "===========================================" << std::endl;
        instructionIter++;
      }
      const auto &endActiveSet = activeTable.at(block.get())[instructions.size()];
      std::vector<std::string> endActiveNames;
      endActiveNames.reserve(endActiveSet.size());
      for (const auto &active : endActiveSet) {
        endActiveNames.emplace_back(active->getName());
      }
      std::sort(endActiveNames.begin(), endActiveNames.end());
      std::cout << "      ";
      for (const auto &name : endActiveNames) {
        std::cout << name << " ";
      }
      std::cout << std::endl;
    }
  }
}

auto SysyDebugger::printBackEndELSBasicBlocks(const riscv::ExtendedLinearScan &extendedLinearScan,
                                              riscv::Module *pModule) -> void {
  for (const auto &functionItem : pModule->getFunctions()) {
    if (!functionItem.second->isExternalFucntion()) {
      const auto &blockList = extendedLinearScan.getBlockList();
      std::cout << functionItem.first << ":" << std::endl;
      for (const auto &block : functionItem.second->getBlocks()) {
        auto begin = blockList.at(block.get());
        std::cout << block->getName() << ":" << std::endl;
        for (const auto &inst : block->getInstructions()) {
          std::cout << "  ";
          std::cout << begin << ":";
          riscv::RiscvCodePrinter::printInst(inst.get());
          begin += 1;
        }
      }
    }
  }
}

auto SysyDebugger::printBackEndELSMapResult(const riscv::ExtendedLinearScan &extendedLinearScan) -> void {
  for (const auto &symbolRegisterItem : extendedLinearScan.getAllocaResult()) {
    auto symbol = symbolRegisterItem.first;
    std::cout << symbol->getName() << ":";
    if (dynamic_cast<riscv::IntSymRegister *>(symbol) != nullptr) {
      std::cout << "int" << std::endl;
    } else {
      std::cout << "float" << std::endl;
    }
    for (const auto &interval : symbolRegisterItem.second) {
      if (extendedLinearScan.isSymbolSpilled(symbol)) {
        std::cout << "(" << interval->left / 2 << "," << interval->right / 2 << ")"
                  << ":"
                  << "spill" << std::endl;
      } else {
        if (interval->left != interval->right) {
          std::cout << "(" << interval->left / 2 << "," << interval->right / 2 << ")"
                    << ":" << riscv::RiscvCodePrinter::getOperandName(interval->physicRegister) << std::endl;
        } else {
          std::cout << "(" << interval->left / 2 << "," << interval->right / 2 << ")"
                    << ":" << riscv::RiscvCodePrinter::getOperandName(interval->physicRegister);

          if (interval->left % 2 == 0) {
            std::cout << " read" << std::endl;
          } else {
            std::cout << " write" << std::endl;
          }
        }
      }
    }
  }
}
//  指令调度打印
auto SysyDebugger::printBackEndDependGraph(const riscv::InstructionSchedule &instructionSchedule) -> void {
  auto dependGraph = instructionSchedule.getDependGraph();
  auto delayTable = instructionSchedule.getDelaytable();
  // auto instOrderTable = instructionSchedule.getInstOrderTable();
  riscv::RiscvCodePrinter debugPrinter;
  // print inst order

  for (const auto &pair : dependGraph) {
    auto block = pair.first;

    auto graph = pair.second;
    std::cout << std::endl << std::endl;
    std::cout << "Block: " << block->getName() << std::endl;
    for (const auto &iter : graph) {
      std::cout << "new_inst" << std::endl;
      std::cout << "inst Order: " << iter.second->getIndex() << std::endl;
      riscv::RiscvCodePrinter::printInst(iter.first);
      std::cout << "  Parent: " << std::endl;
      for (auto const &parent : iter.second->getParents()) {  //< 打印父节点成员
        std::cout << parent->getIndex() << "  ";
        riscv::RiscvCodePrinter::printInst(parent->getCurInst());
      }
      std::cout << "  Child: " << std::endl;
      for (auto const &child : iter.second->getChilds()) {  //< 打印子节点成员
        std::cout << child->getIndex() << "  ";
        riscv::RiscvCodePrinter::printInst(child->getCurInst());
      }
      std::cout << "True Child" << std::endl;
      for (const auto &child : iter.second->getTrueChilds()) {  //< 打印子节点成员
        std::cout << child->getIndex() << "  ";
        riscv::RiscvCodePrinter::printInst(child->getCurInst());
      }
    }
    std::cout << std::endl << "End The Block" << std::endl;
  }
  std::cout << std::endl << "End The Graph" << std::endl;
}

/// @brief 打印调度好的指令
/// @param instructionSchedule
/// @return
auto SysyDebugger::printScheduledInstructions(const riscv::InstructionSchedule &instructionSchedule) -> void {
  auto gloablScheduledInst = instructionSchedule.getGloabalScheduledInsts();
  std::cout << std::endl << std::endl << std::endl;
  std::cout << "开始调度后的指令" << std::endl;
  for (const auto &pair : gloablScheduledInst) {
    auto block = pair.first;
    auto ScheduledInst = pair.second;
    std::cout << std::endl << std::endl;
    std::cout << "Block: " << block->getName() << std::endl;
    riscv::RiscvCodePrinter debugPrint;
    for (const auto &newInst : ScheduledInst) {
      // std::cout << "1";
      riscv::RiscvCodePrinter::printInst(newInst);
    }
  }
}

auto SysyDebugger::printBackEndBasicblockInfo(riscv::Module *pModule) -> void {
  for (const auto &func : pModule->getFunctions()) {
    std::cout << "Function: " << func.first << std::endl;
    for (const auto &block : func.second->getBlocks()) {
      std::cout << "  block: " << block->getName() << std::endl;
    }
  }
}

}  // namespace sysy
