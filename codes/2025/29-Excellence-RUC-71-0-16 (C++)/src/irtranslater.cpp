#include "../include/irtranslater.h"
#include "../include/llvm_instruction.h"
#include <algorithm>
#include <iostream>
#include <utility>

extern std::map<std::string, int> function_name_to_maxreg;
int next_virtual_reg_no = 0;

void RiscvBlock::InsertInstruction(int pos, RiscvInstruction *ins) {
  if (pos == 0)
    instruction_list.push_front(ins);
  else
    instruction_list.push_back(ins);
}

void RiscvBlock::print(std::ostream &s) {
  s << ".L" << block_id << ":\n";
  for (auto &inst : instruction_list)
    inst->PrintIR(s);
}

void Riscv::print(std::ostream &s) {
  // 输出全局变量定义
  s << "  .file \"" << file_name << "\"\n";
  s << "  .option nopic\n";
  s << "  .attribute stack_align, 16\n";
  s << "  .data\n";
  for (auto &global : global_def)
    global->PrintIR(s);
  // 输出代码段
  s << "  .text\n";
  // 输出函数
  for (auto &func_pair : function_block_map) {
    s << "  .globl " << func_pair.first << "\n";
    s << "  .type " << func_pair.first << ", @function\n";
    s << func_pair.first << ":";
    s << "\n";
    // 输出函数的所有基本块
    for (auto &block_pair : func_pair.second)
      block_pair.second->print(s);
    s << "  .size " << func_pair.first << ", .-" << func_pair.first << "\n";
    s << "\n";
  }
}

void Translator::translate(const LLVMIR &llvmir) {
  translateGlobal(llvmir.global_def);
  translateFunctions(llvmir);
}

void Translator::translateGlobal(const std::vector<Instruction> &global_def) {
  for (const auto &ins : global_def) {
    if (ins->GetOpcode() == LLVMIROpcode::GLOBAL_VAR) {
      auto *global_var = dynamic_cast<GlobalVarDefineInstruction *>(ins);
      if (global_var) {
        auto riscv_global = new RiscvGlobalVarInstruction(
            global_var->GetName(), getLLVMTypeString(global_var->GetType()));
        const auto &attr = global_var->GetAttr();
        riscv_global->dim = attr.dims;
        riscv_global->init_vals = attr.IntInitVals;
        riscv_global->init_float_vals = attr.FloatInitVals;
        riscv.global_def.push_back(riscv_global);
      }
    } else if (ins->GetOpcode() == LLVMIROpcode::GLOBAL_STR) {
      auto *global_str = dynamic_cast<GlobalStringConstInstruction *>(ins);
      if (global_str) {
        auto riscv_global = new RiscvStringConstInstruction(
            global_str->GetName(), global_str->GetValue());
        riscv.global_def.push_back(riscv_global);
      }
    }
  }
}

void Translator::translateFunctions(const LLVMIR &llvmir) {
  for (const auto &func_pair : llvmir.function_block_map)
    translateFunction(func_pair.first, func_pair.second);
}

void Translator::translateFunction(FuncDefInstruction func,
                                   const std::map<int, LLVMBlock> &blocks) {
  current_function = func->Func_name;
  current_function_return_type = func->return_type;
  // 初始化栈帧信息
  initFunctionStackFrame(current_function);
  // 为函数创建块映射
  riscv.function_block_map[current_function] = std::map<int, RiscvBlock *>();
  // 第一步：分析函数，收集栈帧信息
  analyzeFunction(blocks);
  // 第二步：完成栈帧计算
  finalizeFunctionStackFrame();
  next_virtual_reg_no =
      function_name_to_maxreg[current_function]; // 更新虚拟寄存器号
  // 第三步：翻译每个基本块
  RiscvBlock *entry_block = nullptr;
  for (const auto &block_pair : blocks) {
    auto riscv_block = new RiscvBlock(block_pair.first);
    RiscvBlock *block_ptr = riscv_block;
    // 记录入口块
    if (entry_block == nullptr)
      entry_block = block_ptr;
    riscv.function_block_map[current_function][block_pair.first] = riscv_block;
    translateBlock(block_pair.second, block_ptr);
  }

  // 第四步：处理所有的phi指令
  processPendingPhis();

  if (entry_block) {
    // 设置入口块的栈帧大小信息
    entry_block->stack_size = current_stack_frame->total_frame_size;
    if (current_stack_frame->total_frame_size > 0)
      generateStackFrameProlog(entry_block);

    // 生成函数参数接收代码
    generateFunctionParameterReceive(func, entry_block);
  }
}

void Translator::analyzeFunction(const std::map<int, LLVMBlock> &blocks) {
  // 分析所有指令，收集栈帧信息
  for (const auto &block_pair : blocks)
    for (const auto &inst : block_pair.second->Instruction_list)
      analyzeInstruction(inst);
}

void Translator::analyzeInstruction(Instruction inst) {
  switch (inst->GetOpcode()) {
  case LLVMIROpcode::ALLOCA: {
    auto *alloca_inst = dynamic_cast<AllocaInstruction *>(inst);
    if (alloca_inst) {
      // 将BasicOperand转换为RegOperand以获取寄存器号
      auto *reg_operand = dynamic_cast<RegOperand *>(alloca_inst->GetResult());
      if (reg_operand) {
        if (!alloca_inst->GetDims().empty())
          addLocalArray(reg_operand->GetRegNo(), alloca_inst->GetType(),
                        alloca_inst->GetDims());
        else
          addLocalVariable(reg_operand->GetRegNo(), alloca_inst->GetType());
      }
    }
    break;
  }
  case LLVMIROpcode::CALL: {
    auto *call_inst = dynamic_cast<CallInstruction *>(inst);
    if (call_inst)
      updateCallArgsSize(call_inst->GetArgs().size());
    break;
  }
  default:
    // 其他指令不影响栈帧大小
    break;
  }
}

void Translator::translateBlock(LLVMBlock block, RiscvBlock *riscv_block) {
  for (const auto &inst : block->Instruction_list)
    translateInstruction(inst, riscv_block);
}

void Translator::translateInstruction(Instruction inst,
                                      RiscvBlock *riscv_block) {
  switch (inst->GetOpcode()) {
  case LLVMIROpcode::ADD:
    translateAdd(inst, riscv_block);
    break;
  case LLVMIROpcode::SUB:
    translateSub(inst, riscv_block);
    break;
  case LLVMIROpcode::MUL_OP:
    translateMul(inst, riscv_block);
    break;
  case LLVMIROpcode::DIV_OP:
    translateDiv(inst, riscv_block);
    break;
  case LLVMIROpcode::MOD_OP:
    translateMod(inst, riscv_block);
    break;
  case LLVMIROpcode::FADD:
    translateFadd(inst, riscv_block);
    break;
  case LLVMIROpcode::FSUB:
    translateFsub(inst, riscv_block);
    break;
  case LLVMIROpcode::FMUL:
    translateFmul(inst, riscv_block);
    break;
  case LLVMIROpcode::FDIV:
    translateFdiv(inst, riscv_block);
    break;
  case LLVMIROpcode::LOAD:
    translateLoad(dynamic_cast<LoadInstruction *>(inst), riscv_block);
    break;
  case LLVMIROpcode::STORE:
    translateStore(dynamic_cast<StoreInstruction *>(inst), riscv_block);
    break;
  case LLVMIROpcode::BR_COND:
    translateBr_cond(dynamic_cast<BrCondInstruction *>(inst), riscv_block);
    break;
  case LLVMIROpcode::BR_UNCOND:
    translateBr_uncond(dynamic_cast<BrUncondInstruction *>(inst), riscv_block);
    break;
  case LLVMIROpcode::CALL:
    translateCall(dynamic_cast<CallInstruction *>(inst), riscv_block);
    break;
  case LLVMIROpcode::RET:
    translateReturn(dynamic_cast<RetInstruction *>(inst), riscv_block);
    break;
  case LLVMIROpcode::ICMP:
    translateIcmp(dynamic_cast<IcmpInstruction *>(inst), riscv_block);
    break;
  case LLVMIROpcode::FCMP:
    translateFcmp(dynamic_cast<FcmpInstruction *>(inst), riscv_block);
    break;
  case LLVMIROpcode::GETELEMENTPTR:
    translateGetElementptr(dynamic_cast<GetElementptrInstruction *>(inst),
                           riscv_block);
    break;
  case LLVMIROpcode::FPTOSI:
    translateFptosi(dynamic_cast<FptosiInstruction *>(inst), riscv_block);
    break;
  case LLVMIROpcode::SITOFP:
    translateSitofp(dynamic_cast<SitofpInstruction *>(inst), riscv_block);
    break;
  case LLVMIROpcode::BITAND:
    translateAnd(dynamic_cast<ArithmeticInstruction *>(inst), riscv_block);
    break;
  case LLVMIROpcode::BITOR:
    translateOr(dynamic_cast<ArithmeticInstruction *>(inst), riscv_block);
    break;
  case LLVMIROpcode::ALLOCA:
    translateAlloca(dynamic_cast<AllocaInstruction *>(inst), riscv_block);
    break;
  case LLVMIROpcode::ZEXT:
    translateZext(dynamic_cast<ZextInstruction *>(inst), riscv_block);
    break;
  case LLVMIROpcode::ASHR:
    translateAshr(dynamic_cast<ArithmeticInstruction *>(inst), riscv_block);
    break;
  case LLVMIROpcode::LSHR:
    translateLshr(dynamic_cast<ArithmeticInstruction *>(inst), riscv_block);
    break;
  case LLVMIROpcode::TRUNC:
    translateTrunc(dynamic_cast<TruncInstruction *>(inst), riscv_block);
    break;
  case LLVMIROpcode::PHI:
    translatePhi(dynamic_cast<PhiInstruction *>(inst), riscv_block);
    break;
  default:
    std::cerr << "Unsupported instruction opcode: " << inst->GetOpcode()
              << std::endl;
    break;
  }
}

void Translator::translateLoad(LoadInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  RiscvOpcode op = (inst->GetType() == LLVMType::FLOAT32) ? RiscvOpcode::FLW
                                                          : RiscvOpcode::LW;
  if (op == RiscvOpcode::FLW) {
    // 浮点加载指令
    auto rs = translateOperand(inst->GetResult());
    int offset = 0;
    RiscvOperand *s0 = nullptr;
    if (inst->GetPointer()->GetOperandType() == BasicOperand::GLOBAL) {
      auto global_op = dynamic_cast<GlobalOperand *>(inst->GetPointer());
      auto global_var = new RiscvGlobalOperand(global_op->GetName());
      s0 = createVirtualReg();
      auto la_inst = new RiscvLaInstruction(s0, global_var);
      block->InsertInstruction(1, la_inst);
    } else {
      auto addr = dynamic_cast<RegOperand *>(inst->GetPointer());
      if (current_stack_frame->var_offsets.find(addr->GetRegNo()) !=
          current_stack_frame->var_offsets.end()) {
        s0 = getS0Reg(); // 获取s0寄存器
        offset = current_stack_frame->var_offsets[addr->GetRegNo()];
      } else {
        s0 = createVirtualReg(addr->GetRegNo()); // 如果是数组，创建对应的寄存器
        offset = current_stack_frame->array_offsets[addr->GetRegNo()];
      }
    }
    auto riscv_inst = new RiscvPtrOperand(-offset, s0);
    auto riscv_load_inst = new RiscvFlwInstruction(rs, riscv_inst);
    block->InsertInstruction(1, riscv_load_inst);
  } else {
    // 整数加载指令
    auto rs = translateOperand(inst->GetResult());
    int offset = 0;
    RiscvOperand *s0 = nullptr;
    if (inst->GetPointer()->GetOperandType() == BasicOperand::GLOBAL) {
      auto global_op = dynamic_cast<GlobalOperand *>(inst->GetPointer());
      s0 = createVirtualReg();
      auto global_var = new RiscvGlobalOperand(global_op->GetName());
      auto la_inst = new RiscvLaInstruction(s0, global_var);
      block->InsertInstruction(1, la_inst);
    } else {
      auto addr = dynamic_cast<RegOperand *>(inst->GetPointer());
      if (current_stack_frame->var_offsets.find(addr->GetRegNo()) !=
          current_stack_frame->var_offsets.end()) {
        s0 = getS0Reg(); // 获取s0寄存器
        offset = current_stack_frame->var_offsets[addr->GetRegNo()];
      } else {
        s0 = createVirtualReg(addr->GetRegNo()); // 如果是数组，创建对应的寄存器
        offset = current_stack_frame->array_offsets[addr->GetRegNo()];
      }
    }
    auto riscv_inst = new RiscvPtrOperand(-offset, s0);
    insertLwInstruction(rs, riscv_inst, block);
  }
}

void Translator::translateStore(StoreInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  RiscvOpcode op = (inst->GetType() == LLVMType::FLOAT32) ? RiscvOpcode::FSW
                                                          : RiscvOpcode::SW;
  if (op == RiscvOpcode::FSW) {
    // 浮点存储指令
    auto rs = translateOperand(inst->GetValue());

    // 检查是否是浮点立即数，如果是，需要先加载到寄存器
    if (inst->GetValue()->GetOperandType() == BasicOperand::IMMF32) {
      auto imm_f32 = dynamic_cast<RiscvImmF32Operand *>(rs);
      // 创建虚拟寄存器来存储浮点立即数
      auto float_reg = createVirtualReg();

      // 将浮点立即数的二进制表示作为整数立即数
      float float_val = imm_f32->GetFloatVal();
      uint32_t float_bits = *(uint32_t *)&float_val;
      if (float_bits == 0) {
        // 浮点0可以直接用整数0通过fmv.w.x创建
        auto zero_reg = getZeroReg();
        auto fmv_inst = new RiscvFmvInstruction(float_reg, zero_reg);
        block->InsertInstruction(1, fmv_inst);
      } else {
        // 其他浮点常量需要先用li加载到整数寄存器，再用fmv.w.x转换
        auto int_reg = createVirtualReg();
        auto li_inst = new RiscvLiInstruction(int_reg, (int32_t)float_bits);
        auto fmv_inst = new RiscvFmvInstruction(float_reg, int_reg);
        block->InsertInstruction(1, li_inst);
        block->InsertInstruction(1, fmv_inst);
      }
      rs = float_reg;
    }

    int offset = 0;
    RiscvOperand *s0 = nullptr;
    if (inst->GetPointer()->GetOperandType() == BasicOperand::GLOBAL) {
      auto global_op = dynamic_cast<GlobalOperand *>(inst->GetPointer());
      s0 = createVirtualReg();
      auto label_op = new RiscvLabelOperand(global_op->GetName());
      auto la_inst = new RiscvLaInstruction(s0, label_op);
      block->InsertInstruction(1, la_inst);
    } else {
      auto addr = dynamic_cast<RegOperand *>(inst->GetPointer());
      if (current_stack_frame->var_offsets.find(addr->GetRegNo()) !=
          current_stack_frame->var_offsets.end()) {
        s0 = getS0Reg(); // 获取s0寄存器
        offset = current_stack_frame->var_offsets[addr->GetRegNo()];
      } else {
        s0 = createVirtualReg(addr->GetRegNo()); // 如果是数组，创建对应的寄存器
        offset = current_stack_frame->array_offsets[addr->GetRegNo()];
      }
    }
    auto riscv_inst = new RiscvPtrOperand(-offset, s0);
    auto riscv_store_inst = new RiscvFswInstruction(rs, riscv_inst);
    block->InsertInstruction(1, riscv_store_inst);
  } else {
    // 整数存储指令
    auto rs = translateOperand(inst->GetValue());
    if (inst->GetValue()->GetOperandType() == BasicOperand::IMMI32) {
      // 如果是浮点数，转换为整数存储
      int imm_val =
          dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetValue()))
              ->GetIntImmVal();
      if (imm_val == 0) {
        rs = getZeroReg(); // 如果是0，使用zero寄存器
      } else {
        rs = createVirtualReg();
        auto li_inst =
            new RiscvLiInstruction(rs, imm_val); // 将立即数转换为寄存器
        block->InsertInstruction(1, li_inst);
      }
    }
    int offset = 0;
    RiscvOperand *s0 = nullptr;
    if (inst->GetPointer()->GetOperandType() == BasicOperand::GLOBAL) {
      auto global_op = dynamic_cast<GlobalOperand *>(inst->GetPointer());
      s0 = createVirtualReg();
      auto label_op = new RiscvLabelOperand(global_op->GetName());
      auto la_inst = new RiscvLaInstruction(s0, label_op);
      block->InsertInstruction(1, la_inst);
    } else {
      auto addr = dynamic_cast<RegOperand *>(inst->GetPointer());
      if (current_stack_frame->var_offsets.find(addr->GetRegNo()) !=
          current_stack_frame->var_offsets.end()) {
        s0 = getS0Reg(); // 获取s0寄存器
        offset = current_stack_frame->var_offsets[addr->GetRegNo()];
      } else {
        s0 = createVirtualReg(addr->GetRegNo()); // 如果是数组，创建对应的寄存器
        offset = current_stack_frame->array_offsets[addr->GetRegNo()];
      }
    }
    auto riscv_inst = new RiscvPtrOperand(-offset, s0);
    insertSwInstruction(rs, riscv_inst, block);
  }
}

void Translator::translateCall(CallInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;

  // 生成参数传递指令 - 在call指令之前
  auto args = inst->GetArgs();
  for (int i = 0; i < std::min((int)args.size(), 8); i++) {
    auto arg_operand = translateOperand(args[i].second); // second是Operand
    auto arg_reg = getArgReg(i); // 获取参数寄存器 a0, a1, ..., a7

    // 根据参数类型选择合适的指令
    if (i < (int)args.size() && args[i].first == LLVMType::FLOAT32) {
      // 浮点参数：使用fmv指令
      auto fmv_inst = new RiscvFmvInstruction(arg_reg, arg_operand);
      block->InsertInstruction(1, fmv_inst);
    } else {
      // 整数参数：使用mv指令
      auto mv_inst = new RiscvMvInstruction(arg_reg, arg_operand);
      block->InsertInstruction(1, mv_inst);
    }
  }

  auto call_inst = new RiscvCallInstruction(inst->GetFuncName());
  block->InsertInstruction(1, call_inst);

  // 处理函数返回值
  if (inst->result && inst->ret_type != LLVMType::VOID_TYPE) {
    auto result_operand = translateOperand(inst->result);
    if (inst->ret_type == LLVMType::FLOAT32) {
      // 浮点返回值从fa0移动到目标寄存器
      auto fmv_inst = new RiscvFmvInstruction(result_operand, getFa0Reg());
      block->InsertInstruction(1, fmv_inst);
    } else {
      // 整数返回值从a0移动到目标寄存器
      auto mv_inst = new RiscvMvInstruction(result_operand, getA0Reg());
      block->InsertInstruction(1, mv_inst);
    }
  }
}

void Translator::translateAlloca(AllocaInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;

  auto result_operand = translateOperand(inst->GetResult());
  auto *reg_operand = dynamic_cast<RegOperand *>(inst->GetResult());

  if (reg_operand) {
    int virtual_reg = reg_operand->GetRegNo();
    int offset = getLocalVariableOffset(virtual_reg);

    if (offset != 0) {
      // 生成计算栈地址的指令: addi %result_reg, s0, -offset
      // 因为栈向下增长，数组在负偏移位置
      auto s0_reg = getS0Reg();
      insertAddiInstruction(result_operand, s0_reg, -offset, block, 1);

      // 如果这是数组类型的alloca，记录到array_offsets中
      if (!inst->GetDims().empty()) {
        current_stack_frame->array_offsets[virtual_reg] = offset;
      }
    }
  }
}

void Translator::translateReturn(RetInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  if (inst->GetRetVal()) {
    auto ret_val = inst->GetRetVal();
    if (current_function_return_type == LLVMType::FLOAT32) {
      if (ret_val->GetOperandType() == BasicOperand::REG) {
        auto rs = translateOperand(ret_val);
        auto fmv_inst = new RiscvFmvInstruction(getFa0Reg(), rs);
        block->InsertInstruction(1, fmv_inst);
      } else if (ret_val->GetOperandType() == BasicOperand::IMMF32) {
        auto imm_val =
            dynamic_cast<RiscvImmF32Operand *>(translateOperand(ret_val));
        auto tmp_reg = createVirtualReg();
        auto li_inst = new RiscvLiInstruction(tmp_reg, imm_val->GetFloatVal());
        block->InsertInstruction(1, li_inst);
        auto fmvwx_inst = new RiscvFmvwxInstruction(getFa0Reg(), tmp_reg);
        block->InsertInstruction(1, fmvwx_inst);
      }
    } else {
      // 整数返回值处理
      if (ret_val->GetOperandType() == BasicOperand::REG) {
        auto rs = translateOperand(ret_val);
        auto mv_inst = new RiscvMvInstruction(getA0Reg(), rs);
        block->InsertInstruction(1, mv_inst);
      } else if (ret_val->GetOperandType() == BasicOperand::IMMI32) {
        auto imm_val =
            dynamic_cast<RiscvImmI32Operand *>(translateOperand(ret_val));
        auto li_inst =
            new RiscvLiInstruction(getA0Reg(), imm_val->GetIntImmVal());
        block->InsertInstruction(1, li_inst);
      }
    }
  }

  // 生成完整的函数结尾代码
  generateFunctionEpilog(block);
}

void Translator::translateIcmp(IcmpInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto cond = inst->GetCond();
  switch (cond) {
  case IcmpCond::eq:
    translateIeq(inst, block);
    break;
  case IcmpCond::ne:
    translateIne(inst, block);
    break;
  case IcmpCond::sgt:
    translateIgt(inst, block);
    break;
  case IcmpCond::sge:
    translateIge(inst, block);
    break;
  case IcmpCond::slt:
    translateIlt(inst, block);
    break;
  case IcmpCond::sle:
    translateIle(inst, block);
    break;
  default:
    std::cerr << "Unsupported Icmp condition: " << static_cast<int>(cond)
              << std::endl;
    break;
  }
}

void Translator::translateIeq(IcmpInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->GetResult());
  if (inst->GetOp1()->isIMM() && inst->GetOp2()->isIMM()) {
    // 如果两个操作数都是立即数，直接计算结果
    auto op1 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    auto op2 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    auto li_inst = new RiscvLiInstruction(result, op1->GetIntImmVal() ==
                                                      op2->GetIntImmVal());
    block->InsertInstruction(1, li_inst);
  } else if (inst->GetOp1()->isIMM()) {
    // 如果第一个操作
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    auto rs2 = translateOperand(inst->GetOp2());
    RiscvRegOperand *rs1 = nullptr;
    if (imm->GetIntImmVal() == 0) {
      rs1 = getZeroReg(); // 如果是0，使用zero寄存器
    } else {
      rs1 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs1, imm->GetIntImmVal());
      block->InsertInstruction(1, li_inst);
    }
    auto xor_inst =
        new RiscvXorInstruction(result, rs1, rs2); // 使用XOR指令比较
    block->InsertInstruction(1, xor_inst);
    auto seqz_inst =
        new RiscvSeqzInstruction(result, result); // 检查结果是否为0
    block->InsertInstruction(2, seqz_inst);
  } else if (inst->GetOp2()->isIMM()) {
    // 如果第二个操作数是立即数
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    auto rs1 = translateOperand(inst->GetOp1());
    RiscvRegOperand *rs2 = nullptr;
    if (imm->GetIntImmVal() == 0) {
      rs2 = getZeroReg(); // 如果是0，使用zero寄存器
    } else {
      rs2 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs2, imm->GetIntImmVal());
      block->InsertInstruction(1, li_inst);
    }
    auto xor_inst =
        new RiscvXorInstruction(result, rs1, rs2); // 使用XOR指令比较
    block->InsertInstruction(1, xor_inst);
    auto seqz_inst =
        new RiscvSeqzInstruction(result, result); // 检查结果是否为0
    block->InsertInstruction(2, seqz_inst);
  } else {
    // 否则使用SEQZ指令
    auto rs1 = translateOperand(inst->GetOp1());
    auto rs2 = translateOperand(inst->GetOp2());
    auto xor_inst = new RiscvXorInstruction(result, rs1, rs2);
    block->InsertInstruction(1, xor_inst);
    auto seqz_inst = new RiscvSeqzInstruction(result, result);
    block->InsertInstruction(2, seqz_inst);
  }
}

void Translator::translateIne(IcmpInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->GetResult());
  if (inst->GetOp1()->isIMM() && inst->GetOp2()->isIMM()) {
    // 如果两个操作数都是立即数，直接计算结果
    auto op1 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    auto op2 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    auto li_inst = new RiscvLiInstruction(result, op1->GetIntImmVal() !=
                                                      op2->GetIntImmVal());
    block->InsertInstruction(1, li_inst);
  } else if (inst->GetOp1()->isIMM()) {
    // 如果第一个操作数是立即数
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    auto rs2 = translateOperand(inst->GetOp2());
    RiscvRegOperand *rs1 = nullptr;
    if (imm->GetIntImmVal() == 0) {
      rs1 = getZeroReg(); // 如果是0，使用zero寄存器
    } else {
      rs1 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs1, imm->GetIntImmVal());
      block->InsertInstruction(1, li_inst);
    }
    auto xor_inst =
        new RiscvXorInstruction(result, rs1, rs2); // 使用XOR指令比较
    block->InsertInstruction(1, xor_inst);
    auto snez_inst =
        new RiscvSnezInstruction(result, result); // 检查结果是否非0
    block->InsertInstruction(2, snez_inst);
  } else if (inst->GetOp2()->isIMM()) {
    // 如果第二个操作数是立即数
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    auto rs1 = translateOperand(inst->GetOp1());
    RiscvRegOperand *rs2 = nullptr;
    if (imm->GetIntImmVal() == 0) {
      rs2 = getZeroReg(); // 如果是0，使用zero寄存器
    } else {
      rs2 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs2, imm->GetIntImmVal());
      block->InsertInstruction(1, li_inst);
    }
    auto xor_inst = new RiscvXorInstruction(result, rs1, rs2);
    block->InsertInstruction(1, xor_inst);
    auto snez_inst = new RiscvSnezInstruction(result, result);
    block->InsertInstruction(2, snez_inst);
  } else {
    // 否则使用SNEZ指令
    auto rs1 = translateOperand(inst->GetOp1());
    auto rs2 = translateOperand(inst->GetOp2());
    auto xor_inst = new RiscvXorInstruction(result, rs1, rs2);
    block->InsertInstruction(1, xor_inst);
    auto snez_inst = new RiscvSnezInstruction(result, result);
    block->InsertInstruction(2, snez_inst);
  }
}

void Translator::translateIgt(IcmpInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->GetResult());
  if (inst->GetOp1()->isIMM() && inst->GetOp2()->isIMM()) {
    // 如果两个操作数都是立即数，直接计算结果
    auto op1 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    auto op2 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    auto li_inst = new RiscvLiInstruction(result, op1->GetIntImmVal() >
                                                      op2->GetIntImmVal());
    block->InsertInstruction(1, li_inst);
  } else if (inst->GetOp1()->isIMM()) {
    // 如果第一个操作数是立即数
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    auto rs2 = translateOperand(inst->GetOp2());
    RiscvRegOperand *rs1 = nullptr;
    if (imm->GetIntImmVal() == 0) {
      rs1 = getZeroReg(); // 如果是0，使用zero寄存器
    } else {
      rs1 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs1, imm->GetIntImmVal());
      block->InsertInstruction(1, li_inst);
    }
    auto slt_inst = new RiscvSltInstruction(result, rs2, rs1);
    block->InsertInstruction(1, slt_inst);
  } else if (inst->GetOp2()->isIMM()) {
    // 如果第二个操作数是立即数
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    auto rs1 = translateOperand(inst->GetOp1());
    RiscvRegOperand *rs2 = nullptr;
    if (imm->GetIntImmVal() == 0) {
      rs2 = getZeroReg(); // 如果是0，使用zero寄存器
    } else {
      rs2 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs2, imm->GetIntImmVal());
      block->InsertInstruction(1, li_inst);
    }
    auto slt_inst = new RiscvSltInstruction(result, rs2, rs1);
    block->InsertInstruction(1, slt_inst);
  } else {
    // 否则使用SLT指令
    auto rs1 = translateOperand(inst->GetOp1());
    auto rs2 = translateOperand(inst->GetOp2());
    auto slt_inst = new RiscvSltInstruction(result, rs2, rs1);
    block->InsertInstruction(1, slt_inst);
  }
}

void Translator::translateIge(IcmpInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->GetResult());
  if (inst->GetOp1()->isIMM() && inst->GetOp2()->isIMM()) {
    // 如果两个操作数都是立即数，直接计算结果
    auto op1 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    auto op2 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    auto li_inst = new RiscvLiInstruction(result, op1->GetIntImmVal() >=
                                                      op2->GetIntImmVal());
    block->InsertInstruction(1, li_inst);
  } else if (inst->GetOp1()->isIMM()) {
    // 如果第一个操作数是立即数
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    auto rs2 = translateOperand(inst->GetOp2());
    RiscvRegOperand *rs1 = nullptr;
    if (imm->GetIntImmVal() == 0) {
      rs1 = getZeroReg(); // 如果是0，使用zero寄存器
    } else {
      rs1 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs1, imm->GetIntImmVal());
      block->InsertInstruction(1, li_inst);
    }
    auto slt_inst = new RiscvSltInstruction(result, rs1, rs2);
    block->InsertInstruction(1, slt_inst);
    auto xori_inst = new RiscvXoriInstruction(result, result, 1);
    block->InsertInstruction(2, xori_inst); // 取反结果
  } else if (inst->GetOp2()->isIMM()) {
    // 如果第二个操作数是立即数
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    auto rs1 = translateOperand(inst->GetOp1());
    RiscvRegOperand *rs2 = nullptr;
    if (imm->GetIntImmVal() == 0) {
      rs2 = getZeroReg(); // 如果是0，使用zero寄存器
    } else {
      rs2 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs2, imm->GetIntImmVal());
      block->InsertInstruction(1, li_inst);
    }
    auto slt_inst = new RiscvSltInstruction(result, rs1, rs2);
    block->InsertInstruction(1, slt_inst);
    auto xori_inst = new RiscvXoriInstruction(result, result, 1);
    block->InsertInstruction(2, xori_inst); // 取反结果
  } else {
    // 否则使用SLT指令
    auto rs1 = translateOperand(inst->GetOp1());
    auto rs2 = translateOperand(inst->GetOp2());
    auto slt_inst = new RiscvSltInstruction(result, rs1, rs2);
    block->InsertInstruction(1, slt_inst);
    auto xori_inst = new RiscvXoriInstruction(result, result, 1);
    block->InsertInstruction(2, xori_inst); // 取反结果
  }
}

void Translator::translateIlt(IcmpInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->GetResult());
  if (inst->GetOp1()->isIMM() && inst->GetOp2()->isIMM()) {
    // 如果两个操作数都是立即数，直接计算结果
    auto op1 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    auto op2 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    auto li_inst = new RiscvLiInstruction(result, op1->GetIntImmVal() <
                                                      op2->GetIntImmVal());
    block->InsertInstruction(1, li_inst);
  } else if (inst->GetOp1()->isIMM()) {
    // 如果第一个操作数是立即数
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    auto rs2 = translateOperand(inst->GetOp2());
    RiscvRegOperand *rs1 = nullptr;
    if (imm->GetIntImmVal() == 0) {
      rs1 = getZeroReg(); // 如果是0，使用zero寄存器
    } else {
      rs1 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs1, imm->GetIntImmVal());
      block->InsertInstruction(1, li_inst);
    }
    auto slt_inst = new RiscvSltInstruction(result, rs1, rs2);
    block->InsertInstruction(1, slt_inst);
  } else if (inst->GetOp2()->isIMM()) {
    // 如果第二个操作数是立即数
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    auto rs1 = translateOperand(inst->GetOp1());
    RiscvRegOperand *rs2 = nullptr;
    if (imm->GetIntImmVal() == 0) {
      rs2 = getZeroReg(); // 如果是0，使用zero寄存器
    } else {
      rs2 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs2, imm->GetIntImmVal());
      block->InsertInstruction(1, li_inst);
    }
    auto slt_inst = new RiscvSltInstruction(result, rs1, rs2);
    block->InsertInstruction(1, slt_inst);
  } else {
    // 否则使用SLT指令
    auto rs1 = translateOperand(inst->GetOp1());
    auto rs2 = translateOperand(inst->GetOp2());
    auto slt_inst = new RiscvSltInstruction(result, rs1, rs2);
    block->InsertInstruction(1, slt_inst);
  }
}

void Translator::translateIle(IcmpInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->GetResult());
  if (inst->GetOp1()->isIMM() && inst->GetOp2()->isIMM()) {
    // 如果两个操作数都是立即数，直接计算结果
    auto op1 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    auto op2 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    auto li_inst = new RiscvLiInstruction(result, op1->GetIntImmVal() <=
                                                      op2->GetIntImmVal());
    block->InsertInstruction(1, li_inst);
  } else if (inst->GetOp1()->isIMM()) {
    // 如果第一个操作数是立即数
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    auto rs2 = translateOperand(inst->GetOp2());
    RiscvRegOperand *rs1 = nullptr;
    if (imm->GetIntImmVal() == 0) {
      rs1 = getZeroReg(); // 如果是0，使用zero寄存器
    } else {
      rs1 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs1, imm->GetIntImmVal());
      block->InsertInstruction(1, li_inst);
    }
    auto slt_inst = new RiscvSltInstruction(result, rs2, rs1);
    block->InsertInstruction(1, slt_inst);
    auto xori_inst = new RiscvXoriInstruction(result, result, 1);
    block->InsertInstruction(2, xori_inst); // 取反结果
  } else if (inst->GetOp2()->isIMM()) {
    // 如果第二个操作数是立即数
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    auto rs1 = translateOperand(inst->GetOp1());
    RiscvRegOperand *rs2 = nullptr;
    if (imm->GetIntImmVal() == 0) {
      rs2 = getZeroReg(); // 如果是0，使用zero寄存器
    } else {
      rs2 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs2, imm->GetIntImmVal());
      block->InsertInstruction(1, li_inst);
    }
    auto slt_inst = new RiscvSltInstruction(result, rs2, rs1);
    block->InsertInstruction(1, slt_inst);
    auto xori_inst = new RiscvXoriInstruction(result, result, 1);
    block->InsertInstruction(2, xori_inst); // 取反结果
  } else {
    // 否则使用SLT指令
    auto rs1 = translateOperand(inst->GetOp1());
    auto rs2 = translateOperand(inst->GetOp2());
    auto slt_inst = new RiscvSltInstruction(result, rs2, rs1);
    block->InsertInstruction(1, slt_inst);
    auto xori_inst = new RiscvXoriInstruction(result, result, 1);
    block->InsertInstruction(2, xori_inst); // 取反结果
  }
}

void Translator::translateFcmp(FcmpInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->GetResult());
  auto cond = inst->GetCond();
  switch (cond) {
  case FcmpCond::OEQ:
    translateFeq(inst, block);
    break;
  case FcmpCond::ONE:
    translateFne(inst, block);
    break;
  case FcmpCond::OLT:
    translateFlt(inst, block);
    break;
  case FcmpCond::OLE:
    translateFle(inst, block);
    break;
  case FcmpCond::OGT:
    translateFgt(inst, block);
    break;
  case FcmpCond::OGE:
    translateFge(inst, block);
    break;
  default:
    std::cerr << "Unsupported Fcmp condition: " << static_cast<int>(cond)
              << std::endl;
    break;
  }
}

void Translator::translateFeq(FcmpInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->GetResult());
  if (inst->GetOp1()->isIMM() && inst->GetOp2()->isIMM()) {
    // 如果两个操作数都是浮点立即数，直接计算结果
    auto op1 =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp1()));
    auto op2 =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp2()));
    auto li_inst = new RiscvLiInstruction(result, op1->GetFloatVal() ==
                                                      op2->GetFloatVal());
    block->InsertInstruction(1, li_inst);
  } else if (inst->GetOp1()->isIMM()) {
    // 如果第一个操作数是浮点立即数
    auto imm =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp1()));
    auto rs2 = translateOperand(inst->GetOp2());
    RiscvRegOperand *rs1 = nullptr;
    if (imm->GetFloatVal() == 0.0f) {
      rs1 = getZeroReg(); // 如果是0.0，使用zero寄存器
    } else {
      rs1 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs1, imm->GetFloatVal());
      block->InsertInstruction(1, li_inst);
      auto fmvwx_inst = new RiscvFmvwxInstruction(rs1, rs1);
      block->InsertInstruction(2, fmvwx_inst);
    }
    auto feq_inst = new RiscvFeqInstruction(result, rs1, rs2);
    block->InsertInstruction(1, feq_inst);
  } else if (inst->GetOp2()->isIMM()) {
    // 如果第二个操作数是浮点立即数
    auto imm =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp2()));
    auto rs1 = translateOperand(inst->GetOp1());
    RiscvRegOperand *rs2 = nullptr;
    if (imm->GetFloatVal() == 0.0f) {
      rs2 = getZeroReg(); // 如果是0.0，使用zero寄存器
    } else {
      rs2 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs2, imm->GetFloatVal());
      block->InsertInstruction(1, li_inst);
      auto fmvwx_inst = new RiscvFmvwxInstruction(rs2, rs2);
      block->InsertInstruction(2, fmvwx_inst);
    }
    auto feq_inst = new RiscvFeqInstruction(result, rs1, rs2);
    block->InsertInstruction(1, feq_inst);
  } else {
    // 否则使用FEQ指令
    auto rs1 = translateOperand(inst->GetOp1());
    auto rs2 = translateOperand(inst->GetOp2());
    auto feq_inst = new RiscvFeqInstruction(result, rs1, rs2);
    block->InsertInstruction(1, feq_inst);
  }
}

void Translator::translateFne(FcmpInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->GetResult());
  if (inst->GetOp1()->isIMM() && inst->GetOp2()->isIMM()) {
    // 如果两个操作数都是浮点立即数，直接计算结果
    auto op1 =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp1()));
    auto op2 =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp2()));
    auto li_inst = new RiscvLiInstruction(result, op1->GetFloatVal() !=
                                                      op2->GetFloatVal());
    block->InsertInstruction(1, li_inst);
  } else if (inst->GetOp1()->isIMM()) {
    // 如果第一个操作数是浮点立即数
    auto imm =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp1()));
    auto rs2 = translateOperand(inst->GetOp2());
    RiscvRegOperand *rs1 = nullptr;
    if (imm->GetFloatVal() == 0.0f) {
      rs1 = getZeroReg(); // 如果是0.0，使用zero寄存器
    } else {
      rs1 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs1, imm->GetFloatVal());
      block->InsertInstruction(1, li_inst);
      auto fmvwx_inst = new RiscvFmvwxInstruction(rs1, rs1);
      block->InsertInstruction(2, fmvwx_inst);
    }
    auto fne_inst = new RiscvFeqInstruction(result, rs1, rs2);
    block->InsertInstruction(1, fne_inst);
    auto seqz_inst =
        new RiscvSeqzInstruction(result, result); // 检查结果是否为0
    block->InsertInstruction(2, seqz_inst);
  } else if (inst->GetOp2()->isIMM()) {
    // 如果第二个操作数是浮点立即数
    auto imm =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp2()));
    auto rs1 = translateOperand(inst->GetOp1());
    RiscvRegOperand *rs2 = nullptr;
    if (imm->GetFloatVal() == 0.0f) {
      rs2 = getZeroReg(); // 如果是0.0，使用zero寄存器
    } else {
      rs2 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs2, imm->GetFloatVal());
      block->InsertInstruction(1, li_inst);
      auto fmvwx_inst = new RiscvFmvwxInstruction(rs2, rs2);
      block->InsertInstruction(2, fmvwx_inst);
    }
    auto fne_inst = new RiscvFeqInstruction(result, rs1, rs2);
    block->InsertInstruction(1, fne_inst);
    auto seqz_inst =
        new RiscvSeqzInstruction(result, result); // 检查结果是否为0
    block->InsertInstruction(2, seqz_inst);
  } else {
    // 否则使用FNE指令
    auto rs1 = translateOperand(inst->GetOp1());
    auto rs2 = translateOperand(inst->GetOp2());
    auto fne_inst = new RiscvFeqInstruction(result, rs1, rs2);
    block->InsertInstruction(1, fne_inst);
    auto seqz_inst = new RiscvSeqzInstruction(result, result);
    block->InsertInstruction(2, seqz_inst); // 检查结果是否为0
  }
}

void Translator::translateFlt(FcmpInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->GetResult());
  if (inst->GetOp1()->isIMM() && inst->GetOp2()->isIMM()) {
    // 如果两个操作数都是浮点立即数，直接计算结果
    auto op1 =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp1()));
    auto op2 =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp2()));
    auto li_inst =
        new RiscvLiInstruction(result, op1->GetFloatVal() < op2->GetFloatVal());
    block->InsertInstruction(1, li_inst);
  } else if (inst->GetOp1()->isIMM()) {
    // 如果第一个操作数是浮点立即数
    auto imm =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp1()));
    auto rs2 = translateOperand(inst->GetOp2());
    RiscvRegOperand *rs1 = nullptr;
    if (imm->GetFloatVal() == 0.0f) {
      rs1 = getZeroReg(); // 如果是0.0，使用zero寄存器
    } else {
      rs1 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs1, imm->GetFloatVal());
      block->InsertInstruction(1, li_inst);
      auto fmvwx_inst = new RiscvFmvwxInstruction(rs1, rs1);
      block->InsertInstruction(2, fmvwx_inst);
    }
    auto flt_inst = new RiscvFltInstruction(result, rs2, rs1);
    block->InsertInstruction(1, flt_inst);
  } else if (inst->GetOp2()->isIMM()) {
    // 如果第二个操作数是浮点立即数
    auto imm =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp2()));
    auto rs1 = translateOperand(inst->GetOp1());
    RiscvRegOperand *rs2 = nullptr;
    if (imm->GetFloatVal() == 0.0f) {
      rs2 = getZeroReg(); // 如果是0.0，使用zero寄存器
    } else {
      rs2 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs2, imm->GetFloatVal());
      block->InsertInstruction(1, li_inst);
      auto fmvwx_inst = new RiscvFmvwxInstruction(rs2, rs2);
      block->InsertInstruction(2, fmvwx_inst);
    }
    auto flt_inst = new RiscvFltInstruction(result, rs1, rs2);
    block->InsertInstruction(1, flt_inst);
  } else {
    auto rs1 = translateOperand(inst->GetOp1());
    auto rs2 = translateOperand(inst->GetOp2());
    auto flt_inst = new RiscvFltInstruction(result, rs1, rs2);
    block->InsertInstruction(1, flt_inst);
  }
}

void Translator::translateFle(FcmpInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->GetResult());
  if (inst->GetOp1()->isIMM() && inst->GetOp2()->isIMM()) {
    // 如果两个操作数都是浮点立即数，直接计算结果
    auto op1 =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp1()));
    auto op2 =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp2()));
    auto li_inst = new RiscvLiInstruction(result, op1->GetFloatVal() <=
                                                      op2->GetFloatVal());
    block->InsertInstruction(1, li_inst);
  } else if (inst->GetOp1()->isIMM()) {
    // 如果第一个操作数是浮点立即数
    auto imm =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp1()));
    auto rs2 = translateOperand(inst->GetOp2());
    RiscvRegOperand *rs1 = nullptr;
    if (imm->GetFloatVal() == 0.0f) {
      rs1 = getZeroReg(); // 如果是0.0，使用zero寄存器
    } else {
      rs1 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs1, imm->GetFloatVal());
      block->InsertInstruction(1, li_inst);
      auto fmvwx_inst = new RiscvFmvwxInstruction(rs1, rs1);
      block->InsertInstruction(2, fmvwx_inst);
    }
    auto fle_inst = new RiscvFleInstruction(result, rs1, rs2);
    block->InsertInstruction(1, fle_inst);
  } else if (inst->GetOp2()->isIMM()) {
    // 如果第二个操作数是浮点立即数
    auto imm =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp2()));
    auto rs1 = translateOperand(inst->GetOp1());
    RiscvRegOperand *rs2 = nullptr;
    if (imm->GetFloatVal() == 0.0f) {
      rs2 = getZeroReg(); // 如果是0.0，使用zero寄存器
    } else {
      rs2 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs2, imm->GetFloatVal());
      block->InsertInstruction(1, li_inst);
      auto fmvwx_inst = new RiscvFmvwxInstruction(rs2, rs2);
      block->InsertInstruction(2, fmvwx_inst);
    }
    auto fle_inst = new RiscvFleInstruction(result, rs1, rs2);
    block->InsertInstruction(1, fle_inst);
  } else {
    // 否则使用FLE指令
    auto rs1 = translateOperand(inst->GetOp1());
    auto rs2 = translateOperand(inst->GetOp2());
    auto fle_inst = new RiscvFleInstruction(result, rs1, rs2);
    block->InsertInstruction(1, fle_inst);
  }
}

void Translator::translateFgt(FcmpInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->GetResult());
  if (inst->GetOp1()->isIMM() && inst->GetOp2()->isIMM()) {
    // 如果两个操作数都是浮点立即数，直接计算结果
    auto op1 =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp1()));
    auto op2 =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp2()));
    auto li_inst =
        new RiscvLiInstruction(result, op1->GetFloatVal() > op2->GetFloatVal());
    block->InsertInstruction(1, li_inst);
  } else if (inst->GetOp1()->isIMM()) {
    // 如果第一个操作数是浮点立即数
    auto imm =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp1()));
    auto rs2 = translateOperand(inst->GetOp2());
    RiscvRegOperand *rs1 = nullptr;
    if (imm->GetFloatVal() == 0.0f) {
      rs1 = getZeroReg(); // 如果是0.0，使用zero寄存器
    } else {
      rs1 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs1, imm->GetFloatVal());
      block->InsertInstruction(1, li_inst);
      auto fmvwx_inst = new RiscvFmvwxInstruction(rs1, rs1);
      block->InsertInstruction(2, fmvwx_inst);
    }
    auto fle_inst = new RiscvFleInstruction(result, rs1, rs2);
    block->InsertInstruction(1, fle_inst);
    auto xori_inst = new RiscvXoriInstruction(result, result, 1);
    block->InsertInstruction(2, xori_inst);
  } else if (inst->GetOp2()->isIMM()) {
    // 如果第二个操作数是浮点立即数
    auto imm =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp2()));
    auto rs1 = translateOperand(inst->GetOp1());
    RiscvRegOperand *rs2 = nullptr;
    if (imm->GetFloatVal() == 0.0f) {
      rs2 = getZeroReg(); // 如果是0.0，使用zero寄存器
    } else {
      rs2 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs2, imm->GetFloatVal());
      block->InsertInstruction(1, li_inst);
      auto fmvwx_inst = new RiscvFmvwxInstruction(rs2, rs2);
      block->InsertInstruction(2, fmvwx_inst);
    }
    auto fle_inst = new RiscvFleInstruction(result, rs2, rs1);
    block->InsertInstruction(1, fle_inst);
    auto xori_inst = new RiscvXoriInstruction(result, result, 1);
    block->InsertInstruction(2, xori_inst);
  } else {
    // 否则使用FGT指令
    auto rs1 = translateOperand(inst->GetOp1());
    auto rs2 = translateOperand(inst->GetOp2());
    auto fle_inst = new RiscvFleInstruction(result, rs1, rs2);
    block->InsertInstruction(1, fle_inst);
    auto xori_inst = new RiscvXoriInstruction(result, result, 1);
    block->InsertInstruction(2, xori_inst); // 取反结果
  }
}

void Translator::translateFge(FcmpInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->GetResult());
  if (inst->GetOp1()->isIMM() && inst->GetOp2()->isIMM()) {
    // 如果两个操作数都是浮点立即数，直接计算结果
    auto op1 =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp1()));
    auto op2 =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp2()));
    auto li_inst = new RiscvLiInstruction(result, op1->GetFloatVal() >=
                                                      op2->GetFloatVal());
    block->InsertInstruction(1, li_inst);
  } else if (inst->GetOp1()->isIMM()) {
    // 如果第一个操作数是浮点立即数
    auto imm =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp1()));
    auto rs2 = translateOperand(inst->GetOp2());
    RiscvRegOperand *rs1 = nullptr;
    if (imm->GetFloatVal() == 0.0f) {
      rs1 = getZeroReg(); // 如果是0.0，使用zero寄存器
    } else {
      rs1 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs1, imm->GetFloatVal());
      block->InsertInstruction(1, li_inst);
      auto fmvwx_inst = new RiscvFmvwxInstruction(rs1, rs1);
      block->InsertInstruction(2, fmvwx_inst);
    }
    auto flt_inst = new RiscvFltInstruction(result, rs1, rs2);
    block->InsertInstruction(1, flt_inst);
    auto xori_inst = new RiscvXoriInstruction(result, result, 1);
    block->InsertInstruction(2, xori_inst); // 取反结果
  } else if (inst->GetOp2()->isIMM()) {
    // 如果第二个操作数是浮点立即数
    auto imm =
        dynamic_cast<RiscvImmF32Operand *>(translateOperand(inst->GetOp2()));
    auto rs1 = translateOperand(inst->GetOp1());
    RiscvRegOperand *rs2 = nullptr;
    if (imm->GetFloatVal() == 0.0f) {
      rs2 = getZeroReg(); // 如果是0.0，使用zero寄存器
    } else {
      rs2 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs2, imm->GetFloatVal());
      block->InsertInstruction(1, li_inst);
      auto fmvwx_inst = new RiscvFmvwxInstruction(rs2, rs2);
      block->InsertInstruction(2, fmvwx_inst);
    }
    auto flt_inst = new RiscvFltInstruction(result, rs1, rs2);
    block->InsertInstruction(1, flt_inst);
    auto xori_inst = new RiscvXoriInstruction(result, result, 1);
    block->InsertInstruction(2, xori_inst); // 取反结果
  } else {
    // 否则使用FGE指令
    auto rs1 = translateOperand(inst->GetOp1());
    auto rs2 = translateOperand(inst->GetOp2());
    auto flt_inst = new RiscvFltInstruction(result, rs1, rs2);
    block->InsertInstruction(1, flt_inst);
    auto xori_inst = new RiscvXoriInstruction(result, result, 1);
    block->InsertInstruction(2, xori_inst); // 取反结果
  }
}

void Translator::translateAdd(Instruction inst, RiscvBlock *block) {
  auto *add_inst = dynamic_cast<ArithmeticInstruction *>(inst);
  if (!add_inst)
    return;
  if (add_inst->GetOp1()->isIMM() && add_inst->GetOp2()->isIMM()) {
    auto result = translateOperand(add_inst->GetResult());
    auto op1 = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(add_inst->GetOp1()));
    auto op2 = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(add_inst->GetOp2()));
    auto li_inst = new RiscvLiInstruction(result, op1->GetIntImmVal() +
                                                      op2->GetIntImmVal());
    block->InsertInstruction(1, li_inst);
  } else if (add_inst->GetOp2()->isIMM()) {
    // 如果第二个操作数是立即数，使用ADDI指令
    auto result = translateOperand(add_inst->GetResult());
    auto rs1 = translateOperand(add_inst->GetOp1());
    auto imm_op = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(add_inst->GetOp2()));
    int imm_val = imm_op->GetIntImmVal();
    insertAddiInstruction(result, rs1, imm_val, block);
  } else if (add_inst->GetOp1()->isIMM()) {
    // 如果第一个操作数是立即数，使用ADDI指令
    auto result = translateOperand(add_inst->GetResult());
    auto rs2 = translateOperand(add_inst->GetOp2());
    auto imm_op = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(add_inst->GetOp1()));
    int imm_val = imm_op->GetIntImmVal();
    insertAddiInstruction(result, rs2, imm_val, block);
  } else {
    // 否则使用ADD指令
    auto result = translateOperand(add_inst->GetResult());
    auto rs1 = translateOperand(add_inst->GetOp1());
    auto rs2 = translateOperand(add_inst->GetOp2());
    auto add_inst = new RiscvAddInstruction(result, rs1, rs2);
    block->InsertInstruction(1, add_inst);
  }
}

void Translator::translateMod(Instruction inst, RiscvBlock *block) {
  auto *mod_inst = dynamic_cast<ArithmeticInstruction *>(inst);
  if (!mod_inst)
    return;
  if (mod_inst->GetOp1()->isIMM() && mod_inst->GetOp2()->isIMM()) {
    auto result = translateOperand(mod_inst->GetResult());
    auto op1 = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(mod_inst->GetOp1()));
    auto op2 = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(mod_inst->GetOp2()));
    auto li_inst = new RiscvLiInstruction(result, op1->GetIntImmVal() %
                                                      op2->GetIntImmVal());
    block->InsertInstruction(1, li_inst);
  } else if (mod_inst->GetOp2()->isIMM()) {
    auto result = translateOperand(mod_inst->GetResult());
    auto rs1 = translateOperand(mod_inst->GetOp1());
    auto imm = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(mod_inst->GetOp2()));
    RiscvRegOperand *rs2 = nullptr;
    if (imm->GetIntImmVal() == 0) {
      rs2 = getZeroReg(); // 如果是0，使用zero寄存器
    } else {
      rs2 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs2, imm->GetIntImmVal());
      block->InsertInstruction(1, li_inst);
    }
    auto _mod_inst = new RiscvModInstruction(result, rs1, rs2);
    block->InsertInstruction(1, _mod_inst);
  } else if (mod_inst->GetOp1()->isIMM()) {
    auto result = translateOperand(mod_inst->GetResult());
    auto rs2 = translateOperand(mod_inst->GetOp2());
    auto imm = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(mod_inst->GetOp1()));
    RiscvRegOperand *rs1 = nullptr;
    if (imm->GetIntImmVal() == 0) {
      rs1 = getZeroReg(); // 如果是0，使用zero寄存器
    } else {
      rs1 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs1, imm->GetIntImmVal());
      block->InsertInstruction(1, li_inst);
    }
    auto _mod_inst = new RiscvModInstruction(std::move(result), std::move(rs1),
                                             std::move(rs2));
    block->InsertInstruction(1, _mod_inst);
  } else {
    // 否则使用MOD指令
    auto result = translateOperand(mod_inst->GetResult());
    auto rs1 = translateOperand(mod_inst->GetOp1());
    auto rs2 = translateOperand(mod_inst->GetOp2());
    auto mod_inst = new RiscvModInstruction(std::move(result), std::move(rs1),
                                            std::move(rs2));
    block->InsertInstruction(1, mod_inst);
  }
}

void Translator::translateSub(Instruction inst, RiscvBlock *block) {
  auto *sub_inst = dynamic_cast<ArithmeticInstruction *>(inst);
  if (!sub_inst)
    return;
  if (sub_inst->GetOp1()->isIMM() && sub_inst->GetOp2()->isIMM()) {
    auto result = translateOperand(sub_inst->GetResult());
    auto op1 = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(sub_inst->GetOp1()));
    auto op2 = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(sub_inst->GetOp2()));
    auto li_inst = new RiscvLiInstruction(result, op1->GetIntImmVal() -
                                                      op2->GetIntImmVal());
    block->InsertInstruction(1, li_inst);
  } else if (sub_inst->GetOp2()->isIMM()) {
    // 如果第二个操作数是立即数，使用ADDI指令
    auto result = translateOperand(sub_inst->GetResult());
    auto rs1 = translateOperand(sub_inst->GetOp1());
    auto imm = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(sub_inst->GetOp2()));
    insertAddiInstruction(result, rs1, -imm->GetIntImmVal(), block);
  } else if (sub_inst->GetOp1()->isIMM()) {
    auto result = translateOperand(sub_inst->GetResult());
    auto rs2 = translateOperand(sub_inst->GetOp2());
    auto imm = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(sub_inst->GetOp1()));
    RiscvRegOperand *rs1 = nullptr;
    if (imm->GetIntImmVal() == 0) {
      rs1 = getZeroReg(); // 如果是0，使用zero寄存器
    } else {
      rs1 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs1, imm->GetIntImmVal());
      block->InsertInstruction(1, li_inst);
    }
    auto sub_inst = new RiscvSubInstruction(result, rs1, rs2);
    block->InsertInstruction(1, sub_inst);
  } else {
    // 否则使用SUB指令
    auto result = translateOperand(sub_inst->GetResult());
    auto rs1 = translateOperand(sub_inst->GetOp1());
    auto rs2 = translateOperand(sub_inst->GetOp2());
    auto sub_inst = new RiscvSubInstruction(result, rs1, rs2);
    block->InsertInstruction(1, sub_inst);
  }
}

void Translator::translateMul(Instruction inst, RiscvBlock *block) {
  auto *mul_inst = dynamic_cast<ArithmeticInstruction *>(inst);
  if (!mul_inst)
    return;
  if (mul_inst->GetOp1()->isIMM() && mul_inst->GetOp2()->isIMM()) {
    auto result = translateOperand(mul_inst->GetResult());
    auto op1 = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(mul_inst->GetOp1()));
    auto op2 = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(mul_inst->GetOp2()));
    auto li_inst = new RiscvLiInstruction(result, op1->GetIntImmVal() *
                                                      op2->GetIntImmVal());
    block->InsertInstruction(1, li_inst);
  } else if (mul_inst->GetOp2()->isIMM()) {
    auto result = translateOperand(mul_inst->GetResult());
    auto rs1 = translateOperand(mul_inst->GetOp1());
    auto imm = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(mul_inst->GetOp2()));
    RiscvRegOperand *rs2 = nullptr;
    if (imm->GetIntImmVal() == 0) {
      rs2 = getZeroReg(); // 如果是0，使用zero寄存器
    } else {
      rs2 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs2, imm->GetIntImmVal());
      block->InsertInstruction(1, li_inst);
    }
    auto _mul_inst = new RiscvMulInstruction(result, rs1, rs2);
    block->InsertInstruction(1, _mul_inst);
  } else if (mul_inst->GetOp1()->isIMM()) {
    auto result = translateOperand(mul_inst->GetResult());
    auto rs2 = translateOperand(mul_inst->GetOp2());
    auto imm = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(mul_inst->GetOp1()));
    RiscvRegOperand *rs1 = nullptr;
    if (imm->GetIntImmVal() == 0) {
      rs1 = getZeroReg(); // 如果是0，使用zero寄存器
    } else {
      rs1 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs1, imm->GetIntImmVal());
      block->InsertInstruction(1, li_inst);
    }
    auto mul_inst = new RiscvMulInstruction(result, rs1, rs2);
    block->InsertInstruction(1, mul_inst);
  } else {
    // 否则使用MUL指令
    auto result = translateOperand(mul_inst->GetResult());
    auto rs1 = translateOperand(mul_inst->GetOp1());
    auto rs2 = translateOperand(mul_inst->GetOp2());
    auto mul_inst = new RiscvMulInstruction(result, rs1, rs2);
    block->InsertInstruction(1, mul_inst);
  }
}

void Translator::translateDiv(Instruction inst, RiscvBlock *block) {
  auto *div_inst = dynamic_cast<ArithmeticInstruction *>(inst);
  if (!div_inst)
    return;
  if (div_inst->GetOp1()->isIMM() && div_inst->GetOp2()->isIMM()) {
    auto result = translateOperand(div_inst->GetResult());
    auto op1 = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(div_inst->GetOp1()));
    auto op2 = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(div_inst->GetOp2()));
    auto li_inst = new RiscvLiInstruction(result, op1->GetIntImmVal() /
                                                      op2->GetIntImmVal());
    block->InsertInstruction(1, li_inst);
  } else if (div_inst->GetOp2()->isIMM()) {
    auto result = translateOperand(div_inst->GetResult());
    auto rs1 = translateOperand(div_inst->GetOp1());
    auto imm = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(div_inst->GetOp2()));
    auto rs2 = createVirtualReg();
    auto li_inst = new RiscvLiInstruction(rs2, imm->GetIntImmVal());
    auto _div_inst = new RiscvDivInstruction(result, rs1, rs2);
    block->InsertInstruction(1, li_inst);
    block->InsertInstruction(1, _div_inst);
  } else if (div_inst->GetOp1()->isIMM()) {
    auto result = translateOperand(div_inst->GetResult());
    auto rs2 = translateOperand(div_inst->GetOp2());
    auto imm = dynamic_cast<RiscvImmI32Operand *>(
        translateOperand(div_inst->GetOp1()));
    RiscvRegOperand *rs1 = nullptr;
    if (imm->GetIntImmVal() == 0) {
      rs1 = getZeroReg(); // 如果是0，使用zero寄存器
    } else {
      rs1 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs1, imm->GetIntImmVal());
      block->InsertInstruction(1, li_inst);
    }
    auto div_inst = new RiscvDivInstruction(result, rs1, rs2);
    block->InsertInstruction(1, div_inst);
  } else {
    // 否则使用DIV指令
    auto result = translateOperand(div_inst->GetResult());
    auto rs1 = translateOperand(div_inst->GetOp1());
    auto rs2 = translateOperand(div_inst->GetOp2());
    auto div_inst = new RiscvDivInstruction(result, rs1, rs2);
    block->InsertInstruction(1, div_inst);
  }
}

void Translator::translateFadd(Instruction inst, RiscvBlock *block) {
  auto *fadd_inst = dynamic_cast<ArithmeticInstruction *>(inst);
  if (!fadd_inst)
    return;
  if (fadd_inst->GetOp1()->isIMM() && fadd_inst->GetOp2()->isIMM()) {
    auto result = translateOperand(fadd_inst->GetResult());
    auto op1 = dynamic_cast<RiscvImmF32Operand *>(
        translateOperand(fadd_inst->GetOp1()));
    auto op2 = dynamic_cast<RiscvImmF32Operand *>(
        translateOperand(fadd_inst->GetOp2()));
    auto tmp_reg = createVirtualReg();
    auto li_inst = new RiscvLiInstruction(tmp_reg, op1->GetFloatVal() +
                                                       op2->GetFloatVal());
    block->InsertInstruction(1, li_inst);
    auto fmvwx_inst = new RiscvFmvwxInstruction(result, tmp_reg);
    block->InsertInstruction(1, fmvwx_inst);
  } else if (fadd_inst->GetOp2()->isIMM()) {
    auto result = translateOperand(fadd_inst->GetResult());
    auto rs1 = translateOperand(fadd_inst->GetOp1());
    auto imm = dynamic_cast<RiscvImmF32Operand *>(
        translateOperand(fadd_inst->GetOp2()));
    auto li_reg = createVirtualReg();
    auto rs2 = createVirtualReg();
    auto li_inst = new RiscvLiInstruction(li_reg, imm->GetFloatVal());
    auto fmvwx_inst = new RiscvFmvwxInstruction(rs2, li_reg);
    auto fadd_inst = new RiscvFAddInstruction(result, rs1, rs2);
    block->InsertInstruction(1, li_inst);
    block->InsertInstruction(1, fmvwx_inst);
    block->InsertInstruction(1, fadd_inst);
  } else if (fadd_inst->GetOp1()->isIMM()) {
    auto result = translateOperand(fadd_inst->GetResult());
    auto rs2 = translateOperand(fadd_inst->GetOp2());
    auto imm = dynamic_cast<RiscvImmF32Operand *>(
        translateOperand(fadd_inst->GetOp1()));
    auto li_reg = createVirtualReg();
    auto rs1 = createVirtualReg();
    auto li_inst = new RiscvLiInstruction(li_reg, imm->GetFloatVal());
    auto fmvwx_inst = new RiscvFmvwxInstruction(rs1, li_reg);
    auto fadd_inst = new RiscvFAddInstruction(result, rs1, rs2);
    block->InsertInstruction(1, li_inst);
    block->InsertInstruction(1, fmvwx_inst);
    block->InsertInstruction(1, fadd_inst);
  } else {
    // 否则使用FADD指令
    auto result = translateOperand(fadd_inst->GetResult());
    auto rs1 = translateOperand(fadd_inst->GetOp1());
    auto rs2 = translateOperand(fadd_inst->GetOp2());
    auto fadd_inst = new RiscvFAddInstruction(result, rs1, rs2);
    block->InsertInstruction(1, fadd_inst);
  }
}

void Translator::translateFsub(Instruction inst, RiscvBlock *block) {
  auto *fsub_inst = dynamic_cast<ArithmeticInstruction *>(inst);
  if (!fsub_inst)
    return;
  if (fsub_inst->GetOp1()->isIMM() && fsub_inst->GetOp2()->isIMM()) {
    auto result = translateOperand(fsub_inst->GetResult());
    auto op1 = dynamic_cast<RiscvImmF32Operand *>(
        translateOperand(fsub_inst->GetOp1()));
    auto op2 = dynamic_cast<RiscvImmF32Operand *>(
        translateOperand(fsub_inst->GetOp2()));
    auto li_reg = createVirtualReg();
    auto li_inst =
        new RiscvLiInstruction(li_reg, op1->GetFloatVal() - op2->GetFloatVal());
    block->InsertInstruction(1, li_inst);
    auto fmv_inst = new RiscvFmvwxInstruction(result, li_reg);
    block->InsertInstruction(1, fmv_inst);
  } else if (fsub_inst->GetOp2()->isIMM()) {
    auto result = translateOperand(fsub_inst->GetResult());
    auto rs1 = translateOperand(fsub_inst->GetOp1());
    auto imm = dynamic_cast<RiscvImmF32Operand *>(
        translateOperand(fsub_inst->GetOp2()));
    auto li_reg = createVirtualReg();
    auto rs2 = createVirtualReg();
    auto li_inst = new RiscvLiInstruction(li_reg, imm->GetFloatVal());
    auto fmv_inst = new RiscvFmvwxInstruction(rs2, li_reg);
    auto fsub_inst = new RiscvFSubInstruction(result, rs1, rs2);
    block->InsertInstruction(1, li_inst);
    block->InsertInstruction(1, fmv_inst);
    block->InsertInstruction(1, fsub_inst);
  } else if (fsub_inst->GetOp1()->isIMM()) {
    auto result = translateOperand(fsub_inst->GetResult());
    auto rs2 = translateOperand(fsub_inst->GetOp2());
    auto imm = dynamic_cast<RiscvImmF32Operand *>(
        translateOperand(fsub_inst->GetOp1()));
    auto li_reg = createVirtualReg();
    auto rs1 = createVirtualReg();
    auto li_inst = new RiscvLiInstruction(li_reg, imm->GetFloatVal());
    auto fmv_inst = new RiscvFmvwxInstruction(rs1, li_reg);
    auto fsub_inst = new RiscvFSubInstruction(result, rs1, rs2);
    block->InsertInstruction(1, li_inst);
    block->InsertInstruction(1, fmv_inst);
    block->InsertInstruction(1, fsub_inst);
  } else {
    // 否则使用FSUB指令
    auto result = translateOperand(fsub_inst->GetResult());
    auto rs1 = translateOperand(fsub_inst->GetOp1());
    auto rs2 = translateOperand(fsub_inst->GetOp2());
    auto fsub_inst = new RiscvFSubInstruction(result, rs1, rs2);
    block->InsertInstruction(1, fsub_inst);
  }
}

void Translator::translateFmul(Instruction inst, RiscvBlock *block) {
  auto *fmul_inst = dynamic_cast<ArithmeticInstruction *>(inst);
  if (!fmul_inst)
    return;
  if (fmul_inst->GetOp1()->isIMM() && fmul_inst->GetOp2()->isIMM()) {
    auto result = translateOperand(fmul_inst->GetResult());
    auto op1 = dynamic_cast<RiscvImmF32Operand *>(
        translateOperand(fmul_inst->GetOp1()));
    auto op2 = dynamic_cast<RiscvImmF32Operand *>(
        translateOperand(fmul_inst->GetOp2()));
    auto li_reg = createVirtualReg();
    auto li_inst =
        new RiscvLiInstruction(li_reg, op1->GetFloatVal() * op2->GetFloatVal());
    block->InsertInstruction(1, li_inst);
    auto fmv_inst = new RiscvFmvwxInstruction(result, li_reg);
    block->InsertInstruction(1, fmv_inst);
  } else if (fmul_inst->GetOp2()->isIMM()) {
    auto result = translateOperand(fmul_inst->GetResult());
    auto rs1 = translateOperand(fmul_inst->GetOp1());
    auto imm = dynamic_cast<RiscvImmF32Operand *>(
        translateOperand(fmul_inst->GetOp2()));
    auto li_reg = createVirtualReg();
    auto rs2 = createVirtualReg();
    auto li_inst = new RiscvLiInstruction(li_reg, imm->GetFloatVal());
    auto fmv_inst = new RiscvFmvwxInstruction(rs2, li_reg);
    auto fmul_inst = new RiscvFMulInstruction(result, rs1, rs2);
    block->InsertInstruction(1, li_inst);
    block->InsertInstruction(1, fmv_inst);
    block->InsertInstruction(1, fmul_inst);
  } else if (fmul_inst->GetOp1()->isIMM()) {
    auto result = translateOperand(fmul_inst->GetResult());
    auto rs2 = translateOperand(fmul_inst->GetOp2());
    auto imm = dynamic_cast<RiscvImmF32Operand *>(
        translateOperand(fmul_inst->GetOp1()));
    auto li_reg = createVirtualReg();
    auto rs1 = createVirtualReg();
    auto li_inst = new RiscvLiInstruction(li_reg, imm->GetFloatVal());
    auto fmv_inst = new RiscvFmvwxInstruction(rs1, li_reg);
    auto fmul_inst = new RiscvFMulInstruction(result, rs1, rs2);
    block->InsertInstruction(1, li_inst);
    block->InsertInstruction(1, fmv_inst);
    block->InsertInstruction(1, fmul_inst);
  } else {
    // 否则使用FMUL指令
    auto result = translateOperand(fmul_inst->GetResult());
    auto rs1 = translateOperand(fmul_inst->GetOp1());
    auto rs2 = translateOperand(fmul_inst->GetOp2());
    auto fmul_inst = new RiscvFMulInstruction(std::move(result), std::move(rs1),
                                              std::move(rs2));
    block->InsertInstruction(1, fmul_inst);
  }
}

void Translator::translateFdiv(Instruction inst, RiscvBlock *block) {
  auto *fdiv_inst = dynamic_cast<ArithmeticInstruction *>(inst);
  if (!fdiv_inst)
    return;
  if (fdiv_inst->GetOp1()->isIMM() && fdiv_inst->GetOp2()->isIMM()) {
    auto result = translateOperand(fdiv_inst->GetResult());
    auto op1 = dynamic_cast<RiscvImmF32Operand *>(
        translateOperand(fdiv_inst->GetOp1()));
    auto op2 = dynamic_cast<RiscvImmF32Operand *>(
        translateOperand(fdiv_inst->GetOp2()));
    auto li_reg = createVirtualReg();
    auto li_inst =
        new RiscvLiInstruction(li_reg, op1->GetFloatVal() / op2->GetFloatVal());
    block->InsertInstruction(1, li_inst);
    auto fmv_inst = new RiscvFmvwxInstruction(result, li_reg);
    block->InsertInstruction(1, fmv_inst);
  } else if (fdiv_inst->GetOp2()->isIMM()) {
    auto result = translateOperand(fdiv_inst->GetResult());
    auto rs1 = translateOperand(fdiv_inst->GetOp1());
    auto imm = dynamic_cast<RiscvImmF32Operand *>(
        translateOperand(fdiv_inst->GetOp2()));
    auto li_reg = createVirtualReg();
    auto rs2 = createVirtualReg();
    auto li_inst = new RiscvLiInstruction(li_reg, imm->GetFloatVal());
    auto fmv_inst = new RiscvFmvwxInstruction(rs2, li_reg);
    auto fdiv_inst = new RiscvFDivInstruction(result, rs1, rs2);
    block->InsertInstruction(1, li_inst);
    block->InsertInstruction(1, fmv_inst);
    block->InsertInstruction(1, fdiv_inst);
  } else if (fdiv_inst->GetOp1()->isIMM()) {
    auto result = translateOperand(fdiv_inst->GetResult());
    auto rs2 = translateOperand(fdiv_inst->GetOp2());
    auto imm = dynamic_cast<RiscvImmF32Operand *>(
        translateOperand(fdiv_inst->GetOp1()));
    auto li_reg = createVirtualReg();
    auto rs1 = createVirtualReg();
    auto li_inst = new RiscvLiInstruction(li_reg, imm->GetFloatVal());
    auto fmv_inst = new RiscvFmvwxInstruction(rs1, li_reg);
    auto fdiv_inst = new RiscvFDivInstruction(result, rs1, rs2);
    block->InsertInstruction(1, li_inst);
    block->InsertInstruction(1, fmv_inst);
    block->InsertInstruction(1, fdiv_inst);
  } else {
    // 否则使用FDIV指令
    auto result = translateOperand(fdiv_inst->GetResult());
    auto rs1 = translateOperand(fdiv_inst->GetOp1());
    auto rs2 = translateOperand(fdiv_inst->GetOp2());
    auto fdiv_inst = new RiscvFDivInstruction(result, rs1, rs2);
    block->InsertInstruction(1, fdiv_inst);
  }
}

void Translator::translateGetElementptr(GetElementptrInstruction *inst,
                                        RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->GetResult());
  RiscvRegOperand *base_addr = nullptr;
  RiscvRegOperand *offset_reg = createVirtualReg();
  int offset = 0; // 偏移量初始化为0
  auto indexes = inst->GetIndexes();
  auto dim = inst->GetDims();
  auto li_inst = new RiscvLiInstruction(offset_reg, 0);
  block->InsertInstruction(1, li_inst);
  for (size_t i = 0; i < indexes.size(); ++i) {
    int size = 1;
    for (size_t j = i + 1; j < dim.size(); ++j)
      size *= dim[j];
    auto index = indexes[i];
    if (index->GetOperandType() == BasicOperand::IMMI32) {
      auto imm_op = dynamic_cast<ImmI32Operand *>(index);
      insertAddiInstruction(offset_reg, offset_reg,
                            imm_op->GetIntImmVal() * size, block);
    } else if (index->GetOperandType() == BasicOperand::REG) {
      auto tmp_reg = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(tmp_reg, size);
      auto mul_inst =
          new RiscvMulInstruction(tmp_reg, tmp_reg, translateOperand(index));
      auto add_inst = new RiscvAddInstruction(offset_reg, offset_reg, tmp_reg);
      block->InsertInstruction(1, li_inst);
      block->InsertInstruction(2, mul_inst);
      block->InsertInstruction(3, add_inst);
    } else {
      std::cerr << "Unsupported index type in GetElementptr.\n";
      return;
    }
  }
  auto slli_inst = new RiscvSlliInstruction(offset_reg, offset_reg,
                                            2); // 假设每个元素大小为4字节
  block->InsertInstruction(1, slli_inst);
  if (inst->GetPtrVal()->GetOperandType() == BasicOperand::GLOBAL) {
    auto ptrval = dynamic_cast<GlobalOperand *>(inst->GetPtrVal());
    auto global_op = new RiscvGlobalOperand(ptrval->GetName());
    base_addr = createVirtualReg();
    auto la_inst = new RiscvLaInstruction(base_addr, global_op);
    block->InsertInstruction(1, la_inst);
  } else {
    auto ptrval = dynamic_cast<RegOperand *>(inst->GetPtrVal());
    int ptr_reg_no = ptrval->GetRegNo();

    // 首先检查是否是数组（在array_offsets中）
    auto array_it = current_stack_frame->array_offsets.find(ptr_reg_no);
    if (array_it != current_stack_frame->array_offsets.end()) {
      // 这是栈上的数组，直接使用其地址（由translateAlloca计算）
      base_addr =
          dynamic_cast<RiscvRegOperand *>(translateOperand(inst->GetPtrVal()));
      offset = 0;
    } else {
      // 检查是否是栈中的指针变量
      auto var_it = current_stack_frame->var_offsets.find(ptr_reg_no);
      if (var_it != current_stack_frame->var_offsets.end()) {
        // 指针存储在栈中，需要先加载指针值
        offset = var_it->second;
        base_addr = createVirtualReg();
        auto s0_reg = getS0Reg();
        auto ptr_operand = new RiscvPtrOperand(-offset, s0_reg);
        insertLwInstruction(base_addr, ptr_operand, block);
      } else {
        // 指针就是虚拟寄存器值，直接使用
        base_addr = dynamic_cast<RiscvRegOperand *>(
            translateOperand(inst->GetPtrVal()));
        offset = 0;
      }
    }
  }
  auto add_inst =
      new RiscvAddInstruction(result, base_addr, offset_reg); // 计算最终地址
  block->InsertInstruction(1, add_inst);
  int reg_no = dynamic_cast<RiscvRegOperand *>(result)->GetRegNo();
  current_stack_frame->array_offsets[reg_no] = offset; // 记录数组偏移
}

void Translator::translateFptosi(FptosiInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->result);
  auto value = translateOperand(inst->value);
  auto fcvtws_inst = new RiscvFcvtwsInstruction(result, value);
  block->InsertInstruction(1, fcvtws_inst);
}

void Translator::translateSitofp(SitofpInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->result);
  auto value = translateOperand(inst->value);
  auto fcvtsw_inst = new RiscvFcvtswInstruction(result, value);
  block->InsertInstruction(1, fcvtsw_inst);
}

void Translator::translateBr_uncond(BrUncondInstruction *inst,
                                    RiscvBlock *block) {
  if (!inst)
    return;
  auto label_op =
      dynamic_cast<RiscvLabelOperand *>(translateOperand(inst->GetLabel()));
  auto br_inst = new RiscvJInstruction(label_op);
  block->InsertInstruction(1, br_inst);
}

void Translator::translateBr_cond(BrCondInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto cond_op = translateOperand(inst->GetCond());
  auto true_label_op =
      dynamic_cast<RiscvLabelOperand *>(translateOperand(inst->GetTrueLabel()));
  auto false_label_op = dynamic_cast<RiscvLabelOperand *>(
      translateOperand(inst->GetFalseLabel()));
  auto bnez_inst = new RiscvBnezInstruction(cond_op, true_label_op);
  block->InsertInstruction(1, bnez_inst);
  auto j_inst = new RiscvJInstruction(false_label_op);
  block->InsertInstruction(1, j_inst);
}

void Translator::translateAnd(ArithmeticInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->GetResult());
  if (inst->GetOp1()->isIMM() && inst->GetOp2()->isIMM()) {
    // 如果两个操作数都是立即数，直接计算结果
    auto imm1 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    auto imm2 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    auto li_inst = new RiscvLiInstruction(result, imm1->GetIntImmVal() &&
                                                      imm2->GetIntImmVal());
    block->InsertInstruction(1, li_inst);
  } else if (inst->GetOp1()->isIMM()) {
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    if (imm->GetIntImmVal() == 0) {
      // 如果第一个操作数是0，结果也是0
      auto li_inst = new RiscvLiInstruction(result, 0);
      block->InsertInstruction(1, li_inst);
    } else {
      auto rs2 = translateOperand(inst->GetOp2());
      auto snez_inst =
          new RiscvSnezInstruction(result, rs2); // 如果第一个操作数是非0立即数
      block->InsertInstruction(1, snez_inst);
    }
  } else if (inst->GetOp2()->isIMM()) {
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    if (imm->GetIntImmVal() == 0) {
      // 如果第二个操作数是0，结果也是0
      auto li_inst = new RiscvLiInstruction(result, 0);
      block->InsertInstruction(1, li_inst);
    } else {
      auto rs1 = translateOperand(inst->GetOp1());
      auto snez_inst =
          new RiscvSnezInstruction(result, rs1); // 如果第二个操作数是非0立即数
      block->InsertInstruction(1, snez_inst);
    }
  } else {
    // 否则使用AND指令
    auto rs1 = translateOperand(inst->GetOp1());
    auto rs2 = translateOperand(inst->GetOp2());
    auto op1 = createVirtualReg();
    auto op2 = createVirtualReg();
    auto snez_inst = new RiscvSnezInstruction(op1, rs1);
    auto snez_inst2 = new RiscvSnezInstruction(op2, rs2);
    auto and_inst = new RiscvAndInstruction(result, op1, op2);
    block->InsertInstruction(1, snez_inst);
    block->InsertInstruction(2, snez_inst2);
    block->InsertInstruction(3, and_inst);
  }
}

void Translator::translateOr(ArithmeticInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->GetResult());
  if (inst->GetOp1()->isIMM() && inst->GetOp2()->isIMM()) {
    // 如果两个操作数都是立即数，直接计算结果
    auto imm1 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    auto imm2 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    auto li_inst = new RiscvLiInstruction(result, imm1->GetIntImmVal() ||
                                                      imm2->GetIntImmVal());
    block->InsertInstruction(1, li_inst);
  } else if (inst->GetOp1()->isIMM()) {
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    if (imm->GetIntImmVal() != 0) {
      auto li_inst = new RiscvLiInstruction(result, 1);
      block->InsertInstruction(1, li_inst);
    } else {
      auto rs2 = translateOperand(inst->GetOp2());
      auto snez_inst =
          new RiscvSnezInstruction(result, rs2); // 如果第一个操作数是非0立即数
      block->InsertInstruction(1, snez_inst);
    }
  } else if (inst->GetOp2()->isIMM()) {
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    if (imm->GetIntImmVal() != 0) {
      auto li_inst = new RiscvLiInstruction(result, 1);
      block->InsertInstruction(1, li_inst);
    } else {
      auto rs1 = translateOperand(inst->GetOp1());
      auto snez_inst =
          new RiscvSnezInstruction(result, rs1); // 如果第二个操作数是非0立即数
      block->InsertInstruction(1, snez_inst);
    }
  } else {
    // 否则使用AND指令
    auto rs1 = translateOperand(inst->GetOp1());
    auto rs2 = translateOperand(inst->GetOp2());
    auto op1 = createVirtualReg();
    auto op2 = createVirtualReg();
    auto snez_inst = new RiscvSnezInstruction(op1, rs1);
    auto snez_inst2 = new RiscvSnezInstruction(op2, rs2);
    auto and_inst = new RiscvOrInstruction(result, op1, op2);
    block->InsertInstruction(1, snez_inst);
    block->InsertInstruction(2, snez_inst2);
    block->InsertInstruction(3, and_inst);
  }
}

void Translator::translateZext(ZextInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->result);
  auto value = translateOperand(inst->value);
  auto andi_inst = new RiscvAndiInstruction(result, value, 1);
  block->InsertInstruction(1, andi_inst);
}

void Translator::translateAshr(ArithmeticInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->GetResult());
  if (inst->GetOp1()->isIMM() && inst->GetOp2()->isIMM()) {
    // 如果两个操作数都是立即数，直接计算结果
    auto imm1 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    auto imm2 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    auto li_inst = new RiscvLiInstruction(result, imm1->GetIntImmVal() >>
                                                      imm2->GetIntImmVal());
    block->InsertInstruction(1, li_inst);
  } else if (inst->GetOp1()->isIMM()) {
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    if (imm->GetIntImmVal() == 0) {
      // 如果第一个操作数是0，结果也是0
      auto li_inst = new RiscvLiInstruction(result, 0);
      block->InsertInstruction(1, li_inst);
    } else {
      auto rs2 = translateOperand(inst->GetOp2());
      auto rs1 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs1, imm->GetIntImmVal());
      auto srai_inst = new RiscvSraInstruction(result, rs1, rs2);
      block->InsertInstruction(1, li_inst);
      block->InsertInstruction(1, srai_inst);
    }
  } else if (inst->GetOp2()->isIMM()) {
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    if (imm->GetIntImmVal() == 0) {
      // 如果第二个操作数是0，结果也是0
      auto rs1 = translateOperand(inst->GetOp1());
      auto mv_inst = new RiscvMvInstruction(result, rs1);
      block->InsertInstruction(1, mv_inst);
    } else {
      auto rs1 = translateOperand(inst->GetOp1());
      auto srai_inst =
          new RiscvSraiInstruction(result, rs1, imm->GetIntImmVal());
      block->InsertInstruction(1, srai_inst);
    }
  } else {
    auto rs1 = translateOperand(inst->GetOp1());
    auto rs2 = translateOperand(inst->GetOp2());
    auto srai_inst = new RiscvSraInstruction(result, rs1, rs2);
    block->InsertInstruction(1, srai_inst);
  }
}

void Translator::translateLshr(ArithmeticInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->GetResult());
  if (inst->GetOp1()->isIMM() && inst->GetOp2()->isIMM()) {
    // 如果两个操作数都是立即数，直接计算结果
    auto imm1 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    auto imm2 =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    auto li_inst = new RiscvLiInstruction(
        result,
        (int)(((unsigned int)imm1->GetIntImmVal()) >> imm2->GetIntImmVal()));
    block->InsertInstruction(1, li_inst);
  } else if (inst->GetOp1()->isIMM()) {
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp1()));
    if (imm->GetIntImmVal() == 0) {
      // 如果第一个操作数是0，结果也是0
      auto li_inst = new RiscvLiInstruction(result, 0);
      block->InsertInstruction(1, li_inst);
    } else {
      auto rs2 = translateOperand(inst->GetOp2());
      auto rs1 = createVirtualReg();
      auto li_inst = new RiscvLiInstruction(rs1, imm->GetIntImmVal());
      auto srli_inst = new RiscvSrlInstruction(result, rs1, rs2);
      block->InsertInstruction(1, li_inst);
      block->InsertInstruction(1, srli_inst);
    }
  } else if (inst->GetOp2()->isIMM()) {
    auto imm =
        dynamic_cast<RiscvImmI32Operand *>(translateOperand(inst->GetOp2()));
    if (imm->GetIntImmVal() == 0) {
      auto rs1 = translateOperand(inst->GetOp1());
      auto mv_inst = new RiscvMvInstruction(result, rs1);
      block->InsertInstruction(1, mv_inst);
    } else {
      auto rs1 = translateOperand(inst->GetOp1());
      auto srli_inst =
          new RiscvSrliInstruction(result, rs1, imm->GetIntImmVal());
      block->InsertInstruction(1, srli_inst);
    }
  } else {
    auto rs1 = translateOperand(inst->GetOp1());
    auto rs2 = translateOperand(inst->GetOp2());
    auto srl_inst = new RiscvSrlInstruction(result, rs1, rs2);
    block->InsertInstruction(1, srl_inst);
  }
}

void Translator::translateTrunc(TruncInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  auto result = translateOperand(inst->result);
  auto value = translateOperand(inst->value);
  auto slli_inst = new RiscvSlliInstruction(result, value, 32);
  block->InsertInstruction(1, slli_inst);
  auto srli_inst = new RiscvSrliInstruction(result, result, 32);
  block->InsertInstruction(1, srli_inst);
}

void Translator::translatePhi(PhiInstruction *inst, RiscvBlock *block) {
  if (!inst)
    return;
  // 收集phi指令信息，延迟处理
  PhiTranslationInfo phi_info;
  phi_info.block_id = block->block_id;
  phi_info.result = translateOperand(inst->GetResult());
  // 解析phi指令的参数列表
  for (const auto &phi_pair : inst->phi_list) {
    RiscvOperand *value = translateOperand(phi_pair.second);
    // 从标签操作数中提取前驱块ID
    int pred_block_id = -1;
    if (phi_pair.first->GetOperandType() == BasicOperand::LABEL) {
      LabelOperand *label_op = dynamic_cast<LabelOperand *>(phi_pair.first);
      if (label_op)
        pred_block_id = label_op->GetLabelNo();
    }
    if (pred_block_id != -1)
      phi_info.phi_args.push_back(std::make_pair(value, pred_block_id));
  }
  // 将phi指令信息添加到待处理列表中
  pending_phi_instructions.push_back(phi_info);
}

void Translator::processPendingPhis() {
  // 处理所有待处理的phi指令
  for (const auto &phi_info : pending_phi_instructions)
    insertPhiMoveInstructions(phi_info);
  // 清空待处理列表
  pending_phi_instructions.clear();
}

void Translator::insertPhiMoveInstructions(const PhiTranslationInfo &phi_info) {
  // 为每个前驱块插入mov指令
  for (const auto &phi_arg : phi_info.phi_args) {
    RiscvOperand *value = phi_arg.first;
    int pred_block_id = phi_arg.second;
    // 查找前驱块
    auto pred_block_it =
        riscv.function_block_map[current_function].find(pred_block_id);
    if (pred_block_it == riscv.function_block_map[current_function].end()) {
      std::cerr << "Warning: Predecessor block " << pred_block_id
                << " not found for phi instruction in block "
                << phi_info.block_id << std::endl;
      continue;
    }

    RiscvBlock *pred_block = pred_block_it->second;
    // 检查phi_info.result和value的类型，选择合适的指令
    RiscvOperand *result_copy = phi_info.result->CopyOperand();

    // 根据操作数类型选择合适的指令
    if (value->GetOperandType() == RiscvOperand::REG &&
        result_copy->GetOperandType() == RiscvOperand::REG) {
      // 寄存器到寄存器的移动
      auto mv_inst = new RiscvMvInstruction(result_copy, value->CopyOperand());
      insertBeforeTerminator(pred_block, mv_inst);
    } else if (value->GetOperandType() == RiscvOperand::IMMI32) {
      // 立即数到寄存器
      RiscvImmI32Operand *imm_op = dynamic_cast<RiscvImmI32Operand *>(value);
      if (imm_op) {
        auto li_inst =
            new RiscvLiInstruction(result_copy, imm_op->GetIntImmVal());
        insertBeforeTerminator(pred_block, li_inst);
      }
    } else if (value->GetOperandType() == RiscvOperand::IMMF32) {
      // 浮点立即数到寄存器
      RiscvImmF32Operand *float_imm_op =
          dynamic_cast<RiscvImmF32Operand *>(value);
      if (float_imm_op) {
        auto li_inst =
            new RiscvLiInstruction(result_copy, float_imm_op->GetFloatVal());
        insertBeforeTerminator(pred_block, li_inst);
      }
    } else {
      // 其他情况，使用通用的add指令（rd = rs1 + 0）
      auto zero_reg = getZeroReg();
      auto add_inst =
          new RiscvAddInstruction(result_copy, value->CopyOperand(), zero_reg);
      insertBeforeTerminator(pred_block, add_inst);
    }
  }
}

bool Translator::isRiscvTerminatorInstruction(RiscvInstruction *inst) {
  if (!inst)
    return false;

  RiscvOpcode opcode = inst->GetOpcode();

  // 检查是否是终止指令
  return (opcode == RiscvOpcode::J ||    // 无条件跳转
          opcode == RiscvOpcode::JR ||   // 寄存器跳转
          opcode == RiscvOpcode::BNEZ || // 分支指令
          dynamic_cast<RiscvBranchInstruction *>(inst) !=
              nullptr); // 其他分支指令
}

void Translator::insertBeforeTerminator(RiscvBlock *block,
                                        RiscvInstruction *inst) {
  if (!block || !inst)
    return;
  // 检查最后一个指令是否为终止指令
  // std::cout << "Insert inst" << block->block_id << ": ";
  // inst->PrintIR(std::cout);
  if (!block->instruction_list.empty()) {
    RiscvInstruction *last_inst = block->instruction_list.back();
    auto it = block->instruction_list.end();
    --it; // 指向最后一个指令
    while (it >= block->instruction_list.begin() &&
           isRiscvTerminatorInstruction(*it)) {
      --it; // 向前查找直到找到非终止指令
    }
    ++it;
    block->instruction_list.insert(it, inst);
  } else {
    // 空块，直接添加
    block->instruction_list.push_back(inst);
  }
}

RiscvRegOperand *Translator::createVirtualReg() {
  return new RiscvRegOperand(++next_virtual_reg_no); // 返回一个新的虚拟寄存器
}

RiscvRegOperand *Translator::createVirtualReg(int reg_no) {
  return new RiscvRegOperand(reg_no); // 正数表示虚拟寄存器
}

RiscvRegOperand *Translator::getS0Reg() {
  return new RiscvRegOperand(-8); // S0寄存器(x8)作为帧指针
}

RiscvRegOperand *Translator::getSpReg() {
  return new RiscvRegOperand(-3); // Sp寄存器作为帧指针
}

RiscvRegOperand *Translator::getRaReg() {
  return new RiscvRegOperand(-2); // ra寄存器作为帧指针
}

RiscvRegOperand *Translator::getA0Reg() {
  return new RiscvRegOperand(-10); // a0寄存器 (x10)
}

// 获取参数寄存器 a0-a7
RiscvRegOperand *Translator::getArgReg(int arg_index) {
  if (arg_index >= 0 && arg_index < 8) {
    return new RiscvRegOperand(
        -(10 + arg_index)); // a0=x10, a1=x11, ..., a7=x17
  }
  throw std::runtime_error("参数索引超出范围，只支持a0-a7");
}

RiscvRegOperand *Translator::getZeroReg() {
  return new RiscvRegOperand(-1); // zero寄存器
}

RiscvRegOperand *Translator::getFa0Reg() {
  return new RiscvRegOperand(-43); // fa0寄存器作为帧指针
}

RiscvOperand *Translator::translateOperand(Operand op) {
  if (auto *reg_op = dynamic_cast<RegOperand *>(op))
    return createVirtualReg(reg_op->GetRegNo());
  else if (auto *imm_i32 = dynamic_cast<ImmI32Operand *>(op))
    return new RiscvImmI32Operand(imm_i32->GetIntImmVal());
  else if (auto *imm_f32 = dynamic_cast<ImmF32Operand *>(op))
    return new RiscvImmF32Operand(imm_f32->GetFloatVal());
  else if (auto *global_op = dynamic_cast<GlobalOperand *>(op))
    return new RiscvGlobalOperand(global_op->GetName());
  else if (auto *label_op = dynamic_cast<LabelOperand *>(op))
    return new RiscvLabelOperand(".L" + std::to_string(label_op->GetLabelNo()));
  return createVirtualReg();
}

std::string Translator::getLLVMTypeString(LLVMType type) {
  switch (type) {
  case LLVMType::I32:
    return "i32";
  case LLVMType::FLOAT32:
    return "float";
  case LLVMType::PTR:
    return "ptr";
  case LLVMType::VOID_TYPE:
    return "void";
  default:
    return "unknown";
  }
}

// 栈帧管理方法实现
void Translator::initFunctionStackFrame(const std::string &func_name) {
  function_stack_frames[func_name] = StackFrameInfo();
  current_stack_frame = &function_stack_frames[func_name];
}

int Translator::getTypeSize(LLVMType type) {
  switch (type) {
  case LLVMType::I32:
    return 4;
  case LLVMType::FLOAT32:
    return 4;
  case LLVMType::I1:
    return 1; // 布尔类型1字节
  case LLVMType::I64:
  default:
    return 4; // 默认4字节
  }
}

void Translator::addLocalVariable(int virtual_reg, LLVMType type) {
  if (!current_stack_frame)
    return;

  int var_size = getTypeSize(type);
  int offset = current_stack_frame->local_vars_size + var_size;

  // 变量按4字节对齐
  if (offset % 4 != 0)
    offset += 4 - (offset % 4);

  current_stack_frame->var_offsets[virtual_reg] = offset;
  current_stack_frame->local_vars_size = offset;
}

void Translator::addLocalArray(int virtual_reg, LLVMType type,
                               const std::vector<int> &dims) {
  if (!current_stack_frame)
    return;

  int element_size = getTypeSize(type);
  int total_elements = 1;
  for (int dim : dims)
    total_elements *= dim;
  int array_size = element_size * total_elements;
  int offset = current_stack_frame->local_vars_size + array_size;

  // 数组按8字节对齐
  if (offset % 8 != 0)
    offset += 8 - (offset % 8);

  current_stack_frame->var_offsets[virtual_reg] = offset;
  current_stack_frame->local_vars_size = offset;
}

void Translator::updateCallArgsSize(int num_args) {
  if (!current_stack_frame)
    return;
  // RISC-V调用约定：前8个参数通过寄存器传递(a0-a7)，其余通过栈传递
  if (num_args > 8) {
    int stack_args = num_args - 8;
    int args_size = stack_args * 4; // 每个参数4字节
    current_stack_frame->call_args_size =
        std::max(current_stack_frame->call_args_size, args_size);
  }
}

void Translator::finalizeFunctionStackFrame() {
  if (!current_stack_frame)
    return;
  current_stack_frame->calculateTotalSize();
}

void Translator::generateStackFrameProlog(RiscvBlock *entry_block) {
  if (!current_stack_frame || current_stack_frame->total_frame_size == 0)
    return;
  auto s0 = getS0Reg();
  auto sp = getSpReg();
  insertAddiInstruction(s0, sp, current_stack_frame->total_frame_size,
                        entry_block, 0);

  auto ra = getRaReg();
  auto sp_2 = getSpReg();
  auto addr_1 =
      new RiscvPtrOperand(current_stack_frame->total_frame_size - 16, sp_2);
  insertSdInstruction(ra, addr_1, entry_block);

  auto s0_1 = getS0Reg();
  auto sp_1 = getSpReg();
  auto addr =
      new RiscvPtrOperand(current_stack_frame->total_frame_size - 8, sp_1);
  insertSdInstruction(s0_1, addr, entry_block);

  // 生成栈帧分配代码
  // addi sp, sp, -frame_size
  auto sp_3 = getSpReg();
  insertAddiInstruction(sp_3->CopyOperand(), sp_3->CopyOperand(),
                        -current_stack_frame->total_frame_size, entry_block, 0);
}

void Translator::generateStackFrameEpilog(RiscvBlock *exit_block) {
  if (!current_stack_frame || current_stack_frame->total_frame_size == 0)
    return;
  auto s0 = getS0Reg();
  auto sp = getSpReg();
  auto addr =
      new RiscvPtrOperand(current_stack_frame->total_frame_size - 8, sp);
  insertLdInstruction(s0, addr, exit_block);

  auto ra = getRaReg();
  auto sp_1 = getSpReg();
  auto addr_1 =
      new RiscvPtrOperand(current_stack_frame->total_frame_size - 16, sp_1);
  insertLdInstruction(ra, addr_1, exit_block);

  // 生成栈帧释放代码
  // addi sp, sp, frame_size
  auto sp_2 = getSpReg();
  insertAddiInstruction(sp_2->CopyOperand(), sp_2->CopyOperand(),
                        current_stack_frame->total_frame_size, exit_block);
}

// 生成函数参数接收代码
void Translator::generateFunctionParameterReceive(FuncDefInstruction func,
                                                  RiscvBlock *entry_block) {
  // 为每个函数参数生成 mv <param_virtual_reg>, <arg_reg> 指令
  // 逆序插入，确保参数顺序正确
  for (int i = func->formals_reg.size() - 1; i >= 0 && i < 8; i--) {
    // 获取参数的虚拟寄存器操作数
    auto param_operand = translateOperand(func->formals_reg[i]);

    // 获取对应的参数寄存器 (a0, a1, ..., a7)
    auto arg_reg = getArgReg(i);

    // 根据参数类型选择合适的指令
    // 浮点参数在RISC-V中通过整数寄存器传递，需要使用fmv.w.x转换
    RiscvInstruction *move_inst;
    if (i < (int)func->formals.size() &&
        func->formals[i] == LLVMType::FLOAT32) {
      // 浮点参数：使用智能move指令，从整数寄存器转到浮点寄存器
      // RiscvFmvInstruction会自动选择合适的指令（fmv.w.x, fmv.s, fmv.x.w, mv）
      move_inst = new RiscvFmvInstruction(param_operand, arg_reg);
    } else {
      // 整数参数：使用普通move指令
      move_inst = new RiscvMvInstruction(param_operand, arg_reg);
    }

    // 使用pos=0插入到栈帧序言之后的开头位置
    entry_block->InsertInstruction(0, move_inst);
  }
}

// 生成完整的函数结尾代码
void Translator::generateFunctionEpilog(RiscvBlock *exit_block) {
  // 1. 生成栈帧结尾代码（如果有栈帧）
  if (current_stack_frame && current_stack_frame->total_frame_size > 0)
    generateStackFrameEpilog(exit_block);

  // 2. 生成返回指令
  auto ra = getRaReg(); // ra寄存器作为返回地址
  auto jr_inst = new RiscvJrInstruction(ra);
  exit_block->InsertInstruction(1, jr_inst);
}

int Translator::getLocalVariableOffset(int virtual_reg) {
  if (!current_stack_frame)
    return 0;
  auto it = current_stack_frame->var_offsets.find(virtual_reg);
  if (it != current_stack_frame->var_offsets.end())
    return it->second;
  return 0;
}

const StackFrameInfo *
Translator::getFunctionStackFrame(const std::string &func_name) const {
  auto it = function_stack_frames.find(func_name);
  if (it != function_stack_frames.end())
    return &it->second;
  return nullptr;
}

int Translator::getCurrentFrameSize() const {
  if (current_stack_frame)
    return current_stack_frame->total_frame_size;
  return 0;
}

void Translator::insertAddiInstruction(RiscvOperand *dest, RiscvOperand *src,
                                       int imm, RiscvBlock *block, int pos) {
  if (imm < -2048 || imm > 2047) {
    // 如果立即数超出范围，使用LI指令加载到寄存器
    auto list_reg = createVirtualReg();
    if (pos) {
      auto li_inst = new RiscvLiInstruction(list_reg, imm);
      block->InsertInstruction(pos, li_inst);
    }
    auto addi_inst = new RiscvAddInstruction(dest, src, list_reg);
    block->InsertInstruction(pos, addi_inst);
    if (!pos) {
      auto li_inst = new RiscvLiInstruction(list_reg, imm);
      block->InsertInstruction(pos, li_inst);
    }
  } else {
    // 否则使用ADDI指令
    auto addi_inst = new RiscvAddiInstruction(dest, src, imm);
    block->InsertInstruction(pos, addi_inst);
  }
}

void Translator::insertLwInstruction(RiscvOperand *dest, RiscvPtrOperand *addr,
                                     RiscvBlock *block) {
  int offset = addr->offset;
  if (offset < -2048 || offset > 2047) {
    // 如果偏移量超出范围，使用LI指令加载到寄存器
    auto list_reg = createVirtualReg();
    auto li_inst = new RiscvLiInstruction(list_reg, offset);
    block->InsertInstruction(1, li_inst);
    auto add_reg = createVirtualReg();
    auto addi_inst = new RiscvAddInstruction(add_reg, addr->base_reg, list_reg);
    block->InsertInstruction(1, addi_inst);
    auto ptr = new RiscvPtrOperand(0, add_reg); // 创建新的指针操作数
    auto lw_inst = new RiscvLwInstruction(dest, ptr);
    block->InsertInstruction(1, lw_inst);
  } else {
    // 否则使用LW指令
    auto lw_inst = new RiscvLwInstruction(dest, addr);
    block->InsertInstruction(1, lw_inst);
  }
}

void Translator::insertLdInstruction(RiscvOperand *dest, RiscvPtrOperand *addr,
                                     RiscvBlock *block) {
  int offset = addr->offset;
  if (offset < -2048 || offset > 2047) {
    // 如果偏移量超出范围，使用LI指令加载到寄存器
    auto list_reg = createVirtualReg();
    auto li_inst = new RiscvLiInstruction(list_reg, offset);
    block->InsertInstruction(1, li_inst);
    auto add_reg = createVirtualReg();
    auto addi_inst = new RiscvAddInstruction(add_reg, addr->base_reg, list_reg);
    block->InsertInstruction(1, addi_inst);
    auto ptr = new RiscvPtrOperand(0, add_reg); // 创建新的指针操作数
    auto ld_inst = new RiscvLdInstruction(dest, ptr);
    block->InsertInstruction(1, ld_inst);
  } else {
    // 否则使用LD指令
    auto ld_inst = new RiscvLdInstruction(dest, addr);
    block->InsertInstruction(1, ld_inst);
  }
}

void Translator::insertSdInstruction(RiscvOperand *src, RiscvPtrOperand *addr,
                                     RiscvBlock *block) {
  int offset = addr->offset;
  if (offset < -2048 || offset > 2047) {
    // 如果偏移量超出范围，使用LI指令加载到寄存器
    auto list_reg = createVirtualReg();
    auto add_reg = createVirtualReg();
    auto ptr = new RiscvPtrOperand(0, add_reg); // 创建新的指针操作数
    auto sd_inst = new RiscvSdInstruction(src, ptr);
    block->InsertInstruction(0, sd_inst);
    auto addi_inst = new RiscvAddInstruction(add_reg, addr->base_reg, list_reg);
    block->InsertInstruction(0, addi_inst);
    auto li_inst = new RiscvLiInstruction(list_reg, offset);
    block->InsertInstruction(0, li_inst);
  } else {
    // 否则使用SD指令
    auto sd_inst = new RiscvSdInstruction(src, addr);
    block->InsertInstruction(0, sd_inst);
  }
}

void Translator::insertSwInstruction(RiscvOperand *dest, RiscvPtrOperand *addr,
                                     RiscvBlock *block) {
  int offset = addr->offset;
  if (offset < -2048 || offset > 2047) {
    // 如果偏移量超出范围，使用LI指令加载到寄存器
    auto list_reg = createVirtualReg();
    auto li_inst = new RiscvLiInstruction(list_reg, offset);
    block->InsertInstruction(1, li_inst);
    auto add_reg = createVirtualReg();
    auto addi_inst = new RiscvAddInstruction(add_reg, addr->base_reg, list_reg);
    block->InsertInstruction(1, addi_inst);
    auto ptr = new RiscvPtrOperand(0, add_reg); // 创建新的指针操作数
    auto sw_inst = new RiscvSwInstruction(dest, ptr);
    block->InsertInstruction(1, sw_inst);
  } else {
    // 否则使用SW指令
    auto sw_inst = new RiscvSwInstruction(dest, addr);
    block->InsertInstruction(1, sw_inst);
  }
}
