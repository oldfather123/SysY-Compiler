#include "GVN.h"
#include "Dom.h"
#include "SysYIROptUtils.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

extern int DEBUG;

namespace sysy {

// GVN 遍的静态 ID
void *GVN::ID = (void *)&GVN::ID;

// ======================================================================
// GVN 类的实现
// ======================================================================

bool GVN::runOnFunction(Function *func, AnalysisManager &AM) {
  if (func->getBasicBlocks().empty()) {
    return false;
  }

  if (DEBUG) {
    std::cout << "\n=== Running GVN on function: " << func->getName() << " ===" << std::endl;
  }

  bool changed = false;
  GVNContext context;
  context.run(func, &AM, changed);

  if (DEBUG) {
    if (changed) {
      std::cout << "GVN: Function " << func->getName() << " was modified" << std::endl;
    } else {
      std::cout << "GVN: Function " << func->getName() << " was not modified" << std::endl;
    }
    std::cout << "=== GVN completed for function: " << func->getName() << " ===" << std::endl;
  }
  changed |= SysYIROptUtils::eliminateRedundantPhisInFunction(func);
  return changed;
}

void GVN::getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const {
  // GVN依赖以下分析：
  // 1. 支配树分析 - 用于检查指令的支配关系，确保替换的安全性
  analysisDependencies.insert(&DominatorTreeAnalysisPass::ID);
  
  // 2. 副作用分析 - 用于判断函数调用是否可以进行GVN
  analysisDependencies.insert(&SysYSideEffectAnalysisPass::ID);

  // GVN不会使任何分析失效，因为：
  // - GVN只删除冗余计算，不改变CFG结构
  // - GVN不修改程序的语义，只是消除重复计算
  // - 支配关系保持不变
  // - 副作用分析结果保持不变
  // analysisInvalidations 保持为空
  
  if (DEBUG) {
    std::cout << "GVN: Declared analysis dependencies (DominatorTree, SideEffectAnalysis)" << std::endl;
  }
}

// ======================================================================
// GVNContext 类的实现 - 重构版本
// ======================================================================

// 简单的表达式哈希结构
struct ExpressionKey {
  enum Type { BINARY, UNARY, LOAD, GEP, CALL } type;
  int opcode;
  std::vector<Value*> operands;
  Type* resultType;
  
  bool operator==(const ExpressionKey& other) const {
    return type == other.type && opcode == other.opcode && 
           operands == other.operands && resultType == other.resultType;
  }
};

struct ExpressionKeyHash {
  size_t operator()(const ExpressionKey& key) const {
    size_t hash = std::hash<int>()(static_cast<int>(key.type)) ^ 
                  std::hash<int>()(key.opcode);
    for (auto op : key.operands) {
      hash ^= std::hash<Value*>()(op) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    return hash;
  }
};

void GVNContext::run(Function *func, AnalysisManager *AM, bool &changed) {
  if (DEBUG) {
    std::cout << "  Starting GVN analysis for function: " << func->getName() << std::endl;
  }

  // 获取分析结果
  if (AM) {
    domTree = AM->getAnalysisResult<DominatorTree, DominatorTreeAnalysisPass>(func);
    sideEffectAnalysis = AM->getAnalysisResult<SideEffectAnalysisResult, SysYSideEffectAnalysisPass>();

    if (DEBUG) {
      if (domTree) {
        std::cout << "    GVN: Using dominator tree analysis" << std::endl;
      } else {
        std::cout << "    GVN: Warning - dominator tree analysis not available" << std::endl;
      }
      if (sideEffectAnalysis) {
        std::cout << "    GVN: Using side effect analysis" << std::endl;
      } else {
        std::cout << "    GVN: Warning - side effect analysis not available" << std::endl;
      }
    }
  }

  // 清空状态
  valueToNumber.clear();
  numberToValue.clear();
  expressionToNumber.clear();
  nextValueNumber = 1;
  visited.clear();
  rpoBlocks.clear();
  needRemove.clear();

  // 计算逆后序遍历
  computeRPO(func);

  if (DEBUG) {
    std::cout << "    Computed RPO with " << rpoBlocks.size() << " blocks" << std::endl;
  }

  // 按逆后序遍历基本块进行GVN
  int blockCount = 0;
  for (auto bb : rpoBlocks) {
    if (DEBUG) {
      std::cout << "    Processing block " << ++blockCount << "/" << rpoBlocks.size() 
                << ": " << bb->getName() << std::endl;
    }
    
    processBasicBlock(bb, changed);
  }

  if (DEBUG) {
    std::cout << "    Found " << needRemove.size() << " redundant instructions to remove" << std::endl;
  }

  // 删除冗余指令
  eliminateRedundantInstructions(changed);

  if (DEBUG) {
    std::cout << "  GVN analysis completed for function: " << func->getName() << std::endl;
    std::cout << "    Total values numbered: " << valueToNumber.size() << std::endl;
    std::cout << "    Instructions eliminated: " << needRemove.size() << std::endl;
  }
}

void GVNContext::computeRPO(Function *func) {
  rpoBlocks.clear();
  visited.clear();

  auto entry = func->getEntryBlock();
  if (entry) {
    dfs(entry);
    std::reverse(rpoBlocks.begin(), rpoBlocks.end());
  }
}

void GVNContext::dfs(BasicBlock *bb) {
  if (!bb || visited.count(bb)) {
    return;
  }

  visited.insert(bb);

  // 访问所有后继基本块
  for (auto succ : bb->getSuccessors()) {
    if (visited.find(succ) == visited.end()) {
      dfs(succ);
    }
  }

  rpoBlocks.push_back(bb);
}

unsigned GVNContext::getValueNumber(Value* value) {
  // 如果已经有值编号，直接返回
  auto it = valueToNumber.find(value);
  if (it != valueToNumber.end()) {
    return it->second;
  }
  
  // 为新值分配编号
  return assignValueNumber(value);
}

unsigned GVNContext::assignValueNumber(Value* value) {
  unsigned number = nextValueNumber++;
  valueToNumber[value] = number;
  numberToValue[number] = value;
  
  if (DEBUG >= 2) {
    std::cout << "            Assigned value number " << number 
              << " to " << value->getName() << std::endl;
  }
  
  return number;
}

void GVNContext::processBasicBlock(BasicBlock* bb, bool& changed) {
  int instCount = 0;
  for (auto &instPtr : bb->getInstructions()) {
    if (DEBUG) {
      std::cout << "      Processing instruction " << ++instCount 
                << ": " << instPtr->getName() << std::endl;
    }
    
    if (processInstruction(instPtr.get())) {
      changed = true;
    }
  }
}

bool GVNContext::processInstruction(Instruction* inst) {
  // 跳过分支指令和其他不可优化的指令
  if (inst->isBranch() || dynamic_cast<ReturnInst*>(inst) || 
      dynamic_cast<AllocaInst*>(inst) || dynamic_cast<StoreInst*>(inst)) {
    
    // 如果是store指令，需要使相关的内存值失效
    if (auto store = dynamic_cast<StoreInst*>(inst)) {
      invalidateMemoryValues(store);
    }
    
    // 为这些指令分配值编号但不尝试优化
    getValueNumber(inst);
    return false;
  }
  
  if (DEBUG) {
    std::cout << "        Processing optimizable instruction: " << inst->getName() 
              << " (kind: " << static_cast<int>(inst->getKind()) << ")" << std::endl;
  }
  
  // 构建表达式键
  std::string exprKey = buildExpressionKey(inst);
  if (exprKey.empty()) {
    // 不可优化的指令，只分配值编号
    getValueNumber(inst);
    return false;
  }
  
  if (DEBUG >= 2) {
    std::cout << "          Expression key: " << exprKey << std::endl;
  }
  
  // 查找已存在的等价值
  Value* existing = findExistingValue(exprKey, inst);
  if (existing && existing != inst) {
    // 检查支配关系
    if (auto existingInst = dynamic_cast<Instruction*>(existing)) {
      if (dominates(existingInst, inst)) {
        if (DEBUG) {
          std::cout << "        GVN: Replacing " << inst->getName() 
                    << " with existing " << existing->getName() << std::endl;
        }
        
        // 用已存在的值替换当前指令
        inst->replaceAllUsesWith(existing);
        needRemove.insert(inst);
        
        // 将当前指令的值编号指向已存在的值
        unsigned existingNumber = getValueNumber(existing);
        valueToNumber[inst] = existingNumber;
        
        return true;
      } else {
        if (DEBUG) {
          std::cout << "          Found equivalent but dominance check failed" << std::endl;
        }
      }
    }
  }
  
  // 没有找到等价值，为这个表达式分配新的值编号
  unsigned number = assignValueNumber(inst);
  expressionToNumber[exprKey] = number;
  
  if (DEBUG) {
    std::cout << "        Instruction " << inst->getName() << " is unique" << std::endl;
  }
  
  return false;
}

std::string GVNContext::buildExpressionKey(Instruction* inst) {
  std::ostringstream oss;
  
  if (auto binary = dynamic_cast<BinaryInst*>(inst)) {
    oss << "binary_" << static_cast<int>(binary->getKind()) << "_";
    oss << getValueNumber(binary->getLhs()) << "_" << getValueNumber(binary->getRhs());
    
    // 对于可交换操作，确保操作数顺序一致
    if (binary->isCommutative()) {
      unsigned lhsNum = getValueNumber(binary->getLhs());
      unsigned rhsNum = getValueNumber(binary->getRhs());
      if (lhsNum > rhsNum) {
        oss.str("");
        oss << "binary_" << static_cast<int>(binary->getKind()) << "_";
        oss << rhsNum << "_" << lhsNum;
      }
    }
  } else if (auto unary = dynamic_cast<UnaryInst*>(inst)) {
    oss << "unary_" << static_cast<int>(unary->getKind()) << "_";
    oss << getValueNumber(unary->getOperand());
  } else if (auto gep = dynamic_cast<GetElementPtrInst*>(inst)) {
    oss << "gep_" << getValueNumber(gep->getBasePointer());
    for (unsigned i = 0; i < gep->getNumIndices(); ++i) {
      oss << "_" << getValueNumber(gep->getIndex(i));
    }
  } else if (auto load = dynamic_cast<LoadInst*>(inst)) {
    oss << "load_" << getValueNumber(load->getPointer());
    oss << "_" << reinterpret_cast<uintptr_t>(load->getType()); // 类型区分
  } else if (auto call = dynamic_cast<CallInst*>(inst)) {
    // 只为无副作用的函数调用建立表达式
    if (sideEffectAnalysis && sideEffectAnalysis->isPureFunction(call->getCallee())) {
      oss << "call_" << call->getCallee()->getName();
      for (size_t i = 1; i < call->getNumOperands(); ++i) { // 跳过函数指针
        oss << "_" << getValueNumber(call->getOperand(i));
      }
    } else {
      return ""; // 有副作用的函数调用不可优化
    }
  } else {
    return ""; // 不支持的指令类型
  }
  
  return oss.str();
}

Value* GVNContext::findExistingValue(const std::string& exprKey, Instruction* inst) {
  auto it = expressionToNumber.find(exprKey);
  if (it != expressionToNumber.end()) {
    unsigned number = it->second;
    auto valueIt = numberToValue.find(number);
    if (valueIt != numberToValue.end()) {
      Value* existing = valueIt->second;
      
      // 对于load指令，需要额外检查内存安全性
      if (auto loadInst = dynamic_cast<LoadInst*>(inst)) {
        if (auto existingLoad = dynamic_cast<LoadInst*>(existing)) {
          if (!isMemorySafe(existingLoad, loadInst)) {
            return nullptr;
          }
        }
      }
      
      return existing;
    }
  }
  return nullptr;
}

bool GVNContext::dominates(Instruction* a, Instruction* b) {
  auto aBB = a->getParent();
  auto bBB = b->getParent();
  
  // 同一基本块内的情况
  if (aBB == bBB) {
    auto &insts = aBB->getInstructions();
    auto aIt = std::find_if(insts.begin(), insts.end(), 
                           [a](const auto &ptr) { return ptr.get() == a; });
    auto bIt = std::find_if(insts.begin(), insts.end(),
                           [b](const auto &ptr) { return ptr.get() == b; });
    
    if (aIt == insts.end() || bIt == insts.end()) {
      return false;
    }
    
    return std::distance(insts.begin(), aIt) < std::distance(insts.begin(), bIt);
  }
  
  // 不同基本块的情况，使用支配树
  if (domTree) {
    auto dominators = domTree->getDominators(bBB);
    return dominators && dominators->count(aBB);
  }
  
  return false; // 保守做法
}

bool GVNContext::isMemorySafe(LoadInst* earlierLoad, LoadInst* laterLoad) {
  // 检查两个load是否访问相同的内存位置
  unsigned earlierPtr = getValueNumber(earlierLoad->getPointer());
  unsigned laterPtr = getValueNumber(laterLoad->getPointer());
  
  if (earlierPtr != laterPtr) {
    return false; // 不同的内存位置
  }
  
  // 检查类型是否匹配
  if (earlierLoad->getType() != laterLoad->getType()) {
    return false;
  }
  
  // 简单情况：如果在同一个基本块且没有中间的store，则安全
  auto earlierBB = earlierLoad->getParent();
  auto laterBB = laterLoad->getParent();
  
  if (earlierBB != laterBB) {
    // 跨基本块的情况需要更复杂的分析，暂时保守处理
    return false;
  }
  
  // 同一基本块内检查是否有中间的store
  auto &insts = earlierBB->getInstructions();
  auto earlierIt = std::find_if(insts.begin(), insts.end(),
                               [earlierLoad](const auto &ptr) { return ptr.get() == earlierLoad; });
  auto laterIt = std::find_if(insts.begin(), insts.end(),
                              [laterLoad](const auto &ptr) { return ptr.get() == laterLoad; });
  
  if (earlierIt == insts.end() || laterIt == insts.end()) {
    return false;
  }
  
  // 确保earlierLoad真的在laterLoad之前
  if (std::distance(insts.begin(), earlierIt) >= std::distance(insts.begin(), laterIt)) {
    return false;
  }
  
  // 检查中间是否有store指令修改了相同的内存位置
  for (auto it = std::next(earlierIt); it != laterIt; ++it) {
    if (auto store = dynamic_cast<StoreInst*>(it->get())) {
      unsigned storePtr = getValueNumber(store->getPointer());
      if (storePtr == earlierPtr) {
        return false; // 找到中间的store
      }
    }
    
    // 检查函数调用是否可能修改内存
    if (auto call = dynamic_cast<CallInst*>(it->get())) {
      if (sideEffectAnalysis && !sideEffectAnalysis->isPureFunction(call->getCallee())) {
        // 保守处理：有副作用的函数可能修改内存
        return false;
      }
    }
  }
  
  return true; // 安全
}

void GVNContext::invalidateMemoryValues(StoreInst* store) {
  unsigned storePtr = getValueNumber(store->getPointer());
  
  if (DEBUG) {
    std::cout << "        Invalidating memory values affected by store" << std::endl;
  }
  
  // 找到所有可能被这个store影响的load表达式
  std::vector<std::string> toRemove;
  
  for (auto& [exprKey, number] : expressionToNumber) {
    if (exprKey.find("load_" + std::to_string(storePtr)) == 0) {
      toRemove.push_back(exprKey);
      if (DEBUG) {
        std::cout << "          Invalidating expression: " << exprKey << std::endl;
      }
    }
  }
  
  // 移除失效的表达式
  for (const auto& key : toRemove) {
    expressionToNumber.erase(key);
  }
}

void GVNContext::eliminateRedundantInstructions(bool& changed) {
  int removeCount = 0;
  for (auto inst : needRemove) {
    if (DEBUG) {
      std::cout << "    Removing redundant instruction " << ++removeCount 
                << "/" << needRemove.size() << ": " << inst->getName() << std::endl;
    }
    
    // 删除指令前先断开所有使用关系
    // inst->replaceAllUsesWith 已在 processInstruction 中调用
    SysYIROptUtils::usedelete(inst);
    changed = true;
  }
}

} // namespace sysy
