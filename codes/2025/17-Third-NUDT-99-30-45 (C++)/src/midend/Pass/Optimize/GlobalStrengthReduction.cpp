#include "GlobalStrengthReduction.h"
#include "SysYIROptUtils.h"
#include "IRBuilder.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <cmath>

extern int DEBUG;

namespace sysy {

// 全局强度削弱优化遍的静态 ID
void *GlobalStrengthReduction::ID = (void *)&GlobalStrengthReduction::ID;

// ======================================================================
// GlobalStrengthReduction 类的实现
// ======================================================================

bool GlobalStrengthReduction::runOnFunction(Function *func, AnalysisManager &AM) {
  if (func->getBasicBlocks().empty()) {
    return false;
  }

  if (DEBUG) {
    std::cout << "\n=== Running GlobalStrengthReduction on function: " << func->getName() << " ===" << std::endl;
  }

  bool changed = false;
  GlobalStrengthReductionContext context(builder);
  context.run(func, &AM, changed);

  if (DEBUG) {
    if (changed) {
      std::cout << "GlobalStrengthReduction: Function " << func->getName() << " was modified" << std::endl;
    } else {
      std::cout << "GlobalStrengthReduction: Function " << func->getName() << " was not modified" << std::endl;
    }
    std::cout << "=== GlobalStrengthReduction completed for function: " << func->getName() << " ===" << std::endl;
  }

  return changed;
}

void GlobalStrengthReduction::getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const {
  // 强度削弱依赖副作用分析来判断指令是否可以安全优化
  analysisDependencies.insert(&SysYSideEffectAnalysisPass::ID);
  
  // 强度削弱不会使分析失效，因为：
  // - 只替换计算指令，不改变控制流
  // - 不修改内存，不影响别名分析
  // - 保持程序语义不变
  // analysisInvalidations 保持为空
  
  if (DEBUG) {
    std::cout << "GlobalStrengthReduction: Declared analysis dependencies (SideEffectAnalysis)" << std::endl;
  }
}

// ======================================================================
// GlobalStrengthReductionContext 类的实现
// ======================================================================

void GlobalStrengthReductionContext::run(Function *func, AnalysisManager *AM, bool &changed) {
  if (DEBUG) {
    std::cout << "  Starting GlobalStrengthReduction analysis for function: " << func->getName() << std::endl;
  }

  // 获取分析结果
  if (AM) {
    sideEffectAnalysis = AM->getAnalysisResult<SideEffectAnalysisResult, SysYSideEffectAnalysisPass>();
    
    if (DEBUG) {
      if (sideEffectAnalysis) {
        std::cout << "    GlobalStrengthReduction: Using side effect analysis" << std::endl;
      } else {
        std::cout << "    GlobalStrengthReduction: Warning - side effect analysis not available" << std::endl;
      }
    }
  }

  // 重置计数器
  algebraicOptCount = 0;
  strengthReductionCount = 0;
  divisionOptCount = 0;

  // 遍历所有基本块进行优化
  for (auto &bb_ptr : func->getBasicBlocks()) {
    if (processBasicBlock(bb_ptr.get())) {
      changed = true;
    }
  }

  if (DEBUG) {
    std::cout << "  GlobalStrengthReduction completed for function: " << func->getName() << std::endl;
    std::cout << "    Algebraic optimizations: " << algebraicOptCount << std::endl;
    std::cout << "    Strength reductions: " << strengthReductionCount << std::endl;
    std::cout << "    Division optimizations: " << divisionOptCount << std::endl;
  }
}

bool GlobalStrengthReductionContext::processBasicBlock(BasicBlock *bb) {
  bool changed = false;
  
  if (DEBUG) {
    std::cout << "    Processing block: " << bb->getName() << std::endl;
  }

  // 收集需要处理的指令（避免迭代器失效）
  std::vector<Instruction*> instructions;
  for (auto &inst_ptr : bb->getInstructions()) {
    instructions.push_back(inst_ptr.get());
  }

  // 处理每条指令
  for (auto inst : instructions) {
    if (processInstruction(inst)) {
      changed = true;
    }
  }

  return changed;
}

bool GlobalStrengthReductionContext::processInstruction(Instruction *inst) {

  if (DEBUG) {
    std::cout << "      Processing instruction: " << inst->getName() << std::endl;
  }

  // 先尝试代数优化
  if (tryAlgebraicOptimization(inst)) {
    algebraicOptCount++;
    return true;
  }

  // 再尝试强度削弱
  if (tryStrengthReduction(inst)) {
    strengthReductionCount++;
    return true;
  }

  return false;
}

// ======================================================================
// 代数优化方法
// ======================================================================

bool GlobalStrengthReductionContext::tryAlgebraicOptimization(Instruction *inst) {
  auto binary = dynamic_cast<BinaryInst*>(inst);
  if (!binary) {
    return false;
  }

  switch (binary->getKind()) {
    case Instruction::kAdd:
      return optimizeAddition(binary);
    case Instruction::kSub:
      return optimizeSubtraction(binary);
    case Instruction::kMul:
      return optimizeMultiplication(binary);
    case Instruction::kDiv:
      return optimizeDivision(binary);
    case Instruction::kICmpEQ:
    case Instruction::kICmpNE:
    case Instruction::kICmpLT:
    case Instruction::kICmpGT:
    case Instruction::kICmpLE:
    case Instruction::kICmpGE:
      return optimizeComparison(binary);
    case Instruction::kAnd:
    case Instruction::kOr:
      return optimizeLogical(binary);
    default:
      return false;
  }
}

bool GlobalStrengthReductionContext::optimizeAddition(BinaryInst *inst) {
  Value *lhs = inst->getLhs();
  Value *rhs = inst->getRhs();
  int constVal;

  // x + 0 = x
  if (isConstantInt(rhs, constVal) && constVal == 0) {
    if (DEBUG) {
      std::cout << "        Algebraic: " << inst->getName() << " = x + 0 -> x" << std::endl;
    }
    replaceWithOptimized(inst, lhs);
    return true;
  }
  
  // 0 + x = x
  if (isConstantInt(lhs, constVal) && constVal == 0) {
    if (DEBUG) {
      std::cout << "        Algebraic: " << inst->getName() << " = 0 + x -> x" << std::endl;
    }
    replaceWithOptimized(inst, rhs);
    return true;
  }

  // x + (-y) = x - y
  if (auto rhsInst = dynamic_cast<UnaryInst*>(rhs)) {
    if (rhsInst->getKind() == Instruction::kNeg) {
      if (DEBUG) {
        std::cout << "        Algebraic: " << inst->getName() << " = x + (-y) -> x - y" << std::endl;
      }
      // 创建减法指令
      builder->setPosition(inst->getParent(), inst->getParent()->findInstIterator(inst));
      auto subInst = builder->createSubInst(lhs, rhsInst->getOperand());
      replaceWithOptimized(inst, subInst);
      return true;
    }
  }

  return false;
}

bool GlobalStrengthReductionContext::optimizeSubtraction(BinaryInst *inst) {
  Value *lhs = inst->getLhs();
  Value *rhs = inst->getRhs();
  int constVal;

  // x - 0 = x
  if (isConstantInt(rhs, constVal) && constVal == 0) {
    if (DEBUG) {
      std::cout << "        Algebraic: " << inst->getName() << " = x - 0 -> x" << std::endl;
    }
    replaceWithOptimized(inst, lhs);
    return true;
  }

  // x - x = 0 (如果x没有副作用)
  if (lhs == rhs && hasOnlyLocalUses(dynamic_cast<Instruction*>(lhs))) {
    if (DEBUG) {
      std::cout << "        Algebraic: " << inst->getName() << " = x - x -> 0" << std::endl;
    }
    replaceWithOptimized(inst, getConstantInt(0));
    return true;
  }

  // x - (-y) = x + y
  if (auto rhsInst = dynamic_cast<UnaryInst*>(rhs)) {
    if (rhsInst->getKind() == Instruction::kNeg) {
      if (DEBUG) {
        std::cout << "        Algebraic: " << inst->getName() << " = x - (-y) -> x + y" << std::endl;
      }
      builder->setPosition(inst->getParent(), inst->getParent()->findInstIterator(inst));
      auto addInst = builder->createAddInst(lhs, rhsInst->getOperand());
      replaceWithOptimized(inst, addInst);
      return true;
    }
  }

  return false;
}

bool GlobalStrengthReductionContext::optimizeMultiplication(BinaryInst *inst) {
  Value *lhs = inst->getLhs();
  Value *rhs = inst->getRhs();
  int constVal;

  // x * 0 = 0
  if (isConstantInt(rhs, constVal) && constVal == 0) {
    if (DEBUG) {
      std::cout << "        Algebraic: " << inst->getName() << " = x * 0 -> 0" << std::endl;
    }
    replaceWithOptimized(inst, getConstantInt(0));
    return true;
  }
  
  // 0 * x = 0
  if (isConstantInt(lhs, constVal) && constVal == 0) {
    if (DEBUG) {
      std::cout << "        Algebraic: " << inst->getName() << " = 0 * x -> 0" << std::endl;
    }
    replaceWithOptimized(inst, getConstantInt(0));
    return true;
  }

  // x * 1 = x
  if (isConstantInt(rhs, constVal) && constVal == 1) {
    if (DEBUG) {
      std::cout << "        Algebraic: " << inst->getName() << " = x * 1 -> x" << std::endl;
    }
    replaceWithOptimized(inst, lhs);
    return true;
  }
  
  // 1 * x = x
  if (isConstantInt(lhs, constVal) && constVal == 1) {
    if (DEBUG) {
      std::cout << "        Algebraic: " << inst->getName() << " = 1 * x -> x" << std::endl;
    }
    replaceWithOptimized(inst, rhs);
    return true;
  }

  // x * (-1) = -x
  if (isConstantInt(rhs, constVal) && constVal == -1) {
    if (DEBUG) {
      std::cout << "        Algebraic: " << inst->getName() << " = x * (-1) -> -x" << std::endl;
    }
    builder->setPosition(inst->getParent(), inst->getParent()->findInstIterator(inst));
    auto negInst = builder->createNegInst(lhs);
    replaceWithOptimized(inst, negInst);
    return true;
  }

  return false;
}

bool GlobalStrengthReductionContext::optimizeDivision(BinaryInst *inst) {
  Value *lhs = inst->getLhs();
  Value *rhs = inst->getRhs();
  int constVal;

  // x / 1 = x
  if (isConstantInt(rhs, constVal) && constVal == 1) {
    if (DEBUG) {
      std::cout << "        Algebraic: " << inst->getName() << " = x / 1 -> x" << std::endl;
    }
    replaceWithOptimized(inst, lhs);
    return true;
  }

  // x / (-1) = -x
  if (isConstantInt(rhs, constVal) && constVal == -1) {
    if (DEBUG) {
      std::cout << "        Algebraic: " << inst->getName() << " = x / (-1) -> -x" << std::endl;
    }
    builder->setPosition(inst->getParent(), inst->getParent()->findInstIterator(inst));
    auto negInst = builder->createNegInst(lhs);
    replaceWithOptimized(inst, negInst);
    return true;
  }

  // x / x = 1 (如果x != 0且没有副作用)
  if (lhs == rhs && hasOnlyLocalUses(dynamic_cast<Instruction*>(lhs))) {
    if (DEBUG) {
      std::cout << "        Algebraic: " << inst->getName() << " = x / x -> 1" << std::endl;
    }
    replaceWithOptimized(inst, getConstantInt(1));
    return true;
  }

  return false;
}

bool GlobalStrengthReductionContext::optimizeComparison(BinaryInst *inst) {
  Value *lhs = inst->getLhs();
  Value *rhs = inst->getRhs();

  // x == x = true (如果x没有副作用)
  if (inst->getKind() == Instruction::kICmpEQ && lhs == rhs && 
      hasOnlyLocalUses(dynamic_cast<Instruction*>(lhs))) {
    if (DEBUG) {
      std::cout << "        Algebraic: " << inst->getName() << " = x == x -> true" << std::endl;
    }
    replaceWithOptimized(inst, getConstantInt(1));
    return true;
  }

  // x != x = false (如果x没有副作用)
  if (inst->getKind() == Instruction::kICmpNE && lhs == rhs && 
      hasOnlyLocalUses(dynamic_cast<Instruction*>(lhs))) {
    if (DEBUG) {
      std::cout << "        Algebraic: " << inst->getName() << " = x != x -> false" << std::endl;
    }
    replaceWithOptimized(inst, getConstantInt(0));
    return true;
  }

  return false;
}

bool GlobalStrengthReductionContext::optimizeLogical(BinaryInst *inst) {
  Value *lhs = inst->getLhs();
  Value *rhs = inst->getRhs();
  int constVal;

  if (inst->getKind() == Instruction::kAnd) {
    // x && 0 = 0
    if (isConstantInt(rhs, constVal) && constVal == 0) {
      if (DEBUG) {
        std::cout << "        Algebraic: " << inst->getName() << " = x && 0 -> 0" << std::endl;
      }
      replaceWithOptimized(inst, getConstantInt(0));
      return true;
    }
    
    // x && -1 = x
    if (isConstantInt(rhs, constVal) && constVal == -1) {
      if (DEBUG) {
        std::cout << "        Algebraic: " << inst->getName() << " = x && 1 -> x" << std::endl;
      }
      replaceWithOptimized(inst, lhs);
      return true;
    }

    // x && x = x
    if (lhs == rhs) {
      if (DEBUG) {
        std::cout << "        Algebraic: " << inst->getName() << " = x && x -> x" << std::endl;
      }
      replaceWithOptimized(inst, lhs);
      return true;
    }
  } else if (inst->getKind() == Instruction::kOr) {
    // x || 0 = x
    if (isConstantInt(rhs, constVal) && constVal == 0) {
      if (DEBUG) {
        std::cout << "        Algebraic: " << inst->getName() << " = x || 0 -> x" << std::endl;
      }
      replaceWithOptimized(inst, lhs);
      return true;
    }

    // x || x = x
    if (lhs == rhs) {
      if (DEBUG) {
        std::cout << "        Algebraic: " << inst->getName() << " = x || x -> x" << std::endl;
      }
      replaceWithOptimized(inst, lhs);
      return true;
    }
  }

  return false;
}

// ======================================================================
// 强度削弱方法
// ======================================================================

bool GlobalStrengthReductionContext::tryStrengthReduction(Instruction *inst) {
  if (auto binary = dynamic_cast<BinaryInst*>(inst)) {
    switch (binary->getKind()) {
      case Instruction::kMul:
        return reduceMultiplication(binary);
      case Instruction::kDiv:
        return reduceDivision(binary);
      default:
        return false;
    }
  } else if (auto call = dynamic_cast<CallInst*>(inst)) {
    return reducePower(call);
  }

  return false;
}

bool GlobalStrengthReductionContext::reduceMultiplication(BinaryInst *inst) {
  Value *lhs = inst->getLhs();
  Value *rhs = inst->getRhs();
  int constVal;

  // 尝试右操作数为常数
  Value* variable = lhs;
  if (isConstantInt(rhs, constVal) && constVal > 0) {
    return tryComplexMultiplication(inst, variable, constVal);
  }
  
  // 尝试左操作数为常数
  if (isConstantInt(lhs, constVal) && constVal > 0) {
    variable = rhs;
    return tryComplexMultiplication(inst, variable, constVal);
  }

  return false;
}

bool GlobalStrengthReductionContext::tryComplexMultiplication(BinaryInst* inst, Value* variable, int constant) {
  // 首先检查是否为2的幂，使用简单位移
  if (isPowerOfTwo(constant)) {
    int shiftAmount = log2OfPowerOfTwo(constant);
    if (DEBUG) {
      std::cout << "        StrengthReduction: " << inst->getName() 
                << " = x * " << constant << " -> x << " << shiftAmount << std::endl;
    }
    
    builder->setPosition(inst->getParent(), inst->getParent()->findInstIterator(inst));
    auto shiftInst = builder->createBinaryInst(Instruction::kSll, Type::getIntType(), variable, getConstantInt(shiftAmount));
    replaceWithOptimized(inst, shiftInst);
    return true;
  }
  
  // 尝试分解为位移和加法的组合
  std::vector<int> shifts;
  if (findOptimalShiftDecomposition(constant, shifts)) {
    if (DEBUG) {
      std::cout << "        StrengthReduction: " << inst->getName() 
                << " = x * " << constant << " -> shift decomposition with " << shifts.size() << " terms" << std::endl;
    }
    
    Value* result = createShiftDecomposition(inst, variable, shifts);
    if (result) {
      replaceWithOptimized(inst, result);
      return true;
    }
  }
  
  return false;
}

bool GlobalStrengthReductionContext::findOptimalShiftDecomposition(int constant, std::vector<int>& shifts) {
  shifts.clear();
  
  // 常见的有效分解模式
  switch (constant) {
    case 3:   // 3 = 2^1 + 2^0 -> (x << 1) + x
      shifts = {1, 0};
      return true;
    case 5:   // 5 = 2^2 + 2^0 -> (x << 2) + x  
      shifts = {2, 0};
      return true;
    case 6:   // 6 = 2^2 + 2^1 -> (x << 2) + (x << 1)
      shifts = {2, 1};
      return true;
    case 7:   // 7 = 2^2 + 2^1 + 2^0 -> (x << 2) + (x << 1) + x
      shifts = {2, 1, 0};
      return true;
    case 9:   // 9 = 2^3 + 2^0 -> (x << 3) + x
      shifts = {3, 0};
      return true;
    case 10:  // 10 = 2^3 + 2^1 -> (x << 3) + (x << 1)
      shifts = {3, 1};
      return true;
    case 11:  // 11 = 2^3 + 2^1 + 2^0 -> (x << 3) + (x << 1) + x
      shifts = {3, 1, 0};
      return true;
    case 12:  // 12 = 2^3 + 2^2 -> (x << 3) + (x << 2)
      shifts = {3, 2};
      return true;
    case 13:  // 13 = 2^3 + 2^2 + 2^0 -> (x << 3) + (x << 2) + x
      shifts = {3, 2, 0};
      return true;
    case 14:  // 14 = 2^3 + 2^2 + 2^1 -> (x << 3) + (x << 2) + (x << 1)
      shifts = {3, 2, 1};
      return true;
    case 15:  // 15 = 2^3 + 2^2 + 2^1 + 2^0 -> (x << 3) + (x << 2) + (x << 1) + x
      shifts = {3, 2, 1, 0};
      return true;
    case 17:  // 17 = 2^4 + 2^0 -> (x << 4) + x
      shifts = {4, 0};
      return true;
    case 18:  // 18 = 2^4 + 2^1 -> (x << 4) + (x << 1)
      shifts = {4, 1};
      return true;
    case 20:  // 20 = 2^4 + 2^2 -> (x << 4) + (x << 2)
      shifts = {4, 2};
      return true;
    case 24:  // 24 = 2^4 + 2^3 -> (x << 4) + (x << 3)
      shifts = {4, 3};
      return true;
    case 25:  // 25 = 2^4 + 2^3 + 2^0 -> (x << 4) + (x << 3) + x
      shifts = {4, 3, 0};
      return true;
    case 100: // 100 = 2^6 + 2^5 + 2^2 -> (x << 6) + (x << 5) + (x << 2)
      shifts = {6, 5, 2};
      return true;
  }
  
  // 通用二进制分解（最多4个项，避免过度复杂化）
  if (constant > 0 && constant < 256) {
    std::vector<int> binaryShifts;
    int temp = constant;
    int bit = 0;
    
    while (temp > 0 && binaryShifts.size() < 4) {
      if (temp & 1) {
        binaryShifts.push_back(bit);
      }
      temp >>= 1;
      bit++;
    }
    
    // 只有当项数不超过3个时才使用二进制分解（比直接乘法更有效）
    if (binaryShifts.size() <= 3 && binaryShifts.size() >= 2) {
      shifts = binaryShifts;
      return true;
    }
  }
  
  return false;
}

Value* GlobalStrengthReductionContext::createShiftDecomposition(BinaryInst* inst, Value* variable, const std::vector<int>& shifts) {
  if (shifts.empty()) return nullptr;
  
  builder->setPosition(inst->getParent(), inst->getParent()->findInstIterator(inst));
  
  Value* result = nullptr;
  
  for (int shift : shifts) {
    Value* term;
    if (shift == 0) {
      // 0位移就是原变量
      term = variable;
    } else {
      // 创建位移指令
      term = builder->createBinaryInst(Instruction::kSll, Type::getIntType(), variable, getConstantInt(shift));
    }
    
    if (result == nullptr) {
      result = term;
    } else {
      // 累加到结果中
      result = builder->createAddInst(result, term);
    }
  }
  
  return result;
}

bool GlobalStrengthReductionContext::reduceDivision(BinaryInst *inst) {
  Value *lhs = inst->getLhs();
  Value *rhs = inst->getRhs();
  uint32_t constVal;

  // x / 2^n = x >> n (对于无符号除法或已知为正数的情况)
  if (isConstantInt(rhs, constVal) && constVal > 0 && isPowerOfTwo(constVal)) {
    builder->setPosition(inst->getParent(), inst->getParent()->findInstIterator(inst));
    int shiftAmount = log2OfPowerOfTwo(constVal);
    // 有符号除法校正：(x + (x >> 31) & mask) >> k
    int maskValue = constVal - 1;
    
    // x >> 31 (算术右移获取符号位)
    Value* signShift = ConstantInteger::get(31);
    Value* signBits = builder->createBinaryInst(
      Instruction::Kind::kSra,  // 算术右移
      lhs->getType(),
      lhs,
      signShift
    );
    
    // (x >> 31) & mask
    Value* mask = ConstantInteger::get(maskValue);
    Value* correction = builder->createBinaryInst(
      Instruction::Kind::kAnd,
      lhs->getType(),
      signBits,
      mask
    );
    
    // x + correction
    Value* corrected = builder->createAddInst(lhs, correction);
    
    // (x + correction) >> k
    Value* divShift = ConstantInteger::get(shiftAmount);
    Value* shiftInst = builder->createBinaryInst(
      Instruction::Kind::kSra,  // 算术右移
      lhs->getType(),
      corrected,
      divShift
    );

    if (DEBUG) {
      std::cout << "        StrengthReduction: " << inst->getName() 
                << " = x / " << constVal << " -> (x + (x >> 31) & mask) >> " << shiftAmount << std::endl;
    }
    
    // builder->setPosition(inst->getParent(), inst->getParent()->findInstIterator(inst));
    // Value* divisor_minus_1 = ConstantInteger::get(constVal - 1);
    // Value* adjusted = builder->createAddInst(lhs, divisor_minus_1);
    // Value* shiftInst = builder->createBinaryInst(Instruction::kSra, Type::getIntType(), adjusted, getConstantInt(shiftAmount));
    replaceWithOptimized(inst, shiftInst);
    strengthReductionCount++;
    return true;
  }

  // x / c = x * magic_number (魔数乘法优化 - 使用libdivide算法)
  // if (isConstantInt(rhs, constVal) && constVal > 1 && constVal != (uint32_t)(-1)) {
  //   // auto magicPair = computeMulhMagicNumbers(static_cast<int>(constVal));
  //   Value* magicResult = createMagicDivisionLibdivide(inst, static_cast<int>(constVal));
  //   replaceWithOptimized(inst, magicResult);
  //   divisionOptCount++;
  //   return true;
  // }

  return false;
}

bool GlobalStrengthReductionContext::reducePower(CallInst *inst) {
  // 检查是否是pow函数调用
  Function* callee = inst->getCallee();
  if (!callee || callee->getName() != "pow") {
    return false;
  }

  // pow(x, 2) = x * x
  if (inst->getNumOperands() >= 2) {
    int exponent;
    if (isConstantInt(inst->getOperand(1), exponent)) {
      if (exponent == 2) {
        if (DEBUG) {
          std::cout << "        StrengthReduction: pow(x, 2) -> x * x" << std::endl;
        }
        
        Value* base = inst->getOperand(0);
        builder->setPosition(inst->getParent(), inst->getParent()->findInstIterator(inst));
        auto mulInst = builder->createMulInst(base, base);
        replaceWithOptimized(inst, mulInst);
        strengthReductionCount++;
        return true;
      } else if (exponent >= 3 && exponent <= 8) {
        // 对于小的指数，展开为连续乘法
        if (DEBUG) {
          std::cout << "        StrengthReduction: pow(x, " << exponent << ") -> repeated multiplication" << std::endl;
        }
        
        Value* base = inst->getOperand(0);
        Value* result = base;
        builder->setPosition(inst->getParent(), inst->getParent()->findInstIterator(inst));
        
        for (int i = 1; i < exponent; i++) {
          result = builder->createMulInst(result, base);
        }
        
        replaceWithOptimized(inst, result);
        strengthReductionCount++;
        return true;
      }
    }
  }

  return false;
}

Value* GlobalStrengthReductionContext::createMagicDivisionLibdivide(BinaryInst* divInst, int divisor) {
  builder->setPosition(divInst->getParent(), divInst->getParent()->findInstIterator(divInst));
  // 使用mulh指令优化任意常数除法
  auto [magic, shift] = SysYIROptUtils::computeMulhMagicNumbers(divisor);
  
  // 检查是否无法优化（magic == -1, shift == -1 表示失败）
  if (magic == -1 && shift == -1) {
    if (DEBUG) {
      std::cout << "[SR] Cannot optimize division by " << divisor 
                << ", keeping original division" << std::endl;
    }
    // 返回 nullptr 表示无法优化，调用方应该保持原始除法
    return nullptr;
  }
  
  // 2的幂次方除法可以用移位优化（但这不是魔数法的情况）这种情况应该不会被分类到这里但是还是做一个保护措施
  if ((divisor & (divisor - 1)) == 0 && divisor > 0) {
    // 是2的幂次方，可以用移位
    int shift_amount = 0;
    int temp = divisor;
    while (temp > 1) {
      temp >>= 1;
      shift_amount++;
    }
    
    Value* shiftConstant = ConstantInteger::get(shift_amount);
    // 对于有符号除法，需要先加上除数-1然后再移位（为了正确处理负数舍入）
    Value* divisor_minus_1 = ConstantInteger::get(divisor - 1);
    Value* adjusted = builder->createAddInst(divInst->getOperand(0), divisor_minus_1);
    return builder->createBinaryInst(
      Instruction::Kind::kSra,  // 算术右移
      divInst->getOperand(0)->getType(),
      adjusted,
      shiftConstant
    );
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
    divInst->getOperand(0)->getType(),
    divInst->getOperand(0),
    magicConstant
  );
  
  if (needAdd) {
    // ADD_MARKER 情况：需要在移位前加上被除数
    // 这对应于 libdivide 的加法调整算法
    if (DEBUG) {
      std::cout << "[SR] Applying ADD_MARKER: adding dividend before shift" << std::endl;
    }
    mulhResult = builder->createAddInst(mulhResult, divInst->getOperand(0));
  }
  
  if (actualShift > 0) {
    // 如果需要额外移位
    Value* shiftConstant = ConstantInteger::get(actualShift);
    mulhResult = builder->createBinaryInst(
      Instruction::Kind::kSra,  // 算术右移
      divInst->getOperand(0)->getType(),
      mulhResult,
      shiftConstant
    );
  }
  
  // 标准的有符号除法符号修正：如果被除数为负，商需要加1
  // 这对所有有符号除法都需要，不管是否可能有负数
  Value* isNegative = builder->createICmpLTInst(divInst->getOperand(0), ConstantInteger::get(0));
  // 将i1转换为i32：负数时为1，非负数时为0 ICmpLTInst的结果会默认转化为32位
  mulhResult = builder->createAddInst(mulhResult, isNegative);
  
  return mulhResult; 
}

// ======================================================================
// 辅助方法
// ======================================================================

bool GlobalStrengthReductionContext::isPowerOfTwo(uint32_t n) {
  return n > 0 && (n & (n - 1)) == 0;
}

int GlobalStrengthReductionContext::log2OfPowerOfTwo(uint32_t n) {
  int result = 0;
  while (n > 1) {
    n >>= 1;
    result++;
  }
  return result;
}

bool GlobalStrengthReductionContext::isConstantInt(Value* val, int& constVal) {
  if (auto constInt = dynamic_cast<ConstantInteger*>(val)) {
    constVal = std::get<int>(constInt->getVal());
    return true;
  }
  return false;
}

bool GlobalStrengthReductionContext::isConstantInt(Value* val, uint32_t& constVal) {
  if (auto constInt = dynamic_cast<ConstantInteger*>(val)) {
    int signedVal = std::get<int>(constInt->getVal());
    if (signedVal >= 0) {
      constVal = static_cast<uint32_t>(signedVal);
      return true;
    }
  }
  return false;
}

ConstantInteger* GlobalStrengthReductionContext::getConstantInt(int val) {
  return ConstantInteger::get(val);
}

bool GlobalStrengthReductionContext::hasOnlyLocalUses(Instruction* inst) {
  if (!inst) return true;
  
  // 简单检查：如果指令没有副作用，则认为是本地的
  if (sideEffectAnalysis) {
    auto sideEffect = sideEffectAnalysis->getInstructionSideEffect(inst);
    return sideEffect.type == SideEffectType::NO_SIDE_EFFECT;
  }
  
  // 没有副作用分析时，保守处理
  return !inst->isCall() && !inst->isStore() && !inst->isLoad();
}

void GlobalStrengthReductionContext::replaceWithOptimized(Instruction* original, Value* replacement) {
  if (DEBUG) {
    std::cout << "          Replacing " << original->getName() 
              << " with " << replacement->getName() << std::endl;
  }
  
  original->replaceAllUsesWith(replacement);
  
  // 如果替换值是新创建的指令，确保它有合适的名字
//   if (auto replInst = dynamic_cast<Instruction*>(replacement)) {
//     if (replInst->getName().empty()) {
//       replInst->setName(original->getName() + "_opt");
//     }
//   }
  
  // 删除原指令，让调用者处理
  SysYIROptUtils::usedelete(original);
}

} // namespace sysy
