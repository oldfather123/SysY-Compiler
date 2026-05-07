#include "LoopCharacteristics.h"
#include "Dom.h"
#include "Loop.h"
#include "Liveness.h"
#include "AliasAnalysis.h"
#include "SideEffectAnalysis.h"
#include <iostream>
#include <cmath>

// 使用全局调试开关
extern int DEBUG;

namespace sysy {

// 定义 Pass 的唯一 ID
void *LoopCharacteristicsPass::ID = (void *)&LoopCharacteristicsPass::ID;

void LoopCharacteristicsResult::print() const {
  if (!DEBUG) return;

  std::cout << "\n--- Loop Characteristics Analysis Results for Function: " 
            << AssociatedFunction->getName() << " ---" << std::endl;

  if (CharacteristicsMap.empty()) {
    std::cout << "  No loop characteristics found." << std::endl;
    return;
  }

  // 打印统计信息
  auto stats = getOptimizationStats();
  std::cout << "\n=== Basic Loop Characteristics Statistics ===" << std::endl;
  std::cout << "Total Loops: " << stats.totalLoops << std::endl;
  std::cout << "Counting Loops: " << stats.countingLoops << std::endl;
  std::cout << "Unrolling Candidates: " << stats.unrollingCandidates << std::endl;
  std::cout << "Pure Loops: " << stats.pureLoops << std::endl;
  std::cout << "Local Memory Only Loops: " << stats.localMemoryOnlyLoops << std::endl;
  std::cout << "No Alias Conflict Loops: " << stats.noAliasConflictLoops << std::endl;
  std::cout << "Avg Instructions per Loop: " << stats.avgInstructionCount << std::endl;
  std::cout << "Avg Compute/Memory Ratio: " << stats.avgComputeMemoryRatio << std::endl;

  // 按热度排序并打印循环特征
  auto loopsByHotness = getLoopsByHotness();
  std::cout << "\n=== Loop Characteristics (by hotness) ===" << std::endl;
  
  for (auto* loop : loopsByHotness) {
    auto* chars = getCharacteristics(loop);
    if (!chars) continue;
    
    std::cout << "\n--- Loop: " << loop->getName() << " (Hotness: " 
              << loop->getLoopHotness() << ") ---" << std::endl;
    std::cout << "  Level: " << loop->getLoopLevel() << std::endl;
    std::cout << "  Blocks: " << loop->getLoopSize() << std::endl;
    std::cout << "  Instructions: " << chars->instructionCount << std::endl;
    std::cout << "  Memory Operations: " << chars->memoryOperationCount << std::endl;
    std::cout << "  Compute/Memory Ratio: " << chars->computeToMemoryRatio << std::endl;
    
    // 循环形式
    std::cout << "  Form: ";
    if (chars->isCountingLoop) std::cout << "Counting ";
    if (chars->isSimpleForLoop) std::cout << "SimpleFor ";
    if (chars->isInnermost) std::cout << "Innermost ";
    if (chars->hasComplexControlFlow) std::cout << "Complex ";
    if (chars->isPure) std::cout << "Pure ";
    if (chars->accessesOnlyLocalMemory) std::cout << "LocalMemOnly ";
    if (chars->hasNoMemoryAliasConflicts) std::cout << "NoAliasConflicts ";
    std::cout << std::endl;
    
    // 边界信息
    if (chars->staticTripCount.has_value()) {
      std::cout << "  Static Trip Count: " << *chars->staticTripCount << std::endl;
    }
    if (chars->hasKnownBounds) {
      std::cout << "  Has Known Bounds: Yes" << std::endl;
    }
    
    // 优化机会
    std::cout << "  Optimization Opportunities: ";
    if (chars->benefitsFromUnrolling) 
      std::cout << "Unroll(factor=" << chars->suggestedUnrollFactor << ") ";
    std::cout << std::endl;
    
    // 归纳变量
    if (!chars->InductionVars.empty()) {
      std::cout << "  Induction Vars: " << chars->InductionVars.size() << std::endl;
    }
    
    // 循环不变量
    if (!chars->loopInvariants.empty()) {
      std::cout << "  Loop Invariants: " << chars->loopInvariants.size() << std::endl;
    }
    if (!chars->invariantInsts.empty()) {
      std::cout << "  Hoistable Instructions: " << chars->invariantInsts.size() << std::endl;
    }
  }
  
  std::cout << "-----------------------------------------------" << std::endl;
}

bool LoopCharacteristicsPass::runOnFunction(Function *F, AnalysisManager &AM) {
  if (F->getBasicBlocks().empty()) {
    CurrentResult = std::make_unique<LoopCharacteristicsResult>(F);
    return false; // 空函数
  }

  if (DEBUG)
    std::cout << "Running LoopCharacteristicsPass on function: " << F->getName() << std::endl;

  // 获取并缓存所有需要的分析结果
  loopAnalysis = AM.getAnalysisResult<LoopAnalysisResult, LoopAnalysisPass>(F);
  if (!loopAnalysis) {
    std::cerr << "Error: LoopAnalysisResult not available for function " << F->getName() << std::endl;
    CurrentResult = std::make_unique<LoopCharacteristicsResult>(F);
    return false;
  }

  // 如果没有循环，直接返回
  if (!loopAnalysis->hasLoops()) {
    CurrentResult = std::make_unique<LoopCharacteristicsResult>(F);
    return false;
  }

  // 获取别名分析和副作用分析结果并缓存
  aliasAnalysis = AM.getAnalysisResult<AliasAnalysisResult, SysYAliasAnalysisPass>(F);
  sideEffectAnalysis = AM.getAnalysisResult<SideEffectAnalysisResult, SysYSideEffectAnalysisPass>();

  if (DEBUG) {
    if (aliasAnalysis) std::cout << "LoopCharacteristics: Using alias analysis results" << std::endl;
    if (sideEffectAnalysis) std::cout << "LoopCharacteristics: Using side effect analysis results" << std::endl;
  }

  CurrentResult = std::make_unique<LoopCharacteristicsResult>(F);

  // 分析每个循环的特征 - 现在不需要传递分析结果参数
  for (const auto& loop_ptr : loopAnalysis->getAllLoops()) {
    Loop* loop = loop_ptr.get();
    auto characteristics = std::make_unique<LoopCharacteristics>(loop);
    
    // 执行各种特征分析，使用缓存的分析结果
    analyzeLoop(loop, characteristics.get());
    
    // 添加到结果中
    CurrentResult->addLoopCharacteristics(std::move(characteristics));
  }

  if (DEBUG) {
    std::cout << "LoopCharacteristicsPass completed for function: " << F->getName() << std::endl;
    auto stats = CurrentResult->getOptimizationStats();
    std::cout << "Analyzed " << stats.totalLoops << " loops, found " 
              << stats.countingLoops << " counting loops, " 
              << stats.unrollingCandidates << " unroll candidates" << std::endl;
  }

  return false; // 特征分析不修改IR
}

void LoopCharacteristicsPass::analyzeLoop(Loop* loop, LoopCharacteristics* characteristics) {
  if (DEBUG)
    std::cout << "  Analyzing basic characteristics of loop: " << loop->getName() << std::endl;

  // 按顺序执行基础分析 - 现在使用缓存的分析结果
  computePerformanceMetrics(loop, characteristics);
  analyzeLoopForm(loop, characteristics);
  analyzePurityAndSideEffects(loop, characteristics);
  identifyBasicInductionVariables(loop, characteristics);
  identifyBasicLoopInvariants(loop, characteristics);
  analyzeBasicLoopBounds(loop, characteristics);
  analyzeBasicMemoryAccessPatterns(loop, characteristics);
  evaluateBasicOptimizationOpportunities(loop, characteristics);
}

void LoopCharacteristicsPass::computePerformanceMetrics(Loop* loop, LoopCharacteristics* characteristics) {
  size_t totalInsts = 0;
  size_t memoryOps = 0;
  size_t arithmeticOps = 0;

  // 遍历循环中的所有指令
  for (BasicBlock* bb : loop->getBlocks()) {
    for (auto& inst : bb->getInstructions()) {
      totalInsts++;
      
      // 分类指令类型
      if (dynamic_cast<LoadInst*>(inst.get()) || dynamic_cast<StoreInst*>(inst.get())) {
        memoryOps++;
      } else if (dynamic_cast<BinaryInst*>(inst.get())) {
        // 检查是否为算术运算
        auto* binInst = dynamic_cast<BinaryInst*>(inst.get());
        // 简化：假设所有二元运算都是算术运算
        arithmeticOps++;
      }
    }
  }

  characteristics->instructionCount = totalInsts;
  characteristics->memoryOperationCount = memoryOps;
  characteristics->arithmeticOperationCount = arithmeticOps;
  
  // 计算计算与内存操作比率
  if (memoryOps > 0) {
    characteristics->computeToMemoryRatio = static_cast<double>(arithmeticOps) / memoryOps;
  } else {
    characteristics->computeToMemoryRatio = arithmeticOps; // 纯计算循环
  }
}

void LoopCharacteristicsPass::analyzeLoopForm(Loop* loop, LoopCharacteristics* characteristics) {
  // 基本形式判断
  characteristics->isInnermost = loop->isInnermost();
  
  // 检查是否为简单循环 (只有一个回边)
  bool isSimple = loop->isSimpleLoop();
  characteristics->isSimpleForLoop = isSimple;
  
  // 检查复杂控制流 (多个出口表示可能有break/continue)
  auto exitingBlocks = loop->getExitingBlocks();
  characteristics->hasComplexControlFlow = exitingBlocks.size() > 1;
  
  // 初步判断是否为计数循环 (需要更复杂的分析)
  // 简化版本：如果是简单循环且是最内层，很可能是计数循环
  characteristics->isCountingLoop = isSimple && loop->isInnermost() && exitingBlocks.size() == 1;
}

void LoopCharacteristicsPass::analyzePurityAndSideEffects(Loop* loop, LoopCharacteristics* characteristics) {
  if (!sideEffectAnalysis) {
    // 没有副作用分析结果，保守处理
    characteristics->isPure = false;
    return;
  }
  
  // 检查循环是否有副作用
  characteristics->isPure = !loop->mayHaveSideEffects(sideEffectAnalysis);
  
  if (DEBUG && characteristics->isPure) {
    std::cout << "    Loop " << loop->getName() << " is identified as PURE (no side effects)" << std::endl;
  }
}

void LoopCharacteristicsPass::analyzeBasicMemoryAccessPatterns(Loop* loop, LoopCharacteristics* characteristics) {
  if (!aliasAnalysis) {
    // 没有别名分析结果，保守处理
    characteristics->accessesOnlyLocalMemory = false;
    characteristics->hasNoMemoryAliasConflicts = false;
    return;
  }
  
  // 检查是否只访问局部内存
  characteristics->accessesOnlyLocalMemory = !loop->accessesGlobalMemory(aliasAnalysis);
  
  // 检查是否有内存别名冲突
  characteristics->hasNoMemoryAliasConflicts = !loop->hasMemoryAliasConflicts(aliasAnalysis);
  
  if (DEBUG) {
    if (characteristics->accessesOnlyLocalMemory) {
      std::cout << "    Loop " << loop->getName() << " accesses ONLY LOCAL MEMORY" << std::endl;
    }
    if (characteristics->hasNoMemoryAliasConflicts) {
      std::cout << "    Loop " << loop->getName() << " has NO MEMORY ALIAS CONFLICTS" << std::endl;
    }
  }
  
  // 分析基础的内存访问模式
  for (BasicBlock* bb : loop->getBlocks()) {
    for (auto& inst : bb->getInstructions()) {
      if (auto* loadInst = dynamic_cast<LoadInst*>(inst.get())) {
        Value* ptr = loadInst->getPointer();
        
        auto& pattern = characteristics->memoryPatterns[ptr];
        pattern.loadInsts.push_back(loadInst);
        pattern.isArrayParameter = aliasAnalysis->isFunctionParameter(ptr);
        pattern.isGlobalArray = aliasAnalysis->isGlobalArray(ptr);
        pattern.hasConstantIndices = aliasAnalysis->hasConstantAccess(ptr);
        
      } else if (auto* storeInst = dynamic_cast<StoreInst*>(inst.get())) {
        Value* ptr = storeInst->getPointer();
        
        auto& pattern = characteristics->memoryPatterns[ptr];
        pattern.storeInsts.push_back(storeInst);
        pattern.isArrayParameter = aliasAnalysis->isFunctionParameter(ptr);
        pattern.isGlobalArray = aliasAnalysis->isGlobalArray(ptr);
        pattern.hasConstantIndices = aliasAnalysis->hasConstantAccess(ptr);
      }
    }
  }
}

bool LoopCharacteristicsPass::isBasicInductionVariable(Value* val, Loop* loop) {
  // 简化的基础归纳变量检测
  auto* phiInst = dynamic_cast<PhiInst*>(val);
  if (!phiInst) return false;
  
  // 检查phi指令是否在循环头
  if (phiInst->getParent() != loop->getHeader()) return false;
  
  // 检查是否有来自循环内的更新
  for (auto& [incomingBB, incomingVal] : phiInst->getIncomingValues()) {
    if (loop->contains(incomingBB)) {
      return true; // 简化：有来自循环内的值就认为是基础归纳变量
    }
  }
  
  return false;
}


void LoopCharacteristicsPass::identifyBasicInductionVariables(
    Loop* loop, LoopCharacteristics* characteristics) {
  BasicBlock* header = loop->getHeader();
  std::vector<std::unique_ptr<InductionVarInfo>> ivs;

  if (DEBUG) {
    std::cout << "    === Identifying Induction Variables for Loop: " << loop->getName() << " ===" << std::endl;
    std::cout << "    Loop header: " << header->getName() << std::endl;
    std::cout << "    Loop blocks: ";
    for (auto* bb : loop->getBlocks()) {
      std::cout << bb->getName() << " ";
    }
    std::cout << std::endl;
  }

  // 1. 识别所有BIV
  for (auto& inst : header->getInstructions()) {
    auto* phi = dynamic_cast<PhiInst*>(inst.get());
    if (!phi) continue;
    if (isBasicInductionVariable(phi, loop)) {
      ivs.push_back(InductionVarInfo::createBasicBIV(phi, Instruction::Kind::kPhi, phi));
      if (DEBUG) {
        std::cout << "    [BIV] Found basic induction variable: " << phi->getName() << std::endl;
        std::cout << "          Incoming values: ";
        for (auto& [incomingBB, incomingVal] : phi->getIncomingValues()) {
          std::cout << "{" << incomingBB->getName() << ": " << incomingVal->getName() << "} ";
        }
        std::cout << std::endl;
      }
    }
  }

  if (DEBUG) {
    std::cout << "    Found " << ivs.size() << " basic induction variables" << std::endl;
  }

  // 2. 递归识别所有派生DIV
  std::set<Value*> visited;
  size_t initialSize = ivs.size();
  
  // 保存初始的BIV列表，避免在遍历过程中修改向量导致迭代器失效
  std::vector<InductionVarInfo*> bivList;
  for (size_t i = 0; i < initialSize; ++i) {
    if (ivs[i] && ivs[i]->ivkind == IVKind::kBasic) {
      bivList.push_back(ivs[i].get());
    }
  }
  
  for (auto* biv : bivList) {
    if (DEBUG) {
      if (biv && biv->div) {
        std::cout << "    Searching for derived IVs from BIV: " << biv->div->getName() << std::endl;
      } else {
        std::cout << "    ERROR: Invalid BIV pointer or div field is null" << std::endl;
        continue;
      }
    }
    findDerivedInductionVars(biv->div, biv->base, loop, ivs, visited);
  }

  if (DEBUG) {
    size_t derivedCount = ivs.size() - initialSize;
    std::cout << "    Found " << derivedCount << " derived induction variables" << std::endl;
    
    // 打印所有归纳变量的详细信息
    std::cout << "    === Final Induction Variables Summary ===" << std::endl;
    for (size_t i = 0; i < ivs.size(); ++i) {
      const auto& iv = ivs[i];
      std::cout << "    [" << i << "] " << iv->div->getName() 
                << " (kind: " << (iv->ivkind == IVKind::kBasic ? "Basic" : 
                               iv->ivkind == IVKind::kLinear ? "Linear" : "Complex") << ")" << std::endl;
      std::cout << "        Operation: " << static_cast<int>(iv->Instkind) << std::endl;
      if (iv->base) {
        std::cout << "        Base: " << iv->base->getName() << std::endl;
      }
      if (iv->Multibase.first || iv->Multibase.second) {
        std::cout << "        Multi-base: ";
        if (iv->Multibase.first) std::cout << iv->Multibase.first->getName() << " ";
        if (iv->Multibase.second) std::cout << iv->Multibase.second->getName() << " ";
        std::cout << std::endl;
      }
      std::cout << "        Factor: " << iv->factor << ", Offset: " << iv->offset << std::endl;
      std::cout << "        Valid: " << (iv->valid ? "Yes" : "No") << std::endl;
    }
    std::cout << "    =============================================" << std::endl;
  }

  characteristics->InductionVars = std::move(ivs);
}


struct LinearExpr {
  // 表达为: a * base1 + b * base2 + offset
  Value* base1 = nullptr;
  Value* base2 = nullptr;
  int factor1 = 0;
  int factor2 = 0;
  int offset = 0;
  bool valid = false;
  bool isSimple = false; // 仅一个BIV时true
};

static LinearExpr analyzeLinearExpr(Value* val, Loop* loop, std::vector<std::unique_ptr<InductionVarInfo>>& ivs) {
  // 递归归约val为线性表达式
  // 只支持单/双BIV线性组合
  // 见下方详细实现
  // ----------
  if (DEBUG >= 2) {  // 更详细的调试级别
    if (auto* inst = dynamic_cast<Instruction*>(val)) {
      std::cout << "        Analyzing linear expression for: " << val->getName() 
                << " (kind: " << static_cast<int>(inst->getKind()) << ")" << std::endl;
    } else {
      std::cout << "        Analyzing linear expression for value: " << val->getName() << std::endl;
    }
  }
  
  // 基本变量：常数
  if (auto* cint = dynamic_cast<ConstantInteger*>(val)) {
    if (DEBUG >= 2) {
      std::cout << "        -> Constant: " << cint->getInt() << std::endl;
    }
    return {nullptr, nullptr, 0, 0, cint->getInt(), true, false};
  }
  
  // 基本变量：BIV或派生IV
  for (auto& iv : ivs) {
    if (iv->div == val) {
      if (iv->ivkind == IVKind::kBasic ||
          iv->ivkind == IVKind::kLinear) {
        if (DEBUG >= 2) {
          std::cout << "        -> Found " << (iv->ivkind == IVKind::kBasic ? "Basic" : "Linear") 
                    << " IV with base: " << (iv->base ? iv->base->getName() : "null") 
                    << ", factor: " << iv->factor << ", offset: " << iv->offset << std::endl;
        }
        return {iv->base, nullptr, iv->factor, 0, iv->offset, true, true};
      }
      // 复杂归纳变量
      if (iv->ivkind == IVKind::kCmplx) {
        if (DEBUG >= 2) {
          std::cout << "        -> Found Complex IV with multi-base" << std::endl;
        }
        return {iv->Multibase.first, iv->Multibase.second, 1, 1, 0, true, false};
      }
    }
  }
  
  // 一元负号
  if (auto* inst = dynamic_cast<Instruction*>(val)) {
    auto kind = inst->getKind();
    if (kind == Instruction::Kind::kNeg) {
      if (DEBUG >= 2) {
        std::cout << "        -> Analyzing negation" << std::endl;
      }
      auto expr = analyzeLinearExpr(inst->getOperand(0), loop, ivs);
      if (!expr.valid) return expr;
      expr.factor1 = -expr.factor1;
      expr.factor2 = -expr.factor2;
      expr.offset = -expr.offset;
      expr.isSimple = (expr.base2 == nullptr);
      if (DEBUG >= 2) {
        std::cout << "        -> Negation result: valid=" << expr.valid << ", simple=" << expr.isSimple << std::endl;
      }
      return expr;
    }
    
    // 二元加减乘
    if (kind == Instruction::Kind::kAdd || kind == Instruction::Kind::kSub) {
      if (DEBUG >= 2) {
        std::cout << "        -> Analyzing " << (kind == Instruction::Kind::kAdd ? "addition" : "subtraction") << std::endl;
      }
      auto expr0 = analyzeLinearExpr(inst->getOperand(0), loop, ivs);
      auto expr1 = analyzeLinearExpr(inst->getOperand(1), loop, ivs);
      if (!expr0.valid || !expr1.valid) {
        if (DEBUG >= 2) {
          std::cout << "        -> Failed: operand not linear (expr0.valid=" << expr0.valid << ", expr1.valid=" << expr1.valid << ")" << std::endl;
        }
        return {nullptr, nullptr, 0, 0, 0, false, false};
      }
      
      // 合并：若BIV相同或有一个是常数
      // 单BIV+常数
      if (expr0.base1 && !expr1.base1 && !expr1.base2) {
        int sign = (kind == Instruction::Kind::kAdd ? 1 : -1);
        if (DEBUG >= 2) {
          std::cout << "        -> Single BIV + constant pattern" << std::endl;
        }
        return {expr0.base1, nullptr, expr0.factor1, 0, expr0.offset + sign * expr1.offset, true, expr0.isSimple};
      }
      if (!expr0.base1 && !expr0.base2 && expr1.base1) {
        int sign = (kind == Instruction::Kind::kAdd ? 1 : -1);
        int f = sign * expr1.factor1;
        int off = expr0.offset + sign * expr1.offset;
        if (DEBUG >= 2) {
          std::cout << "        -> Constant + single BIV pattern" << std::endl;
        }
        return {expr1.base1, nullptr, f, 0, off, true, expr1.isSimple};
      }
      
      // 双BIV线性组合
      if (expr0.base1 && expr1.base1 && expr0.base1 != expr1.base1 && !expr0.base2 && !expr1.base2) {
        int sign = (kind == Instruction::Kind::kAdd ? 1 : -1);
        Value* base1 = expr0.base1;
        Value* base2 = expr1.base1;
        int f1 = expr0.factor1;
        int f2 = sign * expr1.factor1;
        int off = expr0.offset + sign * expr1.offset;
        if (DEBUG >= 2) {
          std::cout << "        -> Double BIV linear combination" << std::endl;
        }
        return {base1, base2, f1, f2, off, true, false};
      }
      
      // 同BIV合并
      if (expr0.base1 && expr1.base1 && expr0.base1 == expr1.base1 && !expr0.base2 && !expr1.base2) {
        int sign = (kind == Instruction::Kind::kAdd ? 1 : -1);
        int f = expr0.factor1 + sign * expr1.factor1;
        int off = expr0.offset + sign * expr1.offset;
        if (DEBUG >= 2) {
          std::cout << "        -> Same BIV combination" << std::endl;
        }
        return {expr0.base1, nullptr, f, 0, off, true, true};
      }
    }
    
    // 乘法：BIV*const 或 const*BIV
    if (kind == Instruction::Kind::kMul) {
      if (DEBUG >= 2) {
        std::cout << "        -> Analyzing multiplication" << std::endl;
      }
      auto expr0 = analyzeLinearExpr(inst->getOperand(0), loop, ivs);
      auto expr1 = analyzeLinearExpr(inst->getOperand(1), loop, ivs);
      
      // 只允许一侧为常数
      if (expr0.base1 && !expr1.base1 && !expr1.base2 && expr1.offset) {
        if (DEBUG >= 2) {
          std::cout << "        -> BIV * constant pattern" << std::endl;
        }
        return {expr0.base1, nullptr, expr0.factor1 * expr1.offset, 0, expr0.offset * expr1.offset, true, true};
      }
      if (!expr0.base1 && !expr0.base2 && expr0.offset && expr1.base1) {
        if (DEBUG >= 2) {
          std::cout << "        -> Constant * BIV pattern" << std::endl;
        }
        return {expr1.base1, nullptr, expr1.factor1 * expr0.offset, 0, expr1.offset * expr0.offset, true, true};
      }
      // 双BIV乘法不支持
      if (DEBUG >= 2) {
        std::cout << "        -> Multiplication pattern not supported" << std::endl;
      }
    }
    
    // 除法：BIV/const（仅当const是2的幂时）
    if (kind == Instruction::Kind::kDiv) {
      if (DEBUG >= 2) {
        std::cout << "        -> Analyzing division" << std::endl;
      }
      auto expr0 = analyzeLinearExpr(inst->getOperand(0), loop, ivs);
      auto expr1 = analyzeLinearExpr(inst->getOperand(1), loop, ivs);
      
      // 只支持 BIV / 2^n 形式
      if (expr0.base1 && !expr1.base1 && !expr1.base2 && expr1.offset > 0) {
        // 检查是否为2的幂
        int divisor = expr1.offset;
        if ((divisor & (divisor - 1)) == 0) { // 2的幂检查
          if (DEBUG >= 2) {
            std::cout << "        -> BIV / power_of_2 pattern (divisor=" << divisor << ")" << std::endl;
          }
          // 对于除法，我们记录为特殊的归纳变量模式
          // factor表示除数（用于后续强度削弱）
          return {expr0.base1, nullptr, -divisor, 0, expr0.offset / divisor, true, true};
        }
      }
      if (DEBUG >= 2) {
        std::cout << "        -> Division pattern not supported (not power of 2)" << std::endl;
      }
    }
    
    // 取模：BIV % const（仅当const是2的幂时）
    if (kind == Instruction::Kind::kRem) {
      if (DEBUG >= 2) {
        std::cout << "        -> Analyzing remainder" << std::endl;
      }
      auto expr0 = analyzeLinearExpr(inst->getOperand(0), loop, ivs);
      auto expr1 = analyzeLinearExpr(inst->getOperand(1), loop, ivs);
      
      // 只支持 BIV % 2^n 形式
      if (expr0.base1 && !expr1.base1 && !expr1.base2 && expr1.offset > 0) {
        // 检查是否为2的幂
        int modulus = expr1.offset;
        if ((modulus & (modulus - 1)) == 0) { // 2的幂检查
          if (DEBUG >= 2) {
            std::cout << "        -> BIV % power_of_2 pattern (modulus=" << modulus << ")" << std::endl;
          }
          // 对于取模，我们记录为特殊的归纳变量模式
          // 使用负的模数来区分取模和除法
          return {expr0.base1, nullptr, -10000 - modulus, 0, 0, true, true}; // 特殊标记
        }
      }
      if (DEBUG >= 2) {
        std::cout << "        -> Remainder pattern not supported (not power of 2)" << std::endl;
      }
    }
  }
  
  // 其它情况
  if (DEBUG >= 2) {
    std::cout << "        -> Other case: not linear" << std::endl;
  }
  return {nullptr, nullptr, 0, 0, 0, false, false};
}

void LoopCharacteristicsPass::identifyBasicLoopInvariants(Loop* loop, LoopCharacteristics* characteristics) {
  // 经典推进法：反复遍历，直到收敛 TODO：优化
  bool changed;
  std::unordered_set<Value*> invariants = characteristics->loopInvariants; // 可能为空

  do {
    changed = false;
    for (BasicBlock* bb : loop->getBlocks()) {
      for (auto& inst : bb->getInstructions()) {
        Instruction* I = inst.get();
        // 跳过phi和terminator
        if (dynamic_cast<PhiInst*>(I)) continue;
        if (I->isTerminator()) continue;
        if (invariants.count(I)) continue;

        if (isClassicLoopInvariant(I, loop, invariants)) {
          invariants.insert(I);
          characteristics->invariantInsts.insert(I);
          if (DEBUG)
            std::cout << "    Found loop invariant: " << I->getName() << std::endl;
          changed = true;
        }
      }
    }
  } while (changed);

  characteristics->loopInvariants = std::move(invariants);
}

void LoopCharacteristicsPass::analyzeBasicLoopBounds(Loop* loop, LoopCharacteristics* characteristics) {
  // 简化的基础边界分析
  // 检查是否有静态可确定的循环次数（简化版本）
  if (characteristics->isCountingLoop && !characteristics->InductionVars.empty()) {
    // 简化：如果是计数循环且有基本归纳变量，尝试确定循环次数
    if (characteristics->instructionCount < 10) {
      characteristics->staticTripCount = 100; // 简化估计
      characteristics->hasKnownBounds = true;
      
      if (DEBUG) {
        std::cout << "    Estimated static trip count: " << *characteristics->staticTripCount << std::endl;
      }
    }
  }
}

void LoopCharacteristicsPass::evaluateBasicOptimizationOpportunities(Loop* loop, LoopCharacteristics* characteristics) {
  // 评估基础循环展开机会
  characteristics->benefitsFromUnrolling = 
    characteristics->isInnermost && 
    characteristics->instructionCount > 3 && 
    characteristics->instructionCount < 50 &&
    !characteristics->hasComplexControlFlow;
    
  if (characteristics->benefitsFromUnrolling) {
    // 基于循环体大小估算展开因子
    if (characteristics->instructionCount <= 5) characteristics->suggestedUnrollFactor = 8;
    else if (characteristics->instructionCount <= 10) characteristics->suggestedUnrollFactor = 4;
    else if (characteristics->instructionCount <= 20) characteristics->suggestedUnrollFactor = 2;
    else characteristics->suggestedUnrollFactor = 1;
  }
    
  if (DEBUG) {
    if (characteristics->benefitsFromUnrolling) {
      std::cout << "    Loop " << loop->getName() << " benefits from UNROLLING (factor=" 
                << characteristics->suggestedUnrollFactor << ")" << std::endl;
    }
  }
}

// ========== 辅助方法实现 ==========

// 递归识别DIV，支持线性与复杂归纳变量
void LoopCharacteristicsPass::findDerivedInductionVars(
    Value* root,
    Value* base, // 只传单一BIV base
    Loop* loop,
    std::vector<std::unique_ptr<InductionVarInfo>>& ivs,
    std::set<Value*>& visited)
{
  if (visited.count(root)) return;
  visited.insert(root);

  if (DEBUG) {
    std::cout << "      Analyzing uses of: " << root->getName() << std::endl;
  }

  for (auto use : root->getUses()) {
    auto user = use->getUser();
    Instruction* inst = dynamic_cast<Instruction*>(user);
    if (!inst) continue;
    if (!loop->contains(inst->getParent())) {
      if (DEBUG) {
        std::cout << "      Skipping user outside loop: " << inst->getName() << std::endl;
      }
      continue;
    }

    if (DEBUG) {
      std::cout << "      Checking instruction: " << inst->getName() 
                << " (kind: " << static_cast<int>(inst->getKind()) << ")" << std::endl;
    }

    // 线性归约分析
    auto expr = analyzeLinearExpr(inst, loop, ivs);

    if (!expr.valid) {
      if (DEBUG) {
        std::cout << "      Linear expression analysis failed for: " << inst->getName() << std::endl;
      }
      // 复杂非线性归纳变量，作为kCmplx记录（假如你想追踪）
      // 这里假设expr.base1、base2都有效才记录double
      if (expr.base1 && expr.base2) {
        if (DEBUG) {
          std::cout << "      [DIV-COMPLEX] Creating complex derived IV: " << inst->getName() 
                    << " with bases: " << expr.base1->getName() << ", " << expr.base2->getName() << std::endl;
        }
        ivs.push_back(InductionVarInfo::createDoubleDIV(inst, inst->getKind(), expr.base1, expr.base2, 0, expr.offset));
      }
      continue;
    }

    // 单BIV线性
    if (expr.base1 && !expr.base2) {
      // 检查这个指令是否已经是一个已知的IV（特别是BIV），避免重复创建
      bool alreadyExists = false;
      for (const auto& existingIV : ivs) {
        if (existingIV->div == inst) {
          alreadyExists = true;
          if (DEBUG) {
            std::cout << "      [DIV-SKIP] Instruction " << inst->getName() 
                      << " already exists as IV, skipping creation" << std::endl;
          }
          break;
        }
      }
      
      if (!alreadyExists) {
        if (DEBUG) {
          std::cout << "      [DIV-LINEAR] Creating single-base derived IV: " << inst->getName() 
                    << " with base: " << expr.base1->getName() 
                    << ", factor: " << expr.factor1 
                    << ", offset: " << expr.offset << std::endl;
        }
        ivs.push_back(InductionVarInfo::createSingleDIV(inst, inst->getKind(), expr.base1, expr.factor1, expr.offset));
        findDerivedInductionVars(inst, expr.base1, loop, ivs, visited);
      }
    }
    // 双BIV线性
    else if (expr.base1 && expr.base2) {
      if (DEBUG) {
        std::cout << "      [DIV-COMPLEX] Creating double-base derived IV: " << inst->getName() 
                  << " with bases: " << expr.base1->getName() << ", " << expr.base2->getName() 
                  << ", offset: " << expr.offset << std::endl;
      }
      ivs.push_back(InductionVarInfo::createDoubleDIV(inst, inst->getKind(), expr.base1, expr.base2, 0, expr.offset));
      // 双BIV情形一般不再递归下游
    }
  }

  if (DEBUG) {
    std::cout << "      Finished analyzing uses of: " << root->getName() << std::endl;
  }
}

// 检查操作数是否都是不变量
bool LoopCharacteristicsPass::isInvariantOperands(Instruction* inst, Loop* loop, const std::unordered_set<Value*>& invariants) {
  for (size_t i = 0; i < inst->getNumOperands(); ++i) {
    Value* op = inst->getOperand(i);
    if (!isClassicLoopInvariant(op, loop, invariants) && !invariants.count(op)) {
      return false;
    }
  }
  return true;
}

// 检查内存位置是否在循环中被修改
bool LoopCharacteristicsPass::isMemoryLocationModifiedInLoop(Value* ptr, Loop* loop) {
  // 遍历循环中的所有指令，检查是否有对该内存位置的写入
  for (BasicBlock* bb : loop->getBlocks()) {
    for (auto& inst : bb->getInstructions()) {
      // 1. 检查直接的Store指令
      if (auto* storeInst = dynamic_cast<StoreInst*>(inst.get())) {
        Value* storeTar = storeInst->getPointer();
        
        // 使用别名分析检查是否可能别名
        if (aliasAnalysis) {
          auto aliasType = aliasAnalysis->queryAlias(ptr, storeTar);
          if (aliasType != AliasType::NO_ALIAS) {
            if (DEBUG) {
              std::cout << "      Memory location " << ptr->getName() 
                        << " may be modified by store to " << storeTar->getName() << std::endl;
            }
            return true;
          }
        } else {
          // 如果没有别名分析，保守处理 - 只检查精确匹配
          if (ptr == storeTar) {
            return true;
          }
        }
      }
      
      // 2. 检查函数调用是否可能修改该内存位置
      else if (auto* callInst = dynamic_cast<CallInst*>(inst.get())) {
        Function* calledFunc = callInst->getCallee();
        
        // 如果是纯函数，不会修改内存
        if (isPureFunction(calledFunc)) {
          continue;
        }
        
        // 检查函数参数中是否有该内存位置的指针
        for (size_t i = 1; i < callInst->getNumOperands(); ++i) { // 跳过函数指针
          Value* arg = callInst->getOperand(i);
          
          // 检查参数是否是指针类型且可能指向该内存位置
          if (auto* ptrType = dynamic_cast<PointerType*>(arg->getType())) {
            // 使用别名分析检查
            if (aliasAnalysis) {
              auto aliasType = aliasAnalysis->queryAlias(ptr, arg);
              if (aliasType != AliasType::NO_ALIAS) {
                if (DEBUG) {
                  std::cout << "      Memory location " << ptr->getName() 
                            << " may be modified by function call " << calledFunc->getName()
                            << " through parameter " << arg->getName() << std::endl;
                }
                return true;
              }
            } else {
              // 没有别名分析，检查精确匹配
              if (ptr == arg) {
                if (DEBUG) {
                  std::cout << "      Memory location " << ptr->getName() 
                            << " may be modified by function call " << calledFunc->getName()
                            << " (exact match)" << std::endl;
                }
                return true;
              }
            }
          }
        }
      }
    }
  }
  return false;
}

// 检查内存位置是否在循环中被读取
bool LoopCharacteristicsPass::isMemoryLocationLoadedInLoop(Value* ptr, Loop* loop, Instruction* excludeInst) {
  // 遍历循环中的所有Load指令，检查是否有对该内存位置的读取
  for (BasicBlock* bb : loop->getBlocks()) {
    for (auto& inst : bb->getInstructions()) {
      if (inst.get() == excludeInst) continue; // 排除当前指令本身
      
      if (auto* loadInst = dynamic_cast<LoadInst*>(inst.get())) {
        Value* loadSrc = loadInst->getPointer();
        
        // 使用别名分析检查是否可能别名
        if (aliasAnalysis) {
          auto aliasType = aliasAnalysis->queryAlias(ptr, loadSrc);
          if (aliasType != AliasType::NO_ALIAS) {
            return true;
          }
        } else {
          // 如果没有别名分析，保守处理 - 只检查精确匹配
          if (ptr == loadSrc) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

// 检查函数调用是否为纯函数
bool LoopCharacteristicsPass::isPureFunction(Function* calledFunc) {
  if (!calledFunc) return false;
  
  // 使用副作用分析检查函数是否为纯函数
  if (sideEffectAnalysis && sideEffectAnalysis->isPureFunction(calledFunc)) {
    return true;
  }
  
  // 检查是否为内置纯函数（如数学函数）
  std::string funcName = calledFunc->getName();
  static const std::set<std::string> pureFunctions = {
    "abs", "fabs", "sqrt", "sin", "cos", "tan", "exp", "log", "pow",
    "floor", "ceil", "round", "min", "max"
  };
  
  return pureFunctions.count(funcName) > 0;
}

// 递归/推进式判定 - 完善版本
bool LoopCharacteristicsPass::isClassicLoopInvariant(Value* val, Loop* loop, const std::unordered_set<Value*>& invariants) {
  if (DEBUG >= 2) {
    std::cout << "    Checking loop invariant for: " << val->getName() << std::endl;
  }
  
  // 1. 常量
  if (auto* constval = dynamic_cast<ConstantValue*>(val)) {
    if (DEBUG >= 2) std::cout << "    -> Constant: YES" << std::endl;
    return true;
  }

  // 2. 参数（函数参数）通常不在任何BasicBlock内，直接判定为不变量
  // 在SSA形式下，参数不会被重新赋值
  if (auto* arg = dynamic_cast<Argument*>(val)) {
    if (DEBUG >= 2) std::cout << "    -> Function argument: YES" << std::endl;
    return true;
  }

  // 3. 指令且定义在循环外
  if (auto* inst = dynamic_cast<Instruction*>(val)) {
    if (!loop->contains(inst->getParent())) {
      if (DEBUG >= 2) std::cout << "    -> Defined outside loop: YES" << std::endl;
      return true;
    }

    // 4. 跳转指令、phi指令不能外提
    if (inst->isTerminator() || inst->isPhi()) {
      if (DEBUG >= 2) std::cout << "    -> Terminator or PHI: NO" << std::endl;
      return false;
    }

    // 5. 根据指令类型进行具体分析
    switch (inst->getKind()) {
      case Instruction::Kind::kStore: {
        // Store指令：检查循环内是否有对该内存的load
        auto* storeInst = dynamic_cast<StoreInst*>(inst);
        Value* storePtr = storeInst->getPointer();
        
        // 首先检查操作数是否不变
        if (!isInvariantOperands(inst, loop, invariants)) {
          if (DEBUG >= 2) std::cout << "    -> Store: operands not invariant: NO" << std::endl;
          return false;
        }
        
        // 检查是否有对该内存位置的load
        if (isMemoryLocationLoadedInLoop(storePtr, loop, inst)) {
          if (DEBUG >= 2) std::cout << "    -> Store: memory location loaded in loop: NO" << std::endl;
          return false;
        }
        
        if (DEBUG >= 2) std::cout << "    -> Store: safe to hoist: YES" << std::endl;
        return true;
      }
      
      case Instruction::Kind::kLoad: {
        // Load指令：检查循环内是否有对该内存的store
        auto* loadInst = dynamic_cast<LoadInst*>(inst);
        Value* loadPtr = loadInst->getPointer();
        
        // 首先检查指针操作数是否不变
        if (!isInvariantOperands(inst, loop, invariants)) {
          if (DEBUG >= 2) std::cout << "    -> Load: pointer not invariant: NO" << std::endl;
          return false;
        }
        
        // 检查是否有对该内存位置的store
        if (isMemoryLocationModifiedInLoop(loadPtr, loop)) {
          if (DEBUG >= 2) std::cout << "    -> Load: memory location modified in loop: NO" << std::endl;
          return false;
        }
        
        if (DEBUG >= 2) std::cout << "    -> Load: safe to hoist: YES" << std::endl;
        return true;
      }
      
      case Instruction::Kind::kCall: {
        // Call指令：检查是否为纯函数且参数不变
        auto* callInst = dynamic_cast<CallInst*>(inst);
        Function* calledFunc = callInst->getCallee();
        
        // 检查是否为纯函数
        if (!isPureFunction(calledFunc)) {
          if (DEBUG >= 2) std::cout << "    -> Call: not pure function: NO" << std::endl;
          return false;
        }
        
        // 检查参数是否都不变
        if (!isInvariantOperands(inst, loop, invariants)) {
          if (DEBUG >= 2) std::cout << "    -> Call: arguments not invariant: NO" << std::endl;
          return false;
        }
        
        if (DEBUG >= 2) std::cout << "    -> Call: pure function with invariant args: YES" << std::endl;
        return true;
      }
      
      case Instruction::Kind::kGetElementPtr: {
        // GEP指令：检查基址和索引是否都不变
        if (!isInvariantOperands(inst, loop, invariants)) {
          if (DEBUG >= 2) std::cout << "    -> GEP: base or indices not invariant: NO" << std::endl;
          return false;
        }
        
        if (DEBUG >= 2) std::cout << "    -> GEP: base and indices invariant: YES" << std::endl;
        return true;
      }
      
      // 一元运算指令
      case Instruction::Kind::kNeg:
      case Instruction::Kind::kNot:
      case Instruction::Kind::kFNeg:
      case Instruction::Kind::kFNot:
      case Instruction::Kind::kFtoI:
      case Instruction::Kind::kItoF:
      case Instruction::Kind::kBitItoF:
      case Instruction::Kind::kBitFtoI: {
        // 检查操作数是否不变
        if (!isInvariantOperands(inst, loop, invariants)) {
          if (DEBUG >= 2) std::cout << "    -> Unary op: operand not invariant: NO" << std::endl;
          return false;
        }
        
        if (DEBUG >= 2) std::cout << "    -> Unary op: operand invariant: YES" << std::endl;
        return true;
      }
      
      // 二元运算指令
      case Instruction::Kind::kAdd:
      case Instruction::Kind::kSub:
      case Instruction::Kind::kMul:
      case Instruction::Kind::kDiv:
      case Instruction::Kind::kRem:
      case Instruction::Kind::kSll:
      case Instruction::Kind::kSrl:
      case Instruction::Kind::kSra:
      case Instruction::Kind::kAnd:
      case Instruction::Kind::kOr:
      case Instruction::Kind::kFAdd:
      case Instruction::Kind::kFSub:
      case Instruction::Kind::kFMul:
      case Instruction::Kind::kFDiv:
      case Instruction::Kind::kICmpEQ:
      case Instruction::Kind::kICmpNE:
      case Instruction::Kind::kICmpLT:
      case Instruction::Kind::kICmpGT:
      case Instruction::Kind::kICmpLE:
      case Instruction::Kind::kICmpGE:
      case Instruction::Kind::kFCmpEQ:
      case Instruction::Kind::kFCmpNE:
      case Instruction::Kind::kFCmpLT:
      case Instruction::Kind::kFCmpGT:
      case Instruction::Kind::kFCmpLE:
      case Instruction::Kind::kFCmpGE:
      case Instruction::Kind::kMulh: {
        // 检查所有操作数是否不变
        if (!isInvariantOperands(inst, loop, invariants)) {
          if (DEBUG >= 2) std::cout << "    -> Binary op: operands not invariant: NO" << std::endl;
          return false;
        }
        
        if (DEBUG >= 2) std::cout << "    -> Binary op: operands invariant: YES" << std::endl;
        return true;
      }
      
      default: {
        // 其他指令：使用副作用分析
        if (sideEffectAnalysis && sideEffectAnalysis->hasSideEffect(inst)) {
          if (DEBUG >= 2) std::cout << "    -> Other inst: has side effect: NO" << std::endl;
          return false;
        }
        
        // 检查操作数是否都不变
        if (!isInvariantOperands(inst, loop, invariants)) {
          if (DEBUG >= 2) std::cout << "    -> Other inst: operands not invariant: NO" << std::endl;
          return false;
        }
        
        if (DEBUG >= 2) std::cout << "    -> Other inst: no side effect, operands invariant: YES" << std::endl;
        return true;
      }
    }
  }
  
  // 其它情况
  if (DEBUG >= 2) std::cout << "    -> Other value type: NO" << std::endl;
  return false;
}


} // namespace sysy
