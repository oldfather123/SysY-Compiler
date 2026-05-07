#include <algorithm>
#include <unordered_map>
#include "riscv_generator.hpp"
#include "ir_type.hpp"
#include "ir_module.hpp"
#include "ir_basic_block.hpp"
#include "ir_function.hpp"
#include "riscv_basic_block.hpp"
#include "riscv_function.hpp"
#include "riscv_instruction.hpp"
#include "register.hpp"
#include "reg_alloc.hpp"

RiscvGenerator::RiscvGenerator(Module* m): m(m), label_count(0) {
    // 初始化寄存器堆
    for(int i = 0; i < 32; ++i) {
        Register* reg = new Register(RegType::IntReg, i);
        regs.push_back(reg);
        reg_ops.push_back(new RiscvIntReg(reg));
    }
    for(int i = 0; i < 32; ++i) {
        Register* reg = new Register(RegType::FloatReg, i);
        regs.push_back(reg);
        reg_ops.push_back(new RiscvFloatReg(reg));
    }
    // 初始化需要保护的寄存器：ra, s0-s11, fs0-fs11
    reg_protected.push_back(get_reg_op_named("ra"));
    for(int i = 1; i <= 11; i++) {
        reg_protected.push_back(get_reg_op_named("s" + to_string(i)));
    }
    for(int i = 0; i <= 11; i++) {
        reg_protected.push_back(get_reg_op_named("fs" + to_string(i)));
    }
}

RiscvFunction* RiscvGenerator::get_rfunc_from_func(Function* func) {
    if(func_to_rfunc.count(func)) { // 如果已经存在，则直接返回
        return func_to_rfunc[func];
    } else { // 否则需要创建一个新的 RiscvFunction
        return func_to_rfunc[func] = new RiscvFunction(func->name);
    }
}

string RiscvGenerator::to_label(int index) {
    return ".L" + to_string(index);
}

RiscvBasicBlock* RiscvGenerator::get_rbb_from_bb(BasicBlock* bb) {
    if(!bb) {
        auto rbb = new RiscvBasicBlock(to_label(label_count), label_count);
        label_count++;
        return rbb;
    } else {
        if(bb_to_rbb.count(bb)) {
            return bb_to_rbb[bb]; // 如果已经存在，则直接返回
        } else {
            auto rbb = new RiscvBasicBlock(to_label(label_count), label_count);
            label_count++;
            bb_to_rbb[bb] = rbb; // 记录映射关系
            return rbb;
        }
    }
}

RiscvFunction* RiscvGenerator::create_lib_func(Function* func) {
    if(func->name == "__aeabi_memclr4") {
        auto rfunc = get_rfunc_from_func(func);
        // 预处理块
        auto rbb1 = get_rbb_from_bb();
        // .L1:
        //     MV   t5, a0
        //     MV   t6, a1
        //     ADD  t6, a0, t6
        //     LI   a0, zero
        rbb1->add_rinst_back(new RiscvMoveInst(get_reg_op_named("t5"), get_reg_op_named("a0"), rbb1));
        rbb1->add_rinst_back(new RiscvMoveInst(get_reg_op_named("t6"), get_reg_op_named("a1"), rbb1));
        rbb1->add_rinst_back(new RiscvBinaryInst(RiscvInstID::ADD, get_reg_op_named("a0"), get_reg_op_named("t6"), get_reg_op_named("t6"), rbb1));
        rbb1->add_rinst_back(new RiscvLiInst(get_reg_op_named("a0"), 0, rbb1));
        rfunc->add_rbb(rbb1);
        // 循环块：默认clear为全0
        auto rbb2 = get_rbb_from_bb();
        // .L2:
        //     SW   zero, (t5)
        //     ADDI t5, t5, 4
        //     BLT  t5, t6, .L2
        //     RET
        rbb2->add_rinst_back(new RiscvStoreInst(new Type(TypeID::IntegerTy), get_reg_op_named("zero"), new RiscvIntMem(get_reg_named("t5")), rbb2));
        rbb2->add_rinst_back(new RiscvBinaryInst(RiscvInstID::ADDI, get_reg_op_named("t5"), new RiscvConst(4), get_reg_op_named("t5"), rbb2));
        rbb2->add_rinst_back(new RiscvICmpInst(ICmpOp::LT, get_reg_op_named("t5"), get_reg_op_named("t6"), rbb2, nullptr, rbb2));
        rbb2->add_rinst_back(new RiscvReturnInst(rbb2));
        rfunc->add_rbb(rbb2);
        return rfunc;
    } else {
        return nullptr;
    }
}

void RiscvGenerator::init_ret_inst(RegAlloc* reg_alloc, RiscvInstruction* ret_inst, RiscvBasicBlock* rbb, RiscvFunction* rfunc) {
    // 函数尾声 epilogue，与函数序言 prologue 相对称
    // 将被保护的寄存器还原
    // ! FP 必须被最后还原。
    int cur_sp = rfunc->base;
    auto reg_to_recover = reg_protected;
    auto reg_used = reg_alloc->reg_used;
    reverse(reg_to_recover.begin(), reg_to_recover.end());
    for(auto reg: reg_to_recover)
        if(reg_used.find(reg) != reg_used.end()) {
            if(reg->rtid == RiscvTypeID::RIntRegTy) {
                rbb->add_rinst_before_rinst(new RiscvLoadInst(new Type(TypeID::PointerTy), reg, new RiscvIntMem("fp", cur_sp), rbb), ret_inst);
            } else {
                rbb->add_rinst_before_rinst(new RiscvLoadInst(new Type(TypeID::FloatTy), reg, new RiscvIntMem("fp", cur_sp), rbb), ret_inst);
            }
            cur_sp += VARIABLE_ALIGN_BYTE;
        }
    // 还原 fp
    rbb->add_rinst_before_rinst(new RiscvLoadInst(new Type(TypeID::PointerTy), get_reg_op_named("fp"), new RiscvIntMem("fp", cur_sp), rbb), ret_inst);
    // 释放栈帧
    rbb->add_rinst_before_rinst(new RiscvBinaryInst(RiscvInstID::ADDI, get_reg_op_named("sp"), new RiscvConst(-rfunc->base), get_reg_op_named("sp"), rbb), ret_inst);
}

void RiscvGenerator::store_on_stack(Value* val, RiscvFunction* rfunc) {
    if(val == nullptr) {
        cerr<< "[ERROR] Invalid <Value* val>\n";
        exit(1);
    }
    if(allocated_val.count(val)) {
        return;
    }
    // 几种特殊类型，不需要分栈空间
    if(dynamic_cast<Function*>(val) || dynamic_cast<BasicBlock*>(val)) {
        return;
    }
    // 无名常量，或者没有返回值的 ret br call sotre 指令
    if(val->is_const()) {
        return;
    }
    if(dynamic_cast<GlobalVariable*>(val)) { // 全局变量不用给他保存栈上地址，它本身就有对应的内存地址，直接忽略
        auto cur_type = val->type;
        while(true) { // 获取数组最终的元素类型/指针指向的元素类型
            if(cur_type->tid == TypeID::ArrayTy) {
                cur_type = static_cast<ArrayType*>(cur_type)->element_type;
            } else if(cur_type->tid == TypeID::PointerTy) {
                cur_type = static_cast<PointerType*>(cur_type)->pointed_type;
            } else {
                break;
            }
        }
        if(cur_type->tid != TypeID::FloatTy) {
            auto mem = new RiscvIntMem(val->name, 0, true);
            rfunc->reg_alloc->set_val_to_rval(val, mem);
            // rfunc->reg_alloc->set_val_to_mem(val, mem);
        } else {
            auto mem = new RiscvFloatMem(val->name, 0, true);
            rfunc->reg_alloc->set_val_to_rval(val, mem);
            // rfunc->reg_alloc->set_val_to_mem(val, mem);
        }
        return;
    }
    // 除了全局变量之外的参数：函数参数以及函数内变量
    if(dynamic_cast<Argument*>(val)) { // 不用额外分配空间
        if(val->type->tid == TypeID::IntegerTy || val->type->tid == TypeID::PointerTy) { // 整型/指针参数
            if(int_param_count < 8) { // 有剩余寄存器
                rfunc->reg_alloc->set_val_to_reg(val, get_reg_op_named("a" + to_string(int_param_count)));
            }
            auto mem = new RiscvIntMem(get_reg_named("fp"), param_offset);
            rfunc->reg_alloc->set_val_to_rval(val, mem);
            // rfunc->reg_alloc->set_val_to_mem(val, mem);
            int_param_count++;
        } else if(val->type->tid == TypeID::FloatTy) { // 浮点参数
            if(float_param_count < 8) { // 有剩余寄存器
                rfunc->reg_alloc->set_val_to_reg(val, get_reg_op_named("fa" + to_string(float_param_count)));
            }
            auto mem = new RiscvFloatMem(get_reg_named("fp"), param_offset);
            rfunc->reg_alloc->set_val_to_rval(val, mem);
            // rfunc->reg_alloc->set_val_to_mem(val, mem);
            float_param_count++;
        }
        param_offset += VARIABLE_ALIGN_BYTE;
    } else { // 函数内变量
        int cur_sp = rfunc->base;
        RiscvValue* stack_pos = static_cast<RiscvValue*>(new RiscvIntMem(get_reg_named("fp"), cur_sp - VARIABLE_ALIGN_BYTE));
        rfunc->reg_alloc->set_val_to_rval(static_cast<Value*>(val), stack_pos);
        // rfunc->reg_alloc->set_val_to_mem(static_cast<Value*>(val), stack_pos);
        rfunc->add_temp_var(stack_pos);
    }
    allocated_val.insert(val);
}