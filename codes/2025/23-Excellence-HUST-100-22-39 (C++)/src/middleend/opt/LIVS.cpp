#include <queue>
#include <unordered_set>
#include <algorithm>
#include "LIVS.hpp"

bool LIVS::indu_vars_strength_reduction(LoopPtr loop) {
    bool changed = false;
    for(auto& [biv, divs]: loop->biv_to_divs) {
        vector<IVExp*> end_exp; // 归纳变量计算式的路径终点
        for(auto div: divs) {
            auto phi_inst = dynamic_cast<PhiInst*>(div->base);
            if(phi_inst) {
                if(div->inst == phi_inst) { // 起点
                    continue;
                } else if(div->inst == phi_inst->get_op(0) || div->inst == phi_inst->get_op(2)) {
                    continue;
                } else if(div->pre_exp->inst == div->base && (div->inst->is_add() || div->inst->is_sub()) && div->suc_exp.empty()) { // 单纯加减法，无需进行强度削弱
                    continue;
                }
                if(div->suc_exp.empty()) { // 没有后继计算表达式
                    end_exp.push_back(div); // 则为归纳变量计算式的路径终点
                }
            } else {
                cerr << "[ERROR] LIVS::indu_vars_strength_reduction: unexpected IVExp base type in loop " << loop->header->name << endl;
                exit(1);
            }
        }
        // 获取所有归纳变量计算式的追溯路径
        vector<vector<IVExp*>> traces;
        for(auto div: end_exp) {
            vector<IVExp*> trace;
            IVExp* cur_exp = div;
            while(cur_exp) {
                trace.push_back(cur_exp);
                cur_exp = cur_exp->pre_exp; // 向前追溯
            }
            traces.push_back(trace);
        }
        for(auto trace: traces) {
            vector<Instruction*> inst_to_delete; // 需要删除的指令
            for(auto exp: trace) {
                if(exp->suc_exp.empty() && exp->inst != exp->base) {
                    inst_to_delete.push_back(exp->inst); // 记录需要删除的指令
                    loop->biv_to_divs[exp->base].erase(std::remove(loop->biv_to_divs[exp->base].begin(), loop->biv_to_divs[exp->base].end(), exp), loop->biv_to_divs[exp->base].end());
                    if(exp->pre_exp) {
                        // 从前置计算式的后继计算式列表中删除当前计算式
                        exp->pre_exp->suc_exp.erase(std::remove(exp->pre_exp->suc_exp.begin(), exp->pre_exp->suc_exp.end(), exp), exp->pre_exp->suc_exp.end());
                    }
                    changed = true; // 标记发生了变化
                }
            }
            Value* i_init = nullptr;
            if(auto phi_inst = dynamic_cast<PhiInst*>(trace.back()->base)) {
                auto val1 = phi_inst->get_op(0); // 获取初始值
                auto valbb1 = dynamic_cast<BasicBlock*>(phi_inst->get_op(1));
                auto val2 = phi_inst->get_op(2);
                auto valbb2 = dynamic_cast<BasicBlock*>(phi_inst->get_op(3));
                if(valbb1 == loop->pre_header && val1) {
                    i_init = val1;
                } else if(valbb2 == loop->pre_header && val2) {
                    i_init = val2;
                } else {
                    cerr << "[ERROR] LIVS::indu_vars_strength_reduction: unexpected PhiInst operands in loop " << loop->header->name << endl;
                    exit(1);
                }
            } else {
                cerr << "[ERROR] LIVS::indu_vars_strength_reduction: unexpected IVExp base type in loop " << loop->header->name << endl;
                exit(1);
            }
            int new_k = trace.front()->k, new_b = trace.front()->b; // 新的 k 和 b 值
            int step = trace.back()->k; // 基本归纳变量的更新步长
            Instruction* inst = trace.front()->inst; // 归纳变量计算式对应的最后一条指令
            BasicBlock* block = inst->parent;
            BinaryInst* new_change_exp = new BinaryInst(m->int32_type, IRInstID::Add, new ValUndef(m->int32_type), new ConstInt(m->int32_type, new_k * step), block, InsertPos::None);
            new_change_exp->name = std::to_string(new_change_exp->parent->parent->ssa_id++);
            block->add_inst_before_terminator(new_change_exp);
            BinaryInst* init_mul = new BinaryInst(m->int32_type, IRInstID::Mul, i_init, new ConstInt(m->int32_type, new_k), loop->pre_header, InsertPos::None);
            init_mul->name = std::to_string(init_mul->parent->parent->ssa_id++);
            BinaryInst* init_add = new BinaryInst(m->int32_type, IRInstID::Add, init_mul, new ConstInt(m->int32_type, new_b), loop->pre_header, InsertPos::None);
            init_add->name = std::to_string(init_add->parent->parent->ssa_id++);
            // Phi 指令的操作数，源值与源块对应
            vector<Value*> vals = {init_add, new_change_exp};
            vector<BasicBlock*> valbbs = {loop->pre_header, loop->latch};
            PhiInst* new_phi_inst = new PhiInst(m->int32_type, vals, valbbs, loop->header, InsertPos::None);
            new_phi_inst->name = std::to_string(new_phi_inst->parent->parent->ssa_id++);
            new_change_exp->set_op(0, new_phi_inst); // 设置新的归纳变量计算式的操作数为新的 Phi 指令
            inst->replace_use_with(new_phi_inst);
            loop->header->add_inst_front(new_phi_inst); // 在 header 添加新的 Phi 指令
            loop->pre_header->add_inst_before_terminator(init_mul); // 在 pre-header 添加初始值计算式
            loop->pre_header->add_inst_before_terminator(init_add);
            for(auto inst: inst_to_delete) {
                block->delete_inst(inst); // 删除归纳变量计算式对应的指令
            }
            divs.push_back(new IVExp(new_change_exp, new_phi_inst, new_k, new_b, divs[0])); // 手动添加新建的归纳变量变化表达式
        }
    }
    return changed;
}

bool LIVS::basic_indu_vars_elimination(LoopPtr loop) {
    bool changed = false;
    vector<Value*> biv_to_delete; // 需要删除的基本归纳变量
    for(auto& [biv, divs] : loop->biv_to_divs) {
        if(divs.size() < 2) {
            continue; // 如果没有同族归纳变量，不能消除
        }
        // 遍历使用 biv 的 Phi 指令的指令，如果只有变化表达式和循环条件判断用到，那么可以消除，同时更改条件判断的值即可
        auto phi_inst = dynamic_cast<PhiInst*>(biv);
        if(!phi_inst) {
            continue;
        }
        if(phi_inst->use_list.size() > 2) { // TODO: 暂时只消除被循环条件判断使用一次加上变化表达式用到一次的基本归纳变量，如果使用次数大于 2，说明有其他用途，不能消除
            continue;
        }
        for(auto user: phi_inst->use_list) {
            auto user_inst = user.val;
            if(user_inst == phi_inst->get_op(0) || user_inst == phi_inst->get_op(2)) { // TODO: 变化表达式的判断可能略显简单
                changed = true;
            } else if(auto cmp_inst = dynamic_cast<ICmpInst*>(user_inst)) { // 条件判断
                if(biv == cmp_inst->get_op(0) || biv == cmp_inst->get_op(1)) {
                    changed = true;
                } else {
                    changed = false;
                    break; // 如果有其他用途，不能消除
                }
            } else {
                changed = false; // 如果有其他用途，不能消除
                break;
            }
        }
        if(changed) {
            IVExp* target_div = nullptr; // 替换基本归纳变量的归纳变量表达式
            // 选择替换基本归纳变量的归纳变量
            target_div = divs[1]; // TODO: 暂时选择添加的第一个基本归纳变量作为替换的值
            // 如果没有找到合适的归纳变量表达式，跳过
            if(!target_div) {
                continue;
            }
            // 替换基本归纳变量的循环判断条件
            vector<Instruction*> inst_to_delete; // 需要删除的指令
            for(auto user : phi_inst->use_list) {
                auto user_inst = user.val;
                if(auto cmp_inst = dynamic_cast<ICmpInst*>(user_inst)) {
                    // 替换 cmp 的 biv 操作数，并同步修改比较的值
                    if(cmp_inst->get_op(0) == biv) {
                        cmp_inst->set_op(0, target_div->base);
                        // 如果是 biv 的左操作数，修改比较值
                        auto val = cmp_inst->get_op(1);
                        BinaryInst* mul_val = new BinaryInst(m->int32_type, IRInstID::Mul, val, new ConstInt(m->int32_type, target_div->k), loop->pre_header, InsertPos::None);
                        mul_val->name = std::to_string(mul_val->parent->parent->ssa_id++);
                        BinaryInst* add_val = new BinaryInst(m->int32_type, IRInstID::Add, mul_val, new ConstInt(m->int32_type, target_div->b), loop->pre_header, InsertPos::None);
                        add_val->name = std::to_string(add_val->parent->parent->ssa_id++);
                        cmp_inst->set_op(1, add_val);
                        loop->pre_header->add_inst_before_terminator(mul_val); // 在 pre-header 添加新的归纳变量计算式
                        loop->pre_header->add_inst_before_terminator(add_val);
                    } else if(cmp_inst->get_op(1) == biv) {
                        cmp_inst->set_op(1, target_div->base);
                        // 如果是 biv 的右操作数，修改比较值
                        auto val = cmp_inst->get_op(0);
                        BinaryInst* mul_val = new BinaryInst(m->int32_type, IRInstID::Mul, val, new ConstInt(m->int32_type, target_div->k), loop->pre_header, InsertPos::None);
                        mul_val->name = std::to_string(mul_val->parent->parent->ssa_id++);
                        BinaryInst* add_val = new BinaryInst(m->int32_type, IRInstID::Add, mul_val, new ConstInt(m->int32_type, target_div->b), loop->pre_header, InsertPos::None);
                        add_val->name = std::to_string(add_val->parent->parent->ssa_id++);
                        cmp_inst->set_op(0, add_val);
                        loop->pre_header->add_inst_before_terminator(mul_val); // 在 pre-header 添加新的归纳变量计算式
                        loop->pre_header->add_inst_before_terminator(add_val);
                    }
                } else if(auto other_inst = dynamic_cast<Instruction*>(user_inst)) {
                    inst_to_delete.push_back(other_inst); // 其他指令直接删除
                }
            }
            // 删除归纳变量计算式对应的指令
            for(auto inst: inst_to_delete) {
                inst->parent->delete_inst(inst);
            }
            phi_inst->parent->delete_inst(phi_inst); // 删除基本归纳变量的 Phi 指令
            biv_to_delete.push_back(biv); // 记录需要删除的基本归纳变量
        }
    }
    // 删除所有需要删除的基本归纳变量
    for(auto biv: biv_to_delete) {
        loop->biv_to_divs.erase(biv); // 从归纳变量到归纳变量表达式的映射中删除基本归纳变量
    }
    return changed;
}

void LIVS::execute() {
    for(auto& [func, loops]: loops) {
        for(auto loop: *loops) {
            bool changed;
            do {
                changed = false;
                changed |= indu_vars_strength_reduction(loop); // 归纳变量强度削减
                changed |= basic_indu_vars_elimination(loop); // 基本归纳变量消除
            } while(changed);
        }
    }
}