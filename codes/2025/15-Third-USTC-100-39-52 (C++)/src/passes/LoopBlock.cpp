#include "LoopBlock.hpp"
#include "Looplook.hpp"
// #include <deque>
#include <algorithm>
#include "Instclone.hpp"
#include "IRBuilder.hpp"

void LoopBlock::run()
{
    Looplook looplook(m_);
    looplook.run();
    LoopTree loop_tree = looplook.get_looptree();
    LoopInfo loop_info = looplook.get_loopinfo();
    LoopArrayAccessInfo loop_array_access = looplook.get_looparrayinfo();
    looplook.clearloopinfo();
  //  m_->set_print_name();
//  looplook.print_all_loops(loop_tree, loop_info);
//    looplook.print_array_access_info(loop_array_access, loop_tree);
    // LoopControl loop_con;
    // analyze_control_in_loops(loop_tree,loop_info,loop_array_access,loop_con);
    // print_control(loop_tree,loop_con);
    LoopControl loop_con;
    LoopEnd loopend;
    analyzeLoopDependencyUsedByChild(loop_info, loop_tree, loop_array_access, loop_con, loopend);
//    print_control(loop_tree, loop_con, loopend);
    LoopSequence loopseq;
    get_block_info(loop_tree, loop_array_access, loop_con, loopseq, loop_info);
//    print_loop_sequence(loopseq);
    // block();
    addwhileblock addblock;   // 当前使用的 block
    addwhileblock next_block; // 存储新生成的 block
    int i = 0;
    // m_->set_print_name();
    for (const auto &loop_path : loopseq)
    {
        bool is_first = true;

        if (loop_path.empty())
            continue;

        for (auto it = loop_path.rbegin(); it != loop_path.rend(); ++it)
        {
            if (is_first)
            {
                // 第一次：用最外层 loop_path.front()

                addwhile(loop_path.front(), &addblock);
                is_first = false;
            }

            else
            {
                // 之后：用上一次的 addblock 做输入
                addwhile(&addblock, &next_block);
                addblock = next_block; // 更新当前 addblock
            }

            fix_addwhile(loop_info, *it, &addblock, loop_path, loopend, 150);
        }
    }
}

// void LoopBlock::get_block_info(const LoopTree &loop_tree,
//                                const LoopArrayAccessInfo &access_info,
//                                const LoopControl &loop_con,
//                                LoopSequence &sequence,
//                                LoopInfo loop_info)
// {
//     std::function<void(BBset_t *)> collect_sequences;

//     collect_sequences = [&](BBset_t *loop)
//     {
//         auto it_con = loop_con.find(loop);
//         bool is_used_by_child = (it_con != loop_con.end() && matchnoloop(it_con->second));
//         auto it = loop_tree.find(loop);
//         size_t child_count = (it == loop_tree.end()) ? 0 : it->second.size();

//         bool has_access = access_info.count(loop) && !access_info.at(loop).empty();

//         if (!is_used_by_child)
//         {
//             if (child_count > 1)
//             {
//                 // 父循环先 push
//                 if (has_access)
//                     sequence.push_back({loop});

//                 // 再递归子循环
//                 for (auto *child : it->second)
//                     collect_sequences(child);
//             }
//             else
//             {
//                 // 单子循环或无子循环，尝试打包路径
//                 std::vector<BBset_t *> path;
//                 BBset_t *cur = loop;

//                 while (true)
//                 {
//                     auto it_cur_con = loop_con.find(cur);
//                     if (it_cur_con != loop_con.end() && matchnoloop(it_cur_con->second))
//                     {
//                         // 先 push 已经收集的父路径
//                         if (!path.empty())
//                             sequence.push_back(path);

//                         // 遇到 usedbychild：停止打包，但递归它的子循环
//                         auto it_sub = loop_tree.find(cur);
//                         if (it_sub != loop_tree.end())
//                         {
//                             for (auto *child : it_sub->second)
//                                 collect_sequences(child);
//                         }
//                         return; // 路径打包结束
//                     }

//                     path.push_back(cur);

//                     auto it2 = loop_tree.find(cur);
//                     if (it2 == loop_tree.end() || it2->second.size() != 1)
//                     {
//                         // 先 push 父路径
//                         if (!path.empty())
//                             sequence.push_back(path);

//                         // 再递归子循环
//                         if (it2 != loop_tree.end())
//                         {
//                             for (auto *child : it2->second)
//                                 collect_sequences(child);
//                         }
//                         return; // 路径打包结束
//                     }

//                     cur = *it2->second.begin();
//                 }
//             }
//         }
//         else
//         {
//             // 当前 loop 是 usedbychild，不加入 sequence
//             // 但子循环要继续递归判断
//             if (it != loop_tree.end())
//             {
//                 for (auto *child : it->second)
//                     collect_sequences(child);
//             }
//         }
//     };

//     // 找根循环
//     std::unordered_set<BBset_t *> all_loops, non_roots;
//     for (const auto &[p, cs] : loop_tree)
//     {
//         all_loops.insert(p);
//         all_loops.insert(cs.begin(), cs.end());
//         non_roots.insert(cs.begin(), cs.end());
//     }

//     std::vector<BBset_t *> roots;
//     for (auto *loop : all_loops)
//         if (!non_roots.count(loop))
//             roots.push_back(loop);

//     // 从根递归出发
//     for (auto *root : roots)
//         collect_sequences(root);
// }

// void LoopBlock::get_block_info(const LoopTree &loop_tree,
//                                const LoopArrayAccessInfo &access_info,
//                                const LoopControl &loop_con,
//                                LoopSequence &sequence,
//                                LoopInfo loop_info)
// {
//     auto is_start_constant = [&](BBset_t *loop) -> bool
//     {
//         auto it_info = loop_info.find(loop);
//         if (it_info == loop_info.end())
//             return false;
//         auto &start_val = std::get<0>(it_info->second); // 第一项
//         return start_val && dynamic_cast<Constant *>(start_val);
//     };

//     std::function<bool(BBset_t *)> collect_sequences;
//     collect_sequences = [&](BBset_t *loop) -> bool
//     {
//         if (!is_start_constant(loop))
//             return false;

//         auto it_con = loop_con.find(loop);
//         bool is_used_by_child = (it_con != loop_con.end() && matchnoloop(it_con->second));
//         auto it = loop_tree.find(loop);
//         size_t child_count = (it == loop_tree.end()) ? 0 : it->second.size();

//         // bool has_access = access_info.count(loop) && !access_info.at(loop).empty();

//         if (!is_used_by_child)
//         {
//             if (child_count > 1)
//             {
//                 bool has_valid_child = false;

//                 // **先把父循环单独加入sequence**
//                 sequence.push_back({loop});

//                 // 再递归处理所有子循环
//                 for (auto *child : it->second)
//                 {
//                     if (collect_sequences(child))
//                         has_valid_child = true;
//                 }

//                 return has_valid_child || true;
//             }
//             else
//             {
//                 std::vector<BBset_t *> path;
//                 BBset_t *cur = loop;

//                 while (true)
//                 {
//                     if (!is_start_constant(cur))
//                         return false; // path 中任何一个不是常量直接丢掉

//                     auto it_cur_con = loop_con.find(cur);
//                     if (it_cur_con != loop_con.end() && matchnoloop(it_cur_con->second))
//                     {
//                         // path 内是父到子顺序，直接push
//                         if (!path.empty())
//                             sequence.push_back(path);

//                         auto it_sub = loop_tree.find(cur);
//                         bool has_valid = false;
//                         if (it_sub != loop_tree.end())
//                         {
//                             for (auto *child : it_sub->second)
//                                 if (collect_sequences(child))
//                                     has_valid = true;
//                         }
//                         return has_valid || !path.empty();
//                     }

//                     // 先加入当前循环，保证父循环在前
//                     path.push_back(cur);

//                     auto it2 = loop_tree.find(cur);
//                     if (it2 == loop_tree.end() || it2->second.size() != 1)
//                     {
//                         if (!path.empty())
//                             sequence.push_back(path);

//                         bool has_valid = false;
//                         if (it2 != loop_tree.end())
//                         {
//                             for (auto *child : it2->second)
//                                 if (collect_sequences(child))
//                                     has_valid = true;
//                         }
//                         return has_valid || !path.empty();
//                     }

//                     cur = *it2->second.begin();
//                 }
//             }
//         }
//         else
//         {
//             bool has_valid_child = false;
//             if (it != loop_tree.end())
//             {
//                 for (auto *child : it->second)
//                     if (collect_sequences(child))
//                         has_valid_child = true;
//             }
//             return has_valid_child;
//         }
//     };

//     // 找根循环
//     std::unordered_set<BBset_t *> all_loops, non_roots;
//     for (const auto &[p, cs] : loop_tree)
//     {
//         all_loops.insert(p);
//         all_loops.insert(cs.begin(), cs.end());
//         non_roots.insert(cs.begin(), cs.end());
//     }

//     std::vector<BBset_t *> roots;
//     for (auto *loop : all_loops)
//         if (!non_roots.count(loop))
//             roots.push_back(loop);

//     // 从根递归出发
//     for (auto *root : roots)
//         collect_sequences(root);
// }

// void LoopBlock::get_block_info(const LoopTree &loop_tree,
//                                const LoopArrayAccessInfo &access_info,
//                                const LoopControl &loop_con,
//                                LoopSequence &sequence,
//                                LoopInfo loop_info)
// {
//     auto is_start_constant = [&](BBset_t *loop) -> bool
//     {
//         auto it_info = loop_info.find(loop);
//         if (it_info == loop_info.end())
//             return false;
//         auto &start_val = std::get<0>(it_info->second); // 第一项
//         return start_val && dynamic_cast<Constant *>(start_val);
//     };

//     std::function<bool(BBset_t *)> collect_sequences;
//     collect_sequences = [&](BBset_t *loop) -> bool
//     {
//         if (!is_start_constant(loop))
//             return false;

//         auto it_con = loop_con.find(loop);
//         bool is_used_by_child = (it_con != loop_con.end() && matchnoloop(it_con->second));
//         auto it = loop_tree.find(loop);
//         size_t child_count = (it == loop_tree.end()) ? 0 : it->second.size();

//         if (!is_used_by_child)
//         {
//             std::vector<BBset_t *> path;
//             BBset_t *cur = loop;

//             while (true)
//             {
//                 if (!is_start_constant(cur))
//                     return false; // 连续性断掉 → 丢弃整个 path

//                 auto it_cur_con = loop_con.find(cur);
//                 if (it_cur_con != loop_con.end() && matchnoloop(it_cur_con->second))
//                 {
//                     // 到这里截断，path 才有效
//                     if (path.size() >= 2)
//                         sequence.push_back(path);

//                     auto it_sub = loop_tree.find(cur);
//                     bool has_valid = false;
//                     if (it_sub != loop_tree.end())
//                     {
//                         for (auto *child : it_sub->second)
//                             if (collect_sequences(child))
//                                 has_valid = true;
//                     }
//                     return has_valid || path.size() >= 2;
//                 }

//                 // 先加入当前循环，保证父循环在前
//                 path.push_back(cur);

//                 auto it2 = loop_tree.find(cur);
//                 if (it2 == loop_tree.end())
//                 {
//                     // 到底了，整条 path 有效
//                     if (path.size() >= 2)
//                         sequence.push_back(path);
//                     return path.size() >= 2;
//                 }

//                 if (it2->second.size() != 1)
//                 {
//                     // 不连续，直接结束
//                     if (path.size() >= 2)
//                         sequence.push_back(path);
//                     return path.size() >= 2;
//                 }

//                 cur = *it2->second.begin();
//             }
//         }
//         else
//         {
//             // is_used_by_child = true，则不单独 push 当前 loop，只递归子循环
//             bool has_valid_child = false;
//             if (it != loop_tree.end())
//             {
//                 for (auto *child : it->second)
//                     if (collect_sequences(child))
//                         has_valid_child = true;
//             }
//             return has_valid_child;
//         }
//     };

//     // 找根循环
//     std::unordered_set<BBset_t *> all_loops, non_roots;
//     for (const auto &[p, cs] : loop_tree)
//     {
//         all_loops.insert(p);
//         all_loops.insert(cs.begin(), cs.end());
//         non_roots.insert(cs.begin(), cs.end());
//     }

//     std::vector<BBset_t *> roots;
//     for (auto *loop : all_loops)
//         if (!non_roots.count(loop))
//             roots.push_back(loop);

//     // 从根递归出发
//     for (auto *root : roots)
//         collect_sequences(root);
// }

// void LoopBlock::get_block_info(const LoopTree &loop_tree,
//                                const LoopArrayAccessInfo &access_info,
//                                const LoopControl &loop_con,
//                                LoopSequence &sequence,
//                                LoopInfo loop_info)
// {
//     // 判断 loop 的 start 是否是常量
//     auto is_start_constant = [&](BBset_t *loop) -> bool
//     {
//         auto it_info = loop_info.find(loop);
//         if (it_info == loop_info.end())
//             return false;
//         auto &start_val = std::get<0>(it_info->second); // 第一项
//         return start_val && dynamic_cast<Constant *>(start_val);
//     };

//     std::function<bool(BBset_t *)> collect_sequences;
//     collect_sequences = [&](BBset_t *loop) -> bool
//     {
//         if (!is_start_constant(loop))
//             return false;

//         // 判断当前循环是否存在 load/store
//         auto it_acc = access_info.find(loop);
//         bool has_load_store = false;
//         if (it_acc != access_info.end())
//         {
//             for (auto &acc : it_acc->second)
//             {
//                 if (acc.access_type == "load" || acc.access_type == "store")
//                 {
//                     has_load_store = true;
//                     break;
//                 }
//             }
//         }

//         auto it_con = loop_con.find(loop);
//         bool is_used_by_child = (it_con != loop_con.end() && matchnoloop(it_con->second));
//         auto it = loop_tree.find(loop);
//         size_t child_count = (it == loop_tree.end()) ? 0 : it->second.size();

//         // 非最里面层（有子循环）有 load/store，直接丢弃整条路径
//         if (child_count > 0 && has_load_store)
//             return false;

//         if (!is_used_by_child)
//         {
//             if (child_count > 1)
//             {
//                 bool has_valid_child = false;

//                 // 父循环单独加入 sequence
//                 sequence.push_back({loop});

//                 // 递归处理所有子循环
//                 for (auto *child : it->second)
//                 {
//                     if (collect_sequences(child))
//                         has_valid_child = true;
//                 }

//                 return has_valid_child;
//             }
//             else if (child_count == 1)
//             {
//                 // 只有一个子循环，递归下去
//                 auto child = *(it->second.begin());
//                 return collect_sequences(child);
//             }
//             else
//             {
//                 // 最里面层循环，必须有 load/store 才有效
//                 if (!has_load_store)
//                     return false;

//                 sequence.push_back({loop});
//                 return true;
//             }
//         }
//         else
//         {
//             bool has_valid_child = false;
//             if (it != loop_tree.end())
//             {
//                 for (auto *child : it->second)
//                     if (collect_sequences(child))
//                         has_valid_child = true;
//             }
//             return has_valid_child;
//         }
//     };

//     // 找根循环
//     std::unordered_set<BBset_t *> all_loops, non_roots;
//     for (const auto &[p, cs] : loop_tree)
//     {
//         all_loops.insert(p);
//         all_loops.insert(cs.begin(), cs.end());
//         non_roots.insert(cs.begin(), cs.end());
//     }

//     std::vector<BBset_t *> roots;
//     for (auto *loop : all_loops)
//         if (!non_roots.count(loop))
//             roots.push_back(loop);

//     // 从根循环递归出发
//     for (auto *root : roots)
//         collect_sequences(root);
// }

void LoopBlock::get_block_info(const LoopTree &loop_tree,
                               const LoopArrayAccessInfo &access_info,
                               const LoopControl &loop_con,
                               LoopSequence &sequence,
                               LoopInfo loop_info)
{
    auto is_start_constant = [&](BBset_t *loop) -> bool
    {
        auto it_info = loop_info.find(loop);
        if (it_info == loop_info.end())
            return false;
        auto &start_val = std::get<0>(it_info->second);
        return start_val && dynamic_cast<Constant *>(start_val);
    };

    // match 函数判断 loop_con
    auto is_matched = [&](BBset_t *loop) -> bool
    {
        LoopControl::const_iterator it = loop_con.find(loop);
        if (it == loop_con.end())
            return false;         // 默认未匹配
        return matchnoloop(it->second); // 使用外部 match 函数
    };

    std::function<bool(BBset_t *, std::vector<BBset_t *> &)> collect_sequences;
    collect_sequences = [&](BBset_t *loop, std::vector<BBset_t *> &path) -> bool
    {
        // 如果 match 返回 true，则丢弃
        if (is_matched(loop))
            return false;

        if (!is_start_constant(loop))
            return false;

        // load/store 判定
        bool has_load_store = false;
        LoopArrayAccessInfo::const_iterator it_acc = access_info.find(loop);
        if (it_acc != access_info.end())
        {
            for (size_t i = 0; i < it_acc->second.size(); i++)
            {
                if (it_acc->second[i].access_type == "load" ||
                    it_acc->second[i].access_type == "store")
                {
                    has_load_store = true;
                    break;
                }
            }
        }

        // 子循环信息
        LoopTree::const_iterator it = loop_tree.find(loop);
        size_t child_count = (it == loop_tree.end()) ? 0 : it->second.size();
        bool is_outer = (child_count > 0);

        // 外层有 load/store → 丢弃
        if (is_outer && has_load_store)
            return false;

        // 多个子循环 → 丢弃
        if (child_count > 1)
            return false;

        bool valid = false;

        // 单个子循环 → 递归
        if (child_count == 1)
        {
            BBset_t *child = *(it->second.begin());
            std::vector<BBset_t *> child_path;
            valid = collect_sequences(child, child_path);
            if (valid)
            {
                // 如果子循环合法，把自己加入 path
                path.push_back(loop);
                path.insert(path.end(), child_path.begin(), child_path.end());
            }
            return valid;
        }

        // 最内层循环：必须有 load/store
        if (!has_load_store)
            return false;

        // 合法最内层循环
        path.push_back(loop);
        return true;
    };

    // 找根循环
    std::unordered_set<BBset_t *> all_loops, non_roots;
    for (LoopTree::const_iterator it = loop_tree.begin(); it != loop_tree.end(); ++it)
    {
        all_loops.insert(it->first);
        all_loops.insert(it->second.begin(), it->second.end());
        non_roots.insert(it->second.begin(), it->second.end());
    }

    std::vector<BBset_t *> roots;
    for (std::unordered_set<BBset_t *>::iterator it = all_loops.begin(); it != all_loops.end(); ++it)
    {
        if (!non_roots.count(*it))
            roots.push_back(*it);
    }

    // 从根开始递归
    for (size_t i = 0; i < roots.size(); i++)
    {
        std::vector<BBset_t *> path;
        if (collect_sequences(roots[i], path))
        {
            // path 顺序从外到内
            sequence.push_back(path);
        }
    }
}

void LoopBlock::printBBset(const BBset_t *bbset)
{
    std::cout << "Loop@" << bbset << std::endl;
}

void LoopBlock::print_loop_sequence(const LoopSequence &sequence)
{
    int seq_idx = 0;
    for (const auto &loop_path : sequence)
    {
        std::cout << "Sequence " << seq_idx++ << ": ";
        for (auto *loop : loop_path)
        {
            // std::cout << loop << " ";  // 如果有循环名称或ID，可以改成输出那个
            printBBset(loop);
        }
        std::cout << "\n";
    }
}

void LoopBlock::block(LoopSequence &loopseq, LoopInfo &loop_info, LoopEnd &loopend)
{
    addwhileblock addblock;   // 当前使用的 block
    addwhileblock next_block; // 存储新生成的 block
    // int i=0;
    // m_->set_print_name();
    for (const auto &loop_path : loopseq)
    {
        bool is_first = true;

        if (loop_path.empty())
            continue;

        for (auto it = loop_path.rbegin(); it != loop_path.rend(); ++it)
        {
            if (is_first)
            {
                // 第一次：用最外层 loop_path.front()

                addwhile(loop_path.front(), &addblock);
                is_first = false;
            }

            else
            {
                // 之后：用上一次的 addblock 做输入
                addwhile(&addblock, &next_block);
                addblock = next_block; // 更新当前 addblock
            }

            fix_addwhile(loop_info, *it, &addblock, loop_path, loopend, 16);
        }
    }
}

void LoopBlock::addwhile(BBset_t *whileblock, addwhileblock *addblock)
{
    auto builder = IRBuilder(nullptr, m_);
    BasicBlock *header = find_loop_header(whileblock);
    BasicBlock *endwhile;
    BasicBlock *entry = nullptr;
    int count = 0;
    for (auto *pred : header->get_pre_basic_blocks())
    {
        if (whileblock->count(pred) == 0)
        {
            entry = pred;
            ++count;
        }
    }

    if (count != 1)
    {
        std::cerr << "Error: header has " << count << " outside-loop predecessors, expected exactly 1.\n";
        throw std::runtime_error("Loop header external predecessor count != 1");
    }

    BasicBlock *add_head = insert_before_block(entry, m_);

    BasicBlock *add_entry = insert_before_block(add_head, m_);

    BasicBlock *add_body = BasicBlock::create(m_, "", entry->get_parent());
    endwhile = static_cast<BasicBlock *>(header->get_terminator()->get_operand(2));

    header->get_terminator()->set_operand(2, add_body);
    // endwhile=header->get_succ_basic_blocks().front();
    // header和endwhile断
    header->remove_succ_basic_block(endwhile);
    endwhile->remove_pre_basic_block(header);

    // header和add_body接
    header->add_succ_basic_block(add_body);
    add_body->add_pre_basic_block(header);

    // endwhile和add_head接，后面做
    //  endwhile->add_pre_basic_block(add_head);
    //  add_head->add_succ_basic_block(endwhile);

    // add_body->add_succ_basic_block(add_head);
    *addblock = std::make_tuple(add_head, add_entry, add_body, endwhile);
    // add_body指向addhead

    builder.set_insert_point(add_body);
    builder.create_br(add_head);

    // entry的内容应该挪到add_entry
    // std::vector<Instruction *> to_move;
    // for (auto &inst : entry->get_instructions())
    // {
    //     if (inst.isTerminator())
    //         break;
    //     to_move.push_back(&inst);
    // }
    // move_instrs(add_entry, entry, to_move, add_entry->get_terminator());

    // std::list<BasicBlock *> bblist;
    // bblist.push_back(header);
    // bblist.push_back(endwhile);
    // bblist.push_back(entry);
    // bblist.push_back(add_head);
    // bblist.push_back(add_entry);
    // bblist.push_back(add_body);
    // for (auto bb : bblist)
    // {
    //     std::cout << bb << std::endl;
    //     std::cout << "    pre:";
    //     for (auto prebb : bb->get_pre_basic_blocks())
    //     {
    //         std::cout << prebb << " ";
    //     }
    //     std::cout << std::endl;
    //     std::cout << "    succ:";
    //     for (auto succbb : bb->get_succ_basic_blocks())
    //     {
    //         std::cout << succbb << " ";
    //     }
    //     std::cout << std::endl;
    // }
    // m_->set_print_name();
    // std::cout<<add_head->print()<<"  "<<add_entry->print()<<"  "<<add_body->print()<<"  "<<endwhile->print()<<"  "<<std::endl;
}

void LoopBlock::addwhile(addwhileblock *insertedblock, addwhileblock *addblock)
{
    auto builder = IRBuilder(nullptr, m_);
    auto &[header, entry, body, endwhile] = *insertedblock;

    BasicBlock *add_head = insert_before_block(entry, m_);

    BasicBlock *add_entry = insert_before_block(add_head, m_);

    BasicBlock *add_body = BasicBlock::create(m_, "", entry->get_parent());
    header->get_terminator()->set_operand(2, add_body);
    // endwhile=header->get_succ_basic_blocks().front();
    header->remove_succ_basic_block(endwhile);
    endwhile->remove_pre_basic_block(header);
    endwhile->add_pre_basic_block(add_head);
    add_head->add_succ_basic_block(endwhile);
    header->add_succ_basic_block(add_body);
    // add_head->remove_succ_basic_block(entry);
    add_body->add_pre_basic_block(header);
    // add_body->add_succ_basic_block(add_head);
    *addblock = std::make_tuple(add_head, add_entry, add_body, endwhile);
    builder.set_insert_point(add_body);
    builder.create_br(add_head);

    // entry的内容应该挪到add_entry

    // std::vector<Instruction *> to_move;
    // for (auto &inst : entry->get_instructions())
    // {
    //     if (inst.isTerminator())
    //         break;
    //     to_move.push_back(&inst);
    // }
    // move_instrs(add_entry, entry, to_move, add_entry->get_terminator());

    // 返回原来head应该变为返回add_head
}

void pre_succ(BasicBlock *bb)
{
  //  std::cout << bb << std::endl;
   // std::cout << "    pre:";
    for (auto prebb : bb->get_pre_basic_blocks())
    {
      //  std::cout << prebb << " ";
    }
  //  std::cout << std::endl;
  //  std::cout << "    succ:";
    for (auto succbb : bb->get_succ_basic_blocks())
    {
       // std::cout << succbb << " ";
    }
   // std::cout << std::endl;
   // std::cout << "-------------------------------" << std::endl;
}

BasicBlock *LoopBlock::find_loop_header(BBset_t *loop)
{
    BasicBlock *header = nullptr;

    for (auto *bb : *loop)
    {
        for (auto *pred : bb->get_pre_basic_blocks())
        {
            if (!loop->count(pred)) // 有来自外部的前驱
            {
                if (header != nullptr)
                {
                    // std::cout << bb->print() << "   " << pred->print() << std::endl;
                    // std::cerr << "[Error] Multiple loop headers detected in loop set!\n";
                    // std::cerr << "Existing header: " << header->get_name() << ", "
                    //           << "New header: " << bb->get_name() << std::endl;
                    assert(false && "Multiple headers found in loop!");
                }
                header = bb;
            }
        }
    }

    return header;
}

void LoopBlock::fix_addwhile(LoopInfo loopinfo, BBset_t *loop, addwhileblock *addblock, std::vector<BBset_t *> loopset, LoopEnd loopend, int size)
{
    auto builder = IRBuilder(nullptr, m_);
    auto &[add_head, add_entry, add_body, add_end] = *addblock;
    std::vector<Instruction *> to_move;

    BasicBlock *header = find_loop_header(loop);
    BasicBlock *putinst = BasicBlock::create(m_, "", header->get_parent());

    BasicBlock *entry = nullptr;
    int count = 0;
    for (auto *pred : header->get_pre_basic_blocks())
    {
        if (loop->count(pred) == 0)
        {
            entry = pred;
            ++count;
        }
    }
    if (count != 1)
    {
        std::cerr << "Error: header has " << count << " outside-loop predecessors, expected exactly 1.\n";
        throw std::runtime_error("Loop header external predecessor count != 1");
    }

    auto [start, end, step, step_value, step_type, icmp_type] = loopinfo.at(loop);

    // //end应该都能放到entry外面
    if (dynamic_cast<Instruction *>(end))
    {
        if (dynamic_cast<Instruction *>(end)->get_parent() == header)
        {
            if (dynamic_cast<PhiInst *>(end))
            {
                throw(std::runtime_error("end is phi"));
            }
            to_move.push_back(dynamic_cast<Instruction *>(end));
            move_instrs(entry, header, to_move, entry->get_terminator());
            to_move.clear();
        }
    }

    // entrycopy
    // 后续记得删掉无用指令，嗯
    // std::unordered_map<Value *, Value *> inst_map;
    // if (loop != loopset[0])
    // {
    //     for (auto &inst : entry->get_instructions())
    //     {
    //         if (dynamic_cast<PhiInst *>(&inst))
    //         {
    //             throw(std::runtime_error("entry phi"));
    //         }
    //         if (&inst == entry->get_terminator())
    //         {
    //             break;
    //         }
    //         Instruction *cloneinst = clone_inst(&inst, putinst, inst_map);
    //         inst_map[&inst] = cloneinst;
    //         to_move.push_back(cloneinst);
    //     }
    //     move_instrs(add_entry, putinst, to_move, add_entry->get_terminator());
    //     to_move.clear();
    // }
    // // 最外层直接移动，应该没phi
    // else
    // {
    //     for (auto &inst : entry->get_instructions())
    //     {
    //         if (inst.isTerminator())
    //             break;
    //         to_move.push_back(&inst);
    //     }
    //     move_instrs(add_entry, entry, to_move, add_entry->get_terminator());
    //     to_move.clear();
    // }

    if (loop == loopset[0])
    {
        for (auto &inst : entry->get_instructions())
        {
            if (inst.isTerminator())
                break;
            to_move.push_back(&inst);
        }
        move_instrs(add_entry, entry, to_move, add_entry->get_terminator());
        to_move.clear();
    }

    // newend使用Loopend
    //  auto newend = resolve_loop_end(end, loopset, loopinfo, loop);
    Value *final_newend;
    auto [newend, offest] = loopend[loop];
    final_newend = newend;
    // auto newstart=resolve_loop_start(start, loopset, loopinfo, loop);
    // std::cout << newend << "    " << end << std::endl;
    if (offest != 0)
    {
        builder.set_insert_point(putinst);
        if (offest > 0)
        {
            ConstantInt *constant = ConstantInt::get(offest, m_);
            final_newend = builder.create_iadd(newend, constant);
        }
        else
        {
            ConstantInt *constant = ConstantInt::get(-offest, m_);
            final_newend = builder.create_isub(newend, constant);
        }
        to_move.push_back(dynamic_cast<Instruction *>(final_newend));
        // std::cout << to_move.size() << std::endl;
        move_instrs(add_entry, putinst, to_move, add_entry->get_terminator());
        to_move.clear();
    }
    // 如果最后是N-1这样的，嗯
    // if (final_newend == end)
    // {
    //     if (inst_map.count(end))
    //     {
    //         final_newend = inst_map[end];
    //     }
    // }

    if (final_newend == end)
    {
        if (static_cast<Instruction *>(final_newend)->get_parent() == entry)
        {
            to_move.push_back(static_cast<Instruction *>(final_newend));
            move_instrs(add_entry, entry, to_move, add_entry->get_terminator());
            to_move.clear();
        }
    }

    if (!start || !end)
    {
        std::cerr << "Error: loop start or end is nullptr.\n";
        return;
    }

    auto phi_jj = PhiInst::create_phi(start->get_type(), add_head);
    phi_jj->add_phi_pair_operand(start, add_entry);
    add_head->add_instr_begin(phi_jj);

    builder.set_insert_point(add_head);

    auto outerentry = static_cast<BasicBlock *>(add_head->get_terminator()->get_operand(0));
    if (auto old_br = add_head->get_terminator())
    {
        // BasicBlock* bb=static_cast<BasicBlock*>(old_br->get_operand(0));
        std::list<BasicBlock *> del;
        for (auto succbb : add_head->get_succ_basic_blocks())
        {
            del.push_back(succbb);
        }
        for (auto delbb : del)
        {
            delbb->remove_pre_basic_block(add_head);
            add_head->remove_succ_basic_block(delbb);
        }
        add_head->remove_instr(old_br);
    }

    BasicBlock *endwhile = add_end;

    // auto cmp1 = builder.create_icmp_lt(phi_jj, newend);//jj<end
    // builder.create_cond_br(cmp1,outerentry,endwhile);
    // 对4种step_type
    Instruction *cmp1 = nullptr;
    if (icmp_type == "lt")
        cmp1 = builder.create_icmp_lt(phi_jj, final_newend);
    else if (icmp_type == "le")
        cmp1 = builder.create_icmp_le(phi_jj, final_newend);
    else if (icmp_type == "gt")
        cmp1 = builder.create_icmp_gt(phi_jj, final_newend);
    else if (icmp_type == "ge")
        cmp1 = builder.create_icmp_ge(phi_jj, final_newend);
    else
    {
        std::cerr << "Unsupported icmp_type: " << icmp_type << "\n";
        throw std::runtime_error("Unsupported icmp_type");
    }

    builder.create_cond_br(cmp1, outerentry, endwhile);

    // auto const_Bj = ConstantInt::get(size, m_);

    // 步长*size
    auto conststep = dynamic_cast<ConstantInt *>(step);
    if (!conststep)
    {
        std::cerr << "Error: loop step is not constant.\n";
        return;
    }

    int step_int = conststep->get_value(); // 假设你有 get_value() 方法
    auto const_Bj = ConstantInt::get(size * step_int, m_);

    BasicBlock *retBB;
    if (header->get_pre_basic_blocks().size() != 2)
    {
        std::cerr << "Error: header size not 2.\n";
        return;
    }
    for (auto preBB : header->get_pre_basic_blocks())
    {
        if (preBB != entry)
        {
            retBB = preBB;
        }
    }

    // auto phi_j = PhiInst::create_phi(start->get_type(), header);
    auto icmpinst = static_cast<Instruction *>(header->get_terminator()->get_operand(0));
    auto phi_j = dynamic_cast<PhiInst *>(icmpinst->get_operand(0));
    if (!phi_j)
    {
        std::cerr << "Error: Expected phi as icmp operand.\n";
        return;
    }

    // 只替换来自 entry 的 phi 输入为 phi_jj
    bool replaced = false;
    for (int i = 0; i < phi_j->get_num_operand(); i += 2)
    {
        if (phi_j->get_operand(i + 1) == entry)
        {
            phi_j->replace_phi_operand(phi_j->get_operand(i), phi_jj, entry);
            phi_j->set_operand(i, phi_jj);
            // auto replace = false;
            // for (auto use : phi_jj->get_use_list())
            // {
            //     auto val = use.val_;
            //     if (val == phi_j)
            //         replace = true;
            // }
            // if (!replaced)
            // {
            //     throw(std::runtime_error("no replace phi_j"));
            // }

            replaced = true;
            break;
        }
    }
    if (!replaced)
    {
        std::cerr << "Warning: No matching phi incoming from entry block.\n";
    }

    // phi_j->add_phi_pair_operand(phi_jj, entry);
    // std::cout << "phi_j" << std::endl;
    // std::cout << phi_j << " " << step_value << std::endl;
    // phi_j->add_phi_pair_operand(step_value, retBB);
    // header->add_instr_begin(phi_j);
    builder.set_insert_point(putinst);

    Instruction *j_plus_Bjj = nullptr;
    if (step_type == "add")
        j_plus_Bjj = builder.create_iadd(phi_jj, const_Bj);
    else if (step_type == "sub")
        j_plus_Bjj = builder.create_isub(phi_jj, const_Bj);
    else
    {
        std::cerr << "Unsupported step_type: " << step_type << "\n";
        throw std::runtime_error("Unsupported step_type");
    }

    Instruction *cmp2 = nullptr;
    if (icmp_type == "lt")
        cmp2 = builder.create_icmp_lt(phi_j, j_plus_Bjj);
    else if (icmp_type == "le")
        cmp2 = builder.create_icmp_le(phi_j, j_plus_Bjj);
    else if (icmp_type == "gt")
        cmp2 = builder.create_icmp_gt(phi_j, j_plus_Bjj);
    else if (icmp_type == "ge")
        cmp2 = builder.create_icmp_ge(phi_j, j_plus_Bjj);
    else
    {
        std::cerr << "Unsupported icmp_type: " << icmp_type << "\n";
        throw std::runtime_error("Unsupported icmp_type");
    }
    to_move.push_back(j_plus_Bjj);
    move_instrs(entry, putinst, to_move, entry->get_terminator());
    to_move.clear();

    // 关于j<end,原来的phi要删,icmp要改
    auto phiinst = static_cast<Instruction *>(icmpinst->get_operand(0));
    // std::cout << phi_j << " " << step_value << std::endl;
    // auto cmp3 = builder.create_icmp_iiand(cmp2, icmpinst);
    auto cmp3 = builder.create_and(cmp2, icmpinst);

    // auto endcmp=builder.create_icmp_eq(cmp3,one);

    to_move.push_back(cmp2);
    to_move.push_back(cmp3);
    header->get_terminator()->set_operand(0, cmp3);
    move_instrs(header, putinst, to_move, header->get_terminator());
    to_move.clear();

    Instruction *jj_plus_Bjj = nullptr;
    if (step_type == "add")
        jj_plus_Bjj = builder.create_iadd(phi_jj, const_Bj);
    else if (step_type == "sub")
        jj_plus_Bjj = builder.create_isub(phi_jj, const_Bj);

    to_move.push_back(jj_plus_Bjj);
    move_instrs(add_body, putinst, to_move, add_body->get_terminator());
    to_move.clear();

    //[jj+B,add_body]
    phi_jj->add_phi_pair_operand(jj_plus_Bjj, add_body);
    // std::cout << phi_j << " " << step_value << std::endl;

    // entry问题

    // entry指令全部copy到add_entry,改到前面去了
    // 如果被外面使用，在add_head新建phi[copy,add_entry][origin,body]，然后使用replace_if
    // 也不对，应该还是找head的phi;说明entry赋值。里面也赋值，还被外部引用需要加phi,需要注意循环数的phi可以跳过
    // 就检查
    // 对entry中使用循环变量的，要进行处理,j换成jj

    // for (auto &inst : entry->get_instructions())
    // {
    //     std::unordered_map<Instruction *, bool> replace_outer;
    //     bool outeruse = false;
    //     for (auto use : inst.get_use_list())
    //     {
    //         if (auto useinst = dynamic_cast<Instruction *>(use.val_))
    //         {
    //             if (loop->count(useinst->get_parent())||useinst->get_parent()==entry)
    //             {

    //                 replace_outer[useinst] = false;
    //             }
    //             else
    //             {
    //                 std::cout<<"true"<<std::endl;
    //                 outeruse = true;
    //                 replace_outer[useinst] = true;
    //             }
    //         }
    //     }
    //     if (!outeruse)
    //         continue;
    //     // std::cout<<"phi"<<std::endl;
    //     auto phi = PhiInst::create_phi(inst.get_type(), add_head);
    //     add_head->add_instr_begin(phi);
    //     phi->add_phi_pair_operand(&inst, add_body);
    //     phi->add_phi_pair_operand(inst_map[&inst], add_entry);
    //     inst.replace_use_with_if(phi, [&](Use use)
    //                              {
    //         if (auto userInst = dynamic_cast<Instruction *>(use.val_)) {
    //             auto it = replace_outer.find(userInst);
    //             return it != replace_outer.end() && it->second;
    //         }
    //         return false; });
    // }

    // std::vector<PhiInst *> origin_phis;
    // for (auto &inst : header->get_instructions())
    // {
    //     if (auto phi = dynamic_cast<PhiInst *>(&inst))
    //     {
    //         origin_phis.push_back(phi);
    //     }
    // }

    // for (auto originphi : origin_phis)
    // {
    //     std::cout << originphi << " " << step_value << std::endl;
    //     // 循环数跳过，直接换成对phi_jj引用就可以了
    //     if (originphi == step_value)
    //         continue;
    //     Value *phiformentry = nullptr;
    //     for (int i = 0; i < originphi->get_num_operand(); i += 2)
    //     {
    //         if (originphi->get_operand(i + 1) == entry)
    //         {
    //             phiformentry = originphi->get_operand(i);
    //         }
    //     }
    //     if (!phiformentry)
    //         throw(std::runtime_error("phi no value from entry"));
    //     // if (!inst_map.count(phiformentry))//后来插的，不动
    //     //     continue;
    //     std::unordered_map<Instruction *, bool> replace_outer;
    //     bool outeruse = false;
    //     for (auto use : originphi->get_use_list())
    //     {
    //         if (auto useinst = dynamic_cast<Instruction *>(use.val_))
    //         {
    //             // 应该是在整体循环的外面
    //             auto wholeloop = loopset[0];
    //             if (wholeloop->count(useinst->get_parent()))
    //             {

    //                 replace_outer[useinst] = false;
    //             }
    //             else
    //             {
    //                 std::cout << "true" << std::endl;
    //                 outeruse = true;
    //                 replace_outer[useinst] = true;
    //             }
    //         }
    //     }
    //     if (!outeruse)
    //         continue;
    //     // std::cout<<"phi"<<std::endl;
    //     auto phi = PhiInst::create_phi(originphi->get_type(), add_head);
    //     add_head->add_instr_begin(phi);

    //     //说明entry是来自循环外？
    //     if (!inst_map.count(phiformentry))
    //     {
    //         if(loopset[0]->count(static_cast<Instruction*>(phiformentry)->get_parent())){
    //             throw(std::runtime_error("no form entry and out,insert?"));
    //         }
    //         phi->add_phi_pair_operand(phiformentry, add_entry);
    //     }
    //     else
    //     {
    //         phi->add_phi_pair_operand(inst_map[phiformentry], add_entry);
    //     }

    //     //如果origin_phi所在的块不是直接前驱，就需要转发
    //     // if(originphi->get_parent()!=add_body){
    //     //     auto tempphi=PhiInst::create_phi(originphi->get_type(), add_body);
    //     //     add_body->add_instr_begin(tempphi);
    //     //     auto bodyfrom=add_body->get_pre_basic_blocks().front();
    //     //     tempphi->add_phi_pair_operand(originphi,bodyfrom);
    //     //     phi->add_phi_pair_operand(tempphi, add_body);
    //     // }
    //     // else{
    //         phi->add_phi_pair_operand(originphi, add_body);

    //     // }
    //     replace_outer[phi] = false;
    //     originphi->replace_use_with_if(phi, [&](Use use)
    //                                    {
    //         if (auto userInst = dynamic_cast<Instruction *>(use.val_)) {
    //             auto it = replace_outer.find(userInst);
    //             return it != replace_outer.end() && it->second;
    //         }
    //         return false; });

    //     //来源变为phi
    //     for (int i = 0; i < originphi->get_num_operand(); i += 2)
    //     {
    //         if (originphi->get_operand(i + 1) == entry)
    //         {
    //             originphi->set_operand(i,phi);
    //         }
    //     }
    // }
    // std::cout << "endheader" << std::endl;
    // m_->set_print_name();
    // entry phi问题，在在外层时处理
    if (loopset[0] == loop)
    {
        for (auto &inst : header->get_instructions())
        {
            if (auto originphi = dynamic_cast<PhiInst *>(&inst))
            {
                std::unordered_map<Instruction *, bool> replace_outer;
                bool outeruse = false;
                for (auto use : originphi->get_use_list())
                {
                    if (auto useinst = dynamic_cast<Instruction *>(use.val_))
                    {
                        if (loop->count(useinst->get_parent()))
                        {
                            // std::cout<<"false"<<std::endl;
                            replace_outer[useinst] = false;
                        }
                        else
                        {
                            outeruse = true;
                            replace_outer[useinst] = true;
                        }
                    }
                }
                if (!outeruse)
                    continue;
                // std::cout<<"phi"<<std::endl;
                auto phi = PhiInst::create_phi(originphi->get_type(), putinst);
                putinst->add_instr_begin(phi);
                to_move.push_back(phi);
                Instruction *phiend;
                for (auto &inst : add_head->get_instructions())
                {
                    if (dynamic_cast<PhiInst *>(&inst))
                    {
                        continue;
                    }
                    phiend = &inst;
                    break;
                }
                move_instrs(add_head, putinst, to_move, phiend);
                to_move.clear();

                // add_head->add_instr_begin(phi);
                Value *entryval = nullptr;
                for (int i = 0; i < originphi->get_num_operand(); i += 2)
                {
                    if (originphi->get_operand(i + 1) == entry)
                    {
                        entryval = originphi->get_operand(i);
                        break;
                    }
                }
                if (entryval == nullptr)
                {
                    throw(std::runtime_error("header phi error"));
                }
                phi->add_phi_pair_operand(entryval, add_entry);
                phi->add_phi_pair_operand(originphi, add_body);
                originphi->replace_use_with_if(phi, [&](Use use)
                                               {
                if (auto inst = dynamic_cast<Instruction*>(use.val_)) {
                    auto it = replace_outer.find(inst);
                    return it != replace_outer.end() && it->second;
                }
                return false; });
                // for (int i = 0; i < originphi->get_num_operand(); i += 2)
                // {
                //     if (originphi->get_operand(i + 1) == entry)
                //     {
                //         originphi->set_operand(i, phi);
                //     }
                // }
                std::vector<BasicBlock *> entrys;
                std::vector<BasicBlock *> headers;
                std::vector<BasicBlock *> bodys;
                std::vector<PhiInst *> phis;
                BasicBlock *newhead = add_head;
                for (int i = 0; i < loopset.size() - 1; i++)
                {
                    PhiInst *newphi;
                    auto br = newhead->get_terminator();
                    auto entry = static_cast<BasicBlock *>(br->get_operand(1)); // true后entry
                    auto head = entry->get_succ_basic_blocks().front();         // 只有一个后继
                    BasicBlock *body = nullptr;
                    for (auto bb : head->get_pre_basic_blocks())
                    {
                        if (bb == entry)
                            continue;
                        body = bb;
                    }
                    if (!body)
                        throw std::runtime_error("cannot find body block");
                    newphi = PhiInst::create_phi(phi->get_type(), head);
                    head->add_instr_begin(newphi);
                    phis.push_back(newphi);
                    entrys.push_back(entry);
                    headers.push_back(head);
                    bodys.push_back(body);
                    newhead = head;
                }
                if (phis.size() != 0)
                {
                    phis.push_back(originphi); // phis[i+1]
                    // 插phi
                    phis[0]->add_phi_pair_operand(phi, entrys[0]);
                    phis[0]->add_phi_pair_operand(phis[1], bodys[0]);
                    for (int i = 1; i + 1 < phis.size(); i++)
                    {
                        phis[i]->add_phi_pair_operand(phis[i - 1], entrys[i]);
                        phis[i]->add_phi_pair_operand(phis[i + 1], bodys[i]);
                    }
                    for (int i = 0; i < originphi->get_num_operand(); i += 2)
                    {
                        if (originphi->get_operand(i + 1) == entry)
                        {
                            originphi->replace_phi_operand(originphi->get_operand(i), phis[phis.size() - 2], entry);
                            originphi->set_operand(i, phis[phis.size() - 2]);
                        }
                    }
                    for (int i = 0; i < phi->get_num_operand(); i += 2)
                    {
                        if (phi->get_operand(i + 1) == add_body)
                        {
                            phi->replace_phi_operand(phi->get_operand(i), phis.front(), add_body);
                            phi->set_operand(i, phis.front());
                            break;
                        }
                    }
                }

                // 特殊情况之循环变量,由于已经被jj给替换了，需要再替换回start
                // 好像只有ii会遇到
                if (originphi == step_value)
                {
                    phi->replace_phi_operand(phi->get_operand(0), start, static_cast<BasicBlock *>(phi->get_operand(1)));
                    phi->set_operand(0, start);
                }

                // phi的来源如果是entry,需要使用copy后结果。移动后就没问题。
                // if (auto inst = dynamic_cast<Instruction *>(phi->get_operand(0)))
                // {
                //     if (inst->get_parent() == entry)
                //     {
                //         phi->set_operand(0, inst_map[phi->get_operand(0)]);
                //     }
                // }
            }
            if (inst.isTerminator())
                break;
        }
    }
    if (loopset[0] == loop)
    {
        std::vector<Value *> values;
        std::vector<BasicBlock *> entrys;
        BasicBlock *newhead = add_head;
        std::vector<Value *> phi_jjs;
        for (int i = 0; i < loopset.size() - 1; i++)
        {
            auto [start, end, step, step_value, step_type, icmp_type] = loopinfo.at(loopset[i]);
            values.push_back(step_value);
        }
        // 最外层的entry不会用到循环变量
        //  entrys.push_back(add_entry);
        phi_jjs.push_back(phi_jj);
        for (int i = 0; i < loopset.size() - 1; i++)
        {
            auto br = newhead->get_terminator();
            auto new_entry = static_cast<BasicBlock *>(br->get_operand(1)); // true后entry
            auto head = entry->get_succ_basic_blocks().front();
            auto head_cmp = static_cast<Instruction *>(head->get_terminator()->get_operand(0));
            auto new_phi_jj = head_cmp->get_operand(0);
            phi_jjs.push_back(new_phi_jj);
            entrys.push_back(new_entry);
            newhead = head;
        }
        // 第i层会用到所有层的循环变量，比如k=i
        // 已对齐,entrys:1->n,phi_jjs:0->n-1
        for (auto new_entry : entrys)
        {
            // std::cout << new_entry->print() << std::endl;
            for (auto &inst : new_entry->get_instructions())
            {
                for (int j = 0; j < inst.get_num_operand(); j++)
                {
                    auto op = inst.get_operand(j);
                    for (int i = 0; i < values.size(); i++)
                    {
                        if (op == values[i])
                        {
                            // std::cout << inst.print() << std::endl;
                            inst.set_operand(j, phi_jjs[i]);
                            break;
                        }
                    }
                }
            }
        }
    }

    putinst->get_parent()->remove(putinst);
}
// Value *resolve_loop_end(Value *end, std::vector<BBset_t *> loopset, LoopInfo &loopinfo, BBset_t *self_loop)
// {
//     while (true)
//     {
//         bool found = false;

//         for (auto *loop : loopset)
//         {
//             if (loop == self_loop)
//                 continue;

//             auto [start, cur_end, step, step_value, step_type, icmp_type] = loopinfo[loop];

//             if (end == step_value)
//             {
//                 end = cur_end;
//                 found = true;
//                 break;
//             }
//         }

//         if (!found)
//             break;
//     }

//     return end;
// }

Value *resolve_loop_start(Value *start, const std::vector<BBset_t *> &loopset, LoopInfo &loopinfo, BBset_t *self_loop)
{
    for (auto *loop : loopset)
    {
        if (loop == self_loop)
            continue;

        auto [cur_start, end, step, step_value, step_type, icmp_type] = loopinfo[loop];

        if (start == step_value)
        {
            if (auto *phi = dynamic_cast<PhiInst *>(cur_start))
            {
                // std::cout << "replace" << std::endl;
                return phi->get_operand(0);
            }
            else
            {
                std::cerr << "Error: cur_start is not a PhiInst when resolving loop start.\n";
                std::abort();
            }
        }
    }
    return start;
}

bool matchnoloop(std::string str)
{
    if (str == "usedbychild")
    {
        return true;
    }
    if (str == "noloop")
    {
        return true;
    }
    if (str == "noused")
    {
        return true;
    }
    return false;
}
