#include "SideEffectAnalysis.h"
#include "AliasAnalysis.h"
#include "CallGraphAnalysis.h"
#include "SysYIRPrinter.h"
#include <iostream>

namespace sysy {

// 副作用分析遍的静态 ID
void *SysYSideEffectAnalysisPass::ID = (void *)&SysYSideEffectAnalysisPass::ID;

// ======================================================================
// SideEffectAnalysisResult 类的实现
// ======================================================================

SideEffectAnalysisResult::SideEffectAnalysisResult() { initializeKnownFunctions(); }

const SideEffectInfo &SideEffectAnalysisResult::getInstructionSideEffect(Instruction *inst) const {
  auto it = instructionSideEffects.find(inst);
  if (it != instructionSideEffects.end()) {
    return it->second;
  }
  // 返回默认的无副作用信息
  static SideEffectInfo noEffect;
  return noEffect;
}

const SideEffectInfo &SideEffectAnalysisResult::getFunctionSideEffect(Function *func) const {
  // 首先检查分析过的用户定义函数
  auto it = functionSideEffects.find(func);
  if (it != functionSideEffects.end()) {
    return it->second;
  }
  
  // 如果没有找到，检查是否为已知的库函数
  if (func) {
    std::string funcName = func->getName();
    const SideEffectInfo *knownInfo = getKnownFunctionSideEffect(funcName);
    if (knownInfo) {
      return *knownInfo;
    }
  }
  
  // 返回默认的无副作用信息
  static SideEffectInfo noEffect;
  return noEffect;
}

void SideEffectAnalysisResult::setInstructionSideEffect(Instruction *inst, const SideEffectInfo &info) {
  instructionSideEffects[inst] = info;
}

void SideEffectAnalysisResult::setFunctionSideEffect(Function *func, const SideEffectInfo &info) {
  functionSideEffects[func] = info;
}

bool SideEffectAnalysisResult::hasSideEffect(Instruction *inst) const {
  const auto &info = getInstructionSideEffect(inst);
  return info.type != SideEffectType::NO_SIDE_EFFECT;
}

bool SideEffectAnalysisResult::mayModifyMemory(Instruction *inst) const {
  const auto &info = getInstructionSideEffect(inst);
  return info.mayModifyMemory;
}

bool SideEffectAnalysisResult::mayModifyGlobal(Instruction *inst) const {
  const auto &info = getInstructionSideEffect(inst);
  return info.mayModifyGlobal;
}

bool SideEffectAnalysisResult::isPureFunction(Function *func) const {
  const auto &info = getFunctionSideEffect(func);
  return info.isPure;
}

void SideEffectAnalysisResult::initializeKnownFunctions() {
  // SysY标准库函数的副作用信息

  // I/O函数 - 有副作用
  SideEffectInfo ioEffect;
  ioEffect.type = SideEffectType::IO_OPERATION;
  ioEffect.mayModifyGlobal = true;
  ioEffect.mayModifyMemory = true;
  ioEffect.mayCallFunction = true;
  ioEffect.isPure = false;

  // knownFunctions["printf"] = ioEffect;
  // knownFunctions["scanf"] = ioEffect;
  knownFunctions["getint"] = ioEffect;
  knownFunctions["getch"] = ioEffect;
  knownFunctions["getfloat"] = ioEffect;
  knownFunctions["getarray"] = ioEffect;
  knownFunctions["getfarray"] = ioEffect;
  knownFunctions["putint"] = ioEffect;
  knownFunctions["putch"] = ioEffect;
  knownFunctions["putfloat"] = ioEffect;
  knownFunctions["putarray"] = ioEffect;
  knownFunctions["putfarray"] = ioEffect;

  // 时间函数 - 有副作用
  SideEffectInfo timeEffect;
  timeEffect.type = SideEffectType::FUNCTION_CALL;
  timeEffect.mayModifyGlobal = true;
  timeEffect.mayModifyMemory = false;
  timeEffect.mayCallFunction = true;
  timeEffect.isPure = false;

  knownFunctions["_sysy_starttime"] = timeEffect;
  knownFunctions["_sysy_stoptime"] = timeEffect;
}

const SideEffectInfo *SideEffectAnalysisResult::getKnownFunctionSideEffect(const std::string &funcName) const {
  auto it = knownFunctions.find(funcName);
  return (it != knownFunctions.end()) ? &it->second : nullptr;
}

// ======================================================================
// SysYSideEffectAnalysisPass 类的实现
// ======================================================================

bool SysYSideEffectAnalysisPass::runOnModule(Module *M, AnalysisManager &AM) {
  if (DEBUG) {
    std::cout << "Running SideEffect analysis on module" << std::endl;
  }

  // 创建分析结果（构造函数中已经调用了initializeKnownFunctions）
  result = std::make_unique<SideEffectAnalysisResult>();

  // 获取调用图分析结果
  callGraphAnalysis = AM.getAnalysisResult<CallGraphAnalysisResult, CallGraphAnalysisPass>();
  if (!callGraphAnalysis) {
    std::cerr << "Warning: CallGraphAnalysis not available, falling back to conservative analysis" << std::endl;
  }

  // 按拓扑序分析函数，确保被调用函数先于调用者分析
  if (callGraphAnalysis) {
    // 使用调用图的拓扑排序结果
    const auto &topOrder = callGraphAnalysis->getTopologicalOrder();

    // 处理强连通分量（递归函数群）
    const auto &sccs = callGraphAnalysis->getStronglyConnectedComponents();
    for (const auto &scc : sccs) {
      if (scc.size() > 1) {
        // 多个函数的强连通分量，使用不动点算法
        analyzeStronglyConnectedComponent(scc, AM);
      } else {
        // 单个函数，检查是否自递归
        Function *func = scc[0];
        if (callGraphAnalysis->isSelfRecursive(func)) {
          // 自递归函数也需要不动点算法
          analyzeStronglyConnectedComponent(scc, AM);
        } else {
          // 非递归函数，直接分析
          SideEffectInfo funcEffect = analyzeFunction(func, AM);
          result->setFunctionSideEffect(func, funcEffect);
        }
      }
    }
  } else {
    // 没有调用图，保守地分析每个函数
    for (auto &pair : M->getFunctions()) {
      Function *func = pair.second.get();
      SideEffectInfo funcEffect = analyzeFunction(func, AM);
      result->setFunctionSideEffect(func, funcEffect);
    }
  }

  if (DEBUG) {
    std::cout << "---- Side Effect Analysis Results for Module ----\n";
    for (auto &pair : M->getFunctions()) {
      Function *func = pair.second.get();
      const auto &funcInfo = result->getFunctionSideEffect(func);

      std::cout << "Function " << func->getName() << ": ";
      switch (funcInfo.type) {
      case SideEffectType::NO_SIDE_EFFECT:
        std::cout << "No Side Effect";
        break;
      case SideEffectType::MEMORY_WRITE:
        std::cout << "Memory Write";
        break;
      case SideEffectType::FUNCTION_CALL:
        std::cout << "Function Call";
        break;
      case SideEffectType::IO_OPERATION:
        std::cout << "I/O Operation";
        break;
      case SideEffectType::UNKNOWN:
        std::cout << "Unknown";
        break;
      }
      std::cout << " (Pure: " << (funcInfo.isPure ? "Yes" : "No")
                << ", Modifies Global: " << (funcInfo.mayModifyGlobal ? "Yes" : "No") << ")\n";
    }
    std::cout << "--------------------------------------------------\n";
  }

  return false; // Analysis passes return false since they don't modify the IR
}

std::unique_ptr<AnalysisResultBase> SysYSideEffectAnalysisPass::getResult() { return std::move(result); }

SideEffectInfo SysYSideEffectAnalysisPass::analyzeFunction(Function *func, AnalysisManager &AM) {
  SideEffectInfo functionSideEffect;

  // 为每个指令分析副作用
  for (auto &BB : func->getBasicBlocks()) {
    for (auto &I : BB->getInstructions_Range()) {
      Instruction *inst = I.get();
      SideEffectInfo instEffect = analyzeInstruction(inst, func, AM);

      // 记录指令的副作用信息
      result->setInstructionSideEffect(inst, instEffect);

      // 合并到函数级别的副作用信息中
      functionSideEffect = functionSideEffect.merge(instEffect);
    }
  }

  return functionSideEffect;
}

void SysYSideEffectAnalysisPass::analyzeStronglyConnectedComponent(const std::vector<Function *> &scc,
                                                                   AnalysisManager &AM) {
  // 使用不动点算法处理递归函数群
  std::unordered_map<Function *, SideEffectInfo> currentEffects;
  std::unordered_map<Function *, SideEffectInfo> previousEffects;

  // 初始化：所有函数都假设为纯函数
  for (Function *func : scc) {
    SideEffectInfo initialEffect;
    initialEffect.isPure = true;
    currentEffects[func] = initialEffect;
    result->setFunctionSideEffect(func, initialEffect);
  }

  bool converged = false;
  int iterations = 0;
  const int maxIterations = 10; // 防止无限循环

  while (!converged && iterations < maxIterations) {
    previousEffects = currentEffects;

    // 重新分析每个函数
    for (Function *func : scc) {
      SideEffectInfo newEffect = analyzeFunction(func, AM);
      currentEffects[func] = newEffect;
      result->setFunctionSideEffect(func, newEffect);
    }

    // 检查是否收敛
    converged = hasConverged(previousEffects, currentEffects);
    iterations++;
  }

  if (iterations >= maxIterations) {
    std::cerr << "Warning: SideEffect analysis did not converge for SCC after " << maxIterations << " iterations"
              << std::endl;
  }
}

bool SysYSideEffectAnalysisPass::hasConverged(const std::unordered_map<Function *, SideEffectInfo> &oldEffects,
                                              const std::unordered_map<Function *, SideEffectInfo> &newEffects) const {
  for (const auto &pair : oldEffects) {
    Function *func = pair.first;
    const SideEffectInfo &oldEffect = pair.second;

    auto it = newEffects.find(func);
    if (it == newEffects.end()) {
      return false; // 函数不存在于新结果中
    }

    const SideEffectInfo &newEffect = it->second;

    // 比较关键属性是否相同
    if (oldEffect.type != newEffect.type || oldEffect.mayModifyGlobal != newEffect.mayModifyGlobal ||
        oldEffect.mayModifyMemory != newEffect.mayModifyMemory ||
        oldEffect.mayCallFunction != newEffect.mayCallFunction || oldEffect.isPure != newEffect.isPure) {
      return false;
    }
  }

  return true;
}

SideEffectInfo SysYSideEffectAnalysisPass::analyzeInstruction(Instruction *inst, Function *currentFunc,
                                                              AnalysisManager &AM) {
  SideEffectInfo info;

  // 根据指令类型进行分析
  if (inst->isCall()) {
    return analyzeCallInstruction(static_cast<CallInst *>(inst), currentFunc, AM);
  } else if (inst->isStore()) {
    return analyzeStoreInstruction(static_cast<StoreInst *>(inst), currentFunc, AM);
  } else if (inst->isMemset()) {
    return analyzeMemsetInstruction(static_cast<MemsetInst *>(inst), currentFunc, AM);
  } else if (inst->isBranch() || inst->isReturn()) {
    // 控制流指令无副作用，但必须保留
    info.type = SideEffectType::NO_SIDE_EFFECT;
    info.isPure = true;
  } else {
    // 其他指令（算术、逻辑、比较等）通常无副作用
    info.type = SideEffectType::NO_SIDE_EFFECT;
    info.isPure = true;
  }

  return info;
}

SideEffectInfo SysYSideEffectAnalysisPass::analyzeCallInstruction(CallInst *call, Function *currentFunc,
                                                                  AnalysisManager &AM) {
  SideEffectInfo info;

  // 获取被调用的函数
  Function *calledFunc = call->getCallee();
  if (!calledFunc) {
    // 间接调用，保守处理
    info.type = SideEffectType::UNKNOWN;
    info.mayModifyGlobal = true;
    info.mayModifyMemory = true;
    info.mayCallFunction = true;
    info.isPure = false;
    return info;
  }

  std::string funcName = calledFunc->getName();

  // 检查是否为已知的标准库函数
  const SideEffectInfo *knownInfo = result->getKnownFunctionSideEffect(funcName);
  if (knownInfo) {
    return *knownInfo;
  }

  // 利用调用图分析结果进行精确分析
  if (callGraphAnalysis) {
    // 检查被调用函数是否已分析过
    const SideEffectInfo &funcEffect = result->getFunctionSideEffect(calledFunc);
    if (funcEffect.type != SideEffectType::NO_SIDE_EFFECT || !funcEffect.isPure) {
      return funcEffect;
    }

    // 检查递归调用
    if (callGraphAnalysis->isRecursive(calledFunc)) {
      // 递归函数保守处理（在不动点算法中会精确分析）
      info.type = SideEffectType::FUNCTION_CALL;
      info.mayModifyGlobal = true;
      info.mayModifyMemory = true;
      info.mayCallFunction = true;
      info.isPure = false;
      return info;
    }
  }

  // 对于未分析的用户函数，保守处理
  info.type = SideEffectType::FUNCTION_CALL;
  info.mayModifyGlobal = true;
  info.mayModifyMemory = true;
  info.mayCallFunction = true;
  info.isPure = false;

  return info;
}

SideEffectInfo SysYSideEffectAnalysisPass::analyzeStoreInstruction(StoreInst *store, Function *currentFunc,
                                                                   AnalysisManager &AM) {
  SideEffectInfo info;
  info.type = SideEffectType::MEMORY_WRITE;
  info.mayModifyMemory = true;
  info.isPure = false;

  // 获取函数的别名分析结果
  AliasAnalysisResult *aliasAnalysis = AM.getAnalysisResult<AliasAnalysisResult, SysYAliasAnalysisPass>(currentFunc);
  if (aliasAnalysis) {
    Value *storePtr = store->getPointer();

    // 如果存储到全局变量或可能别名的位置，则可能修改全局状态
    if (!aliasAnalysis->isLocalArray(storePtr)) {
      info.mayModifyGlobal = true;
    }
  } else {
    // 没有别名分析结果，保守处理
    info.mayModifyGlobal = true;
  }

  return info;
}

SideEffectInfo SysYSideEffectAnalysisPass::analyzeMemsetInstruction(MemsetInst *memset, Function *currentFunc,
                                                                    AnalysisManager &AM) {
  SideEffectInfo info;
  info.type = SideEffectType::MEMORY_WRITE;
  info.mayModifyMemory = true;
  info.isPure = false;

  // 获取函数的别名分析结果
  AliasAnalysisResult *aliasAnalysis = AM.getAnalysisResult<AliasAnalysisResult, SysYAliasAnalysisPass>(currentFunc);
  if (aliasAnalysis) {
    Value *memsetPtr = memset->getPointer();

    // 如果memset操作全局变量或可能别名的位置，则可能修改全局状态
    if (!aliasAnalysis->isLocalArray(memsetPtr)) {
      info.mayModifyGlobal = true;
    }
  } else {
    // 没有别名分析结果，保守处理
    info.mayModifyGlobal = true;
  }

  return info;
}

} // namespace sysy
