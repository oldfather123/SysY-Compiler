#include "ConstPropagation.hpp"

#include "Instruction.hpp"
#include "logging.hpp"
#include <algorithm>

ConstantInt *ConstFolder::compute(Instruction::OpID op, ConstantInt *value1, ConstantInt *value2)
{
    int c_value1 = value1->get_value();
    int c_value2 = value2->get_value();

    switch (op)
    {
    case Instruction::add:
        return ConstantInt::get(c_value1 + c_value2, module_);
        break;
    case Instruction::sub:
        return ConstantInt::get(c_value1 - c_value2, module_);
        break;
    case Instruction::mul:
        return ConstantInt::get(c_value1 * c_value2, module_);
        break;
    case Instruction::sdiv:
        return ConstantInt::get(static_cast<int>(c_value1 / c_value2), module_);
        break;
    case Instruction::eq:
        return ConstantInt::get(c_value1 == c_value2, module_);
        break;
    case Instruction::ne:
        return ConstantInt::get(c_value1 != c_value2, module_);
        break;
    case Instruction::gt:
        return ConstantInt::get(c_value1 > c_value2, module_);
        break;
    case Instruction::ge:
        return ConstantInt::get(c_value1 >= c_value2, module_);
        break;
    case Instruction::lt:
        return ConstantInt::get(c_value1 < c_value2, module_);
        break;
    case Instruction::le:
        return ConstantInt::get(c_value1 <= c_value2, module_);
        break;
    case Instruction::srem:
        return ConstantInt::get(c_value1 % c_value2, module_);
    default:
        return nullptr;
        break;
    }
}

Constant *ConstFolder::compute(Instruction::OpID op, ConstantFP *value1, ConstantFP *value2)
{
    float c_value1 = value1->get_value();
    float c_value2 = value2->get_value();

    switch (op)
    {
    case Instruction::fadd:
        return ConstantFP::get(c_value1 + c_value2, module_);
    case Instruction::fsub:
        return ConstantFP::get(c_value1 - c_value2, module_);
    case Instruction::fmul:
        return ConstantFP::get(c_value1 * c_value2, module_);
    case Instruction::fdiv:
        return ConstantFP::get(c_value1 / c_value2, module_);
    case Instruction::feq:
        return ConstantInt::get((c_value1 - c_value2)<1e-4, module_);
    case Instruction::fne:
        return ConstantInt::get(c_value1 != c_value2, module_);
    case Instruction::fgt:
        return ConstantInt::get(c_value1 > c_value2, module_);
    case Instruction::fge:
        return ConstantInt::get(c_value1 >= c_value2, module_);
    case Instruction::flt:
        return ConstantInt::get(c_value1 < c_value2, module_);
    case Instruction::fle:
        return ConstantInt::get(c_value1 <= c_value2, module_);
    default:
        return nullptr;
    }
}

ConstantFP *ConstFolder::compute(Instruction::OpID op, ConstantInt *value1)
{
    int c_value1 = value1->get_value();

    switch (op)
    {
    case Instruction::sitofp:
        return ConstantFP::get((float)c_value1, module_);
        break;

    default:
        return nullptr;
        break;
    }
}

ConstantInt *ConstFolder::compute(Instruction::OpID op, ConstantFP *value1)
{
    float c_value1 = value1->get_value();
    switch (op)
    {
    case Instruction::fptosi:
        return ConstantInt::get(static_cast<int>(c_value1), module_);
        break;

    default:
        return nullptr;
        break;
    }
}

// 处理 zext 指令的常量折叠
ConstantInt *ConstFolder::compute_zext(Value *value)
{
    // 判断是否为布尔常量类型
    if (auto constBool = dynamic_cast<ConstantInt *>(value))
    {
        // 这里假设i1类型的布尔常量，值为0或1
        int val = constBool->get_value();
        // 直接返回同值的i32常量
        return ConstantInt::get(val, module_);
    }
    return nullptr;
}

ConstantFP *cast_constantfp(Value *value)
{
    auto constant_fp_ptr = dynamic_cast<ConstantFP *>(value);
    if (constant_fp_ptr)
    {
        return constant_fp_ptr;
    }
    return nullptr;
}
ConstantInt *cast_constantint(Value *value)
{
    auto constant_int_ptr = dynamic_cast<ConstantInt *>(value);
    if (constant_int_ptr)
    {
        return constant_int_ptr;
    }
    return nullptr;
}



void ConstPropagation::run()
{
    
    
    for (size_t i = 0; i < 3; i++)
    {
        /* code */

        for (auto &func : m_->get_functions())
        {

            for (auto &bb : func.get_basic_blocks())
            {
                // printf("%s\n",bb.get_name().c_str());
                // printf("%d\n",bb.get_instructions().size());
                wait_delete.clear();

                for (auto &instr : bb.get_instructions())
                {
                    // clear glbalvar_def map

                    if (instr.is_add() || instr.is_sub() || instr.is_mul() || instr.is_div())
                    {
                        auto value1 = cast_constantint(instr.get_operand(0));
                        auto value2 = cast_constantint(instr.get_operand(1));
                        if (value1 && value2)
                        {
                            auto fold_const = folder->compute(instr.get_instr_type(), value1, value2);

                            instr.replace_all_use_with_phi(fold_const);
                            wait_delete.push_back(&instr);
                        }
                    }
                    if (instr.is_srem())
                    {
                        auto value1 = cast_constantint(instr.get_operand(0));
                        auto value2 = cast_constantint(instr.get_operand(1));
                        if (value1 && value2)
                        {
                            auto fold_const = folder->compute(instr.get_instr_type(), value1, value2);
                            instr.replace_all_use_with_phi(fold_const);
                            wait_delete.push_back(&instr);
                        }
                    }
                    if (instr.is_cmp())
                    {
                        auto value1 = cast_constantint(instr.get_operand(0));
                        auto value2 = cast_constantint(instr.get_operand(1));
                        if (value1 && value2)
                        {
                            auto fold_const = folder->compute(instr.get_instr_type(), value1, value2);
                            instr.replace_all_use_with_phi(fold_const);
                            wait_delete.push_back(&instr);
                        }
                    }

                    if (instr.is_fcmp())
                    {
                        auto value1 = cast_constantfp(instr.get_operand(0));
                        auto value2 = cast_constantfp(instr.get_operand(1));
                        if (value1 && value2)
                        {
                            auto fold_const = folder->compute(instr.get_instr_type(), value1, value2);
                            instr.replace_all_use_with_phi(fold_const);
                            wait_delete.push_back(&instr);
                        }
                    }

                    if (instr.is_fadd() || instr.is_fsub() || instr.is_fmul() || instr.is_fdiv())
                    {
                        auto value1 = cast_constantfp(instr.get_operand(0));
                        auto value2 = cast_constantfp(instr.get_operand(1));
                        if (value1 && value2)
                        {
                            auto fold_const = folder->compute(instr.get_instr_type(), value1, value2);

                            instr.replace_all_use_with_phi(fold_const);
                            wait_delete.push_back(&instr);
                        }
                    }
                    if (instr.is_fp2si())
                    {
                        if (auto value_fp = cast_constantfp(instr.get_operand(0)))
                        {
                            auto fold_const = folder->compute(instr.get_instr_type(), value_fp);
                            instr.replace_all_use_with_phi(fold_const);
                            wait_delete.push_back(&instr);
                        }
                    }
                    if (instr.is_si2fp())
                    {
                        if (auto value_int = cast_constantint(instr.get_operand(0)))
                        {
                            auto fold_const = folder->compute(instr.get_instr_type(), value_int);
                            instr.replace_all_use_with_phi(fold_const);
                            wait_delete.push_back(&instr);
                        }
                    }

                    if (instr.is_zext())
                    {
                        auto operand = instr.get_operand(0);
                        // 调用折叠函数
                        auto folded_const = folder->compute_zext(operand);
                        if (folded_const)
                        {
                            instr.replace_all_use_with_phi(folded_const);
                            wait_delete.push_back(&instr);
                        }
                    }
                }

                // globalvar_def.clear();
                for (auto instr : wait_delete)
                {
                    bb.remove_instr(instr);
                }
            }
        }

        target_bb.clear();
        delete_bb.clear();
        for (auto &func : m_->get_functions()) {
            // 1) 先把本轮所有 dead_target 收进 new_dead
            std::vector<BasicBlock*> new_dead;

            for (auto &bb : func.get_basic_blocks()) {
                // printf("%zu\n", bb.get_instructions().size());
                for (auto &instr : bb.get_instructions()) {
                    if (!instr.is_br() || instr.get_num_operand() != 3) continue;
                    auto cond = cast_constantint(instr.get_operand(0));
                    if (!cond) continue;

                    bool take_true = cond->get_value() != 0;
                    auto &brInst    = static_cast<BranchInst&>(instr);
                    BasicBlock *liveBB  = take_true
                                        ? brInst.get_true_block()
                                        : brInst.get_false_block();
                    BasicBlock *deadBB  = take_true
                                        ? brInst.get_false_block()
                                        : brInst.get_true_block();

                    // —— 拆旧边、删原 br、插无条件 br —— //
                    

                    // —— 标记 liveBB —— //
                    target_bb.push_back(liveBB);
                    delete_bb.erase(
                        std::remove(delete_bb.begin(), delete_bb.end(), liveBB),
                        delete_bb.end()
                    );

                    // —— 收集 deadBB —— //
                    if (std::find(new_dead.begin(), new_dead.end(), deadBB) == new_dead.end() &&
                        std::find(target_bb.begin(), target_bb.end(), deadBB) == target_bb.end())
                    {
                        // printf("push in delete %zu\n", deadBB->get_instructions().size());
                        new_dead.push_back(deadBB);
                        bb.remove_succ_basic_block(liveBB);
                        bb.remove_succ_basic_block(deadBB);
                        liveBB->remove_pre_basic_block(&bb);
                        deadBB->remove_pre_basic_block(&bb);

                        bb.remove_instr(&brInst);
                        builder->set_insert_point(&bb);
                        builder->create_br(liveBB);
                    }
                    break;  // 每个 bb 只处理第一条常量分支
                }
            }

            // 2) 把新收集的死块合并到 delete_bb
            delete_bb.insert(delete_bb.end(), new_dead.begin(), new_dead.end());

            // 3) 统一清理这一轮的死块（用拷贝避免递归修改容器）
            auto to_clear = delete_bb;
            for (auto *dead : to_clear) {
                LOG(DEBUG) << "to delete " << dead->get_name();
                clear_blocks_recs(dead);
            }
            delete_bb.clear();
        }

    }
    clearblock();
}

void ConstPropagation::clearblock() {
    for (auto &f : m_->get_functions()) {
        auto function_names = std::vector<std::string>{
            "getint", "getch", "getarray", "getfloat", "getfarray",
            "putint", "putch", "putarray", "putfloat", "putfarray",
            "putf", "before_main", "after_main", "_sysy_starttime", "_sysy_stoptime","memset"
        };
    
        if(std::find(function_names.begin(), function_names.end(), f.get_name()) != function_names.end()){
            continue;
        }
        auto entry = f.get_entry_block();
        if (!entry) continue;

        // 1. 遍历所有可达块
        std::unordered_set<BasicBlock*> reachable;
        std::queue<BasicBlock*> q;
        q.push(entry);
        reachable.insert(entry);

        while (!q.empty()) {
            BasicBlock* bb = q.front();
            q.pop();
            // std::cout<<bb->get_succ_basic_blocks().size()<<std::endl;
            if(bb->get_succ_basic_blocks().size()==0)continue;
            for (auto succ : bb->get_succ_basic_blocks()) {
                if (reachable.find(succ) == reachable.end()) {
                    reachable.insert(succ);
                    q.push(succ);
                }
            }
        }
        // std::cout<<f.get_name()<<std::endl;
        // std::cout<<"reachable"<<std::endl;
        // std::cout<<reachable.size()<<std::endl;

        // 2. 遍历所有块，把不可达的删除
        std::vector<BasicBlock*> to_delete;
        for (auto &start_bb : f.get_basic_blocks()) {
            if (reachable.find(&start_bb) == reachable.end()) {
                to_delete.push_back(&start_bb);
            }
        }

        // std::cout<<"delete"<<std::endl;
        // std::cout<<to_delete.size()<<std::endl;
        for (auto start_bb : to_delete) {
            LOG(DEBUG) << "delete " << start_bb->get_name();
            wait_delete.clear();
            LOG(DEBUG) << "[ConstProp] Checking block: " << start_bb->get_name()
                        << ", is_entry = " << (is_entry(start_bb) ? "true" : "false");

            auto succ_bb = start_bb->get_succ_basic_blocks();
            LOG(DEBUG) << " succ_bb_count = " << succ_bb.size();
            for (auto *each_succ_bb : succ_bb)
            {
                // 1) 先清空本次要删除的指令列表
                wait_delete.clear();

                // 2) 扫描 PHI 指令
                for (auto &instr1 : each_succ_bb->get_instructions())
                {
                    auto *instr = &instr1;
                    if (!instr->is_phi())
                        continue;

                    // LOG(DEBUG) << "hi phi ";
                    // LOG(DEBUG) << "Found PHI in " << each_succ_bb->get_name();

                    // 3) 反向配对删除
                    int n = static_cast<int>(instr->get_num_operand());
                    LOG(DEBUG) << "start phi operands " << n;
                    for (int i = n - 1; i >= 1; i -= 2)
                    {
                        auto *pred = dynamic_cast<BasicBlock *>(instr->get_operand(i));
                        LOG(DEBUG) << "the" << i << "operands";
                        // 如果这个分支对应的前驱已经不存在了，就删掉这对操作数
                        if (pred == start_bb && start_bb->get_pre_basic_blocks().empty())
                        {
                            instr->remove_operand(i - 1);
                            instr->remove_operand(i - 1);
                        }
                    }
                    LOG(DEBUG) << "1st " << static_cast<int>(instr->get_num_operand());
                    // 4) 如果删完只剩一对 (value, bb)，就把整个 PHI 删掉
                    if (instr->get_num_operand() == 2)
                    {
                        Value *onlyVal = instr->get_operand(0);
                        instr->replace_all_use_with_phi(onlyVal);
                        wait_delete.push_back(instr);
                        LOG(DEBUG) << "delete phi ";
                    }
                }

                // 5) 真正从 basic block 中移除那些待删的 PHI
                for (auto *phi : wait_delete)
                {
                    each_succ_bb->remove_instr(phi);
                }

                
                clear_blocks_recs(each_succ_bb);
            }
            auto func=start_bb->get_parent();
            func->remove(start_bb);
        }
    }
}

bool ConstPropagation::is_entry(BasicBlock *bb)
{
    if (bb == nullptr)
        return false;
    auto *func = bb->get_parent();
    if (func == nullptr)
        return false;
    return bb == func->get_entry_block();
}

void ConstPropagation::clear_blocks_recs(BasicBlock *start_bb)
{
    auto func = start_bb->get_parent();
    if (func == nullptr)
    {
        LOG(ERROR) << "basic block-" << start_bb->get_name() << " has no parent function";
    }
    else
    {
        auto prev_bb = start_bb->get_pre_basic_blocks();
        // start_bb has no previous bb and is not the entry of parent function
        if (!is_entry(start_bb))
        {
            if (prev_bb.size() == 0)
            {
                wait_delete.clear();
                LOG(DEBUG) << "[ConstProp] Checking block: " << start_bb->get_name()
                           << ", pred_count = " << prev_bb.size()
                           << ", is_entry = " << (is_entry(start_bb) ? "true" : "false");

                auto succ_bb = start_bb->get_succ_basic_blocks();
                LOG(DEBUG) << " succ_bb_count = " << succ_bb.size();
                for (auto *each_succ_bb : succ_bb)
                {
                    // 1) 先清空本次要删除的指令列表
                    wait_delete.clear();

                    // 2) 扫描 PHI 指令
                    for (auto &instr1 : each_succ_bb->get_instructions())
                    {
                        auto *instr = &instr1;
                        if (!instr->is_phi())
                            continue;

                        // 3) 反向配对删除
                        int n = static_cast<int>(instr->get_num_operand());
                        LOG(DEBUG) << "start phi operands " << n;
                        for (int i = n - 1; i >= 1; i -= 2)
                        {
                            auto *pred = dynamic_cast<BasicBlock *>(instr->get_operand(i));
                            LOG(DEBUG) << "the" << i << "operands";
                            // 如果这个分支对应的前驱已经不存在了，就删掉这对操作数
                            if (pred == start_bb && start_bb->get_pre_basic_blocks().empty())
                            {
                                instr->remove_operand(i - 1);
                                instr->remove_operand(i - 1);
                            }
                        }
                        LOG(DEBUG) << "1st " << static_cast<int>(instr->get_num_operand());
                        // 4) 如果删完只剩一对 (value, bb)，就把整个 PHI 删掉
                        if (instr->get_num_operand() == 2)
                        {
                            Value *onlyVal = instr->get_operand(0);
                            instr->replace_all_use_with_phi(onlyVal);
                            wait_delete.push_back(instr);
                            LOG(DEBUG) << "delete phi ";
                        }
                    }

                    // 5) 真正从 basic block 中移除那些待删的 PHI
                    for (auto *phi : wait_delete)
                    {
                        each_succ_bb->remove_instr(phi);
                    }

                    
                    clear_blocks_recs(each_succ_bb);
                }

                func->remove(start_bb);
            }
            else
            {
                wait_delete.clear();
                LOG(DEBUG) << "1958prev no none";
                auto prev_vec = start_bb->get_pre_basic_blocks();
                LOG(DEBUG) << "prev_vec size" << prev_vec.size();
                for (auto &instr1 : start_bb->get_instructions())
                {
                    auto instr = &instr1;
                    if (instr->is_phi())
                    {
                        LOG(DEBUG) << "hi Find a inner PHI instruction in the sucess node of "
                                      "useless branch";
                        for (int i = instr->get_num_operand() - 1; i >= 1; i -= 2)
                        {
                            auto pred_bb = dynamic_cast<BasicBlock *>(instr->get_operand(i));
                            if (!pred_bb)
                                continue; // 安全防护

                            // 替代 prev_bb.count(pred_bb) == 0
                            if (std::find(prev_vec.begin(), prev_vec.end(), pred_bb) == prev_vec.end())
                            {

                                instr->remove_operand(i - 1);
                                instr->remove_operand(i - 1);
                            }
                        }
                        int operands_num_phi = instr->get_num_operand();
                        if (operands_num_phi == 2)
                        {
                            auto value = instr->get_operand(0);
                            if (value == nullptr) {
                                // std::cout << "Operand 0 is nullptr" << std::endl;
                            } else {
                                // std::cout << "Operand 0 name: " << value->get_name() << std::endl;
                            }

                            // std::cout << value->get_name() << std::endl;
                            instr->replace_all_use_with_phi(value);
                            // each_succ_bb->remove_instr(instr);
                            wait_delete.push_back(instr);
                        }
                    }
                }
                for (auto instr : wait_delete)
                {
                    start_bb->remove_instr(instr);
                }
            }
        }
    }
}


