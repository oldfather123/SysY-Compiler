#pragma once

#include "PassManager.hpp"
#include "Instruction.hpp"
#include "BasicBlock.hpp"
#include "Function.hpp"
#include "Constant.hpp"

#include <unordered_map>
#include <vector>

class GEPFolding : public Pass {
  public:
    GEPFolding(Module *m) : Pass(m) {}

    void run() override;

  private:
    int folded_count{0}; // 计数器
    
    // 核心方法
    bool runOnFunction(Function *func);
    bool runOnBasicBlock(BasicBlock *bb);
    bool foldGEPChains(BasicBlock *bb);
    bool allIndicesAreZero(GetElementPtrInst *gep);
    bool canSimplifyGEP(GetElementPtrInst *gep);
    Instruction *simplifyGEP(GetElementPtrInst *gep, BasicBlock *bb);
    
    // GEP折叠方法
    bool canFoldGEP(GetElementPtrInst *gep);
    Constant *foldGEP(GetElementPtrInst *gep);
    Constant *calculateGEPOffset(Type *base_type, const std::vector<Constant*> &indices);
    
    // 辅助方法
    bool allIndicesAreConstant(GetElementPtrInst *gep);
    size_t getTypeSize(Type *type);
    size_t getArrayElementSize(ArrayType *array_type);
    Constant *createConstantGEP(Value *base, const std::vector<Constant*> &indices);
    
    // 冗余GEP消除
    bool eliminateRedundantGEPs(BasicBlock *bb);
    bool areGEPsEquivalent(GetElementPtrInst *gep1, GetElementPtrInst *gep2);
};
