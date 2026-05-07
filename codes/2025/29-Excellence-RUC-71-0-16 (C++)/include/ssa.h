#pragma once

#include "block.h"
#include "llvm_instruction.h"
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class SSAOptimizer {
public:
  LLVMIR optimize(const LLVMIR &ssa_ir);

private:
  // 除法优化
  bool isPowerOfTwo(int n);
  int log2_upper(int x);
  std::tuple<long long, int, int> choose_multiplier(int d, int prec);
  void optimizeDivision(LLVMIR &ir);

  // add删除
  bool isRegAndEqual(Operand op, int reg_no);
  bool isUsedInInstruction(Instruction ins, int reg_no);
  void removeRedundantAdd(LLVMIR &ir);

  // 删除冗余的alloca和store
  void removeRedundantParamAlloca(LLVMIR &ir);
  void replaceOperands(Instruction ins,
                       const std::unordered_map<int, int> &remap);
  Operand replaceOperand(Operand op, const std::unordered_map<int, int> &remap);

  void removeRedundantParamLoadStore(LLVMIR &ir);
  void replaceOperands(Instruction ins,
                       const std::unordered_map<int, Operand> &replace_map);
  Operand replaceOperand(Operand op,
                         const std::unordered_map<int, Operand> &replace_map);

  void delayAllocaInstructions(LLVMIR &ir);
};
