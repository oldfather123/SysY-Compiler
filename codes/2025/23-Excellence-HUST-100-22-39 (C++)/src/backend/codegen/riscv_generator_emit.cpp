#include "riscv_generator.hpp"
#include "ir_type.hpp"
#include "ir_module.hpp"
#include "riscv_basic_block.hpp"
#include "riscv_function.hpp"
#include "register.hpp"

void RiscvGenerator::handle_binary(RiscvBinaryInst* rinst, RiscvBasicBlock* rbb, RiscvFunction* rfunc) {
    if(rinst->riid == RiscvInstID::ADDI && dynamic_cast<RiscvConst*>(rinst->operands[1])) {
        auto const_op = static_cast<RiscvConst*>(rinst->get_op(1));
        if(const_op->int_val < -2048 || const_op->int_val > 2047) { // 检查立即数是否在 -2048 到 2047 范围内，即 12 位有符号整数
            rbb->add_rinst_before_rinst(new RiscvLiInst(get_reg_op_named("t6"), const_op->int_val, rbb), rinst); // 使用 LI 指令加载立即数
            rinst->riid = RiscvInstID::ADD;
            rinst->set_op(1, get_reg_op_named("t6")); // 使用寄存器 t6 作为操作数
        }
    }
}

void RiscvGenerator::handle_load(RiscvLoadInst* rinst, RiscvBasicBlock* rbb, RiscvFunction* rfunc) {
    if(rinst->riid == RiscvInstID::LW || rinst->riid == RiscvInstID::FLW) {
        // 对于 Load 指令，检查是否需要使用 LI 指令加载立即数
        if(auto mem_addr = dynamic_cast<RiscvIntMem*>(rinst->get_op(1))) {
            if(mem_addr->offset < -2048 || mem_addr->offset > 2047) { // 检查立即数是否在 -2048 到 2047 范围内，即 12 位有符号整数
                rbb->add_rinst_before_rinst(new RiscvLiInst(get_reg_op_named("t6"), mem_addr->offset, rbb), rinst);
                rbb->add_rinst_before_rinst(new RiscvBinaryInst(RiscvInstID::ADD, get_reg_op_named("t6"), get_reg_op_named(mem_addr->base_name), get_reg_op_named("t6"), rbb), rinst);
                // 根据类型设置操作数
                if(rinst->type->tid == TypeID::FloatTy) {
                    rinst->set_op(1, new RiscvFloatMem("t6"));
                } else if(rinst->type->tid == TypeID::IntegerTy || rinst->type->tid == TypeID::PointerTy) {
                    rinst->set_op(1, new RiscvIntMem("t6"));
                } else {
                    cerr << "[Error] [RiscvGenerator::handle_load] Unsupported type for load instruction: " << rinst->type->tid << endl;
                    exit(1);
                }
            }
        }
    }
}

void RiscvGenerator::handle_store(RiscvStoreInst* rinst, RiscvBasicBlock* rbb, RiscvFunction* rfunc) {
    if(rinst->riid == RiscvInstID::SW || rinst->riid == RiscvInstID::FSW) {
        // 对于 Store 指令，检查是否需要使用 LI 指令加载立即数
        if(auto mem_addr = dynamic_cast<RiscvIntMem*>(rinst->get_op(1))) {
            if(mem_addr->offset < -2048 || mem_addr->offset > 2047) { // 检查立即数是否在 -2048 到 2047 范围内，即 12 位有符号整数
                rbb->add_rinst_before_rinst(new RiscvLiInst(get_reg_op_named("t6"), mem_addr->offset, rbb), rinst);
                rbb->add_rinst_before_rinst(new RiscvBinaryInst(RiscvInstID::ADD, get_reg_op_named("t6"), get_reg_op_named(mem_addr->base_name), get_reg_op_named("t6"), rbb), rinst);
                // 根据类型设置操作数
                if(rinst->type->tid == TypeID::FloatTy) {
                    rinst->set_op(1, new RiscvFloatMem("t6"));
                } else if(rinst->type->tid == TypeID::IntegerTy || rinst->type->tid == TypeID::PointerTy) {
                    rinst->set_op(1, new RiscvIntMem("t6"));
                } else {
                    cerr << "[Error] [RiscvGenerator::handle_store] Unsupported type for store instruction: " << rinst->type->tid << endl;
                    exit(1);
                }
            }
        }
    }
}

void RiscvGenerator::handel_push(RiscvPushInst* rinst, RiscvBasicBlock* rbb, RiscvFunction* rfunc) {
    int offset = rinst->offset;
    for(auto op: rinst->operands) {
        offset -= VARIABLE_ALIGN_BYTE;
        rbb->add_rinst_before_rinst(new RiscvStoreInst(m->get_pointer_type(m->int32_type), new RiscvIntMem("sp", offset), op, rbb), rinst);
        cerr << "[Info] add sd by push emit\n";
    }
    rbb->delete_rinst(rinst);
}

void RiscvGenerator::handle_pop(RiscvPopInst* rinst, RiscvBasicBlock* rbb, RiscvFunction* rfunc) {
    int offset = rinst->offset;
    for(auto op: rinst->operands) {
        offset -= VARIABLE_ALIGN_BYTE;
        rbb->add_rinst_before_rinst(new RiscvLoadInst(m->get_pointer_type(m->int32_type), op, new RiscvIntMem("sp", offset), rbb), rinst);
        cerr << "[Info] add ld by pop emit\n";
    }
    // 恢复栈指针
    if (-offset < -2048 || -offset > 2047) {
        rbb->add_rinst_before_rinst(new RiscvLiInst(get_reg_op_named("t6"), -offset, rbb), rinst); // 使用 LI 加载立即数
        rbb->add_rinst_before_rinst(new RiscvBinaryInst(RiscvInstID::ADD, get_reg_op_named("sp"), get_reg_op_named("sp"), get_reg_op_named("t6"), rbb), rinst); // 使用 ADD 更新栈指针
    } else {
        rbb->add_rinst_before_rinst(new RiscvBinaryInst(RiscvInstID::ADDI, get_reg_op_named("sp"), get_reg_op_named("sp"), new RiscvConst(-offset), rbb), rinst); // 恢复栈指针
    }
    rbb->delete_rinst(rinst);
}

void RiscvGenerator::handle_branch(RiscvBranchInst* rinst, RiscvBasicBlock* rbb, RiscvFunction* rfunc) {
    if(!rinst->operands[0] && !rinst->operands[1]) {
    }
}

void RiscvGenerator::handle_icmp(RiscvICmpInst* rinst, RiscvBasicBlock* rbb, RiscvFunction* rfunc) {
    // if(rinst->icmp_op == ICmpOp::EQ || rinst->icmp_op == ICmpOp::NE) {
    //     // 对于 EQ 和 NE 操作，使用 BNE 或 BEQ 指令
    //     if(rinst->operands[0] && rinst->operands[1]) {
    //         rbb->add_rinst_before_rinst(new RiscvBranchInst(rinst->operands[0], static_cast<RiscvBasicBlock*>(rinst->operands[2]), static_cast<RiscvBasicBlock*>(rinst->operands[3]), rbb), rinst);
    //     }
    // } else {
    //     // 其他比较操作符使用 BGT、BLT 等指令
    //     if(rinst->operands[0] && rinst->operands[1]) {
    //         rbb->add_rinst_before_rinst(new RiscvBranchInst(rinst->operands[0], static_cast<RiscvBasicBlock*>(rinst->operands[2]), static_cast<RiscvBasicBlock*>(rinst->operands[3]), rbb), rinst);
    //     }
    // }
}

void RiscvGenerator::inst_emit(RiscvFunction* rfunc) {
    if(!rfunc) {
        cerr << "[Error] [RiscvGenerator::inst_emit] Failed to get RiscvFunction for " << rfunc->name << endl;
        exit(1);
    }
    for(auto& rbb: rfunc->rbb_list) {
        // for(auto& rinst: rbb->rinst_list) {
        for(auto it = rbb->rinst_list.begin(); it != rbb->rinst_list.end(); ) {
            auto rinst = *it;
            it++; // 先保存下一个迭代器位置，因为 rbb->delete_rinst() 会使得 rbb->rinst_list 失效
            if(!rinst) {
                cerr << "[Error] [RiscvGenerator::inst_emit] Encountered a null RiscvInstruction in basic block " << rbb->name << endl;
                continue;
            }
            if(auto binary_rinst = dynamic_cast<RiscvBinaryInst*>(rinst)) {
                handle_binary(binary_rinst, rbb, rfunc);
            } else if(auto load_rinst = dynamic_cast<RiscvLoadInst*>(rinst)) {
                handle_load(load_rinst, rbb, rfunc);
            } else if(auto store_rinst = dynamic_cast<RiscvStoreInst*>(rinst)) {
                handle_store(store_rinst, rbb, rfunc);
            } else if(auto push_rinst = dynamic_cast<RiscvPushInst*>(rinst)) {
                handel_push(push_rinst, rbb, rfunc);
            } else if(auto pop_rinst = dynamic_cast<RiscvPopInst*>(rinst)) {
                handle_pop(pop_rinst, rbb, rfunc);
            } else if(auto branch_rinst = dynamic_cast<RiscvBranchInst*>(rinst)) {
                handle_branch(branch_rinst, rbb, rfunc);
            } else if(auto icmp_rinst = dynamic_cast<RiscvICmpInst*>(rinst)) {
                handle_icmp(icmp_rinst, rbb, rfunc);
            }
        }
    }
}