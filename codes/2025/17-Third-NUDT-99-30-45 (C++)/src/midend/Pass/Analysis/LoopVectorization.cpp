#include "LoopVectorization.h"
#include "Dom.h"
#include "Loop.h"
#include "Liveness.h"
#include "AliasAnalysis.h" 
#include "SideEffectAnalysis.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <set>

extern int DEBUG;

namespace sysy {

// 定义 Pass 的唯一 ID
void *LoopVectorizationPass::ID = (void *)&LoopVectorizationPass::ID;

std::vector<int> DependenceVector::getDirectionVector() const {
  std::vector<int> direction;
  direction.reserve(distances.size());
  
  for (int dist : distances) {
    if (dist > 0) direction.push_back(1);       // 前向依赖
    else if (dist < 0) direction.push_back(-1); // 后向依赖  
    else direction.push_back(0);                // 无依赖
  }
  
  return direction;
}

bool DependenceVector::isVectorizationSafe() const {
  if (!isKnown) return false; // 未知依赖，不安全
  
  // 对于向量化，我们主要关心最内层循环的依赖
  if (distances.empty()) return true;
  
  int innermostDistance = distances.back(); // 最内层循环的距离
  
  // 前向依赖 (距离 > 0) 通常是安全的，可以通过调整向量化顺序处理
  // 后向依赖 (距离 < 0) 通常不安全，会阻止向量化
  // 距离 = 0 表示同一迭代内的依赖，通常安全
  
  return innermostDistance >= 0;
}

size_t LoopVectorizationResult::getVectorizableLoopCount() const {
  size_t count = 0;
  for (const auto& [loop, analysis] : VectorizationMap) {
    if (analysis.isVectorizable) count++;
  }
  return count;
}

size_t LoopVectorizationResult::getParallelizableLoopCount() const {
  size_t count = 0;
  for (const auto& [loop, analysis] : ParallelizationMap) {
    if (analysis.isParallelizable) count++;
  }
  return count;
}

std::vector<Loop*> LoopVectorizationResult::getVectorizationCandidates() const {
  std::vector<Loop*> candidates;
  for (const auto& [loop, analysis] : VectorizationMap) {
    if (analysis.isVectorizable) {
      candidates.push_back(loop);
    }
  }
  
  // 按建议的向量宽度排序，优先处理收益更大的循环
  std::sort(candidates.begin(), candidates.end(), 
    [this](Loop* a, Loop* b) {
      const auto& analysisA = VectorizationMap.at(a);
      const auto& analysisB = VectorizationMap.at(b);
      return analysisA.suggestedVectorWidth > analysisB.suggestedVectorWidth;
    });
  
  return candidates;
}

std::vector<Loop*> LoopVectorizationResult::getParallelizationCandidates() const {
  std::vector<Loop*> candidates;
  for (const auto& [loop, analysis] : ParallelizationMap) {
    if (analysis.isParallelizable) {
      candidates.push_back(loop);
    }
  }
  
  // 按建议的线程数排序
  std::sort(candidates.begin(), candidates.end(),
    [this](Loop* a, Loop* b) {
      const auto& analysisA = ParallelizationMap.at(a);
      const auto& analysisB = ParallelizationMap.at(b);
      return analysisA.suggestedThreadCount > analysisB.suggestedThreadCount;
    });
  
  return candidates;
}

void LoopVectorizationResult::print() const {
  if (!DEBUG) return;

  std::cout << "\n--- Loop Vectorization/Parallelization Analysis Results for Function: " 
            << AssociatedFunction->getName() << " ---" << std::endl;

  if (VectorizationMap.empty() && ParallelizationMap.empty()) {
    std::cout << "  No vectorization/parallelization analysis results." << std::endl;
    return;
  }

  // 统计信息
  std::cout << "\n=== Summary ===" << std::endl;
  std::cout << "Total Loops Analyzed: " << VectorizationMap.size() << std::endl;
  std::cout << "Vectorizable Loops: " << getVectorizableLoopCount() << std::endl;
  std::cout << "Parallelizable Loops: " << getParallelizableLoopCount() << std::endl;

  // 详细分析结果
  for (const auto& [loop, vecAnalysis] : VectorizationMap) {
    std::cout << "\n--- Loop: " << loop->getName() << " ---" << std::endl;
    
    // 向量化分析 (暂时搁置)
    std::cout << "  Vectorization: " << (vecAnalysis.isVectorizable ? "YES" : "NO") << std::endl;
    if (!vecAnalysis.preventingFactors.empty()) {
      std::cout << "    Preventing Factors: ";
      for (const auto& factor : vecAnalysis.preventingFactors) {
        std::cout << factor << " ";
      }
      std::cout << std::endl;
    }
    
    // 并行化分析
    auto parallelIt = ParallelizationMap.find(loop);
    if (parallelIt != ParallelizationMap.end()) {
      const auto& parAnalysis = parallelIt->second;
      std::cout << "  Parallelization: " << (parAnalysis.isParallelizable ? "YES" : "NO") << std::endl;
      if (parAnalysis.isParallelizable) {
        std::cout << "    Suggested Thread Count: " << parAnalysis.suggestedThreadCount << std::endl;
        if (parAnalysis.requiresReduction) {
          std::cout << "    Requires Reduction: Yes" << std::endl;
        }
        if (parAnalysis.requiresBarrier) {
          std::cout << "    Requires Barrier: Yes" << std::endl;
        }
      } else if (!parAnalysis.preventingFactors.empty()) {
        std::cout << "    Preventing Factors: ";
        for (const auto& factor : parAnalysis.preventingFactors) {
          std::cout << factor << " ";
        }
        std::cout << std::endl;
      }
    }
    
    // 依赖关系
    auto depIt = DependenceMap.find(loop);
    if (depIt != DependenceMap.end()) {
      const auto& dependences = depIt->second;
      std::cout << "  Dependences: " << dependences.size() << " found" << std::endl;
      for (const auto& dep : dependences) {
        if (dep.dependenceVector.isKnown) {
          std::cout << "    " << dep.source->getName() << " -> " << dep.sink->getName();
          std::cout << " [";
          for (size_t i = 0; i < dep.dependenceVector.distances.size(); ++i) {
            if (i > 0) std::cout << ",";
            std::cout << dep.dependenceVector.distances[i];
          }
          std::cout << "]" << std::endl;
        }
      }
    }
  }
  
  std::cout << "-----------------------------------------------" << std::endl;
}

bool LoopVectorizationPass::runOnFunction(Function *F, AnalysisManager &AM) {
  if (F->getBasicBlocks().empty()) {
    CurrentResult = std::make_unique<LoopVectorizationResult>(F);
    return false;
  }

  if (DEBUG) {
    std::cout << "Running LoopVectorizationPass on function: " << F->getName() << std::endl;
  }

  // 获取循环分析结果
  auto* loopAnalysisResult = AM.getAnalysisResult<LoopAnalysisResult, LoopAnalysisPass>(F);
  if (!loopAnalysisResult || !loopAnalysisResult->hasLoops()) {
    CurrentResult = std::make_unique<LoopVectorizationResult>(F);
    return false;
  }

  // 获取循环特征分析结果
  auto* loopCharacteristics = AM.getAnalysisResult<LoopCharacteristicsResult, LoopCharacteristicsPass>(F);
  if (!loopCharacteristics) {
    if (DEBUG) {
      std::cout << "Warning: LoopCharacteristics analysis not available" << std::endl;
    }
  }

  // 获取别名分析结果
  auto* aliasAnalysis = AM.getAnalysisResult<AliasAnalysisResult, SysYAliasAnalysisPass>(F);
  
  // 获取副作用分析结果
  auto* sideEffectAnalysis = AM.getAnalysisResult<SideEffectAnalysisResult, SysYSideEffectAnalysisPass>();

  CurrentResult = std::make_unique<LoopVectorizationResult>(F);

  // 分析每个循环的向量化/并行化可行性
  for (const auto& loop_ptr : loopAnalysisResult->getAllLoops()) {
    Loop* loop = loop_ptr.get();
    
    // 获取该循环的特征信息
    LoopCharacteristics* characteristics = nullptr;
    if (loopCharacteristics) {
      characteristics = const_cast<LoopCharacteristics*>(loopCharacteristics->getCharacteristics(loop));
    }
    
    analyzeLoop(loop, characteristics, aliasAnalysis, sideEffectAnalysis);
  }

  if (DEBUG) {
    std::cout << "LoopVectorizationPass completed. Found " 
              << CurrentResult->getVectorizableLoopCount() << " vectorizable loops, "
              << CurrentResult->getParallelizableLoopCount() << " parallelizable loops" << std::endl;
  }

  return false; // 分析遍不修改IR
}

void LoopVectorizationPass::analyzeLoop(Loop* loop, LoopCharacteristics* characteristics, 
                                       AliasAnalysisResult* aliasAnalysis, SideEffectAnalysisResult* sideEffectAnalysis) {
  if (DEBUG) {
    std::cout << "  Analyzing advanced features for loop: " << loop->getName() << std::endl;
  }

  // 1. 计算精确依赖向量
  auto dependences = computeDependenceVectors(loop, aliasAnalysis);
  CurrentResult->addDependenceAnalysis(loop, dependences);

  // 2. 分析向量化可行性 (暂时搁置，总是返回不可向量化)
  auto vecAnalysis = analyzeVectorizability(loop, dependences, characteristics);
  CurrentResult->addVectorizationAnalysis(loop, vecAnalysis);

  // 3. 分析并行化可行性 
  auto parAnalysis = analyzeParallelizability(loop, dependences, characteristics);
  CurrentResult->addParallelizationAnalysis(loop, parAnalysis);
}

// ========== 依赖向量分析实现 ==========

std::vector<PreciseDependence> LoopVectorizationPass::computeDependenceVectors(Loop* loop, 
                                                                              AliasAnalysisResult* aliasAnalysis) {
  std::vector<PreciseDependence> dependences;
  std::vector<Instruction*> memoryInsts;
  
  // 收集所有内存操作指令
  for (BasicBlock* bb : loop->getBlocks()) {
    for (auto& inst : bb->getInstructions()) {
      if (dynamic_cast<LoadInst*>(inst.get()) || dynamic_cast<StoreInst*>(inst.get())) {
        memoryInsts.push_back(inst.get());
      }
    }
  }
  
  // 分析每对内存操作之间的依赖关系
  for (size_t i = 0; i < memoryInsts.size(); ++i) {
    for (size_t j = i + 1; j < memoryInsts.size(); ++j) {
      Instruction* inst1 = memoryInsts[i];
      Instruction* inst2 = memoryInsts[j];
      
      Value* ptr1 = nullptr;
      Value* ptr2 = nullptr;
      
      if (auto* load = dynamic_cast<LoadInst*>(inst1)) {
        ptr1 = load->getPointer();
      } else if (auto* store = dynamic_cast<StoreInst*>(inst1)) {
        ptr1 = store->getPointer();
      }
      
      if (auto* load = dynamic_cast<LoadInst*>(inst2)) {
        ptr2 = load->getPointer();
      } else if (auto* store = dynamic_cast<StoreInst*>(inst2)) {
        ptr2 = store->getPointer();
      }
      
      if (!ptr1 || !ptr2) continue;
      
      // 检查是否可能存在别名关系
      bool mayAlias = false;
      if (aliasAnalysis) {
        mayAlias = aliasAnalysis->queryAlias(ptr1, ptr2) != AliasType::NO_ALIAS;
      } else {
        mayAlias = (ptr1 != ptr2); // 保守估计
      }
      
      if (mayAlias) {
        // 创建依赖关系
        PreciseDependence dep(loop->getLoopDepth());
        dep.source = inst1;
        dep.sink = inst2;
        dep.memoryLocation = ptr1;
        
        // 确定依赖类型
        bool isStore1 = dynamic_cast<StoreInst*>(inst1) != nullptr;
        bool isStore2 = dynamic_cast<StoreInst*>(inst2) != nullptr;
        
        if (isStore1 && !isStore2) {
          dep.type = DependenceType::TRUE_DEPENDENCE; // Write -> Read (RAW)
        } else if (!isStore1 && isStore2) {
          dep.type = DependenceType::ANTI_DEPENDENCE; // Read -> Write (WAR)
        } else if (isStore1 && isStore2) {
          dep.type = DependenceType::OUTPUT_DEPENDENCE; // Write -> Write (WAW)
        } else {
          continue; // Read -> Read (RAR) - 跳过，不是真正的依赖
        }
        
        // 计算依赖向量
        dep.dependenceVector = computeAccessDependence(inst1, inst2, loop);
        
        // 判断是否允许并行化
        dep.allowsParallelization = dep.dependenceVector.isLoopIndependent() || 
                                   (dep.dependenceVector.isKnown && 
                                    std::all_of(dep.dependenceVector.distances.begin(), 
                                               dep.dependenceVector.distances.end(),
                                               [](int d) { return d >= 0; }));
        
        dependences.push_back(dep);
        
        if (DEBUG && dep.dependenceVector.isKnown) {
          std::cout << "        Found dependence: " << inst1->getName() 
                    << " -> " << inst2->getName() << " [";
          for (size_t k = 0; k < dep.dependenceVector.distances.size(); ++k) {
            if (k > 0) std::cout << ",";
            std::cout << dep.dependenceVector.distances[k];
          }
          std::cout << "]" << std::endl;
        }
      }
    }
  }
  
  return dependences;
}

DependenceVector LoopVectorizationPass::computeAccessDependence(Instruction* inst1, Instruction* inst2, Loop* loop) {
  DependenceVector depVec(loop->getLoopDepth());
  
  Value* ptr1 = nullptr;
  Value* ptr2 = nullptr;
  
  if (auto* load = dynamic_cast<LoadInst*>(inst1)) {
    ptr1 = load->getPointer();
  } else if (auto* store = dynamic_cast<StoreInst*>(inst1)) {
    ptr1 = store->getPointer();
  }
  
  if (auto* load = dynamic_cast<LoadInst*>(inst2)) {
    ptr2 = load->getPointer();
  } else if (auto* store = dynamic_cast<StoreInst*>(inst2)) {
    ptr2 = store->getPointer();
  }
  
  if (!ptr1 || !ptr2) return depVec;
  
  // 尝试分析仿射关系
  if (areAccessesAffinelyRelated(ptr1, ptr2, loop)) {
    auto coeff1 = extractInductionCoefficients(ptr1, loop);
    auto coeff2 = extractInductionCoefficients(ptr2, loop);
    
    if (coeff1.size() == coeff2.size()) {
      depVec.isKnown = true;
      depVec.isConstant = true;
      
      for (size_t i = 0; i < coeff1.size(); ++i) {
        depVec.distances[i] = coeff2[i] - coeff1[i];
      }
    }
  }
  
  return depVec;
}

bool LoopVectorizationPass::areAccessesAffinelyRelated(Value* ptr1, Value* ptr2, Loop* loop) {
  // 简化实现：检查是否都是基于归纳变量的数组访问
  // 真正的实现需要复杂的仿射关系分析
  
  // 检查是否为 GEP 指令
  auto* gep1 = dynamic_cast<GetElementPtrInst*>(ptr1);
  auto* gep2 = dynamic_cast<GetElementPtrInst*>(ptr2);
  
  if (!gep1 || !gep2) return false;
  
  // 检查是否访问同一个数组基址
  if (gep1->getBasePointer() != gep2->getBasePointer()) return false;
  
  // 简化：假设都是仿射的
  return true;
}

// ========== 向量化分析实现 (暂时搁置) ==========

VectorizationAnalysis LoopVectorizationPass::analyzeVectorizability(Loop* loop, 
                                                                   const std::vector<PreciseDependence>& dependences,
                                                                   LoopCharacteristics* characteristics) {
  VectorizationAnalysis analysis; // 构造函数已设置为不可向量化
  
  if (DEBUG) {
    std::cout << "    Vectorization analysis: DISABLED (temporarily)" << std::endl;
  }
  
  // 向量化功能暂时搁置，总是返回不可向量化
  // 这里可以添加一些基本的诊断信息用于日志
  if (!loop->isInnermost()) {
    analysis.preventingFactors.push_back("Not innermost loop");
  }
  if (loop->getBlocks().size() > 1) {
    analysis.preventingFactors.push_back("Complex control flow");
  }
  if (!dependences.empty()) {
    analysis.preventingFactors.push_back("Has dependences (not analyzed in detail)");
  }
  
  return analysis;
}

// ========== 并行化分析实现 ==========

ParallelizationAnalysis LoopVectorizationPass::analyzeParallelizability(Loop* loop,
                                                                       const std::vector<PreciseDependence>& dependences,
                                                                       LoopCharacteristics* characteristics) {
  ParallelizationAnalysis analysis;
  
  if (DEBUG) {
    std::cout << "    Analyzing parallelizability for loop: " << loop->getName() << std::endl;
    std::cout << "      Found " << dependences.size() << " dependences" << std::endl;
  }
  
  // 按依赖类型分类分析
  bool hasTrueDependences = false;
  bool hasAntiDependences = false;
  bool hasOutputDependences = false;
  
  for (const auto& dep : dependences) {
    switch (dep.type) {
      case DependenceType::TRUE_DEPENDENCE:
        hasTrueDependences = true;
        // 真依赖通常是最难处理的，需要检查是否为归约模式
        if (dep.isReductionDependence) {
          analysis.requiresReduction = true;
          analysis.reductionVariables.insert(dep.memoryLocation);
        } else {
          analysis.preventingFactors.push_back("Non-reduction true dependence");
        }
        break;
      case DependenceType::ANTI_DEPENDENCE:
        hasAntiDependences = true;
        // 反依赖可以通过变量私有化解决
        analysis.privatizableVariables.insert(dep.memoryLocation);
        break;
      case DependenceType::OUTPUT_DEPENDENCE:
        hasOutputDependences = true;
        // 输出依赖可以通过变量私有化或原子操作解决
        analysis.sharedVariables.insert(dep.memoryLocation);
        break;
    }
  }
  
  // 确定并行化类型
  analysis.parallelType = determineParallelizationType(loop, dependences);
  
  // 基于依赖类型评估可并行性
  if (!hasTrueDependences && !hasOutputDependences) {
    // 只有反依赖或无依赖，完全可并行
    analysis.parallelType = ParallelizationAnalysis::EMBARRASSINGLY_PARALLEL;
    analysis.isParallelizable = true;
  } else if (analysis.requiresReduction) {
    // 有归约模式，可以并行但需要特殊处理
    analysis.parallelType = ParallelizationAnalysis::REDUCTION_PARALLEL;
    analysis.isParallelizable = true;
  } else if (hasTrueDependences) {
    // 有非归约的真依赖，通常不能并行化
    analysis.isParallelizable = false;
    analysis.preventingFactors.push_back("Non-reduction loop-carried true dependences");
  }
  
  if (analysis.isParallelizable) {
    // 进一步分析并行化收益和成本
    estimateParallelizationBenefit(loop, &analysis, characteristics);
    analyzeSynchronizationNeeds(loop, &analysis, dependences);
    analysis.suggestedThreadCount = estimateOptimalThreadCount(loop, characteristics);
  }
  
  if (DEBUG) {
    std::cout << "      Parallelizable: " << (analysis.isParallelizable ? "YES" : "NO") << std::endl;
    if (analysis.isParallelizable) {
      std::cout << "      Type: " << (int)analysis.parallelType << ", Threads: " << analysis.suggestedThreadCount << std::endl;
    }
  }
  
  return analysis;
}

bool LoopVectorizationPass::checkParallelizationLegality(Loop* loop, const std::vector<PreciseDependence>& dependences) {
  // 检查所有依赖是否允许并行化
  for (const auto& dep : dependences) {
    if (!dep.allowsParallelization) {
      return false;
    }
  }
  
  // 检查是否有无法并行化的操作
  for (BasicBlock* bb : loop->getBlocks()) {
    for (auto& inst : bb->getInstructions()) {
      // 检查原子操作、同步操作等
      if (auto* call = dynamic_cast<CallInst*>(inst.get())) {
        // 简化：假设函数调用需要特殊处理
        // 在实际实现中，需要分析函数的副作用
        return false;
      }
    }
  }
  
  return true;
}

int LoopVectorizationPass::estimateOptimalThreadCount(Loop* loop, LoopCharacteristics* characteristics) {
  // 基于循环特征估计最优线程数
  if (!characteristics) return 2;
  
  // 基于循环体大小和计算密度
  int baseThreads = 2;
  
  if (characteristics->instructionCount > 50) baseThreads = 4;
  if (characteristics->instructionCount > 200) baseThreads = 8;
  
  // 基于计算与内存比率调整
  if (characteristics->computeToMemoryRatio > 2.0) {
    baseThreads *= 2; // 计算密集型，可以使用更多线程
  }
  
  return std::min(baseThreads, 16); // 限制最大线程数
}

// ========== 辅助方法实现 ==========

bool LoopVectorizationPass::isConstantStride(Value* ptr, Loop* loop, int& stride) {
  // 简化实现：检查是否为常量步长访问
  stride = 1; // 默认步长
  
  auto* gep = dynamic_cast<GetElementPtrInst*>(ptr);
  if (!gep) return false;
  
  // 检查最后一个索引是否为归纳变量 + 常量
  if (gep->getNumIndices() > 0) {
    Value* lastIndex = gep->getIndex(gep->getNumIndices() - 1);
    
    // 简化：假设是 i 或 i+c 的形式
    if (auto* binInst = dynamic_cast<BinaryInst*>(lastIndex)) {
      if (binInst->getKind() == Instruction::kAdd) {
        // 检查是否为 i + constant
        if (auto* constInt = dynamic_cast<ConstantInteger*>(binInst->getRhs())) {
          stride = constInt->getInt();
          return true;
        }
      }
    }
    
    // 默认为步长1的连续访问
    stride = 1;
    return true;
  }
  
  return false;
}

std::vector<int> LoopVectorizationPass::extractInductionCoefficients(Value* ptr, Loop* loop) {
  // 简化实现：返回默认的仿射系数
  std::vector<int> coefficients;
  
  // 假设是简单的 a[i] 形式，系数为 [0, 1]
  coefficients.push_back(0); // 常数项
  coefficients.push_back(1); // 归纳变量系数
  
  return coefficients;
}

// ========== 缺失的方法实现 ==========

ParallelizationAnalysis::ParallelizationType LoopVectorizationPass::determineParallelizationType(
    Loop* loop, const std::vector<PreciseDependence>& dependences) {
  
  // 检查是否有任何依赖
  if (dependences.empty()) {
    return ParallelizationAnalysis::EMBARRASSINGLY_PARALLEL;
  }
  
  // 检查是否只有归约模式
  bool hasReduction = false;
  bool hasOtherDependences = false;
  
  for (const auto& dep : dependences) {
    if (dep.isReductionDependence) {
      hasReduction = true;
    } else if (dep.type == DependenceType::TRUE_DEPENDENCE) {
      hasOtherDependences = true;
    }
  }
  
  if (hasReduction && !hasOtherDependences) {
    return ParallelizationAnalysis::REDUCTION_PARALLEL;
  } else if (!hasOtherDependences) {
    return ParallelizationAnalysis::EMBARRASSINGLY_PARALLEL;
  }
  
  return ParallelizationAnalysis::NONE;
}

void LoopVectorizationPass::analyzeReductionPatterns(Loop* loop, ParallelizationAnalysis* analysis) {
  // 简化实现：查找常见的归约模式
  for (BasicBlock* bb : loop->getBlocks()) {
    for (auto& inst : bb->getInstructions()) {
      if (auto* binInst = dynamic_cast<BinaryInst*>(inst.get())) {
        if (binInst->getKind() == Instruction::kAdd || binInst->getKind() == Instruction::kMul) {
          // 检查是否为累加/累乘模式
          Value* lhs = binInst->getLhs();
          if (hasReductionPattern(lhs, loop)) {
            analysis->requiresReduction = true;
            analysis->reductionVariables.insert(lhs);
          }
        }
      }
    }
  }
}

void LoopVectorizationPass::analyzeMemoryAccessPatterns(Loop* loop, ParallelizationAnalysis* analysis, 
                                                       AliasAnalysisResult* aliasAnalysis) {
  std::vector<Value*> memoryAccesses;
  
  // 收集所有内存访问
  for (BasicBlock* bb : loop->getBlocks()) {
    for (auto& inst : bb->getInstructions()) {
      if (auto* load = dynamic_cast<LoadInst*>(inst.get())) {
        memoryAccesses.push_back(load->getPointer());
      } else if (auto* store = dynamic_cast<StoreInst*>(inst.get())) {
        memoryAccesses.push_back(store->getPointer());
      }
    }
  }
  
  // 分析内存访问独立性
  bool hasIndependentAccess = true;
  for (size_t i = 0; i < memoryAccesses.size(); ++i) {
    for (size_t j = i + 1; j < memoryAccesses.size(); ++j) {
      if (!isIndependentMemoryAccess(memoryAccesses[i], memoryAccesses[j], loop)) {
        hasIndependentAccess = false;
        analysis->hasMemoryConflicts = true;
      }
    }
  }
  
  analysis->hasIndependentAccess = hasIndependentAccess;
}

void LoopVectorizationPass::estimateParallelizationBenefit(Loop* loop, ParallelizationAnalysis* analysis,
                                                          LoopCharacteristics* characteristics) {
  if (!analysis->isParallelizable) {
    analysis->parallelizationBenefit = 0.0;
    return;
  }
  
  // 基于计算复杂度和并行度计算收益
  double workComplexity = estimateWorkComplexity(loop);
  double parallelFraction = 1.0; // 假设完全可并行
  
  // 根据依赖调整并行度
  if (analysis->requiresReduction) {
    parallelFraction *= 0.8; // 归约降低并行效率
  }
  if (analysis->hasMemoryConflicts) {
    parallelFraction *= 0.6; // 内存冲突降低效率
  }
  
  // Amdahl定律估算
  double serialFraction = 1.0 - parallelFraction;
  int threadCount = analysis->suggestedThreadCount;
  double speedup = 1.0 / (serialFraction + parallelFraction / threadCount);
  
  analysis->parallelizationBenefit = std::min((speedup - 1.0) / threadCount, 1.0);
  
  // 估算同步和通信开销
  analysis->synchronizationCost = analysis->requiresBarrier ? 100 : 0;
  analysis->communicationCost = analysis->sharedVariables.size() * 50;
}

void LoopVectorizationPass::identifyPrivatizableVariables(Loop* loop, ParallelizationAnalysis* analysis) {
  // 简化实现：标识循环内定义的变量为可私有化
  for (BasicBlock* bb : loop->getBlocks()) {
    for (auto& inst : bb->getInstructions()) {
      if (!inst->getType()->isVoid()) {
        // 如果变量只在循环内使用，可能可以私有化
        bool onlyUsedInLoop = true;
        for (auto& use : inst->getUses()) {
          if (auto* userInst = dynamic_cast<Instruction*>(use->getUser())) {
            if (!loop->contains(userInst->getParent())) {
              onlyUsedInLoop = false;
              break;
            }
          }
        }
        
        if (onlyUsedInLoop) {
          analysis->privatizableVariables.insert(inst.get());
        }
      }
    }
  }
}

void LoopVectorizationPass::analyzeSynchronizationNeeds(Loop* loop, ParallelizationAnalysis* analysis,
                                                       const std::vector<PreciseDependence>& dependences) {
  // 根据依赖类型确定同步需求
  for (const auto& dep : dependences) {
    if (dep.type == DependenceType::OUTPUT_DEPENDENCE) {
      analysis->requiresBarrier = true;
      analysis->sharedVariables.insert(dep.memoryLocation);
    }
  }
  
  // 如果有归约，需要特殊的归约同步
  if (analysis->requiresReduction) {
    analysis->requiresBarrier = true;
  }
}

bool LoopVectorizationPass::isIndependentMemoryAccess(Value* ptr1, Value* ptr2, Loop* loop) {
  // 简化实现：基本的独立性检查
  if (ptr1 == ptr2) return false;
  
  // 如果是不同的基址，认为是独立的
  auto* gep1 = dynamic_cast<GetElementPtrInst*>(ptr1);
  auto* gep2 = dynamic_cast<GetElementPtrInst*>(ptr2);
  
  if (gep1 && gep2) {
    if (gep1->getBasePointer() != gep2->getBasePointer()) {
      return true; // 不同的基址
    }
    // 相同基址，需要更精细的分析（这里简化为不独立）
    return false;
  }
  
  return true; // 默认认为独立
}

double LoopVectorizationPass::estimateWorkComplexity(Loop* loop) {
  double complexity = 0.0;
  
  for (BasicBlock* bb : loop->getBlocks()) {
    for (auto& inst : bb->getInstructions()) {
      // 基于指令类型分配复杂度权重
      if (auto* binInst = dynamic_cast<BinaryInst*>(inst.get())) {
        switch (binInst->getKind()) {
          case Instruction::kAdd:
          case Instruction::kSub:
            complexity += 1.0;
            break;
          case Instruction::kMul:
            complexity += 3.0;
            break;
          case Instruction::kDiv:
            complexity += 10.0;
            break;
          default:
            complexity += 2.0;
        }
      } else if (dynamic_cast<LoadInst*>(inst.get()) || dynamic_cast<StoreInst*>(inst.get())) {
        complexity += 2.0; // 内存访问
      } else {
        complexity += 1.0; // 其他指令
      }
    }
  }
  
  return complexity;
}

bool LoopVectorizationPass::hasReductionPattern(Value* var, Loop* loop) {
  // 简化实现：检查是否为简单的累加/累乘模式
  for (auto& use : var->getUses()) {
    if (auto* binInst = dynamic_cast<BinaryInst*>(use->getUser())) {
      if (binInst->getKind() == Instruction::kAdd || binInst->getKind() == Instruction::kMul) {
        // 检查是否为 var = var op something 的模式
        if (binInst->getLhs() == var || binInst->getRhs() == var) {
          return true;
        }
      }
    }
  }
  return false;
}

} // namespace sysy
