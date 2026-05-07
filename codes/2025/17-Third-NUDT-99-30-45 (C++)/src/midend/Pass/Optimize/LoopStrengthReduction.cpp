#include "LoopStrengthReduction.h"
#include "LoopCharacteristics.h"
#include "Loop.h"
#include "Dom.h"
#include "IRBuilder.h"
#include "SysYIROptUtils.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <climits>

// 使用全局调试开关
extern int DEBUG;

namespace sysy {

// 定义 Pass 
void *LoopStrengthReduction::ID = (void *)&LoopStrengthReduction::ID;

bool StrengthReductionContext::analyzeInductionVariableRange(
  const InductionVarInfo* ivInfo, 
  Loop* loop
) const {
  if (!ivInfo->valid) {
    if (DEBUG) {
      std::cout << "        Invalid IV info, assuming potential negative" << std::endl;
    }
    return true; // 保守假设非线性变化可能为负数
  }

  // 获取phi指令的所有入口值
  auto* phiInst = dynamic_cast<PhiInst*>(ivInfo->base);
  if (!phiInst) {
    if (DEBUG) {
      std::cout << "        No phi instruction, assuming potential negative" << std::endl;
    }
    return true; // 无法确定，保守假设
  }

  bool hasNegativePotential = false;
  bool hasNonNegativeInitial = false;
  int initialValue = 0;
  
  for (auto& [incomingBB, incomingVal] : phiInst->getIncomingValues()) {
    // 检查初始值（来自循环外的值）
    if (!loop->contains(incomingBB)) {
      if (auto* constInt = dynamic_cast<ConstantInteger*>(incomingVal)) {
        initialValue = constInt->getInt();
        if (initialValue < 0) {
          if (DEBUG) {
            std::cout << "        Found negative initial value: " << initialValue << std::endl;
          }
          hasNegativePotential = true;
        } else {
          if (DEBUG) {
            std::cout << "        Found non-negative initial value: " << initialValue << std::endl;
          }
          hasNonNegativeInitial = true;
        }
      } else {
        // 如果不是常数初始值，保守假设可能为负数
        if (DEBUG) {
          std::cout << "        Non-constant initial value, assuming potential negative" << std::endl;
        }
        hasNegativePotential = true;
      }
    }
  }

  // 检查递增值和偏移
  if (ivInfo->factor < 0) {
    if (DEBUG) {
      std::cout << "        Negative factor: " << ivInfo->factor << std::endl;
    }
    hasNegativePotential = true;
  }

  if (ivInfo->offset < 0) {
    if (DEBUG) {
      std::cout << "        Negative offset: " << ivInfo->offset << std::endl;
    }
    hasNegativePotential = true;
  }

  // 精确分析：如果初始值非负，递增为正，偏移非负，则整个序列非负
  if (hasNonNegativeInitial && ivInfo->factor > 0 && ivInfo->offset >= 0) {
    if (DEBUG) {
      std::cout << "        ANALYSIS: Confirmed non-negative range" << std::endl;
      std::cout << "          Initial: " << initialValue << " >= 0" << std::endl;
      std::cout << "          Factor: " << ivInfo->factor << " > 0" << std::endl;
      std::cout << "          Offset: " << ivInfo->offset << " >= 0" << std::endl;
    }
    return false; // 确定不会为负数
  }

  // 报告分析结果
  if (DEBUG) {
    if (hasNegativePotential) {
      std::cout << "        ANALYSIS: Potential negative values detected" << std::endl;
    } else {
      std::cout << "        ANALYSIS: No negative indicators, but missing positive confirmation" << std::endl;
    }
  }

  return hasNegativePotential;
}


bool LoopStrengthReduction::runOnFunction(Function* F, AnalysisManager& AM) {
  if (F->getBasicBlocks().empty()) {
    return false; // 空函数
  }

  if (DEBUG) {
    std::cout << "Running LoopStrengthReduction on function: " << F->getName() << std::endl;
  }

  // 创建优化上下文并运行
  StrengthReductionContext context(builder);
  bool modified = context.run(F, AM);

  if (DEBUG) {
    std::cout << "LoopStrengthReduction " << (modified ? "modified" : "did not modify") 
              << " function: " << F->getName() << std::endl;
  }

  return modified;
}

void LoopStrengthReduction::getAnalysisUsage(std::set<void*>& analysisDependencies, 
                                           std::set<void*>& analysisInvalidations) const {
  // 依赖的分析
  analysisDependencies.insert(&LoopAnalysisPass::ID);
  analysisDependencies.insert(&LoopCharacteristicsPass::ID);
  analysisDependencies.insert(&DominatorTreeAnalysisPass::ID);
  
  // 会使失效的分析（强度削弱会修改IR结构）
  analysisInvalidations.insert(&LoopCharacteristicsPass::ID);
  // 注意：支配树分析通常不会因为强度削弱而失效，因为我们不改变控制流
}

// ========== StrengthReductionContext 实现 ==========

bool StrengthReductionContext::run(Function* F, AnalysisManager& AM) {
  if (DEBUG) {
    std::cout << "  Starting strength reduction analysis..." << std::endl;
  }

  // 获取必要的分析结果
  loopAnalysis = AM.getAnalysisResult<LoopAnalysisResult, LoopAnalysisPass>(F);
  if (!loopAnalysis || !loopAnalysis->hasLoops()) {
    if (DEBUG) {
      std::cout << "  No loops found, skipping strength reduction" << std::endl;
    }
    return false;
  }

  loopCharacteristics = AM.getAnalysisResult<LoopCharacteristicsResult, LoopCharacteristicsPass>(F);
  if (!loopCharacteristics) {
    if (DEBUG) {
      std::cout << "  LoopCharacteristics analysis not available" << std::endl;
    }
    return false;
  }

  dominatorTree = AM.getAnalysisResult<DominatorTree, DominatorTreeAnalysisPass>(F);
  if (!dominatorTree) {
    if (DEBUG) {
      std::cout << "  DominatorTree analysis not available" << std::endl;
    }
    return false;
  }

  // 执行三个阶段的优化
  
  // 阶段1：识别候选项
  identifyStrengthReductionCandidates(F);
  
  if (candidates.empty()) {
    if (DEBUG) {
      std::cout << "  No strength reduction candidates found" << std::endl;
    }
    return false;
  }

  if (DEBUG) {
    std::cout << "  Found " << candidates.size() << " potential candidates" << std::endl;
  }

  // 阶段2：分析优化潜力
  analyzeOptimizationPotential();

  // 阶段3：执行优化
  bool modified = performStrengthReduction();

  if (DEBUG) {
    printDebugInfo();
  }

  return modified;
}

void StrengthReductionContext::identifyStrengthReductionCandidates(Function* F) {
  if (DEBUG) {
    std::cout << "  === Phase 1: Identifying Strength Reduction Candidates ===" << std::endl;
  }

  // 遍历所有循环
  for (const auto& loop_ptr : loopAnalysis->getAllLoops()) {
    Loop* loop = loop_ptr.get();
    
    if (DEBUG) {
      std::cout << "    Analyzing loop: " << loop->getName() << std::endl;
    }

    // 获取循环特征
    const LoopCharacteristics* characteristics = loopCharacteristics->getCharacteristics(loop);
    if (!characteristics) {
      if (DEBUG) {
        std::cout << "      No characteristics available for loop" << std::endl;
      }
      continue;
    }

    if (characteristics->InductionVars.empty()) {
      if (DEBUG) {
        std::cout << "      No induction variables found in loop" << std::endl;
      }
      continue;
    }

    // 遍历循环中的所有指令
    for (BasicBlock* bb : loop->getBlocks()) {
      for (auto& inst_ptr : bb->getInstructions()) {
        Instruction* inst = inst_ptr.get();
        
        // 检查是否为强度削弱候选项
        auto candidate = isStrengthReductionCandidate(inst, loop);
        if (candidate) {
          if (DEBUG) {
            std::cout << "      Found candidate: %" << inst->getName() 
                      << " (IV: %" << candidate->inductionVar->getName() 
                      << ", multiplier: " << candidate->multiplier 
                      << ", offset: " << candidate->offset << ")" << std::endl;
          }
          
          // 添加到候选项列表
          loopToCandidates[loop].push_back(candidate.get());
          candidates.push_back(std::move(candidate));
        }
      }
    }
  }

  if (DEBUG) {
    std::cout << "  === End Phase 1: Found " << candidates.size() << " candidates ===" << std::endl;
  }
}

std::unique_ptr<StrengthReductionCandidate> 
StrengthReductionContext::isStrengthReductionCandidate(Instruction* inst, Loop* loop) {
  auto kind = inst->getKind();
  
  // 支持乘法、除法、取模指令
  if (kind != Instruction::Kind::kMul && 
      kind != Instruction::Kind::kDiv && 
      kind != Instruction::Kind::kRem) {
    return nullptr;
  }

  auto* binaryInst = dynamic_cast<BinaryInst*>(inst);
  if (!binaryInst) {
    return nullptr;
  }

  Value* op0 = binaryInst->getOperand(0);
  Value* op1 = binaryInst->getOperand(1);

  // 检查模式：归纳变量 op 常数 或 常数 op 归纳变量
  Value* inductionVar = nullptr;
  int constantValue = 0;
  StrengthReductionCandidate::OpType opType;
  
  // 获取循环特征信息
  const LoopCharacteristics* characteristics = loopCharacteristics->getCharacteristics(loop);
  if (!characteristics) {
    return nullptr;
  }

  // 确定操作类型
  switch (kind) {
    case Instruction::Kind::kMul:
      opType = StrengthReductionCandidate::MULTIPLY;
      break;
    case Instruction::Kind::kDiv:
      opType = StrengthReductionCandidate::DIVIDE;
      break;
    case Instruction::Kind::kRem:
      opType = StrengthReductionCandidate::REMAINDER;
      break;
    default:
      return nullptr;
  }

  // 模式1: IV op const
  const InductionVarInfo* ivInfo = getInductionVarInfo(op0, loop, characteristics);
  if (ivInfo && dynamic_cast<ConstantInteger*>(op1)) {
    inductionVar = op0;
    constantValue = dynamic_cast<ConstantInteger*>(op1)->getInt();
  }
  // 模式2: const op IV (仅对乘法有效)
  else if (opType == StrengthReductionCandidate::MULTIPLY) {
    ivInfo = getInductionVarInfo(op1, loop, characteristics);
    if (ivInfo && dynamic_cast<ConstantInteger*>(op0)) {
      inductionVar = op1;
      constantValue = dynamic_cast<ConstantInteger*>(op0)->getInt();
    }
  }

  if (!inductionVar || constantValue <= 1) {
    return nullptr; // 不是有效的候选项
  }

  // 创建候选项
  auto candidate = std::make_unique<StrengthReductionCandidate>(
    inst, inductionVar, opType, constantValue, 0, inst->getParent(), loop
  );

  // 分析归纳变量是否可能为负数
  candidate->hasNegativeValues = analyzeInductionVariableRange(ivInfo, loop);

  // 根据除法类型选择优化策略
  if (opType == StrengthReductionCandidate::DIVIDE) {
    bool isPowerOfTwo = (constantValue & (constantValue - 1)) == 0;
    
    if (isPowerOfTwo) {
      // 2的幂除法
      if (candidate->hasNegativeValues) {
        candidate->divStrategy = StrengthReductionCandidate::SIGNED_CORRECTION;
        if (DEBUG) {
          std::cout << "        Division by power of 2 with potential negative values, using signed correction" << std::endl;
        }
      } else {
        candidate->divStrategy = StrengthReductionCandidate::SIMPLE_SHIFT;
        if (DEBUG) {
          std::cout << "        Division by power of 2 with non-negative values, using simple shift" << std::endl;
        }
      }
    } else {
      // 任意常数除法，使用mulh指令
      candidate->operationType = StrengthReductionCandidate::DIVIDE_CONST;
      candidate->divStrategy = StrengthReductionCandidate::MULH_OPTIMIZATION;
      if (DEBUG) {
        std::cout << "        Division by arbitrary constant, using mulh optimization" << std::endl;
      }
    }
  } else if (opType == StrengthReductionCandidate::REMAINDER) {
    // 取模运算只支持2的幂
    if ((constantValue & (constantValue - 1)) != 0) {
      return nullptr; // 不是2的幂，无法优化
    }
  }

  return candidate;
}

const InductionVarInfo* 
StrengthReductionContext::getInductionVarInfo(Value* val, Loop* loop, 
                                            const LoopCharacteristics* characteristics) {
  for (const auto& iv : characteristics->InductionVars) {
    if (iv->div == val) {
      return iv.get();
    }
  }
  return nullptr;
}

void StrengthReductionContext::analyzeOptimizationPotential() {
  if (DEBUG) {
    std::cout << "  === Phase 2: Analyzing Optimization Potential ===" << std::endl;
  }

  // 为每个候选项计算优化收益，并过滤不值得优化的
  auto it = candidates.begin();
  while (it != candidates.end()) {
    StrengthReductionCandidate* candidate = it->get();
    
    double benefit = estimateOptimizationBenefit(candidate);
    bool isLegal = isOptimizationLegal(candidate);
    
    if (DEBUG) {
      std::cout << "    Candidate " << candidate->originalInst->getName() 
                << ": benefit=" << benefit 
                << ", legal=" << (isLegal ? "yes" : "no") << std::endl;
    }
    
    // 如果收益太小或不合法，移除候选项
    if (benefit < 1.0 || !isLegal) {
      // 从 loopToCandidates 中移除
      auto& loopCandidates = loopToCandidates[candidate->containingLoop];
      loopCandidates.erase(
        std::remove(loopCandidates.begin(), loopCandidates.end(), candidate),
        loopCandidates.end()
      );
      
      it = candidates.erase(it);
    } else {
      ++it;
    }
  }

  if (DEBUG) {
    std::cout << "  === End Phase 2: " << candidates.size() << " candidates remain ===" << std::endl;
  }
}

double StrengthReductionContext::estimateOptimizationBenefit(const StrengthReductionCandidate* candidate) {
  // 简单的收益估算模型
  double benefit = 0.0;
  
  // 基础收益：乘法变加法的性能提升
  benefit += 2.0; // 假设乘法比加法慢2倍
  
  // 乘数因子：乘数越大，收益越高
  if (candidate->multiplier >= 4) {
    benefit += 1.0;
  }
  if (candidate->multiplier >= 8) {
    benefit += 1.0;
  }
  
  // 循环热度因子
  Loop* loop = candidate->containingLoop;
  double hotness = loop->getLoopHotness();
  benefit *= (1.0 + hotness / 100.0);
  
  // 使用次数因子
  size_t useCount = candidate->originalInst->getUses().size();
  if (useCount > 1) {
    benefit *= (1.0 + useCount * 0.2);
  }
  
  return benefit;
}

bool StrengthReductionContext::isOptimizationLegal(const StrengthReductionCandidate* candidate) {
  // 检查优化的合法性
  
  // 1. 确保归纳变量在循环头有 phi 指令
  auto* phiInst = dynamic_cast<PhiInst*>(candidate->inductionVar);
  if (!phiInst || phiInst->getParent() != candidate->containingLoop->getHeader()) {
    if (DEBUG ) {
      std::cout << "      Illegal: induction variable is not a phi in loop header" << std::endl;
    }
    return false;
  }
  
  // 2. 确保乘法指令在循环内
  if (!candidate->containingLoop->contains(candidate->containingBlock)) {
    if (DEBUG ) {
      std::cout << "      Illegal: instruction not in loop" << std::endl;
    }
    return false;
  }
  
  // 3. 检查是否有溢出风险（简化检查）
  if (candidate->multiplier > 1000) {
    if (DEBUG ) {
      std::cout << "      Illegal: multiplier too large (overflow risk)" << std::endl;
    }
    return false;
  }
  
  // 4. 确保该指令不在循环的退出条件中（避免影响循环语义）
  for (BasicBlock* exitingBB : candidate->containingLoop->getExitingBlocks()) {
    auto terminatorIt = exitingBB->terminator();
    if (terminatorIt != exitingBB->end()) {
      Instruction* terminator = terminatorIt->get();
      if (terminator && (terminator->getOperand(0) == candidate->originalInst ||
                        (terminator->getNumOperands() > 1 && terminator->getOperand(1) == candidate->originalInst))) {
        if (DEBUG ) {
          std::cout << "      Illegal: instruction used in loop exit condition" << std::endl;
        }
        return false;
      }
    }
  }
  
  return true;
}

bool StrengthReductionContext::performStrengthReduction() {
  if (DEBUG) {
    std::cout << "  === Phase 3: Performing Strength Reduction ===" << std::endl;
  }

  bool modified = false;

  for (auto& candidate : candidates) {
    if (DEBUG) {
      std::cout << "    Processing candidate: " << candidate->originalInst->getName() << std::endl;
    }

    // 创建新的归纳变量
    if (!createNewInductionVariable(candidate.get())) {
      if (DEBUG) {
        std::cout << "      Failed to create new induction variable" << std::endl;
      }
      continue;
    }

    // 替换原始指令
    if (!replaceOriginalInstruction(candidate.get())) {
      if (DEBUG) {
        std::cout << "      Failed to replace original instruction" << std::endl;
      }
      continue;
    }

    if (DEBUG) {
      std::cout << "      Successfully optimized: " << candidate->originalInst->getName() 
                << " -> " << candidate->newInductionVar->getName() << std::endl;
    }
    
    modified = true;
  }

  if (DEBUG) {
    std::cout << "  === End Phase 3: " << (modified ? "Optimizations applied" : "No optimizations") << " ===" << std::endl;
  }

  return modified;
}

bool StrengthReductionContext::createNewInductionVariable(StrengthReductionCandidate* candidate) {
  // 只为乘法创建新的归纳变量
  // 除法和取模直接在替换时进行强度削弱，不需要新的归纳变量
  if (candidate->operationType != StrengthReductionCandidate::MULTIPLY) {
    candidate->newInductionVar = candidate->inductionVar; // 直接使用原归纳变量
    return true;
  }

  Loop* loop = candidate->containingLoop;
  BasicBlock* header = loop->getHeader();
  BasicBlock* preheader = loop->getPreHeader();
  
  if (!preheader) {
    if (DEBUG) {
      std::cout << "        No preheader found for loop" << std::endl;
    }
    return false;
  }

  // 获取原始归纳变量的 phi 指令
  auto* originalPhi = dynamic_cast<PhiInst*>(candidate->inductionVar);
  if (!originalPhi) {
    return false;
  }

  

  // 1. 找到原始归纳变量的初始值和步长
  Value* initialValue = nullptr;
  Value* stepValue = nullptr;
  BasicBlock* latchBlock = nullptr;

  for (auto& [incomingBB, incomingVal] : originalPhi->getIncomingValues()) {
    if (!loop->contains(incomingBB)) {
      // 来自循环外的初始值
      initialValue = incomingVal;
    } else {
      // 来自循环内的递增值
      latchBlock = incomingBB;
      // 尝试找到步长
      if (auto* addInst = dynamic_cast<BinaryInst*>(incomingVal)) {
        if (addInst->getKind() == Instruction::Kind::kAdd) {
          if (addInst->getOperand(0) == originalPhi) {
            stepValue = addInst->getOperand(1);
          } else if (addInst->getOperand(1) == originalPhi) {
            stepValue = addInst->getOperand(0);
          }
        }
      }
    }
  }

  if (!initialValue || !stepValue || !latchBlock) {
    if (DEBUG) {
      std::cout << "        Failed to find initial value, step, or latch block" << std::endl;
    }
    return false;
  }

  // 2. 在循环头创建新的 phi 指令
  builder->setPosition(header, header->begin());
  candidate->newPhi = builder->createPhiInst(originalPhi->getType());
  candidate->newPhi->setName("sr_" + originalPhi->getName());

  // 3. 计算新归纳变量的初始值和步长
  // 新IV的初始值 = 原IV初始值 * multiplier
  Value* newInitialValue;
  if (auto* constInt = dynamic_cast<ConstantInteger*>(initialValue)) {
    newInitialValue = ConstantInteger::get(constInt->getInt() * candidate->multiplier);
  } else {
    // 如果初始值不是常数，需要在preheader中插入乘法
    builder->setPosition(preheader, preheader->terminator());
    newInitialValue = builder->createMulInst(initialValue, 
      ConstantInteger::get(candidate->multiplier));
  }

  // 新IV的步长 = 原IV步长 * multiplier  
  Value* newStepValue;
  if (auto* constInt = dynamic_cast<ConstantInteger*>(stepValue)) {
    newStepValue = ConstantInteger::get(constInt->getInt() * candidate->multiplier);
  } else {
    builder->setPosition(latchBlock, latchBlock->terminator());
    newStepValue = builder->createMulInst(stepValue, 
      ConstantInteger::get(candidate->multiplier));
  }

  // 4. 创建新归纳变量的递增指令
  builder->setPosition(latchBlock, latchBlock->terminator());
  Value* newIncrementedValue = builder->createAddInst(candidate->newPhi, newStepValue);
  
  // 5. 设置新 phi 的输入值
  candidate->newPhi->addIncoming(newInitialValue, preheader);
  candidate->newPhi->addIncoming(newIncrementedValue, latchBlock);

  candidate->newInductionVar = candidate->newPhi;

  if (DEBUG) {
    std::cout << "        Created new induction variable: " << candidate->newPhi->getName() << std::endl;
  }

  return true;
}

bool StrengthReductionContext::replaceOriginalInstruction(StrengthReductionCandidate* candidate) {
  if (!candidate->newInductionVar) {
    return false;
  }

  Value* replacementValue = nullptr;
  
  // 根据操作类型生成不同的替换指令
  switch (candidate->operationType) {
    case StrengthReductionCandidate::MULTIPLY: {
      // 乘法：直接使用新的归纳变量
      replacementValue = candidate->newInductionVar;
      break;
    }
    
    case StrengthReductionCandidate::DIVIDE: {
      // 根据除法策略生成不同的代码
      builder->setPosition(candidate->containingBlock, 
                          candidate->containingBlock->findInstIterator(candidate->originalInst));
      replacementValue = generateDivisionReplacement(candidate, builder);
      break;
    }
    
    case StrengthReductionCandidate::DIVIDE_CONST: {
      // 任意常数除法
      // builder->setPosition(candidate->containingBlock, 
      //                     candidate->containingBlock->findInstIterator(candidate->originalInst));
      // replacementValue = generateConstantDivisionReplacement(candidate, builder);
      break;
    }
    
    case StrengthReductionCandidate::REMAINDER: {
      // 取模：使用位与操作 (x % 2^n == x & (2^n - 1))
      builder->setPosition(candidate->containingBlock, 
                          candidate->containingBlock->findInstIterator(candidate->originalInst));
      
      int maskValue = candidate->multiplier - 1; // 2^n - 1
      Value* maskConstant = ConstantInteger::get(maskValue);
      
      if (candidate->hasNegativeValues) {
        // 处理负数的取模运算
        Value* temp = builder->createBinaryInst(
          Instruction::Kind::kAnd, candidate->inductionVar->getType(),
          candidate->inductionVar, maskConstant
        );
        
        // 检查原值是否为负数
        Value* shift31condidata = builder->createBinaryInst(
          Instruction::Kind::kSra, candidate->inductionVar->getType(),
          candidate->inductionVar, ConstantInteger::get(31)
        );
        
        // 如果为负数，需要调整结果
        Value* adjustment = builder->createAndInst(shift31condidata, maskConstant);
        Value* adjustedTemp = builder->createAddInst(candidate->inductionVar, adjustment);
        Value* adjustedResult = builder->createBinaryInst(
          Instruction::Kind::kAnd, candidate->inductionVar->getType(),
          adjustedTemp, maskConstant
        );
        replacementValue = adjustedResult;
      } else {
        // 非负数的取模，直接使用位与
        replacementValue = builder->createBinaryInst(
          Instruction::Kind::kAnd, candidate->inductionVar->getType(),
          candidate->inductionVar, maskConstant
        );
      }
      
      if (DEBUG) {
        std::cout << "        Created modulus operation with mask " << maskValue 
                  << " (handles negatives: " << (candidate->hasNegativeValues ? "yes" : "no") << ")" << std::endl;
      }
      break;
    }
    
    default:
      return false;
  }

  if (!replacementValue) {
    return false;
  }

  // 处理偏移量
  if (candidate->offset != 0) {
    builder->setPosition(candidate->containingBlock, 
                        candidate->containingBlock->findInstIterator(candidate->originalInst));
    replacementValue = builder->createAddInst(
      replacementValue,
      ConstantInteger::get(candidate->offset)
    );
  }

  // 替换所有使用
  candidate->originalInst->replaceAllUsesWith(replacementValue);

  // 从基本块中移除原始指令
  auto* bb = candidate->originalInst->getParent();
  auto it = bb->findInstIterator(candidate->originalInst);
  if (it != bb->end()) {
    SysYIROptUtils::usedelete(it);
    // bb->getInstructions().erase(it);
  }

  if (DEBUG) {
    std::cout << "        Replaced and removed original " 
              << (candidate->operationType == StrengthReductionCandidate::MULTIPLY ? "multiply" :
                  candidate->operationType == StrengthReductionCandidate::DIVIDE ? "divide" : "remainder")
              << " instruction" << std::endl;
  }

  return true;
}

void StrengthReductionContext::printDebugInfo() {
  if (!DEBUG) return;

  std::cout << "\n=== Strength Reduction Optimization Summary ===" << std::endl;
  std::cout << "Total candidates processed: " << candidates.size() << std::endl;
  
  for (auto& [loop, loopCandidates] : loopToCandidates) {
    if (!loopCandidates.empty()) {
      std::cout << "Loop " << loop->getName() << ": " << loopCandidates.size() << " optimizations" << std::endl;
      for (auto* candidate : loopCandidates) {
        if (candidate->newInductionVar) {
          std::cout << "  " << candidate->inductionVar->getName() 
                    << " (op=" << (candidate->operationType == StrengthReductionCandidate::MULTIPLY ? "mul" :
                                   candidate->operationType == StrengthReductionCandidate::DIVIDE ? "div" : "rem")
                    << ", factor=" << candidate->multiplier << ")"
                    << " -> optimized" << std::endl;
        }
      }
    }
  }
  std::cout << "===============================================" << std::endl;
}

Value* StrengthReductionContext::generateDivisionReplacement(
  StrengthReductionCandidate* candidate, 
  IRBuilder* builder
) const {
  switch (candidate->divStrategy) {
    case StrengthReductionCandidate::SIMPLE_SHIFT: {
      // 简单的右移除法 (仅适用于非负数)
      int shiftAmount = __builtin_ctz(candidate->multiplier);
      Value* shiftConstant = ConstantInteger::get(shiftAmount);
      return builder->createBinaryInst(
        Instruction::Kind::kSrl,  // 逻辑右移
        candidate->inductionVar->getType(),
        candidate->inductionVar,
        shiftConstant
      );
    }
    
    case StrengthReductionCandidate::SIGNED_CORRECTION: {
      // 有符号除法校正：(x + (x >> 31) & mask) >> k
      int shiftAmount = __builtin_ctz(candidate->multiplier);
      int maskValue = candidate->multiplier - 1;
      
      // x >> 31 (算术右移获取符号位)
      Value* signShift = ConstantInteger::get(31);
      Value* signBits = builder->createBinaryInst(
        Instruction::Kind::kSra,  // 算术右移
        candidate->inductionVar->getType(),
        candidate->inductionVar,
        signShift
      );
      
      // (x >> 31) & mask
      Value* mask = ConstantInteger::get(maskValue);
      Value* correction = builder->createBinaryInst(
        Instruction::Kind::kAnd,
        candidate->inductionVar->getType(),
        signBits,
        mask
      );
      
      // x + correction
      Value* corrected = builder->createAddInst(candidate->inductionVar, correction);
      
      // (x + correction) >> k
      Value* divShift = ConstantInteger::get(shiftAmount);
      return builder->createBinaryInst(
        Instruction::Kind::kSra,  // 算术右移
        candidate->inductionVar->getType(),
        corrected,
        divShift
      );
    }
    
    default: {
      // 回退到原始除法
      Value* divisor = ConstantInteger::get(candidate->multiplier);
      return builder->createDivInst(candidate->inductionVar, divisor);
    }
  }
}

Value* StrengthReductionContext::generateConstantDivisionReplacement(
  StrengthReductionCandidate* candidate, 
  IRBuilder* builder
) const {
  // 使用mulh指令优化任意常数除法
  auto [magic, shift] = SysYIROptUtils::computeMulhMagicNumbers(candidate->multiplier);
  
  // 检查是否无法优化（magic == -1, shift == -1 表示失败）
  if (magic == -1 && shift == -1) {
    if (DEBUG) {
      std::cout << "[SR] Cannot optimize division by " << candidate->multiplier 
                << ", keeping original division" << std::endl;
    }
    // 返回 nullptr 表示无法优化，调用方应该保持原始除法
    return nullptr;
  }
  
  // 2的幂次方除法可以用移位优化（但这不是魔数法的情况）这种情况应该不会被分类到这里但是还是做一个保护措施
  if ((candidate->multiplier & (candidate->multiplier - 1)) == 0 && candidate->multiplier > 0) {
    // 是2的幂次方，可以用移位
    int shift_amount = 0;
    int temp = candidate->multiplier;
    while (temp > 1) {
      temp >>= 1;
      shift_amount++;
    }
    
    Value* shiftConstant = ConstantInteger::get(shift_amount);
    if (candidate->hasNegativeValues) {
      // 对于有符号除法，需要先加上除数-1然后再移位（为了正确处理负数舍入）
      Value* divisor_minus_1 = ConstantInteger::get(candidate->multiplier - 1);
      Value* adjusted = builder->createAddInst(candidate->inductionVar, divisor_minus_1);
      return builder->createBinaryInst(
        Instruction::Kind::kSra,  // 算术右移
        candidate->inductionVar->getType(),
        adjusted,
        shiftConstant
      );
    } else {
      return builder->createBinaryInst(
        Instruction::Kind::kSrl,  // 逻辑右移
        candidate->inductionVar->getType(),
        candidate->inductionVar,
        shiftConstant
      );
    }
  }
  
  // 创建魔数常量
  // 检查魔数是否能放入32位，如果不能，则不进行优化
  if (magic > INT32_MAX || magic < INT32_MIN) {
    if (DEBUG) {
      std::cout << "[SR] Magic number " << magic << " exceeds 32-bit range, skipping optimization" << std::endl;
    }
    return nullptr; // 无法优化，保持原始除法
  }
  
  Value* magicConstant = ConstantInteger::get((int32_t)magic);
  
  // 检查是否需要ADD_MARKER处理（加法调整）
  bool needAdd = (shift & 0x40) != 0;
  int actualShift = shift & 0x3F; // 提取真实的移位量
  
  if (DEBUG) {
    std::cout << "[SR] IR Generation: magic=" << magic << ", needAdd=" << needAdd 
              << ", actualShift=" << actualShift << std::endl;
  }
  
  // 执行高位乘法：mulh(x, magic)
  Value* mulhResult = builder->createBinaryInst(
    Instruction::Kind::kMulh,  // 高位乘法
    candidate->inductionVar->getType(),
    candidate->inductionVar,
    magicConstant
  );
  
  if (needAdd) {
    // ADD_MARKER 情况：需要在移位前加上被除数
    // 这对应于 libdivide 的加法调整算法
    if (DEBUG) {
      std::cout << "[SR] Applying ADD_MARKER: adding dividend before shift" << std::endl;
    }
    mulhResult = builder->createAddInst(mulhResult, candidate->inductionVar);
  }
  
  if (actualShift > 0) {
    // 如果需要额外移位
    Value* shiftConstant = ConstantInteger::get(actualShift);
    mulhResult = builder->createBinaryInst(
      Instruction::Kind::kSra,  // 算术右移
      candidate->inductionVar->getType(),
      mulhResult,
      shiftConstant
    );
  }
  
  // 标准的有符号除法符号修正：如果被除数为负，商需要加1
  // 这对所有有符号除法都需要，不管是否可能有负数
  Value* isNegative = builder->createICmpLTInst(candidate->inductionVar, ConstantInteger::get(0));
  // 将i1转换为i32：负数时为1，非负数时为0 ICmpLTInst的结果会默认转化为32位
  mulhResult = builder->createAddInst(mulhResult, isNegative);
  
  return mulhResult;
}

} // namespace sysy
