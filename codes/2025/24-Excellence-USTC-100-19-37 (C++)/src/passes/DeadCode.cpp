#include "DeadCode.hpp"

#include <vector>

void DeadCode::run() {
    bool changed = false;
    do {
        changed = false;
        for (auto &func : m_->get_functions()) {
            auto func_ptr = &func;
            changed |= clear_basic_blocks(func_ptr);
            mark_critical_func(func_ptr);
            changed |= clear_func_instructions(func_ptr);
        }
    } while (changed);
    clear_global_declarations();
}



void DeadCode::mark_critical_func(Function *func) {
    worklist.clear();
    is_critical_inst.clear();

    for (auto &bb : func->get_basic_blocks()) {
        for (auto &inst : bb.get_instructions()) {
            if (is_critical(&inst)) {
                is_critical_inst[&inst] = true;
                worklist.push_back(&inst);
            }
        }
    }

    while (!worklist.empty()) {
        auto now = worklist.front();
        worklist.pop_front();

        mark_critical_inst(now);
    }
}

void DeadCode::mark_critical_inst(Instruction *ins) {
    for (auto op : ins->get_operands()) {
        auto def = dynamic_cast<Instruction *>(op);
        if (def == nullptr)
            continue;
        if (is_critical_inst[def])
            continue;
        if (def->get_function() != ins->get_function())
            continue;
        is_critical_inst[def] = true;
        worklist.push_back(def);
    }
}

bool DeadCode::is_critical(Instruction *ins) {
    // 对纯函数的无用调用也可以在删除之列
    if (ins->is_call()) {
        auto call_inst = dynamic_cast<CallInst *>(ins);
        auto callee = dynamic_cast<Function *>(call_inst->get_operand(0));
        if (is_pure_func.at(callee)) {
            return false;
        }
        return true;
    }
    if (ins->is_br() || ins->is_ret())
        return true;
    if (ins->is_store())
        return true;
    return false;
}

bool DeadCode::clear_basic_blocks(Function *func) {
    bool changed = 0;
    std::vector<BasicBlock *> to_erase;
    for (auto &bb1 : func->get_basic_blocks()) {
        auto bb = &bb1;
        if(bb->get_pre_basic_blocks().empty() && bb != func->get_entry_block()) {
            to_erase.push_back(bb);
            changed = 1;
        }
    }
    for (auto &bb : to_erase) {
        bb->erase_from_parent();
        delete bb;
    }
    return changed;
}

bool DeadCode::clear_func_instructions(Function *func) {
    std::unordered_set<Instruction *> wait_del{};
    for (auto &bb : func->get_basic_blocks()) {
        for (auto it = bb.get_instructions().begin(); it != bb.get_instructions().end(); it++) {
            auto inst = &*it;
            if (is_critical_inst[inst]) {
                continue;
            } else {
                wait_del.insert(inst);
            }
        }
    }
    for (auto inst : wait_del) {
        inst->remove_all_operands();
    }
    for (auto inst : wait_del) {
        inst->get_parent()->get_instructions().erase(inst);
    }
    return not wait_del.empty(); // changed
}

void DeadCode::clear_global_declarations() {
    std::vector<Function *> unused_funcs;
    std::vector<GlobalVariable *> unused_globals;
    for (auto &f_r : m_->get_functions()) {
        if (f_r.get_use_list().size() == 0 and f_r.get_name() != "main") {
            unused_funcs.push_back(&f_r);
        }
    }
    for (auto &glob_var_r : m_->get_global_variable()) {
        if (glob_var_r.get_use_list().size() == 0) {
            unused_globals.push_back(&glob_var_r);
        }
    }
    for (auto func : unused_funcs) {
        m_->get_functions().erase(func);
    }
    for (auto glob : unused_globals) {
        m_->get_global_variable().erase(glob);
    }
}
