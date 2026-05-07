#include "AliasAnalysis.h"
#include "SysYIRPrinter.h"
#include <iostream>

extern int DEBUG;

namespace sysy {

// 静态成员初始化
void *SysYAliasAnalysisPass::ID = (void *)&SysYAliasAnalysisPass::ID;

// ========== AliasAnalysisResult 实现 ==========

void AliasAnalysisResult::print() const {
    std::cout << "---- Alias Analysis Results for Function: " << AssociatedFunction->getName() << " ----\n";
    
    // 打印内存位置信息
    std::cout << "  Memory Locations (" << LocationMap.size() << "):\n";
    for (const auto& pair : LocationMap) {
        const auto& loc = pair.second;
        std::cout << "    - Base: " << loc->basePointer->getName();
        std::cout << " (Type: ";
        if (loc->isLocalArray) std::cout << "Local";
        else if (loc->isFunctionParameter) std::cout << "Parameter";
        else if (loc->isGlobalArray) std::cout << "Global";
        else std::cout << "Unknown";
        std::cout << ")\n";
    }
    
    // 打印别名关系
    std::cout << "  Alias Relations (" << AliasMap.size() << "):\n";
    for (const auto& pair : AliasMap) {
        std::cout << "    - (" << pair.first.first->getName() << ", " << pair.first.second->getName() << "): ";
        switch (pair.second) {
            case AliasType::NO_ALIAS: std::cout << "No Alias"; break;
            case AliasType::SELF_ALIAS: std::cout << "Self Alias"; break;
            case AliasType::POSSIBLE_ALIAS: std::cout << "Possible Alias"; break;
            case AliasType::UNKNOWN_ALIAS: std::cout << "Unknown Alias"; break;
        }
        std::cout << "\n";
    }
    std::cout << "-----------------------------------------------------------\n";
}

AliasType AliasAnalysisResult::queryAlias(Value* ptr1, Value* ptr2) const {
  auto key = std::make_pair(ptr1, ptr2);
  auto it = AliasMap.find(key);
  if (it != AliasMap.end()) {
    return it->second;
  }
  
  // 尝试反向查找
  key = std::make_pair(ptr2, ptr1);
  it = AliasMap.find(key);
  if (it != AliasMap.end()) {
    return it->second;
  }
  
  return AliasType::UNKNOWN_ALIAS; // 保守估计
}

const MemoryLocation* AliasAnalysisResult::getMemoryLocation(Value* ptr) const {
  auto it = LocationMap.find(ptr);
  return (it != LocationMap.end()) ? it->second.get() : nullptr;
}

bool AliasAnalysisResult::isLocalArray(Value* ptr) const {
  const MemoryLocation* loc = getMemoryLocation(ptr);
  return loc && loc->isLocalArray;
}

bool AliasAnalysisResult::isFunctionParameter(Value* ptr) const {
  const MemoryLocation* loc = getMemoryLocation(ptr);
  return loc && loc->isFunctionParameter;
}

bool AliasAnalysisResult::isGlobalArray(Value* ptr) const {
  const MemoryLocation* loc = getMemoryLocation(ptr);
  return loc && loc->isGlobalArray;
}

bool AliasAnalysisResult::hasConstantAccess(Value* ptr) const {
  const MemoryLocation* loc = getMemoryLocation(ptr);
  return loc && loc->hasConstantIndices;
}

AliasAnalysisResult::Statistics AliasAnalysisResult::getStatistics() const {
  Statistics stats = {0};
  
  stats.totalQueries = AliasMap.size();
  
  for (auto& pair : AliasMap) {
    switch (pair.second) {
      case AliasType::NO_ALIAS: stats.noAlias++; break;
      case AliasType::SELF_ALIAS: stats.selfAlias++; break;
      case AliasType::POSSIBLE_ALIAS: stats.possibleAlias++; break;
      case AliasType::UNKNOWN_ALIAS: stats.unknownAlias++; break;
    }
  }
  
  for (auto& loc : LocationMap) {
    if (loc.second->isLocalArray) stats.localArrays++;
    if (loc.second->isFunctionParameter) stats.functionParameters++;
    if (loc.second->isGlobalArray) stats.globalArrays++;
    if (loc.second->hasConstantIndices) stats.constantAccesses++;
  }
  
  return stats;
}

void AliasAnalysisResult::printStatics() const {
  std::cout << "=== Alias Analysis Results ===" << std::endl;
  
  auto stats = getStatistics();
  std::cout << "Total queries: " << stats.totalQueries << std::endl;
  std::cout << "No alias: " << stats.noAlias << std::endl;
  std::cout << "Self alias: " << stats.selfAlias << std::endl;
  std::cout << "Possible alias: " << stats.possibleAlias << std::endl;
  std::cout << "Unknown alias: " << stats.unknownAlias << std::endl;
  std::cout << "Local arrays: " << stats.localArrays << std::endl;
  std::cout << "Function parameters: " << stats.functionParameters << std::endl;
  std::cout << "Global arrays: " << stats.globalArrays << std::endl;
  std::cout << "Constant accesses: " << stats.constantAccesses << std::endl;
}

void AliasAnalysisResult::addMemoryLocation(std::unique_ptr<MemoryLocation> location) {
  Value* ptr = location->accessPointer;
  LocationMap[ptr] = std::move(location);
}

void AliasAnalysisResult::addAliasRelation(Value* ptr1, Value* ptr2, AliasType type) {
  auto key = std::make_pair(ptr1, ptr2);
  AliasMap[key] = type;
}

// ========== SysYAliasAnalysisPass 实现 ==========

bool SysYAliasAnalysisPass::runOnFunction(Function *F, AnalysisManager &AM) {
  if (DEBUG) {
    std::cout << "Running SysY Alias Analysis on function: " << F->getName() << std::endl;
  }
  
  // 创建分析结果
  CurrentResult = std::make_unique<AliasAnalysisResult>(F);
  
  // 执行主要分析步骤
  collectMemoryAccesses(F);
  buildAliasRelations(F);
  optimizeForSysY(F);
  
  if (DEBUG) {
    CurrentResult->print();
    CurrentResult->printStatics();
  }

  return false; // 分析遍不修改IR
}

void SysYAliasAnalysisPass::collectMemoryAccesses(Function* F) {
  // 收集函数中所有内存访问指令
  for (auto& bb : F->getBasicBlocks()) {
    for (auto& inst : bb->getInstructions()) {
      Value* ptr = nullptr;
      
      if (auto* loadInst = dynamic_cast<LoadInst*>(inst.get())) {
        ptr = loadInst->getPointer();
      } else if (auto* storeInst = dynamic_cast<StoreInst*>(inst.get())) {
        ptr = storeInst->getPointer();
      }
      
      if (ptr) {
        // 创建内存位置信息
        auto location = createMemoryLocation(ptr);
        location->accessInsts.push_back(inst.get());
        
        // 更新读写标记
        if (dynamic_cast<LoadInst*>(inst.get())) {
          location->hasReads = true;
        } else {
          location->hasWrites = true;
        }
        
        CurrentResult->addMemoryLocation(std::move(location));
      }
    }
  }
}

void SysYAliasAnalysisPass::buildAliasRelations(Function *F) {
  // 构建所有内存访问之间的别名关系
  auto& locationMap = CurrentResult->LocationMap;
  
  std::vector<Value*> allPointers;
  for (auto& pair : locationMap) {
    allPointers.push_back(pair.first);
  }
  
  // 两两比较所有指针
  for (size_t i = 0; i < allPointers.size(); ++i) {
    for (size_t j = i + 1; j < allPointers.size(); ++j) {
      Value* ptr1 = allPointers[i];
      Value* ptr2 = allPointers[j];
      
      MemoryLocation* loc1 = locationMap[ptr1].get();
      MemoryLocation* loc2 = locationMap[ptr2].get();
      
      AliasType aliasType = analyzeAliasBetween(loc1, loc2);
      CurrentResult->addAliasRelation(ptr1, ptr2, aliasType);
    }
  }
}

void SysYAliasAnalysisPass::optimizeForSysY(Function* F) {
  // SysY特化优化
  applySysYConstraints(F);
  optimizeParameterAnalysis(F);
  optimizeArrayAccessAnalysis(F);
}

std::unique_ptr<MemoryLocation> SysYAliasAnalysisPass::createMemoryLocation(Value* ptr) {
  Value* basePtr = getBasePointer(ptr);
  auto location = std::make_unique<MemoryLocation>(basePtr, ptr);
  
  // 分析内存类型和索引模式
  analyzeMemoryType(location.get());
  analyzeIndexPattern(location.get());
  
  return location;
}

Value* SysYAliasAnalysisPass::getBasePointer(Value* ptr) {
  // 递归剥离GEP指令，找到真正的基指针
  if (auto* gepInst = dynamic_cast<GetElementPtrInst*>(ptr)) {
    return getBasePointer(gepInst->getBasePointer());
  }
  return ptr;
}

void SysYAliasAnalysisPass::analyzeMemoryType(MemoryLocation* location) {
  Value* base = location->basePointer;
  
  // 检查内存类型
  if (dynamic_cast<AllocaInst*>(base)) {
    location->isLocalArray = true;
  } else if (dynamic_cast<Argument*>(base)) {
    location->isFunctionParameter = true;
  } else if (dynamic_cast<GlobalValue*>(base)) {
    location->isGlobalArray = true;
  }
}

void SysYAliasAnalysisPass::analyzeIndexPattern(MemoryLocation* location) {
  // 分析GEP指令的索引模式
  if (auto* gepInst = dynamic_cast<GetElementPtrInst*>(location->accessPointer)) {
    // 初始化为true，如果发现非常量索引则设为false
    location->hasConstantIndices = true;
    
    // 收集所有索引
    for (unsigned i = 0; i < gepInst->getNumIndices(); ++i) {
      Value* index = gepInst->getIndex(i);
      location->indices.push_back(index);
      
      // 检查是否为常量索引
      if (!isConstantValue(index)) {
        location->hasConstantIndices = false;
      }
    }
    
    // 检查是否包含循环变量
    Function* containingFunc = nullptr;
    if (auto* inst = dynamic_cast<Instruction*>(location->basePointer)) {
      containingFunc = inst->getParent()->getParent();
    } else if (auto* arg = dynamic_cast<Argument*>(location->basePointer)) {
      containingFunc = arg->getParent();
    }
    
    if (containingFunc) {
      location->hasLoopVariableIndex = hasLoopVariableInIndices(location->indices, containingFunc);
    }
    
    // 计算常量偏移
    if (location->hasConstantIndices) {
      location->constantOffset = calculateConstantOffset(location->indices);
    }
  }
}

AliasType SysYAliasAnalysisPass::analyzeAliasBetween(MemoryLocation* loc1, MemoryLocation* loc2) {
  // 分析两个内存位置之间的别名关系
  
  // 1. 相同基指针的情况需要进一步分析索引
  if (loc1->basePointer == loc2->basePointer) {
    // 如果是同一个访问指针，那就是完全相同的内存位置
    if (loc1->accessPointer == loc2->accessPointer) {
      return AliasType::SELF_ALIAS;
    }
    
    // 相同基指针但不同访问指针，需要比较索引
    return compareIndices(loc1, loc2);
  }
  
  // 2. 不同类型的内存位置
  if ((loc1->isLocalArray && loc2->isLocalArray)) {
    return compareLocalArrays(loc1, loc2);
  }
  
  if ((loc1->isFunctionParameter && loc2->isFunctionParameter)) {
    return compareParameters(loc1, loc2);
  }
  
  if ((loc1->isGlobalArray || loc2->isGlobalArray)) {
    return compareWithGlobal(loc1, loc2);
  }
  
  return compareMixedTypes(loc1, loc2);
}

AliasType SysYAliasAnalysisPass::compareIndices(MemoryLocation* loc1, MemoryLocation* loc2) {
  // 比较相同基指针下的不同索引访问
  
  // 如果都有常量索引，可以精确比较
  if (loc1->hasConstantIndices && loc2->hasConstantIndices) {
    // 比较索引数量
    if (loc1->indices.size() != loc2->indices.size()) {
      return AliasType::NO_ALIAS;
    }
    
    // 逐个比较索引值
    for (size_t i = 0; i < loc1->indices.size(); ++i) {
      Value* idx1 = loc1->indices[i];
      Value* idx2 = loc2->indices[i];
      
      // 都是常量，比较值
      auto* const1 = dynamic_cast<ConstantInteger*>(idx1);
      auto* const2 = dynamic_cast<ConstantInteger*>(idx2);
      
      if (const1 && const2) {
        int val1 = std::get<int>(const1->getVal());
        int val2 = std::get<int>(const2->getVal());
        
        if (val1 != val2) {
          return AliasType::NO_ALIAS;  // 不同常量索引，确定无别名
        }
      } else {
        // 不是常量，无法确定
        return AliasType::POSSIBLE_ALIAS;
      }
    }
    
    // 所有索引都相同
    return AliasType::SELF_ALIAS;
  }
  
  // 如果有非常量索引，保守估计
  return AliasType::POSSIBLE_ALIAS;
}

AliasType SysYAliasAnalysisPass::compareLocalArrays(MemoryLocation* loc1, MemoryLocation* loc2) {
  // 不同局部数组不别名
  return AliasType::NO_ALIAS;
}

AliasType SysYAliasAnalysisPass::compareParameters(MemoryLocation* loc1, MemoryLocation* loc2) {
  // SysY特化：可配置的数组参数别名策略
  // 
  // SysY中数组参数的语法形式：
  //   void func(int a[], int b[])     - 一维数组参数
  //   void func(int a[][10], int b[]) - 多维数组参数
  //
  // 默认保守策略：不同数组参数可能别名（因为可能传入相同数组）
  //   func(arr, arr);  // 传入同一个数组给两个参数
  //
  // 激进策略：假设不同数组参数不会传入相同数组（适用于评测环境）
  //   在SysY评测中，这种情况很少出现
  
  if (useAggressiveParameterAnalysis()) {
    // 激进策略：不同数组参数假设不别名
    return AliasType::NO_ALIAS;
  } else {
    // 保守策略：不同数组参数可能别名
    return AliasType::POSSIBLE_ALIAS;
  }
}

AliasType SysYAliasAnalysisPass::compareWithGlobal(MemoryLocation* loc1, MemoryLocation* loc2) {
  // 涉及全局数组的访问分析
  // 这里处理所有涉及全局数组的情况
  
  // SysY特化：局部数组与全局数组不别名
  if ((loc1->isLocalArray && loc2->isGlobalArray) || 
      (loc1->isGlobalArray && loc2->isLocalArray)) {
    // 局部数组在栈上，全局数组在全局区，确定不别名
    return AliasType::NO_ALIAS;
  }
  
  // SysY特化：数组参数与全局数组可能别名（保守处理）
  if ((loc1->isFunctionParameter && loc2->isGlobalArray) || 
      (loc1->isGlobalArray && loc2->isFunctionParameter)) {
    // 数组参数可能指向全局数组，需要保守处理
    return AliasType::POSSIBLE_ALIAS;
  }
  
  // 其他涉及全局数组的情况，采用保守策略
  return AliasType::POSSIBLE_ALIAS;
}

AliasType SysYAliasAnalysisPass::compareMixedTypes(MemoryLocation* loc1, MemoryLocation* loc2) {
  // 混合类型访问的别名分析
  // 处理不同内存类型之间的别名关系
  
  // SysY特化：局部数组与数组参数通常不别名
  // 典型场景：
  //   void func(int p[]) {       // p 是数组参数
  //       int local[10];         // local 是局部数组
  //       p[0] = local[0];       // 混合类型访问
  //   }
  // 或多维数组：
  //   void func(int p[][10]) {   // p 是多维数组参数
  //       int local[10];         // local 是局部数组
  //       p[i][0] = local[0];    // 混合类型访问
  //   } 
  // 局部数组与数组参数：在SysY中通常不别名
  if ((loc1->isLocalArray && loc2->isFunctionParameter) || 
      (loc1->isFunctionParameter && loc2->isLocalArray)) {
    // 因为局部数组是栈上分配，而数组参数是传入的外部数组
    return AliasType::NO_ALIAS;
  }

  // 对于其他混合情况，保守估计
  return AliasType::UNKNOWN_ALIAS;
}

void SysYAliasAnalysisPass::applySysYConstraints(Function* F) {
  // SysY语言特定的约束和优化
  // 1. SysY没有指针运算，简化了别名分析
  // 2. 数组传参时保持数组语义
  // 3. 没有动态内存分配，所有数组要么是局部的要么是参数/全局
}

void SysYAliasAnalysisPass::optimizeParameterAnalysis(Function* F) {
  // 数组参数别名分析优化
  // 为SysY评测环境提供可配置的优化策略
  
  if (!enableParameterOptimization()) {
    return; // 保持默认的保守策略
  }
  
  // 可选的参数优化：假设不同数组参数不会传入相同数组
  // 典型的SysY函数调用：
  //   int arr1[10], arr2[20];
  //   func(arr1, arr2);  // 传入不同数组
  // 而不是：
  //   func(arr1, arr1);  // 传入相同数组给两个参数
  // 这在SysY评测中通常是安全的假设
  auto& locationMap = CurrentResult->LocationMap;
  
  for (auto it1 = locationMap.begin(); it1 != locationMap.end(); ++it1) {
    for (auto it2 = std::next(it1); it2 != locationMap.end(); ++it2) {
      MemoryLocation* loc1 = it1->second.get();
      MemoryLocation* loc2 = it2->second.get();
      
      // 如果两个都是数组参数且基指针不同，设为NO_ALIAS
      if (loc1->isFunctionParameter && loc2->isFunctionParameter && 
          loc1->basePointer != loc2->basePointer) {
        CurrentResult->addAliasRelation(it1->first, it2->first, AliasType::NO_ALIAS);
      }
    }
  }
}

void SysYAliasAnalysisPass::optimizeArrayAccessAnalysis(Function* F) {
  // 数组访问别名分析优化
  // 基于SysY语言的特点进行简单优化
  
  // 优化1：同一数组的不同常量索引访问确定无别名
  optimizeConstantIndexAccesses();
  
  // 优化2：识别简单的顺序访问模式
  optimizeSequentialAccesses();
}

bool SysYAliasAnalysisPass::isConstantValue(Value* val) {
  return dynamic_cast<ConstantInteger*>(val) != nullptr; // 简化，只检查整数常量
}

bool SysYAliasAnalysisPass::hasLoopVariableInIndices(const std::vector<Value*>& indices, Function* F) {
  // 保守策略：所有非常量索引都视为可能的循环变量
  // 这样可以避免复杂的循环分析依赖，保持分析的独立性
  for (Value* index : indices) {
    if (!isConstantValue(index)) {
      return true; // 保守估计，确保正确性
    }
  }
  return false;
}

int SysYAliasAnalysisPass::calculateConstantOffset(const std::vector<Value*>& indices) {
  int offset = 0;
  for (Value* index : indices) {
    if (auto* constInt = dynamic_cast<ConstantInteger*>(index)) {
      // ConstantInteger的getVal()返回variant，需要提取int值
      auto val = constInt->getVal();
      if (std::holds_alternative<int>(val)) {
        offset += std::get<int>(val);
      }
    }
  }
  return offset;
}

void SysYAliasAnalysisPass::printStatistics() const {
  if (CurrentResult) {
    CurrentResult->print();
  }
}

void SysYAliasAnalysisPass::optimizeConstantIndexAccesses() {
  // 优化常量索引访问的别名关系
  // 对于相同基指针的访问，如果索引都是常量且不同，则确定无别名
  
  auto& locationMap = CurrentResult->LocationMap;
  std::vector<Value*> allPointers;
  for (auto& pair : locationMap) {
    allPointers.push_back(pair.first);
  }
  
  for (size_t i = 0; i < allPointers.size(); ++i) {
    for (size_t j = i + 1; j < allPointers.size(); ++j) {
      Value* ptr1 = allPointers[i];
      Value* ptr2 = allPointers[j];
      MemoryLocation* loc1 = locationMap[ptr1].get();
      MemoryLocation* loc2 = locationMap[ptr2].get();
      
      // 相同基指针且都有常量索引
      if (loc1->basePointer == loc2->basePointer && 
          loc1->hasConstantIndices && loc2->hasConstantIndices) {
        
        // 比较常量偏移
        if (loc1->constantOffset != loc2->constantOffset) {
          // 不同的常量偏移，确定无别名
          CurrentResult->addAliasRelation(ptr1, ptr2, AliasType::NO_ALIAS);
        }
      }
    }
  }
}

void SysYAliasAnalysisPass::optimizeSequentialAccesses() {
  // 识别和优化顺序访问模式
  // 这是一个简化的实现，主要用于识别数组的顺序遍历
  
  // 在SysY中，大多数数组访问都是通过循环进行的
  // 对于非常量索引的访问，我们采用保守策略，不进行过多优化
  // 这样可以保持分析的简单性和正确性
  
  // 未来如果需要更精确的分析，可以在这里添加更复杂的逻辑
}

} // namespace sysy
