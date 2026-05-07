#pragma once

/**
 * @file MacroExpansion.cpp
 *
 * @brief 定义宏扩展的头文件
 *
 */

#include "../backend/Mid2End.h"
#include "../frontend/IR.h"

namespace macroExpansion {
/**
 * @brief 宏扩展指令选择器
 *
 *
 */
class MacroExpansion : public mid2end::InstSelection {
 private:
  riscv::RiscvBuilder builder;                                                         ///< 构建器
  std::unique_ptr<riscv::Module> pModule;                                              ///< 结果汇编模块
  std::map<sysy::User *, riscv::SymRegister *> variableMap;                            ///< 变量-符号寄存器对应
  std::list<std::unique_ptr<riscv::SymRegister>> symbolRegisterList;                   ///< 符号寄存器列表
  std::map<sysy::AllocaInst *, std::unique_ptr<mid2end::LocalArray>> localArrayMap;    ///< 局部数组对应
  std::map<riscv::Function *, std::unique_ptr<mid2end::StackTable>> functionStackMap;  ///< 函数-栈空间表对应
  std::map<sysy::ConstantValue *, riscv::Global *> constantMap;     ///< 浮点字面量-全局数据对应
  std::map<sysy::BasicBlock *, riscv::BasicBlock *> basicBlockMap;  ///< 基本块-基本块对应

  std::set<riscv::BasicBlock *> targetBlocks;  ///< 跳转目标块

  unsigned floatGlobalIndex = 0;
  unsigned tmpIndex = 0;

 public:
  MacroExpansion() = default;

 private:
  auto getParamPhyReg(bool isInt, unsigned index) -> riscv::PhyRegister *;
  auto getTmpRegister(bool isInt, bool is64 = false) -> riscv::SymRegister *;
  auto getOrEmplaceBasicBlock(riscv::Function *parent, sysy::BasicBlock *block) -> riscv::BasicBlock *;
  auto getOrEmplaceSymRegister(sysy::User *variable, mid2end::StackTable *stackTable) -> riscv::SymRegister *;
  auto getOrEmplaceFloatConstantReg(sysy::ConstantValue *floatConstant) -> riscv::FloatSymRegister *;
  void interpreteOneIR(sysy::Function *function, sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void Li32(riscv::IntRegister *rd, int val);
  void Li64(riscv::IntRegister *rd, int val);

 private:
  void interpreteAdd(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteSub(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteMul(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteDiv(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteRem(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteAnd(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteOr(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteICmpEQ(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteICmpNE(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteICmpLT(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteICmpGT(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteICmpLE(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteICmpGE(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteFAdd(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteFSub(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteFMul(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteFDiv(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteFCmpEQ(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteFCmpNE(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteFCmpLT(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteFCmpGT(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteFCmpLE(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteFCmpGE(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteNeg(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteNot(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteFNeg(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteFNot(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteFtoI(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteItoF(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteCondBr(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteBr(sysy::Instruction *inst);
  void interpreteLoad(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteLa(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteStore(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteMemset(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteCall(sysy::Function *function, sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteReturn(sysy::Instruction *inst, mid2end::StackTable *stackTable);
  void interpreteAlloca(sysy::Instruction *inst);

 public:
  void initBasicElements();   ///< 初始化结果汇编模块、局部数组链表以及函数-栈空间表对应
  void generateInstrution();  ///< 生成汇编指令(符号寄存器)
  void select() override;

 public:
  auto getModule() const -> riscv::Module * override { return pModule.get(); }
  auto getStackTable(riscv::Function *function) -> mid2end::StackTable * override {
    return functionStackMap[function].get();
  }
  auto getBuilder() -> riscv::RiscvBuilder & override { return builder; }
};
}  // namespace macroExpansion