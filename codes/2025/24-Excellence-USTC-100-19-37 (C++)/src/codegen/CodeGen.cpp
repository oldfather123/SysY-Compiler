#include "CodeGen.hpp"

#include "CodeGenUtil.hpp"
#include "Instruction.hpp"
#include "Register.hpp"
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <iterator>

#define DEBUG 
#ifdef DEBUG
    #define _DEBUG_(string)                                                         \
        std::cout << "[SysYBuilder] DEBUG: " << string << std::endl << std::flush;
#else
    #define _DEBUG_(string)
#endif

int CodeGen::findExponent(int n) //是2的几次幂
{
    // 处理0和负数情况
    if (n <= 0) return -1;
    
    // 检查是否只有一个位是1（2的幂特性：n & (n-1) == 0）
    if ((n & (n - 1)) != 0) return -1;
    
    // 计算n的二进制指数（右移直到找到1的位置）
    int exponent = 0;
    while (n != 1) {
        n >>= 1;       // 右移一位
        exponent++;     // 指数增加
    }
    
    return exponent;
}

void CodeGen::allocate() {
    // 备份 ra fp
    unsigned offset = PROLOGUE_OFFSET_BASE;

    // 为每个参数分配栈空间
    for (auto &arg : context.func->get_args()) {
        auto size = arg.get_type()->get_size();
        // 1字节不需要对齐，4字节(int, float)需要4字节对齐，8字节(指针)需要8字节对齐
        offset = offset + size;
        offset = ALIGN(offset, size);
        context.offset_map[&arg] = -static_cast<int>(offset);
    }

    // 为指令结果分配栈空间
    for (auto &bb : context.func->get_basic_blocks()) {
        for (auto &instr : bb.get_instructions()) {
            // 每个非 void 的定值都分配栈空间
            if (not instr.is_void()) {
                auto size = instr.get_type()->get_size();
                offset = offset + size;
                offset = ALIGN(offset, size);
                context.offset_map[&instr] = -static_cast<int>(offset);
            }
            // alloca 的副作用：分配额外空间
            // alloca 是指针类型，8字节对齐，这里不需要再对齐
            if (instr.is_alloca()) {
                auto *alloca_inst = static_cast<AllocaInst *>(&instr);
                auto alloc_size = alloca_inst->get_alloca_type()->get_size();
                offset += alloc_size;
            }
        }
    }

    // 分配栈空间，需要是 16 的整数倍
    context.frame_size = ALIGN(offset, PROLOGUE_ALIGN);
}

void CodeGen::copy_stmt() {
    for (auto &succ : context.bb->get_succ_basic_blocks()) {
        for (auto &inst : succ->get_instructions()) {
            if (inst.is_phi()) {
                // 遍历后继块中 phi 的定值 bb
                for (unsigned i = 1; i < inst.get_operands().size(); i += 2) {
                    // phi 的定值 bb 是当前翻译块
                    if (inst.get_operand(i) == context.bb) {
                        auto *lvalue = inst.get_operand(i - 1);
                        if (lvalue->get_type()->is_float_type()) {
                            load_to_freg(lvalue, FReg::fa(0));
                            store_from_freg(&inst, FReg::fa(0));
                        } else {
                            load_to_greg(lvalue, Reg::a(0));
                            store_from_greg(&inst, Reg::a(0));
                        }
                        break;
                    }
                    // 如果没有找到当前翻译块，说明是 undef，无事可做
                }
            } else {
                break;
            }
        }
    }
}

void CodeGen::load_to_greg(Value *val, const Reg &reg) {
    assert(val->get_type()->is_integer_type() ||
           val->get_type()->is_pointer_type());
    
    if (auto *constant = dynamic_cast<ConstantInt *>(val)) {
        
        int32_t val = constant->get_value();
        if (IS_IMM_12(val)) {
            append_inst(ADDI WORD, {reg.print(), "zero", std::to_string(val)});
        } else {
            load_large_int32(val, reg);
        }
    } else if (auto *global = dynamic_cast<GlobalVariable *>(val)) {
        append_inst(LOAD_ADDR, {reg.print(), global->get_name()});
    } else {
        load_from_stack_to_greg(val, reg);
    }
}

void CodeGen::load_large_int32(int32_t val, const Reg &reg) {
    append_inst("li", {reg.print(), std::to_string(val)});
    // int32_t high_20 = val >> 12; // si20
    // uint32_t low_12 = val & LOW_12_MASK;
    // append_inst(LUI, {reg.print(), std::to_string(high_20)});
    // append_inst(ORI, {reg.print(), reg.print(), std::to_string(low_12)});
}

void CodeGen::load_large_int64(int64_t val, const Reg &reg) {
    append_inst("li", {reg.print(), std::to_string(val)});
    // auto low_32 = static_cast<int32_t>(val & LOW_32_MASK);
    // load_large_int32(low_32, reg);

    // auto high_32 = static_cast<int32_t>(val >> 32);
    // int32_t high_32_low_20 = (high_32 << 12) >> 12; // si20
    // int32_t high_32_high_12 = high_32 >> 20;        // si12
    // append_inst(LU32I_D, {reg.print(), std::to_string(high_32_low_20)});
    // append_inst(LU52I_D,
    //             {reg.print(), reg.print(), std::to_string(high_32_high_12)});
}

void CodeGen::load_from_stack_to_greg(Value *val, const Reg &reg) {
    auto offset = context.offset_map.at(val);
    auto offset_str = std::to_string(offset);
    auto *type = val->get_type();
    if (IS_IMM_12(offset)) {
        if (type->is_int1_type()) {
            append_inst(LOAD BYTE, {reg.print(), offset_str+"(fp)"});
        } else if (type->is_int32_type()) {
            append_inst(LOAD WORD, {reg.print(), offset_str+"(fp)"});
        } else { // Pointer
            append_inst(LOAD DOUBLE, {reg.print(), offset_str+"(fp)"});
        }
    } else {
        load_large_int64(offset, reg);
        append_inst(ADD, {reg.print(), "fp", reg.print()});
        if (type->is_int1_type()) {
            append_inst(LOAD BYTE, {reg.print(), "0("+reg.print()+")"});
        } else if (type->is_int32_type()) {
            append_inst(LOAD WORD, {reg.print(), "0("+reg.print()+")"});
        } else { // Pointer
            append_inst(LOAD DOUBLE, {reg.print(), "0("+reg.print()+")"});
        }
    }
}

void CodeGen::store_from_greg(Value *val, const Reg &reg) {
    auto offset = context.offset_map.at(val);
    auto offset_str = std::to_string(offset);
    auto *type = val->get_type();
    if (IS_IMM_12(offset)) {
        if (type->is_int1_type()) {
            append_inst(STORE BYTE, {reg.print(), offset_str+"(fp)"});
        } else if (type->is_int32_type()) {
            append_inst(STORE WORD, {reg.print(), offset_str+"(fp)"});
        } else { // Pointer
            append_inst(STORE DOUBLE, {reg.print(), offset_str+"(fp)"});
        }
    } else {
        auto addr = Reg::t(6);
        load_large_int64(offset, addr);
        append_inst(ADD, {addr.print(), "fp", addr.print()});
        if (type->is_int1_type()) {
            append_inst(STORE BYTE, {reg.print(), "0("+addr.print()+")"});
        } else if (type->is_int32_type()) {
            append_inst(STORE WORD, {reg.print(), "0("+addr.print()+")"});
        } else { // Pointer
            append_inst(STORE DOUBLE, {reg.print(), "0("+addr.print()+")"});
        }
    }
}

void CodeGen::load_to_freg(Value *val, const FReg &freg) {
    assert(val->get_type()->is_float_type());
    if (auto *constant = dynamic_cast<ConstantFP *>(val)) {
        float val = constant->get_value();
        load_float_imm(val, freg);
    } else {
        auto offset = context.offset_map.at(val);
        auto offset_str = std::to_string(offset);
        if (IS_IMM_12(offset)) {
            append_inst(FLOAD WORD, {freg.print(), offset_str+"(fp)"});
        } else {
            auto addr = Reg::t(6);
            load_large_int64(offset, addr);
            append_inst(ADD, {addr.print(), "fp", addr.print()});
            append_inst(FLOAD WORD, {freg.print(), "0("+addr.print()+")"});
        }
    }
}

void CodeGen::load_float_imm(float val, const FReg &r) {
    int32_t bytes = *reinterpret_cast<int32_t *>(&val);
    load_large_int32(bytes, Reg::t(6));
    append_inst(MV2FR, {r.print(), Reg::t(6).print()});
}

void CodeGen::store_from_freg(Value *val, const FReg &r) {
    auto offset = context.offset_map.at(val);
    if (IS_IMM_12(offset)) {
        auto offset_str = std::to_string(offset);
        append_inst(FSTORE WORD, {r.print(), offset_str+"(fp)"});
    } else {
        auto addr = Reg::t(6);
        load_large_int64(offset, addr);
        append_inst(ADD, {addr.print(), "fp", addr.print()});
        append_inst(FSTORE WORD, {r.print(), "0("+addr.print()+")"});
    }
}

void CodeGen::gen_prologue() {
    if (IS_IMM_12(-static_cast<int>(context.frame_size))) {
        append_inst("sd ra, -8(sp)");
        append_inst("sd fp, -16(sp)");
        append_inst("addi fp, sp, 0");
        append_inst("addi sp, sp, " +
                    std::to_string(-static_cast<int>(context.frame_size)));
    } else {
        load_large_int64(context.frame_size, Reg::t(0));
        append_inst("sd ra, -8(sp)");
        append_inst("sd fp, -16(sp)");
        append_inst("sub sp, sp, t0");
        append_inst("add fp, sp, t0");
    }


    int garg_cnt = 0;
    int farg_cnt = 0;
    int stack_offset = 0;
    int stack_args = 0;
    // 获取栈上传参的总偏移量
    // 小于 8 字节的参数在栈上必须占用 8 字节空间, 所以栈上传参一律占用8字节
    for (auto &arg : context.func->get_args()) {
        if (arg.get_type()->is_float_type()) {
            if (farg_cnt < 8) {
                farg_cnt++;
                continue; 
            } else {
                stack_offset += 8;
                stack_args++;
            }
        } else { // int or pointer
            if (garg_cnt < 8) {
                garg_cnt++;
                continue; 
            } else {
                stack_offset += 8;
                stack_args++;
            }
        }
    }
    stack_offset = ALIGN(stack_offset, PROLOGUE_ALIGN);
    garg_cnt = 0;
    farg_cnt = 0;
    // 第一次遍历，先把栈上的参数放到正确的位置
    // auto &args = context.func->get_args();
    for (auto &arg : context.func->get_args()) {
        // 顺序遍历
        // auto &arg = *rit;
        if (stack_args <= 0) {
            break; // 已经没有栈上传参了
        }
        
        if (arg.get_type()->is_float_type()) {
            if (farg_cnt < 8) {
                farg_cnt++;
                continue; 
            } else {
                stack_args--;
                stack_offset -= 8;
                auto offset_str = std::to_string((stack_offset));
                auto addr = FReg::ft(0);
                if (IS_IMM_12(stack_offset)) {
                    append_inst(FLOAD WORD, {addr.print(), offset_str + "(fp)"});
                } else {
                    load_large_int64(stack_offset, Reg::t(0));
                    append_inst(ADD, {Reg::t(0).print(), "fp", Reg::t(0).print()});
                    append_inst(FLOAD WORD, {addr.print(), "0(" + Reg::t(0).print() + ")"});
                }
                store_from_freg(&arg, addr);
            }
        } else { // int or pointer
            if (garg_cnt < 8) {
                garg_cnt++;
                continue; 
            } else {
                stack_args--;
                stack_offset -= 8;
                auto offset_str = std::to_string((stack_offset));
                auto addr = Reg::t(0);
                if (arg.get_type()->is_int32_type()) {
                    // int 32
                    if (IS_IMM_12(stack_offset)) {
                        append_inst(LOAD WORD, {addr.print(), offset_str + "(fp)"});
                    } else {
                        load_large_int64(stack_offset, addr);
                        append_inst(ADD, {addr.print(), "fp", addr.print()});
                        append_inst(LOAD WORD, {addr.print(), "0(" + addr.print() + ")"});
                    }
                } else {
                    // pointer
                    if (IS_IMM_12(stack_offset)) {
                        append_inst(LOAD DOUBLE, {addr.print(), offset_str + "(fp)"});
                    } else {
                        load_large_int64(stack_offset, addr);
                        append_inst(ADD, {addr.print(), "fp", addr.print()});
                        append_inst(LOAD DOUBLE, {addr.print(), "0(" + addr.print() + ")"});
                    }
                }
                store_from_greg(&arg, addr);
            }
        }
    }
    // reset farg_cnt and garg_cnt
    garg_cnt = 0;
    farg_cnt = 0;
    // 第二次遍历，处理寄存器中的参数
    for (auto &arg : context.func->get_args()) {
        if (arg.get_type()->is_float_type()) {
            if (farg_cnt >= 8) {
                farg_cnt++;
                continue; // 已经处理过了
            }
            store_from_freg(&arg, FReg::fa(farg_cnt++));
        } else { // int or pointer
            if (garg_cnt >= 8) {
                garg_cnt++;
                continue; // 已经处理过了
            }
            store_from_greg(&arg, Reg::a(garg_cnt++));
        }
    }
}

void CodeGen::gen_epilogue() {
    // TODO 根据你的理解设定函数的 epilogue
    // 函数结尾标签
    auto *func = context.func;
    auto exit_label = func_exit_label_name(func);
    append_inst(exit_label, ASMInstruction::Label);
    if (IS_IMM_12(static_cast<int>(context.frame_size))) {
        append_inst("addi sp, sp, " +
                    std::to_string(static_cast<int>(context.frame_size)));
        append_inst("ld ra, -8(sp)");
        append_inst("ld fp, -16(sp)");
        append_inst("jr ra");
    } else {
        load_large_int64(context.frame_size, Reg::t(0));
        append_inst("add sp, sp, t0");
        append_inst("ld ra, -8(sp)");
        append_inst("ld fp, -16(sp)");
        append_inst("jr ra");
    }
    //throw not_implemented_error{__FUNCTION__};
}


void CodeGen::gen_ret() {
    // TODO 函数返回，思考如何处理返回值、寄存器备份，如何返回调用者地址
    //ret void，但这里is_void_ret函数用不了？
    if(context.inst->get_num_operand() == 0){
        append_inst("addi a0, zero ,0");
    }
    else if(context.inst->get_operand(0)->get_type()->is_float_type()){
        load_to_freg(context.inst->get_operand(0), FReg::fa(0));
    }
    else {
        load_to_greg(context.inst->get_operand(0), Reg::a(0));
    }
    // 跳转到函数末尾的标签
    auto *func = context.func;
    auto exit_label = func_exit_label_name(func);
    append_inst("jal zero, " + exit_label);
    // gen_epilogue();
    //throw not_implemented_error{__FUNCTION__};
}

void CodeGen::gen_br() {
    auto *branchInst = static_cast<BranchInst *>(context.inst);
    if (branchInst->is_cond_br()) {
        // TODO 补全条件跳转的情况
        load_to_greg(branchInst->get_operand(0), Reg::t(0));
        auto *branchtrue = static_cast<BasicBlock *>(branchInst->get_operand(1));
        auto *branchfalse = static_cast<BasicBlock *>(branchInst->get_operand(2));
        if (context.next_bb == branchtrue) {
            append_inst("beq t0, zero, " + label_name(branchfalse));
        } else if (context.next_bb == branchfalse) {
            append_inst("bne t0, zero, " + label_name(branchtrue));
        } else {
            append_inst("beq t0, zero, " + label_name(branchfalse));
            append_inst("jal zero, " + label_name(branchtrue));
        }
        append_inst("bne t0, zero, " + label_name(branchtrue));
        append_inst("jal zero, " + label_name(branchfalse));
        //throw not_implemented_error{__FUNCTION__};
    } else {
        auto *branchbb = static_cast<BasicBlock *>(branchInst->get_operand(0));
        if (context.next_bb == branchbb) {
            return; // fall through
        }
        append_inst("jal zero, " + label_name(branchbb));
    }
}

void CodeGen::gen_binary() {
    bool ifconst0=false;
    bool ifconst1=false;
    int32_t val0,val1;
    if(auto *const0=dynamic_cast<ConstantInt *>(context.inst->get_operand(0)))
    {
        val0=const0->get_value();
        ifconst0=true;
    }
    if(auto *const1=dynamic_cast<ConstantInt *>(context.inst->get_operand(1)))
    {
        val1=const1->get_value();
        ifconst1=true;
    }

    if(ifconst0==false&&ifconst1==false)//两个操作数都不是常数
    {
        //load_to_greg(context.inst->get_operand(0), Reg::t(0));
        //load_to_greg(context.inst->get_operand(1), Reg::t(1));
        
        switch (context.inst->get_instr_type()) {
        case Instruction::add:
        {
            load_to_greg(context.inst->get_operand(0), Reg::t(0));
            if (context.inst->get_operand(0) == context.inst->get_operand(1)) {
                _DEBUG_("test_same");
                output.emplace_back("slliw t2, t0, 1"); // 乘以2
            } else {
                load_to_greg(context.inst->get_operand(1), Reg::t(1));
                output.emplace_back("addw t2, t0, t1");
            }
            break;
        }
        case Instruction::sub:
        {
            if (context.inst->get_operand(0) == context.inst->get_operand(1))
            {
                _DEBUG_("test_sub");
                output.emplace_back("addiw t2, zero ,0");
            }
            else
            {
                load_to_greg(context.inst->get_operand(0), Reg::t(0));
                load_to_greg(context.inst->get_operand(1), Reg::t(1));
                output.emplace_back("subw t2, t0, t1");
            }
            break;
        }
        case Instruction::mul:
        {
            load_to_greg(context.inst->get_operand(0), Reg::t(0));
            load_to_greg(context.inst->get_operand(1), Reg::t(1));
            output.emplace_back("mulw t2, t0, t1");
            break;
        }
        case Instruction::sdiv:
        {
            if (context.inst->get_operand(0) == context.inst->get_operand(1))
            {
                _DEBUG_("test_sdiv");
                output.emplace_back("addiw t2, zero ,1");
            }
            else
            {
                load_to_greg(context.inst->get_operand(0), Reg::t(0));
                load_to_greg(context.inst->get_operand(1), Reg::t(1));
                output.emplace_back("divw t2, t0, t1");
            }
            
            break;
        }
        case Instruction::srem:
        {
            if (context.inst->get_operand(0) == context.inst->get_operand(1))
            {
                _DEBUG_("test_srem");
                output.emplace_back("addiw t2, zero ,0");
            }
            else
            {
                load_to_greg(context.inst->get_operand(0), Reg::t(0));
                load_to_greg(context.inst->get_operand(1), Reg::t(1));
                output.emplace_back("remw t2, t0, t1");
            }
            break;
        }
        default:
            assert(false);
        }
        store_from_greg(context.inst, Reg::t(2));
    }
    else if(ifconst0==true&&ifconst1==true)//两个操作数都是常数，可以直接计算后赋值
    {
        _DEBUG_("int const");
        //load_to_greg(context.inst->get_operand(0), Reg::t(0));
        //load_to_greg(context.inst->get_operand(1), Reg::t(1));
        int result;
        switch (context.inst->get_instr_type()) {
        case Instruction::add:
        {
            result=val0+val1;
            if(IS_IMM_12(result))
                output.emplace_back("addiw t2, zero, "+std::to_string(result));
            else
                output.emplace_back("li t2, "+std::to_string(result));
            break;
        }
        case Instruction::sub:
        {
            result=val0-val1;
            if(IS_IMM_12(result))
                output.emplace_back("addiw t2, zero, "+std::to_string(result));
            else
                output.emplace_back("li t2, "+std::to_string(result));
            break;
        }
        case Instruction::mul:
        {
            result=val0*val1;
            if(IS_IMM_12(result))
                output.emplace_back("addiw t2, zero, "+std::to_string(result));
            else
                output.emplace_back("li t2, "+std::to_string(result));
            break;
        }
        case Instruction::sdiv:
        {
            result=val0/val1;
            if(IS_IMM_12(result))
                output.emplace_back("addiw t2, zero, "+std::to_string(result));
            else
                output.emplace_back("li t2, "+std::to_string(result));
            break;
        }
        case Instruction::srem:
        {
            result=val0%val1;
            if(IS_IMM_12(result))
                output.emplace_back("addiw t2, zero, "+std::to_string(result));
            else
                output.emplace_back("li t2, "+std::to_string(result));
            break;
        }
        default:
            assert(false);
        }
        store_from_greg(context.inst, Reg::t(2));
    }
    
    else if(ifconst0==true&&ifconst1==false)//操作数0是常数
    {
        switch (context.inst->get_instr_type()) {
        case Instruction::add:
        {
            if(val0==0)//copy
            {
                load_to_greg(context.inst->get_operand(1), Reg::t(1));
                store_from_greg(context.inst, Reg::t(1));
            }
            else
            {
                load_to_greg(context.inst->get_operand(1), Reg::t(1));
                if(IS_IMM_12(val0))
                {
                    output.emplace_back("addiw t2, t1, "+std::to_string(val0));
                    store_from_greg(context.inst, Reg::t(2));
                }
                else
                {
                    load_to_greg(context.inst->get_operand(0), Reg::t(0));
                    output.emplace_back("addw t2, t0, t1");
                    store_from_greg(context.inst, Reg::t(2));
                }
            }

            break;
        }
        case Instruction::sub:
        {
            load_to_greg(context.inst->get_operand(1), Reg::t(1));
            if(val0==0)
            {
                output.emplace_back("subw t2, zero, t1");
                store_from_greg(context.inst, Reg::t(2));
            }
            else
            {
                load_to_greg(context.inst->get_operand(0), Reg::t(0));
                output.emplace_back("subw t2, t0, t1");
                store_from_greg(context.inst, Reg::t(2));
            }
            break;
        }
        case Instruction::mul:
        {
            if(val0==0)
            {
                store_from_greg(context.inst, Reg(0));
            }
            else if(val0==1)//copy
            {
                load_to_greg(context.inst->get_operand(1), Reg::t(1));
                store_from_greg(context.inst, Reg::t(1));
            }
            else
            {
                int flag=findExponent(val0);
                if(flag!=-1)//val0是2的幂，用移位代替乘法
                {
                    //指数一定是IMM12，不然数将会巨大
                    load_to_greg(context.inst->get_operand(1), Reg::t(1));
                    output.emplace_back("slliw t2, t1, "+std::to_string(flag));
                    store_from_greg(context.inst, Reg::t(2));
                }
                else//不优化，正常生成
                {
                    load_to_greg(context.inst->get_operand(0), Reg::t(0));
                    load_to_greg(context.inst->get_operand(1), Reg::t(1));
                    output.emplace_back("mulw t2, t0, t1");
                    store_from_greg(context.inst, Reg::t(2));
                }
            }
            break;
        }
        case Instruction::sdiv:
        {
            if(val0==0)
            {
                store_from_greg(context.inst, Reg(0));
            }
            else//正常生成
            {
                load_to_greg(context.inst->get_operand(0), Reg::t(0));
                load_to_greg(context.inst->get_operand(1), Reg::t(1));
                output.emplace_back("divw t2, t0, t1");
                store_from_greg(context.inst, Reg::t(2));
            }
            break;
        }
        case Instruction::srem:
        {
            if(val0==0)
            {
                store_from_greg(context.inst, Reg(0));
            }
            else//正常生成
            {
                load_to_greg(context.inst->get_operand(0), Reg::t(0));
                load_to_greg(context.inst->get_operand(1), Reg::t(1));
                output.emplace_back("remw t2, t0, t1");
                store_from_greg(context.inst, Reg::t(2));
            }
            break;
        }
        default:
            assert(false);
        }
        //store_from_greg(context.inst, Reg::t(2));

    }
    else if(ifconst0==false&&ifconst1==true)//操作数1是常数
    {
        //load_to_greg(context.inst->get_operand(0), Reg::t(0));
        switch (context.inst->get_instr_type()) {
        case Instruction::add:
        {
            if(val1==0)//copy
            {
                load_to_greg(context.inst->get_operand(0), Reg::t(0));
                store_from_greg(context.inst, Reg::t(0));
            }
            else
            {
                load_to_greg(context.inst->get_operand(0), Reg::t(0));
                if(IS_IMM_12(val1))
                {
                    output.emplace_back("addiw t2, t0, "+std::to_string(val1));
                    store_from_greg(context.inst, Reg::t(2));
                }
                else
                {
                    load_to_greg(context.inst->get_operand(1), Reg::t(1));
                    output.emplace_back("addw t2, t0, t1");
                    store_from_greg(context.inst, Reg::t(2));
                }
            }

            break;
        }
        case Instruction::sub:
        {
            load_to_greg(context.inst->get_operand(0), Reg::t(0));
            if(val1==0)//copy
            {
                store_from_greg(context.inst, Reg::t(0));
            }
            else//正常生成
            {
                load_to_greg(context.inst->get_operand(1), Reg::t(1));
                output.emplace_back("subw t2, t0, t1");
                store_from_greg(context.inst, Reg::t(2));
            }

            break;
        }
        case Instruction::mul:
        {
            if(val1==0)
            {
                store_from_greg(context.inst, Reg(0));
            }
            else if(val1==1)//copy
            {
                load_to_greg(context.inst->get_operand(0), Reg::t(0));
                store_from_greg(context.inst, Reg::t(0));
            }
            else
            {
                int flag=findExponent(val1);
                if(flag!=-1)//val1是2的幂，用移位代替乘法
                {
                    //指数一定是IMM12，不然数将会巨大
                    load_to_greg(context.inst->get_operand(0), Reg::t(0));
                    output.emplace_back("slliw t2, t0, "+std::to_string(flag));
                    store_from_greg(context.inst, Reg::t(2));
                }
                else//不优化，正常生成
                {
                    load_to_greg(context.inst->get_operand(0), Reg::t(0));
                    load_to_greg(context.inst->get_operand(1), Reg::t(1));
                    output.emplace_back("mulw t2, t0, t1");
                    store_from_greg(context.inst, Reg::t(2));
                }
            }
            break;

        }
        case Instruction::sdiv:
        {
            if(val1==1)//copy
            {
                load_to_greg(context.inst->get_operand(0), Reg::t(0));
                store_from_greg(context.inst, Reg::t(0));
            }
            else
            {
                //负数的右移和除法并不完全等价
                
                int flag=findExponent(val1);
                if(flag!=-1)//val1是2的幂，用移位代替除法
                {
                    //指数一定是IMM12，不然数将会巨大
                    load_to_greg(context.inst->get_operand(0), Reg::t(0));
                    output.emplace_back("sraiw t2, t0, 31");
                    if(IS_IMM_12(val1-1))
                    {
                        output.emplace_back("andi t1, t2, "+std::to_string(val1-1));
                    }
                    else
                    {    
                        output.emplace_back("li t1, "+std::to_string(val1-1));
                        output.emplace_back("and t1, t2, t1");
                    }
                    output.emplace_back("addw t0, t0, t1");
                    output.emplace_back("sraiw t2, t0, "+std::to_string(flag));
                    store_from_greg(context.inst, Reg::t(2));
                }
                else//不优化，正常生成
                {
                    load_to_greg(context.inst->get_operand(0), Reg::t(0));
                    load_to_greg(context.inst->get_operand(1), Reg::t(1));
                    output.emplace_back("divw t2, t0, t1");
                    store_from_greg(context.inst, Reg::t(2));
                }
                
                
                /*
                load_to_greg(context.inst->get_operand(0), Reg::t(0));
                load_to_greg(context.inst->get_operand(1), Reg::t(1));
                output.emplace_back("divw t2, t0, t1");
                store_from_greg(context.inst, Reg::t(2));
                */
            }

            break;
        }
            
        case Instruction::srem:
        {
            if(val1==1)//等于0
            {
                store_from_greg(context.inst, Reg(0));
            }
            else
            {
                //负数特殊情况？
                
                int flag=findExponent(val1);
                if(flag!=-1)//val1是2的幂，优化
                {
                    //指数一定是IMM12，不然数将会巨大
                    load_to_greg(context.inst->get_operand(0), Reg::t(0));
                    output.emplace_back("sraiw t2, t0, 31");
                    if(IS_IMM_12(val1-1))
                    {
                        output.emplace_back("andi t2, t2, "+std::to_string(val1-1));
                        output.emplace_back("addw t0, t0, t2");
                        output.emplace_back("andi t0, t0, "+std::to_string(val1-1));
                        output.emplace_back("subw t2, t0, t2");
                    }
                    else
                    {    
                        output.emplace_back("li t1, "+std::to_string(val1-1));
                        output.emplace_back("and t2, t2, t1");
                        output.emplace_back("addw t0, t0, t2");
                        output.emplace_back("and t0, t0, t1");
                        output.emplace_back("subw t2, t0, t2");
                    }
                    store_from_greg(context.inst, Reg::t(2));
                }
                else//不优化，正常生成
                {
                    load_to_greg(context.inst->get_operand(0), Reg::t(0));
                    load_to_greg(context.inst->get_operand(1), Reg::t(1));
                    output.emplace_back("remw t2, t0, t1");
                    store_from_greg(context.inst, Reg::t(2));
                }
                /*
                load_to_greg(context.inst->get_operand(0), Reg::t(0));
                load_to_greg(context.inst->get_operand(1), Reg::t(1));
                output.emplace_back("remw t2, t0, t1");
                store_from_greg(context.inst, Reg::t(2));
                */
            }

            break;
        }
        default:
            assert(false);
        }
        //store_from_greg(context.inst, Reg::t(2));
    }
}

void CodeGen::gen_float_binary() {
    // TODO 浮点类型的二元指令
    bool ifconst0=false;
    bool ifconst1=false;
    float val0,val1;
    if(auto *const0=dynamic_cast<ConstantFP *>(context.inst->get_operand(0)))
    {
        val0=const0->get_value();
        ifconst0=true;
    }
    if(auto *const1=dynamic_cast<ConstantFP *>(context.inst->get_operand(1)))
    {
        val1=const1->get_value();
        ifconst1=true;
    }
    if(ifconst0==true&&ifconst1==true)//都是常数
    {
        _DEBUG_("float const");
        float result;
        switch (context.inst->get_instr_type()) {
            case Instruction::fadd:
            {
                result=val0+val1;
                break;
            }
            case Instruction::fsub:
            {
                result=val0-val1;
                break;
            }
            case Instruction::fmul:
            {
                result=val0*val1;
                break;
            }
            case Instruction::fdiv:
            {
                result=val0/val1;
                break;
            }
            //不考虑浮点数的取余运算
            /*
            case Instruction::frem:
            {
                
                break;
            }
            */
            default:
                assert(false);
        }
        load_float_imm(result, FReg::ft(0));
        store_from_freg(context.inst, FReg::ft(0));
    }
    else
    {
        load_to_freg(context.inst->get_operand(0), FReg::ft(0));
        load_to_freg(context.inst->get_operand(1), FReg::ft(1));
        switch (context.inst->get_instr_type()) {
        case Instruction::fadd:
            output.emplace_back("fadd.s ft2, ft0, ft1");
            break;
        case Instruction::fsub:
            output.emplace_back("fsub.s ft2, ft0, ft1");
            break;
        case Instruction::fmul:
            output.emplace_back("fmul.s ft2, ft0, ft1");
            break;
        case Instruction::fdiv:
            output.emplace_back("fdiv.s ft2, ft0, ft1");
            break;
        case Instruction::frem:
            output.emplace_back("frem.s ft2, ft0, ft1");
            break;
        default:
            assert(false);
        }
        store_from_freg(context.inst, FReg::ft(2));
    }
    //throw not_implemented_error{__FUNCTION__};
}

void CodeGen::gen_alloca() {
    /* 我们已经为 alloca 的内容分配空间，在此我们还需保存 alloca
     * 指令自身产生的定值，即指向 alloca 空间起始地址的指针
     */
    // TODO 将 alloca 出空间的起始地址保存在栈帧上
    int offset=context.offset_map[context.inst];
    auto *alloca_inst=static_cast<AllocaInst *>(context.inst);
    auto alloc_size=alloca_inst->get_alloca_type()->get_size();
    int offset2=static_cast<int>(offset-alloc_size);
    if (IS_IMM_12(offset2)) {
        append_inst("addi t0, fp, "+std::to_string(offset2));
    } else {
        load_large_int64(offset2, Reg::t(0));
        append_inst("add t0, fp, t0");
    }
    // append_inst("addi t0, fp, "+std::to_string(offset2));
    store_from_greg(context.inst, Reg::t(0));
    //throw not_implemented_error{__FUNCTION__};
}

void CodeGen::gen_load() {
    auto *ptr = context.inst->get_operand(0);
    auto *type = context.inst->get_type();
    load_to_greg(ptr, Reg::t(0));

    if (type->is_float_type()) {
        append_inst("flw ft0, 0(t0)");
        store_from_freg(context.inst, FReg::ft(0));
    } else {
        // TODO load 整数类型的数据
        if(type->is_int1_type())
            append_inst("lb t1,0(t0)");
        else if(type->is_int32_type())
            append_inst("lw t1,0(t0)");
        else//pointer
            append_inst("ld t1,0(t0)");
        store_from_greg(context.inst, Reg::t(1));
        //throw not_implemented_error{__FUNCTION__};
    }
}

void CodeGen::gen_store() {
    // TODO 翻译 store 指令
    auto *value=context.inst->get_operand(0);
    auto *ptr = context.inst->get_operand(1);
    auto *type = value->get_type();
    load_to_greg(ptr, Reg::t(0));
    if (type->is_float_type()) {
        load_to_freg(value, FReg::ft(0));
        append_inst("fsw ft0, 0(t0)");
    } else {
        load_to_greg(value, Reg::t(1));
        if(type->is_int1_type())
            append_inst("sb t1,0(t0)");
        else if(type->is_int32_type())
            append_inst("sw t1,0(t0)");
        else//pointer
            append_inst("sd t1,0(t0)");

        //throw not_implemented_error{__FUNCTION__};
    }
    //throw not_implemented_error{__FUNCTION__};
}

void CodeGen::gen_icmp() {
    // TODO 处理各种整数比较的情况
    bool ifconst0=false;
    bool ifconst1=false;
    int32_t val0,val1;
    if(auto *const0=dynamic_cast<ConstantInt *>(context.inst->get_operand(0)))
    {
        val0=const0->get_value();
        ifconst0=true;
    }
    if(auto *const1=dynamic_cast<ConstantInt *>(context.inst->get_operand(1)))
    {
        val1=const1->get_value();
        ifconst1=true;
    }

    if(ifconst0==true&&ifconst1==true)//算完赋值
    {
        _DEBUG_("icmp const");
        switch (context.inst->get_instr_type()) {
            case Instruction::ge://大于等于=不小于
            {
                if(val0>=val1)
                    output.emplace_back("addiw t0, zero, 1");
                else
                    output.emplace_back("addiw t0, zero, 0");
                store_from_greg(context.inst, Reg::t(0));
                break;
            }
            case Instruction::gt:
            {
                if(val0>val1)
                    output.emplace_back("addiw t0, zero, 1");
                else
                    output.emplace_back("addiw t0, zero, 0");
                store_from_greg(context.inst, Reg::t(0));
                break;
            }
            case Instruction::le://小于等于=不大于
            {
                if(val0<=val1)
                    output.emplace_back("addiw t0, zero, 1");
                else
                    output.emplace_back("addiw t0, zero, 0");
                store_from_greg(context.inst, Reg::t(0));
                break;
            }
            case Instruction::lt:
            {
                if(val0<val1)
                    output.emplace_back("addiw t0, zero, 1");
                else
                    output.emplace_back("addiw t0, zero, 0");
                store_from_greg(context.inst, Reg::t(0));
                break;
            }
            case Instruction::eq://ne取反即可
            {
                if(val0==val1)
                    output.emplace_back("addiw t0, zero, 1");
                else
                    output.emplace_back("addiw t0, zero, 0");
                store_from_greg(context.inst, Reg::t(0));
                break;
            }
            case Instruction::ne:
            {
                if(val0!=val1)
                    output.emplace_back("addiw t0, zero, 1");
                else
                    output.emplace_back("addiw t0, zero, 0");
                store_from_greg(context.inst, Reg::t(0));
                break;
            }
            default:
                break;
        }
    }
    else 
    {
        auto *op1=context.inst->get_operand(0);
        auto *op2=context.inst->get_operand(1);
        load_to_greg(op1, Reg::t(0));
        load_to_greg(op2, Reg::t(1));
        switch (context.inst->get_instr_type()) {
            case Instruction::ge://大于等于=不小于
            {
                append_inst("slt t2,t0,t1");
                append_inst("addiw t3,zero,1");
                append_inst("subw t3,t3,t2");
                store_from_greg(context.inst, Reg::t(3));
                break;
            }
            case Instruction::gt:
            {
                append_inst("slt t0,t1,t0");

                store_from_greg(context.inst, Reg::t(0));
                break;
            }
            case Instruction::le://小于等于=不大于
            {
                append_inst("slt t2,t1,t0");
                append_inst("addiw t3,zero,1");
                append_inst("subw t3,t3,t2");
                store_from_greg(context.inst, Reg::t(3));
                break;
            }
            case Instruction::lt:
            {
                append_inst("slt t0,t0,t1");
                store_from_greg(context.inst, Reg::t(0));
                break;
            }
            case Instruction::eq://ne取反即可
            {
                append_inst("slt t2,t1,t0");
                append_inst("slt t3,t0,t1");
                append_inst("or t2,t2,t3");
                append_inst("addiw t4,zero,1");
                append_inst("subw t4,t4,t2");
                store_from_greg(context.inst, Reg::t(4));
                break;
            }
            case Instruction::ne:
            {
                append_inst("slt t2,t1,t0");
                append_inst("slt t3,t0,t1");
                append_inst("or t2,t2,t3");
                store_from_greg(context.inst, Reg::t(2));
                break;
            }
            default:
                break;
        }
    }
    //throw not_implemented_error{__FUNCTION__};
}

void CodeGen::gen_fcmp() {
    // TODO 处理各种浮点数比较的情况
    bool ifconst0=false;
    bool ifconst1=false;
    float val0,val1;
    if(auto *const0=dynamic_cast<ConstantFP *>(context.inst->get_operand(0)))
    {
        val0=const0->get_value();
        ifconst0=true;
    }
    if(auto *const1=dynamic_cast<ConstantFP *>(context.inst->get_operand(1)))
    {
        val1=const1->get_value();
        ifconst1=true;
    }
    if(ifconst0==true&&ifconst1==true)
    {
        _DEBUG_("fcmp const");
        switch (context.inst->get_instr_type())
        {
            case Instruction::fge:
            {
                if(val0>=val1)
                    output.emplace_back("addiw t0, zero, 1");
                else
                    output.emplace_back("addiw t0, zero, 0");
                break;
            }
            case Instruction::fgt:
            {
                if(val0>val1)
                    output.emplace_back("addiw t0, zero, 1");
                else
                    output.emplace_back("addiw t0, zero, 0");
                break;
            }
            case Instruction::fle:
            {
                if(val0<=val1)
                    output.emplace_back("addiw t0, zero, 1");
                else
                    output.emplace_back("addiw t0, zero, 0");
                break;
            }
            case Instruction::flt:
            {
                if(val0<val1)
                    output.emplace_back("addiw t0, zero, 1");
                else
                    output.emplace_back("addiw t0, zero, 0");
                break;
            }
            case Instruction::feq:
            {
                if(val0==val1)
                    output.emplace_back("addiw t0, zero, 1");
                else
                    output.emplace_back("addiw t0, zero, 0");
                break;
            }
            case Instruction::fne:
            {
                if(val0!=val1)
                    output.emplace_back("addiw t0, zero, 1");
                else
                    output.emplace_back("addiw t0, zero, 0");
                break;
            }
            default:
                break;
        }
    }
    else
    {
        auto *op1=context.inst->get_operand(0);
        auto *op2=context.inst->get_operand(1);
        load_to_freg(op1, FReg::ft(0));
        load_to_freg(op2, FReg::ft(1));
        switch (context.inst->get_instr_type())
        {
            case Instruction::fge:
            {
                append_inst("fle.s t0, ft1, ft0");
                break;
            }
            case Instruction::fgt:
            {
                append_inst("flt.s t0, ft1, ft0");
                break;
            }
            case Instruction::fle:
            {
                append_inst("fle.s t0, ft0, ft1");
                break;
            }
            case Instruction::flt:
            {
                append_inst("flt.s t0, ft0, ft1");
                break;
            }
            case Instruction::feq:
            {
                append_inst("feq.s t0, ft0, ft1");
                break;
            }
            case Instruction::fne:
            {
                append_inst("feq.s t0, ft0, ft1");
                // append_inst("bne t0, zero, 12");
                // append_inst("addi t0, zero, 1");
                // append_inst("j . +8");
                // append_inst("addi t0, zero, 0");
                append_inst("xori t0, t0, 1");
                break;
            }
            default:
                break;
        }
        // append_inst(FR2GR, {Reg::t(0).print(), FReg::ft(0).print()});
        // append_inst("bceqz fcc0,0xC");
        // append_inst("addi t0,zero,1");
        // append_inst("b 0x8");
        // append_inst("addi t0,zero,0");
    }
    store_from_greg(context.inst,Reg::t(0));
    //throw not_implemented_error{__FUNCTION__};
}

void CodeGen::gen_zext() {
    // TODO 将窄位宽的整数数据进行零扩展
    auto *op=context.inst->get_operand(0);
    auto *typeop=op->get_type();
    auto *typeresult=context.inst->get_type();
    load_to_greg(op, Reg::t(0));
    if(typeresult->is_int32_type())//to 32bits
    {
        if(typeop->is_int1_type())
            append_inst("andi t0, t0, 1");
    }
    else //to 64bits
    {
        if(typeop->is_int1_type())
            append_inst("andi t0, t0, 1");
        if(typeop->is_int32_type())
        {
            append_inst("slli t0, t0, 32");
            append_inst("srli t0, t0, 32");
            //逻辑左移+逻辑右移
        }
    }
    store_from_greg(context.inst, Reg::t(0));
    //throw not_implemented_error{__FUNCTION__};
}

/*
 * 栈上传参的参数
 * 注意，不确定被调用的函数是否已经遍历过，不能在offsetmap中查找offset
 * 使用简单的方法进行传参
 * 因为栈上传参的约定和我们局部变量分配空间的约定不同，采取把sp后移的方式
 * 具体来说，栈上传参一个参数至少8字节，而我们的局部变量分配空间是按照类型大小对齐的
 * e.g, 16个int32类型的参数，栈上传参需要分配64字节空间(寄存器传递8个，栈上传递8个），
 * 而局部变量分配空间只需要64字节
 * 这导致将栈上传参放回局部变量对应地址时会发生重叠，***会导致栈上传参的参数被覆盖***
 * Memory layout
 *       -            
 * +-----------+      | Larger address
 * |           |---+  |
 * +-----------+   |  |
 * |           |   |  |
 * +-----------+<--fp |
 * |           |      |
 * |           |      |
 * |  caller   |      |
 * |           |      |
 * |           |      |
 * |           |      |
 * |           |      |
 * +-----------+<--old sp
 * |   callee's|      |
 * |space for  |      |
 * |sp, fp     |      |
 * |(abandoned)|      |
 * +-----------+      |
 * |   arg n   |      |
 * +-----------+      |
 * |  arg n+1  |      |
 * +-----------+      |
 * |  ....     |      |
 * +-----------+ <--new sp
 * +-----------+     \/ Smaller address
 *       +
 */
void CodeGen::gen_call() {
    // TODO 函数调用，注意我们只需要通过寄存器传递参数，即不需考虑栈上传参的情况
    // 根据riscv64 abi, 小于 8 字节的参数在栈上必须占用 8 字节空间
    int argnum=context.inst->get_num_operand()-1;//第一个operand是函数名
    int farg=0;
    int iarg=0;
    int stack_args = 0;
    // int stack_offset = PROLOGUE_OFFSET_BASE;
    int stack_offset = 0; // 栈上传参的偏移量
    for(int i=1;i<=argnum;i++)
    {
        auto *arg=context.inst->get_operand(i);
        if(arg->get_type()->is_float_type())
        {
            if (farg < 8) {
                load_to_freg(arg, FReg::fa(farg));
            } else {
                // 如果浮点参数超过8个，则需要放到栈上
                // auto size = arg->get_type()->get_size();
                stack_offset += 8;
                auto offset_str = std::to_string((-stack_offset));
                load_to_freg(arg, FReg::ft(0));
                stack_args++;
                auto addr = FReg::ft(0);
                if (IS_IMM_12(-stack_offset)) {
                    append_inst(FSTORE WORD, {addr.print(), offset_str+"(sp)"});
                } else {
                    load_large_int64(-stack_offset, Reg::t(0));
                    append_inst(ADD, {Reg::t(0).print(), "sp", Reg::t(0).print()});
                    append_inst(FSTORE WORD, {addr.print(), "0("+Reg::t(0).print()+")"});
                }
            }
            farg++;
        }
        else
        {
            if (iarg < 8) {
                load_to_greg(arg, Reg::a(iarg));
            } else {
                // 如果整数参数超过8个，则需要放到栈上
                // auto size = arg->get_type()->get_size();
                stack_offset += 8;
                auto offset_str = std::to_string((-stack_offset));
                load_to_greg(arg, Reg::t(0));
                stack_args++;
                auto addr = Reg::t(0);
                if (arg->get_type()->is_int32_type()) {
                    // int 32
                    if (IS_IMM_12(-stack_offset)) {
                        append_inst(STORE WORD, {addr.print(), offset_str+"(sp)"});
                    } else {
                        auto imm = Reg::t(6);
                        load_large_int64(-stack_offset, imm);
                        append_inst(ADD, {imm.print(), "sp", imm.print()});
                        append_inst(STORE WORD, {addr.print(), "0("+imm.print()+")"});
                    }
                } else {
                    // pointer
                    if (IS_IMM_12(-stack_offset)) {
                        append_inst(STORE DOUBLE, {addr.print(), offset_str+"(sp)"});
                    } else {
                        auto imm = Reg::t(6);
                        load_large_int64(-stack_offset, imm);
                        append_inst(ADD, {imm.print(), "sp", imm.print()});
                        append_inst(STORE DOUBLE, {addr.print(), "0("+imm.print()+")"});
                    }
                }
            }
            iarg++;
        }
    }
    // 如果有栈上传参，则需要调整栈指针
    if (stack_offset > 0) {
        // 调整栈指针分配空间
        // 栈指针16字节对齐
        stack_offset = ALIGN(stack_offset, PROLOGUE_OFFSET_BASE);
        if (IS_IMM_12(-stack_offset)) {
            append_inst("addi sp, sp, " + std::to_string(-stack_offset));
        } else {
            load_large_int64(-stack_offset, Reg::t(0));
            append_inst("add sp, sp, t0");
        }
    }
    // if (stack_args > 0) {
    //     // 计算需要分配的空间大小（每个参数8字节）
    //     int stack_size = stack_args * 8;
    //     int offset = 0;
    //     // 调整栈指针分配空间
    //     append_inst("addi sp, sp, " + std::to_string(-stack_size));
    //     for (int i = 8;i <= argnum; i++) {
    //         auto *arg=context.inst->get_operand(i);
    //         std::string offset_str = std::to_string(offset);
    //         offset += 8; // 每个参数占用8字节
    //         if(arg->get_type()->is_float_type()){
    //             load_to_freg(arg, FReg::ft(0));
                
    //         }
    //     }
    // }
    auto *fun=static_cast<Function*>(context.inst->get_operand(0));
    std::string funname=fun->get_name();
    append_inst("jal ra, "+funname);
    // 回调栈指针
    if (stack_offset > 0) {
        // 调整栈指针回到调用前的位置
        if (IS_IMM_12(stack_offset)) {
            append_inst("addi sp, sp, " + std::to_string(stack_offset));
        } else {
            load_large_int64(stack_offset, Reg::t(0));
            append_inst("add sp, sp, t0");
        }
    }
    if(not context.inst->is_void())
    {
        if(context.inst->get_type()->is_float_type())
            store_from_freg(context.inst, FReg::fa(0));
        else
            store_from_greg(context.inst, Reg::a(0));
    }
    //throw not_implemented_error{__FUNCTION__};
}

/*
 * %op = getelementptr [10 x i32], [10 x i32]* %op, i32 0, i32 %op
 * %op = getelementptr        i32,        i32* %op, i32 %op
 *
 * Memory layout
 *       -            ^
 * +-----------+      | Smaller address
 * |  arg ptr  |---+  |
 * +-----------+   |  |
 * |           |   |  |
 * +-----------+   /  |
 * |           |<--   |
 * |           |   \  |
 * |           |   |  |
 * |   Array   |   |  |
 * |           |   |  |
 * |           |   |  |
 * |           |   |  |
 * +-----------+   |  |
 * |  Pointer  |---+  |
 * +-----------+      |
 * |           |      |
 * +-----------+      |
 * |           |      |
 * +-----------+      |
 * |           |      |
 * +-----------+      | Larger address
 *       +
 */
void CodeGen::gen_gep() {
    // TODO 计算内存地址
    auto ptrval=context.inst->get_operand(0);
    load_to_greg(ptrval, Reg::t(0));//基地址
    auto inst = dynamic_cast<GetElementPtrInst *>(context.inst);
    assert(inst != nullptr && "Expected GetElementPtrInst");
    auto *type = inst->get_element_type();
    if(context.inst->get_num_operand()==3)//数组
    {
        long long element_size = type->get_size();
        ConstantInt *elem_size = ConstantInt::get(element_size, m);
        /*
        bool ifconst0=false;
    bool ifconst1=false;
    int32_t val0,val1;
    if(auto *const0=dynamic_cast<ConstantInt *>(context.inst->get_operand(0)))
    {
        val0=const0->get_value();
        ifconst0=true;
    }
    if(auto *const1=dynamic_cast<ConstantInt *>(context.inst->get_operand(1)))
    {
        val1=const1->get_value();
        ifconst1=true;
    }
        */
        int val=elem_size->get_value();
        if(val==1)
        {
            auto index=context.inst->get_operand(2);
            load_to_greg(index, Reg::t(1));
            append_inst("add t1,t0,t1");
        }
        else if(findExponent(val)!=-1)
        {
            int flag=findExponent(val);
            auto index=context.inst->get_operand(2);
            load_to_greg(index, Reg::t(1));
            append_inst("slliw t1,t1,"+std::to_string(flag));
            append_inst("add t1,t0,t1");
        }
        else{
            load_to_greg(elem_size, Reg::t(2));
            auto index=context.inst->get_operand(2);
            load_to_greg(index, Reg::t(1));
            // append_inst("addi t2,zero,4");
            append_inst("mulw t1,t1,t2");
            append_inst("add t1,t0,t1");
        }
    }
    if(context.inst->get_num_operand()==2)//指针
    {
        long long element_size = type->get_size();
        ConstantInt *elem_size = ConstantInt::get(element_size, m);
        int val=elem_size->get_value();
        if(val==1)
        {
            auto index=context.inst->get_operand(1);
            load_to_greg(index, Reg::t(1));
            // append_inst("addi t2,zero,4");
            append_inst("add t1,t0,t1");
        }
        else if(findExponent(val)!=-1)
        {
            int flag=findExponent(val);
            auto index=context.inst->get_operand(1);
            load_to_greg(index, Reg::t(1));
            append_inst("slliw t1,t1,"+std::to_string(flag));
            append_inst("add t1,t0,t1");
        }
        else
        {
            load_to_greg(elem_size, Reg::t(2));
            auto index=context.inst->get_operand(1);
            load_to_greg(index, Reg::t(1));
            // append_inst("addi t2,zero,4");
            append_inst("mul t1,t1,t2");
            append_inst("add t1,t0,t1");
        }
    }
    store_from_greg(context.inst, Reg::t(1));
    //地址是64位的所以要用double双字
    //throw not_implemented_error{__FUNCTION__};
}

void CodeGen::gen_sitofp() {
    // TODO 整数转向浮点数
    auto op=context.inst->get_operand(0);
    load_to_greg(op, Reg::t(0));
    append_inst(GR2FR, {"ft1", "t0"});
    // append_inst("ffint.s.w ft0, ft1");
    store_from_freg(context.inst, FReg::ft(1));
    //throw not_implemented_error{__FUNCTION__};
}

void CodeGen::gen_fptosi() {
    // TODO 浮点数转向整数，注意向下取整(round to zero)
    auto op=context.inst->get_operand(0);
    load_to_freg(op, FReg::ft(0));
    // append_inst("ftintrz.w.s ft1, ft0");
    append_inst("fcvt.w.s t0, ft0, rtz");
    store_from_greg(context.inst, Reg::t(0));
    //throw not_implemented_error{__FUNCTION__};
}

void CodeGen::gen_bitcast() {
    auto op = context.inst->get_operand(0);
    auto type = context.inst->get_type();
    auto op_type = op->get_type();
    // bitcast 只在memset 中使用，没有做更多处理
    load_to_greg(op, Reg::t(0));
    store_from_greg(context.inst, Reg::t(0));
}

std::string FloatToString(float value, int precision = 15) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
}

void CodeGen::gen_init(Constant *init){
    if (dynamic_cast<ConstantZero *>(init) != nullptr) {
        auto size = init->get_type()->get_size();
        append_inst(".space", {std::to_string(size)},
                    ASMInstruction::Atrribute);
    } else if (dynamic_cast<ConstantInt *>(init) != nullptr) {
        auto const_int = dynamic_cast<ConstantInt *>(init);
        int val = const_int->get_value();
        append_inst(".word", {std::to_string(val)},
                    ASMInstruction::Atrribute);
    } else if (dynamic_cast<ConstantFP *>(init) != nullptr) {
        auto const_fp = dynamic_cast<ConstantFP *>(init);
        float val = const_fp->get_value();
        std::string val_str = FloatToString(val);
        append_inst(".float", {val_str},
                    ASMInstruction::Atrribute);
    } else if (dynamic_cast<ConstantArray *>(init) != nullptr) {
        auto const_array = dynamic_cast<ConstantArray *>(init);
        auto size = const_array->get_size_of_array();
        for (unsigned i = 0; i < size; ++i) {
            auto element = const_array->get_element_value(i);
            gen_init(element);
        }
    } else {
        // 其他类型的常量暂不处理
        assert(false && "Unsupported constant type for initialization");
    }
}

void CodeGen::run() {
    // 确保每个函数中基本块的名字都被设置好

    /* 使用 GNU 伪指令为全局变量分配空间
     * 你可以使用 `la.local` 指令将标签 (全局变量) 的地址载入寄存器中, 比如
     * 要将 `a` 的地址载入 t0, 只需要 `la.local t0, a`
     */
    if (!m->get_global_variable().empty()) {
        append_inst("Global variables", ASMInstruction::Comment);
        /* 虽然下面两条伪指令可以简化为一条 `.bss` 伪指令, 但是我们还是选择使用
         * `.section` 将全局变量放到可执行文件的 BSS 段, 原因如下:
         * - 尽可能对齐交叉编译器 loongarch64-unknown-linux-gnu-gcc 的行为
         * - 支持更旧版本的 GNU 汇编器, 因为 `.bss` 伪指令是应该相对较新的指令,
         *   GNU 汇编器在 2023 年 2 月的 2.37 版本才将其引入
         */
        append_inst(".text", ASMInstruction::Atrribute);
        append_inst(".section", {".bss", "\"aw\"", "@nobits"},
                    ASMInstruction::Atrribute);
        append_inst(".align", {"2"}, ASMInstruction::Atrribute);
        for (auto &global : m->get_global_variable()) {
            // 不是0初始化，跳过
            if (global.get_init() != nullptr &&
                dynamic_cast<ConstantZero *>(global.get_init()) == nullptr) {
                continue;
            }
            auto size =
                global.get_type()->get_pointer_element_type()->get_size();
            append_inst(".globl", {global.get_name()},
                        ASMInstruction::Atrribute);
            append_inst(".type", {global.get_name(), "@object"},
                        ASMInstruction::Atrribute);
            append_inst(".size", {global.get_name(), std::to_string(size)},
                        ASMInstruction::Atrribute);
            append_inst(global.get_name(), ASMInstruction::Label);
            // 这里如果要.word初始化则不能加.space，否则占用内存位置不对
            append_inst(".space", {std::to_string(size)},
                       ASMInstruction::Atrribute);
            // auto ty=global.get_type()->get_pointer_element_type();
            // if(ty->is_float_type())
            // {
            //     _DEBUG_("float");
            //     ConstantFP * f=static_cast<ConstantFP *>(global.get_init());
            //     append_inst(".word", {std::to_string(f->get_value())},
            //             ASMInstruction::Atrribute);
            // }
            // else if(ty->is_integer_type())
            // {
            //     _DEBUG_("int");
            //     ConstantInt * in=static_cast<ConstantInt *>(global.get_init());
            //     append_inst(".word", {std::to_string(in->get_value())},
            //             ASMInstruction::Atrribute);
            // }
            // _DEBUG_("globl");
        }
        // 第二个for循环处理初始化的全局变量
        append_inst(".data", ASMInstruction::Atrribute);
        append_inst(".align", {"2"}, ASMInstruction::Atrribute);
        for (auto &global : m->get_global_variable()) {
            // 是0初始化，跳过
            if (global.get_init() == nullptr ||
                dynamic_cast<ConstantZero *>(global.get_init()) != nullptr) {
                continue;
            }
            auto size =
                global.get_type()->get_pointer_element_type()->get_size();
            append_inst(".globl", {global.get_name()},
                        ASMInstruction::Atrribute);
            append_inst(".type", {global.get_name(), "@object"},
                        ASMInstruction::Atrribute);
            append_inst(".size", {global.get_name(), std::to_string(size)},
                        ASMInstruction::Atrribute);
            append_inst(global.get_name(), ASMInstruction::Label);
            auto *init = global.get_init();
            gen_init(init);
        }

    }

    // 函数代码段
    output.emplace_back(".text", ASMInstruction::Atrribute);
    for (auto &func : m->get_functions()) {
        if (not func.is_declaration()) {
            // 更新 context
            context.clear();
            context.func = &func;

            // 函数信息
            append_inst(".globl", {func.get_name()}, ASMInstruction::Atrribute);
            append_inst(".type", {func.get_name(), "@function"},
                        ASMInstruction::Atrribute);
            append_inst(func.get_name(), ASMInstruction::Label);

            // 分配函数栈帧
            allocate();
            // 生成 prologue
            gen_prologue();
            // 如果是Main函数,设置浮点舍入模式为截断
            // if (func.get_name() == "main") {
            //     append_inst("fsrmi 1");
            // }
            auto &bbs = func.get_basic_blocks();
            for (auto _bb = bbs.begin(); _bb != bbs.end(); ++_bb) {
                auto &bb = *_bb;
                context.bb = &bb;
                auto _next = _bb;
                if (_next == bbs.end()) {
                    // 如果是最后一个基本块，则 next_bb 为空
                    context.next_bb = nullptr;
                } else {
                    // 否则，next_bb 是下一个基本块
                    _next++;
                    if (_next != bbs.end()) {
                        context.next_bb = &(*_next);
                    } else {
                        context.next_bb = nullptr;
                    }
                }
                append_inst(label_name(context.bb), ASMInstruction::Label);
                for (auto &instr : bb.get_instructions()) {
                    context.inst = &instr; // 更新 context
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
                    case Instruction::srem:
                        gen_binary();
                        break;
                    case Instruction::fadd:
                    case Instruction::fsub:
                    case Instruction::fmul:
                    case Instruction::fdiv:
                        gen_float_binary();
                        break;
                    case Instruction::alloca:
                        /* 对于 alloca 指令，我们已经为 alloca
                         * 的内容分配空间，在此我们还需保存 alloca
                         * 指令自身产生的定值，即指向 alloca 空间起始地址的指针
                         */
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
                    case Instruction::phi:
                        /* for phi, just convert to a series of
                         * copy-stmts */
                        /* we can collect all phi and deal them at
                         * the end */
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
                    case Instruction::bitcast:
                        gen_bitcast();
                        break;
                    }
                }
            }
            // 生成 epilogue
            gen_epilogue();
        }
    }
}

std::string CodeGen::print() const {
    std::string result;
    for (const auto &inst : output) {
        result += inst.format();
    }
    return result;
}
