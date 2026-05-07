#include "funusedef.hpp"
#include "Instclone.hpp"
#include "User.hpp"
#include "Instruction.hpp"
#include "Value.hpp"
#include <algorithm>
#include "IRBuilder.hpp"
FunctionInlineFindPass::FunctionInlineFindPass(Module *m) : Pass(m), m_(m) {}

void FunctionInlineFindPass::run()
{
    // find_replace_calls();
    // replace_call();
    round_replace();
    remove_useless_func();
    // std::cout << "Functions and their calls:" << std::endl;
    // for (const auto &entry : function_calls)
    // {
    //     const auto &func_name = entry.first->get_name();
    //     const auto &calls = entry.second;

    //     std::cout << "   " << "Function: " << func_name << std::endl;
    //     std::cout << "   " << "Calls:" << std::endl;
    //     for (auto *call_instr : calls)
    //     {
    //         auto *parent_fun = call_instr->get_parent()->get_parent();
    //         // 输出所在的 BasicBlock 信息
    //         std::cout << "   " << "   " << call_instr->get_operand(0)->get_name() << ":" << parent_fun->get_name() << std::endl;
    //     }
    // }
}

void FunctionInlineFindPass::find_replace_calls()
{
    for (auto &func : m_->get_functions())
    {
        line[&func] = 0;
        // 遍历每个函数的基本块

        for (auto &bb : func.get_basic_blocks())
        {
            // std::cout << "   " << "Block: " << bb.get_name() << std::endl;s
            // 遍历每个基本块的指令
            for (auto &instr : bb.get_instructions())
            {
                line[&func] += 1;
                if (auto *call_instr = dynamic_cast<CallInst *>(&instr))
                {
                    // 记录函数调用的指令和目标函数名称
                    function_calls[call_instr->get_parent()->get_parent()].push_back(call_instr);
                }
            }
        }
    }

    // 打印所有找到的函数调用
    // std::cout << "Functions and their calls:" << std::endl;
    // for (const auto &entry : function_calls)
    // {
    //     const auto &func_name = entry.first->get_name();
    //     const auto &calls = entry.second;

    //     std::cout << "   " << "Function: " << func_name << std::endl;
    //     std::cout << "   " << "Calls:" << std::endl;
    //     for (auto *call_instr : calls)
    //     {
    //         auto *parent_fun = call_instr->get_parent()->get_parent();
    //         // 输出所在的 BasicBlock 信息
    //         std::cout << "   " << "   " << call_instr->get_operand(0)->get_name() << ":" << parent_fun->get_name() << std::endl;
    //     }
    // }
    for (auto &entry : function_calls)
    {
        auto &calls = entry.second;
        calls.erase(
            std::remove_if(
                calls.begin(), calls.end(),
                [&](CallInst *call_instr)
                {
                    auto *callee = dynamic_cast<Function *>(call_instr->get_operand(0));
                    return callee && line[callee] == 0;
                }),
            calls.end());
    }
    // std::cout << "line:" << std::endl;
    for (auto &func : m_->get_functions())
    {
        if (line[&func] == 0)
            continue;
        // if (func.get_num_basic_blocks() > 1)
        // {
        //     continue;
        // }
        // std::cout << "   " << func.get_name() << ":" << line[&func] << std::endl;
        if (function_calls[&func].size() == 0)
        {
            is_simple_fun[&func] = true;
        }
    }

    // std::cout << "Functions and their calls:" << std::endl;
    for (const auto &entry : function_calls)
    {
        const auto &func_name = entry.first->get_name();
        const auto &calls = entry.second;

        // std::cout << "   " << "Function: " << func_name << std::endl;
        // std::cout << "   " << "Calls:" << std::endl;
        for (auto *call_instr : calls)
        {
            auto *parent_fun = call_instr->get_parent()->get_parent();
            // 输出所在的 BasicBlock 信息
            // std::cout << "   " << "   " << call_instr->get_operand(0)->get_name() << ":" << parent_fun->get_name() << std::endl;
        }
    }

    // std::cout << "simple_fun" << std::endl;
    // for (auto &func : m_->get_functions())
    // {
    //     if (line[&func] == 0)
    //         continue;
    //     std::cout << "   " << func.get_name() << ":" << is_simple_fun[&func] << std::endl;
    // }
    for (const auto &entry : function_calls)
    {
        const auto &calls = entry.second;
        for (auto *call_instr : calls)
        {
            func_use[static_cast<Function *>(call_instr->get_operand(0))].push_back(call_instr);
        }
    }
    // std::cout << "fun and its use " << std::endl;
    // for (const auto &entry : func_use)
    // {
    //     const auto &func_name = entry.first->get_name();
    //     const auto &calls = entry.second;
    //     std::cout << "   " << "Function: " << func_name << std::endl;
    //     std::cout << "   " << "Calls:" << std::endl;
    //     for (auto *call_instr : calls)
    //     {
    //         auto *parent_fun = call_instr->get_parent()->get_parent();
    //         // 输出所在的 BasicBlock 信息
    //         std::cout << "       " << call_instr->get_operand(0)->get_name() << ":" << parent_fun->get_name() << std::endl;
    //     }
    // }
    for (const auto &entry : func_use)
    {
        const auto &func = entry.first;
        const auto &calls = entry.second;
        if (is_simple_fun[func] == true)
        {
            for (auto *call_instr : calls)
            {
                if(func->get_name()=="pseudo_sha1"){
                    continue;
                }
                replace_calls.push_back(call_instr);
            }
            is_simple_fun[func] == false;
        }
    }
  //  std::cout << "replace call" << std::endl;
    for (auto *call_instr : replace_calls)
    {
        auto *parent_fun = call_instr->get_parent()->get_parent();
       // std::cout << "   " << call_instr->get_operand(0)->get_name() << ":" << parent_fun->get_name() << std::endl;
    }
}

void FunctionInlineFindPass::replace_call()
{

    for (auto *call_instr : replace_calls)
    {
        auto *func = dynamic_cast<Function *>(call_instr->get_operand(0));
        auto return_type = func->get_function_type()->get_return_type();
        Value *return_value = nullptr;
        int i = 1;
        inst_map.clear();
        for (auto &arg : func->get_args())
        {
            inst_map[&arg] = call_instr->get_operand(i);
            i++;
        }
        // 简单情况直接插
        if (func->get_basic_blocks().size() == 1)
        {
            BasicBlock *newblock = BasicBlock::create(m_, "", call_instr->get_parent()->get_parent());
            for (auto &block : func->get_basic_blocks())
            {

                for (auto &inst : block.get_instructions())
                {
                    if (inst.get_instr_type() == Instruction::ret)
                    {
                        if (return_type->is_void_type())
                            continue;
                        return_value = inst.get_operand(0);
                        auto it = inst_map.find(return_value);
                        if (it != inst_map.end())
                        {
                            return_value = it->second;
                        }
                        continue;
                    }
                    Instruction *cloneinst = clone_inst(&inst, newblock, inst_map);
                    inst_map[&inst] = cloneinst;
                }
            }
            auto callblock = call_instr->get_parent();
            std::vector<Instruction *> to_move;
            for (auto &inst : newblock->get_instructions())
            {
                to_move.push_back(&inst);
            }
            move_instrs(callblock, newblock, to_move, call_instr);
            if (return_value != nullptr)
            {
                call_instr->replace_all_use_with(return_value);
            }
            call_instr->get_parent()->remove_instr(call_instr);
            delete call_instr;
            newblock->erase_from_parent();
            // std::cout << "func_use" << std::endl;
            // for (auto use : func->get_use_list())
            // {
            //     User *user = use.val_;
            //     std::cout << static_cast<Instruction *>(user) << "  ";
            // }
            // std::cout << std::endl;
        }
        else
        {
            auto callblock = call_instr->get_parent();
            // 多块指令需要创建前驱->bb1->- entry - ... - end(ret->phi)->origin->后继 auto callblock = call_instr->get_parent();
            auto frontbb = insert_before_block(callblock, m_);
            // 分开原来块
            std::vector<Instruction *> to_move;
            to_move.clear();
            for (auto &inst : callblock->get_instructions())
            {
                if (&inst == call_instr)
                    break;
                to_move.push_back(&inst);
            }
            move_instrs(frontbb, callblock, to_move, frontbb->get_terminator());
            to_move.clear();
            // 插入函数
            auto entry_ret = clone_func(m_, func, call_instr->get_parent()->get_parent(), inst_map);
            auto [entry, ret, retvalue] = entry_ret;

            // 拆开
            frontbb->remove_succ_basic_block(callblock);
            callblock->remove_pre_basic_block(frontbb);
            frontbb->remove_instr(frontbb->get_terminator());
            auto builder = IRBuilder(nullptr, m_);
            builder.set_insert_point(frontbb);
            builder.create_br(entry);
            builder.set_insert_point(ret);
        //    std::cout<<ret->print()<<std::endl;
            builder.create_br(callblock);
            if (retvalue != nullptr)
            {
                call_instr->replace_all_use_with(retvalue);
            }
            call_instr->get_parent()->remove_instr(call_instr);
            // std::cout<<ret->print()<<std::endl;
            delete call_instr;
        }
    }
}

void FunctionInlineFindPass::remove_useless_func()
{
    for (auto &func : m_->get_functions())
    {
        if (func.get_name() == "main")
        {
            continue;
        }
        if (func.get_use_list().size() == 0)
        {
            m_->get_functions().remove(&func);
            delete &func;
        }
    }
}

void FunctionInlineFindPass::round_replace()
{
    bool changed = true;
    while (changed)
    {
        changed = false;

        // 清空之前的调用信息
        function_calls.clear();
        func_use.clear();
        replace_calls.clear();
        is_simple_fun.clear();
        line.clear();
        inst_map.clear();
        bb_map.clear();
        args.clear();
        find_replace_calls();
        if (!replace_calls.empty())
        {
            changed = true;
            replace_call();
            remove_useless_func();
        }
    }
    function_calls.clear();
    func_use.clear();
    replace_calls.clear();
    is_simple_fun.clear();
    line.clear();
    inst_map.clear();
    bb_map.clear();
    args.clear();
}