#include "DeadCode.hpp"
#include "logging.hpp"
#include <vector>

// 两趟处理，mark 标记有用变量，sweep 删除无用指令
void DeadCode::run()
{
    bool changed{};
    func_info->run();
    
    // 死代码删除循环
    do
    {
        changed = false;
        for (auto &F : m_->get_functions())
        {
            auto func = &F;
            mark(func);
            changed |= sweep(func);
        }
    } while (changed);
    
    // 全局清理
    sweep_globally();
    //LOG_INFO << "dead code pass erased " << ins_count << " instructions";
}

void DeadCode::mark(Function *func)
{
    work_list.clear();
    marked.clear();

    // 标记所有关键指令
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

    // 迭代标记依赖指令
    while (!work_list.empty())
    {
        auto now = work_list.front();
        work_list.pop_front();
        mark(now);
    }
}

void DeadCode::mark(Instruction *ins)
{
    // 标记操作数定义的指令
    for (auto op : ins->get_operands())
    {
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

bool DeadCode::sweep(Function *func)
{
    std::unordered_set<Instruction *> wait_del{};
    
    // 收集未标记的指令
    for (auto &bb : func->get_basic_blocks())
    {
        for (auto it = bb.get_instructions().begin();
             it != bb.get_instructions().end(); ++it)
        {
            if (!marked[&*it])
            {
                wait_del.insert(&*it);
            }
        }
    }
    
    // 先移除操作数，再删除指令
    for (auto inst : wait_del)
        inst->remove_all_operands();
        
    for (auto inst : wait_del)
    {
        inst->get_parent()->get_instructions().erase(inst);
    }
    
    ins_count += wait_del.size();
    return !wait_del.empty();
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

void DeadCode::sweep_globally()
{
    std::vector<Function *> unused_funcs;
    std::vector<GlobalVariable *> unused_globals;
    
    // 收集未使用的函数（除了 main）
    for (auto &f_r : m_->get_functions())
    {
        if (f_r.get_use_list().size() == 0 && f_r.get_name() != "main")
            unused_funcs.push_back(&f_r);
    }
    
    // 收集未使用的全局变量（保护全局常量）
    for (auto &glob_var_r : m_->get_global_variable())
    {
        if (glob_var_r.get_use_list().size() == 0 && !glob_var_r.is_const())
        {
            unused_globals.push_back(&glob_var_r);
        }
    }
    
    // 删除未使用的函数和全局变量
    for (auto func : unused_funcs)
        m_->get_functions().erase(func);
    for (auto glob : unused_globals)
        m_->get_global_variable().erase(glob);
    
    // 清理不可达基本块
    std::vector<BasicBlock *> unused_bbs;
    std::set<Instruction *> unused_phi_instrs;
    
    for (auto &func : m_->get_functions())
    {
        // 收集空基本块和不可达基本块
        for (auto &bb : func.get_basic_blocks())
        {
            if (bb.get_instructions().size() == 0)
            {
                unused_bbs.push_back(&bb);
            }
            else if (bb.get_pre_basic_blocks().size() == 0 && 
                     bb.get_name() != "label_entry")
            {
                unused_bbs.push_back(&bb);
            }
        }
    }
    
    // 处理要删除的基本块
    for (auto bb : unused_bbs)
    {
        auto func = bb->get_parent();
        
        for (auto &bb_ : func->get_basic_blocks())
        {
            for (auto &instr : bb_.get_instructions())
            {
                if (instr.is_phi())
                {
                    // 删除来自要删除基本块的 phi 操作数
                    for (unsigned i = 0; i < instr.get_num_operand(); i++)
                    {
                        if (instr.get_operand(i) == bb)
                        {
                            instr.remove_operand(i);
                            instr.remove_operand(i - 1);
                        }
                    }
                    
                    // 简化 phi 指令
                    if (instr.get_num_operand() == 2)
                    {
                        //若只剩下一个输出，替代
                        auto val = instr.get_operand(0);
                        instr.replace_all_use_with(val);
                        unused_phi_instrs.insert(&instr);
                    }
                    else if (instr.get_num_operand() == 0)
                    {
                        // 没有输入了，删除 phi
                        unused_phi_instrs.insert(&instr);
                    }
                }
            }
        }
        
        // 从函数中删除基本块
        bb->get_parent()->remove(bb);
    }
    
    // 删除无用的 phi 指令
    for (auto instr : unused_phi_instrs)
    {
        instr->get_parent()->remove_instr(instr);
    }
}