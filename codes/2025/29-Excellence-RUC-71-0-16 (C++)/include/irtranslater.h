#pragma once

#include "riscv_instruction.h"
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <vector>

class RiscvBlock {
public:
  std::string comment; // used for debug
  int block_id = 0;
  int stack_size = 0; // Stack size for this block
  std::deque<RiscvInstruction *> instruction_list;

  void InsertInstruction(int pos, RiscvInstruction *ins);
  void print(std::ostream &s);
  RiscvBlock(int id) : block_id(id) {}
};

// 栈帧信息结构
struct StackFrameInfo {
  int local_vars_size = 16;  // 局部变量占用的栈空间
  int call_args_size = 0;    // 函数调用参数传递区大小
  int alignment_padding = 0; // 对齐填充
  int total_frame_size = 0;  // 总栈帧大小

  // 局部变量信息
  std::map<int, int> var_offsets;   // 虚拟寄存器到栈偏移的映射
  std::map<int, int> array_offsets; // 数组虚拟寄存器到栈偏移的映射

  void calculateTotalSize() {
    // RISC-V要求栈指针16字节对齐
    int unaligned_size = local_vars_size + call_args_size;
    alignment_padding = (16 - (unaligned_size % 16)) % 16;
    total_frame_size = unaligned_size + alignment_padding;
  }
};

class Riscv {
public:
  std::string file_name; // 输出文件名
  std::vector<RiscvInstruction *> global_def;
  std::vector<RiscvInstruction *> function_declare;
  std::map<std::string, std::map<int, RiscvBlock *>>
      function_block_map; //<function,<id,block> >
  void print(std::ostream &s);
};

class Translator {
public:
  Translator() = default; // 新增默认构造函数
  Translator(std::string file) { riscv.file_name = std::move(file); }
  Riscv riscv;
  void translate(const LLVMIR &llvmir);

private:
  // 当前正在翻译的函数
  std::string current_function;
  LLVMType current_function_return_type = LLVMType::VOID_TYPE;
  // 栈帧管理
  std::map<std::string, StackFrameInfo>
      function_stack_frames;                     // 每个函数的栈帧信息
  StackFrameInfo *current_stack_frame = nullptr; // 当前函数的栈帧信息

  // Phi指令处理
  struct PhiTranslationInfo {
    int block_id;         // phi指令所在的块ID
    RiscvOperand *result; // phi指令的结果操作数
    std::vector<std::pair<RiscvOperand *, int>> phi_args; // (值, 前驱块ID)对
  };
  std::vector<PhiTranslationInfo> pending_phi_instructions; // 待处理的phi指令

  // 翻译方法
  void translateGlobal(const std::vector<Instruction> &global_def);
  void translateFunctions(const LLVMIR &llvmir);
  void translateFunction(FuncDefInstruction func,
                         const std::map<int, LLVMBlock> &blocks);
  void analyzeFunction(const std::map<int, LLVMBlock> &blocks);
  void analyzeInstruction(Instruction inst);
  void translateBlock(LLVMBlock block, RiscvBlock *riscv_block);
  void translateInstruction(Instruction inst, RiscvBlock *riscv_block);

  // 操作数翻译
  RiscvOperand *translateOperand(Operand op);

  // 物理寄存器创建辅助方法
  RiscvRegOperand *createVirtualReg();
  RiscvRegOperand *createVirtualReg(int reg_no);
  RiscvRegOperand *getS0Reg();
  RiscvRegOperand *getSpReg();
  RiscvRegOperand *getRaReg();
  RiscvRegOperand *getA0Reg();
  RiscvRegOperand *getArgReg(int arg_index); // 获取参数寄存器 a0-a7
  RiscvRegOperand *getZeroReg();
  RiscvRegOperand *getFa0Reg();

  // 指令翻译
  void translateLoad(LoadInstruction *inst, RiscvBlock *block);
  void translateStore(StoreInstruction *inst, RiscvBlock *block);
  void translateBr_uncond(BrUncondInstruction *inst, RiscvBlock *block);
  void translateBr_cond(BrCondInstruction *inst, RiscvBlock *block);
  void translateCall(CallInstruction *inst, RiscvBlock *block);
  void translateAlloca(AllocaInstruction *inst, RiscvBlock *block);
  void translateReturn(RetInstruction *inst, RiscvBlock *block);
  void translateIcmp(IcmpInstruction *inst, RiscvBlock *block);
  void translateIeq(IcmpInstruction *inst, RiscvBlock *block);
  void translateIne(IcmpInstruction *inst, RiscvBlock *block);
  void translateIgt(IcmpInstruction *inst, RiscvBlock *block);
  void translateIge(IcmpInstruction *inst, RiscvBlock *block);
  void translateIlt(IcmpInstruction *inst, RiscvBlock *block);
  void translateIle(IcmpInstruction *inst, RiscvBlock *block);
  void translateFcmp(FcmpInstruction *inst, RiscvBlock *block);
  void translateFeq(FcmpInstruction *inst, RiscvBlock *block);
  void translateFne(FcmpInstruction *inst, RiscvBlock *block);
  void translateFgt(FcmpInstruction *inst, RiscvBlock *block);
  void translateFge(FcmpInstruction *inst, RiscvBlock *block);
  void translateFlt(FcmpInstruction *inst, RiscvBlock *block);
  void translateFle(FcmpInstruction *inst, RiscvBlock *block);
  void translateAdd(Instruction inst, RiscvBlock *block);
  void translateSub(Instruction inst, RiscvBlock *block);
  void translateMul(Instruction inst, RiscvBlock *block);
  void translateDiv(Instruction inst, RiscvBlock *block);
  void translateMod(Instruction inst, RiscvBlock *block);
  void translateFadd(Instruction inst, RiscvBlock *block);
  void translateFsub(Instruction inst, RiscvBlock *block);
  void translateFmul(Instruction inst, RiscvBlock *block);
  void translateFdiv(Instruction inst, RiscvBlock *block);
  void translateGetElementptr(GetElementptrInstruction *inst,
                              RiscvBlock *block);
  void translateFptosi(FptosiInstruction *inst, RiscvBlock *block);
  void translateSitofp(SitofpInstruction *inst, RiscvBlock *block);
  void translateAnd(ArithmeticInstruction *inst, RiscvBlock *block);
  void translateOr(ArithmeticInstruction *inst, RiscvBlock *block);
  void translateZext(ZextInstruction *inst, RiscvBlock *block);
  void translateAshr(ArithmeticInstruction *inst, RiscvBlock *block);
  void translateLshr(ArithmeticInstruction *inst, RiscvBlock *block);
  void translateTrunc(TruncInstruction *inst, RiscvBlock *block);
  void translatePhi(PhiInstruction *inst, RiscvBlock *block);

  // Phi指令处理方法
  void processPendingPhis(); // 处理所有待处理的phi指令
  void insertPhiMoveInstructions(
      const PhiTranslationInfo &phi_info); // 在前驱块中插入move指令
  bool
  isRiscvTerminatorInstruction(RiscvInstruction *inst); // 检查是否是终止指令
  void insertBeforeTerminator(RiscvBlock *block,
                              RiscvInstruction *inst); // 在终止指令之前插入指令

  // 工具方法
  std::string getLLVMTypeString(LLVMType type);
  void insertAddiInstruction(RiscvOperand *dest, RiscvOperand *src, int imm,
                             RiscvBlock *block, int pos = 1);
  void insertLwInstruction(RiscvOperand *dest, RiscvPtrOperand *addr,
                           RiscvBlock *block);
  void insertLdInstruction(RiscvOperand *dest, RiscvPtrOperand *addr,
                           RiscvBlock *block);
  void insertSdInstruction(RiscvOperand *dest, RiscvPtrOperand *addr,
                           RiscvBlock *block);
  void insertSwInstruction(RiscvOperand *dest, RiscvPtrOperand *addr,
                           RiscvBlock *block);

  // 栈帧管理方法
  void initFunctionStackFrame(const std::string &func_name);
  void addLocalVariable(int virtual_reg, LLVMType type);
  void addLocalArray(int virtual_reg, LLVMType type,
                     const std::vector<int> &dims);
  void updateCallArgsSize(int num_args);
  void finalizeFunctionStackFrame();
  void generateStackFrameProlog(RiscvBlock *entry_block);
  void generateStackFrameEpilog(RiscvBlock *exit_block);
  void generateFunctionParameterReceive(FuncDefInstruction func,
                                        RiscvBlock *entry_block);
  void generateFunctionEpilog(RiscvBlock *exit_block);
  int getLocalVariableOffset(int virtual_reg);
  int getTypeSize(LLVMType type);

public:
  // 栈帧信息查询（公开API）
  const StackFrameInfo *
  getFunctionStackFrame(const std::string &func_name) const;
  int getCurrentFrameSize() const;
};