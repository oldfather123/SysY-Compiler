#include "../include/midend/FuncAnalysis.h"
#include <algorithm>
#include <cassert>
#include <queue>
#include <set>
#include "../include/frontend/IR.h"

/**
 * @file FuncAnalysis.cpp
 *
 * 定义了函数分析的源文件
 */

namespace sysy {
/**
 * @brief 运行函数分析
 *
 * 无参数也无返回值，运行函数分析
 *
 * @param [in] void
 * @return 无返回值
 */
auto FuncAnalysis::run() -> void {
  auto CG = buildCallGraph(pModule);
  setFuncAll(CG);
}

/**
 * @brief 构建函数调用图
 *
 * @param [in] inModule 输入模块
 * @return 函数调用图
 */
auto FuncAnalysis::buildCallGraph(Module *inModule) -> std::map<Function *, std::set<Function *>> {
  std::map<Function *, std::set<Function *>> CG;
  const auto &functions = inModule->getFunctions();
  for (const auto &func : functions) {
    CG[func.second.get()] = std::set<Function *>();
  }
  for (const auto &func : functions) {
    for (const auto &use : func.second->getUses()) {
      auto callInst = dynamic_cast<CallInst *>(use->getUser());
      auto caller = callInst->getParent()->getParent();
      CG[caller].insert(func.second.get());
    }
  }
  return CG;
}

/**
 * @brief 拓扑排序
 *
 * @param [in] CG 函数调用图
 * @return 拓扑排序结果
 */
auto FuncAnalysis::topologocalSort(const std::map<Function *, std::set<Function *>> &CG) -> std::vector<Function *> {
  std::map<Function *, int> indegrees;
  std::vector<Function *> sorted;

  auto CGCopy = CG;
  for (const auto &pair : CG) {
    CGCopy[pair.first].erase(pair.first);
  }

  for (const auto &pair : CGCopy) {
    indegrees[pair.first] = 0;
  }
  for (const auto &pair : CGCopy) {
    for (Function *callee : pair.second) {
      indegrees[callee]++;
    }
  }

  assert(indegrees.size() == CGCopy.size());
  assert(CGCopy.size() == CG.size());

  for (const auto &pair : indegrees) {
    assert(pair.first->getName() == "main" && pair.second == 0 || pair.first->getName() != "main" && pair.second != 0);
  }

  std::queue<Function *> q;
  for (auto &pair : indegrees) {
    if (pair.second == 0) {
      q.push(pair.first);
    }
  }

  while (!q.empty()) {
    auto func = q.front();
    q.pop();
    sorted.push_back(func);

    for (Function *callee : CGCopy.at(func)) {
      indegrees[callee]--;
      if (indegrees[callee] == 0) {
        q.push(callee);
      }
    }
  }

  assert(sorted.size() == CG.size());
  assert(CG.size() == CGCopy.size());
  assert(CGCopy.size() == indegrees.size());

  return sorted;
}

/**
 * @brief 设置函数的属性
 *
 * @param [in] CG 函数调用图
 * @return 无返回值
 */
auto FuncAnalysis::setFuncAll(std::map<Function *, std::set<Function *>> &CG) -> void {
  for (const auto &func : CG) {
    func.first->clearCallees();
    func.first->clearAttribute();
    for (auto callee : func.second) {
      func.first->addCallee(callee);
    }
    if (isRecursiveFunc(func.first)) {
      func.first->setAttribute(Function::FunctionAttribute::SelfRecursive);
    }
    if (hasSideEffect(func.first)) {
      func.first->setAttribute(Function::FunctionAttribute::SideEffect);
    }
    if (isNoPureCauseMemRead(func.first)) {
      func.first->setAttribute(Function::FunctionAttribute::NoPureCauseMemRead);
    }
    if ((func.first->getAttribute() &
         (Function::FunctionAttribute::SideEffect | Function::FunctionAttribute::NoPureCauseMemRead)) == 0U) {
      func.first->setAttribute(Function::FunctionAttribute::Pure);
    }
  }
}

/**
 * @brief 判断是否为递归函数
 *
 * @param [in] func 函数
 * @return 是否为递归函数
 */
auto FuncAnalysis::isRecursiveFunc(Function *func) -> bool {
  auto blocks = func->getBasicBlocks();
  for (const auto &block : blocks) {
    for (const auto &inst : block->getInstructions()) {
      if (inst->isCall()) {
        auto callInst = dynamic_cast<CallInst *>(inst.get());
        if (callInst->getCallee() == func) {
          return true;
        }
      }
    }
  }
  return false;
}

/**
 * @brief 判断是否有副作用
 *
 * 1. 先看本函数是否有store全局变量(包括标量和数组)，store参数数组以及调用外部有影响的I/O函数
 * 2. 然后递归callee，看callee是否有store全局变量(包括标量和数组)，store参数数组以及调用getarray和getfarray
 *
 * @param [in] func 函数
 * @return 是否有副作用
 */
auto FuncAnalysis::hasSideEffect(Function *func) -> bool {
  if ((func->getAttribute() & Function::FunctionAttribute::SideEffect) != 0U) {
    return true;
  }
  auto blocks = func->getBasicBlocks();
  for (const auto &block : blocks) {
    for (const auto &inst : block->getInstructions()) {
      if (inst->isStore()) {
        auto storeInst = dynamic_cast<StoreInst *>(inst.get());
        auto pointer = storeInst->getPointer();
        if (isGlobal(pointer) ||
            (isArr(pointer) &&
             std::find(func->getEntryBlock()->getArguments().begin(), func->getEntryBlock()->getArguments().end(),
                       pointer) != func->getEntryBlock()->getArguments().end()) ||
            (isArr(pointer) &&
             std::find(func->getEntryBlock()->getArguments().begin(), func->getEntryBlock()->getArguments().end(),
                       dynamic_cast<AllocaInst *>(pointer)->getAncestorLVal()) !=
                 func->getEntryBlock()->getArguments().end())) {
          return true;
        }
      }
      if (inst->isCall()) {
        auto callInst = dynamic_cast<CallInst *>(inst.get());
        auto callee = callInst->getCallee();
        if (callee->getName() == "getarray" || callee->getName() == "getfarray" || callee->getName() == "putarray" ||
            callee->getName() == "putfarray" || callee->getName() == "putint" || callee->getName() == "putch" ||
            callee->getName() == "putfloat" || callee->getName() == "putf") {
          return true;
        }
      }
    }
  }
  auto calleesWithNoExternalAndSelf = func->getCalleesWithNoExternalAndSelf();
  return std::any_of(calleesWithNoExternalAndSelf.begin(), calleesWithNoExternalAndSelf.end(),
                     [](const auto &callee) { return hasSideEffect(callee); });
}

/**
 * @brief 获取全局变量写集
 *
 * @param [in] func 函数
 * @return 全局变量写集
 */
auto FuncAnalysis::globalWirttenSet(Function *func) -> std::set<GlobalValue *> {
  std::set<GlobalValue *> writtenSet;
  auto blocks = func->getBasicBlocks();
  for (const auto &block : blocks) {
    for (const auto &inst : block->getInstructions()) {
      if (inst->isStore()) {
        auto storeInst = dynamic_cast<StoreInst *>(inst.get());
        auto pointer = storeInst->getPointer();
        if (isGlobal(pointer)) {
          writtenSet.insert(dynamic_cast<GlobalValue *>(pointer));
        }
      }
      // else if (inst->isGetSubArray()) {
      //   auto getSubArrayInst = dynamic_cast<GetSubArrayInst *>(inst.get());
      //   auto pointer = getSubArrayInst->getFatherLVal()->getAncestorLVal();
      //   writtenSet.insert(dynamic_cast<GlobalValue*>(pointer));
      // }
    }
  }
  auto calleesWithNoExternalAndSelf = func->getCalleesWithNoExternalAndSelf();
  for (const auto callee : calleesWithNoExternalAndSelf) {
    auto calleeWrittenSet = globalWirttenSet(callee);
    writtenSet.insert(calleeWrittenSet.begin(), calleeWrittenSet.end());
  }
  return writtenSet;
}

/**
 * @brief 判断是否有内存读导致的非纯函数
 *
 * 1. 先看本函数是否有load全局变量(包括标量和数组)，load参数数组以及调用getint, getch, getfloat, getarray和getfarray
 * 2. 然后递归callee，看callee是否有load全局变量(包括标量和数组)，load参数数组以及调用getint, getch, getfloat,
 * getarray和getfarray
 *
 * @param [in] func 函数
 * @return 是否有内存读导致的非纯函数
 */
auto FuncAnalysis::isNoPureCauseMemRead(Function *func) -> bool {
  if ((func->getAttribute() & Function::FunctionAttribute::NoPureCauseMemRead) != 0U) {
    return true;
  }
  auto blocks = func->getBasicBlocks();
  for (const auto &block : blocks) {
    for (const auto &inst : block->getInstructions()) {
      if (inst->isLoad()) {
        auto loadInst = dynamic_cast<LoadInst *>(inst.get());
        if (loadInst != nullptr) {
          auto target = loadInst->getPointer();
          if (isGlobal(target) ||
              (isArr(target) &&
               std::find(func->getEntryBlock()->getArguments().begin(), func->getEntryBlock()->getArguments().end(),
                         target) != func->getEntryBlock()->getArguments().end()) ||
              (isArr(target) &&
               std::find(func->getEntryBlock()->getArguments().begin(), func->getEntryBlock()->getArguments().end(),
                         dynamic_cast<AllocaInst *>(target)->getAncestorLVal()) !=
                   func->getEntryBlock()->getArguments().end())) {
            return true;
          }
        }
      }
      if (inst->isCall()) {
        auto callInst = dynamic_cast<CallInst *>(inst.get());
        auto callee = callInst->getCallee();
        if (callee->getName() == "getint" || callee->getName() == "getch" || callee->getName() == "getfloat" ||
            callee->getName() == "getarray" || callee->getName() == "getfarray") {
          return true;
        }
      }
    }
  }
  auto calleesWithNoExternalAndSelf = func->getCalleesWithNoExternalAndSelf();
  return std::any_of(calleesWithNoExternalAndSelf.begin(), calleesWithNoExternalAndSelf.end(),
                     [](const auto &callee) { return isNoPureCauseMemRead(callee); });
}

/**
 * @brief 获取函数代码大小
 *
 * @param [in] func 函数
 * @return 函数代码大小
 */
auto FuncAnalysis::getFuncCodeSize(Function *func) -> int {
  int size = 0;
  for (const auto &block : func->getBasicBlocks()) {
    size += block->getInstructions().size();
  }
  return size;
}

/**
 * @brief 判断是否是全局变量
 *
 * @param [in] val 值
 * @return 是否是全局变量
 */
auto FuncAnalysis::isGlobal(Value *val) -> bool {
  auto gval = dynamic_cast<GlobalValue *>(val);
  return gval != nullptr;
}

/**
 * @brief 判断是否是数组
 *
 * @param [in] val 值
 * @return 是否是数组
 */
auto FuncAnalysis::isArr(Value *val) -> bool {
  auto aval = dynamic_cast<AllocaInst *>(val);
  return aval != nullptr && aval->getNumDims() != 0;
}
}  // namespace sysy
