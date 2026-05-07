#include "DeadCode.hpp"
#include "logging.hpp"
#include "Instclone.hpp"

// 处理流程：两趟处理，mark 标记有用变量，sweep 删除无用指令
void DeadCode::run()
{
    bool changed{};
    func_info->run();
    do
    {
        changed = false;
        for (auto &F : m_->get_functions())
        {
            auto func = &F;
            mark(func);
            // optimize_phi();
            changed |= sweep(func);
        }
    } while (changed);
    // eliminate_pseudo_used_allocas(m_);
    LOG_INFO << "dead code pass erased " << ins_count << " instructions";
    work_list.clear();
    marked.clear();
}

void DeadCode::mark(Function *func)
{
    work_list.clear();
    marked.clear();

    for (auto &bb : func->get_basic_blocks())
    {
        for (auto &ins : bb.get_instructions())
        {
            if (is_critical(&ins))
            {
                marked[&ins] = true;
                work_list.push_back(&ins);
            }
        }
    }

    while (work_list.empty() == false)
    {
        auto now = work_list.front();
        work_list.pop_front();

        mark(now);
    }
}

// void DeadCode::mark(Instruction *ins) {
//     for (auto op : ins->get_operands()) {
//         auto def = dynamic_cast<Instruction *>(op);
//         if (def == nullptr)
//             continue;
//         if (marked[def])
//             continue;
//         if (def->get_function() != ins->get_function())
//             continue;
//         marked[def] = true;
//         work_list.push_back(def);
//     }
// }

void DeadCode::mark(Instruction *ins)
{
    work_list.push_back(ins); // 初始指令加入队列
    while (!work_list.empty())
    {
        auto now = work_list.front();
        work_list.pop_front();

        for (auto op : now->get_operands())
        {
            auto def = dynamic_cast<Instruction *>(op);
            if (def == nullptr || marked[def] || def->get_function() != now->get_function())
                continue;
            marked[def] = true;
            work_list.push_back(def);
        }
    }
}

void DeadCode::eliminate_pseudo_used_allocas(Module *m)
{
    std::vector<AllocaInst *> to_delete_allocas;
    std::vector<StoreInst *> to_delete_stores;

    for (auto &func : m->get_functions())
    {
        for (auto &bb : func.get_basic_blocks())
        {
            for (auto &inst : bb.get_instructions())
            {
                auto *alloca = dynamic_cast<AllocaInst *>(&inst);
                if (!alloca)
                    continue;

                bool is_pseudo_used = true;
                std::vector<StoreInst *> store_users;

                for (const auto &use : alloca->get_use_list())
                {
                    User *user = use.val_;
                    auto *store = dynamic_cast<StoreInst *>(user);
                    if (!store || store->get_rval() != alloca)
                    {
                        is_pseudo_used = false;
                        break;
                    }
                    store_users.push_back(store);
                }

                if (is_pseudo_used)
                {
                    to_delete_allocas.push_back(alloca);
                    to_delete_stores.insert(to_delete_stores.end(),
                                            store_users.begin(), store_users.end());
                }
            }
        }
    }
    for (auto *store : to_delete_stores)
    {
        for (unsigned i = 0; i < store->get_num_operand(); ++i)
        {
            Value *v = store->get_operand(i);
            if (v)
                v->remove_use(store, i);
        }
        store->get_parent()->get_instructions().remove(store);
    }
    for (auto *alloca : to_delete_allocas)
    {
        alloca->get_parent()->get_instructions().remove(alloca);
    }
}

// bool DeadCode::sweep(Function *func) {
//     std::unordered_set<Instruction *> wait_del{};
//     for (auto &bb : func->get_basic_blocks()) {
//         for (auto it = bb.get_instructions().begin();
//              it != bb.get_instructions().end();) {
//             if (marked[&*it]) {
//                 ++it;
//                 continue;
//             } else {
//                 auto tmp = &*it;
//                 wait_del.insert(tmp);
//                 it++;
//             }
//         }
//     }
//     for (auto inst : wait_del)
//         inst->remove_all_operands();
//     for (auto inst : wait_del)
//         inst->get_parent()->get_instructions().erase(inst);
//     ins_count += wait_del.size();
//     return not wait_del.empty(); // changed
// }

bool DeadCode::sweep(Function *func)
{
    bool changed = false;
    std::unordered_set<Instruction *> wait_del;

    for (auto &bb : func->get_basic_blocks())
    {
        for (auto it = bb.get_instructions().begin(); it != bb.get_instructions().end();)
        {
            auto &inst = *it;
            if (marked[&inst])
            {
                ++it;
            }
            else
            {
                wait_del.insert(&inst);
                it = bb.get_instructions().erase(it); // 安全删除指令
            }
        }
    }

    for (auto inst : wait_del)
    {
        inst->remove_all_operands(); // 清除操作数
    }

    ins_count += wait_del.size();
    return !wait_del.empty(); // 如果删除了指令，则返回 true
}

bool DeadCode::is_critical(Instruction *ins)
{
    // 对纯函数的无用调用也可以在删除之列
    if (ins->is_call())
    {
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

void DeadCode::deleteuselesslabel()
{
    for (auto &F : m_->get_functions())
    {
        bool del = false;
        std::vector<BasicBlock *> to_del;
        for (auto &bb : F.get_basic_blocks())
        {
            auto prebbs = bb.get_pre_basic_blocks();
            if (prebbs.size() != 1)
            {
                break;
            }
            auto prebb = prebbs.front();
            auto br = prebb->get_terminator();

            bool used_by_phi = false;
            for (auto user : br->get_use_list())
            {
                auto inst = dynamic_cast<Instruction *>(user.val_);
                if (inst && inst->is_phi())
                {
                    used_by_phi = true;
                    break;
                }
            }
            if (used_by_phi)
            {
                continue;
            }
            std::vector<Instruction *> to_move;
            for (auto &inst : bb.get_instructions())
            {
                // if (inst.isTerminator())
                //     break;
                to_move.push_back(&inst);
            }

            prebb->remove_instr(prebb->get_terminator());
            move_instrs(prebb, &bb, to_move, prebb->get_terminator());

            // 删块
            prebb->remove_succ_basic_block(&bb);
            bb.remove_pre_basic_block(prebb);

            for (auto succ : bb.get_succ_basic_blocks())
            {
                succ->remove_pre_basic_block(&bb);
                succ->add_pre_basic_block(prebb);
                bb.remove_succ_basic_block(succ);
                prebb->add_succ_basic_block(succ);
            }
            del = true;
        }
        F.remove(to_del.front());
    }
}


