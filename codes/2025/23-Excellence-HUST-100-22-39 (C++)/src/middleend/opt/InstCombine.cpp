#include "InstCombine.hpp"

bool InstCombine::identity_simplification(Function* func) {
    // std::cerr << "InstCombine::identity_simplification called\n";
    bool changed = false;
    std::set<Instruction*> inst_to_delete; // 被替换的指令
    for(auto bb: func->bb_list) {
        // std::cerr << "-----------------------------------------------------------------------\n";
        for(auto inst: bb->inst_list) {
            // std::cerr << inst->print() << "\n";
            switch(inst->iid) {
                case IRInstID::Add: {
                    // std::cerr << "add\n";
                    if(auto op1 = dynamic_cast<ConstInt*>(inst->get_op(0))) {
                        if(op1->val == 0) {
                            // std::cerr << "op1 is 0\n";
                            inst->replace_use_with(inst->get_op(1));
                            inst_to_delete.insert(inst);
                            changed = true;
                        }
                    }
                    if(auto op2 = dynamic_cast<ConstInt*>(inst->get_op(1))) {
                        if(op2->val == 0) {
                            // std::cerr << "op2 is 0\n";
                            inst->replace_use_with(inst->get_op(0));
                            inst_to_delete.insert(inst);
                            changed = true;
                        }
                    }
                    break;
                }
                case IRInstID::Sub: {
                    // std::cerr << "sub\n";
                    // 由于不存在取负指令，故 0 - x 的情况无需进一步处理
                    if(auto op2 = dynamic_cast<ConstInt*>(inst->get_op(1))) {
                        if(op2->val == 0) { // x - 0 = x
                            // std::cerr << "op2 is 0\n";
                            inst->replace_use_with(inst->get_op(0));
                            inst_to_delete.insert(inst);
                            changed = true;
                        }
                    }
                    break;
                }
                case IRInstID::Mul: {
                    // std::cerr << "mul\n";
                    if(auto op1 = dynamic_cast<ConstInt*>(inst->get_op(0))) {
                        if(op1->val == 1) {
                            // std::cerr << "op1 is 1\n";
                            inst->replace_use_with(inst->get_op(1));
                            inst_to_delete.insert(inst);
                            changed = true;
                        } else if(op1->val == 0) {
                            // std::cerr << "op1 is 0\n";
                            inst->replace_use_with(new ConstInt(m->int32_type, 0));
                            inst_to_delete.insert(inst);
                            changed = true;
                        }
                    }
                    if(auto op2 = dynamic_cast<ConstInt*>(inst->get_op(1))) {
                        if(op2->val == 1) {
                            // std::cerr << "op2 is 1\n";
                            inst->replace_use_with(inst->get_op(0));
                            inst_to_delete.insert(inst);
                            changed = true;
                        } else if(op2->val == 0) {
                            // std::cerr << "op2 is 0\n";
                            inst->replace_use_with(new ConstInt(m->int32_type, 0));
                            inst_to_delete.insert(inst);
                            changed = true;
                        }
                    }
                    break;
                }
                case IRInstID::Div: {
                    // std::cerr << "sdiv\n";
                    if(auto op2 = dynamic_cast<ConstInt*>(inst->get_op(1))) {
                        if(op2->val == 1) {
                            // std::cerr << "op2 is 1\n";
                            inst->replace_use_with(inst->get_op(0));
                            inst_to_delete.insert(inst);
                            changed = true;
                        }
                    }
                    break;
                }
                case IRInstID::FAdd: {
                    // std::cerr << "fadd\n";
                    if(auto op1 = dynamic_cast<ConstFloat*>(inst->get_op(0))) {
                        if(op1->val == 0.0) {
                            // std::cerr << "op1 is 0.0\n";
                            inst->replace_use_with(inst->get_op(1));
                            inst_to_delete.insert(inst);
                            changed = true;
                        }
                    }
                    if(auto op2 = dynamic_cast<ConstFloat*>(inst->get_op(1))) {
                        if(op2->val == 0.0) {
                            // std::cerr << "op2 is 0.0\n";
                            inst->replace_use_with(inst->get_op(0));
                            inst_to_delete.insert(inst);
                            changed = true;
                        }
                    }
                    break;
                }
                case IRInstID::FSub: {
                    // std::cerr << "fsub\n";
                    if(auto op2 = dynamic_cast<ConstFloat*>(inst->get_op(1))) {
                        if(op2->val == 0.0) {
                            // std::cerr << "op2 is 0.0\n";
                            inst->replace_use_with(inst->get_op(0));
                            inst_to_delete.insert(inst);
                            changed = true;
                        }
                    }
                    break;
                }
                case IRInstID::FMul: {
                    // std::cerr << "fmul\n";
                    if(auto op1 = dynamic_cast<ConstFloat*>(inst->get_op(0))) {
                        if(op1->val == 1.0) {
                            // std::cerr << "op1 is 1.0\n";
                            inst->replace_use_with(inst->get_op(1));
                            inst_to_delete.insert(inst);
                            changed = true;
                        } else if(op1->val == 0.0) {
                            // std::cerr << "op1 is 0.0\n";
                            inst->replace_use_with(new ConstFloat(m->float_type, 0.0));
                            inst_to_delete.insert(inst);
                            changed = true;
                        }
                    }
                    if(auto op2 = dynamic_cast<ConstFloat*>(inst->get_op(1))) {
                        if(op2->val == 1.0) {
                            // std::cerr << "op2 is 1.0\n";
                            inst->replace_use_with(inst->get_op(0));
                            inst_to_delete.insert(inst);
                            changed = true;
                        } else if(op2->val == 0.0) {
                            // std::cerr << "op2 is 0.0\n";
                            inst->replace_use_with(new ConstFloat(m->float_type, 0.0));
                            inst_to_delete.insert(inst);
                            changed = true;
                        }
                    }
                    break;
                }
                case IRInstID::FDiv: {
                    // std::cerr << "fdiv\n";
                    if(auto op2 = dynamic_cast<ConstFloat*>(inst->get_op(1))) {
                        if(op2->val == 1.0) {
                            // std::cerr << "op2 is 1.0\n";
                            inst->replace_use_with(inst->get_op(0));
                            inst_to_delete.insert(inst);
                            changed = true;
                        }
                    }
                    break;
                }
                case IRInstID::Shl: {
                    // std::cerr << "shl\n";
                    if(auto op2 = dynamic_cast<ConstInt*>(inst->get_op(1))) {
                        if(op2->val == 0) {
                            // std::cerr << "op2 is 0\n";
                            inst->replace_use_with(inst->get_op(0));
                            inst_to_delete.insert(inst);
                            changed = true;
                        }
                    }
                    break;
                }
                case IRInstID::LShr: {
                    // std::cerr << "lshr\n";
                    if(auto op2 = dynamic_cast<ConstInt*>(inst->get_op(1))) {
                        if(op2->val == 0) {
                            // std::cerr << "op2 is 0\n";
                            inst->replace_use_with(inst->get_op(0));
                            inst_to_delete.insert(inst);
                            changed = true;
                        }
                    }
                    break;
                }
                case IRInstID::AShr: {
                    // std::cerr << "ashr\n";
                    if(auto op2 = dynamic_cast<ConstInt*>(inst->get_op(1))) {
                        if(op2->val == 0) {
                            // std::cerr << "op2 is 0\n";
                            inst->replace_use_with(inst->get_op(0));
                            inst_to_delete.insert(inst);
                            changed = true;
                        }
                    }
                    break;
                }
                case IRInstID::Phi: {
                    // std::cerr << "phi\n";
                    if(auto phi_inst = dynamic_cast<PhiInst*>(inst)) {
                        if(phi_inst->operands.size() == 0) {
                            // std::cerr << "Phi instruction has no operands, deleting\n";
                            inst_to_delete.insert(inst);
                            changed = true;
                        } else if(phi_inst->operands.size() == 2) {
                            // std::cerr << "Phi instruction has only one pair of operands\n";
                            inst->replace_use_with(phi_inst->get_op(0));
                            inst_to_delete.insert(inst);
                            changed = true;
                        } else if(phi_inst->operands.size() > 2) {
                            // // std::cerr << "Phi instruction has several operands\n";
                            // auto first_op = phi_inst->get_op(0);
                            // bool same = true;
                            // for(int i = 2; i < phi_inst->operands.size(); i += 2) {
                            //     auto op = phi_inst->get_op(i);
                            //     if(op != first_op) {
                            //         same = false;
                            //         break;
                            //     }
                            // }
                            // if(same) {
                            //     // std::cerr << "All operands are the same, replacing with first operand\n";
                            //     inst->replace_use_with(first_op);
                            //     inst_to_delete.insert(inst);
                            //     changed = true;
                            // } else {
                            //     // std::cerr << "Operands are not the same, cannot simplify\n";
                            //     continue; // 无法简化
                            // }
                        }
                    }
                    break;
                }
                default: {
                    // std::cerr << "default\n";
                    break;
                }
            }
        }
    }
    // 删除指令
    for(auto inst: inst_to_delete) {
        if(inst->parent->delete_inst(inst)) {
            // std::cerr << "Deleted instruction: " << inst->print() << "\n";
        } else {
            // std::cerr << "Failed to delete instruction: " << inst->print() << "\n";
        }
    }
    return changed;
}

bool InstCombine::chain_combination(Function* func) {
    bool changed = false;
    for(auto bb: func->bb_list) {
        // std::cerr << "-------------------------------------------------------------------------\nBlock: " << bb->name << "\n";
        // for(auto inst: bb->inst_list) {
        for(auto it = bb->inst_list.begin(); it != bb->inst_list.end();) {
            auto inst = *it;
            it++; // 迭代器前移，防止删除时迭代器失效
            auto iid = inst->iid;
            if(!(iid == IRInstID::Add || iid == IRInstID::Sub || iid == IRInstID::Mul ||
               iid == IRInstID::Div || iid == IRInstID::FAdd || iid == IRInstID::FSub ||
               iid == IRInstID::FMul || iid == IRInstID::FDiv)) {
                continue; // 只处理加减乘除和浮点加减乘除
            }
            auto op1 = inst->get_op(0), op2 = inst->get_op(1);
            // 匹配：嵌套结构
            // TODO: 暂时只支持变量在操作数1的位置
            Instruction* inner = dynamic_cast<Instruction*>(op1);
            Const* outter_const = dynamic_cast<Const*>(op2);
            if(!(inner && outter_const)) { // 需要一个指令和一个常量
                continue;
            }
            // std::cerr << "Outer instruction: " << inst->print() << "\n";
            // std::cerr << "Inner instruction: " << inner->print() << "\n";
            // TODO: 位运算的合并
            if(inner->iid != iid) {
                if(inner->iid == IRInstID::Add && iid == IRInstID::Sub      // a+1-2
                || inner->iid == IRInstID::Sub && iid == IRInstID::Add      // a-1+2
                || inner->iid == IRInstID::Mul && iid == IRInstID::Div     // a*1/2
                || inner->iid == IRInstID::Div && iid == IRInstID::Mul     // a/1*2
                || inner->iid == IRInstID::FAdd && iid == IRInstID::FSub    // fa+1.0-2.0
                || inner->iid == IRInstID::FSub && iid == IRInstID::FAdd    // fa-1.0+2.0
                || inner->iid == IRInstID::FMul && iid == IRInstID::FDiv    // fa*1.0/2.0
                || inner->iid == IRInstID::FDiv && iid == IRInstID::FMul) { // fa/1.0*2.0
                    // std::cerr << "Inner instruction type matches outer instruction type\n";
                } else {
                    // std::cerr << "Inner instruction type does not match outer instruction type\n";
                    continue; // 内部指令的操作类型不匹配
                }
            }
            // 判断：常量是否可以与内部指令合并
            Const* inner_const = dynamic_cast<Const*>(inner->get_op(1));
            if(!inner_const) { // 内部指令的第二个操作数不是常量
                // std::cerr << "Inner instruction does not have a constant operand\n";
                continue;
            }
            if(inner_const->type->tid != outter_const->type->tid) { // 内部和外部常量类型不匹配
                // std::cerr << "Inner and outer constants have different types\n";
                continue;
            }
            if(inner->use_list.size() > 1) { // 内部指令的使用次数大于1，不能直接替换
                // std::cerr << "Inner instruction has multiple uses, cannot replace\n";
                continue;
            }
            // 合并：创建新的常量
            Const* new_const = nullptr;
            // std::cerr << "Outer instruction: " << inst->print() << "\n";
            // std::cerr << "Inner instruction: " << inner->print() << "\n";
            if(auto const1 = dynamic_cast<ConstInt*>(inner_const)) {
                auto const2 = dynamic_cast<ConstInt*>(outter_const);
                int new_value = 0;
                if(iid == IRInstID::Add) {
                    if(inner->iid == IRInstID::Add) { // a+1+2
                        new_value = const1->val + const2->val;
                    } else if(inner->iid == IRInstID::Sub) { // a+1-2
                        new_value = const1->val - const2->val;
                    }
                } else if(iid == IRInstID::Sub) { 
                    if(inner->iid == IRInstID::Add) { // a-1+2
                        new_value = const1->val - const2->val;
                    } else if(inner->iid == IRInstID::Sub) { // a-1-2
                        new_value = const1->val + const2->val;
                    }
                } else if(iid == IRInstID::Mul) {
                    if(inner->iid == IRInstID::Mul) { // a*1*2
                        new_value = const1->val * const2->val;
                    } else if(inner->iid == IRInstID::Div) { // a*1/2
                        int r = const1->val % const2->val; // 余数
                        if(r != 0) {
                            // std::cerr << "Warning: Division with remainder, precision may be lost\n";
                            continue; // 如果有余数，可能会导致精度问题
                        } else {
                            new_value = const1->val / const2->val;
                        }
                    }
                } else if(iid == IRInstID::Div) {
                    if(inner->iid == IRInstID::Mul) { // a/1*2
                        int r = const1->val % const2->val; // 余数
                        if(r != 0) {
                            // std::cerr << "Warning: Division with remainder, precision may be lost\n";
                            continue; // 如果有余数，可能会导致精度问题
                        } else {
                            new_value = const1->val / const2->val;
                        }
                    } else if(inner->iid == IRInstID::Div) { // a/1/2
                        new_value = const1->val * const2->val;
                    }
                }
                new_const = new ConstInt(m->int32_type, new_value);
            } else if(auto const1 = dynamic_cast<ConstFloat*>(inner_const)) {
                auto const2 = dynamic_cast<ConstFloat*>(outter_const);
                float new_value = 0.0;
                switch(iid) { // 浮点数除法本身就会有精度损失，在此处无需考虑
                    case IRInstID::FAdd:
                        if(inner->iid == IRInstID::FAdd) { // fa+1.0+2.0
                            new_value = const1->val + const2->val;
                        } else if(inner->iid == IRInstID::FSub) { // fa+1.0-2.0
                            new_value = const1->val - const2->val;
                        }
                        break;
                    case IRInstID::FSub:
                        if(inner->iid == IRInstID::FAdd) { // fa-1.0+2.0
                            new_value = const1->val - const2->val;
                        } else if(inner->iid == IRInstID::FSub) { // fa-1.0-2.0
                            new_value = const1->val + const2->val;
                        }
                        break;
                    case IRInstID::FMul:
                        if(inner->iid == IRInstID::FMul) { // fa*1.0*2.0
                            new_value = const1->val * const2->val;
                        } else if(inner->iid == IRInstID::FDiv) { // fa*1.0/2.0
                            new_value = const1->val / const2->val;
                        }
                        break;
                    case IRInstID::FDiv:
                        if(inner->iid == IRInstID::FMul) { // fa/1.0*2.0
                            new_value = const1->val / const2->val;
                        } else if(inner->iid == IRInstID::FDiv) { // fa/1.0/2.0
                            new_value = const1->val * const2->val;
                        }
                        break;
                    default:
                        continue; // 不处理其他操作
                }
                new_const = new ConstFloat(m->float_type, new_value);
            }
            if(!new_const) {
                // std::cerr << "Failed to create new constant\n";
                continue; // 无法创建新的常量
            }
            // 替换：将内部指令的操作数替换为新的常量，将外部指令的引用替换为内部指令
            // std::cerr << "Inner instruction before replacement: " << inner->print() << "\n";
            inner->replace_op(1, new_const);
            // std::cerr << "Replacing outer instruction with inner instruction\n";
            // std::cerr << "Inner instruction after replacement: " << inner->print() << "\n";
            inst->replace_use_with(inner); // 替换外部指令的所有使用为内部指令
            // 清理：删除无用的指令
            // std::cerr << "Deleting outer instruction: " << inst->print() << "\n";
            inst->parent->delete_inst(inst); // 直接删除外部指令
            changed = true; // 标记为已改变
            // std::cerr << "-----------------------------------------------------------------------\n";
        }
    }
    return changed;
}

bool InstCombine::strength_reduction(Function* func) {
    bool changed = false;
    std::set<Instruction*> inst_to_delete; // 被替换的指令
    for(auto bb: func->bb_list) {
        for(auto inst: bb->inst_list) {
            //! 不处理除法，因为如果变量为负数，除法转换为位移会导致结果不正确
            // if(inst->iid != IRInstID::Mul && inst->iid != IRInstID::Div) {
            if(inst->iid != IRInstID::Mul) {
                continue; // 只处理整型乘法和除法
            }
            Value* var = nullptr;
            ConstInt* const_val = nullptr;
            if(inst->iid == IRInstID::Mul) {
                if(auto op1 = dynamic_cast<ConstInt*>(inst->get_op(0))) {
                    const_val = op1;
                    var = inst->get_op(1);
                } else if(auto op2 = dynamic_cast<ConstInt*>(inst->get_op(1))) {
                    const_val = op2;
                    var = inst->get_op(0);
                } else {
                    continue; // 不是常量和变量的组合
                }
            } else if(inst->iid == IRInstID::Div) {
                //! 除法中只有 x / 2 能被优化为 x >> 1，2 / x 不能被优化
                if(auto op2 = dynamic_cast<ConstInt*>(inst->get_op(1))) {
                    const_val = op2;
                    var = inst->get_op(0);
                } else {
                    continue;
                }
            }
            if(!const_val || const_val->val <= 0) {
                continue; // 常量值无效
            }
            int val = const_val->val;
            // TODO: 暂时只处理 2 的幂次方
            if((val & (val - 1)) != 0) { // 不是2的幂，不能进行位移优化
                continue;
            }
            int shift = 0;
            while(val >>= 1) {
                shift++;
            }
            auto shift_const = new ConstInt(m->int32_type, shift);
            Instruction* new_inst = nullptr;
            if(inst->iid == IRInstID::Mul) {
                // std::cerr << "Replacing multiplication with shift: " << inst->print() << "\n";
                new_inst = new BinaryInst(m->int32_type, IRInstID::Shl, var, shift_const, inst->parent, InsertPos::None);
                new_inst->name = std::to_string(new_inst->parent->parent->ssa_id++);
            } else if(inst->iid == IRInstID::Div) {
                // std::cerr << "Replacing division with shift: " << inst->print() << "\n";
                new_inst = new BinaryInst(m->int32_type, IRInstID::AShr, var, shift_const, inst->parent, InsertPos::None);
                new_inst->name = std::to_string(new_inst->parent->parent->ssa_id++);
            }
            if(!new_inst) {
                // std::cerr << "Failed to create new instruction\n";
                continue; // 无法创建新的指令
            }
            // 替换：将原指令的所有使用替换为新指
            // std::cerr << "Replacing old instruction: " << inst->print() << "\n";
            // std::cerr << "Insert new instruction:" << new_inst->print() << "\n";
            inst->replace_use_with(new_inst);
            // 插入新指令到对应位置
            inst->parent->add_inst_before_inst(new_inst, inst);
            // 清理：删除原指令
            // std::cerr << "Deleted instruction: " << inst->print() << "\n";
            inst_to_delete.insert(inst); // 将原指令加入删除列表
            changed = true; // 标记为已改变
            // std::cerr << "-----------------------------------------------------------------------\n";
        }
    }
    for(auto inst: inst_to_delete) {
        if(inst->parent->delete_inst(inst)) {
            // std::cerr << "Deleted instruction: " << inst->print() << "\n";
        } else {
            // std::cerr << "Failed to delete instruction: " << inst->print() << "\n";
        }
    }
    return changed;
}

void InstCombine::execute() {
    for(auto func: m->func_list) {
        if(func->bb_list.empty()) {
            // std::cerr << "Function " << func->name << " has no basic blocks, skipping.\n";
            continue; // 如果函数没有基本块，跳过
        }
        // std::cerr << func->name << "\n";
        while(true) {
            // std::cerr << "Executing InstCombine on function: " << func->name << "\n";
            bool changed = false;
            changed |= identity_simplification(func);
            // std::cerr << m->print() << "\n";
            // std::cerr << "*****************************************************************\n";
            changed |= chain_combination(func);
            // std::cerr << m->print() << "\n";
            // std::cerr << "*****************************************************************\n";
            changed |= strength_reduction(func);
            // std::cerr << m->print() << "\n";
            // std::cerr << "*****************************************************************\n";
            if(!changed) {
                // std::cerr << "No more changes in function: " << func->name << "\n";
                break; // 没有更多的变化，退出循环
            }
        }
    }
}