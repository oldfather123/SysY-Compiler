
#include "Loopcase.hpp"
#include <algorithm>
#include <vector>
#include <deque>

std::string analyze_control_pattern(BBset_t *loop, LoopInfo loop_info, const std::vector<Value *> &current_loop_vars)
{
    if (!loop_info.count(loop))
        return "unknown";

    Value *start = std::get<0>(loop_info.at(loop));

    bool start_depends_on_outer = false;
    if (auto *phi = dynamic_cast<PhiInst *>(start))
    {
        for (size_t i = 0; i < phi->get_num_operand(); ++i)
        {
            Value *operand = phi->get_operand(i);
            if (std::find(current_loop_vars.begin(), current_loop_vars.end(), operand) != current_loop_vars.end())
            {
                start_depends_on_outer = true;
                break;
            }
        }
    }

    if (start_depends_on_outer)
        return "inner_depends_on_outer_start";
    if (current_loop_vars.size() < 2)
        return "partial_loop_var_usage";

    return "normal_control";
}

std::string analyze_compute_pattern(BBset_t *loop, const LoopArrayAccessInfo &access_info)
{
    if (!access_info.count(loop))
        return "unknown";

    const auto &accesses = access_info.at(loop);
    bool has_2d_load = false, has_2d_store = false;
    std::unordered_set<std::string> arrays_read, arrays_written;

    for (const auto &access : accesses)
    {
        if (access.indices.size() >= 2)
        {
            if (access.access_type == "load")
            {
                has_2d_load = true;
                arrays_read.insert(access.array_name);
            }
            else if (access.access_type == "store")
            {
                has_2d_store = true;
                arrays_written.insert(access.array_name);
            }
        }
    }

    if (has_2d_load && has_2d_store)
    {
        if (arrays_read.size() == 2 && arrays_written.size() == 1)
            return "matrix_multiplication";
        if (arrays_read.size() == 1 && arrays_written.size() == 1)
            return "matrix_transpose";
    }

    return "normal_loop";
}

void analyze_control_in_loops(const LoopTree &loop_tree, const LoopInfo &loop_info,
                              const LoopArrayAccessInfo &access_info, LoopControl &loop_con)
{
    std::set<int> used;
    for (const auto &[loop, _] : loop_tree)
    {
        match_loop_var_index(loop, 0, loop_tree, loop_info, access_info, loop_con, used);
    }
}

void match_loop_var_index(
    BBset_t *loop,
    int depth,
    const LoopTree &loop_tree,
    const LoopInfo &loop_info,
    const LoopArrayAccessInfo &access_info,
    LoopControl &loop_con,
    std::set<int> &loop_var_usage_flags)
{
    if (!loop_info.count(loop))
        return;
    if (!access_info.count(loop))
    {
        return;
    }

    Value *start = std::get<0>(loop_info.at(loop)); // step value
    if (start == nullptr)
    {
        loop_con[loop] = "noloop";
        return;
    }
    const auto &accesses = access_info.at(loop);

    for (const auto &access : accesses)
    {
        int j = 0;
        for (auto *v : access.used_loop_vars)
        {

            if (v == start)
            {
                loop_var_usage_flags.insert(j);
            }
            j++;
            if (j == depth)
            {
                break;
            }
        }
    }
    auto it = loop_tree.find(loop);
    if (it != loop_tree.end())
    {
        for (BBset_t *child : it->second)
        {
            match_loop_var_index(child, depth + 1, loop_tree, loop_info, access_info, loop_con, loop_var_usage_flags);
        }
    }
    if (loop_var_usage_flags.find(depth) != loop_var_usage_flags.end())
    {
        loop_con[loop] = "usedbychild";
        loop_var_usage_flags.erase(depth);
    }
}

void print_control_loop_tree(const LoopTree &loop_tree,
                             LoopControl &loop_con,
                             LoopEnd &loopend,
                             BBset_t *loop, int level)
{
    if (loop == nullptr)
        return;

    std::string indent(level * 2, ' ');
//    std::cout << indent << "Loop: "<<loop<<"    ";
    // for (auto bb : *loop)
    // {
    //     std::cout << bb->get_name() << " ";
    // }

    // 打印 LoopInfo 元组（三元组）
    auto it_info = loop_con.find(loop);
    if (it_info != loop_con.end())
    {
        auto control = it_info->second;

     //   std::cout << "control:" << control;
    }
    else
    {
      //  std::cout << "  [LoopInfo missing]";
    }

    auto if_end = loopend.find(loop);
    if (if_end != loopend.end())
    {
        auto &[end, offset] = if_end->second;
        if (end != nullptr)
        {
       //     std::cout << "Value = " << end->get_name() << ", offset = " << offset;
        }
    }
    else
    {
      //  std::cout << "  [LoopInfo missing]";
    }

  //  std::cout << std::endl;

    auto it = loop_tree.find(loop);
    if (it != loop_tree.end())
    {
        for (auto child_loop : it->second)
        {
            print_control_loop_tree(loop_tree, loop_con, loopend, child_loop, level + 1);
        }
    }
}

void print_control(const LoopTree &loop_tree, LoopControl &loop_con, LoopEnd &loopend)
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
            print_control_loop_tree(loop_tree, loop_con, loopend, loop, 0);
        }
    }
}

bool isValueUsedBefore(Instruction *inst, Value *target, std::unordered_set<Value *> &visited)
{
    if (!inst || visited.count(inst))
        return false;
    visited.insert(inst);

    for (unsigned i = 0; i < inst->get_num_operand(); ++i)
    {
        Value *operand = inst->get_operand(i);

        if (operand == target)
            return true;

        if (Instruction *opInst = dynamic_cast<Instruction *>(operand))
        {
            if (isValueUsedBefore(opInst, target, visited))
                return true;
        }
    }

    return false;
}

Value *resolve_loop_end(Value *end, std::vector<BBset_t *> loopset, LoopInfo &loopinfo, BBset_t *self_loop)
{
    while (true)
    {
        bool found = false;

        for (auto *loop : loopset)
        {
            if (loop == self_loop)
                continue;

            auto [start, cur_end, step, step_value, step_type, icmp_type] = loopinfo[loop];

            if (end == step_value)
            {
                end = cur_end;
                found = true;
                break;
            }
        }

        if (!found)
            break;
    }

    return end;
}

void analyzeLoopDependencyUsedByChild(LoopInfo &loop_info, const LoopTree &loop_tree, LoopArrayAccessInfo &loop_arr,
                                      LoopControl &loop_control, LoopEnd &loopend)
{
    // valueusedbychild(loop_info, loop_tree, loop_arr, loop_control);
    std::unordered_map<Value *, BBset_t *> value_bb;
    for (const auto &[loop, _] : loop_tree)
    {
        if (loop_arr.count(loop))
        {
            if (!loop_info.count(loop))
            {
                loop_control[loop] = "noloop";
            }
            else
            {
                auto &[start, end, step, value, inc_type, cmp_type] = loop_info.at(loop);
                if (value)
                {
                    value_bb[value] = loop;
                }
            }
        }
    }

    std::deque<BBset_t *> worklist;

    for (const auto &[parent, _] : loop_tree)
    {
        worklist.push_back(parent);
    }

    std::unordered_set<BBset_t *> visited;

    while (!worklist.empty())
    {
        BBset_t *parent_loop = worklist.front();
        worklist.pop_front();

        if (visited.count(parent_loop))
            continue;
        visited.insert(parent_loop);
        auto it = loop_tree.find(parent_loop);
        if (it != loop_tree.end())
        {
            for (auto *child : it->second)
            {
                if (!visited.count(child))
                {
                    worklist.push_back(child);
                }
            }
        }
        // 处理
        if (loop_control[parent_loop] == "usedbychild")
        {
            continue;
        }
        if(!loop_info.count(parent_loop)){
            loop_control[parent_loop] = "noloop";
            continue;
        }
        // start
        auto &[start, end, step, value, inc_type, cmp_type] = loop_info.at(parent_loop);
        if (start == nullptr)
        {
            loop_control[parent_loop] = "noloop";
        }
        if (loop_arr.count(parent_loop) && !loop_arr.at(parent_loop).empty())
        {
            const ArrayAccessInfo &info = loop_arr.at(parent_loop)[0];
            const std::vector<Value *> &current = info.current_loop_vars;
            std::unordered_set<Value *> visited;
            for (auto *v : current)
            {
                if (v == value)
                {
                    break;
                }
                visited.clear();
                auto inst = dynamic_cast<Instruction *>(start);
                if (isValueUsedBefore(inst, v, visited))
                {
                    if (value_bb.count(v))
                    {
                        auto father = value_bb[v];
                        loop_control[father] = "usedbychild";
                    }
                }
            }
        }
        // end
        bool isUpperBound;
        if (cmp_type == "le" || cmp_type == "lt")
        {
            isUpperBound = true;
        }
        else
        {
            isUpperBound = false;
        }
        int offset = 0;
        Value *final_end = resolveLoopBoundary(end, isUpperBound, parent_loop, loop_info, value_bb, offset);
        bool final_used = false;
        if (loop_arr.count(parent_loop) && !loop_arr.at(parent_loop).empty())
        {
            const ArrayAccessInfo &info = loop_arr.at(parent_loop)[0];
            const std::vector<Value *> &current = info.current_loop_vars;
            std::unordered_set<Value *> visited;
            for (auto *v : current)
            {
                if (v == final_end)
                {
                    break;
                }
                visited.clear();
                auto inst = dynamic_cast<Instruction *>(final_end);
                if (!inst)
                {
                    if (isValueUsedBefore(inst, v, visited))
                    {
                        final_used = true;
                        break;
                    }
                }
            }
        }
        if (final_used == false)
        {
            loopend[parent_loop] = std::make_tuple(final_end, offset);
            continue;
        }
        else
        {
            if (loop_arr.count(parent_loop) && !loop_arr.at(parent_loop).empty())
            {
                const ArrayAccessInfo &info = loop_arr.at(parent_loop)[0];
                const std::vector<Value *> &current = info.current_loop_vars;
                std::unordered_set<Value *> visited;
                for (auto *v : current)
                {
                    if (v == value)
                    {
                        break;
                    }
                    visited.clear();
                    auto inst = dynamic_cast<Instruction *>(end);
                    if (!inst)
                    {
                        if (isValueUsedBefore(inst, v, visited))
                        {
                            if (value_bb.count(v))
                            {
                                auto father = value_bb[v];
                                loop_control[father] = "usedbychild";
                            }
                        }
                    }
                }
            }
        }
    }
}

Value *resolveLoopBoundary(Value *current,
                           bool isUpperBound,
                           BBset_t *loop,
                           LoopInfo &loopinfo,
                           std::unordered_map<Value *, BBset_t *> &value_bb,
                           int &offset)
{
    offset = 0;

    if (!value_bb.count(current))
    {
        return current;
    }

    auto &[start, end, step, value, inc_type, cmp_type] = loopinfo.at(loop);

    Value *next = nullptr;

    if (isUpperBound)
    {
        if (cmp_type == "ge" || cmp_type == "gt")
        { //>,>=
            next = start;
        }
        else if (cmp_type == "le")
        { //<=
            next = end;
            offset += 1;
        }
        else if (cmp_type == "lt")
        { //<
            next = end;
        }
        else
        {
            throw std::runtime_error("wrong upper");
        }
    }
    else
    {
        if (cmp_type == "le" || cmp_type == "lt")
        { //<,<=
            next = start;
        }
        else if (cmp_type == "ge")
        { //>=
            next = end;
            offset -= 1;
        }
        else if (cmp_type == "gt")
        { //>
            next = end;
        }
        else
        {
            throw std::runtime_error("wrong upper");
        }
    }
    BBset_t *newloop;
    if (value_bb.count(next))
    {
        newloop = value_bb[next];
    }
    else
    {
        return next;
    }

    int inner_offset = 0;
    Value *base = resolveLoopBoundary(next, isUpperBound, newloop, loopinfo, value_bb, inner_offset);
    offset += inner_offset;
    return base;
}

bool check_parent_loop_var_used_in_children(
    BBset_t *parent_loop,
    const LoopInfo &loop_info,
    const LoopTree &loop_tree,
    const LoopArrayAccessInfo &loop_arr)
{
    // 父循环变量
    if (!loop_info.count(parent_loop))
        return false;
    Value *parent_var = std::get<3>(loop_info.at(parent_loop));
    if (!parent_var)
        return false;

    // 递归函数：遍历子循环，检查是否用到父循环变量
    std::function<bool(BBset_t *)> dfs_check = [&](BBset_t *loop) -> bool
    {
        // 如果当前循环有数组访问
        if (loop_arr.count(loop))
        {
            const auto &access_list = loop_arr.at(loop);
            for (const auto &access : access_list)
            {
                for (auto *used_var : access.used_loop_vars)
                {
                    if (used_var == parent_var)
                        return true;
                }
            }
        }
        // 递归检查子循环
        if (loop_tree.count(loop))
        {
            for (auto *child : loop_tree.at(loop))
            {
                if (dfs_check(child))
                    return true;
            }
        }
        return false;
    };

    // 只检查子循环，父循环自身不检查
    if (!loop_tree.count(parent_loop))
        return false;

    for (auto *child_loop : loop_tree.at(parent_loop))
    {
        if (dfs_check(child_loop))
            return true;
    }

    return false;
}

void valueusedbychild(
    LoopInfo &loop_info,
    const LoopTree &loop_tree,
    LoopArrayAccessInfo &loop_arr,
    LoopControl &loop_control)
{
    for (const auto &[loop, _] : loop_info)
    {
        bool used = check_parent_loop_var_used_in_children(loop, loop_info, loop_tree, loop_arr);
        if (!used)
        {
            loop_control[loop] = "noused";
        }
    }
}