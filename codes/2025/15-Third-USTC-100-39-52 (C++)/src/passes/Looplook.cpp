
#include "Looplook.hpp"
#include "LoopSearch.hpp"
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include "stack"
#include <algorithm>
#include "Instruction.hpp"
#include <stack>
#include <tuple>

void Looplook::run()
{
    LoopSearch loop_searcher(m_, false);
    loop_searcher.run();
    // LoopTree loop_tree;
    // LoopInfo loop_info;
    // LoopArrayAccessInfo loop_array_access;
    std::unordered_set<BBset_t *> all_loops;
    for (auto &func1 : m_->get_functions())
    {
        auto func = &func1;
        auto loops = loop_searcher.get_loops_in_func(func);

        LOG(INFO) << "LoopInvHoist: Get func " << func->get_name() << ", addr = " << func;
        for (auto loop : loops)
        {   
            all_loops.insert(loop);
            auto parent = loop_searcher.get_parent_loop(loop);
            if (parent)
            {
                loop_tree[parent].insert(loop);
            }
        }
    }
    new_get_loop_info(all_loops,new_loop_info);
    get_loop_info(loop_tree, loop_info);
    // m_->set_print_name();
    // print_all_loops(loop_tree, loop_info);
    collect_array_access(loop_tree, loop_info, loop_array_access);
    // print_array_access_info(loop_array_access,loop_tree);
}

void Looplook::get_loop_info(LoopTree &loop_tree, LoopInfo &loop_info)
{
    std::unordered_set<BBset_t *> visited;
    std::stack<BBset_t *> worklist;
    // const auto noloop = std::make_tuple(nullptr, nullptr, nullptr, nullptr, "", "");
    for (auto &[loop, _] : loop_tree)
    {
        worklist.push(loop);
    }

    while (!worklist.empty())
    {
        BBset_t *loop = worklist.top();
        worklist.pop();

        if (!loop || visited.count(loop))
            continue;
        visited.insert(loop);

        BasicBlock *header = find_loop_header(loop);
        if (!header)
        {
            // loop_info[loop] = noloop;
            // 继续处理子循环
            auto it = loop_tree.find(loop);
            if (it != loop_tree.end())
            {
                for (auto *child : it->second)
                    worklist.push(child);
            }
            continue;
        }

        // std::cout << header->print() << std::endl;

        // 先找icmp指令，确定循环结束判断
        ICmpInst *icmp = nullptr;
        for (auto &inst : header->get_instructions())
        {
            if (auto cmp = dynamic_cast<ICmpInst *>(&inst))
            {
                icmp = cmp;
                break;
            }
        }
        if (!icmp)
        {
            // loop_info[loop] = noloop;
            // 继续处理子循环
            auto it = loop_tree.find(loop);
            if (it != loop_tree.end())
            {
                for (auto *child : it->second)
                    worklist.push(child);
            }
            continue;
        }

        Value *phi_candidate = icmp->get_operand(0);

        // 确认icmp第一个操作数是否是phi节点
        auto *phi_node = dynamic_cast<PhiInst *>(phi_candidate);
        if (!phi_node)
        {
            // loop_info[loop] = noloop;
            // 继续处理子循环
            auto it = loop_tree.find(loop);
            if (it != loop_tree.end())
            {
                for (auto *child : it->second)
                    worklist.push(child);
            }
            continue;
        }

        // 检查phi操作数个数，要求两个来源（4个操作数）
        if (phi_node->get_num_operand() != 4)
        {
            // loop_info[loop] = noloop;
            // 继续处理子循环
            auto it = loop_tree.find(loop);
            if (it != loop_tree.end())
            {
                for (auto *child : it->second)
                    worklist.push(child);
            }
            continue;
        }

        // 检查是否有break/continue，跳出循环跳转非header块直接标记invalid
        // 还要检查函数调用
        bool invalid = false;
        for (auto *bb : *loop)
        {
            for (auto &inst : bb->get_instructions())
            {
                if (auto br = dynamic_cast<BranchInst *>(&inst))
                {
                    for (size_t i = 0; i < br->get_num_operand(); ++i)
                    {
                        auto *target = br->get_operand(i);
                        if (auto *bb_target = dynamic_cast<BasicBlock *>(target))
                        {
                            if (!loop->count(bb_target))
                            {
                                if (bb != header)
                                {
                                    invalid = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (invalid)
                        break;
                }
                if (auto call = dynamic_cast<CallInst *>(&inst))
                {
                    invalid = true;
                    break;
                }
            }
            if (invalid)
                break;
        }

        if (invalid)
        {
            // loop_info[loop] = noloop;
            // 继续处理子循环
            auto it = loop_tree.find(loop);
            if (it != loop_tree.end())
            {
                for (auto *child : it->second)
                    worklist.push(child);
            }
            continue;
        }

        // 从phi的两个操作数中确定start和update_val
        Value *start = nullptr;
        Value *update_val = nullptr;
        for (int i = 0; i < phi_node->get_num_operand(); i += 2)
        {
            Value *val = phi_node->get_operand(i);
            BasicBlock *pred = dynamic_cast<BasicBlock *>(phi_node->get_operand(i + 1));
            if (!loop->count(pred))
            {
                start = val;
            }
            else
            {
                update_val = val;
            }
        }

        if (!start || !update_val)
        {
            // loop_info[loop] = noloop;
            // 继续处理子循环
            auto it = loop_tree.find(loop);
            if (it != loop_tree.end())
            {
                for (auto *child : it->second)
                    worklist.push(child);
            }
            continue;
        }

        // update_val 要是add或sub指令，且一个操作数是phi_node本身，另一个是step
        IBinaryInst *bin = dynamic_cast<IBinaryInst *>(update_val);
        if (!bin || !(bin->is_add() || bin->is_sub()))
        {
            // loop_info[loop] = noloop;
            // 继续处理子循环
            auto it = loop_tree.find(loop);
            if (it != loop_tree.end())
            {
                for (auto *child : it->second)
                    worklist.push(child);
            }
            continue;
        }

        Value *step = nullptr;
        Value *step_num = nullptr;
        if (bin->get_operand(0) == phi_node)
        {
            step = bin->get_operand(1);
            step_num = bin->get_operand(0);
        }
        else if (bin->get_operand(1) == phi_node)
        {
            step = bin->get_operand(0);
            step_num = bin->get_operand(1);
        }
        else
        {
            // loop_info[loop] = noloop;
            auto it = loop_tree.find(loop);
            if (it != loop_tree.end())
            {
                for (auto *child : it->second)
                    worklist.push(child);
            }
            continue;
        }

        auto conststep = dynamic_cast<ConstantInt *>(step);
        // std::cout<<"const   "<<conststep<<"     "<<conststep->get_value()<<std::endl;
        if (!conststep)
        {
            // loop_info[loop] = noloop;
            // 继续处理子循环
            auto it = loop_tree.find(loop);
            if (it != loop_tree.end())
            {
                for (auto *child : it->second)
                    worklist.push(child);
            }
            continue;
        }

        std::string step_type = "";
        std::string icmp_type = "";
        if (bin->is_add())
        {
            step_type = "add";
        }
        else if (bin->is_sub())
        {
            step_type = "sub";
        }
        Value *end = icmp->get_operand(1);
        switch (icmp->get_instr_type())
        {
        case Instruction::OpID::lt:
            icmp_type = "lt";
            break;
        case Instruction::OpID::gt:
            icmp_type = "gt";
            break;
        case Instruction::OpID::ge:
            icmp_type = "ge";
            break;
        case Instruction::OpID::le:
            icmp_type = "le";
            break;
        }
        loop_info[loop] = std::make_tuple(start, end, step, step_num, step_type, icmp_type);
        // std::string s = start ? start->print() : "nullptr";
        // std::string e = end ? end->print() : "nullptr";
        // std::string st = step ? step->print() : "nullptr";
        // std::string stn = step_num ? step_num->print() : "nullptr";

        // std::cout << "  [start: " << s
        //           << ", end: " << e
        //           << ", step: " << st
        //           << ", step_num: " << stn
        //           << ", step_type: " << step_type << "]";
        // std::cout << std::endl;
        // 继续处理子循环
        auto it = loop_tree.find(loop);
        if (it != loop_tree.end())
        {
            for (auto *child : it->second)
                worklist.push(child);
        }
    }
}

void Looplook::new_get_loop_info(const std::unordered_set<BBset_t*> &all_loops, new_LoopInfo &loop_info)
{
    std::unordered_set<BBset_t *> visited;
    std::stack<BBset_t *> worklist;
    const auto noloop = std::make_tuple(nullptr, nullptr, nullptr, nullptr, "", "");
    for (auto &loop : all_loops)
    {
        worklist.push(loop);
    }

    while (!worklist.empty())
    {
        BBset_t *loop = worklist.top();
        worklist.pop();

        if (!loop || visited.count(loop))
            continue;
        visited.insert(loop);

        BasicBlock *header = find_loop_header(loop);
        if (!header)
        {
            continue;
        }

        //std::cout << header->print() << std::endl;

        // 先找icmp指令，确定循环结束判断
        ICmpInst *icmp = nullptr;
        for (auto &inst : header->get_instructions())
        {
            if (auto cmp = dynamic_cast<ICmpInst *>(&inst))
            {
                icmp = cmp;
                break;
            }
        }
        if (!icmp)
        {
            continue;
        }

        Value *phi_candidate = icmp->get_operand(0);

        // 确认icmp第一个操作数是否是phi节点
        auto *phi_node = dynamic_cast<PhiInst *>(phi_candidate);
        if (!phi_node)
        {
            continue;
        }

        // 检查phi操作数个数，要求两个来源（4个操作数）
        if (phi_node->get_num_operand() != 4)
        {
            continue;
        }

        // 检查是否有break/continue，跳出循环跳转非header块直接标记invalid
        bool invalid = false;
        for (auto *bb : *loop)
        {
            for (auto &inst : bb->get_instructions())
            {
                if (auto br = dynamic_cast<BranchInst *>(&inst))
                {
                    for (size_t i = 0; i < br->get_num_operand(); ++i)
                    {
                        auto *target = br->get_operand(i);
                        if (auto *bb_target = dynamic_cast<BasicBlock *>(target))
                        {
                            if (!loop->count(bb_target))
                            {
                                if (bb != header)
                                {
                                    invalid = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (invalid)
                        break;
                }
            }
            if (invalid)
                break;
        }                                  
        if (invalid)
        {
            continue;
        }

        // 从phi的两个操作数中确定start和update_val
        Value *start = nullptr;
        Value *update_val = nullptr;
        BasicBlock* first_while_bb=nullptr;
        BasicBlock* start_bb=nullptr;
        BasicBlock* back_bb=nullptr;
        BasicBlock* after_end_bb=nullptr;
        start_bb=phi_node->get_parent();
        for (int i = 0; i < phi_node->get_num_operand(); i += 2)
        {
            Value *val = phi_node->get_operand(i);
            BasicBlock *pred = dynamic_cast<BasicBlock *>(phi_node->get_operand(i + 1));
            if (!loop->count(pred))
            {
                start = val;
            }
            else
            {
                update_val = val;
                back_bb=pred;
            }
        }

        if (!start || !update_val)
        {
            continue;
        }

        // update_val 要是add或sub指令，且一个操作数是phi_node本身，另一个是step
        IBinaryInst *bin = dynamic_cast<IBinaryInst *>(update_val);
        if (!bin || !(bin->is_add() || bin->is_sub()))
        {
            continue;
        }

        Value *step = nullptr;
        Value *step_num = nullptr;
        if (bin->get_operand(0) == phi_node)
        {
            step = bin->get_operand(1);
            step_num = bin->get_operand(0);
        }
        else if (bin->get_operand(1) == phi_node)
        {
            step = bin->get_operand(0);
            step_num = bin->get_operand(1);
        }
        else
        {
            continue;
        }
        //确认这个找到的phi的i就是while里面用来比大小的i
        bool find_phi=0;
        if(icmp->get_operand(0)==step_num)
            find_phi=1;
        if(icmp->get_operand(1)==step_num)
            find_phi=1;
        if(!find_phi)
            continue;
        auto conststep = dynamic_cast<ConstantInt *>(step);
        // std::cout<<"const   "<<conststep<<"     "<<conststep->get_value()<<std::endl;
        if (!conststep)
        {
            continue;
        }

        std::string step_type = "";
        std::string icmp_type = "";
        if (bin->is_add())
        {
            step_type = "add";
        }
        else if (bin->is_sub())
        {
            step_type = "sub";
        }
        Value *end = icmp->get_operand(1);
        switch (icmp->get_instr_type())
        {
        case Instruction::OpID::lt:
            icmp_type = "lt";
            break;
        case Instruction::OpID::gt:
            icmp_type = "gt";
            break;
        case Instruction::OpID::ge:
            icmp_type = "ge";
            break;
        case Instruction::OpID::le:
            icmp_type = "le";
            break;
        //其余符号都是错的
        default:
            continue;
        }
        //找出 first_while_bb和after_end_bb
        for(auto&inst: start_bb->get_instructions()){
            if(auto br_inst=dynamic_cast<BranchInst*>(&inst)){
                for(auto *bb:*loop){
                    if(br_inst->get_operand(1)==dynamic_cast<Value*>(bb)){
                        first_while_bb=bb;
                        for(auto *bb2:start_bb->get_succ_basic_blocks()){
                            if(bb2!=bb){
                                after_end_bb=bb2;
                            }
                        }
                        break;
                    }else if(br_inst->get_operand(2)==dynamic_cast<Value*>(bb)){
                        first_while_bb=bb;
                        for(auto *bb2:start_bb->get_succ_basic_blocks()){
                            if(bb2!=bb){
                                after_end_bb=bb2;
                            }
                        }
                        break;
                    }
                }
            }
        }
        loop_info[loop] = std::make_tuple(first_while_bb,after_end_bb,start_bb,back_bb,update_val,start, end, step, step_num, step_type, icmp_type);
    }
}

void Looplook::print_loop_tree(const LoopTree &loop_tree,
                               const LoopInfo &loop_info,
                               BBset_t *loop, int level)
{
    if (loop == nullptr)
        return;

    std::string indent(level * 2, ' ');
  //  std::cout << indent << "Loop: " << loop;
    // for (auto bb : *loop)
    // {
    //     std::cout << bb->get_name() << " ";
    // }

    // 打印 LoopInfo 元组（三元组）
    auto it_info = loop_info.find(loop);
    if (it_info != loop_info.end())
    {
        auto [start, end, step, step_num, step_type, icmp_type] = it_info->second;

        std::string s = start ? start->get_name() : "nullptr";
        std::string e = end ? end->get_name() : "nullptr";
        std::string st = step ? step->get_name() : "nullptr";
        std::string stn = step_num ? step_num->get_name() : "nullptr";
        // std::cout << "  [start: " << s
        //           << ", end: " << e
        //           << ", step: " << st
        //           << ", step_num: " << stn
        //           << ", step_type: " << step_type
        //           << ", icmp_type: " << icmp_type << "]";
    }
    else
    {
    //    std::cout << "  [LoopInfo missing]";
    }

//    std::cout << std::endl;

    auto it = loop_tree.find(loop);
    if (it != loop_tree.end())
    {
        for (auto child_loop : it->second)
        {
            print_loop_tree(loop_tree, loop_info, child_loop, level + 1);
        }
    }
}

void Looplook::print_all_loops(const LoopTree &loop_tree, const LoopInfo &loop_info)
{
    std::unordered_set<BBset_t *> seen;
    for (const auto &[parent, children] : loop_tree)
    {
        seen.insert(children.begin(), children.end());
    }

    for (const auto &[loop, _] : loop_tree)
    {
        if (!seen.count(loop))
        {
            print_loop_tree(loop_tree, loop_info, loop, 0);
        }
    }
}

BasicBlock *Looplook::find_loop_header(BBset_t *loop)
{
    for (auto *bb : *loop)
    {
        for (auto *pred : bb->get_pre_basic_blocks())
        {
            if (!loop->count(pred))
            {
                return bb;
            }
        }
    }
    return nullptr;
}

bool Looplook::contains_var(const std::vector<Value *> &vars, Value *v)
{
    return std::find(vars.begin(), vars.end(), v) != vars.end();
}

void Looplook::build_cumulative_loop_vars_ordered(
    const LoopTree &loop_tree,
    const LoopInfo &loop_info,
    BBset_t *current_loop,
    const std::vector<Value *> &inherited_vars,
    std::unordered_map<BBset_t *, std::vector<Value *>> &cumulative_loop_vars)
{
    std::vector<Value *> current_vars = inherited_vars;

    if (loop_info.count(current_loop))
    {
        Value *ind_var = std::get<3>(loop_info.at(current_loop));
        if (!contains_var(current_vars, ind_var))
        {
            current_vars.push_back(ind_var);
        }
    }

    cumulative_loop_vars[current_loop] = current_vars;

    if (loop_tree.count(current_loop))
    {
        for (auto *child_loop : loop_tree.at(current_loop))
        {
            build_cumulative_loop_vars_ordered(loop_tree, loop_info, child_loop, current_vars, cumulative_loop_vars);
        }
    }
}

bool Looplook::is_loop_var(Value *idx, const std::vector<Value *> &loop_vars)
{
    return contains_var(loop_vars, idx);
}

void Looplook::analyze_array_access(
    Value *ptr,
    const std::string &access_type,
    const std::vector<Value *> &loop_vars,
    std::vector<ArrayAccessInfo> &access_list)
{
    auto *gep = dynamic_cast<GetElementPtrInst *>(ptr);
    if (!gep)
        return;

    Value *array_base = gep->get_operand(0);
    if (!array_base)
        return;

    std::string name = array_base->get_name();

    std::vector<Value *> indices;
    std::vector<Value *> used_loop_vars;

    for (size_t i = 1; i < gep->get_num_operand(); ++i)
    {
        Value *idx = gep->get_operand(i);
        indices.push_back(idx);
        if (is_loop_var(idx, loop_vars))
        {
            used_loop_vars.push_back(idx);
        }
    }
    if (used_loop_vars.empty())
        return;

    // 新建访问信息，带上当前循环变量
    access_list.push_back({name,
                           indices,
                           access_type,
                           used_loop_vars,
                           loop_vars}); // 这里赋值当前循环变量
}

void Looplook::collect_array_access(
    const LoopTree &loop_tree,
    const LoopInfo &loop_info,
    LoopArrayAccessInfo &loop_array_access)
{
    // === 收集所有循环 ===
    std::unordered_set<BBset_t *> all_loops;
    for (auto &[parent, children] : loop_tree)
    {
        all_loops.insert(parent);
        for (auto *c : children)
            all_loops.insert(c);
    }

    // === 找出所有根循环（即无父节点的循环） ===
    std::unordered_set<BBset_t *> non_root_loops;
    for (auto &[parent, children] : loop_tree)
    {
        for (auto *c : children)
            non_root_loops.insert(c);
    }

    std::vector<BBset_t *> root_loops;
    for (auto *loop : all_loops)
    {
        if (!non_root_loops.count(loop))
            root_loops.push_back(loop);
    }

    // === 构建每个循环的累积循环变量 ===
    std::unordered_map<BBset_t *, std::vector<Value *>> cumulative_loop_vars;
    for (auto *root : root_loops)
    {
        build_cumulative_loop_vars_ordered(loop_tree, loop_info, root, {}, cumulative_loop_vars);
    }

    // === 构建每个循环独占的基本块集合（去除子循环的块） ===
    std::unordered_map<BBset_t *, std::unordered_set<BasicBlock *>> loop_own_blocks;

    for (auto *loop : all_loops)
    {
        // 先复制当前循环的所有块
        std::unordered_set<BasicBlock *> own_blocks((*loop).begin(), (*loop).end());

        // 找出所有直接子循环，删除它们的块
        auto it = loop_tree.find(loop);
        if (it != loop_tree.end())
        {
            for (auto *child_loop : it->second)
            {
                for (auto *bb : *child_loop)
                {
                    own_blocks.erase(bb);
                }
            }
        }

        loop_own_blocks[loop] = std::move(own_blocks);
    }

    // === 遍历所有循环，分析其独占块中的数组访问 ===
    for (auto *loop_set : all_loops)
    {
        if (!cumulative_loop_vars.count(loop_set))
        {
            std::cerr << "[Warning] Loop not in cumulative_loop_vars: " << loop_set << "\n";
            continue;
        }
        if (loop_info.count(loop_set))
        {
            auto &info = loop_info.at(loop_set);
            Value *start = std::get<0>(info);
            if (start == nullptr)
            {
                // 跳过
                continue;
            }
        }

        const auto &loop_vars = cumulative_loop_vars[loop_set];
        auto &access_list = loop_array_access[loop_set];

        for (auto *bb : loop_own_blocks[loop_set])
        {
            for (auto &inst : bb->get_instructions())
            {
                if (auto *store = dynamic_cast<StoreInst *>(&inst))
                {
                    analyze_array_access(store->get_lval(), "store", loop_vars, access_list);
                }
                else if (auto *load = dynamic_cast<LoadInst *>(&inst))
                {
                    analyze_array_access(load->get_lval(), "load", loop_vars, access_list);
                }
            }
        }
    }
}

void Looplook::print_array_access_info(
    const LoopArrayAccessInfo &access_info,
    const LoopTree &loop_tree)
{
    // 递归打印函数：loop 是当前循环，depth 是缩进层级
    std::function<void(BBset_t *, int)> print_loop = [&](BBset_t *loop, int depth)
    {
        std::string indent(depth * 2, ' '); // 每层缩进两个空格
    //    std::cout << indent << "Loop @" << loop << ":\n";

        auto it = access_info.find(loop);
        if (it != access_info.end())
        {
            for (const auto &access : it->second)
            {
                //std::cout << indent << "  " << access.access_type << " " << access.array_name << "[";
                for (size_t i = 0; i < access.indices.size(); ++i)
                {
                 //   if (i > 0)
                 //       std::cout << ", ";
                 //   std::cout << access.indices[i]->get_name();
                }
              //  std::cout << "]";
                if (!access.used_loop_vars.empty())
                {
                 //   std::cout << " (loop vars: ";
                 //   for (auto *v : access.used_loop_vars)
                 //       std::cout << v->get_name() << " ";
                 //   std::cout << ")";
                }
                if (!access.current_loop_vars.empty())
                {
                    // std::cout << " (current_loop_vars: ";
                    // for (auto *v : access.current_loop_vars)
                    //     std::cout << v->get_name() << " ";
                    // std::cout << ")";
                }
              //  std::cout << "\n";
            }
        }

        // 递归打印子循环
        auto children_it = loop_tree.find(loop);
        if (children_it != loop_tree.end())
        {
            for (BBset_t *child : children_it->second)
            {
                print_loop(child, depth + 1);
            }
        }
    };

    // 找出所有顶层循环（没有父亲的）
    std::unordered_set<BBset_t *> all_loops;
    std::unordered_set<BBset_t *> child_loops;
    for (const auto &[parent, children] : loop_tree)
    {
        all_loops.insert(parent);
        child_loops.insert(children.begin(), children.end());
    }
    for (BBset_t *loop : all_loops)
    {
        if (!child_loops.count(loop))
        {
            print_loop(loop, 0); // 从顶层循环开始打印
        }
    }
}