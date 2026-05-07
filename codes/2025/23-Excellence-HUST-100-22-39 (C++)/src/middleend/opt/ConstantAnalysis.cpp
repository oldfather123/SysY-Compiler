#include "ConstantAnalysis.hpp"

ConstInt* ConstantAnalysis::cal_int(IRInstID iid, ConstInt* v1, ConstInt* v2) {
    int a = v1->val, b = v2->val;
    switch(iid) {
        case IRInstID::Add:
            return new ConstInt(m->int32_type, a + b);
        case IRInstID::Sub:
            return new ConstInt(m->int32_type, a - b);
        case IRInstID::Mul:
            return new ConstInt(m->int32_type, a * b);
        case IRInstID::Div:
            return new ConstInt(m->int32_type, a / b);
        case IRInstID::Rem:
            return new ConstInt(m->int32_type, a % b);
        case IRInstID::Shl:
            return new ConstInt(m->int32_type, a << b);
        case IRInstID::LShr:
            return new ConstInt(m->int32_type, (unsigned)a >> b);
        case IRInstID::AShr:
            return new ConstInt(m->int32_type, a >> b);
        default:
            return nullptr;
    }
}

ConstFloat* ConstantAnalysis::cal_float(IRInstID iid, ConstFloat* v1, ConstFloat* v2) {
    float a = v1->val, b = v2->val;
    switch(iid) {
        case IRInstID::FAdd:
            return new ConstFloat(m->float_type, a + b);
        case IRInstID::FSub:
            return new ConstFloat(m->float_type, a - b);
        case IRInstID::FMul:
            return new ConstFloat(m->float_type, a * b);
        case IRInstID::FDiv:
            return new ConstFloat(m->float_type, a / b);
        default:
            return nullptr;
    }
}

ConstInt* ConstantAnalysis::cal_icmp(ICmpOp op, ConstInt* v1, ConstInt* v2) {
    int lhs = v1->val, rhs = v2->val;
    switch (op) {
        case ICmpOp::EQ:
            return new ConstInt(m->int1_type, lhs == rhs);
        case ICmpOp::NE:
            return new ConstInt(m->int1_type, lhs != rhs);
        case ICmpOp::GT:
            return new ConstInt(m->int1_type, lhs > rhs);
        case ICmpOp::GE:
            return new ConstInt(m->int1_type, lhs >= rhs);
        case ICmpOp::LE:
            return new ConstInt(m->int1_type, lhs <= rhs);
        case ICmpOp::LT:
            return new ConstInt(m->int1_type, lhs < rhs);
        default:
            return nullptr;
    }
}

ConstInt* ConstantAnalysis::cal_fcmp(FCmpOp op, ConstFloat* v1, ConstFloat* v2) {
    float lhs = v1->val;
    float rhs = v2->val;
    switch (op) {
        case FCmpOp::FEQ:
            return new ConstInt(m->int1_type, lhs == rhs);
        case FCmpOp::FNE:
            return new ConstInt(m->int1_type, lhs != rhs);
        case FCmpOp::FGT:
            return new ConstInt(m->int1_type, lhs > rhs);
        case FCmpOp::FGE:
            return new ConstInt(m->int1_type, lhs >= rhs);
        case FCmpOp::FLE:
            return new ConstInt(m->int1_type, lhs <= rhs);
        case FCmpOp::FLT:
            return new ConstInt(m->int1_type, lhs < rhs);
        default:
            return nullptr;
    }
}

bool ConstantAnalysis::const_fold_prop(Function* func) {
    unordered_set<Instruction*> inst_to_delete; // 用于存储需要删除的指令
    for(auto bb: func->bb_list) {
        for(auto inst: bb->inst_list) {
            switch(inst->iid) {
                // 二元操作
                case IRInstID::Add:
                case IRInstID::Sub:
                case IRInstID::Mul:
                case IRInstID::Div:
                case IRInstID::Rem:
                case IRInstID::Shl:
                case IRInstID::LShr:
                case IRInstID::AShr: {
                    ConstInt* int_op1 = dynamic_cast<ConstInt*>(inst->get_op(0));
                    ConstInt* int_op2 = dynamic_cast<ConstInt*>(inst->get_op(1));
                    if(int_op1 && int_op2) { // 二者均为常量，可以折叠
                        auto intRes = cal_int(inst->iid, int_op1, int_op2); // 常量折叠
                        if(intRes) {
                            inst->replace_use_with(intRes); // 常量传播
                            inst_to_delete.insert(inst);
                        }
                    }
                    break;
                }
                case IRInstID::FAdd:
                case IRInstID::FSub:
                case IRInstID::FMul:
                case IRInstID::FDiv: {
                    ConstFloat* float_op1 = dynamic_cast<ConstFloat*>(inst->get_op(0));
                    ConstFloat* float_op2 = dynamic_cast<ConstFloat*>(inst->get_op(1));
                    if(float_op1 && float_op2) {
                        auto floaRes = cal_float(inst->iid, float_op1, float_op2);
                        if(floaRes) {
                            inst->replace_use_with(floaRes);
                            inst_to_delete.insert(inst);
                        }
                    }
                    break;
                }
                case IRInstID::ICmp: {
                    ConstInt* int_op1 = dynamic_cast<ConstInt*>(inst->get_op(0));
                    ConstInt* int_op2 = dynamic_cast<ConstInt*>(inst->get_op(1));
                    if(int_op1 && int_op2) {
                        auto boolRes = cal_icmp(dynamic_cast<ICmpInst*>(inst)->icmp_op, int_op1, int_op2);
                        if(boolRes) {
                            inst->replace_use_with(boolRes);
                            inst_to_delete.insert(inst);
                        }
                    }
                    break;
                }
                case IRInstID::FCmp: {
                    ConstFloat* float_op1 = dynamic_cast<ConstFloat*>(inst->get_op(0));
                    ConstFloat* float_op2 = dynamic_cast<ConstFloat*>(inst->get_op(1));
                    if(float_op1 && float_op2) {
                        auto boolRes = cal_fcmp(dynamic_cast<FCmpInst*>(inst)->fcmp_op, float_op1, float_op2);
                        if(boolRes) {
                            inst->replace_use_with(boolRes);
                            inst_to_delete.insert(inst);
                        }
                    }
                    break;
                }
                case IRInstID::FpToSi: {
                    auto float_op = dynamic_cast<ConstFloat*>(inst->get_op(0));
                    if(float_op) {
                        inst->replace_use_with(new ConstInt(m->int32_type, float_op->val));
                        inst_to_delete.insert(inst);
                    }
                    break;
                }
                case IRInstID::SiToFp: {
                    auto int_op = dynamic_cast<ConstInt*>(inst->get_op(0));
                    if(int_op) {
                        inst->replace_use_with(new ConstFloat(m->float_type, int_op->val));
                        inst_to_delete.insert(inst);
                    }
                    break;
                }
                case IRInstID::ZExt: {
                    auto int_op = dynamic_cast<ConstInt*>(inst->get_op(0));
                    if(int_op) {
                        inst->replace_use_with(new ConstInt(m->int32_type, int_op->val));
                        inst_to_delete.insert(inst);
                    }
                    break;
                }
                // TODO: 全局变量的常量传播
                default: {
                    break;
                }
            }
        }
    }
    if(!inst_to_delete.empty()) {
        for(auto inst: inst_to_delete) {
            inst->parent->delete_inst(inst);
        }
        return true;
    }
    return false;
}

void ConstantAnalysis::link_cond_branch(BasicBlock* bb, BasicBlock* target_bb) {
    for(auto succ_bb: bb->succ_bbs) {
        succ_bb->remove_pre_basic_block(bb); // 移除当前基本块作为后继基本块的前驱
        //对于不是 false_bb 目标且只有一个前驱的后继块，是不可能到达的
        if(succ_bb != target_bb) {
            succ_bb->remove_pre_basic_block(bb);
            remove_phi_in_succ(bb, succ_bb);
        }
    }
    bb->succ_bbs.clear();
    new BranchInst(dynamic_cast<BasicBlock*>(target_bb), bb);
}

bool ConstantAnalysis::sparse_cond_const_prop(Function* func) {
    bool changed = false;
    for(auto bb: func->bb_list) {   
        //分支语句只可能出现在基本块最后
        auto br = bb->get_terminator();
        if(!br) {
            continue;
        }
        // 条件分支语句
        if(br->iid == IRInstID::Br && dynamic_cast<BranchInst*>(br)->operands.size() == 3) {
            // 判断 condition 是不是常数
            auto cond = dynamic_cast<ConstInt*>(br->get_op(0));
            if(cond == nullptr) {
                continue;
            }
            auto true_bb = br->get_op(1);
            auto false_bb = br->get_op(2);
            changed = true;
            bb->delete_inst(br); // 删除当前基本块的跳转指令
            if(cond->val == 0) { // 分支的处理，对于总是false的分支
                link_cond_branch(bb, dynamic_cast<BasicBlock*>(false_bb));
            } else { // always true
                link_cond_branch(bb, dynamic_cast<BasicBlock*>(true_bb));
            }
        }
    }
    delete_unreachable_blocks(func); // 删除上述步骤带来的不可达基本块
    return changed;
}

void ConstantAnalysis::execute() {
    for(auto func: m->func_list) {
        if(func->bb_list.empty())
            continue;
        bool change = true;
        while(change) {
            change = false;
            change |= const_fold_prop(func); // 常量折叠和常量传播
            change |= sparse_cond_const_prop(func); // 稀疏条件常量传播
        }
    }
}