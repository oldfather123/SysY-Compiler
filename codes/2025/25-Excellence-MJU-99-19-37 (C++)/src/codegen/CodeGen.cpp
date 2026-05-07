#include "CodeGen.hpp"
#include "CodeGenUtil.hpp"
#include <iostream>
#include <iomanip>

#include <sstream>
#include <functional>


#ifndef PROLOGUE_OFFSET_BASE
#define PROLOGUE_OFFSET_BASE 16  
#endif

// 调试用输出
#define DEBUG_CODEGEN 0
#if DEBUG_CODEGEN
#define DEBUG_PRINT(x) std::cerr << "[DEBUG] " << x << std::endl
#else
#define DEBUG_PRINT(x)
#endif

std::string CodeGen::print() const {
   std::string result;
   for (auto &inst : output)
       result += inst.format();
   return result;
}

void CodeGen::gen_initializer(Constant *init) {
    if (auto *const_array = dynamic_cast<ConstantArray*>(init)) {
        for (size_t i = 0; i < const_array->get_size_of_array(); i++) {
            gen_initializer(const_array->get_element_value(i));
        }
    } else if (auto *const_int = dynamic_cast<ConstantInt*>(init)) {
        auto *type = const_int->get_type();
        if (type->is_int32_type()) {
            append_inst(".word", {std::to_string(const_int->get_value())}, ASMInstruction::Attribute);
        } else if (type->is_int1_type()) {
            append_inst(".byte", {std::to_string(const_int->get_value())}, ASMInstruction::Attribute);
        }
    } else if (auto *const_fp = dynamic_cast<ConstantFP*>(init)) {
        float val = const_fp->get_value();
        uint32_t bits = *reinterpret_cast<uint32_t*>(&val);
        append_inst(".word", {std::to_string(bits)}, ASMInstruction::Attribute);
    } else if (auto *const_zero = dynamic_cast<ConstantZero*>(init)) {
        auto *type = const_zero->get_type();
        append_inst(".zero", {std::to_string(type->get_size())}, ASMInstruction::Attribute);
    }
}

void CodeGen::allocate() {
    // 尝试使用不同的基础偏移(之前fpga上有一个疑似字节问题的报错，通过调整偏移量和最小栈帧，没用)
    unsigned offset = 32;  
    int max_offset_used = 0;
    
    // 为函数参数分配空间
    for (auto &arg : context.func->get_args()) {
        auto size = arg.get_type()->get_size();
        size = (size + 7) & ~7;
        offset += size;
        int cur_offset = -static_cast<int>(offset);
        context.offset_map[&arg] = cur_offset;
        max_offset_used = std::min(max_offset_used, cur_offset);
    }
    
    // 记录alloca大小
    std::unordered_map<AllocaInst*, unsigned> alloca_sizes;
    
    for (auto &bb : context.func->get_basic_blocks()) {
        for (auto &instr : bb.get_instructions()) {
            if (instr.is_alloca()) {
                auto *ai = static_cast<AllocaInst*>(&instr);
                auto alloca_type_size = ai->get_alloca_type()->get_size();
                
                
                if (alloca_type_size == 80) {
                    
                    alloca_sizes[ai] = 80;
                } else {
                    alloca_sizes[ai] = (alloca_type_size + 15) & ~15;
                }
            }
            
            if (!instr.is_void() && context.offset_map.find(&instr) == context.offset_map.end()) {
                auto size = instr.get_type()->get_size();
                size = (size + 7) & ~7;
                offset += size;
                int cur_offset = -static_cast<int>(offset);
                context.offset_map[&instr] = cur_offset;
                max_offset_used = std::min(max_offset_used, cur_offset);
            }
        }
    }
    
   
    offset = (offset + 15) & ~15;
    
    // 分配alloca
    for (auto &bb : context.func->get_basic_blocks()) {
        for (auto &instr : bb.get_instructions()) {
            if (instr.is_alloca()) {
                auto *ai = static_cast<AllocaInst*>(&instr);
                auto alloca_size = alloca_sizes[ai];
                offset += alloca_size;
                int cur_offset = -static_cast<int>(offset);
                context.alloca_offset_map[ai] = cur_offset;
                max_offset_used = std::min(max_offset_used, cur_offset);
            }
        }
    }
    
    
    unsigned required_size = -max_offset_used + 32;
    context.frame_size = ((required_size + 15) / 16) * 16;
    
    
    if (context.frame_size < 240) {
        context.frame_size = 240;
    }
    
    int min_safe_offset = -static_cast<int>(context.frame_size) + 16;
    if (max_offset_used <= min_safe_offset) {
        context.frame_size += 16;
    }
}

void CodeGen::copy_stmt() {
   //  phi 处理已经移到 gen_br 中
}

void CodeGen::gen_phi_copy(BasicBlock *target_bb) {
    DEBUG_PRINT("gen_phi_copy for target " << target_bb->get_name());
    
    // 处理目标基本块中的所有 phi 指令
    for (auto &inst : target_bb->get_instructions()) {
        if (!inst.is_phi()) break;
        
       
        for (unsigned i = 1; i < inst.get_operands().size(); i += 2) {
            if (inst.get_operand(i) == context.bb) {
                auto *val = inst.get_operand(i - 1);
                
                
                if (context.offset_map.find(&inst) == context.offset_map.end()) {
                    DEBUG_PRINT("Warning: No offset for phi " << inst.get_name());
                    continue;
                }
                
                // 调试输出
                if (auto *const_int = dynamic_cast<ConstantInt*>(val)) {
                    //DEBUG_PRINT("PHI copy: constant " << const_int->get_value() << " -> " << inst.get_name());
                } else {
                    //DEBUG_PRINT("PHI copy: " << val->get_name() << " -> " << inst.get_name());
                }
                
                // 处理不同类型的值
                if (val->get_type()->is_float_type()) {
                    // 对于浮点类型的值（包括常量）
                    load_to_freg(val, FReg::fa(0));
                    store_from_freg(&inst, FReg::fa(0));
                } else {
                    // 对于整数类型的值（包括常量）- 统一使用 load_to_greg
                    load_to_greg(val, Reg::a(0));
                    store_from_greg(&inst, Reg::a(0));
                }
                break;
            }
        }
    }
}


void CodeGen::load_to_greg(Value *val, const Reg &reg) {
    assert(val->get_type()->is_integer_type() || val->get_type()->is_pointer_type());

    if (auto *c = dynamic_cast<ConstantInt*>(val)) {
        int64_t v = c->get_value();
        if (IS_IMM_12(v)) {
            append_inst("addi", {reg.print(), "zero", std::to_string(v)});
        } else {
            load_large_int64(v, reg);
        }
    } else if (auto *cz = dynamic_cast<ConstantZero*>(val)) {
        
        append_inst("addi", {reg.print(), "zero", "0"});
    } else if (auto *g = dynamic_cast<GlobalVariable*>(val)) {
        // 处理超长的全局变量名
        std::string gname = g->get_name();
        if (gname.length() > 32) {
            std::hash<std::string> hasher;
            size_t hash_val = hasher(gname);
            std::stringstream ss;
            ss << "g_" << std::hex << (hash_val & 0xFFFFFF) 
               << "_" << gname.substr(0, 8);
            gname = ss.str();
        }
        
        //使用 la 伪指令而不是 auipc/ld 组合
        append_inst("la", {reg.print(), gname});
        
    } else if (auto *phi = dynamic_cast<PhiInst*>(val)) {
       
        if (context.offset_map.find(val) == context.offset_map.end()) {
            throw std::runtime_error("PHI node not allocated: " + val->get_name());
        }
        load_from_stack_to_greg(val, reg);
    } else {
        load_from_stack_to_greg(val, reg);
    }
}

// 处理 ConstantZero
void CodeGen::load_to_freg(Value *val, const FReg &r) {
   assert(val->get_type()->is_float_type());
   
   if (auto *c = dynamic_cast<ConstantFP*>(val)) {
       load_float_imm(c->get_value(), r);
   } else if (auto *cz = dynamic_cast<ConstantZero*>(val)) {
       
       load_float_imm(0.0f, r);
   } else {
       if (context.offset_map.find(val) == context.offset_map.end()) {
           throw std::runtime_error("Float value not found in offset_map: " + val->get_name());
       }
       int off = context.offset_map.at(val);
       if (IS_IMM_12(off)) {
           append_inst("flw", {r.print(), std::to_string(off) + "(fp)"});
       } else {
           
           load_large_int64(off, Reg::t(1));
           append_inst("add", {Reg::t(1).print(), "fp", Reg::t(1).print()});
           append_inst("flw", {r.print(), "0(" + Reg::t(1).print() + ")"});
       }
   }
}



void CodeGen::load_large_int32(int32_t val, const Reg &reg) {
   if (IS_IMM_12(val)) {
       append_inst("addi", {reg.print(), "zero", std::to_string(val)});
   } else {
       append_inst("li", {reg.print(), std::to_string(val)});
   }
}

void CodeGen::load_large_int64(int64_t val, const Reg &reg) {
   append_inst("li", {reg.print(), std::to_string(val)});
}

void CodeGen::load_from_stack_to_greg(Value *val, const Reg &reg) {
   
   if (context.offset_map.find(val) == context.offset_map.end()) {
       std::string error_msg = "Value not found in offset_map: ";
       if (val->get_name().empty()) {
           error_msg += "<unnamed>";
           
           if (dynamic_cast<ConstantInt*>(val)) {
               error_msg += " (ConstantInt)";
           } else if (dynamic_cast<ConstantZero*>(val)) {
               error_msg += " (ConstantZero)";
           } else if (dynamic_cast<PhiInst*>(val)) {
               error_msg += " (PhiInst)";
           } else {
               error_msg += " (Unknown type)";
           }
       } else {
           error_msg += val->get_name();
       }
       throw std::runtime_error(error_msg);
   }
   
   int offset = context.offset_map.at(val);
   auto *t = val->get_type();
   
   if (IS_IMM_12(offset)) {
       if (t->is_int1_type()) {
           append_inst("lb", {reg.print(), std::to_string(offset) + "(fp)"});
       } else if (t->is_int32_type()) {
           append_inst("lw", {reg.print(), std::to_string(offset) + "(fp)"});
       } else {
           append_inst("ld", {reg.print(), std::to_string(offset) + "(fp)"});
       }
   } else {
      
       Reg addr_reg = (reg.print() == "t6") ? Reg::t(5) : Reg::t(6);
       
       load_large_int64(offset, addr_reg);
       append_inst("add", {addr_reg.print(), "fp", addr_reg.print()});
       
       if (t->is_int1_type()) {
           append_inst("lb", {reg.print(), "0(" + addr_reg.print() + ")"});
       } else if (t->is_int32_type()) {
           append_inst("lw", {reg.print(), "0(" + addr_reg.print() + ")"});
       } else {
           append_inst("ld", {reg.print(), "0(" + addr_reg.print() + ")"});
       }
   }
}  

   
void CodeGen::store_from_greg(Value *val, const Reg &reg) {
   int offset = context.offset_map.at(val);
   auto *t = val->get_type();
   
   if (IS_IMM_12(offset)) {
       if (t->is_int1_type()) {
           append_inst("sb", {reg.print(), std::to_string(offset) + "(fp)"});
       } else if (t->is_int32_type()) {
           append_inst("sw", {reg.print(), std::to_string(offset) + "(fp)"});
       } else {
           append_inst("sd", {reg.print(), std::to_string(offset) + "(fp)"});
       }
   } else {
       
       Reg addr_reg = (reg.print() == "t1") ? Reg::t(2) : Reg::t(1);
       
       load_large_int64(offset, addr_reg);
       append_inst("add", {addr_reg.print(), "fp", addr_reg.print()});
       
       if (t->is_int1_type()) {
           append_inst("sb", {reg.print(), "0(" + addr_reg.print() + ")"});
       } else if (t->is_int32_type()) {
           append_inst("sw", {reg.print(), "0(" + addr_reg.print() + ")"});
       } else {
           append_inst("sd", {reg.print(), "0(" + addr_reg.print() + ")"});
       }
   }
}




void CodeGen::load_float_imm(float v, const FReg &r) {
   int32_t bits = *reinterpret_cast<int32_t*>(&v);
   load_large_int32(bits, Reg::t(0));
   append_inst("fmv.w.x", {r.print(), Reg::t(0).print()});
}

void CodeGen::store_from_freg(Value *val, const FReg &r) {
   int off = context.offset_map.at(val);
   if (IS_IMM_12(off)) {
       append_inst("fsw", {r.print(), std::to_string(off) + "(fp)"});
   } else {
      
       load_large_int64(off, Reg::t(1));
       append_inst("add", {Reg::t(1).print(), "fp", Reg::t(1).print()});
       append_inst("fsw", {r.print(), "0(" + Reg::t(1).print() + ")"});
   }
}

void CodeGen::gen_prologue() {
    // 确保栈帧大小是16的倍数，不然影响对齐
    if (context.frame_size % 16 != 0) {
        context.frame_size = ((context.frame_size + 15) / 16) * 16;
    }
    
    // 分配栈空间 - 处理大栈帧
    if (IS_IMM_12(-static_cast<int>(context.frame_size))) {
        append_inst("addi", {"sp", "sp", std::to_string(-static_cast<int>(context.frame_size))});
    } else {
        load_large_int64(-static_cast<int>(context.frame_size), Reg::t(0));
        append_inst("add", {"sp", "sp", Reg::t(0).print()});
    }
    
    
    // 保存返回地址和帧指针
    int ra_offset = context.frame_size - 8;
    int fp_offset = context.frame_size - 16;
    
    if (IS_IMM_12(ra_offset)) {
        append_inst("sd", {"ra", std::to_string(ra_offset) + "(sp)"});
    } else {
        load_large_int64(ra_offset, Reg::t(0));
        append_inst("add", {Reg::t(0).print(), "sp", Reg::t(0).print()});
        append_inst("sd", {"ra", "0(" + Reg::t(0).print() + ")"});
    }
    
    if (IS_IMM_12(fp_offset)) {
        append_inst("sd", {"fp", std::to_string(fp_offset) + "(sp)"});
    } else {
        load_large_int64(fp_offset, Reg::t(0));
        append_inst("add", {Reg::t(0).print(), "sp", Reg::t(0).print()});
        append_inst("sd", {"fp", "0(" + Reg::t(0).print() + ")"});
    }
    
    //确保对齐
    if (IS_IMM_12(context.frame_size)) {
        append_inst("addi", {"fp", "sp", std::to_string(context.frame_size)});
    } else {
        load_large_int64(context.frame_size, Reg::t(0));
        append_inst("add", {"fp", "sp", Reg::t(0).print()});
    }
    
    // 处理函数参数
    int gi = 0, fi = 0;
    int stack_param_offset = 0;
    
    for (auto &arg : context.func->get_args()) {
        if (arg.get_type()->is_float_type() && fi < 8) {
            store_from_freg(&arg, FReg::fa(fi++));
        } else if (!arg.get_type()->is_float_type() && gi < 8) {
            store_from_greg(&arg, Reg::a(gi++));
        } else {
            // 从栈上加载参数
            int offset = context.offset_map[&arg];
            if (arg.get_type()->is_float_type()) {
                append_inst("flw", {FReg::ft(0).print(), std::to_string(stack_param_offset) + "(fp)"});
                append_inst("fsw", {FReg::ft(0).print(), std::to_string(offset) + "(fp)"});
            } else {
                if (arg.get_type()->is_int32_type()) {
                    append_inst("lw", {Reg::t(0).print(), std::to_string(stack_param_offset) + "(fp)"});
                } else {
                    append_inst("ld", {Reg::t(0).print(), std::to_string(stack_param_offset) + "(fp)"});
                }
                append_inst("sd", {Reg::t(0).print(), std::to_string(offset) + "(fp)"});
            }
            stack_param_offset += 8;
        }
    }
}

void CodeGen::gen_ret() {
   if (context.inst->get_num_operand() > 0) {
       auto *rv = context.inst->get_operand(0);
       if (rv->get_type()->is_float_type()) {
           load_to_freg(rv, FReg::fa(0));
       } else {
           load_to_greg(rv, Reg::a(0));
       }
   }
   append_inst("j", {func_exit_label_name(context.func)});
}

void CodeGen::gen_br() {
    auto *br = static_cast<BranchInst*>(context.inst);
    if (br->is_cond_br()) {
        auto *true_bb = static_cast<BasicBlock*>(br->get_operand(1));
        auto *false_bb = static_cast<BasicBlock*>(br->get_operand(2));
        
        // 加载条件值
        load_to_greg(br->get_operand(0), Reg::t(0));
        
        // 生成条件跳转
        static unsigned br_label_counter = 0;
        std::string false_label = ".L_false_" + std::to_string(br_label_counter++);
        
        append_inst("beqz", {Reg::t(0).print(), false_label});
        
        // True分支：先处理PHI，再跳转
        gen_phi_copy(true_bb);
        append_inst("j", {label_name(true_bb)});
        
        // False分支标签
        append_inst(ASMInstruction::makeLabel(false_label));
        
        // False分支：先处理PHI，再跳转
        gen_phi_copy(false_bb);
        append_inst("j", {label_name(false_bb)});
    } else {
        // 无条件跳转
        auto *target_bb = static_cast<BasicBlock*>(br->get_operand(0));
        gen_phi_copy(target_bb);
        append_inst("j", {label_name(target_bb)});
    }
}

void CodeGen::gen_binary() {
   load_to_greg(context.inst->get_operand(0), Reg::t(0));
   load_to_greg(context.inst->get_operand(1), Reg::t(1));
   
   switch (context.inst->get_instr_type()) {
       case Instruction::add: 
           append_inst("addw", {"t2", "t0", "t1"}); 
           break;
       case Instruction::sub: 
           append_inst("subw", {"t2", "t0", "t1"}); 
           break;
       case Instruction::mul: 
           append_inst("mulw", {"t2", "t0", "t1"}); 
           break;
       case Instruction::sdiv:
           append_inst("divw", {"t2", "t0", "t1"}); 
           break;
       default: 
           assert(false);
   }
   store_from_greg(context.inst, Reg::t(2));
}

void CodeGen::gen_float_binary() {
   load_to_freg(context.inst->get_operand(0), FReg::ft(0));
   load_to_freg(context.inst->get_operand(1), FReg::ft(1));
   
   switch (context.inst->get_instr_type()) {
       case Instruction::fadd: 
           append_inst("fadd.s", {"ft2", "ft0", "ft1"}); 
           break;
       case Instruction::fsub: 
           append_inst("fsub.s", {"ft2", "ft0", "ft1"}); 
           break;
       case Instruction::fmul: 
           append_inst("fmul.s", {"ft2", "ft0", "ft1"}); 
           break;
       case Instruction::fdiv: 
           append_inst("fdiv.s", {"ft2", "ft0", "ft1"}); 
           break;
       default: 
           assert(false);
   }
   store_from_freg(context.inst, FReg::ft(2));
}

void CodeGen::gen_icmp() {
   load_to_greg(context.inst->get_operand(0), Reg::t(0));
   load_to_greg(context.inst->get_operand(1), Reg::t(1));
   
   switch (context.inst->get_instr_type()) {
       case Instruction::eq: 
           append_inst("xor", {"t2", "t0", "t1"}); 
           append_inst("seqz", {"t2", "t2"}); 
           break;
       case Instruction::ne: 
           append_inst("xor", {"t2", "t0", "t1"}); 
           append_inst("snez", {"t2", "t2"}); 
           break;
       case Instruction::lt: 
           append_inst("slt", {"t2", "t0", "t1"}); 
           break;
       case Instruction::gt: 
           append_inst("slt", {"t2", "t1", "t0"}); 
           break;
       case Instruction::le: 
           append_inst("slt", {"t2", "t1", "t0"}); 
           append_inst("xori", {"t2", "t2", "1"}); 
           break;
       case Instruction::ge: 
           append_inst("slt", {"t2", "t0", "t1"}); 
           append_inst("xori", {"t2", "t2", "1"}); 
           break;
       default: 
           assert(false);
   }
   store_from_greg(context.inst, Reg::t(2));
}

void CodeGen::gen_fcmp() {
   load_to_freg(context.inst->get_operand(0), FReg::ft(0));
   load_to_freg(context.inst->get_operand(1), FReg::ft(1));
   context.fcmp_cnt++;
   
   switch (context.inst->get_instr_type()) {
       case Instruction::feq: 
           append_inst("feq.s", {"t0", "ft0", "ft1"}); 
           break;
       case Instruction::fne: 
           append_inst("feq.s", {"t0", "ft0", "ft1"}); 
           append_inst("xori", {"t0", "t0", "1"}); 
           break;
       case Instruction::flt: 
           append_inst("flt.s", {"t0", "ft0", "ft1"}); 
           break;
       case Instruction::fgt: 
           append_inst("flt.s", {"t0", "ft1", "ft0"}); 
           break;
       case Instruction::fle: 
           append_inst("fle.s", {"t0", "ft0", "ft1"}); 
           break;
       case Instruction::fge: 
           append_inst("fle.s", {"t0", "ft1", "ft0"}); 
           break;
       default: 
           assert(false);
   }
   store_from_greg(context.inst, Reg::t(0));
}

void CodeGen::gen_alloca() {
    auto *ai = static_cast<AllocaInst*>(context.inst);
    
    if (context.alloca_offset_map.find(ai) != context.alloca_offset_map.end()) {
        int total_off = context.alloca_offset_map[ai];
        
        // 计算alloca的地址
        if (IS_IMM_12(total_off)) {
            append_inst("addi", {"t0", "fp", std::to_string(total_off)});
        } else {
            load_large_int64(total_off, Reg::t(1));
            append_inst("add", {"t0", "fp", "t1"});
        }
        
        // 对于数组类型，需要清零初始化
        auto *alloca_type = ai->get_alloca_type();
        if (alloca_type->is_array_type()) {
            // 获取数组大小
            int array_size = alloca_type->get_size();
            
           
            append_inst("mv", {"t2", "t0"});
            
            // 清零循环
            if (array_size <= 64) {
                // 小数组，直接用sd指令清零
                for (int i = 0; i < array_size; i += 8) {
                    if (i == 0) {
                        append_inst("sd", {"zero", "0(t2)"});
                    } else {
                        append_inst("sd", {"zero", std::to_string(i) + "(t2)"});
                    }
                }
            } else {
                // 大数组，使用循环
                append_inst("li", {"t3", std::to_string(array_size)});
                append_inst("add", {"t3", "t2", "t3"});  // t3 = 结束地址
                
                // 循环标签
                std::string loop_label = ".L_zero_loop_" + std::to_string(reinterpret_cast<uintptr_t>(ai));
                append_inst(ASMInstruction::makeLabel(loop_label));
                
                append_inst("sd", {"zero", "0(t2)"});
                append_inst("addi", {"t2", "t2", "8"});
                append_inst("bltu", {"t2", "t3", loop_label});
            }
            
            // 恢复原始地址到t0
            if (IS_IMM_12(total_off)) {
                append_inst("addi", {"t0", "fp", std::to_string(total_off)});
            } else {
                load_large_int64(total_off, Reg::t(1));
                append_inst("add", {"t0", "fp", "t1"});
            }
        }
    } else {
        
        int ptr_off = context.offset_map[context.inst];
        int alloca_size = ai->get_alloca_type()->get_size();
        alloca_size = (alloca_size + 31) & ~31;
        int total_off = ptr_off - alloca_size;
        
        if (IS_IMM_12(total_off)) {
            append_inst("addi", {"t0", "fp", std::to_string(total_off)});
        } else {
            load_large_int64(total_off, Reg::t(1));
            append_inst("add", {"t0", "fp", "t1"});
        }
    }
    
    store_from_greg(context.inst, Reg::t(0));
}

void CodeGen::gen_load() {
   auto *ptr = context.inst->get_operand(0);
   load_to_greg(ptr, Reg::t(0));
   auto *t = context.inst->get_type();
   
   if (t->is_float_type()) {
       append_inst("flw", {"ft0", "0(t0)"});
       store_from_freg(context.inst, FReg::ft(0));
   } else {
       if (t->is_int1_type()) {
           append_inst("lb", {"t1", "0(t0)"});
       } else if (t->is_int32_type()) {
           append_inst("lw", {"t1", "0(t0)"});
       } else {
           append_inst("ld", {"t1", "0(t0)"});
       }
       store_from_greg(context.inst, Reg::t(1));
   }
}

void CodeGen::gen_store() {
   auto *val = context.inst->get_operand(0);
   auto *ptr = context.inst->get_operand(1);
   load_to_greg(ptr, Reg::t(1));
   
   if (val->get_type()->is_float_type()) {
       load_to_freg(val, FReg::ft(0));
       append_inst("fsw", {"ft0", "0(t1)"});
   } else {
       load_to_greg(val, Reg::t(0));
       auto *t = val->get_type();
       
       if (t->is_int1_type()) {
           append_inst("sb", {"t0", "0(t1)"});
       } else if (t->is_int32_type()) {
           append_inst("sw", {"t0", "0(t1)"});
       } else {
           append_inst("sd", {"t0", "0(t1)"});
       }
   }
}

void CodeGen::gen_call() {
    // 获取被调用的函数
    auto *fn = dynamic_cast<Function*>(context.inst->get_operand(0));
    std::string fname = fn->get_name();
    
    // 特殊处理 starttime 和 stoptime 函数，转换为 _sysy_starttime 和 _sysy_stoptime(及其关键修改，不然没成绩)
    if (fname == "starttime" || fname == "stoptime") {
       
        std::vector<std::string> saved_regs;
        saved_regs.push_back("t0");
        saved_regs.push_back("t1");
        saved_regs.push_back("t2");
        saved_regs.push_back("t3");
        saved_regs.push_back("t4");
        saved_regs.push_back("t5");
        saved_regs.push_back("t6");
        
        // 8字节对齐
        int save_space = saved_regs.size() * 8;
        save_space = (save_space + 15) & ~15;  // 16字节对齐
        
        // 分配栈空间保存寄存器
        append_inst("addi", {"sp", "sp", std::to_string(-save_space)});
        
        // 保存寄存器
        for (size_t i = 0; i < saved_regs.size(); i++) {
            append_inst("sd", {saved_regs[i], std::to_string(i * 8) + "(sp)"});
        }
        
        // 将行号参数加载到 a0 寄存器
        if (context.inst->get_num_operand() > 1) {
           
            load_to_greg(context.inst->get_operand(1), Reg::a(0));
        } else {
            
            append_inst("li", {Reg::a(0).print(), "0"});
        }
        
        // 使用 jal 指令调用外部函数，转换函数名
        std::string real_fname = "_sysy_" + fname;
        append_inst("jal", {real_fname});
        
        // 恢复寄存器
        for (size_t i = 0; i < saved_regs.size(); i++) {
            append_inst("ld", {saved_regs[i], std::to_string(i * 8) + "(sp)"});
        }
        
        // 恢复栈指针
        append_inst("addi", {"sp", "sp", std::to_string(save_space)});
        
        // 如果有返回值，保存到栈上
        if (!context.inst->is_void()) {
            store_from_greg(context.inst, Reg::a(0));
        }
        
        return;
    }
    
    // 通用函数调用处理逻辑
    int ic = 0, fc = 0;
    int stack_args = 0;
    
    // 计算需要压栈的参数数量
    for (unsigned i = 1; i < context.inst->get_num_operand(); ++i) {
        auto *arg = context.inst->get_operand(i);
        if (arg->get_type()->is_float_type()) {
            if (fc >= 8) stack_args++;
            else fc++;
        } else {
            if (ic >= 8) stack_args++;
            else ic++;
        }
    }
    
    // 分配栈空间
    if (stack_args > 0) {
        int stack_size = stack_args * 8;
        append_inst("addi", {"sp", "sp", std::to_string(-stack_size)});
    }
    
    // 处理参数
    ic = fc = 0;
    int stack_offset = 0;
    
    for (unsigned i = 1; i < context.inst->get_num_operand(); ++i) {
        auto *arg = context.inst->get_operand(i);
        
        if (arg->get_type()->is_float_type()) {
            if (fc < 8) {
                load_to_freg(arg, FReg::fa(fc++));
            } else {
                load_to_freg(arg, FReg::ft(0));
                append_inst("fsw", {FReg::ft(0).print(), std::to_string(stack_offset) + "(sp)"});
                stack_offset += 8;
            }
        } else {
            if (ic < 8) {
                load_to_greg(arg, Reg::a(ic++));
            } else {
                load_to_greg(arg, Reg::t(0));
                append_inst("sd", {Reg::t(0).print(), std::to_string(stack_offset) + "(sp)"});
                stack_offset += 8;
            }
        }
    }
    
    // 处理超长函数名
    if (fname.length() > 32 && fname != "main") {
        std::hash<std::string> hasher;
        size_t hash_val = hasher(fname);
        std::stringstream ss;
        ss << fname.substr(0, 16) << "_" << std::hex << (hash_val & 0xFFFF) 
           << "_" << fname.substr(fname.length() - 8);
        fname = ss.str();
    }
    
    append_inst("call", {fname});
    
    // 恢复栈指针
    if (stack_args > 0) {
        int stack_size = stack_args * 8;
        append_inst("addi", {"sp", "sp", std::to_string(stack_size)});
    }
    
    // 保存返回值
    if (!context.inst->is_void()) {
        if (context.inst->get_type()->is_float_type()) {
            store_from_freg(context.inst, FReg::fa(0));
        } else {
            store_from_greg(context.inst, Reg::a(0));
        }
    }
}

void CodeGen::gen_gep() {
    // 检查第一个操作数是否是全局变量
    bool is_global = dynamic_cast<GlobalVariable*>(context.inst->get_operand(0)) != nullptr;
    
    if (is_global) {
        // 对于全局变量，我们需要先加载地址
        auto *gv = dynamic_cast<GlobalVariable*>(context.inst->get_operand(0));
        std::string gname = gv->get_name();
        
        // 处理超长名称
        if (gname.length() > 32) {
            std::hash<std::string> hasher;
            size_t hash_val = hasher(gname);
            std::stringstream ss;
            ss << "g_" << std::hex << (hash_val & 0xFFFFFF) 
               << "_" << gname.substr(0, 8);
            gname = ss.str();
        }
        
        // 使用 la 指令加载全局变量地址
        append_inst("la", {"t0", gname});
    } else {
        // 对于非全局变量，正常加载基地址
        load_to_greg(context.inst->get_operand(0), Reg::t(0));
    }
    
    auto *ptr_type = context.inst->get_operand(0)->get_type();
    auto *base_type = ptr_type->get_pointer_element_type();
    
    // 处理第一个索引（结构体或数组的索引）
    if (context.inst->get_num_operand() > 1) {
        auto *idx1 = context.inst->get_operand(1);
        
        // 检查第一个索引是否为0（常见情况）
        bool idx1_is_zero = false;
        if (auto *const_idx = dynamic_cast<ConstantInt*>(idx1)) {
            idx1_is_zero = (const_idx->get_value() == 0);
        }
        
        if (!idx1_is_zero) {
            load_to_greg(idx1, Reg::t(1));
            int base_size = base_type->get_size();
            
            if (base_size != 1) {
                append_inst("li", {"t2", std::to_string(base_size)});
                append_inst("mul", {"t1", "t1", "t2"});
            }
            append_inst("add", {"t0", "t0", "t1"});
        }
    }
    
    // 处理剩余的索引
    auto *current_type = base_type;
    for (unsigned i = 2; i < context.inst->get_num_operand(); ++i) {
        if (current_type->is_array_type()) {
            auto *array_type = static_cast<ArrayType*>(current_type);
            current_type = array_type->get_element_type();
            
            load_to_greg(context.inst->get_operand(i), Reg::t(1));
            int elem_size = current_type->get_size();
            
            if (elem_size != 1) {
                append_inst("li", {"t2", std::to_string(elem_size)});
                append_inst("mul", {"t1", "t1", "t2"});
            }
            append_inst("add", {"t0", "t0", "t1"});
        }
    }
    
    store_from_greg(context.inst, Reg::t(0));
}

void CodeGen::gen_epilogue() {
    append_inst(ASMInstruction::makeLabel(func_exit_label_name(context.func)));
    
    // 恢复寄存器 - 处理大偏移
    int ra_offset = context.frame_size - 8;
    int fp_offset = context.frame_size - 16;
    
    if (IS_IMM_12(ra_offset)) {
        append_inst("ld", {"ra", std::to_string(ra_offset) + "(sp)"});
    } else {
        load_large_int64(ra_offset, Reg::t(0));
        append_inst("add", {Reg::t(0).print(), "sp", Reg::t(0).print()});
        append_inst("ld", {"ra", "0(" + Reg::t(0).print() + ")"});
    }
    
    if (IS_IMM_12(fp_offset)) {
        append_inst("ld", {"fp", std::to_string(fp_offset) + "(sp)"});
    } else {
        load_large_int64(fp_offset, Reg::t(0));
        append_inst("add", {Reg::t(0).print(), "sp", Reg::t(0).print()});
        append_inst("ld", {"fp", "0(" + Reg::t(0).print() + ")"});
    }
    
    // 恢复栈指针 - 确保恢复后sp仍然是对齐的
    if (IS_IMM_12(context.frame_size)) {
        append_inst("addi", {"sp", "sp", std::to_string(context.frame_size)});
    } else {
        load_large_int64(context.frame_size, Reg::t(0));
        append_inst("add", {"sp", "sp", Reg::t(0).print()});
    }
    
    append_inst("ret", {});
}

void CodeGen::gen_zext() {
   auto *op = context.inst->get_operand(0);
   load_to_greg(op, Reg::t(0));
   append_inst("andi", {"t0", "t0", "0xff"});
   store_from_greg(context.inst, Reg::t(0));
}

void CodeGen::gen_sitofp() {
   load_to_greg(context.inst->get_operand(0), Reg::t(0));
   append_inst("fcvt.s.w", {"ft0", "t0"});
   store_from_freg(context.inst, FReg::ft(0));
}

void CodeGen::gen_fptosi() {
   load_to_freg(context.inst->get_operand(0), FReg::ft(0));
   append_inst("fcvt.w.s", {"t0", "ft0", "rtz"});
   store_from_greg(context.inst, Reg::t(0));
}

void CodeGen::run() {
    m->set_print_name();

    // 生成数据段
    bool has_real_globals = false;
    
    // 检查是否有真正的全局变量
    for (auto &g : m->get_global_variable()) {
        if (auto *gvar = dynamic_cast<GlobalVariable*>(&g)) {
            // 过滤掉可能被误认为是全局变量的函数名（有点用）
            std::string name = gvar->get_name();
            if (name != "input" && name != "output" && name != "outputFloat" && 
                name != "getint" && name != "putint" && name != "getch" && 
                name != "putch" && name != "getfloat" && name != "putfloat" &&
                name != "getarray" && name != "putarray" && name != "getfarray" &&
                name != "putfarray" && name != "starttime" && name != "stoptime" &&
                name != "neg_idx_except" && name != "_sysy_starttime" && 
                name != "_sysy_stoptime" && name != "main") {
                has_real_globals = true;
                break;
            }
        }
    }
    
    if (has_real_globals) {
        append_inst(".data", {}, ASMInstruction::Attribute);
        
        for (auto &g : m->get_global_variable()) {
            // 强制类型转换，确保是GlobalVariable
            auto *gvar = dynamic_cast<GlobalVariable*>(&g);
            if (!gvar) {
                // 如果不是GlobalVariable，跳过
                continue;
            }
            
            // 获取变量名
            std::string gname = gvar->get_name();
            
            // 再次检查，确保不是函数名
            if (gname == "input" || gname == "output" || gname == "outputFloat" || 
                gname == "getint" || gname == "putint" || gname == "getch" || 
                gname == "putch" || gname == "getfloat" || gname == "putfloat" ||
                gname == "getarray" || gname == "putarray" || gname == "getfarray" ||
                gname == "putfarray" || gname == "starttime" || gname == "stoptime" ||
                gname == "neg_idx_except" || gname == "_sysy_starttime" || 
                gname == "_sysy_stoptime" || gname == "main") {
                continue;
            }
            
            auto *elem_type = gvar->get_type()->get_pointer_element_type();
            size_t sz = elem_type->get_size();
            
            // 处理超长的全局变量名 
            if (gname.length() > 32) {
                std::hash<std::string> hasher;
                size_t hash_val = hasher(gname);
                std::stringstream ss;
                
                ss << "g_" << std::hex << (hash_val & 0xFFFFFF) 
                   << "_" << gname.substr(0, 8);
                gname = ss.str();
            }
            
            append_inst(".globl", {gname}, ASMInstruction::Attribute);
            append_inst(".align", {"3"}, ASMInstruction::Attribute);
            append_inst(".type", {gname, "@object"}, ASMInstruction::Attribute);
            append_inst(".size", {gname, std::to_string(sz)}, ASMInstruction::Attribute);
            append_inst(ASMInstruction::makeLabel(gname));
            
            auto *init = gvar->get_init();
            if (init && !dynamic_cast<ConstantZero*>(init)) {
                gen_initializer(init);
            } else {
                append_inst(".zero", {std::to_string(sz)}, ASMInstruction::Attribute);
            }
        }
    }

    // 生成代码段
    append_inst(".text", {}, ASMInstruction::Attribute);
    
    for (auto &f : m->get_functions()) {
        if (f.is_declaration()) continue;
        
        context.clear();
        context.func = &f;
        
        // 处理超长的函数名
        std::string fname = f.get_name();
        if (fname.length() > 32 && fname != "main") {  // main函数名不能改
            std::hash<std::string> hasher;
            size_t hash_val = hasher(fname);
            std::stringstream ss;
            ss << fname.substr(0, 16) << "_" << std::hex << (hash_val & 0xFFFF) 
               << "_" << fname.substr(fname.length() - 8);
            fname = ss.str();
        }
        
        // 函数标签
        append_inst(".globl", {fname}, ASMInstruction::Attribute);
        append_inst(".type", {fname, "@function"}, ASMInstruction::Attribute);
        append_inst(".align", {"2"}, ASMInstruction::Attribute);
        append_inst(ASMInstruction::makeLabel(fname));
        
        // 分配寄存器和栈帧
        allocate();
        gen_prologue();
        
        // 生成每个基本块的代码
        for (auto &bb : f.get_basic_blocks()) {
            context.bb = &bb;
            append_inst(ASMInstruction::makeLabel(label_name(context.bb)));
            
            for (auto &instr : bb.get_instructions()) {
                std::string instr_str = instr.print();
                if (instr_str.length() > 200) {  
                   //截断，若大于200字符
                    instr_str = instr_str.substr(0, 80) + " ... " + 
                               instr_str.substr(instr_str.length() - 80);
                }
                append_inst(ASMInstruction::makeComment(instr_str));
                context.inst = &instr;
                
                switch (instr.get_instr_type()) {
                    case Instruction::ret:    
                        gen_ret(); 
                        break;
                    case Instruction::br:     
                        copy_stmt(); 
                        gen_br(); 
                        break;
                    case Instruction::add:
                    case Instruction::sub:
                    case Instruction::mul:
                    case Instruction::sdiv: 
                        gen_binary(); 
                        break;
                    case Instruction::fadd:
                    case Instruction::fsub:
                    case Instruction::fmul:
                    case Instruction::fdiv: 
                        gen_float_binary(); 
                        break;
                    case Instruction::alloca: 
                        gen_alloca(); 
                        break;
                    case Instruction::load:   
                        gen_load(); 
                        break;
                    case Instruction::store:  
                        gen_store(); 
                        break;
                    case Instruction::ge:
                    case Instruction::gt:
                    case Instruction::le:
                    case Instruction::lt:
                    case Instruction::eq:
                    case Instruction::ne: 
                        gen_icmp(); 
                        break;
                    case Instruction::fge:
                    case Instruction::fgt:
                    case Instruction::fle:
                    case Instruction::flt:
                    case Instruction::feq:
                    case Instruction::fne: 
                        gen_fcmp(); 
                        break;
                    case Instruction::call:   
                        gen_call(); 
                        break;
                    case Instruction::getelementptr: 
                        gen_gep(); 
                        break;
                    case Instruction::zext:   
                        gen_zext(); 
                        break;
                    case Instruction::fptosi: 
                        gen_fptosi(); 
                        break;
                    case Instruction::sitofp: 
                        gen_sitofp(); 
                        break;
                    case Instruction::phi:
                        // PHI 节点在 gen_br 中处理
                        break;
                    default:
                        // 忽略未实现的指令
                        break;
                }
            }
        }
        
        gen_epilogue();
    }
}