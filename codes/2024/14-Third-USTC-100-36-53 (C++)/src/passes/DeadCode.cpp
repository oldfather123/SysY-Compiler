#include "DeadCode.hpp"
#include "Instruction.hpp"
#include "logging.hpp"

// 处理流程：两趟处理，mark 标记有用变量，sweep 删除无用指令
void DeadCode::run(Module *m, AnalysisPassManager &APM) {
    bool changed{};
    // func_info->run();
    auto func_info = APM.getResult<FuncInfo>(m);
    do {
        changed = false;
        for (auto &F : m->get_functions()) {
            auto func = &F;
            changed |= clear_basic_blocks(func);
            changed |= clear_dead_store(func);
            mark(func, func_info);
            changed |= sweep(func);
        }
    } while (changed);
    LOG_INFO << "dead code pass erased " << ins_count << " instructions";
    APM.invalidateAll<Module>(m);
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
    }
    return changed;
}

void DeadCode::mark(Function *func, const FuncInfo *func_info) {
    work_list.clear();
    marked.clear();

    for (auto &bb : func->get_basic_blocks()) {
        for (auto &ins : bb.get_instructions()) {
            if (is_critical(&ins, func_info)) {
                marked[&ins] = true;
                work_list.push_back(&ins);
            }
        }
    }

    while (work_list.empty() == false) {
        auto now = work_list.front();
        work_list.pop_front();

        mark(now);
    }
}

void DeadCode::mark(Instruction *ins) {
    for (auto op : ins->get_operands()) {
        auto def = dynamic_cast<Instruction *>(op);
        if (def == nullptr)
            continue;
        if (marked[def])
            continue;
        if (def->get_function() != ins->get_function())
            continue;
        marked[def] = true;
        work_list.push_back(def);
    }
}

bool DeadCode::sweep(Function *func) {
    std::unordered_set<Instruction *> wait_del{};
    for (auto &bb : func->get_basic_blocks()) {
        for (auto it = bb.get_instructions().begin();
             it != bb.get_instructions().end();) {
            if (marked[&*it]) {
                ++it;
                continue;
            } else {
                auto tmp = &*it;
                wait_del.insert(tmp);
                it++;
            }
        }
    }
    for (auto inst : wait_del)
        inst->remove_all_operands();
    for (auto inst : wait_del)
        inst->get_parent()->get_instructions().erase(inst);
    ins_count += wait_del.size();
    return not wait_del.empty(); // changed
}

bool DeadCode::is_critical(Instruction *ins, const FuncInfo *func_info) {
    // 对纯函数的无用调用也可以在删除之列
    if (ins->is_call()) {
        auto call_inst = dynamic_cast<CallInst *>(ins);
        auto callee = dynamic_cast<Function *>(call_inst->get_operand(0));
        if (func_info->is_pure_function(callee))
            return false;
        return true;
    }
    if (ins->is_br() || ins->is_ret())
        return true;
    if (ins->is_store())
        return true;
    return false;
}

bool DeadCode::clear_dead_store(Function *func) {
    // maybe it can be implemented more efficiently
    std::unordered_map<Value *, Value *> cached;
    auto find_base_addr = [&](Value *addr) {
        if (auto it = cached.find(addr); it != cached.end())
            return it->second;
        Value *p = addr;
        while (auto *gep = dynamic_cast<GetElementPtrInst *>(p))
            p = gep->get_operand(0);
        return p;
    };

    std::unordered_set<Instruction *> readed_alloca;
    auto read = [&](Value *addr) {
        auto base = find_base_addr(addr);
        if (auto alloca = dynamic_cast<AllocaInst *>(base))
            readed_alloca.insert(alloca);
    };


    for (auto &bb : func->get_basic_blocks()) {
        for (auto &ins : bb.get_instructions()) {
            if (ins.is_load()) {
                read(ins.get_operand(0));
            } else if (ins.is_call()) {
                // TODO: interprocedure?
                for (size_t i = 1; i < ins.get_num_operand(); ++i)
                    read(ins.get_operand(i));
            }
        }
    }

    std::vector<Instruction *> wait_del;
    for (auto &bb : func->get_basic_blocks()) {
        for (auto &ins : bb.get_instructions()) {
            if (!ins.is_store())
                continue;
            auto base = find_base_addr(ins.get_operand(1));
            if (auto alloca = dynamic_cast<AllocaInst *>(base)) {
                if (readed_alloca.find(alloca) == readed_alloca.end())
                    wait_del.push_back(&ins);
            }
        }
    }

    bool changed = !wait_del.empty();

    for (auto inst : wait_del)
        inst->get_parent()->get_instructions().erase(inst);

    return changed;
}

